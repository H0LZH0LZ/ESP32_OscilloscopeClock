/******************************************************************************

  ESP32 Oscilloscope Clock v2.0
  X/Y-Vektoruhr über die internen DACs, NTP-Zeit, Status-OLED

  Basiert auf: ESP32 Oscilloscope Clock v1.0
               Mauro Pintus, Milano 2018/05/25
               https://github.com/maurohh/ESP32_OscilloscopeClock
               (Linien-/Tabellenlogik und DataTable.h aus v1.0 übernommen)

  ---------------------------------------------------------------------------
  Funktionsprinzip
  ---------------------------------------------------------------------------
  Ein Oszilloskop im X/Y-Modus lenkt den Elektronenstrahl horizontal mit der
  Spannung an Kanal 1 (X) und vertikal mit der Spannung an Kanal 2 (Y) ab.
  Der ESP32 erzeugt beide Spannungen mit seinen zwei 8-Bit-DACs
  (GPIO25 = X, GPIO26 = Y, je 0…255 ≙ ca. 0…3,2 V) und "fährt" damit das
  Zifferblatt und die Zeiger Punkt für Punkt ab. Weil das Bild auf dem
  Schirm nur kurz nachleuchtet, muss es ohne Pause immer wieder neu
  gezeichnet werden – sonst flimmert es oder es entstehen helle Punkte.

  ---------------------------------------------------------------------------
  Aufgabenteilung auf die beiden Kerne (FreeRTOS)
  ---------------------------------------------------------------------------
    Core 1 │ scopeTask  │ zeichnet endlos Zifferblatt + Zeiger über die DACs
    ───────┼────────────┼────────────────────────────────────────────────────
    Core 0 │ uiTask     │ Uhrzeit berechnen + an Core 1 übergeben (alle 10 ms),
           │            │ OLED aktualisieren (alle 200 ms), Konsolen-Log (1 s)
           │ wifiTask   │ WLAN-Verbindung aufbauen/halten (WiFiMulti)
           │ (System)   │ WLAN-Stack, SNTP (NTP-Client des ESP-IDF)

  Zwischen den Kernen wird nur ein einziges 32-Bit-Wort ausgetauscht
  (g_timePacked = Stunde/Minute/Sekunde). Der Zeichenpfad auf Core 1 liest
  es einmal pro Bild und muss nie auf Core 0 warten.

  ---------------------------------------------------------------------------
  Dateien
  ---------------------------------------------------------------------------
    ESP32_OscilloscopeClock_v2.0.ino  dieser Sketch
    config.h                          Pins, Zeitzone, NTP, OLED, DAC-Timing
    secrets.h                         WLAN-Liste (lokal, nicht im Git)
    secrets_example.h                 Vorlage für secrets.h
    DataTable.h                       Vektordaten: Zifferblatt, Ziffern, Zeiger
    ../HARDWARE.md                    Verdrahtung, Gehäuse, Oszi-Einstellungen
    ../SOFTWARE.md                    Aufbau der Software, Tabellenformat, Tuning

  Benötigte Bibliothek (Arduino-Bibliotheksverwalter):  U8g2 by oliver
  Board-Paket: "esp32" by Espressif (2.0.x oder 3.x), Board "ESP32 Dev Module"

******************************************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>        // mehrere WLANs, verbindet mit dem stärksten
#include <Wire.h>             // I2C (wird von U8g2 benutzt)
#include <time.h>             // time(), localtime_r(), struct tm
#include <esp_sntp.h>         // NTP-Client des ESP-IDF (Sync-Callback, Intervall)
#include <soc/soc.h>          // SET_PERI_REG_BITS()
#include <soc/rtc_io_reg.h>   // Registeradressen der DAC-Pads
#include <U8g2lib.h>          // OLED-Treiber

#include "config.h"

//*****************************************************************************
// WLAN-Zugangsdaten
//*****************************************************************************
// Ein WLAN-Eintrag. secrets.h definiert daraus das Array WIFI_NETWORKS[],
// deshalb muss die Struktur VOR dem #include von secrets.h stehen.
struct WifiCredential {
  const char *ssid;       // Netzwerkname
  const char *password;   // Passwort (wird nie ausgegeben)
};

// secrets.h ist optional: fehlt sie, wird die (leere) Vorlage verwendet und
// die Uhr läuft offline. __has_include prüft das zur Übersetzungszeit.
#if __has_include("secrets.h")
  #include "secrets.h"
#else
  #include "secrets_example.h"
#endif

// Kompatibilität mit dem alten secrets.h-Format (nur ein Netz):
//   #define WIFI_SSID     "..."
//   #define WIFI_PASSWORD "..."
#if defined(WIFI_SSID) && !defined(WIFI_NETWORKS_DEFINED)
static const WifiCredential WIFI_NETWORKS[] = { { WIFI_SSID, WIFI_PASSWORD } };
#endif
static const size_t WIFI_NETWORK_COUNT = sizeof(WIFI_NETWORKS) / sizeof(WIFI_NETWORKS[0]);

#include "DataTable.h"

// Nur der klassische ESP32 hat DACs an GPIO25/26 (S2 hat andere Pins und nur
// einen Kern; S3/C3/C6 haben gar keine DACs).
#if !defined(CONFIG_IDF_TARGET_ESP32)
  #error "Dieses Projekt benötigt einen klassischen ESP32 (mit DAC an GPIO25/26). Board: 'ESP32 Dev Module'."
#endif

#define FW_VERSION "2.0"

//*****************************************************************************
// Gemeinsamer Zustand zwischen den Tasks / Kernen
//*****************************************************************************

// Aktuelle Uhrzeit als ein 32-Bit-Wort:
//   Bits 23..16 = Stunde (0-23), 15..8 = Minute (0-59), 7..0 = Sekunde (0-59)
// Schreiber: uiTask (Core 0), Leser: scopeTask (Core 1).
// Ausgerichtete 32-Bit-Zugriffe sind auf dem ESP32 atomar, also kann der Leser
// nie eine "halbe" Uhrzeit sehen – ganz ohne Mutex, der Core 1 bremsen würde.
static volatile uint32_t g_timePacked = 0;

static inline uint32_t packTime(int h, int m, int s) {
  return ((uint32_t)h << 16) | ((uint32_t)m << 8) | (uint32_t)s;
}

// NTP-Status. Schreiber: SNTP-Callback (läuft im lwIP-Task auf Core 0),
// Leser: uiTask. time_t ist auf aktuellen ESP32-Cores 64 Bit breit und damit
// NICHT atomar – daher der Spinlock für den Zeitstempel.
static volatile bool g_ntpSynced     = false;  // true nach dem ersten erfolgreichen Sync
static time_t        g_lastSyncEpoch = 0;      // Unix-Zeit der letzten Synchronisierung
static portMUX_TYPE  g_syncMux       = portMUX_INITIALIZER_UNLOCKED;

static time_t lastSyncEpoch()
{
  portENTER_CRITICAL(&g_syncMux);
  const time_t e = g_lastSyncEpoch;
  portEXIT_CRITICAL(&g_syncMux);
  return e;
}

static TaskHandle_t scopeTaskHandle = nullptr;
static TaskHandle_t uiTaskHandle    = nullptr;
static TaskHandle_t wifiTaskHandle  = nullptr;
static WiFiMulti    wifiMulti;

//*****************************************************************************
// DAC-Ausgabe (Core 1)
//*****************************************************************************
// Alle Funktionen im Zeichenpfad tragen IRAM_ATTR: Sie liegen im internen RAM
// und laufen auch dann weiter, wenn der Flash-Cache kurz gesperrt ist (z.B.
// während der WLAN-Stack Daten ins NVS schreibt). Sonst würde das Bild stocken.

// Zuletzt ausgegebene Werte – unveränderte Achsen werden nicht neu geschrieben.
static int lastx = -1, lasty = -1;

// Schreibt einen 8-Bit-Wert auf den X-DAC (GPIO25).
// USE_FAST_DAC: direkter Registerzugriff, nur wenige CPU-Takte.
// Sonst: Arduino-dacWrite(), deutlich langsamer (Treiberaufruf).
static inline void IRAM_ATTR dacOutX(uint8_t v) {
#if USE_FAST_DAC
  SET_PERI_REG_BITS(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_DAC, v, RTC_IO_PDAC1_DAC_S);
#else
  dacWrite(DAC_X_PIN, v);
#endif
}

// Schreibt einen 8-Bit-Wert auf den Y-DAC (GPIO26).
static inline void IRAM_ATTR dacOutY(uint8_t v) {
#if USE_FAST_DAC
  SET_PERI_REG_BITS(RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_DAC, v, RTC_IO_PDAC2_DAC_S);
#else
  dacWrite(DAC_Y_PIN, v);
#endif
}

// Schaltet beide DACs ein. dacWrite() übernimmt die komplette Pad-Konfiguration
// (Power-up, Kosinusgenerator aus) für ESP-IDF 4 und 5; danach schreiben
// dacOutX/Y nur noch den Ausgangswert ins Register. Startwert = Bildmitte.
static void dacInit() {
  dacWrite(DAC_X_PIN, 0x80);
  dacWrite(DAC_Y_PIN, 0x80);
}

//*****************************************************************************
// Dot – setzt den Strahl auf einen Punkt (x, y), je 0…255
//*****************************************************************************
static inline void IRAM_ATTR Dot(int x, int y)
{
  if (lastx != x) {
    lastx = x;
    dacOutX((uint8_t)x);
  }
  if (lasty != y) {
    lasty = y;
    dacOutY((uint8_t)y);
  }
#if DOT_DELAY_CYCLES > 0
  // Optionale Verweildauer pro Punkt, damit der DAC-Ausgang (inkl. Kabel-
  // kapazität) einschwingen kann. Aktives Warten auf den CPU-Taktzähler,
  // da vTaskDelay() viel zu grob (1 ms) wäre.
  uint32_t t0 = ESP.getCycleCount();
  while ((uint32_t)(ESP.getCycleCount() - t0) < (uint32_t)DOT_DELAY_CYCLES) { }
#endif
#if defined EXCEL
  // Diagnosemodus: jeden Punkt seriell ausgeben (siehe config.h)
  Serial.printf("0x%02X,0x%02X,\n", x & 0xFF, y & 0xFF);
#endif
}

//*****************************************************************************
// Line – zeichnet eine Strecke von (x1,y1) nach (x2,y2)
//*****************************************************************************
// Bresenham-/DDA-Verfahren aus v1.0: Der Strahl wird in 1er-Schritten entlang
// der längeren Achse bewegt; der Akkumulator "acc" entscheidet, wann die
// kürzere Achse einen Schritt nachzieht. Für Geschwindigkeit gibt es für jeden
// der 8 Oktanten (4 Quadranten × flach/steil) eine eigene Schleife, so dass
// in der inneren Schleife keine Vorzeichen-Fallunterscheidungen nötig sind.
// Jeder Schritt erzeugt einen Dot() – die Helligkeit einer Linie hängt also
// von ihrer Länge in Punkten und von DOT_DELAY_CYCLES ab.
static void IRAM_ATTR Line(byte x1, byte y1, byte x2, byte y2)
{
    int acc;
    if (y1 < y2) { // Quadrant 1 oder 2 (nach oben)
        byte dy = y2 - y1;
        if (x1 < x2) { // Quadrant 1 (nach rechts oben)
            byte dx = x2 - x1;
            if (dx > dy) { // flach (< 45°): x ist die schnelle Achse
                acc = (dx >> 1);
                for (; x1 <= x2; x1++) {
                    Dot(x1, y1);
                    acc -= dy;
                    if (acc < 0) { y1++; acc += dx; }
                }
            }
            else {   // steil (> 45°): y ist die schnelle Achse
                acc = dy >> 1;
                for (; y1 <= y2; y1++) {
                    Dot(x1, y1);
                    acc -= dx;
                    if (acc < 0) { x1++; acc += dy; }
                }
            }
        }
        else {  // Quadrant 2 (nach links oben)
            byte dx = x1 - x2;
            if (dx > dy) { // flach
                acc = dx >> 1;
                for (; x1 >= x2; x1--) {
                    Dot(x1, y1);
                    acc -= dy;
                    if (acc < 0) { y1++; acc += dx; }
                }
            }
            else {  // steil
                acc = dy >> 1;
                for (; y1 <= y2; y1++) {
                    Dot(x1, y1);
                    acc -= dx;
                    if (acc < 0) { x1--; acc += dy; }
                }
            }
        }
    }
    else { // Quadrant 3 oder 4 (nach unten bzw. waagerecht)
        byte dy = y1 - y2;
        if (x1 < x2) { // Quadrant 4 (nach rechts unten)
            byte dx = x2 - x1;
            if (dx > dy) {  // flach
                acc = dx >> 1;
                for (; x1 <= x2; x1++) {
                    Dot(x1, y1);
                    acc -= dy;
                    if (acc < 0) { y1--; acc += dx; }
                }
            }
            else {  // steil
                acc = dy >> 1;
                for (; y1 >= y2; y1--) {
                    Dot(x1, y1);
                    acc -= dx;
                    if (acc < 0) { x1++; acc += dy; }
                }
            }
        }
        else {  // Quadrant 3 (nach links unten)
            byte dx = x1 - x2;
            if (dx > dy) { // flach
                acc = dx >> 1;
                for (; x1 >= x2; x1--) {
                    Dot(x1, y1);
                    acc -= dy;
                    if (acc < 0) { y1--; acc += dx; }
                }
            }
            else {  // steil
                acc = dy >> 1;
                for (; y1 >= y2; y1--) {
                    Dot(x1, y1);
                    acc -= dx;
                    if (acc < 0) { x1--; acc += dy; }
                }
            }
        }
    }
}

//*****************************************************************************
// PlotTable – zeichnet einen Polygonzug aus einer Tabelle in DataTable.h
//*****************************************************************************
// Tabellenformat: Folge von Punkten (x, y), je 1 Byte. Aufeinanderfolgende
// Punkte werden mit Linien verbunden.
//
//   SubTable      Tabelle (Bytes)
//   SubTableSize  Länge der Tabelle in Bytes (sizeof)
//   skip          Trennwert zwischen zwei Polygonzügen:
//                   0x00 bei Zifferblatt/Ziffern  (x=0 kommt als Koordinate
//                        deshalb dort nicht vor),
//                   0xFF bei den Zeigern
//   opt           0 = normal
//                 1 = nach einem Trenner einen evtl. zweiten Trenner
//                     überspringen (für das Format der Ziffern-Tabellen)
//                 2 = jede Linie hin UND zurück zeichnen (Strahl endet wieder
//                     am Startpunkt, vermeidet sichtbare Sprunglinien;
//                     im Original für Tests verwendet)
//   offset        Start-Index in Bytes. Bei den Zeigertabellen wählt er die
//                 Zeigerstellung aus (siehe scopeTask).
//
// Ende: sobald die x-Koordinate des NÄCHSTEN Punktes 0xFF ist.
// Hinweis: Zwischen zwei Polygonzügen springt der Strahl ohne Dunkeltastung
// (der ESP32 hat keinen Z-Ausgang) – der Sprung ist schnell und daher kaum
// sichtbar.
static void IRAM_ATTR PlotTable(byte *SubTable, int SubTableSize, int skip, int opt, int offset)
{
  int i = offset;
  while (i < SubTableSize) {
    if (SubTable[i+2] == skip) {                 // nächster Eintrag ist ein Trenner:
      i = i + 3;                                 //   aktuellen Punkt + Trenner überspringen
      if (opt == 1) if (SubTable[i] == skip) i++;
    }
    Line(SubTable[i], SubTable[i+1], SubTable[i+2], SubTable[i+3]);
    if (opt == 2) {
      Line(SubTable[i+2], SubTable[i+3], SubTable[i], SubTable[i+1]);
    }
    i = i + 2;                                   // nächster Punkt
    if (SubTable[i+2] == 0xFF) break;            // Ende des Polygonzugs/Zeigers
  }
}

//*****************************************************************************
// scopeTask – Core 1: zeichnet endlos Zifferblatt + Zeiger
//*****************************************************************************
// Ein komplettes Bild besteht aus: Ring mit Minutenstrichen, Ziffern,
// Stunden-, Minuten- und Sekundenzeiger. Danach beginnt sofort das nächste.
//
// Zeigertabellen (DataTable.h) enthalten je 60 vorberechnete Stellungen:
//   HrPtrData, MinPtrData: 60 × 9 Byte  (4 Punkte = schmale Raute + 0xFF)
//   SecPtrData:            60 × 5 Byte  (2 Punkte = Linie + 0xFF)
// Der Offset "Stellung × Eintragslänge" wählt die passende Stellung aus.
static void scopeTask(void *)
{
  // Dieser Task gibt Core 1 nie frei (kein vTaskDelay). Der Idle-Task von
  // Core 1 kommt daher nie dran – falls dessen Watchdog in der verwendeten
  // Core-Version aktiv ist, würde er auslösen. Deshalb hier abschalten.
  disableCore1WDT();
  dacInit();
  Serial.printf("[Scope] laeuft auf Core %d\n", xPortGetCoreID());

  for (;;) {
    // Uhrzeit einmal pro Bild atomar lesen, damit alle drei Zeiger
    // garantiert zur selben Sekunde gehören.
    const uint32_t t = g_timePacked;
    const int h = (t >> 16) & 0xFF;
    const int m = (t >> 8)  & 0xFF;
    const int s =  t        & 0xFF;

    // Stundenzeiger in 60 Schritten: 5 Schritte pro Stunde, plus 1 Schritt
    // je 12 Minuten – er wandert also wie bei einer echten Uhr mit.
    const int hpos = (h % 12) * 5 + m / 12;

    PlotTable(DialData, sizeof(DialData), 0x00, 1, 0);              // Ring + Striche
#if DIAL_STYLE == 1
    PlotTable(DialDigits24, sizeof(DialDigits24), 0x00, 1, 0);      // Ziffern 24h
#elif DIAL_STYLE == 2
    PlotTable(DialDigitsRoman, sizeof(DialDigitsRoman), 0x00, 1, 0);// röm. Ziffern
#else
    PlotTable(DialDigits12, sizeof(DialDigits12), 0x00, 1, 0);      // Ziffern 1-12
#endif
    PlotTable(HrPtrData,  sizeof(HrPtrData),  0xFF, 0, 9 * hpos);   // Stundenzeiger
    PlotTable(MinPtrData, sizeof(MinPtrData), 0xFF, 0, 9 * m);      // Minutenzeiger
    PlotTable(SecPtrData, sizeof(SecPtrData), 0xFF, 0, 5 * s);      // Sekundenzeiger

#if defined EXCEL
    // Diagnosemodus: nach einem Bild anhalten (alle Punkte wurden ausgegeben)
    Serial.println("EXCEL-Ausgabe beendet.");
    vTaskSuspend(nullptr);
#endif
    // Bewusst kein vTaskDelay(): jede Pause wäre als heller Punkt sichtbar.
  }
}

//*****************************************************************************
// Zeit (Core 0)
//*****************************************************************************

// Wird vom SNTP-Client nach JEDER erfolgreichen Synchronisierung aufgerufen
// (erste nach dem Verbinden, danach alle NTP_SYNC_INTERVAL_MS). Läuft im
// lwIP-Task – daher hier nur Zustand merken, Ausgabe macht wifiLogChanges().
static void onNtpSync(struct timeval *tv)
{
  const time_t e = tv ? tv->tv_sec : time(nullptr);
  portENTER_CRITICAL(&g_syncMux);
  g_lastSyncEpoch = e;
  portEXIT_CRITICAL(&g_syncMux);
  g_ntpSynced = true;
}

// Liefert die aktuelle lokale Zeit (Zeitzone/Sommerzeit aus TZ_INFO).
// Rückgabe true  = echte Zeit nach NTP-Sync (inkl. Datum),
//         false = noch kein Sync: Zeit läuft ab FALLBACK_H:M:S seit Einschalten,
//                 Datumsfelder sind 0 und dürfen nicht angezeigt werden.
// Nach dem ersten Sync läuft die Systemuhr des ESP32 selbstständig weiter,
// auch wenn das WLAN später wegfällt.
static bool currentTime(struct tm &out)
{
  if (g_ntpSynced) {
    time_t now = time(nullptr);
    localtime_r(&now, &out);
    return true;
  }
  static const uint32_t base = FALLBACK_H * 3600UL + FALLBACK_M * 60UL + FALLBACK_S;
  uint32_t sec = base + millis() / 1000UL;
  memset(&out, 0, sizeof(out));
  out.tm_hour = (sec / 3600UL) % 24;
  out.tm_min  = (sec / 60UL) % 60;
  out.tm_sec  =  sec % 60;
  out.tm_wday = -1;                         // Markierung: Datum unbekannt
  return false;
}

// Berechnet die aktuelle Zeit und übergibt sie an den scopeTask (Core 1).
static void publishTime()
{
  struct tm t;
  currentTime(t);
  g_timePacked = packTime(t.tm_hour, t.tm_min, t.tm_sec);
}

//*****************************************************************************
// WLAN (Core 0)
//*****************************************************************************

// true, sobald in secrets.h mindestens ein Netz mit nicht-leerer SSID steht.
static bool wifiConfigured()
{
  for (size_t i = 0; i < WIFI_NETWORK_COUNT; i++)
    if (WIFI_NETWORKS[i].ssid && WIFI_NETWORKS[i].ssid[0]) return true;
  return false;
}

// Hält die WLAN-Verbindung. Läuft als eigener Task, weil WiFiMulti::run()
// zuerst einen Scan macht und dabei mehrere Sekunden blockiert – im uiTask
// würde das OLED so lange stehen bleiben.
// Ablauf: nicht verbunden → alle Netze scannen → mit dem stärksten bekannten
// verbinden (max. 10 s) → alle 2 s Verbindung prüfen.
static void wifiTask(void *)
{
  for (;;) {
    if (WiFi.status() != WL_CONNECTED) {
      wifiMulti.run(10000);
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

// Initialisiert WLAN und NTP. Muss vom uiTask (Core 0) aus aufgerufen werden:
// Der WLAN-Treiber legt seine Interrupts auf dem aufrufenden Kern an, und sie
// sollen den Zeichenpfad auf Core 1 nicht unterbrechen.
static void wifiStart()
{
  if (!wifiConfigured()) {
    Serial.println("[WiFi] keine Zugangsdaten (secrets.h) – Offline-Betrieb");
    WiFi.mode(WIFI_OFF);
    return;
  }
  WiFi.mode(WIFI_STA);                 // nur Client, kein eigener Access Point
  WiFi.setHostname(WIFI_HOSTNAME);     // Name in der Geräteliste des Routers

  // Alle eingetragenen Netze bei WiFiMulti anmelden (leere Einträge ignorieren)
  for (size_t i = 0; i < WIFI_NETWORK_COUNT; i++) {
    const WifiCredential &n = WIFI_NETWORKS[i];
    if (!n.ssid || !n.ssid[0]) continue;
    wifiMulti.addAP(n.ssid, n.password);
    Serial.printf("[WiFi] bekanntes Netz: \"%s\"\n", n.ssid);   // Passwort wird nie ausgegeben
  }

  // NTP: configTzTime() setzt die Zeitzone (inkl. Sommerzeitregel) und startet
  // den SNTP-Client im Hintergrund. Er wartet selbstständig, bis das WLAN steht,
  // und synchronisiert danach alle NTP_SYNC_INTERVAL_MS erneut.
  sntp_set_time_sync_notification_cb(onNtpSync);
  sntp_set_sync_interval(NTP_SYNC_INTERVAL_MS);
  configTzTime(TZ_INFO, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);

  xTaskCreatePinnedToCore(wifiTask, "wifi", 4096, nullptr, 1, &wifiTaskHandle, UI_CORE);
}

// Schreibt Statusänderungen auf die serielle Konsole (aufgerufen 1× pro Sekunde):
//  - WLAN verbunden (mit SSID, IP, Signal) / Verbindung verloren
//  - jede NTP-Synchronisierung mit Datum und Uhrzeit
//  - einmalige Warnung, wenn der letzte Sync länger als 2 Intervalle her ist
static void wifiLogChanges()
{
  static wl_status_t last = WL_IDLE_STATUS;
  static time_t lastLoggedSync = 0;
  static bool   overdueLogged  = false;

  wl_status_t st = WiFi.status();
  if (st != last) {
    if (st == WL_CONNECTED)
      Serial.printf("[WiFi] verbunden mit \"%s\", IP %s, RSSI %d dBm\n",
                    WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI());
    else if (last == WL_CONNECTED)
      Serial.println("[WiFi] Verbindung verloren – versuche erneut");
    last = st;
  }

  // Neuer Sync erkannt = Zeitstempel hat sich seit der letzten Ausgabe geändert
  const time_t syncEpoch = lastSyncEpoch();
  if (g_ntpSynced && syncEpoch != lastLoggedSync) {
    struct tm st;
    localtime_r(&syncEpoch, &st);
    Serial.printf("[NTP] Sync %02d.%02d.%04d %02d:%02d:%02d, naechster in ca. %lu min\n",
                  st.tm_mday, st.tm_mon + 1, st.tm_year + 1900, st.tm_hour, st.tm_min, st.tm_sec,
                  (unsigned long)(NTP_SYNC_INTERVAL_MS / 60000UL));
    lastLoggedSync = syncEpoch;
    overdueLogged  = false;
  }

  // Überfällig? (z.B. WLAN dauerhaft weg oder NTP-Server nicht erreichbar)
  if (g_ntpSynced && !overdueLogged &&
      time(nullptr) - syncEpoch > (time_t)(2 * NTP_SYNC_INTERVAL_MS / 1000UL)) {
    Serial.println("[NTP] Warnung: letzter Sync liegt mehr als 2 Intervalle zurueck");
    overdueLogged = true;
  }
}

//*****************************************************************************
// OLED – Waveshare 2.42" SSD1309 128x64, I2C (Core 0)
//*****************************************************************************
// "_F_" = voller Framebuffer (1 KB RAM): Das Bild wird komplett im RAM
// aufgebaut und mit sendBuffer() in einem Rutsch übertragen (~25 ms bei
// 400 kHz) – kein Flackern beim Neuzeichnen.
// Falls das Bild um 2 Pixel verschoben erscheint: NONAME0 durch NONAME2 ersetzen.
static U8G2_SSD1309_128X64_NONAME0_F_HW_I2C u8g2(U8G2_R0, OLED_RST_PIN, OLED_SCL_PIN, OLED_SDA_PIN);

// Display initialisieren und kurz einen Startbildschirm zeigen.
// Wird im uiTask aufgerufen, damit der I2C-Interrupt auf Core 0 liegt.
static void oledInit()
{
  u8g2.setI2CAddress(OLED_I2C_ADDR << 1);   // U8g2 erwartet die 8-Bit-Adresse
  u8g2.setBusClock(OLED_I2C_HZ);
  u8g2.begin();
  u8g2.setContrast(OLED_CONTRAST);
  u8g2.enableUTF8Print();

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14B_tf);
  u8g2.drawStr(10, 26, "Scope Clock");
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(10, 44, "Firmware v" FW_VERSION);
  u8g2.drawStr(10, 58, "Starte ...");
  u8g2.sendBuffer();
}

// ---- 7-Segment-Ziffern ------------------------------------------------------
// Die fertigen 7-Segment-Fonts von U8g2 sind für HH:MM:SS auf 128 px zu breit,
// daher werden die Ziffern hier aus 7 Rechtecken selbst gezeichnet.
//
//        aaa
//       f   b        Segment-Bits in SEG_MAP:
//       f   b          a=0x01  b=0x02  c=0x04  d=0x08
//        ggg           e=0x10  f=0x20  g=0x40
//       e   c
//       e   c
//        ddd
static const uint8_t SEG_MAP[10] = {
  0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F   // 0 … 9
};

// Zeichnet eine Ziffer an (x, y) = linke obere Ecke.
//   w, h  Breite/Höhe der Ziffer in Pixeln
//   t     Segmentstärke in Pixeln
// Die Ecken bleiben frei, so entsteht der typische Segment-Look.
static void drawSegDigit(int x, int y, int digit, int w, int h, int t)
{
  const uint8_t m = SEG_MAP[digit % 10];
  const int mid = y + (h - t) / 2;                 // Oberkante Segment g
  const int vUp = mid - (y + t);                   // Höhe der oberen Senkrechten (f, b)
  const int vLo = (y + h - t) - (mid + t);         // Höhe der unteren Senkrechten (e, c)
  if (m & 0x01) u8g2.drawBox(x + t,     y,         w - 2 * t, t);    // a
  if (m & 0x02) u8g2.drawBox(x + w - t, y + t,     t, vUp);          // b
  if (m & 0x04) u8g2.drawBox(x + w - t, mid + t,   t, vLo);          // c
  if (m & 0x08) u8g2.drawBox(x + t,     y + h - t, w - 2 * t, t);    // d
  if (m & 0x10) u8g2.drawBox(x,         mid + t,   t, vLo);          // e
  if (m & 0x20) u8g2.drawBox(x,         y + t,     t, vUp);          // f
  if (m & 0x40) u8g2.drawBox(x + t,     mid,       w - 2 * t, t);    // g
}

// Zeichnet HH:MM:SS (24h) als 7-Segment-Anzeige, horizontal zentriert.
//   y   Oberkante der Ziffern
//   dx  horizontaler Pixel-Shift (0…3)
// Gesamtbreite: 6×11 + 3×2 + 2×(3+2+3) = 88 px.
static void drawSegTime(int y, int hh, int mm, int ss, int dx)
{
  const int W = 11, H = 20, T = 2;   // Ziffer 11x20 px, Segment 2 px
  const int GAP = 2;                 // Abstand zwischen zwei Ziffern eines Paares
  const int COLON = 2, CGAP = 3;     // Doppelpunkt 2x2 px, 3 px Abstand je Seite
  const int total = 6 * W + 3 * GAP + 2 * (CGAP + COLON + CGAP);
  int x = (128 - 3 - total) / 2 + dx; // 3 px Reserve für den Pixel-Shift

  const int v[3] = { hh, mm, ss };
  for (int i = 0; i < 3; i++) {
    drawSegDigit(x, y, v[i] / 10, W, H, T);  x += W + GAP;   // Zehner
    drawSegDigit(x, y, v[i] % 10, W, H, T);  x += W;         // Einer
    if (i < 2) {                                             // Doppelpunkt
      x += CGAP;
      u8g2.drawBox(x, y + H / 3 - 1,     COLON, COLON);
      u8g2.drawBox(x, y + 2 * H / 3 - 1, COLON, COLON);
      x += COLON + CGAP;
    }
  }
}

// Zeichnet Text horizontal zentriert (aktueller Font).
//   baseline  Grundlinie (U8g2 positioniert Text an der Grundlinie, nicht oben)
//   dx        horizontaler Pixel-Shift
// Sehr breite Zeilen werden am Rand festgehalten, damit sie trotz Shift
// nicht abgeschnitten werden.
static void drawCentered(const char *txt, int baseline, int dx)
{
  const int w = u8g2.getStrWidth(txt);
  int x = (128 - 3 - w) / 2 + dx;
  if (x + w > 128) x = 128 - w;
  if (x < 0) x = 0;
  u8g2.drawStr(x, baseline, txt);
}

// ---- Kalenderwoche nach ISO 8601 (wie im deutschen Kalender) ----------------
// Regeln: Die Woche beginnt am Montag; KW 1 ist die Woche mit dem 4. Januar
// (= mit dem ersten Donnerstag). Tage Anfang Januar können daher zur letzten
// KW des Vorjahres gehören, Tage Ende Dezember zur KW 1 des Folgejahres.
// Geprüft gegen strftime("%V") für alle Tage 2000–2040.

// Anzahl der ISO-Wochen eines Jahres (52 oder 53). Ein Jahr hat 53 Wochen,
// wenn es an einem Donnerstag beginnt oder (Schaltjahr) an einem Mittwoch.
// p(y) = Wochentag des 31.12. des Jahres y (0 = Sonntag … 4 = Donnerstag).
static int isoWeeksInYear(int y)
{
  auto p = [](int yy) { return (yy + yy / 4 - yy / 100 + yy / 400) % 7; };
  return (p(y) == 4 || p(y - 1) == 3) ? 53 : 52;
}

// ISO-Kalenderwoche zu einem Datum (benötigt tm_yday, tm_wday, tm_year).
static int isoWeek(const struct tm &t)
{
  const int year = t.tm_year + 1900;
  const int wd   = t.tm_wday ? t.tm_wday : 7;          // Mo=1 … So=7
  const int w    = (t.tm_yday + 1 - wd + 10) / 7;      // Standardformel (Tag des Jahres 1-basiert)
  if (w < 1) return isoWeeksInYear(year - 1);          // gehört zur letzten KW des Vorjahres
  if (w > isoWeeksInYear(year)) return 1;              // gehört zur KW 1 des Folgejahres
  return w;
}

// Kürzt einen Text in-place auf maxChars Zeichen und markiert das mit "..".
// buf muss mindestens maxChars+1 Bytes groß sein.
static void fitText(char *buf, size_t maxChars)
{
  if (strlen(buf) > maxChars && maxChars > 2) {
    buf[maxChars - 2] = '.';
    buf[maxChars - 1] = '.';
    buf[maxChars]     = '\0';
  }
}

// ---- Bildaufbau ----------------------------------------------------------------
// Alle Zeilen horizontal zentriert, vertikal mit kleinen Abständen:
//
//   y  3…22   22:15:06               HH:MM:SS, 24h, 7-Segment (11x20 px/Ziffer)
//             ~6 px Abstand
//   y 29…35   KW 46 - 15.11.2026     Font 6x10
//             ~7 px Abstand (trennt Zeit/Datum von WLAN)
//   y 43…49   WLAN Gastzugang        Font 5x8, max. 25 Zeichen
//   y 52…58   Signal -61 dBm         Font 5x8
//
// Ohne NTP-Sync steht statt des Datums "Datum unbekannt". Der NTP-Sync selbst
// wird nur auf der seriellen Konsole gemeldet (wifiLogChanges).
//
// Pixel-Shift gegen Einbrennen: alle OLED_SHIFT_EVERY_S Sekunden wird das
// gesamte Bild um 0…3 px nach rechts und 0…1 px nach unten versetzt.
static void oledUpdate()
{
  const size_t LINE_CHARS = 25;     // 5x8-Font: 125 px Breite + 3 px Shift-Reserve
  struct tm t;
  const bool valid = currentTime(t);

  int dx = 0, dy = 0;
#if OLED_SHIFT_EVERY_S > 0
  const uint32_t step = (millis() / 1000UL / OLED_SHIFT_EVERY_S) % 4;
  dx = step;            // 0,1,2,3,0,…
  dy = (step + 1) % 2;  // 1,0,1,0,…
#endif

  char buf[48];
  u8g2.clearBuffer();

  // Vertikale Positionen (zum Nachjustieren am echten Display)
  const int Y_TIME  = 3;    // Oberkante der 7-Segment-Ziffern
  const int Y_DATE  = 36;   // Grundlinie Kalenderwoche/Datum
  const int Y_SSID  = 49;   // Grundlinie Netzwerkname
  const int Y_RSSI  = 58;   // Grundlinie Signalstärke

  // 1) Uhrzeit
  drawSegTime(Y_TIME + dy, t.tm_hour, t.tm_min, t.tm_sec, dx);

  // 2) Kalenderwoche + Datum (nur mit gültiger NTP-Zeit)
  u8g2.setFont(u8g2_font_6x10_tf);
  if (valid)
    snprintf(buf, sizeof(buf), "KW %02d - %02d.%02d.%04d", isoWeek(t), t.tm_mday,
             t.tm_mon + 1, t.tm_year + 1900);   // 18 Zeichen = 108 px
  else
    snprintf(buf, sizeof(buf), "Datum unbekannt");
  drawCentered(buf, Y_DATE + dy, dx);

  u8g2.setFont(u8g2_font_5x8_tf);
  const bool connected = wifiConfigured() && WiFi.status() == WL_CONNECTED;

  // 3) Netzwerkname: verbundenes Netz / Suche läuft / kein WLAN eingetragen
  if (!wifiConfigured())  snprintf(buf, sizeof(buf), "WLAN aus");
  else if (connected)     snprintf(buf, sizeof(buf), "WLAN %s", WiFi.SSID().c_str());
  else                    snprintf(buf, sizeof(buf), "WLAN suche ...");
  fitText(buf, LINE_CHARS);
  drawCentered(buf, Y_SSID + dy, dx);

  // 4) Signalstärke (RSSI in dBm; näher an 0 = besser, z.B. -50 sehr gut, -85 schwach)
  if (connected) {
    snprintf(buf, sizeof(buf), "Signal %d dBm", (int)WiFi.RSSI());
  } else if (wifiConfigured()) {
    snprintf(buf, sizeof(buf), "keine Verbindung");
  } else {
    snprintf(buf, sizeof(buf), "Offline-Betrieb");
  }
  fitText(buf, LINE_CHARS);
  drawCentered(buf, Y_RSSI + dy, dx);

  u8g2.sendBuffer();                // Framebuffer per I2C ans Display
}

//*****************************************************************************
// uiTask – Core 0: Zeitübergabe, OLED, Konsolen-Log
//*****************************************************************************
// Einfacher Zeitplan ohne weitere Tasks:
//   alle  10 ms  publishTime()    → Sekundenzeiger springt max. 10 ms zu spät
//   alle 200 ms  oledUpdate()     → Sekundenanzeige auf dem OLED flüssig
//   alle   1 s   wifiLogChanges() → Statusmeldungen auf der Konsole
static void uiTask(void *)
{
  // WICHTIG: OLED (I2C) und WLAN werden HIER initialisiert, nicht in setup():
  // ihre Interrupts landen damit auf Core 0 und stören den Zeichenpfad nicht.
  Serial.printf("[UI] laeuft auf Core %d\n", xPortGetCoreID());
  oledInit();
  wifiStart();

  uint32_t lastOled = 0, lastLog = 0;
  for (;;) {
    publishTime();

    const uint32_t now = millis();
    if (now - lastOled >= OLED_REFRESH_MS) {
      lastOled = now;
      oledUpdate();                       // ~25 ms I2C-Transfer, blockiert nur Core 0
    }
    if (now - lastLog >= 1000) {
      lastLog = now;
      wifiLogChanges();
    }
    vTaskDelay(pdMS_TO_TICKS(10));        // Rechenzeit an WLAN-Stack & Co. abgeben
  }
}

//*****************************************************************************
// setup / loop
//*****************************************************************************
// setup() läuft im Arduino-loopTask (Core 1). Es startet nur die beiden
// Haupttasks und überlässt ihnen dann die Arbeit.
void setup()
{
  Serial.begin(SERIAL_BAUD);
  delay(200);                             // USB-Seriell kurz Zeit geben
  Serial.println("\nESP32 Oscilloscope Clock v" FW_VERSION);
  Serial.println("basierend auf v1.0 von Mauro Pintus 2018");
  Serial.printf("CPU: %lu MHz\n", (unsigned long)getCpuFrequencyMhz());

  // Bis zum ersten publishTime() die Startzeit anzeigen
  g_timePacked = packTime(FALLBACK_H, FALLBACK_M, FALLBACK_S);

  //                      Funktion   Name     Stack  Param    Prio Handle           Kern
  xTaskCreatePinnedToCore(uiTask,    "ui",    8192, nullptr, 1,   &uiTaskHandle,    UI_CORE);
  xTaskCreatePinnedToCore(scopeTask, "scope", 4096, nullptr, 1,   &scopeTaskHandle, SCOPE_CORE);
}

// Die Arduino-loop() wird nicht gebraucht: Core 1 gehört dem scopeTask.
// Der loopTask legt sich beim ersten Durchlauf dauerhaft schlafen.
void loop()
{
  vTaskSuspend(nullptr);
}
