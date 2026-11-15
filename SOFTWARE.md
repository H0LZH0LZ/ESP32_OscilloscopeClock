# Software – ESP32 Oscilloscope Clock v2.0

Diese Datei beschreibt, wie die Firmware aufgebaut ist. Für Verdrahtung und Gehäuse siehe
[HARDWARE.md](HARDWARE.md), für den Schnellstart die [README](README.md).

## Dateien

| Datei | Inhalt |
|---|---|
| `ESP32_OscilloscopeClock_v2.0/ESP32_OscilloscopeClock_v2.0.ino` | Firmware (Tasks, DAC, Zeit, WLAN, OLED) |
| `ESP32_OscilloscopeClock_v2.0/config.h` | Alle Einstellungen: Zeitzone, NTP, Pins, OLED, DAC-Timing, Zifferblatt |
| `ESP32_OscilloscopeClock_v2.0/secrets.h` | WLAN-Liste, **nur lokal** (steht in `.gitignore`) |
| `ESP32_OscilloscopeClock_v2.0/secrets_example.h` | Vorlage für `secrets.h`. Wird verwendet, wenn `secrets.h` fehlt (dann Offline-Betrieb) |
| `ESP32_OscilloscopeClock_v2.0/DataTable.h` | Vektordaten für Zifferblatt, Ziffern und Zeiger (aus v1.0) |
| `platformio.ini` | Alternative zur Arduino IDE |

## Aufgabenteilung auf die beiden Kerne

```
            Core 1                                   Core 0
 ┌──────────────────────────┐          ┌────────────────────────────────────────┐
 │ scopeTask                │          │ uiTask (alle 10 ms)                    │
 │  endlos:                 │  g_time  │  publishTime()  → g_timePacked         │
 │   Zeit lesen  ◄──────────┼──Packed──┤  oledUpdate()   alle 200 ms (I2C)      │
 │   Zifferblatt + Ziffern  │ (32 Bit) │  wifiLogChanges() jede Sekunde (Serial)│
 │   3 Zeiger               │          ├────────────────────────────────────────┤
 │   → DAC GPIO25 / GPIO26  │          │ wifiTask: WiFiMulti, Verbindung halten │
 └──────────────────────────┘          │ System:   WLAN-Stack, SNTP-Client      │
                                       └────────────────────────────────────────┘
```

- **Core 1** zeichnet ohne jede Pause, damit das Bild auf dem Oszilloskop nicht flimmert.
  Der Task wartet nie auf eine Sperre, ruft kein `vTaskDelay()` auf und liest pro Bild genau
  einen 32-Bit-Wert. Der Idle-Watchdog von Core 1 ist deshalb abgeschaltet (`disableCore1WDT()`).
- **Core 0** übernimmt alles andere. WLAN und I2C werden im `uiTask` initialisiert,
  damit ihre Interrupts auf Core 0 laufen.
- Die Funktionen des Zeichenpfads (`Dot`, `Line`, `PlotTable`, DAC-Zugriffe) liegen per `IRAM_ATTR`
  im internen RAM, die Tabellen in `DataTable.h` ebenfalls. So stockt das Bild nicht, wenn der
  Flash-Cache kurz gesperrt ist, zum Beispiel während der WLAN-Stack Daten ins NVS schreibt.

### Gemeinsame Daten

| Variable | Schreiber | Leser | Absicherung |
|---|---|---|---|
| `g_timePacked` (Std/Min/Sek) | uiTask | scopeTask | 32-Bit-Zugriff ist atomar |
| `g_ntpSynced` | SNTP-Callback | uiTask | `bool`, atomar |
| `g_lastSyncEpoch` | SNTP-Callback | uiTask | Spinlock (`time_t` ist 64 Bit) |

## Zeit und NTP

- `configTzTime()` startet den eingebauten SNTP-Client des ESP-IDF und setzt die Zeitzone
  (`TZ_INFO`, inkl. Sommerzeitregel). Er wartet selbst, bis das WLAN steht, und synchronisiert
  danach alle `NTP_SYNC_INTERVAL_MS` (Standard: 1 h).
- Nach jedem Sync ruft er `onNtpSync()` auf. Die Funktion merkt sich nur den Zeitstempel,
  die Ausgabe auf der Konsole übernimmt `wifiLogChanges()`.
- Vor dem ersten Sync zählt `currentTime()` ab `FALLBACK_H:M:S` hoch. Datum und KW sind dann
  unbekannt. Nach dem ersten Sync läuft die Systemuhr des ESP32 selbstständig weiter, auch ohne WLAN.

## WLAN

- `secrets.h` enthält eine Liste `WIFI_NETWORKS[]`. Alle Einträge mit nicht-leerer SSID werden
  bei `WiFiMulti` angemeldet.
- `wifiTask` prüft alle 2 s die Verbindung. Ist sie weg, scannt `WiFiMulti::run()` und verbindet
  sich mit dem **stärksten bekannten** Netz. Der Scan blockiert einige Sekunden, deshalb läuft er
  im eigenen Task und das OLED läuft weiter.
- Ein älteres `secrets.h` mit nur `WIFI_SSID`/`WIFI_PASSWORD` wird weiterhin akzeptiert.

## Konsolenausgaben (115200 Baud)

```
ESP32 Oscilloscope Clock v2.0
CPU: 240 MHz
[UI] laeuft auf Core 0
[Scope] laeuft auf Core 1
[WiFi] bekanntes Netz: "Gastzugang"
[WiFi] bekanntes Netz: "iPhone SE"
[WiFi] verbunden mit "Gastzugang", IP 192.168.1.23, RSSI -61 dBm
[NTP] Sync 15.11.2026 21:15:06, naechster in ca. 60 min
[WiFi] Verbindung verloren – versuche erneut
[NTP] Warnung: letzter Sync liegt mehr als 2 Intervalle zurueck
```

Passwörter werden nie ausgegeben.

## OLED-Anzeige

```
      22:15:06            7-Segment, 24h, Ziffern 11×20 px (selbst gezeichnet)
 KW 46 - 15.11.2026       Font 6x10, KW nach ISO 8601
WLAN Gastzugang           Font 5x8, max. 25 Zeichen (sonst mit ".." gekürzt)
   Signal -61 dBm         Font 5x8
```

- **7-Segment-Ziffern:** Die fertigen 7-Segment-Fonts von U8g2 sind für `HH:MM:SS` auf 128 px zu
  breit. `drawSegDigit()` setzt jede Ziffer deshalb aus bis zu 7 Rechtecken zusammen. Welche
  Segmente leuchten, steht in `SEG_MAP`.
- **Kalenderwoche:** `isoWeek()` berechnet sie nach ISO 8601: Die Woche beginnt am Montag,
  KW 1 enthält den 4. Januar. Gegen `strftime("%V")` geprüft für alle Tage von 2000 bis 2040.
- **Pixel-Shift:** Alle `OLED_SHIFT_EVERY_S` Sekunden verschiebt sich das ganze Bild um 0–3 px
  horizontal und 0–1 px vertikal. Das beugt dem Einbrennen statischer Inhalte vor.
- **Positionen nachjustieren:** `Y_TIME`, `Y_DATE`, `Y_SSID`, `Y_RSSI` am Anfang von `oledUpdate()`.
  Text wird in U8g2 an der **Grundlinie** positioniert, nicht an der Oberkante.

## Oszilloskop-Bild: Tabellenformat (`DataTable.h`)

- Ein Punkt besteht aus 2 Bytes `(x, y)`, Wertebereich 0–255 = DAC-Wert. `(0x80, 0x80)` ist die
  Bildmitte, y wächst nach oben.
- Aufeinanderfolgende Punkte werden mit Linien verbunden (`Line()`, Bresenham-Verfahren).
- `0x00` trennt bei Zifferblatt und Ziffern zwei Linienzüge. `x = 0` kommt dort daher nicht als
  Koordinate vor. `0xFF` beendet eine Zeigerstellung.
- Zeiger: je 60 vorberechnete Stellungen, beginnend bei 12 Uhr im Uhrzeigersinn.
  Sekundenzeiger 5 Byte pro Stellung, Minuten- und Stundenzeiger 9 Byte (Raute aus 4 Punkten).
  Der Offset `Stellung × Länge` wählt die Stellung aus. Der Stundenzeiger rückt alle 12 Minuten
  einen Schritt weiter: `hpos = (h % 12) * 5 + m / 12`.
- Eigene Motive lassen sich als weitere Tabelle im selben Format ergänzen und im `scopeTask`
  mit `PlotTable(Tabelle, sizeof(Tabelle), 0x00, 1, 0)` zeichnen. Zum Prüfen ohne Oszilloskop
  dient der `EXCEL`-Modus in `config.h`: Er gibt alle Punkte seriell aus, die Liste lässt sich
  als XY-Diagramm darstellen.

## Feineinstellung

| Symptom | Stellschraube (`config.h`) |
|---|---|
| Ziffern verzerrt, Ecken verrundet, Linien verwaschen | `DOT_DELAY_CYCLES` erhöhen (z. B. 50, 100, 200) |
| Bild flimmert | `DOT_DELAY_CYCLES` verringern. Prüfen, ob `USE_FAST_DAC 1` gesetzt ist |
| Zeit falsch um ganze Stunden | `TZ_INFO` prüfen |
| OLED-Bild 2 px versetzt | in der `.ino` `NONAME0` → `NONAME2` |
| OLED bleibt dunkel | I2C-Adresse (`OLED_I2C_ADDR`), Pull-ups, Brücken für den I2C-Modus (siehe HARDWARE.md) |
