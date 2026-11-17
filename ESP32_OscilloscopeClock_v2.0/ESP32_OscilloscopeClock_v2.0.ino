/******************************************************************************

  ESP32 Oscilloscope Clock v2.0
  X/Y-Vektoruhr über die internen DACs, NTP-Zeit, Status-OLED

  Idee nach:   ESP32 Oscilloscope Clock von Mauro Pintus (Milano 2018)
               https://github.com/maurohh/ESP32_OscilloscopeClock
               Programm und Vektorgrafiken (DataTable.h, erzeugt mit
               tools/gen_vectors.py) sind eigenständig neu geschrieben.

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

  Von Core 0 nach Core 1 gehen nur einzelne Werte, alle atomar lesbar (≤ 32 Bit):
  g_timePacked (Stunde/Minute/Sekunde, einmal pro Bild gelesen), g_scopeProfile,
  g_dialStyle, g_testPattern sowie die Einstellungen g_beamNs, g_handPct,
  g_tickPct, g_digitPct, g_jumpSpeed, g_zblank, g_zblankHigh. Der Zeichenpfad
  muss nie warten; geänderte Werte übernimmt er spätestens zwischen zwei Bildern.

  ---------------------------------------------------------------------------
  Bedienung
  ---------------------------------------------------------------------------
  BOOT-Taster im Betrieb 3 s halten → Oszilloskop-Profil wechselt
  (DIGITAL ↔ ANALOG, siehe config.h: SCOPE_PROFILE). Das OLED zeigt dabei
  "DIGITAL > ANALOG" und einen Countdown 3, 2, 1; das Profil wird im Flash
  gespeichert. Weiter halten bis 10 s → Werkseinstellungen (alle Einstellungen
  und WLANs löschen, Neustart); ab 5 s zählt das OLED 5 … 1 herunter, Loslassen bricht ab.
  Nicht beim Einschalten drücken (Download-Modus).

  Webinterface: http://scope-clock.local (oder IP-Adresse der Uhr) – Live-
  Vorschau, Status, Debug-Protokoll, WLAN-Einrichtung, Seitenmenü; für Handys
  optimiert, ohne Passwort. Wischen über die Vorschau wechselt das Zifferblatt
  (1–12 / 12-3-6-9 / nur Striche / XII-III-VI-IX / römisch I–XII) – auch am
  Oszilloskop, wird gespeichert. Unter "Einstellungen" lassen sich alle Werte
  online ändern (sofort wirksam, gespeichert; config.h liefert nur die
  Startwerte). Ohne bekanntes WLAN öffnet die Uhr das offene Setup-WLAN
  "ScopeClock-Setup" (Einrichtung unter http://192.168.4.1).

  ---------------------------------------------------------------------------
  Dateien
  ---------------------------------------------------------------------------
    ESP32_OscilloscopeClock_v2.0.ino  dieser Sketch
    config.h                          Pins, Zeitzone, NTP, OLED, Strahl-
                                      geschwindigkeit, Oszilloskop-Profil
    DataTable.h                       Vektordaten: Zifferblatt, Ziffern, Zeiger, Testbild
    vectors.h                         dieselben Daten für die Vorschau der Webseite (gzip-JSON)
    webpage.h                         Webinterface (aus web/index.html, gzip)
    font.h, icon.h                    Schrift und App-Symbol des Webinterfaces
    ../HARDWARE.md                    Verdrahtung, Gehäuse, Oszilloskop-Einstellungen
    ../SOFTWARE.md                    Aufbau der Software, Tabellenformat, Tuning

  Benötigte Bibliothek (Arduino-Bibliotheksverwalter):  U8g2 by oliver
  Board-Paket: "esp32" by Espressif (2.0.x oder 3.x), Board "ESP32 Dev Module"

******************************************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>        // mehrere WLANs, verbindet mit dem stärksten
#include <SPI.h>              // OLED über Hardware-SPI
#include <time.h>             // time(), localtime_r(), struct tm
#include <esp_sntp.h>         // NTP-Client des ESP-IDF (Sync-Callback, Intervall)
#include <soc/soc.h>          // SET_PERI_REG_BITS()
#include <soc/rtc_io_reg.h>   // Registeradressen der DAC-Pads
#include <soc/gpio_reg.h>     // GPIO-Setzen/Löschen für die optionale Dunkeltastung
#include <U8g2lib.h>          // OLED-Treiber
#include <Preferences.h>      // dauerhafte Einstellungen im Flash (NVS): Profil, WLANs
#include <WebServer.h>        // Webinterface (HTTP, Port 80)
#include <Wire.h>             // I²C für die optionale Echtzeituhr (DS3231)
#include <DNSServer.h>        // Setup-WLAN: alle Namen auf die Uhr umleiten (Captive Portal)
#include <ESPmDNS.h>          // Erreichbar als http://<WIFI_HOSTNAME>.local
#include <stdarg.h>
#include <esp_freertos_hooks.h>   // Leerlauf-Hook für die CPU-Last-Messung (Core 0)
#include <esp_timer.h>
#include <esp_system.h>           // esp_reset_reason()

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

#include "DataTable.h"
#include "webpage.h"          // Webinterface, gzip-komprimiert (aus web/index.html, tools/gen_webpage.py)
#include "font.h"             // Schrift des Webinterfaces (2 Schnitte WOFF2, PROGMEM, ca. 30 KB)
#include "icon.h"             // App-Symbol (SVG/PNG) und Manifest für den Startbildschirm
#include "vectors.h"          // Vorschau-Daten für das Webinterface (gzip, aus tools/gen_vectors.py --vectors)

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

// Stunde, Minute, Sekunde in ein 32-Bit-Wort packen (h<<16 | m<<8 | s)
static inline uint32_t packTime(int h, int m, int s) {
  return ((uint32_t)h << 16) | ((uint32_t)m << 8) | (uint32_t)s;
}

// Oszilloskop-Profil: 0 = analoges Röhren-Oszilloskop, 1 = digitales Oszilloskop.
// Startwert aus dem Flash (NVS), im Betrieb per BOOT-Taster umschaltbar.
// Schreiber: uiTask (Core 0), Leser: beamJump (Core 1). 1 Byte = atomar.
static volatile uint8_t g_scopeProfile = SCOPE_PROFILE;

// Zifferblatt: 0 = Ziffern 1–12, 1 = nur 12/3/6/9, 2 = römisch I–XII,
// 3 = nur XII/III/VI/IX, 4 = nur Striche (siehe dialDigits). Startwert aus dem
// Flash (NVS), im Betrieb über das Webinterface umschaltbar (Wischen über die
// Vorschau). Schreiber: uiTask, Leser: scopeTask (baut dann die Rundtour neu).
static volatile uint8_t g_dialStyle = DIAL_STYLE;

// Weitere Laufzeit-Einstellungen für die Strahlausgabe (Webinterface →
// "Einstellungen", gespeichert im Flash; config.h liefert nur die Startwerte).
// Schreiber: uiTask, Leser: scopeTask – je ≤ 16 Bit, atomar.
static volatile uint16_t g_beamNs    = BEAM_NS_PER_UNIT;   // Strahlgeschwindigkeit (ns pro Schritt)
static volatile uint8_t  g_handPct   = HAND_BRIGHT_PCT;    // Helligkeit Stunden-/Minutenzeiger (%)
static volatile uint8_t  g_tickPct   = TICK_BRIGHT_PCT;    // Helligkeit Minutenstriche (%)
static volatile uint8_t  g_digitPct  = DIGIT_BRIGHT_PCT;   // Helligkeit Ziffern (%)
static volatile uint8_t  g_jumpSpeed = JUMP_SPEED;         // Rücklauf-Tempo (Profil ANALOG), 0 = hart
static volatile bool     g_zblank    = ZBLANK_ENABLE;      // Dunkeltastung über den Z-Eingang an (GPIO ZBLANK_PIN)
static volatile bool     g_zblankHigh = ZBLANK_ACTIVE_HIGH; // true = High-Pegel macht dunkel, false = Low-Pegel

// Einstellungen, die nur Core 0 nutzt (Zeit, OLED)
static char    s_tz[48]        = TZ_INFO;                  // POSIX-Zeitzone
static char    s_ntpServer[64] = NTP_SERVER_1;             // erster NTP-Server (muss dauerhaft gültig bleiben)
static uint8_t s_oledPct       = OLED_BRIGHT_PCT;          // OLED-Helligkeit 0–100 % (0 = aus)
static bool    s_oledSec       = OLED_SECONDS;             // Uhrzeit mit Sekunden
static char    s_host[33]      = WIFI_HOSTNAME;            // Gerätename (Router, http://<name>.local)
static volatile bool g_testPattern = BOOT_TEST_S > 0;    // Testbild statt Uhr (nicht gespeichert); beim Start BOOT_TEST_S Sekunden an
static bool s_bootTest = BOOT_TEST_S > 0;                 // Start-Testbild läuft noch (endet automatisch)
static bool    s_oledShift     = OLED_SHIFT_EVERY_S > 0;
static bool    s_oledFlip      = OLED_FLIP;                // Anzeige um 180° gedreht
static bool    s_h12           = CLOCK_12H;                // Uhrzeit im 12-Stunden-Format (AM/PM)
static uint8_t s_lang          = UI_LANG;                  // Sprache: 0 = Deutsch, 1 = English

// Text in der eingestellten Sprache (Webinterface-Meldungen, OLED, Protokoll)
static inline const char *tr(const char *de, const char *en) { return s_lang ? en : de; }

// NTP-Status. Schreiber: SNTP-Callback (läuft im lwIP-Task auf Core 0),
// Leser: uiTask. time_t ist auf aktuellen ESP32-Cores 64 Bit breit und damit
// NICHT atomar – daher der Spinlock für den Zeitstempel.
static volatile bool g_ntpSynced     = false;  // true nach dem ersten erfolgreichen Sync
static volatile bool g_timeManual    = false;  // true, wenn die Zeit im Webinterface gestellt wurde
static volatile bool g_timeRtc       = false;  // true, wenn die Zeit beim Start aus der Echtzeituhr (RTC) kam
static int8_t        s_dstForced     = -1;     // von Hand gewählt: -1 = automatisch, 0 = Winterzeit, 1 = Sommerzeit
static time_t        g_lastSyncEpoch = 0;      // Unix-Zeit der letzten Synchronisierung

// Gültige Uhrzeit mit Datum: per NTP, von Hand gestellt (ohne Internet) oder aus der RTC.
static inline bool timeValid() { return g_ntpSynced || g_timeManual || g_timeRtc; }
static portMUX_TYPE  g_syncMux       = portMUX_INITIALIZER_UNLOCKED;

// Zeitpunkt der letzten NTP-Synchronisierung lesen (unter Spinlock, da 64 Bit)
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
  if (timeValid()) {
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

// Leert den Ringpuffer (POST /api/log/clear). Die serielle Ausgabe bleibt davon unberührt.
static void logClear()
{
  portENTER_CRITICAL(&s_logMux);
  s_logHead = 0; s_logWrapped = false;
  memset(s_logBuf, 0, sizeof(s_logBuf));
  portEXIT_CRITICAL(&s_logMux);
}

// Momentaufnahme des Logs in zeitlicher Reihenfolge (älteste Zeile zuerst), als zwei
// Teilstücke im statischen Puffer – ohne String und ohne zusätzlichen Heap.
// Nach einem Überlauf wird die angeschnittene erste Zeile weggelassen.
static void logSnapshot(const char *&a, size_t &alen, const char *&b, size_t &blen)
{
  static char copy[LOG_BUFFER_SIZE];
  size_t head; bool wrapped;
  portENTER_CRITICAL(&s_logMux);
  memcpy(copy, s_logBuf, sizeof(copy));
  head = s_logHead; wrapped = s_logWrapped;
  portEXIT_CRITICAL(&s_logMux);
  a = copy; alen = 0;
  if (wrapped) {
    size_t start = head;
    while (start < sizeof(copy) && copy[start] != '\n') start++;   // angeschnittene Zeile überspringen
    if (start + 1 < sizeof(copy)) { a = copy + start + 1; alen = sizeof(copy) - start - 1; }
  }
  b = copy; blen = head;
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
//      c) Der Sekundenzeiger wird hin (Mitte → Spitze) und zurück je
//         mit halber Helligkeit gezeichnet. Der lange Sprung vom Rand zurück
//         zur Mitte entfällt damit ganz – der Strahl "läuft" auf dem Zeiger.
//      d) Jeder Linienzug wird von dem Ende aus gezeichnet, das näher am
//         Strahl liegt.
//     Optional (Webinterface, Kabel an ZBLANK_PIN): Dunkeltastung über den
//     Z-Eingang des Oszilloskops – der Strahl ist während des Sprungs aus, die
//     Spur verschwindet ganz.
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
  s_settleCycles = (uint32_t)((uint64_t)ZBLANK_SETTLE_NS * getCpuFrequencyMhz() / 1000ULL);
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

// ---- Dunkeltastung über den Z-Eingang (optional, im Webinterface einschaltbar) ----
// Ein drittes Kabel von GPIO ZBLANK_PIN zum Z-Eingang des Oszilloskops schaltet den
// Strahl während der Sprünge aus – die Rücklaufspuren verschwinden ganz.
static_assert(ZBLANK_PIN >= 0 && ZBLANK_PIN < 32 && ZBLANK_PIN != DAC_X_PIN && ZBLANK_PIN != DAC_Y_PIN,
              "config.h: ZBLANK_PIN muss ein freier GPIO 0-31 sein (nicht 25/26)");
// Schaltet den Strahl aus (dark=true) oder wieder an. Direkter Registerzugriff:
// wenige Takte, IRAM-sicher.
static inline void IRAM_ATTR zBlank(bool dark)
{
  if (dark == g_zblankHigh) REG_WRITE(GPIO_OUT_W1TS_REG, 1UL << ZBLANK_PIN);
  else                      REG_WRITE(GPIO_OUT_W1TC_REG, 1UL << ZBLANK_PIN);
}

// Dunkeltastung ein-/ausschalten (Core 0, beim Start und aus dem Webinterface).
// Aus: Pin hochohmig (Eingang) – ohne Kabel am Z-Eingang passiert also nichts.
// An: erst den Pegel "Strahl an" setzen, dann als Ausgang schalten.
static void zblankApply(bool on, bool activeHigh)
{
  g_zblank = false;                                      // Zeichenpfad tastet ab jetzt nicht mehr
  g_zblankHigh = activeHigh;
  if (!on) { pinMode(ZBLANK_PIN, INPUT); return; }
  digitalWrite(ZBLANK_PIN, activeHigh ? LOW : HIGH);     // Strahl an
  pinMode(ZBLANK_PIN, OUTPUT);
  g_zblank = true;
}

// Sprung (Rücklauf) zum Punkt (x2,y2) – siehe Regel 5.
// Die Sprungart hängt vom Oszilloskop-Profil ab (BOOT-Taster, g_scopeProfile):
//   digital → harter Sprung: ein digitales Oszilloskop würde jeden Zwischenpunkt
//             eines gefahrenen Sprungs als hellen Abtastpunkt zeigen.
//   analog  → gebremster Sprung (JUMP_SPEED/JUMP_LANDING/JUMP_RAMP).
// Mit Dunkeltastung (g_zblank) wird immer im Dunkeln hart gesprungen.
static void IRAM_ATTR beamJump(int x2, int y2)
{
  if (x2 == lastx && y2 == lasty) return;
  if (g_zblank) {
    // Dunkel tasten, hart springen, im Dunkeln einschwingen lassen, Strahl an.
    zBlank(true);
    beamPoint(x2, y2, s_settleCycles);
    zBlank(false);
    return;
  }
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
// Tabellenformat (Details in DataTable.h):
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
// Reihenfolge (Regel 5d; die Helligkeit ist richtungsunabhängig). Linienzüge
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
//   4 = keine Ziffern, nur Minutenstriche
static const int DIAL_STYLES = 5;                         // Anzahl der Zifferblätter

static const uint8_t *dialDigits(int style, int &size)
{
  switch (style) {
    case 1:  size = sizeof(DialDigitsMin);    return DialDigitsMin;
    case 2:  size = sizeof(DialDigitsRoman);  return DialDigitsRoman;
    case 3:  size = sizeof(DialDigitsRoman4); return DialDigitsRoman4;
    case 4:  size = sizeof(DialDigitsNone);   return DialDigitsNone;
    default: size = sizeof(DialDigits12);    return DialDigits12;
  }
}

static const char *dialName(int style)
{
  switch (style) {
    case 1:  return "12/3/6/9";
    case 2:  return "I-XII";
    case 3:  return "XII/III/VI/IX";
    case 4:  return tr("nur Striche", "ticks only");
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
static const int TOUR_MAX = 160;                 // 7-Segment 1–12: 60 Striche + 62 Segmente
static Stroke s_tour[TOUR_MAX];
static int    s_tourLen = 0;

static float tourGap(const Stroke &from, const Stroke &to)       // Sprung from.Ende → to.Anfang
{
  const float dx = strokeX(to, 0) - strokeX(from, 1), dy = strokeY(to, 0) - strokeY(from, 1);
  return sqrtf(dx * dx + dy * dy);
}

// Alle Linienzüge einer Tabelle an die Rundtour anhängen (bis TOUR_MAX)
static void tourAdd(const byte *table, int size)
{
  int i = 0;
  Stroke st;
  while (s_tourLen < TOUR_MAX && nextStroke(table, size, 0x00, i, st)) s_tour[s_tourLen++] = st;
}

// Linienzug umdrehen (Anfang und Ende tauschen)
static void reverseStroke(Stroke &st) { const int16_t tmp = st.a; st.a = st.b; st.b = tmp; }

// Rundtour aus Zifferblatt + Ziffern bauen und sortieren (siehe Erklärung oben)
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
  logMsg(tr("[Scope] Rundtour: %d Linienzuege, Spruenge gesamt %d DAC-Einheiten", "[Scope] round trip: %d polylines, jumps total %d DAC units"), n, (int)(total + 0.5f));
}

//*****************************************************************************
// scopeTask – Core 1: zeichnet endlos Zifferblatt + Zeiger
//*****************************************************************************
// Ablauf eines Bildes (Strahl startet in der Mitte):
//   1. Stundenzeiger, Minutenzeiger      (schmale Nadeln ab der Mitte)
//   2. Sekundenzeiger Mitte → Spitze     (halbe Helligkeit)
//   3. Rundtour Zifferblatt + Ziffern    (Einstieg nahe der Sekundenzeiger-Spitze,
//                                          endet wieder dort)
//   4. Sekundenzeiger Spitze → Mitte     (halbe Helligkeit, ersetzt den Rücksprung)
// Danach beginnt sofort das nächste Bild.
//
// Zeigertabellen (DataTable.h) enthalten je 60 vorberechnete Stellungen:
//   HrPtrData, MinPtrData: 60 × 9 Byte  (4 Punkte = schmale Nadel + 0xFF)
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
  logMsg(tr("[Scope] laeuft auf Core %d", "[Scope] running on core %d"), xPortGetCoreID());

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

    // Testbild (Einstellungen → Testbild): Rahmen, Kreis, Mittelkreuz statt der Uhr –
    // zum Einstellen von Position, Größe, X/Y-Verhältnis und Fokus am Oszilloskop
    if (g_testPattern) {
      PlotTable(TestData, sizeof(TestData), 0x00, 0);
      g_frameCount = g_frameCount + 1;
      continue;
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
    // Helligkeit 0–100 % der normalen Verweilzeit; 0 % = nicht zeichnen
    const int handPct = g_handPct;
    if (handPct > 0) {
      PlotTable(HrPtrData,  sizeof(HrPtrData),  0xFF, 9 * hpos, handPct);
      PlotTable(MinPtrData, sizeof(MinPtrData), 0xFF, 9 * m,    handPct);
    }

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
    // Minutenstriche (Tabelle DialData) und Ziffern mit eigener Helligkeit:
    // 0–100 % der normalen Verweilzeit pro Schritt, 0 % = ausgeblendet
    const uint32_t tickCycles  = (uint32_t)((uint64_t)s_stepCycles * g_tickPct  / 100);
    const uint32_t digitCycles = (uint32_t)((uint64_t)s_stepCycles * g_digitPct / 100);
    for (int k = 0; k < s_tourLen; k++) {
      const Stroke &st = s_tour[(start + k) % s_tourLen];
      const uint32_t c = st.t == DialData ? tickCycles : digitCycles;
      if (c) drawStroke(st, c);
    }

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
// Rückgabe true  = echte Zeit nach NTP-Sync oder von Hand gestellt (inkl. Datum),
//         false = weder noch: Zeit läuft ab FALLBACK_H:M:S seit Einschalten,
//                 Datumsfelder sind 0 und dürfen nicht angezeigt werden.
// Danach läuft die Systemuhr des ESP32 selbstständig weiter, auch wenn das
// WLAN später wegfällt. Eine von Hand gestellte Zeit geht beim Neustart
// verloren (der ESP32 hat keine gepufferte Echtzeituhr).
static bool currentTime(struct tm &out)
{
  if (timeValid()) {
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
// Echtzeituhr DS3231 (optional, Core 0)
//*****************************************************************************
// Ein DS3231-Modul (z.B. ZS-042) an I²C (SDA = RTC_SDA_PIN, SCL = RTC_SCL_PIN, 3V3, GND)
// hält Datum und Uhrzeit mit seiner Knopfzelle auch ohne Strom und ohne Internet
// (±2 ppm ≈ 1 Minute pro Jahr). Im Webinterface ein-/ausschaltbar (Standard: aus).
//   Start:      gültige RTC-Zeit → Systemuhr stellen (g_timeRtc), bis NTP kommt
//   NTP-Sync:   RTC nachstellen (jede Stunde, siehe wifiLogChanges)
//   Von Hand:   gestellte Zeit auch in die RTC schreiben
//   ohne NTP:   alle 10 min die Systemuhr aus der RTC nachstellen (genauer als der ESP32)
// Die RTC läuft in UTC – Zeitzone und Sommerzeit rechnet weiterhin der ESP32.
// Alle Zugriffe laufen im uiTask (auch die Webserver-Handler), also ohne Sperren.
static const uint8_t RTC_ADDR = 0x68;
static bool    s_rtcEnabled = RTC_ENABLE;   // Einstellung (NVS "rtc")
static bool    s_rtcFound   = false;        // DS3231 antwortet
static bool    s_rtcValid   = false;        // RTC-Zeit gültig (Oszillator lief durch, Jahr plausibel)
static bool    s_wireOn     = false;

static uint8_t bcd2bin(uint8_t v) { return (v >> 4) * 10 + (v & 0x0F); }
static uint8_t bin2bcd(int v)     { return (uint8_t)(((v / 10) << 4) | (v % 10)); }

// Tage seit 1970-01-01 für ein Datum (gregorianisch) – ohne timegm(), das newlib nicht hat
static int64_t daysFromCivil(int y, int m, int d)
{
  y -= m <= 2;
  const int64_t era = (y >= 0 ? y : y - 399) / 400;
  const int yoe = y - era * 400;
  const int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}

static bool rtcRead(uint8_t reg, uint8_t *buf, size_t n)
{
  Wire.beginTransmission(RTC_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)RTC_ADDR, (int)n) != (int)n) return false;
  for (size_t i = 0; i < n; i++) buf[i] = Wire.read();
  return true;
}

static bool rtcWrite(uint8_t reg, const uint8_t *buf, size_t n)
{
  Wire.beginTransmission(RTC_ADDR);
  Wire.write(reg);
  Wire.write(buf, n);
  return Wire.endTransmission() == 0;
}

// Liest die RTC-Zeit (UTC) als Unix-Zeit. false: keine Antwort, Oszillator war
// gestoppt (Batterie leer/fehlt, OSF-Bit) oder unplausibles Datum.
static bool rtcGet(time_t &out)
{
  uint8_t r[7], st;
  if (!rtcRead(0x00, r, 7) || !rtcRead(0x0F, &st, 1)) { s_rtcFound = false; return false; }
  s_rtcFound = true;
  if (st & 0x80) { s_rtcValid = false; return false; }               // OSF: Zeit nicht verlässlich
  const int sec = bcd2bin(r[0] & 0x7F), min = bcd2bin(r[1] & 0x7F);
  const int hour = (r[2] & 0x40) ? (bcd2bin(r[2] & 0x1F) % 12 + ((r[2] & 0x20) ? 12 : 0))   // 12-h-Modus
                                 : bcd2bin(r[2] & 0x3F);
  const int day = bcd2bin(r[4] & 0x3F), mon = bcd2bin(r[5] & 0x1F), year = 2000 + bcd2bin(r[6]);
  if (year < 2024 || mon < 1 || mon > 12 || day < 1 || day > 31 || hour > 23 || min > 59 || sec > 59) { s_rtcValid = false; return false; }
  out = (time_t)(daysFromCivil(year, mon, day) * 86400LL + hour * 3600 + min * 60 + sec);
  s_rtcValid = true;
  return true;
}

// Schreibt die aktuelle Systemzeit (UTC) in die RTC und löscht das OSF-Bit.
static bool rtcSetFromSystem()
{
  if (!s_wireOn) return false;
  const time_t now = time(nullptr);
  struct tm u;
  gmtime_r(&now, &u);
  const uint8_t r[7] = { bin2bcd(u.tm_sec), bin2bcd(u.tm_min), bin2bcd(u.tm_hour),   // 24-h-Modus
                         (uint8_t)(u.tm_wday + 1), bin2bcd(u.tm_mday), bin2bcd(u.tm_mon + 1),
                         bin2bcd((u.tm_year + 1900) % 100) };
  uint8_t ctl = 0, st = 0;
  bool ok = rtcWrite(0x00, r, 7);
  if (ok && rtcRead(0x0E, &ctl, 1) && (ctl & 0x80)) { ctl &= 0x7F; rtcWrite(0x0E, &ctl, 1); }   // EOSC aus → läuft auch auf Batterie
  if (ok && rtcRead(0x0F, &st, 1)) { st &= 0x7F; ok = rtcWrite(0x0F, &st, 1); }                  // OSF löschen
  s_rtcFound = ok;
  if (ok) s_rtcValid = true;
  return ok;
}

// Systemuhr aus der RTC stellen (nur solange kein NTP da ist). Liefert true bei Erfolg.
static bool rtcToSystem()
{
  time_t e;
  if (!s_wireOn || g_ntpSynced || !rtcGet(e)) return false;
  struct timeval tv = { e, 0 };
  settimeofday(&tv, nullptr);
  g_timeRtc = true;
  return true;
}

// RTC ein- oder ausschalten (Start und Webinterface). Beim Einschalten: Ist die
// Systemzeit schon gültig (NTP oder von Hand), wird sie in die RTC geschrieben,
// sonst wird die Systemzeit aus der RTC gestellt.
static void rtcApply(bool on)
{
  s_rtcEnabled = on;
  if (!on) {
    if (s_wireOn) { Wire.end(); s_wireOn = false; }
    s_rtcFound = s_rtcValid = false;
    return;
  }
  if (!s_wireOn) { s_wireOn = Wire.begin(RTC_SDA_PIN, RTC_SCL_PIN, 100000); Wire.setTimeOut(20); }
  time_t e;
  if (g_ntpSynced || g_timeManual) {
    if (rtcSetFromSystem()) logMsg("%s", tr("[RTC] DS3231 gefunden, Uhrzeit uebertragen", "[RTC] DS3231 found, time transferred"));
  } else if (rtcToSystem()) {
    e = time(nullptr); struct tm lt; localtime_r(&e, &lt);
    logMsg(tr("[RTC] Uhrzeit aus der RTC: %02d.%02d.%04d %02d:%02d:%02d", "[RTC] time from RTC: %02d.%02d.%04d %02d:%02d:%02d"),
           lt.tm_mday, lt.tm_mon + 1, lt.tm_year + 1900, lt.tm_hour, lt.tm_min, lt.tm_sec);
  }
  if (!s_rtcFound)
    logMsg(tr("[RTC] kein DS3231 gefunden (SDA GPIO%d, SCL GPIO%d)", "[RTC] no DS3231 found (SDA GPIO%d, SCL GPIO%d)"), RTC_SDA_PIN, RTC_SCL_PIN);
  else if (!s_rtcValid)
    logMsg("%s", tr("[RTC] Zeit ungueltig (Batterie leer oder neu) – wird beim naechsten NTP-Abgleich oder von Hand gestellt",
                    "[RTC] time invalid (battery empty or new) – set at the next NTP sync or by hand"));
}

//*****************************************************************************
// WLAN (Core 0)
//*****************************************************************************

// ---- Im Webinterface gespeicherte WLANs ---------------------------------------
// WLANs werden ausschließlich im Webinterface eingetragen (bis zu
// WEB_MAX_NETWORKS Netze). Sie liegen im Flash (NVS, Namensraum "scopeclock",
// Schlüssel netn, net_s0…, net_p0…) und bleiben auch nach einem Update erhalten.
static StoredNet    s_webNets[WEB_MAX_NETWORKS];
static volatile int s_webNetCount = 0;   // gültige Einträge in s_webNets (erst Eintrag füllen, dann erhöhen)
static bool         s_apActive    = false; // Setup-WLAN offen (siehe webApManage)

// Abstimmung zwischen wifiTask und Webserver (beide auf Core 0):
//   s_multiCount   so viele Einträge aus s_webNets kennt WiFiMulti schon. Neue Netze
//                  trägt NUR der wifiTask ein – WiFiMulti ist nicht threadsicher, und
//                  addAP() aus dem Webserver während wifiMulti.run() könnte abstürzen.
//   s_multiBusy    wifiTask sucht/verbindet gerade (wifiMulti.run läuft)
//   s_webScanUntil Webinterface sucht Netze: wifiTask pausiert bis dahin, sonst würde
//                  seine eigene Suche die Ergebnisse der Seite löschen
static int               s_multiCount   = 0;
static volatile bool     s_multiBusy    = false;
static volatile uint32_t s_webScanUntil = 0;

// Im Webinterface gespeicherte WLANs aus dem NVS laden
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
    if (s_webNets[s_webNetCount].ssid[0]) s_webNetCount = s_webNetCount + 1;
  }
  p.end();
}

// Im Webinterface gespeicherte WLANs ins NVS schreiben
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

// true, sobald mindestens ein WLAN im Webinterface eingetragen ist
static bool wifiConfigured() { return s_webNetCount > 0; }

// Hält die WLAN-Verbindung. Läuft als eigener Task, weil WiFiMulti::run()
// zuerst einen Scan macht und dabei mehrere Sekunden blockiert – im uiTask
// würde das OLED so lange stehen bleiben.
// Ablauf: neue Netze bei WiFiMulti eintragen → nicht verbunden: alle Netze
// scannen → mit dem stärksten bekannten verbinden (max. 10 s) → alle 2 s
// Verbindung prüfen. Ohne bekanntes Netz wartet der Task, bis im Webinterface
// eins eingetragen wird.
// Ist jemand mit dem Setup-WLAN verbunden, wird nur alle 60 s ein Versuch
// gemacht: Jeder Versuch wechselt kurz den Funkkanal und kann das Handy aus dem
// Setup-WLAN werfen.
static void wifiTask(void *)
{
  uint32_t lastTry = 0;
  for (;;) {
    while (s_multiCount < s_webNetCount) {               // neu eingetragene Netze übernehmen
      wifiMulti.addAP(s_webNets[s_multiCount].ssid, s_webNets[s_multiCount].pw);
      s_multiCount++;
    }
    const uint32_t now = millis();
    const bool webScan  = (int32_t)(s_webScanUntil - now) > 0;
    const bool apGuests = s_apActive && WiFi.softAPgetStationNum() > 0;
    if (wifiConfigured() && WiFi.status() != WL_CONNECTED && !webScan &&
        (!apGuests || lastTry == 0 || now - lastTry >= 60000UL)) {
      s_multiBusy = true;
      wifiMulti.run(10000);
      s_multiBusy = false;
      lastTry = millis();
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

static void webStart();                     // siehe Abschnitt "Webinterface"
static void oledSetFlip(bool flip);         // siehe Abschnitt "OLED"

// Initialisiert WLAN, NTP und das Webinterface. Muss vom uiTask (Core 0) aus
// aufgerufen werden: Der WLAN-Treiber legt seine Interrupts auf dem
// aufrufenden Kern an, und sie sollen den Zeichenpfad auf Core 1 nicht stören.
static void wifiStart()
{
  WiFi.mode(WIFI_STA);                 // Client; das Setup-WLAN kommt bei Bedarf dazu
  WiFi.setHostname(s_host);            // Name in der Geräteliste des Routers (Einstellungen → System)

  // Gespeicherte Netze laden (bei WiFiMulti trägt sie der wifiTask ein)
  webNetsLoad();
  for (int i = 0; i < s_webNetCount; i++) {
    logMsg(tr("[WiFi] bekanntes Netz: \"%s\"", "[WiFi] known network: \"%s\""), s_webNets[i].ssid);   // Passwort wird nie ausgegeben
  }
  if (!wifiConfigured())
    logMsg(tr("[WiFi] noch kein WLAN eingetragen – Einrichtung ueber das Setup-WLAN \"%s\"", "[WiFi] no network configured yet – set up via the setup network \"%s\""), WEB_SETUP_AP_SSID);

  // NTP: configTzTime() setzt die Zeitzone (inkl. Sommerzeitregel) und startet
  // den SNTP-Client im Hintergrund. Er wartet selbstständig, bis das WLAN steht,
  // und synchronisiert danach alle NTP_SYNC_INTERVAL_MS erneut.
  sntp_set_time_sync_notification_cb(onNtpSync);
  sntp_set_sync_interval(NTP_SYNC_INTERVAL_MS);
  configTzTime(s_tz, s_ntpServer, NTP_SERVER_2, NTP_SERVER_3);

  xTaskCreatePinnedToCore(wifiTask, "wifi", 4096, nullptr, 1, &wifiTaskHandle, UI_CORE);
  webStart();
}

// true, wenn der letzte NTP-Abgleich länger als 2 Intervalle zurückliegt
static bool ntpOverdue()
{
  return g_ntpSynced && time(nullptr) - lastSyncEpoch() > (time_t)(2 * NTP_SYNC_INTERVAL_MS / 1000UL);
}

// Schreibt Statusänderungen ins Log (aufgerufen 1× pro Sekunde):
//  - WLAN verbunden (mit SSID, IP, Signal) / Verbindung verloren
//  - beim ersten Verbinden: mDNS starten (http://<WIFI_HOSTNAME>.local)
//  - jede NTP-Synchronisierung mit Datum und Uhrzeit
//  - einmalige Warnung, wenn der letzte Sync länger als 2 Intervalle her ist
static void wifiLogChanges()
{
  static wl_status_t last = WL_IDLE_STATUS;
  static time_t lastLoggedSync = 0;
  static bool   overdueLogged  = false;
  static bool   mdnsStarted    = false;

  wl_status_t st = WiFi.status();
  if (st != last) {
    if (st == WL_CONNECTED) {
      logMsg(tr("[WiFi] verbunden mit \"%s\", IP %s, RSSI %d dBm", "[WiFi] connected to \"%s\", IP %s, RSSI %d dBm"),
             WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI());
      if (!mdnsStarted && MDNS.begin(s_host)) {
        MDNS.addService("http", "tcp", 80);
        mdnsStarted = true;
        logMsg(tr("[Web] erreichbar unter http://%s.local und http://%s", "[Web] reachable at http://%s.local and http://%s"),
               s_host, WiFi.localIP().toString().c_str());
      }
    } else if (last == WL_CONNECTED) {
      logMsg(tr("[WiFi] Verbindung verloren – versuche erneut", "[WiFi] connection lost – retrying"));
    }
    last = st;
  }

  // Neuer Sync erkannt = Zeitstempel hat sich seit der letzten Ausgabe geändert
  const time_t syncEpoch = lastSyncEpoch();
  if (g_ntpSynced && syncEpoch != lastLoggedSync) {
    if (s_dstForced >= 0) applyDstOverride(-1);         // Zeitserver da: wieder automatische Sommerzeit
    struct tm st;
    localtime_r(&syncEpoch, &st);
    logMsg("[NTP] Sync %02d.%02d.%04d %02d:%02d:%02d",
           st.tm_mday, st.tm_mon + 1, st.tm_year + 1900, st.tm_hour, st.tm_min, st.tm_sec);
    if (s_rtcEnabled && !rtcSetFromSystem()) logMsg("%s", tr("[RTC] Nachstellen fehlgeschlagen", "[RTC] update failed"));
    lastLoggedSync = syncEpoch;
    overdueLogged  = false;
  }

  // Überfällig? (z.B. WLAN dauerhaft weg oder NTP-Server nicht erreichbar)
  if (!overdueLogged && ntpOverdue()) {
    logMsg(tr("[NTP] Warnung: letzter Sync liegt mehr als 2 Intervalle zurueck", "[NTP] warning: last sync is more than 2 intervals ago"));
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
static uint32_t  s_lastConnMs   = 0;    // zuletzt mit WLAN verbunden (0 = noch nie)
static uint32_t  s_connSinceMs  = 0;    // verbunden seit (0 = nicht verbunden)
static uint32_t  s_restartAtMs  = 0;    // geplanter Neustart (0 = keiner)
static uint32_t  s_reconnectAtMs = 0;   // geplantes Neu-Verbinden (0 = keins), nach der Antwort
static uint32_t  s_fps          = 0;    // Bildrate des Oszilloskop-Bildes
static uint8_t   s_cpu0Load     = 0;    // CPU-Last Core 0 in % (siehe idleHookCore0)

// ---- CPU-Last von Core 0 -----------------------------------------------------
// Der Leerlauf-Task von Core 0 ruft idleHookCore0() in einer Schleife auf, solange
// nichts anderes zu tun ist. Folgen zwei Aufrufe dicht aufeinander (< 200 µs),
// war der Kern dazwischen frei – diese Zeit wird als Leerlauf aufsummiert.
// Einmal pro Sekunde ergibt sich daraus die Last: 100 % − Leerlaufanteil.
// (Core 1 ist absichtlich immer zu 100 % mit der Strahlausgabe belegt.)
// Rückgabe false: kein Stromsparschlaf im Leerlauf, damit die Messung stimmt.
static volatile uint32_t s_idleUs   = 0;      // Leerlaufzeit seit der letzten Auswertung (µs)
static int64_t           s_idleLast = 0;
static bool idleHookCore0()
{
  const int64_t now = esp_timer_get_time();
  const int64_t d = now - s_idleLast;
  if (d > 0 && d < 200) s_idleUs = s_idleUs + (uint32_t)d;
  s_idleLast = now;
  return false;
}

// Grund des letzten Neustarts im Klartext (für Status und Log).
static const char *resetReasonText()
{
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:   return tr("Einschalten", "Power on");
    case ESP_RST_EXT:       return tr("Reset-Taster", "Reset button");
    case ESP_RST_SW:        return tr("Neustart durch Software", "Software restart");
    case ESP_RST_PANIC:     return tr("Absturz (Programmfehler)", "Crash (exception)");
    case ESP_RST_INT_WDT:   return tr("Absturz (Interrupt-Watchdog)", "Crash (interrupt watchdog)");
    case ESP_RST_TASK_WDT:  return tr("Absturz (Task-Watchdog)", "Crash (task watchdog)");
    case ESP_RST_WDT:       return tr("Absturz (Watchdog)", "Crash (watchdog)");
    case ESP_RST_DEEPSLEEP: return tr("Aufwachen aus Tiefschlaf", "Wake from deep sleep");
    case ESP_RST_BROWNOUT:  return tr("Unterspannung", "Brown-out");
    case ESP_RST_SDIO:      return "SDIO";
    default:                return tr("Unbekannt", "Unknown");
  }
}

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

// Antwort {"ok":…,"msg":"…"} an das Webinterface senden
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
  o += ",\"manual\":";          o += g_timeManual ? "true" : "false";
  o += ",\"rtc\":";             o += (g_timeRtc && !g_ntpSynced && !g_timeManual) ? "true" : "false";   // Zeit kommt aus der RTC
  o += ",\"dst\":";             o += (int)s_dstForced;
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
  o += ",\"host\":";                 jsonStr(o, (String(s_host) + ".local").c_str());
  o += "},\"ap\":{\"active\":";      o += s_apActive ? "true" : "false";
  o += ",\"ssid\":";                 jsonStr(o, WEB_SETUP_AP_SSID);
  o += ",\"ip\":";                   jsonStr(o, WiFi.softAPIP().toString().c_str());
  o += "},\"profile\":";             jsonStr(o, profileName(g_scopeProfile));
  o += ",\"dial\":";                 o += (int)g_dialStyle;
  o += ",\"fps\":";                  o += s_fps;
  o += ",\"uptime\":";               o += (uint32_t)(millis() / 1000UL);
  o += ",\"heap\":";                 o += (uint32_t)ESP.getFreeHeap();        // RAM frei (Byte)
  o += ",\"heapTotal\":";            o += (uint32_t)ESP.getHeapSize();        // RAM gesamt (Heap)
  o += ",\"heapMin\":";              o += (uint32_t)ESP.getMinFreeHeap();     // kleinster freier Wert seit Start
  o += ",\"heapBlock\":";            o += (uint32_t)ESP.getMaxAllocHeap();    // größter freier Block
  o += ",\"cpu0\":";                 o += (int)s_cpu0Load;                    // Last Core 0 in %
  o += ",\"reset\":";                jsonStr(o, resetReasonText());
  o += ",\"fw\":";                   jsonStr(o, FW_VERSION);
  o += ",\"test\":";                 o += g_testPattern ? "true" : "false";
  o += ",\"oled\":";                 o += (int)s_oledPct;                     // OLED-Helligkeit in %, 0 = aus
  o += ",\"rtc\":{\"enabled\":";      o += s_rtcEnabled ? "true" : "false";     // Echtzeituhr (DS3231)
  o += ",\"found\":";              o += s_rtcFound ? "true" : "false";
  o += ",\"valid\":";              o += s_rtcValid ? "true" : "false";
  o += '}';
  o += ",\"h12\":";                  o += s_h12 ? "true" : "false";
  o += ",\"lang\":";                 o += s_lang ? "\"en\"" : "\"de\"";
  o += '}';
  web.send(200, "application/json", o);
}

// GET /api/vectors – Vektordaten für die Oszilloskop-Vorschau der Webseite:
// dieselben Tabellen, die auch der scopeTask zeichnet (Zifferblatt, Ziffern aller
// Zifferblätter, alle 60 Stellungen der drei Zeiger). Fertig erzeugt mit
// tools/gen_vectors.py --vectors (vectors.h, gzip ca. 2 KB) – die Firmware muss
// nichts zusammensetzen. Die Seite lädt sie einmal und stellt die Zeiger selbst.
static void handleVectors()
{
  web.sendHeader("Content-Encoding", "gzip");
  web.sendHeader("Cache-Control", "max-age=3600");
  web.send_P(200, "application/json", (const char *)VECTORS_JSON_GZ, sizeof(VECTORS_JSON_GZ));
}

// GET /api/networks – gespeicherte Netze (nur Namen, nie Passwörter)
static void handleNetworks()
{
  String o = "[";
  bool first = true;
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
  // wifiTask für die Dauer der Suche pausieren (er würde sonst selbst suchen und
  // dabei die Ergebnisse löschen). Läuft sein Versuch gerade, wartet die Seite einfach.
  s_webScanUntil = millis() + 15000;
  if (s_multiBusy) { web.send(200, "application/json", "{\"running\":true}"); return; }
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
  s_webScanUntil = 0;                                        // fertig: wifiTask darf wieder
  web.send(200, "application/json", o);
}

// POST /api/wifi (ssid, pw) – Netz speichern bzw. Passwort ändern
// Gerätename prüfen: 1–32 Zeichen, a–z, 0–9, Bindestrich (nicht am Anfang/Ende)
static bool hostValid(const String &h)
{
  if (h.length() < 1 || h.length() > 32 || h[0] == '-' || h[h.length() - 1] == '-') return false;
  for (size_t i = 0; i < h.length(); i++) { const char c = h[i]; if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')) return false; }
  return true;
}

static void handleWifiSave()
{
  const String ssid = web.arg("ssid");
  const String pw   = web.arg("pw");
  if (ssid.length() < 1 || ssid.length() > 32) { sendJsonResult(false, tr("Netzwerkname: 1 bis 32 Zeichen", "Network name: 1 to 32 characters")); return; }
  if (pw.length() > 0 && (pw.length() < 8 || pw.length() > 63)) { sendJsonResult(false, tr("Passwort: 8 bis 63 Zeichen (oder leer für offene Netze)", "Password: 8 to 63 characters (or empty for open networks)")); return; }

  int slot = -1;
  for (int i = 0; i < s_webNetCount; i++) if (ssid == s_webNets[i].ssid) slot = i;   // vorhandenes ersetzen
  const bool isNew = slot < 0;
  if (isNew) {
    if (s_webNetCount >= WEB_MAX_NETWORKS) { sendJsonResult(false, tr("Speicher voll – bitte zuerst ein Netz löschen", "Storage full – please delete a network first")); return; }
    slot = s_webNetCount;
  } else if (pw == s_webNets[slot].pw) {
    sendJsonResult(true, tr("Dieses Netz ist bereits mit diesem Passwort gespeichert.", "This network is already saved with this password.")); return;
  }
  strlcpy(s_webNets[slot].ssid, ssid.c_str(), sizeof(s_webNets[slot].ssid));
  strlcpy(s_webNets[slot].pw,   pw.c_str(),   sizeof(s_webNets[slot].pw));
  if (isNew) s_webNetCount = slot + 1;          // erst jetzt sichtbar – der wifiTask trägt es bei WiFiMulti ein
  webNetsSave();
  logMsg(tr("[Web] WLAN \"%s\" %s", "[Web] network \"%s\" %s"), s_webNets[slot].ssid, isNew ? tr("gespeichert", "saved") : tr("aktualisiert – Neustart", "updated – restarting"));

  // Neues Passwort für ein bekanntes Netz: WiFiMulti kann einen Eintrag nicht ändern
  // (es würde weiter das alte Passwort nehmen) – daher wie beim Löschen neu starten.
  if (!isNew) {
    sendJsonResult(true, tr("Passwort geändert. Die Uhr startet neu – Seite in ca. 10 s neu laden.", "Password changed. The clock restarts – reload the page in about 10 s."));
    s_restartAtMs = millis() + 1500;
    return;
  }
  if (WiFi.status() == WL_CONNECTED)
    sendJsonResult(true, tr("Gespeichert. Wird beim nächsten Verbindungsaufbau genutzt – oder „Neu verbinden“ antippen.", "Saved. Used on the next connection attempt – or tap “Reconnect”."));
  else
    sendJsonResult(true, tr("Gespeichert. Die Uhr verbindet sich jetzt …", "Saved. The clock is connecting now …"));
}

// POST /api/wifi/delete (ssid) – nur im Webinterface gespeicherte Netze.
// WiFiMulti kann Netze nicht wieder vergessen, deshalb startet die Uhr neu.
static void handleWifiDelete()
{
  const String ssid = web.arg("ssid");
  int slot = -1;
  for (int i = 0; i < s_webNetCount; i++) if (ssid == s_webNets[i].ssid) slot = i;
  if (slot < 0) { sendJsonResult(false, tr("Dieses Netz ist nicht im Webinterface gespeichert", "This network is not saved in the web interface")); return; }
  for (int i = slot; i < s_webNetCount - 1; i++) s_webNets[i] = s_webNets[i + 1];
  s_webNetCount = s_webNetCount - 1;
  webNetsSave();
  logMsg(tr("[Web] WLAN \"%s\" geloescht – Neustart", "[Web] network \"%s\" deleted – restarting"), ssid.c_str());
  sendJsonResult(true, tr("Gelöscht. Die Uhr startet neu – Seite in ca. 10 s neu laden.", "Deleted. The clock restarts – reload the page in about 10 s."));
  s_restartAtMs = millis() + 1500;
}

// POST /api/wifi/reset – alle gespeicherten WLANs löschen, danach Neustart.
// Ohne bekanntes Netz öffnet die Uhr anschließend das Setup-WLAN.
static void handleWifiReset()
{
  s_webNetCount = 0;
  webNetsSave();
  logMsg(tr("[Web] alle WLANs geloescht – Neustart", "[Web] all networks deleted – restarting"));
  sendJsonResult(true, tr("Alle Netze gelöscht. Die Uhr startet neu und öffnet das Setup-WLAN „ScopeClock-Setup“.",
                          "All networks deleted. The clock restarts and opens the setup network “ScopeClock-Setup”."));
  s_restartAtMs = millis() + 1500;
}

// ---- Sommer-/Winterzeit von Hand (nur für die manuell gestellte Uhrzeit) ----
// Liest aus einer POSIX-Zeitzone wie "CET-1CEST,M3.5.0,M10.5.0/3" Name und
// Versatz (in Sekunden, POSIX-Vorzeichen: "-1" = UTC+1) ab Position p.
static bool tzPart(const char *&p, char *name, size_t nameLen, long &off, bool needOff)
{
  size_t n = 0;
  if (*p == '<') { p++; while (*p && *p != '>' && n + 1 < nameLen) name[n++] = *p++; if (*p != '>') return false; p++; }
  else while (isalpha((unsigned char)*p) && n + 1 < nameLen) name[n++] = *p++;
  name[n] = 0;
  if (n < 3) return false;
  if (!(*p == '+' || *p == '-' || isdigit((unsigned char)*p))) return !needOff;
  const int sign = (*p == '-') ? -1 : 1;
  if (*p == '+' || *p == '-') p++;
  long h = 0, m = 0, sec = 0;
  while (isdigit((unsigned char)*p)) h = h * 10 + (*p++ - '0');
  if (*p == ':') { p++; while (isdigit((unsigned char)*p)) m = m * 10 + (*p++ - '0'); }
  if (*p == ':') { p++; while (isdigit((unsigned char)*p)) sec = sec * 10 + (*p++ - '0'); }
  off = sign * (h * 3600 + m * 60 + sec);
  return true;
}

// Zeitzone fest auf Winterzeit (mode 0) bzw. Sommerzeit (mode 1) stellen – ohne
// automatischen Wechsel. mode -1 = wieder die normale Zeitzone mit Umschaltregeln.
// Rückgabe false: Die Zeitzone kennt keine Sommerzeit (z.B. UTC) – nichts geändert.
static bool applyDstOverride(int mode)
{
  if (mode < 0) {
    setenv("TZ", s_tz, 1); tzset();
    s_dstForced = -1;
    return true;
  }
  char stdName[16], dstName[16];
  long stdOff = 0, dstOff = 0;
  const char *p = s_tz;
  if (!tzPart(p, stdName, sizeof(stdName), stdOff, true)) return false;
  if (!*p || *p == ',') return false;                       // keine Sommerzeit-Regel
  dstOff = stdOff - 3600;                                   // Standard: eine Stunde vor
  if (!tzPart(p, dstName, sizeof(dstName), dstOff, false)) return false;
  const char *name = mode ? dstName : stdName;
  const long  off  = mode ? dstOff  : stdOff;
  const long  a    = off < 0 ? -off : off;
  char tz[48];
  bool alpha = true;                                        // <…> nur, wenn nötig (z.B. "+03")
  for (const char *c = name; *c; c++) if (!isalpha((unsigned char)*c)) alpha = false;
  snprintf(tz, sizeof(tz), alpha ? "%s%s%ld:%02ld" : "<%s>%s%ld:%02ld",
           name, off < 0 ? "-" : "", a / 3600, (a % 3600) / 60);
  setenv("TZ", tz, 1); tzset();
  s_dstForced = (int8_t)mode;
  return true;
}

// POST /api/time – Uhrzeit von Hand stellen (für den Betrieb ohne Internet).
//   epoch=<Unix-Sekunden, UTC>        "Uhrzeit vom Handy übernehmen"
//   local=JJJJ-MM-TTTHH:MM[:SS]       Datum/Uhrzeit aus den Eingabefeldern,
//                                     gilt in der eingestellten Zeitzone
//   dst=-1/0/1 (optional)             automatisch / Winterzeit / Sommerzeit fest
// Ein späterer NTP-Abgleich überschreibt die Zeit wieder (gewollt).
static void handleTimeSet()
{
  // Sommer-/Winterzeit zuerst festlegen – mktime() rechnet dann mit dem richtigen Versatz
  int dst = web.hasArg("dst") ? web.arg("dst").toInt() : -1;
  if (dst > 1) dst = 1;
  if (dst < 0 || !applyDstOverride(dst)) { applyDstOverride(-1); dst = -1; }
  time_t e = 0;
  if (web.hasArg("epoch")) {
    e = (time_t)strtoll(web.arg("epoch").c_str(), nullptr, 10);
  } else if (web.hasArg("local")) {
    struct tm t = {};
    // Zahlen der Reihe nach lesen (ohne sscanf – spart ca. 20 KB Flash)
    int v[6] = {0, 0, 0, 0, 0, 0}, n = 0;
    const String arg = web.arg("local");                 // String muss leben, solange c darauf zeigt
    for (const char *c = arg.c_str(); *c && n < 6; ) {
      if (*c < '0' || *c > '9') { c++; continue; }
      while (*c >= '0' && *c <= '9') v[n] = v[n] * 10 + (*c++ - '0');
      n++;
    }
    const int Y = v[0], M = v[1], D = v[2], h = v[3], m = v[4], sec = v[5];
    if (n < 5 || M < 1 || M > 12 || D < 1 || D > 31 || h < 0 || h > 23 || m < 0 || m > 59 || sec < 0 || sec > 59) {
      sendJsonResult(false, tr("Datum oder Uhrzeit ungültig", "Invalid date or time")); return;
    }
    t.tm_year = Y - 1900; t.tm_mon = M - 1; t.tm_mday = D;
    t.tm_hour = h; t.tm_min = m; t.tm_sec = sec;
    t.tm_isdst = dst < 0 ? -1 : 0;                        // automatisch: Sommerzeit aus der Zeitzone bestimmen
    e = mktime(&t);
  }
  // Plausibel: 2024 bis 2099
  if (e < (time_t)1704067200 || e > (time_t)4102444800LL) { sendJsonResult(false, tr("Datum muss zwischen 2024 und 2099 liegen", "Date must be between 2024 and 2099")); return; }
  struct timeval tv = { e, 0 };
  settimeofday(&tv, nullptr);
  g_timeManual = true;
  if (s_rtcEnabled) rtcSetFromSystem();                 // auch die Echtzeituhr stellen
  struct tm lt;
  localtime_r(&e, &lt);
  logMsg(tr("[Zeit] von Hand gestellt (%s, %s): %02d.%02d.%04d %02d:%02d:%02d", "[Time] set by hand (%s, %s): %02d.%02d.%04d %02d:%02d:%02d"),
         web.hasArg("epoch") ? tr("vom Handy", "from phone") : tr("Eingabe", "input"),
         dst < 0 ? tr("Sommer-/Winterzeit automatisch", "summer/winter time automatic")
                 : dst ? tr("Sommerzeit", "summer time") : tr("Winterzeit", "winter time"),
         lt.tm_mday, lt.tm_mon + 1, lt.tm_year + 1900, lt.tm_hour, lt.tm_min, lt.tm_sec);
  char msg[64];                                         // Rückmeldung mit der gestellten Zeit
  if (s_lang) snprintf(msg, sizeof(msg), "Time set: %04d-%02d-%02d %02d:%02d:%02d", lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
  else        snprintf(msg, sizeof(msg), "Uhrzeit gestellt: %02d.%02d.%04d %02d:%02d:%02d", lt.tm_mday, lt.tm_mon + 1, lt.tm_year + 1900, lt.tm_hour, lt.tm_min, lt.tm_sec);
  sendJsonResult(true, msg);
}

// POST /api/dial (style = 0 … DIAL_STYLES-1, siehe dialDigits) – Zifferblatt umschalten: gilt sofort
// für das Oszilloskop (scopeTask baut die Rundtour neu) und wird gespeichert.
static void handleDial()
{
  const int st = web.arg("style").toInt();
  if (!web.hasArg("style") || st < 0 || st >= DIAL_STYLES) { sendJsonResult(false, tr("Zifferblatt: 0 bis 4", "Dial: 0 to 4")); return; }
  if (st != g_dialStyle) {
    g_dialStyle = (uint8_t)st;
    saveSetting("dial", (uint8_t)st);
    logMsg(tr("[Scope] Zifferblatt: %s, gespeichert", "[Scope] dial: %s, saved"), dialName(st));
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
//   Schlüssel  Wert                          Bereich
//   profile    Oszilloskop-Profil            0 = ANALOG, 1 = DIGITAL
//   dial       Zifferblatt                   0…4 (siehe dialDigits)
//   beam       Verweilzeit des Strahls       1000…10000 ns pro Schritt
//   hand       Helligkeit Std.-/Min.-Zeiger  0…100 %
//   tick       Helligkeit Minutenstriche     0…100 %
//   digit      Helligkeit Ziffern            0…100 %
//   jump       Rücklauf-Tempo                0 (hart) … 48 (nur Profil ANALOG)
//   zbl, zhi   Dunkeltastung an / High = dunkel   0/1
//   tz         Zeitzone                      eine aus TIMEZONES[] bzw. TZ_INFO
//   ntp        erster NTP-Server             Hostname, max. 63 Zeichen
//   h12        12-Stunden-Format             0/1
//   rtc        Echtzeituhr DS3231 nutzen     0/1
//   oledp      OLED-Helligkeit               0…100 % (0 = aus; früher "oled" 1…255)
//   osec       Sekunden auf dem OLED         0/1
//   shift      OLED-Pixel-Shift              0/1
//   flip       OLED um 180° gedreht          0/1
//   lang       Sprache                       0 = Deutsch, 1 = English
//   host       Gerätename                    a–z, 0–9, Bindestrich, max. 32
//   netn, net_s0…, net_p0…  gespeicherte WLANs (siehe webNetsLoad)

struct TzOption { const char *label; const char *labelEn; const char *tz; };
static const TzOption TIMEZONES[] = {
  { "Mitteleuropa (Berlin, Wien, Zürich)", "Central Europe (Berlin, Vienna, Zurich)", "CET-1CEST,M3.5.0,M10.5.0/3" },
  { "Westeuropa (London, Lissabon)",       "Western Europe (London, Lisbon)","GMT0BST,M3.5.0/1,M10.5.0" },
  { "Osteuropa (Helsinki, Athen)",         "Eastern Europe (Helsinki, Athens)","EET-2EEST,M3.5.0/3,M10.5.0/4" },
  { "Moskau",                              "Moscow","MSK-3" },
  { "UTC",                                 "UTC","UTC0" },
  { "USA Ost (New York)",                  "US Eastern (New York)","EST5EDT,M3.2.0,M11.1.0" },
  { "USA Zentral (Chicago)",               "US Central (Chicago)","CST6CDT,M3.2.0,M11.1.0" },
  { "USA Mountain (Denver)",               "US Mountain (Denver)","MST7MDT,M3.2.0,M11.1.0" },
  { "USA West (Los Angeles)",              "US Pacific (Los Angeles)","PST8PDT,M3.2.0,M11.1.0" },
  { "Indien (Delhi)",                      "India (Delhi)","IST-5:30" },
  { "China (Peking)",                      "China (Beijing)","CST-8" },
  { "Japan (Tokio)",                       "Japan (Tokyo)","JST-9" },
  { "Australien Ost (Sydney)",             "Australia Eastern (Sydney)","AEST-10AEDT,M10.1.0,M4.1.0/3" },
};
static const int TIMEZONE_COUNT = sizeof(TIMEZONES) / sizeof(TIMEZONES[0]);

// true, wenn die Zeitzone aus der Liste stammt (oder TZ_INFO aus config.h ist)
static bool tzAllowed(const char *tz)
{
  if (!strcmp(tz, TZ_INFO)) return true;
  for (int i = 0; i < TIMEZONE_COUNT; i++) if (!strcmp(tz, TIMEZONES[i].tz)) return true;
  return false;
}

// Prüft einen NTP-Servernamen: 1–63 Zeichen, nur Buchstaben, Ziffern, Punkt, Bindestrich
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
  g_handPct   = hand <= 100 ? hand : HAND_BRIGHT_PCT;
  const uint8_t tick = p.getUChar("tick", TICK_BRIGHT_PCT);
  g_tickPct   = tick <= 100 ? tick : 100;                  // ältere Werte bis 150 % → 100 %
  const uint8_t digit = p.getUChar("digit", DIGIT_BRIGHT_PCT);
  g_digitPct  = digit <= 100 ? digit : 100;
  const uint8_t jump = p.getUChar("jump", JUMP_SPEED);
  g_jumpSpeed = jump <= 48 ? jump : JUMP_SPEED;
  g_zblank     = p.getUChar("zbl", ZBLANK_ENABLE) != 0;
  g_zblankHigh = p.getUChar("zhi", ZBLANK_ACTIVE_HIGH) != 0;
  char buf[64];
  if (p.getString("tz", buf, sizeof(buf)) && tzAllowed(buf)) strlcpy(s_tz, buf, sizeof(s_tz));
  if (p.getString("ntp", buf, sizeof(buf)) && hostnameValid(String(buf))) strlcpy(s_ntpServer, buf, sizeof(s_ntpServer));
  // OLED-Helligkeit in Prozent; ältere Firmware speicherte den Kontrast 1–255 ("oled")
  if (p.isKey("oledp")) s_oledPct = min((int)p.getUChar("oledp", OLED_BRIGHT_PCT), 100);
  else if (p.isKey("oled")) s_oledPct = (p.getUChar("oled", 128) * 100 + 127) / 255;
  s_oledSec      = p.getUChar("osec", OLED_SECONDS) != 0;
  char hb[33];
  if (p.getString("host", hb, sizeof(hb)) && hostValid(String(hb))) strlcpy(s_host, hb, sizeof(s_host));
  s_oledShift    = p.getUChar("shift", OLED_SHIFT_EVERY_S > 0) != 0;
  s_oledFlip     = p.getUChar("flip", OLED_FLIP) != 0;
  s_h12          = p.getUChar("h12", CLOCK_12H) != 0;
  s_lang         = p.getUChar("lang", UI_LANG) ? 1 : 0;
  s_rtcEnabled   = p.getUChar("rtc", RTC_ENABLE) != 0;
  p.end();
}

// Aktuelle Einstellungen, Standardwerte und Zeitzonen als JSON.
static void settingsJson(String &o)
{
  o += "{\"profile\":";      o += (int)g_scopeProfile;
  o += ",\"dial\":";         o += (int)g_dialStyle;
  o += ",\"beamNs\":";       o += (int)g_beamNs;
  o += ",\"handPct\":";      o += (int)g_handPct;
  o += ",\"tickPct\":";      o += (int)g_tickPct;
  o += ",\"digitPct\":";     o += (int)g_digitPct;
  o += ",\"jumpSpeed\":";    o += (int)g_jumpSpeed;
  o += ",\"zblank\":";       o += g_zblank ? "true" : "false";
  o += ",\"zblankHigh\":";   o += g_zblankHigh ? "true" : "false";
  o += ",\"zblankPin\":";    o += ZBLANK_PIN;
  o += ",\"tz\":";           jsonStr(o, s_tz);
  o += ",\"ntp\":";          jsonStr(o, s_ntpServer);
  o += ",\"oledPct\":";      o += (int)s_oledPct;
  o += ",\"oledSec\":";      o += s_oledSec ? "true" : "false";
  o += ",\"host\":";         jsonStr(o, s_host);
  o += ",\"test\":";         o += g_testPattern ? "true" : "false";
  o += ",\"oledShift\":";    o += s_oledShift ? "true" : "false";
  o += ",\"oledFlip\":";     o += s_oledFlip ? "true" : "false";
  o += ",\"h12\":";          o += s_h12 ? "true" : "false";
  o += ",\"rtc\":";          o += s_rtcEnabled ? "true" : "false";
  o += ",\"rtcSda\":";       o += RTC_SDA_PIN;
  o += ",\"rtcScl\":";       o += RTC_SCL_PIN;
  o += ",\"lang\":";         o += s_lang ? "\"en\"" : "\"de\"";
  o += ",\"defaults\":{\"profile\":"; o += SCOPE_PROFILE;
  o += ",\"dial\":";         o += DIAL_STYLE;
  o += ",\"beamNs\":";       o += BEAM_NS_PER_UNIT;
  o += ",\"handPct\":";      o += HAND_BRIGHT_PCT;
  o += ",\"tickPct\":";      o += TICK_BRIGHT_PCT;
  o += ",\"digitPct\":";     o += DIGIT_BRIGHT_PCT;
  o += ",\"jumpSpeed\":";    o += JUMP_SPEED;
  o += ",\"zblank\":";       o += ZBLANK_ENABLE ? "true" : "false";
  o += ",\"zblankHigh\":";   o += ZBLANK_ACTIVE_HIGH ? "true" : "false";
  o += ",\"tz\":";           jsonStr(o, TZ_INFO);
  o += ",\"ntp\":";          jsonStr(o, NTP_SERVER_1);
  o += ",\"oledPct\":";      o += OLED_BRIGHT_PCT;
  o += ",\"oledSec\":";      o += OLED_SECONDS ? "true" : "false";
  o += ",\"host\":";         jsonStr(o, WIFI_HOSTNAME);
  o += ",\"oledShift\":";    o += OLED_SHIFT_EVERY_S > 0 ? "true" : "false";
  o += ",\"oledFlip\":";     o += OLED_FLIP ? "true" : "false";
  o += ",\"h12\":";          o += CLOCK_12H ? "true" : "false";
  o += ",\"rtc\":";          o += RTC_ENABLE ? "true" : "false";
  o += ",\"lang\":";         o += UI_LANG ? "\"en\"" : "\"de\"";
  o += "},\"timezones\":[";
  bool cfgListed = false;
  for (int i = 0; i < TIMEZONE_COUNT; i++) {
    if (i) o += ',';
    o += "{\"label\":"; jsonStr(o, tr(TIMEZONES[i].label, TIMEZONES[i].labelEn)); o += ",\"tz\":"; jsonStr(o, TIMEZONES[i].tz); o += '}';
    if (!strcmp(TIMEZONES[i].tz, TZ_INFO)) cfgListed = true;
  }
  if (!cfgListed) { o += ",{\"label\":"; jsonStr(o, tr("aus config.h", "from config.h")); o += ",\"tz\":"; jsonStr(o, TZ_INFO); o += '}'; }
  o += "]}";
}

// Antwort mit Meldung und allen aktuellen Einstellungen senden
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
  s_dstForced = -1;                        // configTzTime setzt die normale Zeitzone
}

// GET /api/settings
static void handleSettingsGet() { sendSettings(true, ""); }

// POST /api/settings – beliebige Teilmenge der Felder; jedes Feld wird geprüft,
// sofort angewendet und gespeichert.
static void handleSettingsPost()
{
  Preferences p;
  if (!p.begin("scopeclock", false)) { sendSettings(false, tr("Flash-Speicher nicht verfügbar", "Flash storage not available")); return; }
  String err;
  int changed = 0;
  auto bad = [&](const char *m) { if (err.length()) err += "; "; err += m; };

  if (web.hasArg("profile")) {
    const int v = web.arg("profile").toInt();
    if (v == 0 || v == 1) { g_scopeProfile = v; p.putUChar("profile", v); changed++;
      logMsg(tr("[Web] Oszilloskop-Profil: %s", "[Web] oscilloscope profile: %s"), profileName(v)); }
    else bad(tr("Oszilloskop-Profil ungültig", "Invalid oscilloscope profile"));
  }
  if (web.hasArg("dial")) {
    const int v = web.arg("dial").toInt();
    if (v >= 0 && v < DIAL_STYLES) { g_dialStyle = v; p.putUChar("dial", v); changed++;
      logMsg(tr("[Web] Zifferblatt: %s", "[Web] dial: %s"), dialName(v)); }
    else bad(tr("Zifferblatt ungültig", "Invalid dial"));
  }
  if (web.hasArg("beamNs")) {
    const int v = web.arg("beamNs").toInt();
    if (v >= 1000 && v <= 10000) { g_beamNs = v; p.putUShort("beam", v); changed++;
      logMsg(tr("[Web] Verweilzeit des Strahls: %d ns pro Schritt", "[Web] beam dwell time: %d ns per step"), v); }
    else bad(tr("Verweilzeit des Strahls: 1 bis 10 µs", "Beam dwell time: 1 to 10 µs"));
  }
  if (web.hasArg("handPct")) {
    const int v = web.arg("handPct").toInt();
    if (v >= 0 && v <= 100) { g_handPct = v; p.putUChar("hand", v); changed++;
      logMsg(tr("[Web] Zeigerhelligkeit: %d %%", "[Web] hand brightness: %d %%"), v); }
    else bad(tr("Zeigerhelligkeit: 0 bis 100 %", "Hand brightness: 0 to 100 %"));
  }
  if (web.hasArg("tickPct")) {
    const int v = web.arg("tickPct").toInt();
    if (v >= 0 && v <= 100) { g_tickPct = v; p.putUChar("tick", v); changed++;
      logMsg(tr("[Web] Helligkeit Minutenstriche: %d %%", "[Web] tick mark brightness: %d %%"), v); }
    else bad(tr("Helligkeit Minutenstriche: 0 bis 100 %", "Tick mark brightness: 0 to 100 %"));
  }
  if (web.hasArg("digitPct")) {
    const int v = web.arg("digitPct").toInt();
    if (v >= 0 && v <= 100) { g_digitPct = v; p.putUChar("digit", v); changed++;
      logMsg(tr("[Web] Helligkeit Ziffern: %d %%", "[Web] numeral brightness: %d %%"), v); }
    else bad(tr("Helligkeit Ziffern: 0 bis 100 %", "Numeral brightness: 0 to 100 %"));
  }
  if (web.hasArg("jumpSpeed")) {
    const int v = web.arg("jumpSpeed").toInt();
    if (v >= 0 && v <= 48) { g_jumpSpeed = v; p.putUChar("jump", v); changed++;
      logMsg(tr("[Web] Ruecklauf-Tempo: %d%s", "[Web] retrace speed: %d%s"), v, v ? "x" : tr(" (sofortiger Sprung)", " (instant jump)")); }
    else bad(tr("Rücklauf-Tempo: 0 bis 48", "Retrace speed: 0 to 48"));
  }
  // Dunkeltastung über den Z-Eingang: an/aus und Pegel (High oder Low = dunkel)
  if (web.hasArg("zblank") || web.hasArg("zblankHigh")) {
    const bool on = web.hasArg("zblank")     ? (web.arg("zblank") == "1" || web.arg("zblank") == "true") : g_zblank;
    const bool hi = web.hasArg("zblankHigh") ? (web.arg("zblankHigh") == "1" || web.arg("zblankHigh") == "true") : g_zblankHigh;
    zblankApply(on, hi);
    p.putUChar("zbl", on); p.putUChar("zhi", hi); changed++;
    logMsg(tr("[Web] Dunkeltastung (GPIO%d): %s", "[Web] Z blanking (GPIO%d): %s"), ZBLANK_PIN,
           !on ? tr("aus", "off") : hi ? tr("an, High = dunkel", "on, high = dark") : tr("an, Low = dunkel", "on, low = dark"));
  }
  bool timeChanged = false;
  if (web.hasArg("tz")) {
    const String v = web.arg("tz");
    if (tzAllowed(v.c_str())) { strlcpy(s_tz, v.c_str(), sizeof(s_tz)); p.putString("tz", s_tz); changed++; timeChanged = true;
      logMsg(tr("[Web] Zeitzone: %s", "[Web] time zone: %s"), s_tz); }
    else bad(tr("Zeitzone unbekannt", "Unknown time zone"));
  }
  if (web.hasArg("ntp")) {
    const String v = web.arg("ntp");
    if (hostnameValid(v)) { strlcpy(s_ntpServer, v.c_str(), sizeof(s_ntpServer)); p.putString("ntp", s_ntpServer); changed++; timeChanged = true;
      logMsg(tr("[Web] Zeitserver: %s", "[Web] time server: %s"), s_ntpServer); }
    else bad(tr("Zeitserver: nur Buchstaben, Ziffern, Punkt und Bindestrich (max. 63 Zeichen)", "Time server: only letters, digits, dot and hyphen (max. 63 characters)"));
  }
  // OLED: Helligkeit 0–100 % (0 = aus), Sekunden (wirkt beim nächsten Neuzeichnen)
  if (web.hasArg("oledPct")) {
    const int v = web.arg("oledPct").toInt();
    if (v >= 0 && v <= 100) { s_oledPct = v; p.putUChar("oledp", v); changed++; }
    else bad(tr("OLED-Helligkeit: 0 bis 100 %", "OLED brightness: 0 to 100 %"));
  }
  // Gerätename: wirkt sofort für http://<name>.local, im Router nach dem nächsten Verbinden
  if (web.hasArg("host")) {
    String v = web.arg("host"); v.toLowerCase();
    if (hostValid(v)) {
      strlcpy(s_host, v.c_str(), sizeof(s_host)); p.putString("host", s_host); changed++;
      WiFi.setHostname(s_host);
      if (WiFi.status() == WL_CONNECTED) { MDNS.end(); if (MDNS.begin(s_host)) MDNS.addService("http", "tcp", 80); }
      logMsg(tr("[Web] Geraetename: %s (http://%s.local)", "[Web] device name: %s (http://%s.local)"), s_host, s_host);
    } else bad(tr("Gerätename: 1–32 Zeichen, nur a–z, 0–9 und Bindestrich", "Device name: 1–32 characters, only a–z, 0–9 and hyphen"));
  }
  // Testbild: nur bis zum Ausschalten oder Neustart (wird nicht gespeichert)
  if (web.hasArg("test")) {
    g_testPattern = web.arg("test") == "1" || web.arg("test") == "true"; changed++;
    s_bootTest = false;                                  // von Hand geschaltet: Start-Testbild nicht mehr automatisch beenden
    logMsg(tr("[Web] Testbild: %s", "[Web] test pattern: %s"), g_testPattern ? tr("an", "on") : tr("aus", "off"));
  }
  if (web.hasArg("oledSec")) {
    s_oledSec = web.arg("oledSec") == "1" || web.arg("oledSec") == "true";
    p.putUChar("osec", s_oledSec); changed++;
  }
  // Zeitformat für OLED und Webinterface: 12 Stunden (AM/PM) oder 24 Stunden
  if (web.hasArg("h12")) {
    s_h12 = web.arg("h12") == "1" || web.arg("h12") == "true";
    p.putUChar("h12", s_h12); changed++;
    logMsg(tr("[Web] Zeitformat: %s", "[Web] time format: %s"), s_h12 ? tr("12 Stunden", "12 hours") : tr("24 Stunden", "24 hours"));
  }
  if (web.hasArg("oledFlip")) {
    s_oledFlip = web.arg("oledFlip") == "1" || web.arg("oledFlip") == "true";
    oledSetFlip(s_oledFlip);
    p.putUChar("flip", s_oledFlip); changed++;
    logMsg(tr("[Web] OLED-Ausrichtung: %s", "[Web] OLED orientation: %s"), s_oledFlip ? tr("um 180 Grad gedreht", "rotated by 180 degrees") : tr("normal", "normal"));
  }
  if (web.hasArg("oledShift")) {
    s_oledShift = web.arg("oledShift") == "1" || web.arg("oledShift") == "true";
    p.putUChar("shift", s_oledShift); changed++;
  }
  // Echtzeituhr DS3231 ein/aus
  if (web.hasArg("rtc")) {
    const bool on = web.arg("rtc") == "1" || web.arg("rtc") == "true";
    p.putUChar("rtc", on); changed++;
    logMsg(tr("[Web] Echtzeituhr (RTC): %s", "[Web] real-time clock (RTC): %s"), on ? tr("an", "on") : tr("aus", "off"));
    rtcApply(on);
  }
  // Sprache für Webinterface, OLED und Protokoll ("de" oder "en")
  if (web.hasArg("lang")) {
    const String v = web.arg("lang");
    if (v == "de" || v == "en") {
      s_lang = v == "en" ? 1 : 0;
      p.putUChar("lang", s_lang); changed++;
      logMsg("%s", tr("[Web] Sprache: Deutsch", "[Web] language: English"));
    } else bad(tr("Sprache: de oder en", "Language: de or en"));
  }
  p.end();
  if (timeChanged) applyTime();

  if (err.length()) sendSettings(false, err.c_str());
  else sendSettings(true, changed ? tr("Gespeichert", "Saved") : tr("Keine Änderung", "No change"));
}

// POST /api/settings/reset – Einstellungen einer Gruppe auf die Werte aus config.h
//   group=scope    Oszilloskop: Profil, Zifferblatt, Verweilzeit, Zeigerhelligkeit, Rücklauf-Tempo
//   group=time     Uhrzeit: Zeitformat, Zeitzone, Zeitserver
//   group=display  Display: OLED-Helligkeit, Pixelverschiebung, Drehung
//   ohne group     alles (auch die Sprache); gespeicherte WLANs bleiben immer erhalten
static void handleSettingsReset()
{
  const String g = web.arg("group");
  const bool all = g.length() == 0;
  const bool scope = all || g == "scope", tim = all || g == "time", disp = all || g == "display";
  if (!scope && !tim && !disp) { sendSettings(false, tr("Unbekannte Gruppe", "Unknown group")); return; }
  Preferences p;
  if (p.begin("scopeclock", false)) {
    if (scope) for (const char *k : { "profile", "dial", "beam", "hand", "tick", "digit", "jump", "zbl", "zhi" }) p.remove(k);
    if (tim)   for (const char *k : { "tz", "ntp", "h12", "rtc" }) p.remove(k);
    if (disp)  for (const char *k : { "oled", "oledp", "osec", "night", "nfrom", "nto", "npct", "shift", "flip" }) p.remove(k);
    if (all)   { p.remove("lang"); p.remove("host"); }
    p.end();
  }
  if (scope) {
    g_scopeProfile = SCOPE_PROFILE;  g_dialStyle = DIAL_STYLE;
    g_beamNs = BEAM_NS_PER_UNIT;     g_handPct = HAND_BRIGHT_PCT;   g_jumpSpeed = JUMP_SPEED;
    g_tickPct = TICK_BRIGHT_PCT;     g_digitPct = DIGIT_BRIGHT_PCT;
    zblankApply(ZBLANK_ENABLE, ZBLANK_ACTIVE_HIGH);
  }
  if (tim) {
    strlcpy(s_tz, TZ_INFO, sizeof(s_tz));
    strlcpy(s_ntpServer, NTP_SERVER_1, sizeof(s_ntpServer));
    s_h12 = CLOCK_12H;
    applyTime();
    if (s_rtcEnabled != (bool)RTC_ENABLE) rtcApply(RTC_ENABLE);
  }
  if (disp) {
    s_oledPct = OLED_BRIGHT_PCT;     s_oledSec = OLED_SECONDS;
    s_oledShift = OLED_SHIFT_EVERY_S > 0;
    s_oledFlip  = OLED_FLIP;  oledSetFlip(s_oledFlip);
  }
  if (all) {
    s_lang = UI_LANG;
    g_testPattern = false; s_bootTest = false;
    if (strcmp(s_host, WIFI_HOSTNAME) != 0) {           // Gerätename zurück auf WIFI_HOSTNAME
      strlcpy(s_host, WIFI_HOSTNAME, sizeof(s_host));
      WiFi.setHostname(s_host);
      if (WiFi.status() == WL_CONNECTED) { MDNS.end(); if (MDNS.begin(s_host)) MDNS.addService("http", "tcp", 80); }
    }
  }
  const char *name = all ? tr("alle", "all") : scope ? tr("Oszilloskop", "oscilloscope")
                   : tim ? tr("Uhrzeit", "time") : "Display";
  logMsg(tr("[Web] Einstellungen (%s) auf Standardwerte zurueckgesetzt", "[Web] settings (%s) reset to defaults"), name);
  sendSettings(true, tr("Auf Standardwerte zurückgesetzt", "Reset to defaults"));
}

// POST /api/reconnect – Verbindung trennen; wifiTask verbindet sich dann mit
// dem stärksten bekannten Netz (z.B. nach dem Eintragen eines neuen Netzes)
static void handleReconnect()
{
  if (!wifiConfigured()) { sendJsonResult(false, tr("Noch kein WLAN eingetragen", "No network configured yet")); return; }
  logMsg(tr("[Web] Neu verbinden angefordert", "[Web] reconnect requested"));
  sendJsonResult(true, tr("Verbinde neu … die Seite ist kurz nicht erreichbar.", "Reconnecting … the page is briefly unavailable."));
  s_reconnectAtMs = millis() + 300;              // erst trennen, wenn die Antwort beim Browser ist
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

// Webserver starten: alle Adressen (Seite, Schrift, /api/…) registrieren
static void webStart()
{
  web.on("/",                HTTP_GET,  [] {               // Seite gzip-komprimiert (siehe tools/gen_webpage.py)
    web.sendHeader("Content-Encoding", "gzip");
    web.sendHeader("Cache-Control", "no-cache");             // nach einem Update immer die neue Seite
    web.send_P(200, "text/html; charset=utf-8", (const char *)INDEX_HTML_GZ, sizeof(INDEX_HTML_GZ));
  });
  // Schrift: liegt in der Firmware, damit die Seite auch ohne Internet (Setup-WLAN)
  // gleich aussieht. Ändert sich nur mit einer neuen Firmware → ein Jahr cachen.
  web.on("/font.woff2",      HTTP_GET,  [] {
    web.sendHeader("Cache-Control", "max-age=31536000");
    web.send_P(200, "font/woff2", (const char *)FONT_WOFF2, sizeof(FONT_WOFF2));
  });
  web.on("/font-600.woff2",  HTTP_GET,  [] {                // halbfetter Schnitt (Überschriften, Knöpfe)
    web.sendHeader("Cache-Control", "max-age=31536000");
    web.send_P(200, "font/woff2", (const char *)FONT_WOFF2_600, sizeof(FONT_WOFF2_600));
  });
  // „Zum Startbildschirm hinzufügen“: Manifest und Symbol (ändern sich nur mit der Firmware)
  web.on("/manifest.json",   HTTP_GET,  [] {
    web.sendHeader("Cache-Control", "max-age=86400");
    web.send_P(200, "application/manifest+json", MANIFEST_JSON);
  });
  web.on("/icon.svg",        HTTP_GET,  [] {
    web.sendHeader("Cache-Control", "max-age=31536000");
    web.send_P(200, "image/svg+xml", ICON_SVG);
  });
  web.on("/icon.png",        HTTP_GET,  [] {
    web.sendHeader("Cache-Control", "max-age=31536000");
    web.send_P(200, "image/png", (const char *)ICON_PNG, sizeof(ICON_PNG));
  });
  web.on("/api/status",      HTTP_GET,  handleStatus);
  web.on("/api/log",         HTTP_GET,  [] {               // Log direkt aus dem Puffer senden
    const char *a, *b; size_t alen, blen;
    logSnapshot(a, alen, b, blen);
    web.setContentLength(alen + blen);
    web.send(200, "text/plain; charset=utf-8", "");
    if (alen) web.sendContent(a, alen);
    if (blen) web.sendContent(b, blen);
  });
  web.on("/api/networks",    HTTP_GET,  handleNetworks);
  web.on("/api/vectors",     HTTP_GET,  handleVectors);
  web.on("/api/scan",        HTTP_GET,  handleScan);
  web.on("/api/wifi",        HTTP_POST, handleWifiSave);
  web.on("/api/wifi/delete", HTTP_POST, handleWifiDelete);
  web.on("/api/wifi/reset",  HTTP_POST, handleWifiReset);
  web.on("/api/reconnect",   HTTP_POST, handleReconnect);
  web.on("/api/dial",        HTTP_POST, handleDial);
  web.on("/api/time",        HTTP_POST, handleTimeSet);
  web.on("/api/settings",       HTTP_GET,  handleSettingsGet);
  web.on("/api/settings",       HTTP_POST, handleSettingsPost);
  web.on("/api/settings/reset", HTTP_POST, handleSettingsReset);
  web.on("/api/log/clear",   HTTP_POST, [] {
    logClear();
    logMsg("%s", tr("[Web] Protokoll geloescht", "[Web] log cleared"));
    sendJsonResult(true, tr("Protokoll gelöscht", "Log cleared"));
  });
  web.onNotFound(handleNotFound);
  web.begin();
  logMsg(tr("[Web] Webserver gestartet (Port 80)", "[Web] web server started (port 80)"));
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
    logMsg(tr("[Web] Setup-WLAN \"%s\" aktiv – Einrichtung unter http://%s", "[Web] setup network \"%s\" active – set up at http://%s"),
           WEB_SETUP_AP_SSID, WiFi.softAPIP().toString().c_str());
  } else if (s_apActive && conn && now - s_connSinceMs >= 30000UL) {
    dns.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    s_apActive = false;
    logMsg(tr("[Web] Setup-WLAN beendet (verbunden mit \"%s\")", "[Web] setup network closed (connected to \"%s\")"), WiFi.SSID().c_str());
  }
}

// Wird im uiTask alle 10 ms aufgerufen.
static void webLoop()
{
  if (s_apActive) dns.processNextRequest();
  web.handleClient();
  if (s_reconnectAtMs && (int32_t)(millis() - s_reconnectAtMs) >= 0) { s_reconnectAtMs = 0; WiFi.disconnect(false, false); }
  if (s_restartAtMs && (int32_t)(millis() - s_restartAtMs) >= 0) ESP.restart();
}

//*****************************************************************************
// OLED – Waveshare 2.42" SSD1309 128x64 über 4-Wire-SPI (Core 0)
//*****************************************************************************
// Hardware-SPI: CLK = GPIO18, DIN = GPIO23, CS/DC/RST aus config.h.
// "_F_" = voller Framebuffer (1 KB RAM): Das Bild wird komplett im RAM
// aufgebaut und mit sendBuffer() in einem Rutsch übertragen (ca. 1 ms bei
// 8 MHz) – kein Flackern.
// Falls das Bild um 2 Pixel verschoben erscheint: NONAME0 durch NONAME2 ersetzen.
static U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI u8g2(U8G2_R0, OLED_CS_PIN, OLED_DC_PIN, OLED_RST_PIN);

// ---- OLED-Helligkeit -----------------------------------------------------------
// Wirksame Helligkeit in Prozent (s_oledPct); während der BOOT-Taster gehalten wird
// mindestens 50 %, damit der Countdown auch bei ausgeschaltetem Display zu sehen ist.
// 0 % schaltet das Panel ab (Power-Save), sonst Kontrast 1…255.

// Helligkeit anwenden – nur bei Änderung (wird bei jedem Neuzeichnen aufgerufen)
static void oledApplyBrightness(bool overlay)
{
  static int last = -1;
  int pct = s_oledPct;
  if (overlay && pct < 50) pct = 50;
  if (pct == last) return;
  last = pct;
  if (pct <= 0) { u8g2.setPowerSave(1); return; }
  u8g2.setPowerSave(0);
  u8g2.setContrast((uint8_t)((pct * 255 + 50) / 100));
}

// Anzeige um 180° drehen (U8G2_R2) oder normal (U8G2_R0) – wirkt ab dem
// nächsten Neuzeichnen (spätestens nach OLED_REFRESH_MS).
static void oledSetFlip(bool flip) { u8g2.setDisplayRotation(flip ? U8G2_R2 : U8G2_R0); }

// Display initialisieren und kurz einen Startbildschirm zeigen.
// Wird im uiTask aufgerufen, damit die Bus-Interrupts auf Core 0 liegen.
static void oledInit()
{
  u8g2.setBusClock(OLED_SPI_HZ);
  u8g2.begin();
  oledApplyBrightness(true);              // Startbildschirm immer sichtbar
  oledSetFlip(s_oledFlip);
  u8g2.enableUTF8Print();

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14B_tf);
  u8g2.drawStr(10, 26, "Scope Clock");
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(10, 44, "Firmware v" FW_VERSION);
  u8g2.drawStr(10, 58, tr("Starte ...", "Starting ..."));
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
// 12-Stunden-Format (s_h12): Stunde 1–12 ohne führende Null, rechts daneben
// klein "AM"/"PM" (Font 5x8, oben bündig). Der Block bleibt zentriert.
static void drawSegTime(int y, int hh, int mm, int ss, int dx)
{
  const int W = 11, H = 20, T = 2;   // Ziffer 11x20 px, Segment 2 px
  const int GAP = 2;                 // Abstand zwischen zwei Ziffern eines Paares
  const int COLON = 2, CGAP = 3;     // Doppelpunkt 2x2 px, 3 px Abstand je Seite
  const int AMPM = s_h12 ? 3 + 10 : 0; // Abstand + "AM"/"PM" (2 Zeichen à 5 px)
  const int N = s_oledSec ? 3 : 2;     // ohne Sekunden nur HH:MM
  const int total = 2 * N * W + N * GAP + (N - 1) * (CGAP + COLON + CGAP) + AMPM;
  int x = (128 - 3 - total) / 2 + dx; // 3 px Reserve für den Pixel-Shift

  const bool pm = hh >= 12;
  if (s_h12) { hh %= 12; if (hh == 0) hh = 12; }
  const int v[3] = { hh, mm, ss };
  for (int i = 0; i < N; i++) {
    if (!(i == 0 && s_h12 && hh < 10))                       // 12h: keine führende Null
      drawSegDigit(x, y, v[i] / 10, W, H, T);
    x += W + GAP;                                            // Zehner
    drawSegDigit(x, y, v[i] % 10, W, H, T);  x += W;         // Einer
    if (i < N - 1) {                                         // Doppelpunkt
      x += CGAP;
      u8g2.drawBox(x, y + H / 3 - 1,     COLON, COLON);
      u8g2.drawBox(x, y + 2 * H / 3 - 1, COLON, COLON);
      x += COLON + CGAP;
    }
  }
  if (s_h12) {
    u8g2.setFont(u8g2_font_5x8_tf);
    u8g2.drawStr(x + 3, y + 7, pm ? "PM" : "AM");
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

// Zeichnet eine Textzeile horizontal zentriert, in der alle Ziffern als kleine
// 7-Segment-Ziffern erscheinen (5x9 px, Segment 1 px, offene Ecken wie bei der
// Uhrzeit). '.' und '-' werden passend dazu als Punkt bzw. Mittelsegment
// gezeichnet, alle anderen Zeichen im aktuellen Font (hier 6x10).
// Unterkante der Ziffern = Grundlinie des Fonts. Verwendet für KW/Datum.
static const int SEG_SW = 5, SEG_SH = 9;   // kleine Ziffer: Breite, Höhe

static int segTextWidth(const char *s)
{
  int w = 0;
  char c[2] = { 0, 0 };
  for (; *s; s++) {
    if ((*s >= '0' && *s <= '9') || *s == '-') w += SEG_SW + 1;   // Ziffer + 1 px Abstand
    else if (*s == '.')                        w += 2;            // Punkt 1 px + 1 px Abstand
    else { c[0] = *s; w += u8g2.getStrWidth(c); }
  }
  return w;
}

static void drawSegTextCentered(const char *s, int baseline, int dx)
{
  const int w = segTextWidth(s);
  int x = (128 - 3 - w) / 2 + dx;
  if (x + w > 128) x = 128 - w;
  if (x < 0) x = 0;
  const int top = baseline - SEG_SH + 1;
  char c[2] = { 0, 0 };
  for (; *s; s++) {
    if (*s >= '0' && *s <= '9') { drawSegDigit(x, top, *s - '0', SEG_SW, SEG_SH, 1); x += SEG_SW + 1; }
    else if (*s == '-')         { u8g2.drawBox(x + 1, top + (SEG_SH - 1) / 2, SEG_SW - 2, 1); x += SEG_SW + 1; }
    else if (*s == '.')         { u8g2.drawBox(x, baseline, 1, 1); x += 2; }
    else { c[0] = *s; u8g2.drawStr(x, baseline, c); x += u8g2.getStrWidth(c); }
  }
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
//             ~5 px Abstand
//   y 28…36   KW 46 - 15.11.2026     Ziffern 7-Segment 5x9 px, "KW" Font 6x10
//             ~6 px Abstand (trennt Zeit/Datum von WLAN)
//   y 43…49   WLAN Gastzugang        Font 5x8, max. 25 Zeichen
//   y 52…58   Signal -61 dBm         Font 5x8
//
// Ohne NTP-Sync steht statt des Datums "Keine NTP-Verbindung". Der NTP-Sync selbst
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
static bool     s_factoryDone  = false;  // Werkseinstellungen ausgelöst, Neustart folgt

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

// Werkseinstellungen: löscht ALLE gespeicherten Einstellungen und WLANs (NVS-Namensraum
// "scopeclock") und startet neu. Danach gelten wieder die Werte aus config.h, und ohne
// WLAN öffnet die Uhr das Setup-WLAN.
static void factoryReset()
{
  Preferences p;
  if (p.begin("scopeclock", false)) { p.clear(); p.end(); }
  WiFi.disconnect(false, true);              // auch die vom WLAN-Treiber gemerkten Zugangsdaten löschen
  logMsg("%s", tr("[Taster] Werkseinstellungen: alle Einstellungen und WLANs geloescht – Neustart",
                  "[Button] factory reset: all settings and networks erased – restarting"));
  s_factoryDone = true;
  s_restartAtMs = millis() + 2000;           // Meldung auf dem OLED noch kurz zeigen
}

// Wird alle 10 ms aus dem uiTask aufgerufen.
//   BUTTON_LONG_MS     halten → Oszilloskop-Profil umschalten (wie bisher)
//   BUTTON_FACTORY_MS  weiter halten → Werkseinstellungen (die letzten 5 s zeigt das OLED einen Countdown)
static void buttonPoll()
{
  const bool down = digitalRead(BOOT_BUTTON_PIN) == LOW;
  const uint32_t now = millis();
  if (s_factoryDone) return;
  if (!down) { s_btnDownSince = 0; s_btnHandled = false; return; }
  if (s_btnDownSince == 0) { s_btnDownSince = now ? now : 1; return; }
  if (!s_btnHandled && now - s_btnDownSince >= BUTTON_LONG_MS) {
    s_btnHandled = true;
    const uint8_t p = g_scopeProfile ? 0 : 1;
    g_scopeProfile = p;                       // gilt ab dem nächsten Sprung
    saveSetting("profile", p);
    logMsg(tr("[Scope] Profil umgeschaltet: %s, gespeichert", "[Scope] profile switched: %s, saved"), profileName(p));
  }
  if (now - s_btnDownSince >= BUTTON_FACTORY_MS) factoryReset();
}

// true, solange der Taster lange genug gehalten wird, dass die Warnung zu den
// Werkseinstellungen erscheint – oder sie bereits ausgelöst wurden.
static bool factoryPhase()
{
  return s_factoryDone || (s_btnDownSince && millis() - s_btnDownSince >= BUTTON_FACTORY_MS - 5000UL);
}

// Werkseinstellungen (BOOT-Taster weiter gehalten): Überschrift und die ablaufende
// Zeit als große 7-Segment-Ziffer (wie die Uhrzeit, 11x20 px) – 5, 4, 3, 2, 1, dann 0
// bis zum Neustart. Loslassen vorher bricht ab.
//
//      Werkseinstellungen      Font 7x14 fett (EN: "Factory reset")
//              5
static void oledFactoryOverlay()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14B_tf);
  drawCentered(tr("Werkseinstellungen", "Factory reset"), 16, 1);   // 18 Zeichen = 126 px
  const uint32_t held = millis() - s_btnDownSince;
  const uint32_t left = s_factoryDone || held >= BUTTON_FACTORY_MS ? 0 : (BUTTON_FACTORY_MS - held + 999) / 1000;   // aufgerundet
  const int CW = 11, CH = 20, CT = 2;
  int x0 = (128 - 3 - CW) / 2 + 1;
  if (left == 1) x0 -= (CW - CT) / 2;                    // "1" besteht nur aus den rechten Segmenten
  drawSegDigit(x0, 32, (int)left, CW, CH, CT);
  u8g2.sendBuffer();
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
  if (factoryPhase()) { oledFactoryOverlay(); return true; }
  const bool holding = s_btnDownSince && !s_btnHandled && now - s_btnDownSince >= 300;
  if (!holding) return false;

  const uint8_t  cur  = g_scopeProfile;
  const uint8_t  next = cur ? 0 : 1;
  const uint32_t left = (BUTTON_LONG_MS - (now - s_btnDownSince) + 999) / 1000;   // aufgerundet
  char buf[24];

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  drawCentered(tr("Oszilloskop-Profil", "Oscilloscope profile"), 11, 1);   // max. 20 Zeichen = 120 px
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

// OLED neu zeichnen: Uhrzeit, KW/Datum, WLAN-Name, Signalstärke (1× pro Sekunde)
static void oledUpdate()
{
  const bool overlay = (s_btnDownSince && !s_btnHandled && millis() - s_btnDownSince >= 300) || factoryPhase();
  oledApplyBrightness(overlay);
  if (oledProfileOverlay()) return;
  if (s_oledPct == 0) return;   // Display aus: nichts zeichnen

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
    if (s_lang) snprintf(buf, sizeof(buf), "Week %02d - %04d-%02d-%02d", isoWeek(t), t.tm_year + 1900,
                         t.tm_mon + 1, t.tm_mday);       // ca. 114 px (ISO-Datum)
    else        snprintf(buf, sizeof(buf), "KW %02d - %02d.%02d.%04d", isoWeek(t), t.tm_mday,
                         t.tm_mon + 1, t.tm_year + 1900);   // ca. 96 px
  else
    snprintf(buf, sizeof(buf), "%s", tr("Keine NTP-Verbindung", "No NTP connection"));   // ohne gültige Zeit (max. 21 Zeichen)
  if (valid) drawSegTextCentered(buf, Y_DATE + dy, dx);   // Ziffern als 7-Segment (5x9 px)
  else       drawCentered(buf, Y_DATE + dy, dx);

  u8g2.setFont(u8g2_font_5x8_tf);
  const bool connected = wifiConfigured() && WiFi.status() == WL_CONNECTED;

  // 3) Netzwerkname: verbundenes Netz / Setup-WLAN / Suche läuft
  if (connected)          snprintf(buf, sizeof(buf), "%s %s", tr("WLAN", "WiFi"), WiFi.SSID().c_str());
  else if (s_apActive)    snprintf(buf, sizeof(buf), "%s %s", tr("WLAN", "WiFi"), WEB_SETUP_AP_SSID);   // mit diesem WLAN verbinden
  else if (!wifiConfigured()) snprintf(buf, sizeof(buf), "%s", tr("WLAN aus", "WiFi off"));
  else                    snprintf(buf, sizeof(buf), "%s", tr("WLAN suche ...", "WiFi searching ..."));
  fitText(buf, LINE_CHARS);
  drawCentered(buf, Y_SSID + dy, dx);

  // 4) Signalstärke (RSSI in dBm; näher an 0 = besser, z.B. -50 sehr gut, -85 schwach)
  if (connected) {
    snprintf(buf, sizeof(buf), "Signal %d dBm", (int)WiFi.RSSI());
  } else if (s_apActive) {
    snprintf(buf, sizeof(buf), "http://%s", WiFi.softAPIP().toString().c_str());
  } else if (wifiConfigured()) {
    snprintf(buf, sizeof(buf), "%s", tr("Keine Verbindung", "Not connected"));
  } else {
    snprintf(buf, sizeof(buf), "%s", tr("Offline-Betrieb", "Offline mode"));
  }
  fitText(buf, LINE_CHARS);
  drawCentered(buf, Y_RSSI + dy, dx);

  u8g2.sendBuffer();                // Framebuffer per SPI ans Display
}

//*****************************************************************************
// uiTask – Core 0: Zeitübergabe, OLED, Konsolen-Log
//*****************************************************************************
// Einfacher Zeitplan ohne weitere Tasks:
//   alle  10 ms  publishTime()    → Sekundenzeiger springt max. 10 ms zu spät
//                buttonPoll()     → BOOT-Taster (Profil, Werkseinstellungen)
//                webLoop()        → Webinterface, DNS im Setup-WLAN
//   alle 200 ms  oledUpdate()     → Sekundenanzeige auf dem OLED flüssig
//   alle   1 s   Bildrate, CPU-Last Core 0
//                wifiLogChanges() → Statusmeldungen ins Log
//                webApManage()    → Setup-WLAN ein/aus
static void uiTask(void *)
{
  // WICHTIG: OLED (SPI) und WLAN werden HIER initialisiert, nicht in setup():
  // ihre Interrupts landen damit auf Core 0 und stören den Zeichenpfad nicht.
  logMsg(tr("[UI] laeuft auf Core %d", "[UI] running on core %d"), xPortGetCoreID());
  oledInit();
  if (s_rtcEnabled) rtcApply(true);         // Echtzeituhr: Zeit holen, bevor WLAN/NTP starten (I²C-Interrupts auf Core 0)
  wifiStart();
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);    // DevKitC hat zusätzlich einen externen Pull-up

  uint32_t lastOled = 0, lastLog = 0;
  for (;;) {
    publishTime();
    buttonPoll();
    if (s_bootTest && millis() >= (uint32_t)BOOT_TEST_S * 1000UL) {   // Start-Testbild beenden
      s_bootTest = false; g_testPattern = false;
      logMsg("%s", tr("[Scope] Start-Testbild beendet, Uhr laeuft", "[Scope] start-up test pattern finished, clock running"));
    }
    webLoop();

    const uint32_t now = millis();
    if (now - lastOled >= OLED_REFRESH_MS) {
      lastOled = now;
      oledUpdate();                       // ~1 ms SPI-Transfer, nur Core 0
    }
    if (now - lastLog >= 1000) {
      // Bildrate aus dem Bildzähler des scopeTask
      static uint32_t lastFrames = 0;
      const uint32_t f = g_frameCount;
      s_fps = (f - lastFrames) * 1000UL / (now - lastLog);
      // CPU-Last Core 0 aus der Leerlaufzeit der letzten Sekunde
      const uint32_t idle = s_idleUs;
      s_idleUs = 0;
      const uint32_t spanUs = (now - lastLog) * 1000UL;
      s_cpu0Load = idle >= spanUs ? 0 : (uint8_t)(100UL - idle * 100UL / spanUs);
      lastFrames = f;
      lastLog = now;
      wifiLogChanges();
      webApManage();
      // Ohne NTP: Systemuhr alle 10 min aus der genaueren RTC nachstellen
      static uint32_t lastRtc = 0;
      if (s_rtcEnabled && !g_ntpSynced && now - lastRtc >= 600000UL) { lastRtc = now; rtcToSystem(); }
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
  // Gespeicherte Einstellungen laden (Profil, Zifferblatt, Strahl, Zeit, OLED, Sprache) –
  // vor der ersten Ausgabe (wegen der Sprache) und vor dem Start der Tasks
  loadSettings();
  zblankApply(g_zblank, g_zblankHigh);      // Z-Ausgang wie gespeichert (aus = Pin hochohmig)
  if (BOOT_TEST_S > 0) logMsg(tr("[Scope] Testbild fuer %d s", "[Scope] test pattern for %d s"), (int)BOOT_TEST_S);
  logMsg(tr("ESP32 Oscilloscope Clock v" FW_VERSION " (Idee nach Mauro Pintus 2018)", "ESP32 Oscilloscope Clock v" FW_VERSION " (idea by Mauro Pintus 2018)"));
  logMsg("CPU: %lu MHz", (unsigned long)getCpuFrequencyMhz());

  // Bis zum ersten publishTime() die Startzeit anzeigen
  g_timePacked = packTime(FALLBACK_H, FALLBACK_M, FALLBACK_S);

  logMsg(tr("[Scope] Profil: %s – BOOT-Taster %u s halten zum Umschalten", "[Scope] profile: %s – hold BOOT button %u s to switch"),
                profileName(g_scopeProfile), (unsigned)(BUTTON_LONG_MS / 1000));

  //                      Funktion   Name     Stack  Param    Prio Handle           Kern
  logMsg(tr("Letzter Neustart: %s", "Last restart: %s"), resetReasonText());
  esp_register_freertos_idle_hook_for_cpu(idleHookCore0, 0);   // CPU-Last-Messung Core 0

  xTaskCreatePinnedToCore(uiTask,    "ui",    8192, nullptr, 1,   &uiTaskHandle,    UI_CORE);
  xTaskCreatePinnedToCore(scopeTask, "scope", 4096, nullptr, 1,   &scopeTaskHandle, SCOPE_CORE);
}

// Die Arduino-loop() wird nicht gebraucht: Core 1 gehört dem scopeTask.
// Der loopTask legt sich beim ersten Durchlauf dauerhaft schlafen.
void loop()
{
  vTaskSuspend(nullptr);
}
