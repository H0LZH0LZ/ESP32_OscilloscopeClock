# Software – ESP32 Oscilloscope Clock v2.0

Diese Datei beschreibt, wie die Firmware aufgebaut ist. Für Verdrahtung und Gehäuse siehe
[HARDWARE.md](HARDWARE.md), für den Schnellstart die [README](README.md).

## Dateien

| Datei | Inhalt |
|---|---|
| `ESP32_OscilloscopeClock_v2.0/ESP32_OscilloscopeClock_v2.0.ino` | Firmware (Tasks, DAC, Zeit, WLAN, OLED) |
| `ESP32_OscilloscopeClock_v2.0/config.h` | Alle Einstellungen: Zeitzone, NTP, Pins, OLED, Strahlgeschwindigkeit, Oszilloskop-Profil, Zifferblatt |
| `ESP32_OscilloscopeClock_v2.0/secrets.h` | WLAN-Liste, **nur lokal** (steht in `.gitignore`) |
| `ESP32_OscilloscopeClock_v2.0/secrets_example.h` | Vorlage für `secrets.h`. Wird verwendet, wenn `secrets.h` fehlt (dann Offline-Betrieb) |
| `ESP32_OscilloscopeClock_v2.0/DataTable.h` | Vektordaten für Zifferblatt, Ziffern und Zeiger (aus v1.0) |
| `ESP32_OscilloscopeClock_v2.0/webpage.h` | Webinterface: eine Seite (HTML, CSS, JavaScript) im Flash |
| `platformio.ini` | Alternative zur Arduino IDE |

## Aufgabenteilung auf die beiden Kerne

```
            Core 1                                   Core 0
 ┌──────────────────────────┐          ┌────────────────────────────────────────┐
 │ scopeTask                │          │ uiTask (alle 10 ms)                    │
 │  endlos:                 │  g_time  │  publishTime()  → g_timePacked         │
 │   Zeit lesen  ◄──────────┼──Packed──┤  buttonPoll()   → g_scopeProfile       │
 │   Stunden-, Minutenzeiger│ (32 Bit) │  oledUpdate()   alle 200 ms (I2C)      │
 │   Sekundenzeiger hin     │          │  wifiLogChanges() jede Sekunde (Serial)│
 │   Rundtour Zifferblatt   │ g_scope- │                                        │
 │   Sekundenzeiger zurück  │◄─Profile─┤                                        │
 │   → DAC GPIO25 / GPIO26  │ (1 Byte) ├────────────────────────────────────────┤
 └──────────────────────────┘          │ wifiTask: WiFiMulti, Verbindung halten │
                                       │ System:   WLAN-Stack, SNTP-Client      │
                                       └────────────────────────────────────────┘
```

- **Core 1** zeichnet ohne jede Pause, damit das Bild auf dem Oszilloskop nicht flimmert.
  Der Task wartet nie auf eine Sperre, ruft kein `vTaskDelay()` auf und liest pro Bild genau
  einen 32-Bit-Wert. Der Idle-Watchdog von Core 1 ist deshalb abgeschaltet (`disableCore1WDT()`).
- **Core 0** übernimmt alles andere. WLAN und I2C werden im `uiTask` initialisiert,
  damit ihre Interrupts auf Core 0 laufen.
- Die Funktionen des Zeichenpfads (`beamPoint`, `beamMove`, `PlotTable`, DAC-Zugriffe) liegen per
  `IRAM_ATTR` im internen RAM, die Tabellen in `DataTable.h` ebenfalls. Es gibt dadurch keine
  Flash-Cache-Fehlzugriffe, die das Timing und damit die Helligkeit ungleichmäßig machen würden.

### Gemeinsame Daten

| Variable | Schreiber | Leser | Absicherung |
|---|---|---|---|
| `g_timePacked` (Std/Min/Sek) | uiTask | scopeTask | 32-Bit-Zugriff ist atomar |
| `g_scopeProfile` (ANALOG/DIGITAL) | uiTask (BOOT-Taster), `setup()` | scopeTask (`beamJump`) | 1 Byte, atomar |
| `g_dialStyle` (Zifferblatt 0–3) | uiTask (Webinterface), `setup()` | scopeTask (baut Rundtour neu) | 1 Byte, atomar |
| `g_frameCount` (Bildzähler) | scopeTask | uiTask (Bildrate) | 32-Bit-Zugriff ist atomar |
| Log-Ringpuffer | `logMsg()` auf beiden Kernen | Webinterface | Spinlock |
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

## Webinterface

Erreichbar unter `http://scope-clock.local` bzw. der IP-Adresse der Uhr (steht im Log), ohne
Passwort. Das gilt immer, wenn die Uhr mit einem WLAN verbunden ist, nicht nur im Setup-WLAN. Der `WebServer` läuft im `uiTask` auf Core 0 (`webLoop()` alle 10 ms) und beeinflusst
die Strahlausgabe auf Core 1 nicht.

**Seite (`webpage.h`):** eine Seite mit **einklappbarem Seitenmenü** (☰ oben links, bleibt beim
Scrollen sichtbar): Uhr, Status, WLAN, Log. Der oberste Abschnitt (Überschrift, Vorschau, Uhrzeit,
Datum) ist zentriert. Die Abschnittsüberschriften bleiben beim Scrollen oben neben der Menütaste
stehen (`position: sticky`). Jede ist nur innerhalb ihres Abschnitts (`.grp`) angeheftet, so
schiebt die nächste die vorige hinaus. Tippen auf einen Eintrag springt weich dorthin und
schließt das Menü wieder, ebenso Tippen daneben oder Esc. Ganz oben steht eine **Live-Vorschau des Oszilloskop-Bildes**
(Canvas im Look eines grünen Leuchtschirms mit 8×8-Raster). Die Seite lädt dafür einmal die
Vektordaten von `/api/vectors`, also dieselben Tabellen, die der `scopeTask` zeichnet, und stellt
die Zeiger selbst nach der Uhrzeit (Stunde = `(h%12)*5 + m/12`, wie in der Firmware). Zwischen den
Statusabfragen läuft die Zeit im Browser sekundengenau weiter.

**Zifferblatt umschalten:** Tippen auf die Vorschau wechselt reihum zwischen vier Zifferblättern:

| Stil | Ziffern | Tabelle |
|---|---|---|
| 0 | 1–12 | `DialDigits12` |
| 1 | nur 12, 3, 6, 9 | `DialDigitsMin` (aus v1.0) |
| 2 | römisch I–XII | `DialDigitsRoman` |
| 3 | nur XII, III, VI, IX | `DialDigitsRoman4`, erzeugt mit `tools/gen_dial_quarters.py`: übernimmt die Linienzüge dieser vier Zahlen unverändert aus `DialDigitsRoman` |
 Die Vorschau zeigt den neuen Stil sofort, kurz blendet sie seinen Namen ein.
`POST /api/dial` setzt `g_dialStyle`. Der `scopeTask` erkennt den Wechsel vor dem nächsten Bild
und berechnet die Rundtour neu. Das dauert einige zehn Millisekunden, so lange steht der Strahl
(kurzer Lichtpunkt). Die Auswahl wird im NVS gespeichert (Schlüssel `dial`), `DIAL_STYLE` in
`config.h` ist nur der Startwert. Alle vier Ziffernsätze liegen deshalb im RAM (zusammen ca. 0,7 KB).

Darunter folgen drei Bereiche:
- **Status:** große Uhrzeit, KW und Datum, NTP mit letztem Sync, WLAN, Signal, Adresse, Profil,
  Bildrate, Laufzeit, freier Speicher. Wird alle 2 s aktualisiert.
- **WLAN:** gespeicherte Netze (Netze aus dem Webinterface lassen sich löschen), Netzsuche
  (Netz antippen übernimmt den Namen), Formular, „Neu verbinden“.
- **Log:** die letzten `LOG_BUFFER_SIZE` Byte, alle 5 s aktualisiert, scrollt automatisch mit.

Für Handys optimiert: eine Spalte, Schaltflächen mindestens 44 px hoch, Eingabefelder mit 16 px
Schrift (iOS zoomt sonst), helles/dunkles Design nach Systemeinstellung, Rand für Notch und
Home-Balken, keine externen Dateien (funktioniert im Setup-WLAN ohne Internet). Abfragen
pausieren, solange die Seite im Hintergrund ist. Texte von der Uhr werden immer als Text
eingesetzt (`textContent`), nie als HTML.

**Schnittstellen:**

| Methode und Pfad | Funktion |
|---|---|
| `GET /` | die Seite |
| `GET /api/status` | JSON: `time`, `date`, `kw`, `ntp{synced,overdue,last,server}`, `wifi{configured,connected,ssid,rssi,ip,host}`, `ap{active,ssid,ip}`, `profile`, `dial`, `fps`, `uptime`, `heap`, `fw` |
| `GET /api/log` | Log als Text, älteste Zeile zuerst |
| `GET /api/networks` | gespeicherte Netze `[{ssid, source:"secrets"/"web"}]`, nie Passwörter |
| `GET /api/vectors` | Vektordaten für die Vorschau (ca. 9 KB): `dial`, `digits` (alle vier Zifferblätter, Index = Stil), `hr`, `min`, `sec` (je 60 Stellungen). Linienzüge als `[x0,y0,x1,y1,…]`, 0–255, y nach oben |
| `GET /api/scan` | Netzsuche im Hintergrund: erst `{"running":true}`, dann `{"networks":[{ssid,rssi,open}]}` |
| `POST /api/wifi` | `ssid`, `pw` speichern bzw. Passwort ändern (SSID 1–32, Passwort leer oder 8–63 Zeichen) |
| `POST /api/wifi/delete` | `ssid` löschen, danach Neustart (WiFiMulti kann Netze nicht vergessen) |
| `POST /api/reconnect` | Verbindung trennen, `wifiTask` verbindet mit dem stärksten Netz |
| `GET /api/settings` | aktuelle Einstellungen, Standardwerte (`defaults`) und Zeitzonen-Liste |
| `POST /api/settings` | beliebige Felder `profile`, `dial`, `beamNs`, `handPct`, `jumpSpeed`, `tz`, `ntp`, `oledContrast`, `oledShift`: prüfen, sofort anwenden, speichern. Antwort `{ok, msg, settings}` |
| `POST /api/settings/reset` | alle Einstellungen auf die Werte aus `config.h` |
| `POST /api/dial` | `style` 0 = 1–12, 1 = 12/3/6/9, 2 = römisch I–XII, 3 = römisch XII/III/VI/IX: Zifferblatt umschalten (Oszilloskop und Vorschau), wird gespeichert |

**Gespeicherte WLANs:** bis zu `WEB_MAX_NETWORKS` (5) Netze im NVS (Namensraum `scopeclock`,
Schlüssel `netn`, `net_s0…`, `net_p0…`), zusätzlich zu `secrets.h`. Sie bleiben bei einem
Firmware-Update erhalten.

**Setup-WLAN (Captive Portal):** Ist kein WLAN eingetragen oder seit `WEB_SETUP_AP_AFTER_S`
(60 s) keine Verbindung möglich, öffnet `webApManage()` das offene WLAN `WEB_SETUP_AP_SSID`
(„ScopeClock-Setup“) im Modus AP+STA. Ein `DNSServer` beantwortet jeden Namen mit 192.168.4.1,
unbekannte Adressen werden auf die Startseite umgeleitet. So öffnen Handys die
Einrichtungsseite automatisch. Steht die Verbindung zu einem bekannten WLAN 30 s lang, schließt
das Setup-WLAN wieder. Das OLED zeigt in dieser Zeit „Setup ScopeClock-Setup“ und
„http://192.168.4.1“.

**Log:** `logMsg()` ersetzt die direkten `Serial`-Ausgaben. Jede Zeile bekommt einen Zeitstempel
(`hh:mm:ss` nach dem ersten NTP-Sync, davor `+<s>s` seit dem Start) und geht an die Konsole und in
einen Ringpuffer (`LOG_BUFFER_SIZE`, 4 KB). Ein Spinlock schützt den Puffer, weil beide Kerne
schreiben können.

**Bildrate:** `scopeTask` zählt die Bilder (`g_frameCount`), der `uiTask` rechnet daraus jede
Sekunde die Bildrate für die Statusanzeige.

## Einstellungen im Webinterface

Der Abschnitt „Einstellungen“ ändert Werte im laufenden Betrieb. Jede Änderung wirkt sofort und wird
im NVS gespeichert. Schieberegler senden beim Loslassen, damit nicht bei jeder Bewegung in den Flash
geschrieben wird. `config.h` liefert nur die Startwerte. „Auf Standard zurücksetzen“ löscht die
gespeicherten Werte, die WLANs bleiben erhalten.

| Einstellung | NVS-Schlüssel | Bereich | Wirkung |
|---|---|---|---|
| Oszilloskop-Profil | `profile` | DIGITAL / ANALOG | `g_scopeProfile`, auch per BOOT-Taster |
| Zifferblatt | `dial` | 4 Stile | `g_dialStyle`, synchron mit Tippen auf die Vorschau |
| Strahlgeschwindigkeit | `beam` | 1.000–10.000 ns pro Schritt | `g_beamNs`, `scopeTask` berechnet das Timing zwischen zwei Bildern neu (`beamTiming()`) |
| Zeiger-Helligkeit | `hand` | 30–100 % | `g_handPct` |
| Rücklauf-Tempo | `jump` | 0 (hart) – 48 | `g_jumpSpeed`, nur im Profil ANALOG wirksam |
| Zeitzone | `tz` | Liste `TIMEZONES[]` (bzw. `TZ_INFO`) | `configTzTime()` neu |
| NTP-Server | `ntp` | Hostname, max. 63 Zeichen | erster Server, `configTzTime()` neu |
| OLED-Helligkeit | `oled` | 1–255 | `u8g2.setContrast()` |
| OLED-Pixel-Shift | `shift` | an/aus | Versatz gegen Einbrennen |

Die Firmware prüft jeden Wert (`handleSettingsPost()`) und meldet ungültige Eingaben mit Grund
zurück. `loadSettings()` liest alle Werte in `setup()`, bevor die Tasks starten.

## BOOT-Taster: Oszilloskop-Profil

`buttonPoll()` im `uiTask` fragt den BOOT-Taster (GPIO0, gedrückt = LOW) alle 10 ms ab.
Nach `BUTTON_LONG_MS` (3 s) Halten wird `g_scopeProfile` umgeschaltet. Das ist ein Byte und
damit atomar, `beamJump()` auf Core 1 liest es beim nächsten Sprung. Danach wird das Profil im
NVS gespeichert (`Preferences`, Namensraum `scopeclock`, Schlüssel `profile`). Pro Tastendruck
wird nur einmal umgeschaltet, auch wenn der Taster länger gehalten wird. Wer vorher loslässt,
bricht ab. `setup()` lädt das Profil vor dem Start des `scopeTask`. Ist noch nichts gespeichert,
gilt `SCOPE_PROFILE` aus `config.h`.

Während des Haltens (ab 0,3 s) ersetzt `oledProfileOverlay()` die normale Anzeige:

```
 Oszilloskop-Profil       Font 6x10
  DIGITAL > ANALOG        Font 7x14 fett: aktuelles > neues Profil
         3                Countdown 3, 2, 1 als 7-Segment-Ziffer (9×16 px, drawSegDigit)
```

Eine „1“ besteht nur aus den rechten Segmenten und wird im Countdown deshalb optisch zur Mitte
gerückt. Nach der Umschaltung erscheint sofort wieder die normale Anzeige.

## Konsolenausgaben (115200 Baud) und Log im Webinterface

Beide zeigen dieselben Zeilen, mit Zeitstempel (`+<s>s` bis zum ersten NTP-Sync):

```
+0s ESP32 Oscilloscope Clock v2.0 (basierend auf v1.0 von Mauro Pintus 2018)
+0s CPU: 240 MHz
+0s [Scope] Profil: DIGITAL – BOOT-Taster 3 s halten zum Umschalten
+0s [UI] laeuft auf Core 0
+0s [Scope] laeuft auf Core 1
+0s [Scope] Rundtour: 76 Linienzuege, Spruenge gesamt 1033 DAC-Einheiten
+0s [WiFi] bekanntes Netz (secrets.h): "Gastzugang"
+0s [WiFi] bekanntes Netz (Webinterface): "Werkstatt"
+0s [Web] Webserver gestartet (Port 80)
+3s [WiFi] verbunden mit "Gastzugang", IP 192.168.1.23, RSSI -61 dBm
+3s [Web] erreichbar unter http://scope-clock.local und http://192.168.1.23
21:15:06 [NTP] Sync 15.11.2026 21:15:06
21:40:12 [WiFi] Verbindung verloren – versuche erneut
21:41:12 [Web] Setup-WLAN "ScopeClock-Setup" aktiv – Einrichtung unter http://192.168.4.1
22:02:30 [Web] WLAN "Werkstatt" gespeichert
23:15:06 [NTP] Warnung: letzter Sync liegt mehr als 2 Intervalle zurueck
23:20:00 [Scope] Profil umgeschaltet: ANALOG, gespeichert
```

Passwörter werden nie ausgegeben.

## OLED-Anzeige

```
      22:15:06            7-Segment, 24h, Ziffern 11×20 px (selbst gezeichnet)
 KW 46 - 15.11.2026       Font 6x10, KW nach ISO 8601
WLAN Gastzugang           Font 5x8, max. 25 Zeichen (sonst mit ".." gekürzt)
   Signal -61 dBm         Font 5x8
```

Im Setup-WLAN stehen in den beiden unteren Zeilen „Setup ScopeClock-Setup“ und „http://192.168.4.1“.

- **7-Segment-Ziffern:** Die fertigen 7-Segment-Fonts von U8g2 sind für `HH:MM:SS` auf 128 px zu
  breit. `drawSegDigit()` setzt jede Ziffer deshalb aus bis zu 7 Rechtecken zusammen. Welche
  Segmente leuchten, steht in `SEG_MAP`.
- **Kalenderwoche:** `isoWeek()` berechnet sie nach ISO 8601: Die Woche beginnt am Montag,
  KW 1 enthält den 4. Januar. Gegen `strftime("%V")` geprüft für alle Tage von 2000 bis 2040.
- **Pixel-Shift:** Alle `OLED_SHIFT_EVERY_S` Sekunden verschiebt sich das ganze Bild um 0–3 px
  horizontal und 0–1 px vertikal. Das beugt dem Einbrennen statischer Inhalte vor.
- **Positionen nachjustieren:** `Y_TIME`, `Y_DATE`, `Y_SSID`, `Y_RSSI` am Anfang von `oledUpdate()`.
  Text wird in U8g2 an der **Grundlinie** positioniert, nicht an der Oberkante.

## Vektor-Engine: gleiche Helligkeit für alle Linien

Wie hell eine Spur auf dem Schirm leuchtet, hängt davon ab, wie lange der Strahl pro Wegstück
verweilt. Die Zeichenroutine aus v1.0 (Bresenham, gleiche Zeit pro Rasterpunkt) hatte deshalb
sichtbare Helligkeitsunterschiede. v2.0 ersetzt sie durch eine Vektor-Engine mit konstanter
Strahlgeschwindigkeit:

| Regel | Umsetzung | Original v1.0 |
|---|---|---|
| Gleiche Abstände | Jede Linie in Schritte von genau 1 DAC-Einheit **Länge** zerlegt (euklidisch) | Bresenham: bei 45° nur 0,71 Punkte pro Einheit, schräge Linien dunkler |
| Gleiche Verweilzeit | Jeder Schritt genau `BEAM_NS_PER_UNIT`, über den CPU-Taktzähler. Wird ein Schritt durch einen Interrupt verzögert, werden die folgenden kürzer | Zeit pro Punkt je nach Treiberaufruf |
| Ecken | Endpunkt einer Linie = Startpunkt der nächsten, wird nur **einmal** ausgegeben | doppelt, helle Punkte an Ecken |
| Kurze Linien | Rundungsrest wird an die nächste Linie weitergereicht, ein Linienzug bekommt exakt seine Länge an Zeit | jede Linie für sich gerundet |
| Oszilloskop-Profil | DIGITAL: harte Sprünge, sonst zeigt ein abtastendes Oszilloskop Zwischenpunkte. ANALOG: gebremste Sprünge (siehe nächste Zeile). Umschalten im Betrieb per BOOT-Taster (3 s), gespeichert im NVS (`Preferences`, Namensraum `scopeclock`) | – |
| Sprünge (ANALOG) | in 1-Einheit-Schritten mit 24-facher Geschwindigkeit (`JUMP_SPEED`), auf den letzten 4 Einheiten bremsend (`JUMP_LANDING`, `JUMP_RAMP`). Die Rücklaufspur hat ca. 1/24 der Linienhelligkeit, alte Verstärker landen trotzdem genau | harter Sprung |
| Reihenfolge | Zifferblatt und Ziffern werden beim Start einmal zu einer Rundtour mit minimalen Sprüngen sortiert (Nächster-Nachbar + 2-opt), Einstieg dort, wo der Sekundenzeiger endet | Tabellenreihenfolge |
| Sekundenzeiger | hin und zurück mit je halber Helligkeit gezeichnet, dadurch entfällt der lange Sprung vom Rand zur Mitte | – |
| Richtung | jeder Linienzug wird vom näheren Ende aus gezeichnet | immer vom Tabellenanfang |
| Dunkeltastung | optional über den Z-Eingang des Oszilloskops (`ZBLANK_PIN`), dann gibt es gar keine Rücklaufspur mehr | – |
| Zeiger-Rauten | Stunden- und Minutenzeiger mit `HAND_BRIGHT_PCT` (67 %), weil ihre zwei dicht liegenden Kanten verschmelzen | doppelt so hell |
| Tabellengrenzen | immer geprüft | Lesezugriff hinter das Tabellenende, wenn ein Endzeichen fehlt |

Ergebnis der Simulation (Leuchtspur mit ca. 2,4 Einheiten Strahldurchmesser, Helligkeit relativ
zu den Zifferblatt-Strichen, mehrere Uhrzeiten):

| | Original v1.0 | v2.0 |
|---|---|---|
| Sekundenzeiger | 0,80 – 1,00 | 0,92 – 1,07 |
| Minutenzeiger | 1,23 – 1,65 | 0,95 – 1,07 |
| Stundenzeiger | 1,25 – 1,71 | 0,99 – 1,19 |
| Verweilzeit pro Länge, Sekundenzeiger in allen 60 Stellungen | 0,73 – 1,00 | 0,98 – 1,00 |
| Rücklauf-Energie pro Bild (Schritt-Äquivalente, ohne Z-Dunkeltastung) | – (harter Sprung) | 56 (zuvor mit einfachen Sprüngen: 190) |

Liegen zwei Zeiger genau übereinander, ist die Stelle naturgemäß heller.

**Geschwindigkeit und alte Röhren:** Ein Bild dauert ca. 1.400 Schrittzeiten, bei 3 µs pro Schritt sind
das etwa 230 Bilder/s (römisch ca. 160). Beim Schreiben bewegt sich der Strahl mit
ca. 4 mV/µs, bei Sprüngen mit bis zu ca. 100 mV/µs, beim Ankommen mit ca. 33 mV/µs. Dazu kommt
am Linienanfang ein voller Schritt (3 µs) Verweilzeit, in dem der Strahl sich setzen kann. Das reicht
für Oszilloskope ab etwa 0,5 MHz X/Y-Bandbreite. Bei sehr alten Geräten `JUMP_SPEED` auf 12 senken,
bei Bedarf `BEAM_NS_PER_UNIT` erhöhen (hellere Spur, weniger Bilder/s).

## Oszilloskop-Bild: Tabellenformat (`DataTable.h`)

- Ein Punkt besteht aus 2 Bytes `(x, y)`, Wertebereich 0–255 = DAC-Wert. `(0x80, 0x80)` ist die
  Bildmitte, y wächst nach oben.
- Aufeinanderfolgende Punkte werden mit Linien verbunden (`beamMove()`, siehe Vektor-Engine).
- `0x00` trennt bei Zifferblatt und Ziffern zwei Linienzüge. `x = 0` kommt dort daher nicht als
  Koordinate vor. `0xFF` beendet eine Zeigerstellung.
- Zeiger: je 60 vorberechnete Stellungen, beginnend bei 12 Uhr im Uhrzeigersinn.
  Sekundenzeiger 5 Byte pro Stellung, Minuten- und Stundenzeiger 9 Byte (Raute aus 4 Punkten).
  Der Offset `Stellung × Länge` wählt die Stellung aus. Der Stundenzeiger rückt alle 12 Minuten
  einen Schritt weiter: `hpos = (h % 12) * 5 + m / 12`.
- Eigene Motive lassen sich als weitere Tabelle im selben Format ergänzen und im `scopeTask`
  mit `PlotTable(Tabelle, sizeof(Tabelle), 0x00, 0)` zeichnen. Zum Prüfen ohne Oszilloskop
  dient der `EXCEL`-Modus in `config.h`: Er gibt alle Punkte seriell aus, die Liste lässt sich
  als XY-Diagramm darstellen.

## Feineinstellung

| Symptom | Stellschraube (`config.h`) |
|---|---|
| Bild flimmert | `BEAM_NS_PER_UNIT` verringern (z. B. 2000) |
| Linien zu dunkel | `BEAM_NS_PER_UNIT` erhöhen (z. B. 5000) oder Intensität am Oszilloskop erhöhen |
| Ecken verrundet, Ziffern verwaschen (Oszilloskop zu langsam) | `BEAM_NS_PER_UNIT` erhöhen |
| Linienanfänge mit kleinen „Haken“ | `JUMP_SPEED` (z. B. 12) oder `JUMP_LANDING` (z. B. 4) verringern |
| Rücklaufspuren zwischen den Linien sichtbar | `JUMP_SPEED` erhöhen (32), `0` (harter Sprung, für schnelle Oszilloskope) oder Z-Dunkeltastung nutzen (`ZBLANK_PIN`) |
| Zeiger heller/dunkler als das Zifferblatt | `HAND_BRIGHT_PCT` anpassen |
| Zeit falsch um ganze Stunden | `TZ_INFO` prüfen |
| OLED-Bild 2 px versetzt | in der `.ino` `NONAME0` → `NONAME2` |
| OLED bleibt dunkel | I2C-Adresse (`OLED_I2C_ADDR`), Pull-ups, Brücken für den I2C-Modus (siehe HARDWARE.md) |
