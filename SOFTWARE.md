# Software – ESP32 Oscilloscope Clock v2.0

**Deutsch** · [English](SOFTWARE.en.md)

Diese Datei beschreibt, wie die Firmware aufgebaut ist. Für Verdrahtung und Gehäuse siehe
[HARDWARE.md](HARDWARE.md), für den Schnellstart die [README](README.md).

## Dateien

| Datei | Inhalt |
|---|---|
| `ESP32_OscilloscopeClock_v2.0/ESP32_OscilloscopeClock_v2.0.ino` | Firmware (Tasks, DAC, Zeit, WLAN, OLED, Webserver) |
| `ESP32_OscilloscopeClock_v2.0/config.h` | Alle Einstellungen: Zeitzone, NTP, Pins, OLED, Strahlgeschwindigkeit, Oszilloskop-Profil, Zifferblatt |
| `ESP32_OscilloscopeClock_v2.0/DataTable.h` | Vektordaten für Zifferblatt, Ziffern und Zeiger – erzeugt mit `tools/gen_vectors.py` |
| `tools/gen_vectors.py` | Erzeugt `DataTable.h` und mit `--vectors` die Vorschau-Daten `vectors.h` (Python 3, keine Zusatzpakete) |
| `ESP32_OscilloscopeClock_v2.0/vectors.h` | Vorschau-Daten für `/api/vectors`: dieselben Tabellen als gzip-komprimiertes JSON (ca. 2 KB), fertig erzeugt mit `python3 tools/gen_vectors.py --vectors` |
| `ESP32_OscilloscopeClock_v2.0/webpage.h` | Webinterface: eine Seite (HTML, CSS, JavaScript) im Flash |
| `web/index.html` | Quelle der Webseite – nach Änderungen `python3 tools/gen_webpage.py` (erzeugt `webpage.h`, gzip) |
| `tools/gen_site.py` | Setzt die Kapitel 1–11 der Dokumentation (aus `docs/dokumentation.html` und `docs/documentation.en.html`) als aufklappbare Abschnitte in die Projektseite `docs/index.html` ein |
| `docs/index.html`, `docs/demo/index.html` | Projektseite und Testseite für GitHub Pages (Quelle: Ordner `docs/`, online unter https://holzholz123.github.io/ESP32_OscilloscopeClock/). Testseite: dieselbe Webseite mit simulierten Schnittstellen, läuft im Browser ohne ESP32 – erzeugt mit `python3 tools/gen_demo.py` aus `webpage.h`, `font.h` und `web/demo/mock-api.js`, `mock-settings.js`, `demo-ui.js` |
| `ESP32_OscilloscopeClock_v2.0/font.h` | Schrift des Webinterfaces („ScopeClock Sans“ = verkleinertes Raleway, zwei feste Schnitte Light (300) für Text und SemiBold (600) für Überschriften/Knöpfe, WOFF2, zusammen ca. 30 KB), ausgeliefert unter `/font.woff2` und `/font-600.woff2` – erzeugt mit `tools/gen_font.py` |
| `tools/gen_font.py` | Erzeugt `font.h` aus `fonts/Raleway-VF.ttf` mit festem Zeichenvorrat – gleiche Eingabe ergibt immer dieselbe Datei (braucht `pip install fonttools brotli`) |
| `fonts/Raleway-VF.ttf` | Quelle der Schrift (Raleway, variable Schrift), auch für die PDF-Dokumentation |
| `fonts/OFL.txt` | Lizenz der Schrift (SIL Open Font License 1.1) |
| `ESP32_OscilloscopeClock_v2.0/icon.h` | App-Symbol (`/icon.svg`, `/icon.png` 192 px) und `/manifest.json` für „Zum Startbildschirm hinzufügen“ – erzeugt mit `tools/gen_icon.py` (braucht Pillow) |
| `platformio.ini` | Alternative zur Arduino IDE |

## Aufgabenteilung auf die beiden Kerne

```
            Core 1                                   Core 0
 ┌──────────────────────────┐          ┌────────────────────────────────────────┐
 │ scopeTask                │          │ uiTask (alle 10 ms)                    │
 │  endlos:                 │  g_time  │  publishTime()  → g_timePacked         │
 │   Zeit lesen  ◄──────────┼──Packed──┤  buttonPoll()   → g_scopeProfile       │
 │   Stunden-, Minutenzeiger│ (32 Bit) │  oledUpdate()   alle 200 ms (SPI)      │
 │   Sekundenzeiger hin     │          │  wifiLogChanges() jede Sekunde (Serial)│
 │   Rundtour Zifferblatt   │ g_scope- │  webLoop()      Webserver              │
 │   Sekundenzeiger zurück  │◄─Profile─┤                                        │
 │   → DAC GPIO25 / GPIO26  │ (1 Byte) ├────────────────────────────────────────┤
 └──────────────────────────┘          │ wifiTask: WiFiMulti, Verbindung halten │
                                       │ System:   WLAN-Stack, SNTP-Client      │
                                       └────────────────────────────────────────┘
```

- **Core 1** zeichnet ohne jede Pause, damit das Bild auf dem Oszilloskop nicht flimmert.
  Der Task wartet nie auf eine Sperre, ruft kein `vTaskDelay()` auf und liest pro Bild genau
  einen 32-Bit-Wert. Der Idle-Watchdog von Core 1 ist deshalb abgeschaltet (`disableCore1WDT()`).
- **Core 0** übernimmt alles andere. WLAN und der Display-Bus (SPI) werden im `uiTask` initialisiert,
  damit ihre Interrupts auf Core 0 laufen.
- Die Funktionen des Zeichenpfads (`beamPoint`, `beamMove`, `PlotTable`, DAC-Zugriffe) liegen per
  `IRAM_ATTR` im internen RAM, die Tabellen in `DataTable.h` ebenfalls. Es gibt dadurch keine
  Flash-Cache-Fehlzugriffe, die das Timing und damit die Helligkeit ungleichmäßig machen würden.

### Gemeinsame Daten

| Variable | Schreiber | Leser | Absicherung |
|---|---|---|---|
| `g_timePacked` (Std/Min/Sek) | uiTask | scopeTask | 32-Bit-Zugriff ist atomar |
| `g_scopeProfile` (ANALOG/DIGITAL) | uiTask (BOOT-Taster, Webinterface), `setup()` | scopeTask (`beamJump`) | 1 Byte, atomar |
| `g_dialStyle` (Zifferblatt 0–3) | uiTask (Webinterface), `setup()` | scopeTask (baut Rundtour neu) | 1 Byte, atomar |
| `g_frameCount` (Bildzähler) | scopeTask | uiTask (Bildrate) | 32-Bit-Zugriff ist atomar |
| Log-Ringpuffer | `logMsg()` auf beiden Kernen | Webinterface | Spinlock |
| `g_ntpSynced` | SNTP-Callback | uiTask | `bool`, atomar |
| `g_timeManual` | Webinterface (`POST /api/time`) | uiTask | `bool`, atomar |
| `s_lang` (Sprache), `s_dstForced` (Sommer-/Winterzeit von Hand) | uiTask (Webinterface) | uiTask | nur Core 0 |
| `g_lastSyncEpoch` | SNTP-Callback | uiTask | Spinlock (`time_t` ist 64 Bit) |

## Zeit und NTP

- `configTzTime()` startet den eingebauten SNTP-Client des ESP-IDF und setzt die Zeitzone
  (`TZ_INFO`, inkl. Sommerzeitregel). Er wartet selbst, bis das WLAN steht, und synchronisiert
  danach alle `NTP_SYNC_INTERVAL_MS` (Standard: 1 h).
- Nach jedem Sync ruft er `onNtpSync()` auf. Die Funktion merkt sich nur den Zeitstempel,
  die Ausgabe auf der Konsole übernimmt `wifiLogChanges()`.
- Vor dem ersten Sync zählt `currentTime()` ab `FALLBACK_H:M:S` hoch. Datum und KW sind dann
  unbekannt. Nach dem ersten Sync läuft die Systemuhr des ESP32 selbstständig weiter, auch ohne WLAN.
- **Ohne Internet** lässt sich die Zeit im Webinterface stellen (`POST /api/time`): entweder als
  Unix-Zeit vom Handy (`epoch`, „Uhrzeit dieses Geräts übernehmen“) oder als Datum und Uhrzeit aus
  den Eingabefeldern (`local`, gilt in der eingestellten Zeitzone, umgerechnet mit `mktime()`).
  `settimeofday()` setzt die Systemuhr, `g_timeManual` macht sie gültig (`timeValid()`). Ein späterer
  NTP-Abgleich überschreibt sie. Der ESP32 hat keine gepufferte Echtzeituhr: Nach einem Neustart
  muss die Zeit erneut gestellt werden.
- **Sommer-/Winterzeit bei der Handeingabe:** „Automatisch“ (Regeln der Zeitzone), „Sommerzeit“ oder
  „Winterzeit“ (`dst` = -1/1/0). Fest gewählt setzt `applyDstOverride()` eine Zeitzone ohne
  Umschaltregel, z. B. `CEST-2:00` statt `CET-1CEST,M3.5.0,M10.5.0/3` (`tzPart()` liest Name und
  Versatz aus der POSIX-Zeitzone). Beim nächsten NTP-Abgleich oder einer Zeitzonen-Änderung gilt
  wieder die normale Zeitzone. Die Auswahl erscheint nur bei Zeitzonen mit Sommerzeit.

## Echtzeituhr DS3231 (optional)

Ab Werk aus (`RTC_ENABLE 0`), einschaltbar unter Einstellungen → Uhrzeit (NVS-Schlüssel `rtc`).
Der DS3231 hängt an I²C (`RTC_SDA_PIN` 21, `RTC_SCL_PIN` 22, 100 kHz, Adresse 0x68) und läuft in **UTC**.
Die Firmware spricht ihn direkt über `Wire` an (keine zusätzliche Bibliothek); alle Zugriffe laufen im `uiTask`.

- **Start / Einschalten** (`rtcApply`): Ist die Systemzeit schon gültig (NTP oder von Hand), wird sie in die RTC
  geschrieben, sonst wird die Systemzeit aus der RTC gestellt (`g_timeRtc` – zählt als gültige Zeit mit Datum).
- **NTP-Abgleich** (stündlich) und **Uhrzeit von Hand**: RTC nachstellen (`rtcSetFromSystem`, löscht das OSF-Bit).
- **Ohne NTP:** alle 10 min die Systemuhr aus der RTC nachstellen (`rtcToSystem`) – genauer als der ESP32-Takt.
- **Ungültig** ist die RTC-Zeit, wenn das OSF-Bit gesetzt ist (Oszillator stand, z. B. Batterie leer oder neu)
  oder das Datum unplausibel ist. Sie wird dann nicht übernommen.
- **Status:** `/api/status` → `rtc{enabled,found,valid}` und `ntp.rtc`
  (Zeit kommt gerade aus der RTC). Die Gruppe „Uhrzeit stellen“ klappt dann nicht automatisch auf.

## Sprache (Deutsch/English)

- `s_lang` (NVS-Schlüssel `lang`, Startwert `UI_LANG`) gilt für Meldungen des Webinterfaces, das OLED
  und das Protokoll. `tr("deutsch", "english")` wählt den Text. Protokollzeilen behalten die Sprache,
  in der sie geschrieben wurden. `loadSettings()` läuft deshalb vor der ersten Ausgabe in `setup()`.
- OLED auf Englisch: `WiFi …`, `Week 46 - 2026-11-15` (ISO-Datum), „Oscilloscope profile“.
- Webseite: Sie ist auf Deutsch geschrieben, die Tabelle `EN` im Skript übersetzt. `applyLang()`
  übersetzt alle festen Texte (Textknoten, `aria-label`) – über die Rückwärtstabelle auch zurück –,
  `t()` die Texte aus dem Skript. Umgeschaltet wird im Seitenmenü über die Auswahlliste „Sprache“;
  die Seite wechselt sofort und speichert die Sprache auf der Uhr (`POST /api/settings`, `lang`).
  Eine Änderung von einem anderen Gerät übernimmt die Seite mit der nächsten Statusabfrage.

## WLAN

- WLANs werden nur im Webinterface eingetragen (keine Zugangsdaten im Quelltext). Beim Start
  lädt `webNetsLoad()` die gespeicherten Netze; bei `WiFiMulti` trägt sie **nur der `wifiTask`**
  ein (`s_multiCount`), auch neu hinzugefügte. `WiFiMulti` ist nicht threadsicher – ein `addAP()`
  aus dem Webserver während `run()` könnte abstürzen.
- `wifiTask` prüft alle 2 s die Verbindung. Ist sie weg, scannt `WiFiMulti::run()` und verbindet
  sich mit dem **stärksten bekannten** Netz. Der Scan blockiert einige Sekunden, deshalb läuft er
  im eigenen Task und das OLED läuft weiter.
- Sucht die Webseite gerade Netze („Netze in Reichweite suchen“), pausiert der `wifiTask` bis zu
  15 s (`s_webScanUntil`), sonst würde seine eigene Suche die Ergebnisse löschen.
- Ist jemand mit dem Setup-WLAN verbunden, versucht der `wifiTask` nur alle 60 s, ein bekanntes Netz
  zu erreichen: Jeder Versuch wechselt kurz den Funkkanal und könnte das Handy aus dem Setup-WLAN werfen.
- Neues Passwort für ein gespeichertes Netz: `WiFiMulti` kann Einträge nicht ändern, daher startet
  die Uhr danach neu (wie beim Löschen).

## Webinterface

Erreichbar unter `http://scope-clock.local` bzw. der IP-Adresse der Uhr (steht im Log), ohne
Passwort. Das gilt immer, wenn die Uhr mit einem WLAN verbunden ist, nicht nur im Setup-WLAN. Der `WebServer` läuft im `uiTask` auf Core 0 (`webLoop()` alle 10 ms) und beeinflusst
die Strahlausgabe auf Core 1 nicht.

**Seite (`webpage.h`):** eine Seite mit **einklappbarem Seitenmenü** (☰ oben links, bleibt beim
Scrollen sichtbar): oben Sprache und Darstellung, darunter Uhr, Status, Einstellungen mit den Unterpunkten
Oszilloskop, Uhrzeit, Display, WLAN und System (springen direkt in die jeweilige Gruppe) und Debug.
Der Abschnitt in der Bildschirmmitte ist im Menü grün markiert (Scroll-Spy, `spy()`;
bei einer Einstellungsgruppe ist auch „Einstellungen“ grün), ein 3 px schmaler grüner Balken am
oberen Rand zeigt, wie weit die Seite gescrollt ist.
Die Einstellungen sind in fünf Gruppen geteilt: **Oszilloskop** (Profil zuerst, Zifferblatt,
Testbild an zweiter Stelle, Verweilzeit, Helligkeiten, Rücklauf-Tempo), **Uhrzeit** (ggf. „Uhrzeit stellen“, Zeitformat,
Zeitzone, Zeitserver), **Display** (OLED-Helligkeit, Sekunden, Drehung, Pixelverschiebung)
und **WLAN** (gespeicherte Netze, Netzsuche, Netz hinzufügen, Neu verbinden, WLAN zurücksetzen) und **System** (Gerätename, Einstellungen sichern/laden). Der oberste Abschnitt (Überschrift, Vorschau, Uhrzeit,
Datum) ist zentriert. Die Titelleiste „ESP32 Oscilloscope Clock“ bleibt immer oben neben der
Menütaste stehen, die Abschnittsüberschriften direkt darunter (`position: sticky`). Jede
Abschnittsüberschrift ist nur innerhalb ihres Abschnitts (`.grp`) angeheftet, so schiebt die
nächste die vorige hinaus. `scroll-margin-top` sorgt dafür, dass Sprungziele aus dem Menü unter
der Titelleiste landen. Tippen auf einen Eintrag springt weich dorthin und
schließt das Menü wieder, ebenso Tippen daneben oder Esc. Ganz oben steht eine **Live-Vorschau des Oszilloskop-Bildes**
(Canvas mit 8×8-Raster wie am Oszilloskop). Im **dunklen Design** ist das ein schwarzer Schirm mit
leuchtend grünen Linien, im **hellen Design** ist der Hintergrund durchsichtig, mit dunkelgrünen Linien
und zartem Raster. Die Farben stehen in den CSS-Variablen `--scope-*`, das Canvas liest sie bei jedem
Zeichnen neu, so greift ein Wechsel des Designs sofort. Die Seite lädt dafür einmal die
Vektordaten von `/api/vectors`, also dieselben Tabellen, die der `scopeTask` zeichnet, und stellt
die Zeiger selbst nach der Uhrzeit (Stunde = `(h%12)*5 + m/12`, wie in der Firmware). Zwischen den
Statusabfragen läuft die Zeit im Browser sekundengenau weiter. Die Vorschau wird einmal pro Sekunde neu
gezeichnet; der Sekundenzeiger springt wie am Oszilloskop von Sekunde zu Sekunde. Auf Tablet/PC wird die Vorschau größer (bis 460 bzw.
560 px), auf dem Handy im Querformat stehen Vorschau und Uhrzeit nebeneinander.

**Zifferblatt wechseln:** Wischen über die Vorschau wechselt reihum zwischen fünf Zifferblättern –
nach links zum nächsten, nach rechts zum vorigen (Pointer-Events, ab 40 px waagerecht; senkrechtes
Wischen scrollt weiter die Seite, `touch-action: pan-y`). Am PC gehen auch die Pfeiltasten, wenn die
Vorschau den Fokus hat. Tippen allein wechselt nicht. Reihenfolge beim Wischen und in der
Auswahlliste: Ziffern 1–12 → Ziffern 12 · 3 · 6 · 9 → Nur Striche → Römisch XII · III · VI · IX → Römisch I–XII
(Stil 0, 1, 4, 3, 2 – „Nur Striche“ steht in der Mitte):

| Stil | Ziffern | Tabelle |
|---|---|---|
| 0 | 1–12, 7-Segment-Stil | `DialDigits12` |
| 1 | nur 12, 3, 6, 9, 7-Segment-Stil | `DialDigitsMin` |
| 2 | römisch I–XII, serifenlos, 4 als IIII | `DialDigitsRoman` |
| 3 | nur XII, III, VI, IX | `DialDigitsRoman4` |
| 4 | keine Ziffern, nur Minutenstriche | `DialDigitsNone` (leer) |
 Die Vorschau zeigt den neuen Stil sofort.
`POST /api/dial` setzt `g_dialStyle`. Der `scopeTask` erkennt den Wechsel vor dem nächsten Bild
und berechnet die Rundtour neu. Das dauert einige zehn Millisekunden, so lange steht der Strahl
(kurzer Lichtpunkt). Die Auswahl wird im NVS gespeichert (Schlüssel `dial`), `DIAL_STYLE` in
`config.h` ist nur der Startwert. Alle Ziffernsätze liegen deshalb im RAM (zusammen ca. 0,7 KB).

Darunter folgen drei Bereiche. **Status** und **Protokoll** sind einklappbar (Pfeil V): Status ist anfangs offen, Protokoll zu
(bei jedem Aufruf: Status offen, alles andere zu – nichts wird gespeichert; ein Sprung aus dem Menü klappt sie auf). Jede Einstellung und jede Schaltfläche hat eine kurze Beschreibung:
- **Status:** große Uhrzeit, KW und Datum, WLAN, Signalstärke, Netzwerkadresse, darunter
  Zeitabgleich (NTP) mit letztem Abgleich, OLED-Anzeige (An mit Helligkeit oder Aus, orange), Laufzeit, darunter Grund des letzten Neustarts, Arbeitsspeicher (frei/gesamt, darunter Tiefstwert seit
  dem Neustart und größter freier Block), Prozessorauslastung Kern 0 und Kern 1 (je eine Zeile mit
  kurzer Erläuterung; Kern 1 zeichnet ständig und steht immer auf 100 %),
  Firmware-Version. Wird alle 2 s aktualisiert. Oszilloskop-Profil und Bildrate stehen nicht doppelt
  im Status, sondern nur in den Einstellungen (Profil-Auswahl, Bildrate bei „Verweilzeit des Strahls“).
  Farbig hervorgehoben werden nur Probleme: Orange = Warnung (NTP wartet oder überfällig, WLAN
  sucht, Signal unter −80 dBm, Setup-WLAN aktiv, Tiefstwert unter 20 KB, größter Block unter 8 KB, Kern 0 über 85 %,
  Neustart nach Absturz, Watchdog oder Unterspannung), Rot = Fehler (keine Antwort von der Uhr,
  abgelehnte Eingaben). Normale Zustände und Bestätigungen stehen in normaler Schriftfarbe.
- **Einstellungen:** siehe [Einstellungen im Webinterface](#einstellungen-im-webinterface). Solange
  kein Zeitserver erreichbar war, steht ganz oben zusätzlich „Uhrzeit stellen“ (siehe [Zeit und NTP](#zeit-und-ntp)).
- **WLAN** (Gruppe in den Einstellungen): gespeicherte Netze (bis zu 10, je mit „Löschen“), Netzsuche
  (stärkstes Netz oben, Balken und dBm, das verbundene Netz ist markiert; Netz antippen übernimmt den Namen), Formular „Netz hinzufügen“ (anfangs eingeklappt; offen, solange noch kein Netz gespeichert ist, und beim Antippen eines gefundenen Netzes), „Neu verbinden“, „WLAN zurücksetzen“ (alle Netze löschen).
- **Protokoll** (das Log der Firmware): die letzten `LOG_BUFFER_SIZE` Byte (8 KB, ca. 120 Zeilen),
  alle 5 s aktualisiert, solange „Automatisch aktualisieren“ an ist („Aktualisieren“ lädt sofort). Das Feld
  hat eine feste Höhe (max. 60 % des Bildschirms) mit eigener Scrollleiste. Wischen im Feld scrollt
  nur das Log, nicht die Seite. Steht man unten, folgen neue Zeilen automatisch. Liest man weiter
  oben, bleibt die Position beim Aktualisieren erhalten. Darunter steht die Zeilenzahl.
  „Herunterladen“ speichert das ganze Protokoll (ungefiltert) als Textdatei
  `<gerätename>-protokoll-JJJJMMTT-HHMM.txt`.
  „Leeren“ löscht nach Rückfrage den Ringpuffer der Uhr (`POST /api/log/clear`); die erste neue Zeile
  ist „Protokoll geloescht“.
- **System** (Gruppe in den Einstellungen): Gerätename (`host`, a–z, 0–9, Bindestrich, max. 32; gilt
  sofort für mDNS `http://<name>.local` und beim nächsten Verbinden für den Router), Einstellungen
  sichern (JSON-Datei mit allen Einstellungen außer WLANs)
  und laden (sendet die Werte an `POST /api/settings`), „Alle Einstellungen zurücksetzen“ (alle Gruppen,
  Sprache und Gerätename auf `config.h`, Testbild aus; WLANs bleiben; 8 s „Rückgängig“).

**Bedienhilfen der Seite:**
- **Punkte unter der Vorschau** zeigen das aktive Zifferblatt; Punkt oder Pfeil ‹ › antippen wechselt (zusätzlich zum Wischen).
  Beim Wechsel wird kein Name eingeblendet.
- **Gruppen der Einstellungen** sind eingeklappt (Pfeil V) und öffnen sich per Tippen auf die Überschrift (bei jedem Aufruf wieder zu, nichts wird gespeichert). Menü-Unterpunkte öffnen ihre Gruppe; erscheint „Uhrzeit stellen“, öffnet sich „Uhrzeit“ von selbst und klappt wieder zu, sobald der Zeitserver wieder erreicht ist.
- **„✓ Gespeichert“** erscheint kurz direkt an der geänderten Einstellung (Fehler in Rot).
- **Vorschau beim Ziehen:** Verweilzeit ändert Helligkeit der Vorschau und zeigt die geschätzte Bildrate (~), das
  Rücklauf-Tempo zeigt im Profil ANALOG die Rücklaufspuren in der Vorschau (schwächer, je schneller).
- **Zurücksetzen** einer Gruppe ohne Rückfrage, dafür 8 s lang „Rückgängig“ (sendet die vorigen Werte).
  „WLAN zurücksetzen“ fragt weiterhin nach (Neustart, nicht umkehrbar).
- **Signalstärke** im Status mit Balken.
- **Darstellung** im Seitenmenü: Automatisch, Hell oder Dunkel (nur im Browser gespeichert, `data-theme`).
- **Protokoll-Filter** (Auswahlliste): Alle, WLAN, Zeit, Web, Oszilloskop, Warnungen.
- **Als App:** Manifest und Symbol erlauben „Zum Startbildschirm hinzufügen“; die Seite öffnet sich dann ohne Browserleiste.
- **Schrift:** Raleway Light/SemiBold mit Raleways Original-Ziffern (Mediävalziffern), Text in Schwarz.

Für Handys optimiert: eine Spalte, Schaltflächen mindestens 44 px hoch, Eingabefelder mit 16 px
Schrift (iOS zoomt sonst), helles/dunkles Design nach Systemeinstellung, Rand für Notch und
Home-Balken, keine externen Dateien – auch die Schrift kommt von der Uhr (funktioniert im Setup-WLAN ohne Internet). Abfragen
pausieren, solange die Seite im Hintergrund ist. Texte von der Uhr werden immer als Text
eingesetzt (`textContent`), nie als HTML.

**Schnittstellen:**

| Methode und Pfad | Funktion |
|---|---|
| `GET /` | die Seite |
| `GET /manifest.json`, `/icon.svg`, `/icon.png` | App-Manifest und Symbol für den Startbildschirm |
| `GET /api/status` | JSON: `time`, `date`, `kw`, `ntp{synced,overdue,manual,dst,last,server}`, `wifi{configured,connected,ssid,rssi,ip,host}`, `ap{active,ssid,ip}`, `profile`, `dial`, `fps`, `heap`, `heapTotal`, `heapMin`, `heapBlock`, `cpu0`, `reset`, `uptime`, `fw`, `lang`, `h12`, `oled` (Helligkeit in %, 0 = aus), `test` (Testbild an) |
| `GET /api/log` | Log als Text, älteste Zeile zuerst |
| `POST /api/log/clear` | Log-Ringpuffer leeren (serielle Ausgabe unberührt) |
| `GET /api/networks` | gespeicherte Netze `[{ssid, source:"web"}]`, nie Passwörter |
| `GET /font.woff2`, `GET /font-600.woff2` | Schrift der Seite, Light und SemiBold (aus `font.h`), wird ein Jahr im Browser zwischengespeichert. Zwei feste Schnitte statt einer variablen Schrift, weil manche Android-Browser fette Texte einer variablen Schrift sonst in der Systemschrift zeigen |
| `GET /api/vectors` | Vektordaten für die Vorschau (das Testbild zeigt die Vorschau nicht, nur das Oszilloskop) (ca. 9 KB): `dial`, `digits` (alle Zifferblätter, Index = Stil), `hr`, `min`, `sec` (je 60 Stellungen). Linienzüge als `[x0,y0,x1,y1,…]`, 0–255, y nach oben |
| `GET /api/scan` | Netzsuche im Hintergrund: erst `{"running":true}`, dann `{"networks":[{ssid,rssi,open}]}` |
| `POST /api/wifi` | `ssid`, `pw` speichern bzw. Passwort ändern (SSID 1–32, Passwort leer oder 8–63 Zeichen; Passwortänderung → Neustart) |
| `POST /api/wifi/delete` | `ssid` löschen, danach Neustart (WiFiMulti kann Netze nicht vergessen) |
| `POST /api/wifi/reset` | alle gespeicherten Netze löschen, danach Neustart (Setup-WLAN öffnet sich) |
| `POST /api/reconnect` | Verbindung trennen, `wifiTask` verbindet mit dem stärksten Netz |
| `GET /api/settings` | aktuelle Einstellungen, Standardwerte (`defaults`) und Zeitzonen-Liste |
| `POST /api/settings` | beliebige Felder `profile`, `dial`, `beamNs`, `handPct`, `tickPct`, `digitPct`, `jumpSpeed`, `tz`, `ntp`, `oledPct`, `oledSec`, `oledShift`, `oledFlip`, `lang` (`de`/`en`), `h12` (0/1), `host` (Gerätename), `test` (0/1, Testbild, nicht gespeichert): prüfen, sofort anwenden, speichern. Antwort `{ok, msg, settings}` |
| `POST /api/settings/reset` | `group` = `scope` (Oszilloskop), `time` (Uhrzeit) oder `display`: nur diese Gruppe auf die Werte aus `config.h`; ohne `group` alles inkl. Sprache und Gerätename, Testbild aus. WLANs bleiben immer erhalten |
| `POST /api/dial` | `style` 0 = 1–12, 1 = 12/3/6/9, 2 = römisch I–XII, 3 = römisch XII/III/VI/IX, 4 = nur Striche: Zifferblatt umschalten (Oszilloskop und Vorschau), wird gespeichert |
| `POST /api/time` | `epoch` (Unix-Sekunden, UTC) **oder** `local` (`JJJJ-MM-TTTHH:MM[:SS]`, Ortszeit), optional `dst` (-1 automatisch, 1 Sommerzeit, 0 Winterzeit): Uhrzeit von Hand stellen, 2024–2099. Antwort `msg` nennt die gestellte Zeit, die Seite zeigt sie mit ✓ direkt unter dem gedrückten Knopf |

**Gespeicherte WLANs:** bis zu `WEB_MAX_NETWORKS` (10) Netze im NVS (Namensraum `scopeclock`,
Schlüssel `netn`, `net_s0…`, `net_p0…`). Sie bleiben bei einem
Firmware-Update erhalten.

**Setup-WLAN (Captive Portal):** Ist kein WLAN eingetragen oder seit `WEB_SETUP_AP_AFTER_S`
(60 s) keine Verbindung möglich, öffnet `webApManage()` das offene WLAN `WEB_SETUP_AP_SSID`
(„ScopeClock-Setup“) im Modus AP+STA. Ein `DNSServer` beantwortet jeden Namen mit 192.168.4.1,
unbekannte Adressen werden auf die Startseite umgeleitet. So öffnen Handys die
Einrichtungsseite automatisch. Steht die Verbindung zu einem bekannten WLAN 30 s lang, schließt
das Setup-WLAN wieder. Das OLED zeigt in dieser Zeit „WLAN ScopeClock-Setup“ und
„http://192.168.4.1“.

**Log:** `logMsg()` ersetzt die direkten `Serial`-Ausgaben. Jede Zeile bekommt einen Zeitstempel
(`hh:mm:ss`, sobald die Uhrzeit gültig ist – per NTP oder von Hand gestellt –, davor `+<s>s` seit dem Start) und geht an die Konsole und in
einen Ringpuffer (`LOG_BUFFER_SIZE`, 8 KB). Ein Spinlock schützt den Puffer, weil beide Kerne
schreiben können. Die Zeilen kommen ohne Umlaute aus, weil sie auch über die serielle Konsole laufen.

**Bildrate:** `scopeTask` zählt die Bilder (`g_frameCount`), der `uiTask` rechnet daraus jede
Sekunde die Bildrate für die Statusanzeige.

**CPU-Last Core 0:** Ein Leerlauf-Hook (`esp_register_freertos_idle_hook_for_cpu`) summiert die Zeit,
in der der Leerlauf-Task von Core 0 läuft. Folgen zwei Aufrufe dicht aufeinander (< 200 µs), war der
Kern dazwischen frei. Last = 100 % minus Leerlaufanteil, jede Sekunde neu. Core 1 ist absichtlich
immer zu 100 % mit der Strahlausgabe belegt.

**Arbeitsspeicher und Neustartgrund:** `ESP.getFreeHeap()`, `getHeapSize()`, `getMinFreeHeap()`,
`getMaxAllocHeap()` und `esp_reset_reason()`. Der Neustartgrund steht zusätzlich beim Start im Log.

## Einstellungen im Webinterface

Der Abschnitt „Einstellungen“ ändert Werte im laufenden Betrieb. Jede Änderung wirkt sofort und wird
im NVS gespeichert. Schieberegler senden beim Loslassen, damit nicht bei jeder Bewegung in den Flash
geschrieben wird. `config.h` liefert nur die Startwerte. Jede Gruppe (Oszilloskop, Uhrzeit, Display) hat einen eigenen
Knopf „Auf Standardwerte zurücksetzen“: Er löscht nur die gespeicherten Werte dieser Gruppe, die WLANs bleiben erhalten.

| Einstellung | NVS-Schlüssel | Bereich | Wirkung |
|---|---|---|---|
| Oszilloskop-Profil | `profile` | DIGITAL / ANALOG | `g_scopeProfile`, auch per BOOT-Taster |
| Zifferblatt | `dial` | 5 Stile | `g_dialStyle`, synchron mit dem Wischen über die Vorschau |
| Verweilzeit des Strahls | `beam` | 1.000–10.000 ns pro Schritt | `g_beamNs`, `scopeTask` berechnet das Timing zwischen zwei Bildern neu (`beamTiming()`) |
| Helligkeit Stunden-/Minutenzeiger | `hand` | 0–100 % (0 = aus) | `g_handPct` |
| Helligkeit Minutenstriche | `tick` | 0–100 % (0 = aus) | `g_tickPct`: Verweilzeit der Striche (Tabelle `DialData`) in der Rundtour, in Prozent der normalen Verweilzeit |
| Helligkeit Ziffern | `digit` | 0–100 % (0 = aus) | `g_digitPct`: Verweilzeit der Ziffern in der Rundtour |
| Dunkeltastung (Z-Eingang) | `zbl`, `zhi` | an/aus; High oder Low = dunkel | `g_zblank`, `g_zblankHigh`: `zblankApply()` schaltet GPIO `ZBLANK_PIN` (27) als Ausgang bzw. hochohmig; `beamJump()` springt dann im Dunkeln. Nur mit drittem Kabel zum Z-Eingang |
| Rücklauf-Tempo | `jump` | 0 (sofortiger Sprung) – 48 (× Zeichentempo) | `g_jumpSpeed`, nur im Profil ANALOG wirksam. Bei DIGITAL zeigt der Regler ausgegraut 0 (sofortiger Sprung); der ANALOG-Wert bleibt gespeichert und erscheint beim Zurückschalten wieder |
| Zeitzone | `tz` | Liste `TIMEZONES[]` (bzw. `TZ_INFO`) | `configTzTime()` neu |
| Echtzeituhr (RTC) | `rtc` | an/aus | `rtcApply()`: DS3231 an I²C GPIO21/22 nutzen (siehe Abschnitt Echtzeituhr) |
| Zeitserver (NTP) | `ntp` | Hostname, max. 63 Zeichen | erster Server, `configTzTime()` neu |
| OLED-Helligkeit | `oledp` | 0–100 % (0 = aus) | `oledApplyBrightness()`: 0 % = `setPowerSave(1)`, sonst Kontrast 1–255. Ein alter Wert `oled` (Kontrast) wird beim ersten Start umgerechnet |
| Sekunden anzeigen | `osec` | an/aus | OLED-Uhrzeit HH:MM:SS oder HH:MM |
| OLED-Pixelverschiebung | `shift` | an/aus | Versatz gegen Einbrennen |
| OLED um 180° drehen | `flip` | an/aus | `u8g2.setDisplayRotation(U8G2_R2)`, für ein kopfüber eingebautes Display |
| Gerätename | `host` | a–z, 0–9, Bindestrich, max. 32 | `WiFi.setHostname()`, `MDNS.begin()` neu |
| Testbild | – | an/aus | `g_testPattern`: `scopeTask` zeichnet `TestData` statt der Uhr; nicht gespeichert |
| Sprache | `lang` | Deutsch / English | `s_lang`: Webinterface, OLED, Protokoll |
| Zeitformat | `h12` | 24 Stunden / 12 Stunden (AM/PM) | `s_h12`: Uhrzeit auf OLED und Webseite (Protokoll bleibt 24 h) |

Die Firmware prüft jeden Wert (`handleSettingsPost()`) und meldet ungültige Eingaben mit Grund
zurück. `loadSettings()` liest alle Werte in `setup()`, bevor die Tasks starten.


## BOOT-Taster: Oszilloskop-Profil und Werkseinstellungen

**Testbild beim Start:** `g_testPattern` startet mit `BOOT_TEST_S > 0`; der `uiTask` schaltet es nach
`BOOT_TEST_S` Sekunden aus (Log „Start-Testbild beendet“), außer es wurde vorher im Webinterface von
Hand geschaltet.

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

**Werkseinstellungen:** Wird der Taster weiter gehalten, zählt `oledFactoryOverlay()` ab 5 s
5 … 1 herunter. Bei `BUTTON_FACTORY_MS` (10 s) löscht `factoryReset()` den ganzen NVS-Namensraum
`scopeclock` (alle Einstellungen und WLANs) sowie die vom WLAN-Treiber gemerkten Zugangsdaten und
startet nach 2 s neu. Danach gelten die Werte aus `config.h`, ohne WLAN öffnet die Uhr das Setup-WLAN.
Vorher loslassen bricht ab (das Profil ist dann schon umgeschaltet).

```
 Werkseinstellungen       Font 7x14 fett
         5                ablaufende Zeit 5 … 1, dann 0 bis zum Neustart (7-Segment 11×20 px)
```

## Konsolenausgaben (115200 Baud) und Log im Webinterface

Beide zeigen dieselben Zeilen, mit Zeitstempel (`+<s>s` bis zum ersten NTP-Sync):

```
+0s ESP32 Oscilloscope Clock v2.0 (Idee nach Mauro Pintus 2018)
+0s CPU: 240 MHz
+0s Letzter Neustart: Einschalten
+0s [Scope] Profil: DIGITAL – BOOT-Taster 3 s halten zum Umschalten
+0s [UI] laeuft auf Core 0
+0s [Scope] laeuft auf Core 1
+0s [Scope] Rundtour: 76 Linienzuege, Spruenge gesamt 1033 DAC-Einheiten
+0s [WiFi] bekanntes Netz: "Gastzugang"
+0s [WiFi] bekanntes Netz: "Werkstatt"
+0s [Web] Webserver gestartet (Port 80)
+3s [WiFi] verbunden mit "Gastzugang", IP 192.168.1.23, RSSI -61 dBm
+3s [Web] erreichbar unter http://scope-clock.local und http://192.168.1.23
21:15:06 [NTP] Sync 15.11.2026 21:15:06
21:40:12 [WiFi] Verbindung verloren – versuche erneut
21:41:12 [Web] Setup-WLAN "ScopeClock-Setup" aktiv – Einrichtung unter http://192.168.4.1
22:02:30 [Web] WLAN "Werkstatt" gespeichert
22:05:10 [Zeit] von Hand gestellt (vom Handy): 15.11.2026 22:05:10
23:15:06 [NTP] Warnung: letzter Sync liegt mehr als 2 Intervalle zurueck
23:20:00 [Scope] Profil umgeschaltet: ANALOG, gespeichert
```

Passwörter werden nie ausgegeben.

## OLED-Anzeige

```
      22:15:06            7-Segment, Ziffern 11×20 px (selbst gezeichnet); 12 h: "10:15:06 PM" ohne führende Null, AM/PM klein (Font 5x8)
 KW 46 - 15.11.2026       Ziffern, Punkte, Bindestrich als kleine 7-Segment-Ziffern 5×9 px (offene Ecken), „KW“ Font 6x10; KW nach ISO 8601
WLAN Gastzugang           Font 5x8, max. 25 Zeichen (sonst mit ".." gekürzt)
   Signal -61 dBm         Font 5x8
```

Im Setup-WLAN stehen in den beiden unteren Zeilen „WLAN ScopeClock-Setup“ und „http://192.168.4.1“.

- **7-Segment-Ziffern:** Die fertigen 7-Segment-Fonts von U8g2 sind für `HH:MM:SS` auf 128 px zu
  breit. `drawSegDigit()` setzt jede Ziffer deshalb aus bis zu 7 Rechtecken zusammen. Welche
  Segmente leuchten, steht in `SEG_MAP`. Die Datumszeile nutzt dieselben Ziffern in klein
  (5×9 px, Segment 1 px): `drawSegTextCentered()` zeichnet Ziffern, `.` und `-` als Segmente und
  alle übrigen Zeichen („KW“, „Week“) im Font 6x10. Ohne NTP-Zeit steht dort normaler Text.
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
| Oszilloskop-Profil | DIGITAL: harte Sprünge, sonst zeigt ein abtastendes Oszilloskop Zwischenpunkte. ANALOG: gebremste Sprünge (siehe nächste Zeile). Umschalten im Betrieb per BOOT-Taster (3 s) oder im Webinterface, gespeichert im NVS (`Preferences`, Namensraum `scopeclock`) | – |
| Sprünge (ANALOG) | in 1-Einheit-Schritten mit 24-facher Geschwindigkeit (`JUMP_SPEED`), auf den letzten 4 Einheiten bremsend (`JUMP_LANDING`, `JUMP_RAMP`). Die Rücklaufspur hat ca. 1/24 der Linienhelligkeit, alte Verstärker landen trotzdem genau | harter Sprung |
| Reihenfolge | Zifferblatt und Ziffern werden beim Start einmal zu einer Rundtour mit minimalen Sprüngen sortiert (Nächster-Nachbar + 2-opt), Einstieg dort, wo der Sekundenzeiger endet | Tabellenreihenfolge |
| Sekundenzeiger | hin und zurück (Mitte ↔ Spitze) mit je halber Helligkeit gezeichnet, dadurch entfällt der lange Sprung vom Rand zur Mitte | – |
| Richtung | jeder Linienzug wird vom näheren Ende aus gezeichnet | immer vom Tabellenanfang |
| Dunkeltastung | optional über den Z-Eingang des Oszilloskops (drittes Kabel an `ZBLANK_PIN` = GPIO27, im Webinterface einschalten), dann gibt es gar keine Rücklaufspur mehr | – |
| Zeiger-Nadeln | Stunden- und Minutenzeiger mit `HAND_BRIGHT_PCT` (67 %), weil ihre zwei dicht liegenden Kanten verschmelzen | doppelt so hell |
| Tabellengrenzen | immer geprüft | Lesezugriff hinter das Tabellenende, wenn ein Endzeichen fehlt |

Ergebnis der Simulation (Leuchtspur mit ca. 2,4 Einheiten Strahldurchmesser, Helligkeit relativ
zu den Zifferblatt-Strichen, mehrere Uhrzeiten; gemessen mit den früheren Vektordaten und
67 % Zeigerhelligkeit – für die neuen Grafiken noch nicht wiederholt):

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
  Sekundenzeiger 5 Byte pro Stellung (Spitze, Mitte), Minuten- und Stundenzeiger 9 Byte
  (schmale Nadel aus 4 Punkten).
  Der Offset `Stellung × Länge` wählt die Stellung aus. Der Stundenzeiger rückt alle 12 Minuten
  einen Schritt weiter: `hpos = (h % 12) * 5 + m / 12`.
- Alle Tabellen erzeugt `tools/gen_vectors.py`: Zifferblatt, Ziffern (Schrift wählbar mit
  `--font segment|kante|raleway`, Standard 7-Segment) und Zeiger. Formen ändern = Skript anpassen
  und `python3 tools/gen_vectors.py > ESP32_OscilloscopeClock_v2.0/DataTable.h` ausführen.
- Eigene Motive lassen sich als weitere Tabelle im selben Format ergänzen und im `scopeTask`
  mit `PlotTable(Tabelle, sizeof(Tabelle), 0x00, 0)` zeichnen. Zum Prüfen ohne Oszilloskop
  dient der `EXCEL`-Modus in `config.h`: Er gibt alle Punkte seriell aus, die Liste lässt sich
  als XY-Diagramm darstellen.

## Feineinstellung

| Symptom | Stellschraube (`config.h` oder Webinterface) |
|---|---|
| Bild flimmert | `BEAM_NS_PER_UNIT` verringern (z. B. 2000) |
| Linien zu dunkel | `BEAM_NS_PER_UNIT` erhöhen (z. B. 5000) oder Intensität am Oszilloskop erhöhen |
| Ecken verrundet, Ziffern verwaschen (Oszilloskop zu langsam) | `BEAM_NS_PER_UNIT` erhöhen |
| Linienanfänge mit kleinen „Haken“ | `JUMP_SPEED` (z. B. 12) oder `JUMP_LANDING` (z. B. 4) verringern |
| Rücklaufspuren zwischen den Linien sichtbar | `JUMP_SPEED` erhöhen (32), `0` (harter Sprung, für schnelle Oszilloskope) oder Dunkeltastung über den Z-Eingang nutzen (drittes Kabel an GPIO27, Einstellungen → Oszilloskop) |
| Zeiger heller/dunkler als das Zifferblatt | `HAND_BRIGHT_PCT` anpassen |
| Zeit falsch um ganze Stunden | Zeitzone im Webinterface bzw. `TZ_INFO` prüfen |
| Kein Internet, Uhrzeit läuft ab 10:08:37 (Startzeit `FALLBACK_H:M:S`) | Webinterface → Einstellungen → „Uhrzeit stellen“ |
| OLED-Bild 2 px versetzt | in der `.ino` `NONAME0` → `NONAME2` |
| OLED bleibt dunkel | Im Status „OLED-Anzeige“ prüfen: steht dort „Aus“, ist die Helligkeit 0 %. Steht dort „An“, Verdrahtung prüfen (die Uhr kann per SPI nicht erkennen, ob ein Display angeschlossen ist): CLK GPIO18, DIN GPIO23, CS GPIO5, DC GPIO17, RST GPIO4, VCC an 3V3 (siehe HARDWARE.md) |
