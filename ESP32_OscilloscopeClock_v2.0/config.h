/******************************************************************************
  config.h – Hardware- und Laufzeit-Konfiguration
  ESP32 Oscilloscope Clock v2.0
******************************************************************************/
#pragma once

// ---------------------------------------------------------------------------
// Zeitzone / NTP
// ---------------------------------------------------------------------------
// POSIX-TZ-String: Mitteleuropa inkl. automatischer Sommerzeit-Umstellung.
// Andere Zonen: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
#define TZ_INFO        "CET-1CEST,M3.5.0,M10.5.0/3"
#define NTP_SERVER_1   "de.pool.ntp.org"
#define NTP_SERVER_2   "pool.ntp.org"
#define NTP_SERVER_3   "time.nist.gov"
#define NTP_SYNC_INTERVAL_MS (60UL * 60UL * 1000UL)  // Nachsynchronisieren: 1x pro Stunde

// Startzeit, solange (noch) kein NTP-Sync erfolgt ist (wie im Original).
#define FALLBACK_H  10
#define FALLBACK_M   8
#define FALLBACK_S  37

// ---------------------------------------------------------------------------
// DAC-Ausgänge (fest durch den ESP32-Chip vorgegeben, nicht änderbar)
//   DAC1 = GPIO25 -> BNC "X" (Kanal 1 des Oszilloskops)
//   DAC2 = GPIO26 -> BNC "Y" (Kanal 2 des Oszilloskops)
// ---------------------------------------------------------------------------
#define DAC_X_PIN 25
#define DAC_Y_PIN 26

// 1 = direkte Registerzugriffe (sehr schnell, mehr Bilder pro Sekunde, weniger Flimmern)
// 0 = Arduino dacWrite() (langsamer, wie das Original – zum Vergleich/Debuggen)
#define USE_FAST_DAC 1

// Zusätzliche Wartezeit pro Bildpunkt in CPU-Takten (240 MHz -> 240 Takte = 1 µs).
// Erhöhen, falls Linien "verwaschen" oder Ziffern verzerrt aussehen (DAC-Einschwingzeit
// + Kabelkapazität). Verringern, falls das Bild flimmert. Startwert 0, typ. 0…200.
#define DOT_DELAY_CYCLES 0

// Strahlausgabe auf Core 1 pinnen (Core 0 = WLAN, NTP, OLED).
#define SCOPE_CORE 1
#define UI_CORE    0

// ---------------------------------------------------------------------------
// OLED: Waveshare 2.42" (SSD1309, 128x64) im I2C-Modus
// ---------------------------------------------------------------------------
#define OLED_SDA_PIN 21
#define OLED_SCL_PIN 22
#define OLED_RST_PIN  4      // RES des Moduls; auf U8X8_PIN_NONE setzen, falls fest auf 3V3
#define OLED_I2C_HZ  400000  // 400 kHz Fast-Mode
#define OLED_I2C_ADDR 0x3C   // DC-Pin auf GND = 0x3C, DC auf 3V3 = 0x3D

#define OLED_REFRESH_MS   200        // Displayaktualisierung
#define OLED_SHIFT_EVERY_S 60        // Pixel-Shift gegen Einbrennen (0 = aus)
#define OLED_CONTRAST     0x80       // 0x00…0xFF

// ---------------------------------------------------------------------------
// Diverses
// ---------------------------------------------------------------------------
#define WIFI_HOSTNAME "scope-clock"
#define SERIAL_BAUD   115200

// Zifferblatt-Variante auf dem Oszilloskop: 0 = 12 arabisch, 1 = 24h, 2 = römisch
#define DIAL_STYLE 0

// EXCEL-Modus aus dem Original: gibt alle Koordinaten einmal seriell aus und hält an.
//#define EXCEL
