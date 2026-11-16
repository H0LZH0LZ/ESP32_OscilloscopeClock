/******************************************************************************

  ESP32 Oscilloscope Clock v2.0
  X/Y-Vektoruhr über die internen DACs, NTP-Zeit, Status-OLED

  Basiert auf: ESP32 Oscilloscope Clock v1.0
               Mauro Pintus, Milano 2018/05/25
               https://github.com/maurohh/ESP32_OscilloscopeClock
               (Vektordaten DataTable.h aus v1.0 übernommen; die Zeichen-
               routine ist neu: konstante Strahlgeschwindigkeit, damit alle
               Linien gleich hell sind – siehe "Vektor-Engine")

  ---------------------------------------------------------------------------
  Funktionsprinzip
  ---------------------------------------------------------------------------
  Ein Oszilloskop im X/Y-Modus lenkt den Elektronenstrahl horizontal mit der
  Spannung an Kanal 1 (X) und vertikal mit der Spannung an Kanal 2 (Y) ab.
  Der ESP32 erzeugt beide Spannungen mit seinen zwei 8-Bit-DACs
  (GPIO25 = X, GPIO26 = Y, je 0…255 ≙ ca. 0…3,2 V) und "fährt" damit das
  Zifferblatt und die Zeiger Punkt für Punkt ab. Weil das Bild auf dem
  Schirm nur kurz nachleuchtet, muss es ohne Pause immer wieder neu
  gezeichnet werden (~230×/s) – sonst flimmert es oder es entstehen helle
  Punkte. Wie hell eine Linie leuchtet, hängt davon ab, wie lange der Strahl
  auf ihr verweilt; die Vektor-Engine sorgt deshalb für überall gleiche
  Strahlgeschwindigkeit.

  ---------------------------------------------------------------------------
  Aufgabenteilung auf die beiden Kerne (FreeRTOS)
  ---------------------------------------------------------------------------
    Core 1 │ scopeTask  │ zeichnet endlos Zeiger + Zifferblatt über die DACs
           │            │ (Vektor-Engine, Rundtour, s.u.)
    ───────┼────────────┼────────────────────────────────────────────────────
    Core 0 │ uiTask     │ Uhrzeit berechnen + an Core 1 übergeben (alle 10 ms),
           │            │ BOOT-Taster abfragen (10 ms), OLED (200 ms),
           │            │ Konsolen-Log (1 s)
           │            │ Webinterface + Setup-WLAN (handleClient alle 10 ms)
           │ wifiTask   │ WLAN-Verbindung aufbauen/halten (WiFiMulti)
           │ (System)   │ WLAN-Stack, SNTP (NTP-Client des ESP-IDF)

  Von Core 0 nach Core 1 gehen nur drei Werte, alle atomar lesbar:
  g_timePacked (32 Bit: Stunde/Minute/Sekunde, einmal pro Bild gelesen),
  g_scopeProfile (1 Byte: ANALOG/DIGITAL) und g_dialStyle (1 Byte:
  Zifferblatt). Der Zeichenpfad muss nie warten.

  ---------------------------------------------------------------------------
  Bedienung
  ---------------------------------------------------------------------------
  BOOT-Taster im Betrieb 3 s halten → Oszilloskop-Profil wechselt
  (DIGITAL ↔ ANALOG, siehe config.h: SCOPE_PROFILE). Das OLED zeigt dabei
  "DIGITAL > ANALOG" und einen Countdown 3, 2, 1; das Profil wird im Flash
  gespeichert. Nicht beim Einschalten drücken (Download-Modus).

  Webinterface: http://scope-clock.local (oder IP-Adresse der Uhr) – Live-
  Vorschau, Status, Log, WLAN-Einrichtung, Seitenmenü; für Handys optimiert,
  ohne Passwort. Tippen auf die Vorschau schaltet das Zifferblatt um
  (1–12 / 12-3-6-9 / römisch / XII-III-VI-IX) – auch am Oszilloskop,
  wird gespeichert. Ohne bekanntes
  WLAN öffnet die Uhr das offene Setup-WLAN "ScopeClock-Setup"
  (Einrichtung unter http://192.168.4.1).

  ---------------------------------------------------------------------------
  Dateien
  ---------------------------------------------------------------------------
    ESP32_OscilloscopeClock_v2.0.ino  dieser Sketch
    config.h                          Pins, Zeitzone, NTP, OLED, Strahl-
                                      geschwindigkeit, Oszilloskop-Profil
    secrets.h                         WLAN-Liste (lokal, nicht im Git)
    secrets_example.h                 Vorlage für secrets.h
    DataTable.h                       Vektordaten: Zifferblatt, Ziffern, Zeiger
    webpage.h                         Webinterface (HTML/CSS/JS, eine Seite)
    ../HARDWARE.md                    Verdrahtung, Gehäuse, Oszilloskop-Einstellungen
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
#include <soc/gpio_reg.h>     // GPIO-Setzen/Löschen für die optionale Dunkeltastung
#include <U8g2lib.h>          // OLED-Treiber
#include <Preferences.h>      // dauerhafte Einstellungen im Flash (NVS): Profil, WLANs
#include <WebServer.h>        // Webinterface (HTTP, Port 80)
#include <DNSServer.h>        // Setup-WLAN: alle Namen auf die Uhr umleiten (Captive Portal)
#include <ESPmDNS.h>          // Erreichbar als http://<WIFI_HOSTNAME>.local
#include <stdarg.h>

#include "config.h"

//*****************************************************************************
// Datentypen
//*****************************************************************************
// Alle eigenen Typen stehen hier vor der ersten Funktion: Die Arduino-IDE
// erzeugt vor dem Kompilieren automatisch Prototypen aller Funktionen und
// setzt sie an diese Stelle – Typen, die erst weiter unten definiert sind,
// wären für diese Prototypen noch unbekannt.

// Ein Stroke ist ein zusammenhängender Linienzug in einer Tabelle aus
// DataTable.h: Punkte bei den Byte-Offsets a, a±2, …, b. Ist b < a, wird er
// rückwärts gezeichnet (Helligkeit ist richtungsunabhängig).
struct Stroke {
  const uint8_t *t;
  int16_t a, b;
};

// Ein im Webinterface eingetragenes WLAN (gespeichert im Flash/NVS).
struct StoredNet {
  char ssid[33];       // max. 32 Zeichen + Nullbyte
  char pw[65];         // max. 64 Zeichen + Nullbyte (leer = offenes Netz)
};

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
#include "webpage.h"          // HTML/CSS/JS des Webinterfaces (PROGMEM)

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
// Oszilloskop-Profil: 0 = analoges Röhren-Oszilloskop, 1 = digitales Oszilloskop.
// Startwert aus dem Flash (NVS), im Betrieb per BOOT-Taster umschaltbar.
// Schreiber: uiTask (Core 0), Leser: beamJump (Core 1). 1 Byte = atomar.
static volatile uint8_t g_scopeProfile = SCOPE_PROFILE;

// Zifferblatt: 0 = Ziffern 1–12, 1 = nur 12/3/6/9, 2 = römisch I–XII,
// 3 = nur XII/III/VI/IX. Startwert aus dem
// Flash (NVS), im Betrieb über das Webinterface umschaltbar (Tippen auf die
// Vorschau). Schreiber: uiTask, Leser: scopeTask (baut dann die Rundtour neu).
static volatile uint8_t g_dialStyle = DIAL_STYLE;

// Weitere Laufzeit-Einstellungen für die Strahlausgabe (Webinterface →
// "Einstellungen", gespeichert im Flash; config.h liefert nur die Startwerte).
// Schreiber: uiTask, Leser: scopeTask – je ≤ 16 Bit, atomar.
static volatile uint16_t g_beamNs    = BEAM_NS_PER_UNIT;   // Strahlgeschwindigkeit (ns pro Schritt)
static volatile uint8_t  g_handPct   = HAND_BRIGHT_PCT;    // Helligkeit Stunden-/Minutenzeiger (%)
static volatile uint8_t  g_jumpSpeed = JUMP_SPEED;         // Rücklauf-Tempo (Profil ANALOG), 0 = hart

// Einstellungen, die nur Core 0 nutzt (Zeit, OLED)
static char    s_tz[48]        = TZ_INFO;                  // POSIX-Zeitzone
static char    s_ntpServer[64] = NTP_SERVER_1;             // erster NTP-Server (muss dauerhaft gültig bleiben)
static uint8_t s_oledContrast  = OLED_CONTRAST;
static bool    s_oledShift     = OLED_SHIFT_EVERY_S > 0;

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

// Bildzähler: scopeTask erhöht ihn pro Bild (32 Bit, atomar), der uiTask
// berechnet daraus die Bildrate für das Webinterface.
static volatile uint32_t g_frameCount = 0;

//*****************************************************************************
// Log: serielle Konsole + Ringpuffer für das Webinterface
//*****************************************************************************
// logMsg() schreibt eine Zeile mit Zeitstempel auf die Konsole und in einen
// Ringpuffer (LOG_BUFFER_SIZE Byte, die ältesten Zeilen werden überschrieben).
// Das Webinterface zeigt den Puffer unter /api/log an.
// Zeitstempel: hh:mm:ss nach dem ersten NTP-Sync, davor "+<Sekunden seit Start>s".
// Aufrufbar von beiden Kernen; der Spinlock schützt nur das kurze Kopieren.
static char         s_logBuf[LOG_BUFFER_SIZE];
static size_t       s_logHead    = 0;       // nächste Schreibposition
static bool         s_logWrapped = false;   // Puffer schon einmal voll gelaufen
static portMUX_TYPE s_logMux     = portMUX_INITIALIZER_UNLOCKED;

static void logMsg(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void logMsg(const char *fmt, ...)
{
  char line[192];
  int n;
  if (g_ntpSynced) {
    const time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    n = snprintf(line, sizeof(line), "%02d:%02d:%02d ", t.tm_hour, t.tm_min, t.tm_sec);
  } else {
    n = snprintf(line, sizeof(line), "+%lus ", (unsigned long)(millis() / 1000UL));
  }
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(line + n, sizeof(line) - n, fmt, ap);
  va_end(ap);
  size_t len = strlen(line);
  if (len == 0 || line[len - 1] != '\n') {             // jede Meldung endet mit Zeilenumbruch
    if (len < sizeof(line) - 1) { line[len++] = '\n'; line[len] = 0; }
    else line[len - 1] = '\n';
  }
  Serial.print(line);

  portENTER_CRITICAL(&s_logMux);
  for (size_t i = 0; i < len; i++) {
    s_logBuf[s_logHead] = line[i];
    if (++s_logHead >= sizeof(s_logBuf)) { s_logHead = 0; s_logWrapped = true; }
  }
  portEXIT_CRITICAL(&s_logMux);
}

// Liefert den Log-Inhalt in zeitlicher Reihenfolge (älteste Zeile zuerst).
// Nach einem Überlauf wird die angeschnittene erste Zeile weggelassen.
static String logRead()
{
  static char copy[LOG_BUFFER_SIZE];
  size_t head; bool wrapped;
  portENTER_CRITICAL(&s_logMux);
  memcpy(copy, s_logBuf, sizeof(copy));
  head = s_logHead; wrapped = s_logWrapped;
  portEXIT_CRITICAL(&s_logMux);

  String out;
  out.reserve(sizeof(copy) + 1);
  if (wrapped) {
    size_t start = head;
    while (start < sizeof(copy) && copy[start] != '\n') start++;   // angeschnittene Zeile überspringen
    for (size_t i = start + 1; i < sizeof(copy); i++) out += copy[i];
  }
  for (size_t i = 0; i < head; i++) out += copy[i];
  return out;
}

static TaskHandle_t scopeTaskHandle = nullptr;
static TaskHandle_t uiTaskHandle    = nullptr;
static TaskHandle_t wifiTaskHandle  = nullptr;
static WiFiMulti    wifiMulti;

//*****************************************************************************
// Vektor-Engine (Core 1)
//*****************************************************************************
// Ziel: ALLE Linien und Punkte gleich hell – auch auf älteren Röhren-Oszilloskopen.
//
// Die Helligkeit einer Spur auf dem Schirm hängt davon ab, wie lange der
// Strahl auf jedem Stück Weg verweilt (Energie pro Länge). Gleich hell heißt
// also: überall dieselbe Verweilzeit pro Längeneinheit. Daraus folgen die Regeln:
//
//  1. Gleiche Abstände: Jede Linie wird in Schritte von genau 1 DAC-Einheit
//     Länge zerlegt – gemessen entlang der Linie (euklidisch). Bresenham im
//     Original setzt bei 45° nur 0,71 Punkte pro Einheit; schräge Linien waren
//     dort bis zu ~20 % dunkler als waagerechte.
//  2. Gleiche Verweilzeit: Jeder Schreibschritt dauert genau BEAM_NS_PER_UNIT.
//     Jeder Schritt hat dazu einen festen Zieltermin auf dem CPU-Taktzähler;
//     verzögert ein Interrupt einen Schritt, werden die folgenden kürzer.
//  3. Kein Verweilen an Ecken: Der Endpunkt einer Linie ist der Startpunkt
//     der nächsten und wird nur EINMAL ausgegeben (Original: doppelt → helle
//     Punkte an jeder Ecke).
//  4. Rundungsrest: Kurze Linien (in den Ziffern oft nur 1–2 Einheiten) werden
//     nicht einzeln gerundet; der Rest wandert in die nächste Linie. Ein ganzer
//     Linienzug bekommt so exakt seine Länge an Verweilzeit.
//  5. Sprünge zwischen Linienzügen (Rücklauf): Ohne Dunkeltastung bleibt
//     auf dem Schirm eine Spur. Sie wird so schwach wie möglich gehalten:
//      a) Sprünge laufen in 1-Einheit-Schritten mit JUMP_SPEED-facher
//         Geschwindigkeit (Spur = 1/JUMP_SPEED der Linienhelligkeit) und
//         bremsen auf den letzten JUMP_RAMP Einheiten auf JUMP_LANDING ab.
//         So kommen auch langsame X/Y-Verstärker alter Oszilloskope präzise am
//         Ziel an (ein harter Sprung würde dort über- oder nachschwingen).
//      b) Zifferblatt und Ziffern werden beim Start einmal zu einer
//         Rundtour mit möglichst kurzen Sprüngen sortiert (buildTour).
//      c) Der Sekundenzeiger wird hin (Mitte → Spitze) und zurück je mit
//         halber Helligkeit gezeichnet. Der lange Sprung vom Rand zurück
//         zur Mitte entfällt damit ganz – der Strahl "läuft" auf dem Zeiger.
//      d) Jeder Linienzug wird von dem Ende aus gezeichnet, das näher am
//         Strahl liegt.
//     Optional (ZBLANK_PIN): Dunkeltastung über den Z-Eingang des Oszilloskops –
//     der Strahl ist während des Sprungs aus, die Spur verschwindet ganz.
//     JUMP_SPEED 0 = harter Sprung wie im Original.
//  6. Begrenzte Geschwindigkeit: 1 DAC-Einheit (≈ 12,5 mV) pro 3 µs beim
//     Schreiben ist für jede Röhre langsam genug.
//
// Alle Funktionen hier tragen IRAM_ATTR und die Tabellen liegen im RAM:
// So gibt es keine Flash-Cache-Fehlzugriffe (Nachladen aus dem Flash dauert
// unterschiedlich lange), die das Timing und damit die Helligkeit stören.

// Aktuell ausgegebene DAC-Werte (= Strahlposition)
static int lastx = -1, lasty = -1;

// Zeitsteuerung in CPU-Takten
static uint32_t s_stepCycles = 0;   // Verweilzeit eines Schreibschritts (BEAM_NS_PER_UNIT)
// Verweilzeit eines Sprungschritts abhängig vom Restweg r (0 … JUMP_RAMP):
// r = JUMP_RAMP und mehr → volle Sprunggeschwindigkeit, r = 0 → JUMP_LANDING
static uint32_t s_jumpCycles[JUMP_RAMP + 1];
#if ZBLANK_PIN >= 0
static uint32_t s_settleCycles = 0; // Einschwingzeit bei Dunkeltastung
#endif
static uint32_t s_deadline   = 0;   // Zieltermin des aktuellen Schritts

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

// Berechnet die Schrittdauern aus Strahlgeschwindigkeit (ns pro Schritt) und
// Rücklauf-Tempo. Läuft auf Core 1: beim Start und zwischen zwei Bildern, wenn
// die Werte im Webinterface geändert wurden.
static void beamTiming(uint16_t beamNs, uint8_t jumpSpeed)
{
  s_stepCycles = (uint32_t)((uint64_t)beamNs * getCpuFrequencyMhz() / 1000ULL);
  if (s_stepCycles < 1) s_stepCycles = 1;
  // Sprung-Bremsrampe vorberechnen (spart Divisionen im Zeichenpfad).
  // Ist das Tempo kleiner als JUMP_LANDING, wird gleichmäßig mit diesem Tempo gefahren.
  static_assert(JUMP_LANDING >= 1 && JUMP_RAMP >= 1, "config.h: JUMP_LANDING >= 1 und JUMP_RAMP >= 1");
  if (jumpSpeed == 0) return;                          // harter Sprung: keine Rampe nötig
  const uint32_t land = jumpSpeed < JUMP_LANDING ? jumpSpeed : JUMP_LANDING;
  for (int r = 0; r <= JUMP_RAMP; r++) {
    const uint32_t v16 = 16 * land + 16 * (jumpSpeed - land) * r / JUMP_RAMP;
    s_jumpCycles[r] = v16 ? (s_stepCycles * 16) / v16 : s_stepCycles;
    if (s_jumpCycles[r] < 1) s_jumpCycles[r] = 1;
  }
}

// Schaltet beide DACs ein und berechnet die Schrittdauern. dacWrite()
// übernimmt die komplette Pad-Konfiguration (Power-up, Kosinusgenerator aus)
// für ESP-IDF 4 und 5; danach schreiben dacOutX/Y nur noch ins Register.
static void dacInit() {
  dacWrite(DAC_X_PIN, 0x80);
  dacWrite(DAC_Y_PIN, 0x80);
  lastx = lasty = 0x80;
  beamTiming(g_beamNs, g_jumpSpeed);
#if ZBLANK_PIN >= 0
  pinMode(ZBLANK_PIN, OUTPUT);
  digitalWrite(ZBLANK_PIN, ZBLANK_ACTIVE_HIGH ? LOW : HIGH);   // Strahl an
  s_settleCycles = (uint32_t)((uint64_t)ZBLANK_SETTLE_NS * getCpuFrequencyMhz() / 1000ULL);
#endif
  s_deadline = ESP.getCycleCount();
}

// Gibt einen Strahlpunkt aus und hält ihn 'cycles' CPU-Takte lang.
// Die DACs werden nur bei Änderung beschrieben; X und Y unmittelbar
// nacheinander, damit kein sichtbarer Zwischenpunkt entsteht.
static inline void IRAM_ATTR beamPoint(int x, int y, uint32_t cycles)
{
  if (x != lastx) { lastx = x; dacOutX((uint8_t)x); }
  if (y != lasty) { lasty = y; dacOutY((uint8_t)y); }
#if defined EXCEL
  (void)cycles;
  Serial.printf("0x%02X,0x%02X,\n", x & 0xFF, y & 0xFF);
#else
  s_deadline += cycles;
  // Mehr als 64 Schreibschritte im Verzug (z.B. Core 1 wurde für einen
  // Flash-Zugriff kurz angehalten)? Dann nicht "nachholen" – sonst würde eine
  // Strecke im Zeitraffer und damit zu dunkel gezeichnet. Zeitplan neu setzen.
  if ((int32_t)(ESP.getCycleCount() - s_deadline) > (int32_t)(64 * s_stepCycles))
    s_deadline = ESP.getCycleCount();
  while ((int32_t)(ESP.getCycleCount() - s_deadline) < 0) { }
#endif
}

// Ganzzahlige Quadratwurzel (für Längen; ohne Gleitkomma und ohne libm aus
// dem Flash, damit der Zeichenpfad vollständig im IRAM bleibt).
static inline uint32_t IRAM_ATTR isqrt32(uint32_t v)
{
  uint32_t r = 0, bit = 1UL << 30;
  while (bit > v) bit >>= 2;
  while (bit) {
    if (v >= r + bit) { v -= r + bit; r = (r >> 1) + bit; }
    else              { r >>= 1; }
    bit >>= 2;
  }
  return r;
}

// Fährt den Strahl von (x1,y1) nach (x2,y2) in Schritten von 1 DAC-Einheit
// Länge; jeder Schritt verweilt 'cycles' CPU-Takte.
// Der Startpunkt wird NICHT ausgegeben (er ist der aktuelle Strahlpunkt),
// der Endpunkt schon.
//
// carry16: Rundungsrest in 1/16 Einheiten, der von Linie zu Linie eines
// Linienzugs weitergereicht wird (Regel 4). nullptr = Sprung: mindestens ein
// Schritt, kein Übertrag. Linien kürzer als ½ Einheit können mit Übertrag
// 0 Schritte bekommen – ihr Anteil wandert dann in die nächste Linie.
static void IRAM_ATTR beamMove(int x1, int y1, int x2, int y2, uint32_t cycles, int32_t *carry16)
{
  const int dx = x2 - x1, dy = y2 - y1;
  // Länge in 1/16 Einheiten: sqrt((dx²+dy²)·256) = 16·Länge
  int32_t len16 = (int32_t)isqrt32((uint32_t)(dx * dx + dy * dy) << 8);
  int n;
  if (carry16) {
    len16 += *carry16;
    n = (len16 + 8) / 16;                        // gerundet, kann 0 sein
    if (n < 0) n = 0;
    *carry16 = len16 - n * 16;
    if (n == 0) return;
  } else {
    n = (len16 + 8) / 16;
    if (n < 1) n = 1;
  }
  // Zwischenpunkte in Festkomma 16.16 (+0,5 für korrektes Runden)
  int32_t fx = x1 * 65536 + 0x8000, fy = y1 * 65536 + 0x8000;
  const int32_t ix = dx * 65536 / n, iy = dy * 65536 / n;   // dx, dy dürfen negativ sein
  for (int k = 1; k < n; k++) {
    fx += ix; fy += iy;
    beamPoint(fx >> 16, fy >> 16, cycles);
  }
  beamPoint(x2, y2, cycles);             // exakt auf dem Endpunkt landen
}

#if ZBLANK_PIN >= 0
static_assert(ZBLANK_PIN < 32, "ZBLANK_PIN muss GPIO 0-31 sein");
// Schaltet den Strahl über den Z-Eingang des Oszilloskops aus (dark=true)
// oder wieder an. Direkter Registerzugriff: wenige Takte, IRAM-sicher.
static inline void IRAM_ATTR zBlank(bool dark)
{
  if (dark == (bool)ZBLANK_ACTIVE_HIGH) REG_WRITE(GPIO_OUT_W1TS_REG, 1UL << ZBLANK_PIN);
  else                                  REG_WRITE(GPIO_OUT_W1TC_REG, 1UL << ZBLANK_PIN);
}
#endif

// Sprung (Rücklauf) zum Punkt (x2,y2) – siehe Regel 5.
// Die Sprungart hängt vom Oszilloskop-Profil ab (BOOT-Taster, g_scopeProfile):
//   digital → harter Sprung: ein digitales Oszilloskop würde jeden Zwischenpunkt
//             eines gefahrenen Sprungs als hellen Abtastpunkt zeigen.
//   analog  → gebremster Sprung (JUMP_SPEED/JUMP_LANDING/JUMP_RAMP).
// Mit Z-Dunkeltastung (ZBLANK_PIN) wird immer im Dunkeln hart gesprungen.
static void IRAM_ATTR beamJump(int x2, int y2)
{
  if (x2 == lastx && y2 == lasty) return;
#if ZBLANK_PIN >= 0
  // Dunkel tasten, hart springen, im Dunkeln einschwingen lassen, Strahl an.
  zBlank(true);
  beamPoint(x2, y2, s_settleCycles);
  zBlank(false);
#else
  if (g_scopeProfile == 1 || g_jumpSpeed == 0) {
    // Harter Sprung wie im Original (ohne Verweilzeit)
    if (x2 != lastx) { lastx = x2; dacOutX((uint8_t)x2); }
    if (y2 != lasty) { lasty = y2; dacOutY((uint8_t)y2); }
    return;
  }
  // Schnell fahren, auf den letzten JUMP_RAMP Einheiten abbremsen
  const int x1 = lastx, y1 = lasty, dx = x2 - x1, dy = y2 - y1;
  const int32_t len16 = (int32_t)isqrt32((uint32_t)(dx * dx + dy * dy) << 8);
  int n = (len16 + 8) / 16;
  if (n < 1) n = 1;
  int32_t fx = x1 * 65536 + 0x8000, fy = y1 * 65536 + 0x8000;
  const int32_t ix = dx * 65536 / n, iy = dy * 65536 / n;
  for (int k = 1; k < n; k++) {
    fx += ix; fy += iy;
    const int r = n - k;                                 // Restweg in Einheiten
    beamPoint(fx >> 16, fy >> 16, s_jumpCycles[r < JUMP_RAMP ? r : JUMP_RAMP]);
  }
  beamPoint(x2, y2, s_jumpCycles[0]);
#endif
}

//*****************************************************************************
// Linienzüge ("Strokes")
//*****************************************************************************
// (Struktur "Stroke" siehe Dateianfang – sie muss dort stehen, weil die
// Arduino-IDE vor dem Kompilieren Funktionsprototypen an den Anfang setzt.)

static inline int strokeX(const Stroke &st, bool end) { return st.t[end ? st.b : st.a]; }
static inline int strokeY(const Stroke &st, bool end) { return st.t[(end ? st.b : st.a) + 1]; }

// Zeichnet einen Stroke: Sprung zum Startpunkt, dort einen normalen Schritt
// verweilen, dann alle Linien mit 'cycles' pro Schritt.
static void IRAM_ATTR drawStroke(const Stroke &st, uint32_t cycles)
{
  const byte *t = st.t;
  const int dir = (st.b >= st.a) ? 2 : -2;
  beamJump(t[st.a], t[st.a + 1]);
  beamPoint(t[st.a], t[st.a + 1], cycles);               // Startpunkt wie jeder andere Punkt
  int32_t carry16 = 0;                                    // Rundungsrest je Linienzug
  for (int k = st.a; k != st.b; k += dir)
    beamMove(t[k], t[k + 1], t[k + dir], t[k + dir + 1], cycles, &carry16);
}

// Sucht ab Byte-Offset i den nächsten Linienzug mit mindestens 2 Punkten.
// Liefert false am Tabellenende. Danach zeigt i hinter den Linienzug.
static bool IRAM_ATTR nextStroke(const byte *table, int size, byte sep, int &i, Stroke &out)
{
  while (i + 1 < size && table[i] != 0xFF) {
    if (table[i] == sep) { i++; continue; }              // (mehrere) Trenner überspringen
    int end = i;                                          // Punkte bei i, i+2, …, end-2
    while (end + 1 < size && table[end] != sep && table[end] != 0xFF) end += 2;
    const int start = i;
    i = end;
    if ((end - start) / 2 >= 2) {                         // Einzelpunkte wie im Original ignorieren
      out.t = table; out.a = start; out.b = end - 2;
      return true;
    }
  }
  return false;
}

//*****************************************************************************
// PlotTable – zeichnet Linienzüge aus einer Tabelle in DataTable.h
//*****************************************************************************
// Tabellenformat (unverändert aus v1.0, Details in DataTable.h):
//   Punkte (x, y) je 1 Byte; aufeinanderfolgende Punkte = Linie.
//   'sep' (0x00) trennt Linienzüge, 0xFF beendet die Tabelle bzw. bei den
//   Zeigern eine Zeigerstellung.
//
//   table       Tabelle
//   size        Länge in Bytes (sizeof)
//   sep         Trennwert: 0x00 bei Zifferblatt/Ziffern, 0xFF bei Zeigern
//   offset      Start in Bytes; bei Zeigern = Stellung × Eintragslänge
//   brightPct   Helligkeit in Prozent (100 = normal). Verkürzt bzw. verlängert
//               die Verweilzeit pro Schritt – für Motive, deren Linien so dicht
//               nebeneinander liegen, dass sie auf dem Schirm verschmelzen
//               (Stunden-/Minutenzeiger, siehe HAND_BRIGHT_PCT in config.h).
//
// Pro Linienzug: Sprung zum näheren Ende (schwache Rücklaufspur), dort einen
// normalen Schritt verweilen, dann alle Linien – ggf. in umgekehrter
// Reihenfolge (Regel 6; die Helligkeit ist richtungsunabhängig). Linienzüge
// aus nur einem Punkt werden wie im Original nicht gezeichnet. Die
// Tabellengrenze wird immer geprüft, auch wenn ein Endzeichen fehlt.
static void IRAM_ATTR PlotTable(const byte *table, int size, byte sep, int offset, int brightPct = 100)
{
  const uint32_t cycles = (uint32_t)((uint64_t)s_stepCycles * brightPct / 100);
  int i = offset;
  Stroke st;
  while (nextStroke(table, size, sep, i, st)) {
    // Näheres Ende wählen (quadratischer Abstand reicht zum Vergleich)
    const int ds = (strokeX(st, 0) - lastx) * (strokeX(st, 0) - lastx) + (strokeY(st, 0) - lasty) * (strokeY(st, 0) - lasty);
    const int de = (strokeX(st, 1) - lastx) * (strokeX(st, 1) - lastx) + (strokeY(st, 1) - lasty) * (strokeY(st, 1) - lasty);
    if (de < ds) { const int16_t tmp = st.a; st.a = st.b; st.b = tmp; }
    drawStroke(st, cycles);
  }
}

//*****************************************************************************
// Zifferblätter
//*****************************************************************************
// Liefert die Ziffern-Tabelle zum Zifferblatt-Stil und ihre Größe in Bytes:
//   0 = Ziffern 1–12          1 = nur 12, 3, 6, 9
//   2 = römisch I–XII         3 = nur XII, III, VI, IX
static const int DIAL_STYLES = 4;                         // Anzahl der Zifferblätter

static const uint8_t *dialDigits(int style, int &size)
{
  switch (style) {
    case 1:  size = sizeof(DialDigitsMin);    return DialDigitsMin;
    case 2:  size = sizeof(DialDigitsRoman);  return DialDigitsRoman;
    case 3:  size = sizeof(DialDigitsRoman4); return DialDigitsRoman4;
    default: size = sizeof(DialDigits12);    return DialDigits12;
  }
}

static const char *dialName(int style)
{
  switch (style) {
    case 1:  return "12/3/6/9";
    case 2:  return "roemisch I-XII";
    case 3:  return "roemisch XII/III/VI/IX";
    default: return "1-12";
  }
}

//*****************************************************************************
// Rundtour für Zifferblatt + Ziffern (beim Start und bei jedem Zifferblatt-
// Wechsel berechnet)
//*****************************************************************************
// Zifferblatt-Striche und Ziffern ändern sich nie. Ihre Reihenfolge und
// Zeichenrichtung werden deshalb einmal so optimiert, dass die Summe aller
// Sprünge möglichst klein ist – als geschlossene Runde, damit man an jeder
// Stelle einsteigen kann (dort, wo der Sekundenzeiger gerade endet).
//   1. Nächster-Nachbar: immer den Linienzug (und dessen Ende) nehmen, der
//      dem aktuellen Endpunkt am nächsten liegt.
//   2. 2-opt: Teilstücke der Runde umdrehen, solange das Sprünge spart.
static const int TOUR_MAX = 128;
static Stroke s_tour[TOUR_MAX];
static int    s_tourLen = 0;

static float tourGap(const Stroke &from, const Stroke &to)       // Sprung from.Ende → to.Anfang
{
  const float dx = strokeX(to, 0) - strokeX(from, 1), dy = strokeY(to, 0) - strokeY(from, 1);
  return sqrtf(dx * dx + dy * dy);
}

static void tourAdd(const byte *table, int size)
{
  int i = 0;
  Stroke st;
  while (s_tourLen < TOUR_MAX && nextStroke(table, size, 0x00, i, st)) s_tour[s_tourLen++] = st;
}

static void reverseStroke(Stroke &st) { const int16_t tmp = st.a; st.a = st.b; st.b = tmp; }

static void buildTour(const uint8_t *digits, int digitsSize)
{
  s_tourLen = 0;
  tourAdd(DialData, sizeof(DialData));
  tourAdd(digits, digitsSize);
  const int n = s_tourLen;
  if (n < 3) return;

  // 1. Nächster Nachbar
  for (int k = 1; k < n; k++) {
    int best = k; bool rev = false; float bestD = 1e9f;
    for (int j = k; j < n; j++) {
      float d = tourGap(s_tour[k - 1], s_tour[j]);
      if (d < bestD) { bestD = d; best = j; rev = false; }
      Stroke r = s_tour[j]; reverseStroke(r);
      d = tourGap(s_tour[k - 1], r);
      if (d < bestD) { bestD = d; best = j; rev = true; }
    }
    const Stroke tmp = s_tour[k]; s_tour[k] = s_tour[best]; s_tour[best] = tmp;
    if (rev) reverseStroke(s_tour[k]);
  }

  // 2. 2-opt auf der geschlossenen Runde: Abschnitt i+1 … j umdrehen
  //    (Reihenfolge UND Richtung jedes Linienzugs darin)
  for (int pass = 0; pass < 50; pass++) {
    bool improved = false;
    for (int i = 0; i < n - 1; i++) {
      for (int j = i + 1; j < n; j++) {
        const Stroke &A = s_tour[i], &B = s_tour[i + 1], &C = s_tour[j], &D = s_tour[(j + 1) % n];
        if ((j + 1) % n == i) continue;
        const float before = tourGap(A, B) + tourGap(C, D);
        Stroke Cr = C, Br = B; reverseStroke(Cr); reverseStroke(Br);
        const float after  = tourGap(A, Cr) + tourGap(Br, D);
        if (after + 0.01f < before) {
          for (int l = i + 1, r = j; l <= r; l++, r--) {
            Stroke tmp = s_tour[l]; s_tour[l] = s_tour[r]; s_tour[r] = tmp;
            reverseStroke(s_tour[l]);
            if (l != r) reverseStroke(s_tour[r]);
          }
          improved = true;
        }
      }
    }
    if (!improved) break;
  }

  float total = 0;
  for (int k = 0; k < n; k++) total += tourGap(s_tour[k], s_tour[(k + 1) % n]);
  logMsg("[Scope] Rundtour: %d Linienzuege, Spruenge gesamt %.0f DAC-Einheiten", n, total);
}

//*****************************************************************************
// scopeTask – Core 1: zeichnet endlos Zifferblatt + Zeiger
//*****************************************************************************
// Ablauf eines Bildes (Strahl startet in der Mitte):
//   1. Stundenzeiger, Minutenzeiger      (Rauten um die Mitte)
//   2. Sekundenzeiger Mitte → Spitze     (halbe Helligkeit)
//   3. Rundtour Zifferblatt + Ziffern    (Einstieg nahe der Sekundenzeiger-Spitze,
//                                          endet wieder dort)
//   4. Sekundenzeiger Spitze → Mitte     (halbe Helligkeit, ersetzt den Rücksprung)
// Danach beginnt sofort das nächste Bild.
//
// Zeigertabellen (DataTable.h) enthalten je 60 vorberechnete Stellungen:
//   HrPtrData, MinPtrData: 60 × 9 Byte  (4 Punkte = schmale Raute + 0xFF)
//   SecPtrData:            60 × 5 Byte  (2 Punkte Spitze, Mitte + 0xFF)
// Der Offset "Stellung × Eintragslänge" wählt die passende Stellung aus.
// IRAM_ATTR: Der Compiler setzt PlotTable()/nextStroke() hier inline ein –
// damit auch diese Bildschleife ohne Flash-Cache-Zugriffe gleichmäßig läuft.
static void IRAM_ATTR scopeTask(void *)
{
  // Dieser Task gibt Core 1 nie frei (kein vTaskDelay). Der Idle-Task von
  // Core 1 kommt daher nie dran – falls dessen Watchdog in der verwendeten
  // Core-Version aktiv ist, würde er auslösen. Deshalb hier abschalten.
  disableCore1WDT();
  dacInit();
  logMsg("[Scope] laeuft auf Core %d", xPortGetCoreID());

  uint32_t secCycles = s_stepCycles / 2;                  // Sekundenzeiger: 2 × halbe Helligkeit
  int builtDial = -1;                                     // Zifferblatt der aktuellen Rundtour
  uint16_t builtNs = g_beamNs;                            // Timing, mit dem gerade gezeichnet wird
  uint8_t  builtJump = g_jumpSpeed;

  for (;;) {
    // Zifferblatt gewechselt (Webinterface)? Rundtour zwischen zwei Bildern neu
    // berechnen. Dauert einige ms – der Strahl steht so lange (einmaliger Punkt).
    // Strahlgeschwindigkeit oder Rücklauf-Tempo geändert? Zwischen zwei Bildern übernehmen.
    if (g_beamNs != builtNs || g_jumpSpeed != builtJump) {
      builtNs = g_beamNs; builtJump = g_jumpSpeed;
      beamTiming(builtNs, builtJump);
      secCycles = s_stepCycles / 2;
    }

    const int dial = g_dialStyle;
    if (dial != builtDial) {
      int size;
      const uint8_t *digits = dialDigits(dial, size);
      buildTour(digits, size);
      builtDial = dial;
    }

    // Uhrzeit einmal pro Bild atomar lesen, damit alle drei Zeiger
    // garantiert zur selben Sekunde gehören.
    const uint32_t t = g_timePacked;
    const int h = (t >> 16) & 0xFF;
    const int m = (t >> 8)  & 0xFF;
    const int s =  t        & 0xFF;

    // Stundenzeiger in 60 Schritten: 5 Schritte pro Stunde, plus 1 Schritt
    // je 12 Minuten – er wandert also wie bei einer echten Uhr mit.
    const int hpos = (h % 12) * 5 + m / 12;

    // 1. Stunden- und Minutenzeiger
    const int handPct = g_handPct;
    PlotTable(HrPtrData,  sizeof(HrPtrData),  0xFF, 9 * hpos, handPct);
    PlotTable(MinPtrData, sizeof(MinPtrData), 0xFF, 9 * m,    handPct);

    // 2. Sekundenzeiger hin: Tabelle = [Spitze, Mitte] → Stroke Mitte→Spitze
    const Stroke secOut  = { SecPtrData, (int16_t)(5 * s + 2), (int16_t)(5 * s) };
    const Stroke secBack = { SecPtrData, (int16_t)(5 * s),     (int16_t)(5 * s + 2) };
    drawStroke(secOut, secCycles);

    // 3. Rundtour, Einstieg beim Linienzug, der der Zeigerspitze am nächsten liegt
    int start = 0, bestD = 1 << 30;
    for (int k = 0; k < s_tourLen; k++) {
      const int dx = strokeX(s_tour[k], 0) - lastx, dy = strokeY(s_tour[k], 0) - lasty;
      if (dx * dx + dy * dy < bestD) { bestD = dx * dx + dy * dy; start = k; }
    }
    for (int k = 0; k < s_tourLen; k++)
      drawStroke(s_tour[(start + k) % s_tourLen], s_stepCycles);

    // 4. Sekundenzeiger zurück zur Mitte (statt eines langen Rücksprungs)
    drawStroke(secBack, secCycles);
    g_frameCount = g_frameCount + 1;                     // für die Bildrate im Webinterface

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

// ---- Im Webinterface gespeicherte WLANs ---------------------------------------
// Zusätzlich zu secrets.h können im Webinterface bis zu WEB_MAX_NETWORKS Netze
// eingetragen werden. Sie liegen im Flash (NVS, Namensraum "scopeclock",
// Schlüssel netn, net_s0…, net_p0…) und bleiben auch nach einem Update erhalten.
static StoredNet s_webNets[WEB_MAX_NETWORKS];
static int       s_webNetCount = 0;

static void webNetsLoad()
{
  Preferences p;
  s_webNetCount = 0;
  if (!p.begin("scopeclock", true)) return;
  const int n = p.getUChar("netn", 0);
  for (int i = 0; i < n && i < WEB_MAX_NETWORKS; i++) {
    char k[12];
    snprintf(k, sizeof(k), "net_s%d", i); p.getString(k, s_webNets[s_webNetCount].ssid, sizeof(s_webNets[0].ssid));
    snprintf(k, sizeof(k), "net_p%d", i); p.getString(k, s_webNets[s_webNetCount].pw,   sizeof(s_webNets[0].pw));
    if (s_webNets[s_webNetCount].ssid[0]) s_webNetCount++;
  }
  p.end();
}

static void webNetsSave()
{
  Preferences p;
  if (!p.begin("scopeclock", false)) return;
  for (int i = 0; i < WEB_MAX_NETWORKS; i++) {
    char k[12];
    snprintf(k, sizeof(k), "net_s%d", i);
    if (i < s_webNetCount) p.putString(k, s_webNets[i].ssid); else p.remove(k);
    snprintf(k, sizeof(k), "net_p%d", i);
    if (i < s_webNetCount) p.putString(k, s_webNets[i].pw);   else p.remove(k);
  }
  p.putUChar("netn", (uint8_t)s_webNetCount);
  p.end();
}

static bool secretsNetConfigured()
{
  for (size_t i = 0; i < WIFI_NETWORK_COUNT; i++)
    if (WIFI_NETWORKS[i].ssid && WIFI_NETWORKS[i].ssid[0]) return true;
  return false;
}

// true, sobald mindestens ein WLAN bekannt ist (secrets.h oder Webinterface).
static bool wifiConfigured() { return secretsNetConfigured() || s_webNetCount > 0; }

// Hält die WLAN-Verbindung. Läuft als eigener Task, weil WiFiMulti::run()
// zuerst einen Scan macht und dabei mehrere Sekunden blockiert – im uiTask
// würde das OLED so lange stehen bleiben.
// Ablauf: nicht verbunden → alle Netze scannen → mit dem stärksten bekannten
// verbinden (max. 10 s) → alle 2 s Verbindung prüfen. Ohne bekanntes Netz
// wartet der Task, bis im Webinterface eins eingetragen wird.
static void wifiTask(void *)
{
  for (;;) {
    if (wifiConfigured() && WiFi.status() != WL_CONNECTED) {
      wifiMulti.run(10000);
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

static void webStart();                     // siehe Abschnitt "Webinterface"
static void oledSetContrast(uint8_t v);     // siehe Abschnitt "OLED"

// Initialisiert WLAN, NTP und das Webinterface. Muss vom uiTask (Core 0) aus
// aufgerufen werden: Der WLAN-Treiber legt seine Interrupts auf dem
// aufrufenden Kern an, und sie sollen den Zeichenpfad auf Core 1 nicht stören.
static void wifiStart()
{
  WiFi.mode(WIFI_STA);                 // Client; das Setup-WLAN kommt bei Bedarf dazu
  WiFi.setHostname(WIFI_HOSTNAME);     // Name in der Geräteliste des Routers

  // Alle bekannten Netze bei WiFiMulti anmelden (leere Einträge ignorieren)
  for (size_t i = 0; i < WIFI_NETWORK_COUNT; i++) {
    const WifiCredential &n = WIFI_NETWORKS[i];
    if (!n.ssid || !n.ssid[0]) continue;
    wifiMulti.addAP(n.ssid, n.password);
    logMsg("[WiFi] bekanntes Netz (secrets.h): \"%s\"", n.ssid);   // Passwort wird nie ausgegeben
  }
  webNetsLoad();
  for (int i = 0; i < s_webNetCount; i++) {
    wifiMulti.addAP(s_webNets[i].ssid, s_webNets[i].pw);
    logMsg("[WiFi] bekanntes Netz (Webinterface): \"%s\"", s_webNets[i].ssid);
  }
  if (!wifiConfigured())
    logMsg("[WiFi] noch kein WLAN eingetragen – Einrichtung ueber das Setup-WLAN \"%s\"", WEB_SETUP_AP_SSID);

  // NTP: configTzTime() setzt die Zeitzone (inkl. Sommerzeitregel) und startet
  // den SNTP-Client im Hintergrund. Er wartet selbstständig, bis das WLAN steht,
  // und synchronisiert danach alle NTP_SYNC_INTERVAL_MS erneut.
  sntp_set_time_sync_notification_cb(onNtpSync);
  sntp_set_sync_interval(NTP_SYNC_INTERVAL_MS);
  configTzTime(s_tz, s_ntpServer, NTP_SERVER_2, NTP_SERVER_3);

  xTaskCreatePinnedToCore(wifiTask, "wifi", 4096, nullptr, 1, &wifiTaskHandle, UI_CORE);
  webStart();
}

// Schreibt Statusänderungen ins Log (aufgerufen 1× pro Sekunde):
//  - WLAN verbunden (mit SSID, IP, Signal) / Verbindung verloren
//  - beim ersten Verbinden: mDNS starten (http://<WIFI_HOSTNAME>.local)
//  - jede NTP-Synchronisierung mit Datum und Uhrzeit
//  - einmalige Warnung, wenn der letzte Sync länger als 2 Intervalle her ist
static bool ntpOverdue()
{
  return g_ntpSynced && time(nullptr) - lastSyncEpoch() > (time_t)(2 * NTP_SYNC_INTERVAL_MS / 1000UL);
}

static void wifiLogChanges()
{
  static wl_status_t last = WL_IDLE_STATUS;
  static time_t lastLoggedSync = 0;
  static bool   overdueLogged  = false;
  static bool   mdnsStarted    = false;

  wl_status_t st = WiFi.status();
  if (st != last) {
    if (st == WL_CONNECTED) {
      logMsg("[WiFi] verbunden mit \"%s\", IP %s, RSSI %d dBm",
             WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI());
      if (!mdnsStarted && MDNS.begin(WIFI_HOSTNAME)) {
        MDNS.addService("http", "tcp", 80);
        mdnsStarted = true;
        logMsg("[Web] erreichbar unter http://%s.local und http://%s",
               WIFI_HOSTNAME, WiFi.localIP().toString().c_str());
      }
    } else if (last == WL_CONNECTED) {
      logMsg("[WiFi] Verbindung verloren – versuche erneut");
    }
    last = st;
  }

  // Neuer Sync erkannt = Zeitstempel hat sich seit der letzten Ausgabe geändert
  const time_t syncEpoch = lastSyncEpoch();
  if (g_ntpSynced && syncEpoch != lastLoggedSync) {
    struct tm st;
    localtime_r(&syncEpoch, &st);
    logMsg("[NTP] Sync %02d.%02d.%04d %02d:%02d:%02d",
           st.tm_mday, st.tm_mon + 1, st.tm_year + 1900, st.tm_hour, st.tm_min, st.tm_sec);
    lastLoggedSync = syncEpoch;
    overdueLogged  = false;
  }

  // Überfällig? (z.B. WLAN dauerhaft weg oder NTP-Server nicht erreichbar)
  if (!overdueLogged && ntpOverdue()) {
    logMsg("[NTP] Warnung: letzter Sync liegt mehr als 2 Intervalle zurueck");
    overdueLogged = true;
  }
}

//*****************************************************************************
// Webinterface (Core 0)
//*****************************************************************************
// Eine einzige Seite (webpage.h) plus kleine JSON-Schnittstellen. Der Server
// läuft im uiTask auf Core 0 (handleClient alle 10 ms) und beeinflusst die
// Strahlausgabe auf Core 1 nicht. Kein Passwort: Jeder im selben Netz kann
// die Seite öffnen und WLANs eintragen; gespeicherte Passwörter werden aber
// nie ausgegeben.
//
// Setup-WLAN (Captive Portal): Ist kein WLAN eingetragen oder seit
// WEB_SETUP_AP_AFTER_S Sekunden keine Verbindung möglich, öffnet die Uhr das
// offene WLAN WEB_SETUP_AP_SSID. Handy/Laptop zeigen dann automatisch die
// Einrichtungsseite (alle DNS-Anfragen zeigen auf die Uhr, 192.168.4.1).
// Steht die Verbindung zu einem bekannten WLAN 30 s lang, schließt das
// Setup-WLAN wieder.
static WebServer web(80);
static DNSServer dns;
static bool      s_apActive     = false;
static uint32_t  s_lastConnMs   = 0;    // zuletzt mit WLAN verbunden (0 = noch nie)
static uint32_t  s_connSinceMs  = 0;    // verbunden seit (0 = nicht verbunden)
static uint32_t  s_restartAtMs  = 0;    // geplanter Neustart (0 = keiner)
static uint32_t  s_fps          = 0;    // Bildrate des Oszilloskop-Bildes

// Hängt s als JSON-String (mit Anführungszeichen, Sonderzeichen maskiert) an out an.
static void jsonStr(String &out, const char *s)
{
  out += '"';
  for (; *s; s++) {
    const char c = *s;
    if (c == '"' || c == '\\') { out += '\\'; out += c; }
    else if ((uint8_t)c < 0x20) { char b[8]; snprintf(b, sizeof(b), "\\u%04x", c); out += b; }
    else out += c;
  }
  out += '"';
}

static void sendJsonResult(bool ok, const char *msg)
{
  String o = "{\"ok\":";
  o += ok ? "true" : "false";
  o += ",\"msg\":";
  jsonStr(o, msg);
  o += '}';
  web.send(200, "application/json", o);
}

// GET /api/status – alles, was die Statusanzeige der Seite braucht
static void handleStatus()
{
  struct tm t;
  const bool valid = currentTime(t);
  char b[64];
  String o;
  o.reserve(700);
  snprintf(b, sizeof(b), "{\"time\":\"%02d:%02d:%02d\",\"date\":", t.tm_hour, t.tm_min, t.tm_sec);
  o += b;
  if (valid) { snprintf(b, sizeof(b), "\"%02d.%02d.%04d\",\"kw\":%d", t.tm_mday, t.tm_mon + 1, t.tm_year + 1900, isoWeek(t)); o += b; }
  else o += "null,\"kw\":null";

  o += ",\"ntp\":{\"synced\":"; o += g_ntpSynced ? "true" : "false";
  o += ",\"overdue\":";         o += ntpOverdue() ? "true" : "false";
  o += ",\"last\":";
  if (g_ntpSynced) {
    const time_t e = lastSyncEpoch(); struct tm st; localtime_r(&e, &st);
    snprintf(b, sizeof(b), "\"%02d.%02d. %02d:%02d:%02d\"", st.tm_mday, st.tm_mon + 1, st.tm_hour, st.tm_min, st.tm_sec);
    o += b;
  } else o += "null";
  o += ",\"server\":"; jsonStr(o, NTP_SERVER_1); o += '}';

  const bool conn = WiFi.status() == WL_CONNECTED;
  o += ",\"wifi\":{\"configured\":"; o += wifiConfigured() ? "true" : "false";
  o += ",\"connected\":";            o += conn ? "true" : "false";
  o += ",\"ssid\":";                 jsonStr(o, conn ? WiFi.SSID().c_str() : "");
  o += ",\"rssi\":";                 o += conn ? WiFi.RSSI() : 0;
  o += ",\"ip\":";                   jsonStr(o, conn ? WiFi.localIP().toString().c_str() : "");
  o += ",\"host\":";                 jsonStr(o, WIFI_HOSTNAME ".local");
  o += "},\"ap\":{\"active\":";      o += s_apActive ? "true" : "false";
  o += ",\"ssid\":";                 jsonStr(o, WEB_SETUP_AP_SSID);
  o += ",\"ip\":";                   jsonStr(o, WiFi.softAPIP().toString().c_str());
  o += "},\"profile\":";             jsonStr(o, profileName(g_scopeProfile));
  o += ",\"dial\":";                 o += (int)g_dialStyle;
  o += ",\"fps\":";                  o += s_fps;
  o += ",\"uptime\":";               o += (uint32_t)(millis() / 1000UL);
  o += ",\"heap\":";                 o += (uint32_t)ESP.getFreeHeap();
  o += ",\"fw\":";                   jsonStr(o, FW_VERSION);
  o += '}';
  web.send(200, "application/json", o);
}

// Hängt die Linienzüge einer Tabelle als JSON an: [[x0,y0,x1,y1,…],[…],…]
// (bei onlyOne nur den ersten Linienzug ab offset – für eine Zeigerstellung).
static void appendStrokes(String &o, const uint8_t *t, int size, uint8_t sep, int offset, bool onlyOne)
{
  int i = offset;
  Stroke st;
  bool first = true;
  o += '[';
  while (nextStroke(t, size, sep, i, st)) {
    if (!first) o += ',';
    first = false;
    o += '[';
    const int dir = (st.b >= st.a) ? 2 : -2;
    for (int k = st.a; ; k += dir) {
      o += t[k]; o += ','; o += t[k + 1];
      if (k == st.b) break;
      o += ',';
    }
    o += ']';
    if (onlyOne) break;
  }
  o += ']';
}

// Hängt alle 60 Stellungen einer Zeigertabelle an: [[Stellung 0],[1],…,[59]]
static void appendHand(String &o, const uint8_t *t, int size, int stride)
{
  o += '[';
  for (int pos = 0; pos < 60; pos++) {
    if (pos) o += ',';
    String one;
    appendStrokes(one, t, size, 0xFF, pos * stride, true);
    o += one.substring(1, one.length() - 1);              // äußere Klammern weg
  }
  o += ']';
}

// GET /api/vectors – Vektordaten für die Oszilloskop-Vorschau der Webseite:
// dieselben Tabellen, die auch der scopeTask zeichnet (Zifferblatt, alle drei
// Ziffernsätze (Index = Stil 0–3), alle 60 Stellungen der drei Zeiger). Die Seite lädt sie
// einmal und stellt die Zeiger dann selbst nach der Uhrzeit (wie scopeTask).
// Koordinaten 0…255, y nach oben. Ca. 9 KB.
static void handleVectors()
{
  String o;
  o.reserve(9000);
  o += "{\"dial\":";   appendStrokes(o, DialData, sizeof(DialData), 0x00, 0, false);
  o += ",\"digits\":[";                                 // alle Zifferblätter (Index = Stil)
  for (int st = 0; st < DIAL_STYLES; st++) {
    int size;
    const uint8_t *d = dialDigits(st, size);
    if (st) o += ',';
    appendStrokes(o, d, size, 0x00, 0, false);
  }
  o += ']';
  o += ",\"hr\":";  appendHand(o, HrPtrData,  sizeof(HrPtrData),  9);
  o += ",\"min\":"; appendHand(o, MinPtrData, sizeof(MinPtrData), 9);
  o += ",\"sec\":"; appendHand(o, SecPtrData, sizeof(SecPtrData), 5);
  o += '}';
  web.sendHeader("Cache-Control", "max-age=3600");
  web.send(200, "application/json", o);
}

// GET /api/networks – gespeicherte Netze (nur Namen, nie Passwörter)
static void handleNetworks()
{
  String o = "[";
  bool first = true;
  for (size_t i = 0; i < WIFI_NETWORK_COUNT; i++) {
    if (!WIFI_NETWORKS[i].ssid || !WIFI_NETWORKS[i].ssid[0]) continue;
    if (!first) o += ',';
    first = false;
    o += "{\"ssid\":"; jsonStr(o, WIFI_NETWORKS[i].ssid); o += ",\"source\":\"secrets\"}";
  }
  for (int i = 0; i < s_webNetCount; i++) {
    if (!first) o += ',';
    first = false;
    o += "{\"ssid\":"; jsonStr(o, s_webNets[i].ssid); o += ",\"source\":\"web\"}";
  }
  o += ']';
  web.send(200, "application/json", o);
}

// GET /api/scan – startet beim ersten Aufruf eine Suche im Hintergrund
// ({"running":true}); die Seite fragt dann nach, bis die Liste da ist.
// Gleiche Namen werden zusammengefasst (stärkstes Signal), sortiert nach Signal.
static void handleScan()
{
  const int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) { web.send(200, "application/json", "{\"running\":true}"); return; }
  if (n < 0) { WiFi.scanNetworks(true); web.send(200, "application/json", "{\"running\":true}"); return; }

  const int MAXN = 20;
  int idx[MAXN]; int cnt = 0;
  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i).length() == 0) continue;               // versteckte Netze
    int dup = -1;
    for (int k = 0; k < cnt; k++) if (WiFi.SSID(idx[k]) == WiFi.SSID(i)) dup = k;
    if (dup >= 0) { if (WiFi.RSSI(i) > WiFi.RSSI(idx[dup])) idx[dup] = i; continue; }
    if (cnt < MAXN) idx[cnt++] = i;
  }
  for (int a = 1; a < cnt; a++)                              // nach Signal sortieren
    for (int k = a; k > 0 && WiFi.RSSI(idx[k]) > WiFi.RSSI(idx[k - 1]); k--) { int t = idx[k]; idx[k] = idx[k - 1]; idx[k - 1] = t; }

  String o = "{\"running\":false,\"networks\":[";
  for (int k = 0; k < cnt; k++) {
    if (k) o += ',';
    o += "{\"ssid\":"; jsonStr(o, WiFi.SSID(idx[k]).c_str());
    o += ",\"rssi\":"; o += WiFi.RSSI(idx[k]);
    o += ",\"open\":"; o += WiFi.encryptionType(idx[k]) == WIFI_AUTH_OPEN ? "true" : "false";
    o += '}';
  }
  o += "]}";
  WiFi.scanDelete();
  web.send(200, "application/json", o);
}

// POST /api/wifi (ssid, pw) – Netz speichern bzw. Passwort ändern
static void handleWifiSave()
{
  const String ssid = web.arg("ssid");
  const String pw   = web.arg("pw");
  if (ssid.length() < 1 || ssid.length() > 32) { sendJsonResult(false, "Netzwerkname: 1 bis 32 Zeichen"); return; }
  if (pw.length() > 0 && (pw.length() < 8 || pw.length() > 63)) { sendJsonResult(false, "Passwort: 8 bis 63 Zeichen (oder leer für offene Netze)"); return; }

  int slot = -1;
  for (int i = 0; i < s_webNetCount; i++) if (ssid == s_webNets[i].ssid) slot = i;   // vorhandenes ersetzen
  const bool isNew = slot < 0;
  if (isNew) {
    if (s_webNetCount >= WEB_MAX_NETWORKS) { sendJsonResult(false, "Speicher voll – bitte zuerst ein Netz löschen"); return; }
    slot = s_webNetCount++;
  }
  strlcpy(s_webNets[slot].ssid, ssid.c_str(), sizeof(s_webNets[slot].ssid));
  strlcpy(s_webNets[slot].pw,   pw.c_str(),   sizeof(s_webNets[slot].pw));
  webNetsSave();
  wifiMulti.addAP(s_webNets[slot].ssid, s_webNets[slot].pw);   // ab sofort bekannt
  logMsg("[Web] WLAN \"%s\" %s", s_webNets[slot].ssid, isNew ? "gespeichert" : "aktualisiert");

  if (WiFi.status() == WL_CONNECTED)
    sendJsonResult(true, "Gespeichert. Wird beim nächsten Verbindungsaufbau genutzt – oder „Neu verbinden“ antippen.");
  else
    sendJsonResult(true, "Gespeichert. Die Uhr verbindet sich jetzt …");
}

// POST /api/wifi/delete (ssid) – nur im Webinterface gespeicherte Netze.
// WiFiMulti kann Netze nicht wieder vergessen, deshalb startet die Uhr neu.
static void handleWifiDelete()
{
  const String ssid = web.arg("ssid");
  int slot = -1;
  for (int i = 0; i < s_webNetCount; i++) if (ssid == s_webNets[i].ssid) slot = i;
  if (slot < 0) { sendJsonResult(false, "Dieses Netz ist nicht im Webinterface gespeichert"); return; }
  for (int i = slot; i < s_webNetCount - 1; i++) s_webNets[i] = s_webNets[i + 1];
  s_webNetCount--;
  webNetsSave();
  logMsg("[Web] WLAN \"%s\" geloescht – Neustart", ssid.c_str());
  sendJsonResult(true, "Gelöscht. Die Uhr startet neu – Seite in ca. 10 s neu laden.");
  s_restartAtMs = millis() + 1500;
}

// POST /api/dial (style = 0 … DIAL_STYLES-1, siehe dialDigits) – Zifferblatt umschalten: gilt sofort
// für das Oszilloskop (scopeTask baut die Rundtour neu) und wird gespeichert.
static void handleDial()
{
  const int st = web.arg("style").toInt();
  if (!web.hasArg("style") || st < 0 || st >= DIAL_STYLES) { sendJsonResult(false, "style muss 0 bis 3 sein"); return; }
  if (st != g_dialStyle) {
    g_dialStyle = (uint8_t)st;
    saveSetting("dial", (uint8_t)st);
    logMsg("[Scope] Zifferblatt: %s, gespeichert", dialName(st));
  }
  String o = "{\"ok\":true,\"dial\":";
  o += st;
  o += '}';
  web.send(200, "application/json", o);
}

//*****************************************************************************
// Einstellungen (Webinterface → "Einstellungen")
//*****************************************************************************
// Alle Werte wirken sofort und liegen im Flash (NVS, Namensraum "scopeclock").
// config.h liefert nur die Startwerte; "Auf Standard zurücksetzen" löscht die
// gespeicherten Werte wieder. WLANs sind davon nicht betroffen.
//
//   Schlüssel  Wert                    Bereich
//   profile    Oszilloskop-Profil      0 = ANALOG, 1 = DIGITAL
//   dial       Zifferblatt             0…3
//   beam       Strahlgeschwindigkeit   1000…10000 ns pro Schritt
//   hand       Zeiger-Helligkeit       30…100 %
//   jump       Rücklauf-Tempo          0 (hart) … 48 (nur Profil ANALOG)
//   tz         Zeitzone                eine aus TIMEZONES[] bzw. TZ_INFO
//   ntp        erster NTP-Server       Hostname, max. 63 Zeichen
//   oled       OLED-Helligkeit         1…255
//   shift      OLED-Pixel-Shift        0/1

struct TzOption { const char *label; const char *tz; };
static const TzOption TIMEZONES[] = {
  { "Mitteleuropa (Berlin, Wien, Zürich)", "CET-1CEST,M3.5.0,M10.5.0/3" },
  { "Westeuropa (London, Lissabon)",       "GMT0BST,M3.5.0/1,M10.5.0" },
  { "Osteuropa (Helsinki, Athen)",         "EET-2EEST,M3.5.0/3,M10.5.0/4" },
  { "Moskau",                              "MSK-3" },
  { "UTC",                                 "UTC0" },
  { "USA Ost (New York)",                  "EST5EDT,M3.2.0,M11.1.0" },
  { "USA Zentral (Chicago)",               "CST6CDT,M3.2.0,M11.1.0" },
  { "USA Mountain (Denver)",               "MST7MDT,M3.2.0,M11.1.0" },
  { "USA West (Los Angeles)",              "PST8PDT,M3.2.0,M11.1.0" },
  { "Indien (Delhi)",                      "IST-5:30" },
  { "China (Peking)",                      "CST-8" },
  { "Japan (Tokio)",                       "JST-9" },
  { "Australien Ost (Sydney)",             "AEST-10AEDT,M10.1.0,M4.1.0/3" },
};
static const int TIMEZONE_COUNT = sizeof(TIMEZONES) / sizeof(TIMEZONES[0]);

static bool tzAllowed(const char *tz)
{
  if (!strcmp(tz, TZ_INFO)) return true;
  for (int i = 0; i < TIMEZONE_COUNT; i++) if (!strcmp(tz, TIMEZONES[i].tz)) return true;
  return false;
}

static bool hostnameValid(const String &h)
{
  if (h.length() < 1 || h.length() > 63) return false;
  for (size_t i = 0; i < h.length(); i++) {
    const char c = h[i];
    if (!isalnum((unsigned char)c) && c != '.' && c != '-') return false;
  }
  return true;
}

// Liest alle Einstellungen aus dem Flash (in setup(), vor dem Start der Tasks).
static void loadSettings()
{
  g_scopeProfile = loadSetting("profile", SCOPE_PROFILE, 1);
  g_dialStyle    = loadSetting("dial", DIAL_STYLE, DIAL_STYLES - 1);
  Preferences p;
  if (!p.begin("scopeclock", true)) return;
  const uint16_t beam = p.getUShort("beam", BEAM_NS_PER_UNIT);
  g_beamNs    = (beam >= 1000 && beam <= 10000) ? beam : BEAM_NS_PER_UNIT;
  const uint8_t hand = p.getUChar("hand", HAND_BRIGHT_PCT);
  g_handPct   = (hand >= 30 && hand <= 100) ? hand : HAND_BRIGHT_PCT;
  const uint8_t jump = p.getUChar("jump", JUMP_SPEED);
  g_jumpSpeed = jump <= 48 ? jump : JUMP_SPEED;
  char buf[64];
  if (p.getString("tz", buf, sizeof(buf)) && tzAllowed(buf)) strlcpy(s_tz, buf, sizeof(s_tz));
  if (p.getString("ntp", buf, sizeof(buf)) && hostnameValid(String(buf))) strlcpy(s_ntpServer, buf, sizeof(s_ntpServer));
  const uint8_t oled = p.getUChar("oled", OLED_CONTRAST);
  s_oledContrast = oled ? oled : OLED_CONTRAST;
  s_oledShift    = p.getUChar("shift", OLED_SHIFT_EVERY_S > 0) != 0;
  p.end();
}

// Aktuelle Einstellungen, Standardwerte und Zeitzonen als JSON.
static void settingsJson(String &o)
{
  o += "{\"profile\":";      o += (int)g_scopeProfile;
  o += ",\"dial\":";         o += (int)g_dialStyle;
  o += ",\"beamNs\":";       o += (int)g_beamNs;
  o += ",\"handPct\":";      o += (int)g_handPct;
  o += ",\"jumpSpeed\":";    o += (int)g_jumpSpeed;
  o += ",\"tz\":";           jsonStr(o, s_tz);
  o += ",\"ntp\":";          jsonStr(o, s_ntpServer);
  o += ",\"oledContrast\":"; o += (int)s_oledContrast;
  o += ",\"oledShift\":";    o += s_oledShift ? "true" : "false";
  o += ",\"defaults\":{\"profile\":"; o += SCOPE_PROFILE;
  o += ",\"dial\":";         o += DIAL_STYLE;
  o += ",\"beamNs\":";       o += BEAM_NS_PER_UNIT;
  o += ",\"handPct\":";      o += HAND_BRIGHT_PCT;
  o += ",\"jumpSpeed\":";    o += JUMP_SPEED;
  o += ",\"tz\":";           jsonStr(o, TZ_INFO);
  o += ",\"ntp\":";          jsonStr(o, NTP_SERVER_1);
  o += ",\"oledContrast\":"; o += OLED_CONTRAST;
  o += ",\"oledShift\":";    o += OLED_SHIFT_EVERY_S > 0 ? "true" : "false";
  o += "},\"timezones\":[";
  bool cfgListed = false;
  for (int i = 0; i < TIMEZONE_COUNT; i++) {
    if (i) o += ',';
    o += "{\"label\":"; jsonStr(o, TIMEZONES[i].label); o += ",\"tz\":"; jsonStr(o, TIMEZONES[i].tz); o += '}';
    if (!strcmp(TIMEZONES[i].tz, TZ_INFO)) cfgListed = true;
  }
  if (!cfgListed) { o += ",{\"label\":\"aus config.h\",\"tz\":"; jsonStr(o, TZ_INFO); o += '}'; }
  o += "]}";
}

static void sendSettings(bool ok, const char *msg)
{
  String o = "{\"ok\":";
  o += ok ? "true" : "false";
  o += ",\"msg\":"; jsonStr(o, msg);
  o += ",\"settings\":"; settingsJson(o);
  o += '}';
  web.send(200, "application/json", o);
}

// Zeitzone und NTP-Server übernehmen (startet den SNTP-Client neu)
static void applyTime()
{
  configTzTime(s_tz, s_ntpServer, NTP_SERVER_2, NTP_SERVER_3);
}

// GET /api/settings
static void handleSettingsGet() { sendSettings(true, ""); }

// POST /api/settings – beliebige Teilmenge der Felder; jedes Feld wird geprüft,
// sofort angewendet und gespeichert.
static void handleSettingsPost()
{
  Preferences p;
  if (!p.begin("scopeclock", false)) { sendSettings(false, "Flash-Speicher nicht verfügbar"); return; }
  String err;
  int changed = 0;
  auto bad = [&](const char *m) { if (err.length()) err += "; "; err += m; };

  if (web.hasArg("profile")) {
    const int v = web.arg("profile").toInt();
    if (v == 0 || v == 1) { g_scopeProfile = v; p.putUChar("profile", v); changed++;
      logMsg("[Web] Oszilloskop-Profil: %s", profileName(v)); }
    else bad("Profil ungültig");
  }
  if (web.hasArg("dial")) {
    const int v = web.arg("dial").toInt();
    if (v >= 0 && v < DIAL_STYLES) { g_dialStyle = v; p.putUChar("dial", v); changed++;
      logMsg("[Web] Zifferblatt: %s", dialName(v)); }
    else bad("Zifferblatt ungültig");
  }
  if (web.hasArg("beamNs")) {
    const int v = web.arg("beamNs").toInt();
    if (v >= 1000 && v <= 10000) { g_beamNs = v; p.putUShort("beam", v); changed++;
      logMsg("[Web] Strahlgeschwindigkeit: %d ns pro Schritt", v); }
    else bad("Strahlgeschwindigkeit: 1000 bis 10000 ns");
  }
  if (web.hasArg("handPct")) {
    const int v = web.arg("handPct").toInt();
    if (v >= 30 && v <= 100) { g_handPct = v; p.putUChar("hand", v); changed++;
      logMsg("[Web] Zeiger-Helligkeit: %d %%", v); }
    else bad("Zeiger-Helligkeit: 30 bis 100 %");
  }
  if (web.hasArg("jumpSpeed")) {
    const int v = web.arg("jumpSpeed").toInt();
    if (v >= 0 && v <= 48) { g_jumpSpeed = v; p.putUChar("jump", v); changed++;
      logMsg("[Web] Ruecklauf-Tempo: %d%s", v, v ? "x" : " (hart)"); }
    else bad("Rücklauf-Tempo: 0 bis 48");
  }
  bool timeChanged = false;
  if (web.hasArg("tz")) {
    const String v = web.arg("tz");
    if (tzAllowed(v.c_str())) { strlcpy(s_tz, v.c_str(), sizeof(s_tz)); p.putString("tz", s_tz); changed++; timeChanged = true;
      logMsg("[Web] Zeitzone: %s", s_tz); }
    else bad("Zeitzone unbekannt");
  }
  if (web.hasArg("ntp")) {
    const String v = web.arg("ntp");
    if (hostnameValid(v)) { strlcpy(s_ntpServer, v.c_str(), sizeof(s_ntpServer)); p.putString("ntp", s_ntpServer); changed++; timeChanged = true;
      logMsg("[Web] NTP-Server: %s", s_ntpServer); }
    else bad("NTP-Server: nur Buchstaben, Ziffern, Punkt und Bindestrich (max. 63)");
  }
  if (web.hasArg("oledContrast")) {
    const int v = web.arg("oledContrast").toInt();
    if (v >= 1 && v <= 255) { s_oledContrast = v; oledSetContrast(v); p.putUChar("oled", v); changed++; }
    else bad("OLED-Helligkeit: 1 bis 255");
  }
  if (web.hasArg("oledShift")) {
    s_oledShift = web.arg("oledShift") == "1" || web.arg("oledShift") == "true";
    p.putUChar("shift", s_oledShift); changed++;
  }
  p.end();
  if (timeChanged) applyTime();

  if (err.length()) sendSettings(false, err.c_str());
  else sendSettings(true, changed ? "Gespeichert" : "Keine Änderung");
}

// POST /api/settings/reset – alle Einstellungen auf die Werte aus config.h
static void handleSettingsReset()
{
  Preferences p;
  if (p.begin("scopeclock", false)) {
    for (const char *k : { "profile", "dial", "beam", "hand", "jump", "tz", "ntp", "oled", "shift" }) p.remove(k);
    p.end();
  }
  g_scopeProfile = SCOPE_PROFILE;  g_dialStyle = DIAL_STYLE;
  g_beamNs = BEAM_NS_PER_UNIT;     g_handPct = HAND_BRIGHT_PCT;   g_jumpSpeed = JUMP_SPEED;
  strlcpy(s_tz, TZ_INFO, sizeof(s_tz));
  strlcpy(s_ntpServer, NTP_SERVER_1, sizeof(s_ntpServer));
  s_oledContrast = OLED_CONTRAST;  oledSetContrast(s_oledContrast);
  s_oledShift = OLED_SHIFT_EVERY_S > 0;
  applyTime();
  logMsg("[Web] Einstellungen auf Standard zurueckgesetzt");
  sendSettings(true, "Auf Standard zurückgesetzt");
}

// POST /api/reconnect – Verbindung trennen; wifiTask verbindet sich dann mit
// dem stärksten bekannten Netz (z.B. nach dem Eintragen eines neuen Netzes)
static void handleReconnect()
{
  if (!wifiConfigured()) { sendJsonResult(false, "Noch kein WLAN eingetragen"); return; }
  logMsg("[Web] Neu verbinden angefordert");
  sendJsonResult(true, "Verbinde neu … die Seite ist kurz nicht erreichbar.");
  web.client().flush();
  delay(100);
  WiFi.disconnect(false, false);
}

// Unbekannte Adressen: im Setup-WLAN auf die Einrichtungsseite umleiten
// (so erkennen Handys das Captive Portal), sonst ebenfalls zur Startseite.
static void handleNotFound()
{
  const String target = s_apActive && web.hostHeader() != WiFi.softAPIP().toString()
                        ? String("http://") + WiFi.softAPIP().toString() + "/"
                        : String("/");
  web.sendHeader("Location", target, true);
  web.send(302, "text/plain", "");
}

static void webStart()
{
  web.on("/",                HTTP_GET,  [] { web.send_P(200, "text/html; charset=utf-8", INDEX_HTML); });
  web.on("/api/status",      HTTP_GET,  handleStatus);
  web.on("/api/log",         HTTP_GET,  [] { web.send(200, "text/plain; charset=utf-8", logRead()); });
  web.on("/api/networks",    HTTP_GET,  handleNetworks);
  web.on("/api/vectors",     HTTP_GET,  handleVectors);
  web.on("/api/scan",        HTTP_GET,  handleScan);
  web.on("/api/wifi",        HTTP_POST, handleWifiSave);
  web.on("/api/wifi/delete", HTTP_POST, handleWifiDelete);
  web.on("/api/reconnect",   HTTP_POST, handleReconnect);
  web.on("/api/dial",        HTTP_POST, handleDial);
  web.on("/api/settings",       HTTP_GET,  handleSettingsGet);
  web.on("/api/settings",       HTTP_POST, handleSettingsPost);
  web.on("/api/settings/reset", HTTP_POST, handleSettingsReset);
  web.onNotFound(handleNotFound);
  web.begin();
  logMsg("[Web] Webserver gestartet (Port 80)");
}

// Setup-WLAN ein-/ausschalten (aufgerufen 1× pro Sekunde, siehe oben).
static void webApManage()
{
  const uint32_t now  = millis();
  const bool     conn = WiFi.status() == WL_CONNECTED;
  if (conn) { s_lastConnMs = now; if (!s_connSinceMs) s_connSinceMs = now; }
  else      { s_connSinceMs = 0; }

  const bool want = !conn && (!wifiConfigured() || now - s_lastConnMs >= WEB_SETUP_AP_AFTER_S * 1000UL);
  if (want && !s_apActive) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(WEB_SETUP_AP_SSID);                      // offenes WLAN, ohne Passwort
    dns.start(53, "*", WiFi.softAPIP());                 // jeder Name → Uhr (Captive Portal)
    s_apActive = true;
    logMsg("[Web] Setup-WLAN \"%s\" aktiv – Einrichtung unter http://%s",
           WEB_SETUP_AP_SSID, WiFi.softAPIP().toString().c_str());
  } else if (s_apActive && conn && now - s_connSinceMs >= 30000UL) {
    dns.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    s_apActive = false;
    logMsg("[Web] Setup-WLAN beendet (verbunden mit \"%s\")", WiFi.SSID().c_str());
  }
}

// Wird im uiTask alle 10 ms aufgerufen.
static void webLoop()
{
  if (s_apActive) dns.processNextRequest();
  web.handleClient();
  if (s_restartAtMs && (int32_t)(millis() - s_restartAtMs) >= 0) ESP.restart();
}

//*****************************************************************************
// OLED – Waveshare 2.42" SSD1309 128x64, I2C (Core 0)
//*****************************************************************************
// "_F_" = voller Framebuffer (1 KB RAM): Das Bild wird komplett im RAM
// aufgebaut und mit sendBuffer() in einem Rutsch übertragen (~25 ms bei
// 400 kHz) – kein Flackern beim Neuzeichnen.
// Falls das Bild um 2 Pixel verschoben erscheint: NONAME0 durch NONAME2 ersetzen.
static U8G2_SSD1309_128X64_NONAME0_F_HW_I2C u8g2(U8G2_R0, OLED_RST_PIN, OLED_SCL_PIN, OLED_SDA_PIN);

// OLED-Helligkeit setzen (Einstellungen im Webinterface; läuft im uiTask).
static void oledSetContrast(uint8_t v) { u8g2.setContrast(v); }

// Display initialisieren und kurz einen Startbildschirm zeigen.
// Wird im uiTask aufgerufen, damit der I2C-Interrupt auf Core 0 liegt.
static void oledInit()
{
  u8g2.setI2CAddress(OLED_I2C_ADDR << 1);   // U8g2 erwartet die 8-Bit-Adresse
  u8g2.setBusClock(OLED_I2C_HZ);
  u8g2.begin();
  u8g2.setContrast(s_oledContrast);
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
// ---- BOOT-Taster: Oszilloskop-Profil umschalten ------------------------------
// BOOT-Taster des DevKitC (GPIO0, gedrückt = LOW) mindestens BUTTON_LONG_MS
// halten → Profil wechselt (analog ↔ digital), wird im Flash gespeichert und
// gilt sofort, auch nach dem nächsten Neustart. Während des Haltens zeigt
// das OLED den Wechsel mit Countdown; Loslassen vorher bricht ab.
// Achtung: GPIO0 ist ein Boot-Pin. Beim EINSCHALTEN bzw. Reset gedrückt
// gehalten, startet der ESP32 im Download-Modus statt der Uhr.
static uint32_t s_btnDownSince = 0;      // millis() beim Drücken, 0 = losgelassen
static bool     s_btnHandled   = false;  // Umschaltung für diesen Tastendruck erledigt

static const char *profileName(uint8_t p)  { return p == 1 ? "DIGITAL" : "ANALOG"; }

// Einstellungen im Flash (NVS, Namensraum "scopeclock"): "profile", "dial".
// Schreiben hält Core 1 für wenige ms an (einmaliger kurzer Lichtpunkt auf
// dem Oszilloskop) – passiert nur beim Umschalten.
static void saveSetting(const char *key, uint8_t v)
{
  Preferences prefs;
  if (prefs.begin("scopeclock", false)) {
    prefs.putUChar(key, v);
    prefs.end();
  }
}

// Liest eine Einstellung; ungültige Werte (> maxValid) ergeben den Standard.
static uint8_t loadSetting(const char *key, uint8_t def, uint8_t maxValid)
{
  Preferences prefs;
  uint8_t v = def;
  if (prefs.begin("scopeclock", true)) {      // nur lesen
    v = prefs.getUChar(key, def);
    prefs.end();
  }
  return v > maxValid ? def : v;
}

// Wird alle 10 ms aus dem uiTask aufgerufen.
static void buttonPoll()
{
  const bool down = digitalRead(BOOT_BUTTON_PIN) == LOW;
  const uint32_t now = millis();
  if (!down) { s_btnDownSince = 0; s_btnHandled = false; return; }
  if (s_btnDownSince == 0) { s_btnDownSince = now ? now : 1; return; }
  if (!s_btnHandled && now - s_btnDownSince >= BUTTON_LONG_MS) {
    s_btnHandled = true;
    const uint8_t p = g_scopeProfile ? 0 : 1;
    g_scopeProfile = p;                       // gilt ab dem nächsten Sprung
    saveSetting("profile", p);
    logMsg("[Scope] Profil umgeschaltet: %s, gespeichert", profileName(p));
  }
}

// Umschaltbildschirm statt der normalen Anzeige, solange der BOOT-Taster
// gehalten wird (ab 0,3 s bis zur Umschaltung). Liefert true, wenn gezeichnet.
// Nach der Umschaltung erscheint sofort wieder die normale Anzeige.
//
//      Oszilloskop-Profil
//      DIGITAL > ANALOG       (fett)
//              3              (Countdown 3, 2, 1 als 7-Segment-Ziffer,
//                              9x16 px, etwas kleiner als die Uhrzeit)
static bool oledProfileOverlay()
{
  const uint32_t now = millis();
  const bool holding = s_btnDownSince && !s_btnHandled && now - s_btnDownSince >= 300;
  if (!holding) return false;

  const uint8_t  cur  = g_scopeProfile;
  const uint8_t  next = cur ? 0 : 1;
  const uint32_t left = (BUTTON_LONG_MS - (now - s_btnDownSince) + 999) / 1000;   // aufgerundet
  char buf[24];

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  drawCentered("Oszilloskop-Profil", 11, 1);                       // 18 Zeichen = 108 px
  u8g2.setFont(u8g2_font_7x14B_tf);
  snprintf(buf, sizeof(buf), "%s > %s", profileName(cur), profileName(next));
  drawCentered(buf, 30, 1);                                        // max. 15 Zeichen = 105 px
  // Countdown-Ziffer: 9x16 px, Segmentstärke 2 px, zentriert.
  // Eine "1" besteht nur aus den rechten Segmenten – für die Mitte nach links rücken.
  const int CW = 9, CH = 16, CT = 2;
  int x0 = (128 - 3 - CW) / 2 + 1;
  if (left == 1) x0 -= (CW - CT) / 2;
  drawSegDigit(x0, 40, (int)left, CW, CH, CT);
  u8g2.sendBuffer();
  return true;
}

static void oledUpdate()
{
  if (oledProfileOverlay()) return;

  const size_t LINE_CHARS = 25;     // 5x8-Font: 125 px Breite + 3 px Shift-Reserve
  struct tm t;
  const bool valid = currentTime(t);

  int dx = 0, dy = 0;
  if (s_oledShift) {
    const uint32_t every = OLED_SHIFT_EVERY_S > 0 ? OLED_SHIFT_EVERY_S : 60;
    const uint32_t step = (millis() / 1000UL / every) % 4;
    dx = step;            // 0,1,2,3,0,…
    dy = (step + 1) % 2;  // 1,0,1,0,…
  }

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

  // 3) Netzwerkname: verbundenes Netz / Setup-WLAN / Suche läuft
  if (connected)          snprintf(buf, sizeof(buf), "WLAN %s", WiFi.SSID().c_str());
  else if (s_apActive)    snprintf(buf, sizeof(buf), "Setup %s", WEB_SETUP_AP_SSID);
  else if (!wifiConfigured()) snprintf(buf, sizeof(buf), "WLAN aus");
  else                    snprintf(buf, sizeof(buf), "WLAN suche ...");
  fitText(buf, LINE_CHARS);
  drawCentered(buf, Y_SSID + dy, dx);

  // 4) Signalstärke (RSSI in dBm; näher an 0 = besser, z.B. -50 sehr gut, -85 schwach)
  if (connected) {
    snprintf(buf, sizeof(buf), "Signal %d dBm", (int)WiFi.RSSI());
  } else if (s_apActive) {
    snprintf(buf, sizeof(buf), "http://%s", WiFi.softAPIP().toString().c_str());
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
//                buttonPoll()     → BOOT-Taster (Oszilloskop-Profil)
//                webLoop()        → Webinterface, DNS im Setup-WLAN
//   alle 200 ms  oledUpdate()     → Sekundenanzeige auf dem OLED flüssig
//   alle   1 s   wifiLogChanges() → Statusmeldungen ins Log
//                webApManage()    → Setup-WLAN ein/aus, Bildrate
static void uiTask(void *)
{
  // WICHTIG: OLED (I2C) und WLAN werden HIER initialisiert, nicht in setup():
  // ihre Interrupts landen damit auf Core 0 und stören den Zeichenpfad nicht.
  logMsg("[UI] laeuft auf Core %d", xPortGetCoreID());
  oledInit();
  wifiStart();
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);    // DevKitC hat zusätzlich einen externen Pull-up

  uint32_t lastOled = 0, lastLog = 0;
  for (;;) {
    publishTime();
    buttonPoll();
    webLoop();

    const uint32_t now = millis();
    if (now - lastOled >= OLED_REFRESH_MS) {
      lastOled = now;
      oledUpdate();                       // ~25 ms I2C-Transfer, blockiert nur Core 0
    }
    if (now - lastLog >= 1000) {
      // Bildrate aus dem Bildzähler des scopeTask
      static uint32_t lastFrames = 0;
      const uint32_t f = g_frameCount;
      s_fps = (f - lastFrames) * 1000UL / (now - lastLog);
      lastFrames = f;
      lastLog = now;
      wifiLogChanges();
      webApManage();
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
  Serial.println();
  logMsg("ESP32 Oscilloscope Clock v" FW_VERSION " (basierend auf v1.0 von Mauro Pintus 2018)");
  logMsg("CPU: %lu MHz", (unsigned long)getCpuFrequencyMhz());

  // Bis zum ersten publishTime() die Startzeit anzeigen
  g_timePacked = packTime(FALLBACK_H, FALLBACK_M, FALLBACK_S);

  // Gespeicherte Einstellungen laden (Profil, Zifferblatt, Strahl, Zeit, OLED) –
  // vor dem Start der Tasks
  loadSettings();
  logMsg("[Scope] Profil: %s – BOOT-Taster %u s halten zum Umschalten",
                profileName(g_scopeProfile), (unsigned)(BUTTON_LONG_MS / 1000));

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
