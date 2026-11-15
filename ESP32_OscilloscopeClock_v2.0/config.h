/******************************************************************************
  config.h – Hardware- und Laufzeit-Konfiguration
  ESP32 Oscilloscope Clock v2.0

  Alle Einstellungen, die man typischerweise anpasst, stehen hier.
  WLAN-Zugangsdaten gehören NICHT hierher, sondern in secrets.h.
  Nach jeder Änderung neu kompilieren und hochladen.
******************************************************************************/
#pragma once

// ---------------------------------------------------------------------------
// Zeitzone / NTP
// ---------------------------------------------------------------------------
// POSIX-TZ-String: Mitteleuropa inkl. automatischer Sommerzeit-Umstellung.
//   CET-1          Normalzeit = UTC+1 (das Vorzeichen ist bei POSIX umgekehrt)
//   CEST           Sommerzeit (automatisch UTC+2)
//   M3.5.0         Beginn: März, letzte Woche, Sonntag (02:00)
//   M10.5.0/3      Ende:   Oktober, letzte Woche, Sonntag, 03:00
// Andere Zonen: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
#define TZ_INFO        "CET-1CEST,M3.5.0,M10.5.0/3"
// Bis zu drei NTP-Server; der Client nimmt den ersten erreichbaren.
#define NTP_SERVER_1   "de.pool.ntp.org"
#define NTP_SERVER_2   "pool.ntp.org"
#define NTP_SERVER_3   "time.nist.gov"
#define NTP_SYNC_INTERVAL_MS (60UL * 60UL * 1000UL)  // Nachsynchronisieren: 1x pro Stunde

// Startzeit, solange (noch) kein NTP-Sync erfolgt ist (wie im Original).
// Gilt auch dauerhaft, wenn in secrets.h kein WLAN eingetragen ist.
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

// 1 = direkte Registerzugriffe (Standard)
// 0 = Arduino dacWrite() – nur als Notlösung, falls die Registerzugriffe mit
//     einer künftigen Core-Version nicht funktionieren. dacWrite() ist langsamer
//     als ein Schritt (s.u.), das Bild wird dann dunkler und flimmert eher.
#define USE_FAST_DAC 1

// ---- Strahlgeschwindigkeit (gleichmäßige Helligkeit) -----------------------
// Der Strahl bewegt sich auf ALLEN Linien gleich schnell: 1 DAC-Einheit
// (≈ 12,5 mV, 1/255 des Bildes) pro BEAM_NS_PER_UNIT Nanosekunden.
// Ein Bild hat ca. 1.500–1.700 Schritte (römisches Zifferblatt ca. 2.300).
//
//   BEAM_NS_PER_UNIT   Bildfrequenz          Hinweis
//                      (1–12 / römisch)
//        2000          ~310 Hz / ~220 Hz     moderne/schnelle Oszis
//        3000          ~210 Hz / ~150 Hz     Standard, passt für fast alle Röhren
//        5000          ~125 Hz /  ~90 Hz     sehr alte/langsame Röhren, hellere Spur
//        8000           ~75 Hz /  ~55 Hz     Untergrenze – darunter flimmert es
//
// Größer = langsamer = hellere Linien, aber niedrigere Bildfrequenz.
#define BEAM_NS_PER_UNIT 3000

// Sprünge zwischen Linienzügen (der ESP32 kann den Strahl nicht dunkeltasten)
// werden so viel schneller gefahren wie geschrieben. Die Rücklaufspur ist um
// diesen Faktor schwächer (8 → 1/8 der Linienhelligkeit).
//   8      Standard: schwache, gleichmäßige Rücklaufspur, für alle Röhren sicher
//   16     schwächere Rücklaufspur, für Oszis mit ≥ 1 MHz X/Y-Bandbreite
//   4      sehr alte/langsame Oszis, wenn Linienanfänge "Haken" zeigen
//   0      harter Sprung wie im Original: auf schnellen Oszis praktisch
//          unsichtbar, bei langsamen evtl. Über-/Nachschwingen
#define JUMP_SPEED 8

// Stunden- und Minutenzeiger sind schmale Rauten: Ihre beiden Kanten liegen
// nur 0–4 DAC-Einheiten auseinander und verschmelzen auf dem Schirm zu einer
// Linie, die bei voller Helligkeit deutlich heller wäre als der Rest.
// Mit 67 % wirken sie in der Simulation so hell wie Zifferblatt und
// Sekundenzeiger (Abweichung < 15 %). 100 = Zeiger heller hervorheben.
#define HAND_BRIGHT_PCT 67

// Kernzuordnung. Nicht ändern: Der WLAN-Stack des ESP32 läuft auf Core 0,
// die Strahlausgabe braucht Core 1 exklusiv.
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

#define OLED_REFRESH_MS   200        // Displayaktualisierung in ms (Sekunden laufen flüssig)
#define OLED_SHIFT_EVERY_S 60        // Pixel-Shift gegen Einbrennen: alle n Sekunden (0 = aus)
#define OLED_CONTRAST     0x80       // Helligkeit 0x00…0xFF (niedriger = langlebiger)

// ---------------------------------------------------------------------------
// Diverses
// ---------------------------------------------------------------------------
#define WIFI_HOSTNAME "scope-clock"   // Gerätename im Router
#define SERIAL_BAUD   115200          // serielle Konsole (Arduino: Serieller Monitor)

// Zifferblatt-Variante auf dem Oszilloskop:
//   0 = Ziffern 1–12
//   1 = Ziffern 13–24 (an den Positionen 1–12; die Zeiger laufen weiter im 12h-Takt)
//   2 = römische Ziffern I–XII
#define DIAL_STYLE 0

// EXCEL-Modus aus dem Original (Diagnose ohne Oszilloskop): zeichnet genau ein
// Bild, gibt dabei jeden Punkt als "0xXX,0xYY," seriell aus und hält dann an.
// Die Liste lässt sich in Excel/LibreOffice als XY-Diagramm darstellen.
// Zum Aktivieren die // entfernen.
//#define EXCEL
