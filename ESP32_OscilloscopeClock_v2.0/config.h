/******************************************************************************
  config.h – Hardware- und Laufzeit-Konfiguration
  ESP32 Oscilloscope Clock v2.0

  Alle Einstellungen, die man typischerweise anpasst, stehen hier.
  Profil, Zifferblatt, Strahlgeschwindigkeit, Zeiger-Helligkeit, Rücklauf-
  Tempo, Zeitzone, erster NTP-Server und OLED-Helligkeit/-Pixel-Shift/-Drehung lassen
  sich zusätzlich im Webinterface ändern – die Werte hier sind dann nur die
  Startwerte (bzw. das Ziel von "Auf Standard zurücksetzen").
  WLAN-Zugangsdaten gehören NICHT hierher – sie werden im Webinterface eingetragen.
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

// ---- Optional: Echtzeituhr DS3231 (z.B. Modul ZS-042) über I²C ------------------
// Hält Datum und Uhrzeit mit Knopfzelle auch ohne Strom und ohne Internet.
// Anschluss: VCC → 3V3, GND → GND, SDA → RTC_SDA_PIN, SCL → RTC_SCL_PIN.
// Ein-/ausschalten im Webinterface (Einstellungen → Uhrzeit → "Echtzeituhr (RTC)").
// Knopfzelle: normale CR2032. Die Ladeschaltung des ZS-042 (Diode + 200 Ω) ist für
// 5 V gedacht; an 3V3 kommen nach der Diode nur ca. 2,7 V an – unter der Zellspannung,
// es fließt praktisch kein Ladestrom. Daher Modul NUR an 3V3 anschließen (an 5 V würde
// eine CR2032 geladen – dann die Diode auslöten). Eine LIR2032 würde an 3V3 nie geladen.
#define RTC_ENABLE   0          // Startwert: 0 = aus (Standard), 1 = an
#define RTC_SDA_PIN 21
#define RTC_SCL_PIN 22

// Startzeit, solange (noch) kein NTP-Sync erfolgt ist (wie im Original).
// Gilt auch dauerhaft ohne WLAN, solange die Uhrzeit nicht von Hand gestellt wird.
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

// ---- Oszilloskop-Typ ---------------------------------------------------------
//   0 = analoges Röhren-Oszilloskop (Strahl zeichnet direkt, z.B. Hameg HM203, Tek 2213)
//   1 = digitales Oszilloskop / Speicher-Oszilloskop (tastet ab und zeigt Punkte,
//       z.B. Tektronix TDS3012, HP 54200A, Digilent Analog Discovery 2)
// Wirkt sich auf die Sprünge zwischen Linienzügen aus: Digitale Oszilloskope zeigen
// jeden Abtastwert eines gefahrenen Sprungs als Punkt – dort wird hart
// gesprungen; analog wird gebremst gesprungen (JUMP_SPEED usw., s.u.).
//
// UMSCHALTEN OHNE NEU ZU KOMPILIEREN: BOOT-Taster am ESP32 im Betrieb
// BUTTON_LONG_MS lang halten. Das OLED zeigt dabei den Wechsel (z.B.
// "DIGITAL > ANALOG") mit Countdown 3, 2, 1; vorher loslassen bricht ab.
// Das Profil wird im Flash gespeichert und gilt auch nach einem Neustart.
// SCOPE_PROFILE ist nur der Startwert, solange noch nichts gespeichert ist.
// (Nicht beim Einschalten drücken – dann startet der Download-Modus.)
#define SCOPE_PROFILE 1
#define BOOT_BUTTON_PIN 0        // BOOT-Taster des DevKitC (gedrückt = LOW)
#define BUTTON_LONG_MS  3000     // so lange halten zum Umschalten (Countdown 3 s)
// Weiter halten bis BUTTON_FACTORY_MS → Werkseinstellungen: ALLE Einstellungen und
// gespeicherten WLANs werden gelöscht, die Uhr startet neu (danach Setup-WLAN).
// Die letzten 5 s (also ab 5 s Halten) zählt das OLED 5 … 1 herunter; Loslassen bricht ab.
#define BUTTON_FACTORY_MS 10000
static_assert(BUTTON_FACTORY_MS >= BUTTON_LONG_MS + 5000, "config.h: BUTTON_FACTORY_MS mind. 5 s länger als BUTTON_LONG_MS");

// ---- Strahlgeschwindigkeit (gleichmäßige Helligkeit) -----------------------
// Der Strahl bewegt sich auf ALLEN Linien gleich schnell: 1 DAC-Einheit
// (≈ 12,5 mV, 1/255 des Bildes) pro BEAM_NS_PER_UNIT Nanosekunden.
// Ein Bild dauert ca. 1.650 Schrittzeiten (römisches Zifferblatt ca. 1.900).
//
//   BEAM_NS_PER_UNIT   Bildfrequenz          Hinweis
//                      (1–12 / römisch)
//        2000          ~305 Hz / ~265 Hz     moderne/schnelle Oszilloskope
//        3000          ~205 Hz / ~180 Hz     Standard, passt für fast alle Röhren
//        5000          ~125 Hz / ~105 Hz     sehr alte/langsame Röhren, hellere Spur
//        8000           ~80 Hz /  ~80 Hz     beide vom Flimmerschutz begrenzt (FLICKER_MIN_FPS)
//
// Größer = langsamer = hellere Linien, aber niedrigere Bildfrequenz.
#define BEAM_NS_PER_UNIT 3000

// Flimmerschutz: Die Firmware begrenzt die Verweilzeit automatisch so, dass je
// nach Zifferblatt, Helligkeiten und Rücklauf mindestens so viele Bilder pro
// Sekunde gezeichnet werden (Röhren wirken ab etwa 60–70 ruhig). Ist im
// Webinterface eine längere Verweilzeit eingestellt, gilt der Grenzwert; die
// Einstellung selbst bleibt gespeichert. 0 = Flimmerschutz aus.
#define FLICKER_MIN_FPS 75

// ---- Rücklauf: Sprünge zwischen Linienzügen ---------------------------------
// Ohne Dunkeltastung (s.u.) hinterlässt jeder Sprung eine schwache Spur.
// Sprünge fahren mit JUMP_SPEED-facher Schreibgeschwindigkeit (Spur =
// 1/JUMP_SPEED so hell wie eine Linie) und bremsen auf den letzten
// JUMP_RAMP Einheiten auf JUMP_LANDING ab, damit auch langsame
// X/Y-Verstärker alter Oszilloskope genau am Linienanfang ankommen.
//
// Diese Werte gelten im Profil ANALOG (im Profil DIGITAL wird immer hart gesprungen):
//   JUMP_SPEED  24   Standard: Spur ca. 1/24, für Oszilloskope ab ~0,5 MHz X/Y-Bandbreite
//               12   sehr alte/langsame Oszilloskope (Spur heller, aber sicher)
//                0   harter Sprung wie im Original – auf schnellen Analog-Oszilloskopen
//                    praktisch unsichtbar, bei langsamen Über-/Nachschwingen
//   JUMP_LANDING 8   Geschwindigkeit beim Ankommen (kleiner = sanfter)
//   JUMP_RAMP    4   Bremsweg in DAC-Einheiten
// Zeigen Linienanfänge kleine "Haken", JUMP_SPEED oder JUMP_LANDING verkleinern.
#define JUMP_SPEED   24          // gilt im Profil ANALOG; DIGITAL springt immer hart
#define JUMP_LANDING  8
#define JUMP_RAMP     4

// ---- Optional: Dunkeltastung über den Z-Eingang -----------------------------
// Viele Röhren-Oszilloskope haben hinten eine BNC-Buchse "Z" / "Z-Mod" / "Intensity".
// Ein Logikpegel dort schaltet den Strahl aus. Mit einem dritten Kabel
// (GPIO ZBLANK_PIN → Z-Eingang) verschwinden die Rücklaufspuren dann vollständig.
// Ein- und ausschalten sowie den Pegel wählen: Webinterface → Einstellungen →
// Oszilloskop → "Dunkeltastung (Z-Eingang)". Ausgeschaltet ist der Pin hochohmig.
//   ZBLANK_PIN         GPIO für den Z-Ausgang (frei, kein Boot-Pin; Standard 27)
//   ZBLANK_ENABLE      Startwert: 0 = aus (Standard, ohne drittes Kabel), 1 = an
//   ZBLANK_ACTIVE_HIGH Startwert: 1 = High-Pegel (3,3 V) macht dunkel (z.B. viele Hameg)
//                                 0 = Low-Pegel macht dunkel – im Handbuch des Oszilloskops prüfen
//   ZBLANK_SETTLE_NS   Wartezeit im Dunkeln nach dem Sprung, bis X/Y stehen
#define ZBLANK_PIN         27
#define ZBLANK_ENABLE       0
#define ZBLANK_ACTIVE_HIGH  1
#define ZBLANK_SETTLE_NS 5000

// Stunden- und Minutenzeiger sind schmale Nadeln: Ihre beiden Kanten liegen
// nur 0–4 DAC-Einheiten auseinander und verschmelzen auf dem Schirm zu einer
// Linie, die bei voller Helligkeit deutlich heller wäre als der Rest.
// 55 % gleicht das bei den meisten Oszilloskopen aus (im Webinterface unter
// Einstellungen nachstellbar). Richtwerte: Röhren-Oszilloskope 55 % (scharfer,
// dünner Strahl bis 85 %), digitale mit Helligkeitsabstufung (z. B. TDS3000)
// 50–65 %, Geräte ohne Abstufung (z. B. HP 54200A, Analog Discovery) 100 %.
#define HAND_BRIGHT_PCT 55

// Helligkeit der Minutenstriche und der Ziffern: 0–100 % der normalen Verweilzeit
// (0 = ausgeblendet). Im Webinterface regelbar.
#define TICK_BRIGHT_PCT  100
#define DIGIT_BRIGHT_PCT 100

// Kernzuordnung. Nicht ändern: Der WLAN-Stack des ESP32 läuft auf Core 0,
// die Strahlausgabe braucht Core 1 exklusiv.
#define SCOPE_CORE 1
#define UI_CORE    0

// ---------------------------------------------------------------------------
// OLED: Waveshare 2.42" (SSD1309, 128x64) über 4-Wire-SPI
// ---------------------------------------------------------------------------
// Auslieferungszustand des Moduls – nichts umlöten.
// Hardware-SPI (VSPI) des ESP32: CLK und DIN liegen fest auf GPIO18/23.
#define OLED_CLK_PIN 18      // Display CLK  (nur zur Dokumentation – VSPI SCK)
#define OLED_DIN_PIN 23      // Display DIN  (nur zur Dokumentation – VSPI MOSI)
#define OLED_CS_PIN   5      // Display CS
#define OLED_DC_PIN  17      // Display DC
#define OLED_RST_PIN  4      // Display RST; auf U8X8_PIN_NONE setzen, falls fest auf 3V3
#define OLED_SPI_HZ  8000000 // 8 MHz (SSD1309: max. 10 MHz)

#define OLED_REFRESH_MS   200        // Displayaktualisierung in ms (Sekunden laufen flüssig)
#define OLED_SHIFT_EVERY_S 60        // Pixel-Shift gegen Einbrennen: alle n Sekunden (0 = aus)
#define OLED_BRIGHT_PCT   50         // Helligkeit 0–100 % (0 = aus; niedriger = langlebiger)
#define OLED_SECONDS      1          // 1 = Uhrzeit mit Sekunden, 0 = nur HH:MM
#define OLED_FLIP         0          // 1 = Anzeige um 180° drehen (Display kopfüber eingebaut)

// Sprache für Webinterface, OLED und Protokoll: 0 = Deutsch, 1 = English.
// Startwert – im Webinterface umschaltbar (wird gespeichert).
#define UI_LANG           0

// Zeitformat für OLED und Webinterface: 0 = 24 Stunden, 1 = 12 Stunden (AM/PM).
// Startwert – im Webinterface umschaltbar (wird gespeichert).
#define CLOCK_12H         0

// ---------------------------------------------------------------------------
// Webinterface (http://scope-clock.local bzw. IP-Adresse der Uhr)
// ---------------------------------------------------------------------------
// Statusseite (NTP, WLAN, Profil, Bildrate, Log) und WLAN-Einrichtung, ohne
// Passwort. Für Handys optimiert.
//   WEB_SETUP_AP_SSID     offenes Setup-WLAN, das die Uhr selbst aufspannt, wenn
//                         kein WLAN eingetragen ist oder keine Verbindung klappt.
//                         Einrichtung dann unter http://192.168.4.1 (Handys öffnen
//                         die Seite meist automatisch).
//   WEB_SETUP_AP_AFTER_S  so lange ohne Verbindung, bis das Setup-WLAN startet
//   WEB_MAX_NETWORKS      max. Anzahl im Webinterface gespeicherter WLANs
//   LOG_BUFFER_SIZE       Größe des Log-Puffers für die Log-Anzeige (Byte, RAM;
//                         8 KB ≈ 120 Zeilen, belegt 2 × diesen Wert im RAM)
#define WEB_SETUP_AP_SSID    "ScopeClock-Setup"
#define WEB_SETUP_AP_AFTER_S 60
#define WEB_MAX_NETWORKS     10
#define LOG_BUFFER_SIZE      8192

// ---------------------------------------------------------------------------
// Diverses
// ---------------------------------------------------------------------------
#define WIFI_HOSTNAME "scope-clock"   // Start-Gerätename (im Webinterface änderbar) im Router, Webinterface: http://scope-clock.local

// Testbild beim Einschalten: so viele Sekunden Rahmen, Kreis und Mittelkreuz, danach die Uhr
// (zum schnellen Prüfen von Position und Größe am Oszilloskop). 0 = aus.
#define BOOT_TEST_S 10
#define SERIAL_BAUD   115200          // serielle Konsole (Arduino: Serieller Monitor)

// Zifferblatt-Variante auf dem Oszilloskop (Startwert – im Betrieb im
// Webinterface umschaltbar: über die Vorschau wischen; wird gespeichert):
//   0 = Ziffern 1–12
//   1 = nur die vier Hauptziffern 12, 3, 6, 9
//   2 = römische Ziffern I–XII
//   3 = nur die vier römischen Hauptziffern XII, III, VI, IX
//   4 = Bahnhof: Stundenbalken statt Ziffern, dünne Minutenstriche
#define DIAL_STYLE 0

// Zifferblatt automatisch wechseln (Startwert – im Webinterface einstellbar):
// alle n Minuten das nächste der fünf Zifferblätter zeigen, 0 = aus (Standard).
// Erlaubt: 0, 1, 5, 15, 60. Einstellung und zuletzt gezeigtes Zifferblatt werden
// gespeichert – nach dem Einschalten geht der Wechsel dort weiter.
#define DIAL_AUTO_MIN 0

// Bildverschiebung gegen Einbrennen auf der Röhre (Startwert, im Webinterface
// umschaltbar): das ganze Oszilloskop-Bild wandert alle SCOPE_SHIFT_EVERY_S
// Sekunden um bis zu 2 DAC-Stufen (≈ 0,4 mm) auf einem kleinen Kreis weiter.
// So verteilt sich die Belastung der Leuchtschicht auf eine größere Fläche.
// Das Testbild wird nie verschoben.
#define SCOPE_SHIFT        1     // 1 = an (Standard), 0 = aus
#define SCOPE_SHIFT_EVERY_S 60

// EXCEL-Modus (Diagnose ohne Oszilloskop): zeichnet genau ein
// Bild, gibt dabei jeden Punkt als "0xXX,0xYY," seriell aus und hält dann an.
// Die Liste lässt sich in Excel/LibreOffice als XY-Diagramm darstellen.
// Zum Aktivieren die // entfernen.
//#define EXCEL
