/******************************************************************************

  ESP32 Oscilloscope Clock v2.0
  X/Y-Vektoruhr über die internen DACs, NTP-Zeit, Status-OLED

  Basiert auf: ESP32 Oscilloscope Clock v1.0
               Mauro Pintus, Milano 2018/05/25
               https://github.com/maurohh/ESP32_OscilloscopeClock
               (Linien-/Tabellenlogik und DataTable.h unverändert übernommen)

  Neu in v2.0:
   - Zielhardware: ESP32-WROOM-32U DevKitC V4 (externe Antenne über IPEX/U.FL)
   - Feste Aufgabenteilung auf die beiden Kerne (FreeRTOS):
       Core 1: ScopeTask – erzeugt ununterbrochen die X/Y-Signale an GPIO25/26
       Core 0: UiTask    – WLAN, NTP (SNTP des ESP-IDF), OLED über I2C
     Zwischen den Kernen wird nur ein einziges 32-Bit-Wort (die aktuelle Uhrzeit)
     ausgetauscht; der Zeichenpfad auf Core 1 blockiert dadurch nie.
   - NTP über die eingebaute SNTP-Funktion statt NTPtimeESP-Bibliothek,
     inkl. Zeitzone und automatischer Sommer-/Winterzeit (TZ-String in config.h)
   - Schnelle DAC-Ausgabe über direkte Registerzugriffe (weniger Flimmern)
   - Waveshare 2.42" OLED (SSD1309, 128x64) im I2C-Modus mit U8g2:
     Digitaluhr, Datum, WLAN-/NTP-Status, Pixel-Shift gegen Einbrennen
   - Läuft ohne WLAN mit der Startzeit aus config.h weiter (wie das Original)

  Benötigte Bibliothek (Arduino-Bibliotheksverwalter):  U8g2 by oliver
  Board-Paket: "esp32" by Espressif (2.0.x oder 3.x), Board "ESP32 Dev Module"

  Verdrahtung und Aufbau: siehe HARDWARE.md im Repository.

******************************************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <time.h>
#include <esp_sntp.h>
#include <soc/soc.h>
#include <soc/rtc_io_reg.h>
#include <U8g2lib.h>

#include "config.h"
#if __has_include("secrets.h")
  #include "secrets.h"
#else
  #include "secrets_example.h"
#endif
#include "DataTable.h"

#if !defined(CONFIG_IDF_TARGET_ESP32)
  #error "Dieses Projekt benötigt einen klassischen ESP32 (mit DAC an GPIO25/26). Board: 'ESP32 Dev Module'."
#endif

#define FW_VERSION "2.0"

//*****************************************************************************
// Gemeinsamer Zustand zwischen den Kernen
//*****************************************************************************
// Uhrzeit als ein 32-Bit-Wort: [23..16]=Stunde (0-23) [15..8]=Minute [7..0]=Sekunde.
// Ausgerichtete 32-Bit-Zugriffe sind auf dem ESP32 atomar -> kein Mutex nötig,
// der Zeichenpfad auf Core 1 muss nie warten.
static volatile uint32_t g_timePacked = 0;

static inline uint32_t packTime(int h, int m, int s) {
  return ((uint32_t)h << 16) | ((uint32_t)m << 8) | (uint32_t)s;
}

static volatile bool     g_ntpSynced    = false;
static volatile uint32_t g_lastSyncMs   = 0;

static TaskHandle_t scopeTaskHandle = nullptr;
static TaskHandle_t uiTaskHandle    = nullptr;

//*****************************************************************************
// DAC-Ausgabe
//*****************************************************************************
static int lastx = -1, lasty = -1;

static inline void IRAM_ATTR dacOutX(uint8_t v) {
#if USE_FAST_DAC
  SET_PERI_REG_BITS(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_DAC, v, RTC_IO_PDAC1_DAC_S);
#else
  dacWrite(DAC_X_PIN, v);
#endif
}

static inline void IRAM_ATTR dacOutY(uint8_t v) {
#if USE_FAST_DAC
  SET_PERI_REG_BITS(RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_DAC, v, RTC_IO_PDAC2_DAC_S);
#else
  dacWrite(DAC_Y_PIN, v);
#endif
}

static void dacInit() {
  // dacWrite() schaltet die DAC-Pads ein und konfiguriert sie (ESP-IDF 4 und 5).
  // Danach schreiben wir den Ausgangswert direkt ins Register.
  dacWrite(DAC_X_PIN, 0x80);
  dacWrite(DAC_Y_PIN, 0x80);
}

//*****************************************************************************
// Dot
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
  uint32_t t0 = ESP.getCycleCount();
  while ((uint32_t)(ESP.getCycleCount() - t0) < (uint32_t)DOT_DELAY_CYCLES) { }
#endif
#if defined EXCEL
  Serial.printf("0x%02X,0x%02X,\n", x & 0xFF, y & 0xFF);
#endif
}

//*****************************************************************************
// Line – Bresenham / DDA, je Oktant eine Schleife (aus v1.0 übernommen)
//*****************************************************************************
static void IRAM_ATTR Line(byte x1, byte y1, byte x2, byte y2)
{
    int acc;
    if (y1 < y2) { // quadrant 1 or 2
        byte dy = y2 - y1;
        if (x1 < x2) { // quadrant 1
            byte dx = x2 - x1;
            if (dx > dy) { // < 45
                acc = (dx >> 1);
                for (; x1 <= x2; x1++) {
                    Dot(x1, y1);
                    acc -= dy;
                    if (acc < 0) { y1++; acc += dx; }
                }
            }
            else {   // > 45
                acc = dy >> 1;
                for (; y1 <= y2; y1++) {
                    Dot(x1, y1);
                    acc -= dx;
                    if (acc < 0) { x1++; acc += dy; }
                }
            }
        }
        else {  // quadrant 2
            byte dx = x1 - x2;
            if (dx > dy) { // < 45
                acc = dx >> 1;
                for (; x1 >= x2; x1--) {
                    Dot(x1, y1);
                    acc -= dy;
                    if (acc < 0) { y1++; acc += dx; }
                }
            }
            else {  // > 45
                acc = dy >> 1;
                for (; y1 <= y2; y1++) {
                    Dot(x1, y1);
                    acc -= dx;
                    if (acc < 0) { x1--; acc += dy; }
                }
            }
        }
    }
    else { // quadrant 3 or 4
        byte dy = y1 - y2;
        if (x1 < x2) { // quadrant 4
            byte dx = x2 - x1;
            if (dx > dy) {  // < 45
                acc = dx >> 1;
                for (; x1 <= x2; x1++) {
                    Dot(x1, y1);
                    acc -= dy;
                    if (acc < 0) { y1--; acc += dx; }
                }
            }
            else {  // > 45
                acc = dy >> 1;
                for (; y1 >= y2; y1--) {
                    Dot(x1, y1);
                    acc -= dx;
                    if (acc < 0) { x1++; acc += dy; }
                }
            }
        }
        else {  // quadrant 3
            byte dx = x1 - x2;
            if (dx > dy) { // < 45
                acc = dx >> 1;
                for (; x1 >= x2; x1--) {
                    Dot(x1, y1);
                    acc -= dy;
                    if (acc < 0) { y1--; acc += dx; }
                }
            }
            else {  // > 45
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
// PlotTable (aus v1.0 übernommen)
//*****************************************************************************
static void IRAM_ATTR PlotTable(byte *SubTable, int SubTableSize, int skip, int opt, int offset)
{
  int i = offset;
  while (i < SubTableSize) {
    if (SubTable[i+2] == skip) {
      i = i + 3;
      if (opt == 1) if (SubTable[i] == skip) i++;
    }
    Line(SubTable[i], SubTable[i+1], SubTable[i+2], SubTable[i+3]);
    if (opt == 2) {
      Line(SubTable[i+2], SubTable[i+3], SubTable[i], SubTable[i+1]);
    }
    i = i + 2;
    if (SubTable[i+2] == 0xFF) break;
  }
}

//*****************************************************************************
// ScopeTask – Core 1: zeichnet endlos das Zifferblatt + Zeiger
//*****************************************************************************
static void scopeTask(void *)
{
  // Dieser Task gibt Core 1 nie frei. Falls der Idle-Watchdog für Core 1 in der
  // verwendeten Core-Version aktiv ist, wird er hier sicherheitshalber abgeschaltet.
  disableCore1WDT();
  dacInit();
  Serial.printf("[Scope] laeuft auf Core %d\n", xPortGetCoreID());

  for (;;) {
    const uint32_t t = g_timePacked;          // ein atomarer Lesezugriff pro Bild
    const int h = (t >> 16) & 0xFF;
    const int m = (t >> 8)  & 0xFF;
    const int s =  t        & 0xFF;
    const int hpos = (h % 12) * 5 + m / 12;   // Stundenzeiger in 60 Schritten

    PlotTable(DialData, sizeof(DialData), 0x00, 1, 0);
#if DIAL_STYLE == 1
    PlotTable(DialDigits24, sizeof(DialDigits24), 0x00, 1, 0);
#elif DIAL_STYLE == 2
    PlotTable(DialDigitsRoman, sizeof(DialDigitsRoman), 0x00, 1, 0);
#else
    PlotTable(DialDigits12, sizeof(DialDigits12), 0x00, 1, 0);
#endif
    PlotTable(HrPtrData,  sizeof(HrPtrData),  0xFF, 0, 9 * hpos);
    PlotTable(MinPtrData, sizeof(MinPtrData), 0xFF, 0, 9 * m);
    PlotTable(SecPtrData, sizeof(SecPtrData), 0xFF, 0, 5 * s);

#if defined EXCEL
    Serial.println("EXCEL-Ausgabe beendet.");
    vTaskSuspend(nullptr);
#endif
    // Bewusst kein vTaskDelay(): jede Pause wäre als heller Punkt sichtbar.
  }
}

//*****************************************************************************
// Zeit
//*****************************************************************************
static void onNtpSync(struct timeval *)
{
  g_ntpSynced  = true;
  g_lastSyncMs = millis();
}

// Liefert die aktuelle lokale Zeit; ohne NTP ab FALLBACK_* hochgezählt.
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
  out.tm_wday = -1;                         // = Datum unbekannt
  return false;
}

static void publishTime()
{
  struct tm t;
  currentTime(t);
  g_timePacked = packTime(t.tm_hour, t.tm_min, t.tm_sec);
}

//*****************************************************************************
// WLAN
//*****************************************************************************
static bool wifiConfigured() { return strlen(WIFI_SSID) > 0; }

static void wifiStart()
{
  if (!wifiConfigured()) {
    Serial.println("[WiFi] keine Zugangsdaten (secrets.h) – Offline-Betrieb");
    WiFi.mode(WIFI_OFF);
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(WIFI_HOSTNAME);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("[WiFi] verbinde mit \"%s\" ...\n", WIFI_SSID);

  // SNTP: läuft im Hintergrund, synchronisiert periodisch nach.
  sntp_set_time_sync_notification_cb(onNtpSync);
  sntp_set_sync_interval(NTP_SYNC_INTERVAL_MS);
  configTzTime(TZ_INFO, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);
}

static void wifiLogChanges()
{
  static wl_status_t last = WL_IDLE_STATUS;
  static bool lastSynced = false;
  wl_status_t st = WiFi.status();
  if (st != last) {
    if (st == WL_CONNECTED)
      Serial.printf("[WiFi] verbunden, IP %s, RSSI %d dBm\n",
                    WiFi.localIP().toString().c_str(), WiFi.RSSI());
    else if (last == WL_CONNECTED)
      Serial.println("[WiFi] Verbindung verloren – versuche erneut");
    last = st;
  }
  if (g_ntpSynced && !lastSynced) {
    struct tm t; currentTime(t);
    Serial.printf("[NTP] synchronisiert: %02d.%02d.%04d %02d:%02d:%02d\n",
                  t.tm_mday, t.tm_mon + 1, t.tm_year + 1900, t.tm_hour, t.tm_min, t.tm_sec);
    lastSynced = true;
  }
}

//*****************************************************************************
// OLED – Waveshare 2.42" SSD1309 128x64, I2C
//*****************************************************************************
// Falls das Bild um 2 Pixel verschoben erscheint: NONAME0 durch NONAME2 ersetzen.
static U8G2_SSD1309_128X64_NONAME0_F_HW_I2C u8g2(U8G2_R0, OLED_RST_PIN, OLED_SCL_PIN, OLED_SDA_PIN);

static void oledInit()
{
  u8g2.setI2CAddress(OLED_I2C_ADDR << 1);
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

static void drawWifiBars(int x, int y, int rssi)
{
  // 4 Balken, unten bündig bei y
  int bars = 0;
  if (rssi > -55) bars = 4;
  else if (rssi > -65) bars = 3;
  else if (rssi > -75) bars = 2;
  else if (rssi > -85) bars = 1;
  for (int i = 0; i < 4; i++) {
    int hgt = 2 + i * 2;
    if (i < bars) u8g2.drawBox(x + i * 3, y - hgt, 2, hgt);
    else          u8g2.drawPixel(x + i * 3, y - 1);
  }
}

static void oledUpdate()
{
  static const char *WD[] = { "So", "Mo", "Di", "Mi", "Do", "Fr", "Sa" };
  struct tm t;
  const bool valid = currentTime(t);

  // Pixel-Shift: alle OLED_SHIFT_EVERY_S Sekunden um 0..3 px versetzen
  int dx = 0, dy = 0;
#if OLED_SHIFT_EVERY_S > 0
  const uint32_t step = (millis() / 1000UL / OLED_SHIFT_EVERY_S) % 4;
  dx = step;  dy = (step + 1) % 2;
#endif

  char buf[24];
  u8g2.clearBuffer();

  // Kopfzeile: Datum + WLAN
  u8g2.setFont(u8g2_font_6x10_tf);
  if (valid) {
    snprintf(buf, sizeof(buf), "%s %02d.%02d.%04d", WD[t.tm_wday], t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);
  } else {
    snprintf(buf, sizeof(buf), "Datum unbekannt");
  }
  u8g2.drawStr(dx, 9 + dy, buf);

  const bool connected = wifiConfigured() && WiFi.status() == WL_CONNECTED;
  if (connected) drawWifiBars(114 + dx - 3, 9 + dy, WiFi.RSSI());
  else if (wifiConfigured()) u8g2.drawStr(116 + dx - 3, 9 + dy, "--");

  // Große Uhrzeit, zentriert
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
  u8g2.setFont(u8g2_font_logisoso22_tn);
  int w = u8g2.getStrWidth(buf);
  u8g2.drawStr((128 - 3 - w) / 2 + dx, 42 + dy, buf);

  // Statuszeile
  u8g2.setFont(u8g2_font_5x8_tf);
  if (!wifiConfigured()) {
    snprintf(buf, sizeof(buf), "Offline (kein WLAN)");
  } else if (!connected) {
    snprintf(buf, sizeof(buf), g_ntpSynced ? "WLAN weg, Zeit laeuft" : "Verbinde WLAN ...");
  } else if (!g_ntpSynced) {
    snprintf(buf, sizeof(buf), "Warte auf NTP ...");
  } else {
    uint32_t ago = (millis() - g_lastSyncMs) / 60000UL;
    snprintf(buf, sizeof(buf), "NTP ok, vor %lu min", (unsigned long)ago);
  }
  u8g2.drawStr(dx, 62 + dy - 1, buf);

  u8g2.sendBuffer();
}

//*****************************************************************************
// UiTask – Core 0: WLAN, NTP, OLED, Zeitübergabe an Core 1
//*****************************************************************************
static void uiTask(void *)
{
  // WICHTIG: WLAN und I2C werden HIER initialisiert, damit ihre Interrupts
  // auf Core 0 landen und den Zeichenpfad auf Core 1 nicht unterbrechen.
  Serial.printf("[UI] laeuft auf Core %d\n", xPortGetCoreID());
  oledInit();
  wifiStart();

  uint32_t lastOled = 0, lastLog = 0;
  for (;;) {
    publishTime();                        // alle 10 ms -> Sekundenzeiger springt pünktlich

    const uint32_t now = millis();
    if (now - lastOled >= OLED_REFRESH_MS) {
      lastOled = now;
      oledUpdate();                       // ~25 ms I2C-Transfer, blockiert nur Core 0
    }
    if (now - lastLog >= 1000) {
      lastLog = now;
      wifiLogChanges();
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

//*****************************************************************************
// setup / loop
//*****************************************************************************
void setup()
{
  Serial.begin(SERIAL_BAUD);
  delay(200);
  Serial.println("\nESP32 Oscilloscope Clock v" FW_VERSION);
  Serial.println("basierend auf v1.0 von Mauro Pintus 2018");
  Serial.printf("CPU: %lu MHz\n", (unsigned long)getCpuFrequencyMhz());

  g_timePacked = packTime(FALLBACK_H, FALLBACK_M, FALLBACK_S);

  xTaskCreatePinnedToCore(uiTask,    "ui",    8192, nullptr, 1, &uiTaskHandle,    UI_CORE);
  xTaskCreatePinnedToCore(scopeTask, "scope", 4096, nullptr, 1, &scopeTaskHandle, SCOPE_CORE);
}

void loop()
{
  // Die Arduino-loop wird nicht gebraucht; Core 1 gehört dem ScopeTask.
  vTaskSuspend(nullptr);
}
