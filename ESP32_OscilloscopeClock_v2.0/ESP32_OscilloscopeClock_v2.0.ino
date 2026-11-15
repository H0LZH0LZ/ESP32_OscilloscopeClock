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
  gezeichnet werden (~200×/s) – sonst flimmert es oder es entstehen helle
  Punkte. Wie hell eine Linie leuchtet, hängt davon ab, wie lange der Strahl
  auf ihr verweilt; die Vektor-Engine sorgt deshalb für überall gleiche
  Strahlgeschwindigkeit.

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
#include <Preferences.h>      // dauerhafte Einstellungen im Flash (NVS): Oszilloskop-Profil

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
// Oszilloskop-Profil: 0 = analoges Röhren-Oszilloskop, 1 = digitales Oszilloskop.
// Startwert aus dem Flash (NVS), im Betrieb per BOOT-Taster umschaltbar.
// Schreiber: uiTask (Core 0), Leser: beamJump (Core 1). 1 Byte = atomar.
static volatile uint8_t g_scopeProfile = SCOPE_PROFILE;

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
static uint32_t s_settleCycles = 0; // Einschwingzeit bei Dunkeltastung
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

// Schaltet beide DACs ein und berechnet die Schrittdauern. dacWrite()
// übernimmt die komplette Pad-Konfiguration (Power-up, Kosinusgenerator aus)
// für ESP-IDF 4 und 5; danach schreiben dacOutX/Y nur noch ins Register.
static void dacInit() {
  dacWrite(DAC_X_PIN, 0x80);
  dacWrite(DAC_Y_PIN, 0x80);
  lastx = lasty = 0x80;
  s_stepCycles = (uint32_t)((uint64_t)BEAM_NS_PER_UNIT * getCpuFrequencyMhz() / 1000ULL);
  if (s_stepCycles < 1) s_stepCycles = 1;
  // Sprung-Bremsrampe vorberechnen (spart Divisionen im Zeichenpfad)
  static_assert(JUMP_SPEED == 0 || (JUMP_SPEED >= JUMP_LANDING && JUMP_LANDING >= 1 && JUMP_RAMP >= 1),
                "config.h: JUMP_SPEED >= JUMP_LANDING >= 1 und JUMP_RAMP >= 1 (oder JUMP_SPEED 0)");
  for (int r = 0; r <= JUMP_RAMP && JUMP_SPEED > 0; r++) {
    const uint32_t v16 = 16 * JUMP_LANDING + 16 * (JUMP_SPEED - JUMP_LANDING) * r / JUMP_RAMP;
    s_jumpCycles[r] = v16 ? (s_stepCycles * 16) / v16 : s_stepCycles;
    if (s_jumpCycles[r] < 1) s_jumpCycles[r] = 1;
  }
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
  if (g_scopeProfile == 1 || JUMP_SPEED == 0) {
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
// Ein Stroke ist ein zusammenhängender Linienzug in einer Tabelle aus
// DataTable.h: Punkte bei den Byte-Offsets a, a±2, …, b. Ist b < a, wird er
// rückwärts gezeichnet (Helligkeit ist richtungsunabhängig).
struct Stroke {
  const byte *t;
  int16_t a, b;
};

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
// Rundtour für Zifferblatt + Ziffern (einmal beim Start berechnet)
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

static void buildTour(const byte *digits, int digitsSize)
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
  Serial.printf("[Scope] Rundtour: %d Linienzuege, Spruenge gesamt %.0f DAC-Einheiten\n", n, total);
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
static void scopeTask(void *)
{
  // Dieser Task gibt Core 1 nie frei (kein vTaskDelay). Der Idle-Task von
  // Core 1 kommt daher nie dran – falls dessen Watchdog in der verwendeten
  // Core-Version aktiv ist, würde er auslösen. Deshalb hier abschalten.
  disableCore1WDT();
  dacInit();
  Serial.printf("[Scope] laeuft auf Core %d\n", xPortGetCoreID());

#if DIAL_STYLE == 1
  buildTour(DialDigits24, sizeof(DialDigits24));          // Ziffern 13-24
#elif DIAL_STYLE == 2
  buildTour(DialDigitsRoman, sizeof(DialDigitsRoman));    // römische Ziffern
#else
  buildTour(DialDigits12, sizeof(DialDigits12));          // Ziffern 1-12
#endif

  const uint32_t secCycles = s_stepCycles / 2;            // Sekundenzeiger: 2 × halbe Helligkeit

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

    // 1. Stunden- und Minutenzeiger
    PlotTable(HrPtrData,  sizeof(HrPtrData),  0xFF, 9 * hpos, HAND_BRIGHT_PCT);
    PlotTable(MinPtrData, sizeof(MinPtrData), 0xFF, 9 * m,    HAND_BRIGHT_PCT);

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
// ---- BOOT-Taster: Oszilloskop-Profil umschalten ------------------------------
// BOOT-Taster des DevKitC (GPIO0, gedrückt = LOW) mindestens BUTTON_LONG_MS
// halten → Profil wechselt (analog ↔ digital), wird im Flash gespeichert und
// gilt sofort, auch nach dem nächsten Neustart. Während des Haltens und
// danach zeigt das OLED kurz einen Hinweis.
// Achtung: GPIO0 ist ein Boot-Pin. Beim EINSCHALTEN bzw. Reset gedrückt
// gehalten, startet der ESP32 im Download-Modus statt der Uhr.
static uint32_t s_btnDownSince = 0;      // millis() beim Drücken, 0 = losgelassen
static bool     s_btnHandled   = false;  // Umschaltung für diesen Tastendruck erledigt
static uint32_t s_overlayUntil = 0;      // Profil-Hinweis auf dem OLED bis zu diesem Zeitpunkt

static const char *profileName(uint8_t p)  { return p == 1 ? "Digital" : "Analog"; }

static void saveProfile(uint8_t p)
{
  // Schreiben ins Flash hält Core 1 für wenige ms an (einmaliger kurzer
  // Lichtpunkt auf dem Oszilloskop) – passiert nur bei der Umschaltung.
  Preferences prefs;
  if (prefs.begin("scopeclock", false)) {
    prefs.putUChar("profile", p);
    prefs.end();
  }
}

static uint8_t loadProfile()
{
  Preferences prefs;
  uint8_t p = SCOPE_PROFILE;                  // Startwert aus config.h
  if (prefs.begin("scopeclock", true)) {      // nur lesen
    p = prefs.getUChar("profile", SCOPE_PROFILE);
    prefs.end();
  }
  return p > 1 ? SCOPE_PROFILE : p;
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
    saveProfile(p);
    s_overlayUntil = now + 2500;
    Serial.printf("[Scope] Profil umgeschaltet: %s, gespeichert\n", profileName(p));
  }
}

// Profil-Hinweis statt der normalen Anzeige. Liefert true, wenn gezeichnet.
static bool oledProfileOverlay()
{
  const uint32_t now = millis();
  const bool holding = s_btnDownSince && !s_btnHandled && now - s_btnDownSince >= 300;
  if (!holding && (int32_t)(s_overlayUntil - now) <= 0) return false;

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  if (holding) {
    // Countdown bis zur Umschaltung
    const uint32_t left = (BUTTON_LONG_MS - (now - s_btnDownSince) + 999) / 1000;
    char buf[24];
    drawCentered("Oszilloskop-Profil", 12, 1);         // 18 Zeichen = 108 px
    drawCentered("wechseln", 25, 1);
    snprintf(buf, sizeof(buf), "noch %lu s halten", (unsigned long)left);
    drawCentered(buf, 41, 1);
    drawCentered(g_scopeProfile == 1 ? "-> Analog" : "-> Digital", 57, 1);
  } else {
    drawCentered("Oszilloskop-Profil", 24, 1);
    u8g2.setFont(u8g2_font_7x14B_tf);
    drawCentered(profileName(g_scopeProfile), 46, 1);
  }
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
//                buttonPoll()     → BOOT-Taster (Oszilloskop-Profil)
//   alle 200 ms  oledUpdate()     → Sekundenanzeige auf dem OLED flüssig
//   alle   1 s   wifiLogChanges() → Statusmeldungen auf der Konsole
static void uiTask(void *)
{
  // WICHTIG: OLED (I2C) und WLAN werden HIER initialisiert, nicht in setup():
  // ihre Interrupts landen damit auf Core 0 und stören den Zeichenpfad nicht.
  Serial.printf("[UI] laeuft auf Core %d\n", xPortGetCoreID());
  oledInit();
  wifiStart();
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);    // DevKitC hat zusätzlich einen externen Pull-up

  uint32_t lastOled = 0, lastLog = 0;
  for (;;) {
    publishTime();
    buttonPoll();

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

  // Gespeichertes Oszilloskop-Profil laden (vor dem Start des scopeTask)
  g_scopeProfile = loadProfile();
  Serial.printf("[Scope] Profil: %s – BOOT-Taster %u s halten zum Umschalten\n",
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
