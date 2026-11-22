# Hardware & Aufbau – ESP32 Oscilloscope Clock v2.0

**Deutsch** · [English](HARDWARE.en.md) · zurück zur [README](README.md)

## Komponenten

| Teil | Zweck |
|---|---|
| [ESP32-WROOM-32U DevKitC V4](https://www.amazon.de/dp/B0F65G3V81), USB-C, mit Antennen-Kit (DUBEUYEW), 48,5 × 28,5 mm | Klassischer Dual-Core-ESP32 mit zwei 8-Bit-DACs (GPIO25/26); das Kit enthält das IPEX→SMA-Pigtail |
| [Waveshare 2,42inch OLED Display Module](https://www.amazon.de/dp/B0CL6WMHJN), weiß (SSD1309, 128×64, SPI/I²C), 61,5 × 39,5 mm – **optional**, im Prototyp verbaut | Lokale Textanzeige (die Uhr läuft auch ohne): Uhrzeit 24h und Datum/KW (7-Segment-Stil), SSID, Signal |
| [WLAN-Antenne Bingfu](https://www.amazon.de/dp/B086JCJMSN) 2,4/5,8 GHz, 8 dBi, SMA-Stecker | Pflicht beim *-32U*: das Modul hat **keine** PCB-Antenne; SMA und RP-SMA nicht mischen |
| [ESP32-Breakout-Board mit Schraubklemmen](https://www.amazon.de/dp/B0BCWBW4SR) | DevKit aufstecken, alle Leitungen in Schraubklemmen (Beschriftung P25, P26 …) – nichts am DevKit löten |
| 2× [BNC-Pigtail](https://de.aliexpress.com/item/1005006318978533.html), BNC-Stecker auf 2,54-mm-DuPont-Stift, RG316 | X- und Y-Ausgang; der BNC-Stecker kommt direkt ins Oszilloskop (rot = Signal, braun = Schirm). Die DuPont-Stecker werden abgeschnitten |
| 2× [Kabelverschraubung Lapp SKINDICHT MINI](https://de.rs-online.com/web/p/kabelverschraubungen/4583953) M6×1 (52001860), Klemmbereich 2–3 mm | führt die Pigtails durch die Gehäusewand |
| [USB-C-Einbauverlängerung](https://www.amazon.de/dp/B0FM7RLQTF) (IP67, 0,2 m, Einbauöffnung 16 mm) | Versorgung und Programmierung von außen |
| [ABS-Abzweigdose 150 × 100 × 70 mm](https://www.amazon.de/dp/B0B5RGNNXV), IP67, Scharnierdeckel, Montageplatte | Gehäuse (funkdurchlässig) |
| Kleinteile | 2× 220 Ω (an die rote Ader der Pigtails), Aderendhülsen (optional), Abstandsbolzen M2,5/M3 |

Ausführliche Aufbauanleitung mit Bohrplan: [docs/ESP32_Oscilloscope_Clock_Dokumentation.pdf](docs/ESP32_Oscilloscope_Clock_Dokumentation.pdf).

## Verdrahtung

```
Klemme (Breakout-Board)  Ziel
─────────────            ──────────────────────────────────────────────
P25 (DAC1) ──[220Ω]── Pigtail "X" rot (Innenleiter) ──> BNC-Stecker → Oszilloskop Kanal 1   (Spannung X-Ablenkung)
P26 (DAC2) ──[220Ω]── Pigtail "Y" rot (Innenleiter) ──> BNC-Stecker → Oszilloskop Kanal 2   (Spannung Y-Ablenkung)
GND ─────────────────── Pigtail "X" + "Y" braun (Schirm)                                       (Masse, Bezug)

3V3  ─────────────────── OLED VCC    Versorgung 3,3 V
GND  ─────────────────── OLED GND    Masse
GPIO18 (VSPI SCK) ────── OLED CLK    SPI-Takt
GPIO23 (VSPI MOSI) ───── OLED DIN    SPI-Daten (Bildinhalt)
GPIO5  ───────────────── OLED CS     Chip-Auswahl (low = aktiv)
GPIO17 ───────────────── OLED DC     high = Bilddaten, low = Befehl
GPIO4  ───────────────── OLED RST    Reset (low-aktiv)

optional – Echtzeituhr DS3231 (ZS-042):
3V3  ─────────────────── RTC VCC     Versorgung 3,3 V
GND  ─────────────────── RTC GND     Masse
GPIO21 (I²C SDA) ─────── RTC SDA     I²C-Daten
GPIO22 (I²C SCL) ─────── RTC SCL     I²C-Takt

IPEX/U.FL ── Pigtail (Antennen-Kit) ── SMA-Buchse in der gegenüberliegenden Seite ── Bingfu-Antenne (SMA)
```

Die Pins stehen zentral in `ESP32_OscilloscopeClock_v2.0/config.h`. Fest vorgegeben sind GPIO25/26
(nur dort liegen die DACs) und GPIO18/23 (Hardware-SPI für das Display).

### Pin-Positionen am DevKitC (gezählt von oben, Antennenbuchse oben, USB-C unten)

| Leiste | Pos. | Pin | Anschluss |
|---|---|---|---|
| links | 1 | 3V3 | OLED VCC |
| links | 9 | GPIO25 (DAC_1) | über 220 Ω → BNC „X“ Mitte |
| links | 10 | GPIO26 (DAC_2) | über 220 Ω → BNC „Y“ Mitte |
| links | 14 | GND | braune Adern (Schirm) aller Pigtails |
| rechts | 1 | GND | OLED GND |
| rechts | 2 | GPIO23 | OLED DIN |
| rechts | 9 | GPIO18 | OLED CLK |
| rechts | 10 | GPIO5 | OLED CS |
| rechts | 11 | GPIO17 | OLED DC |
| rechts | 13 | GPIO4 | OLED RST |
| rechts | 3 | GPIO22 | optional: RTC SCL |
| rechts | 6 | GPIO21 | optional: RTC SDA |
| rechts | 7 | GND | optional: RTC GND (VCC der RTC an 3V3, links Pos. 1) |

GPIO0 (rechts, Pos. 14) ist der BOOT-Taster auf dem Board – dort nichts anschließen.

### Hinweise zu den DAC-Ausgängen
- Ausgangsbereich ca. 0 … 3,2 V, Bildmitte ca. 1,6 V.
- **220 Ω in Serie** schützen den DAC bei Kurzschluss am BNC-Kabel und dämpfen Schwingneigung
  durch die Kabelkapazität.
- Optional **1 nF vom BNC-Mittelkontakt nach GND**: so entsteht ein RC-Tiefpass (≈ 720 kHz),
  der die DAC-Stufen glättet. Wenn Ziffern dadurch „verrundet“ aussehen, den Kondensator weglassen.
- Kein Pegelwandler und kein Verstärker nötig: das Oszilloskop hat 1 MΩ Eingangswiderstand.

### OLED-Anschluss (Waveshare 2.42", optional)
Das Modul wird im **Auslieferungszustand (4-Wire-SPI)** betrieben, es muss nichts umgelötet werden.
Der 7-polige Stecker wird wie oben verdrahtet (VCC an 3V3, nicht an 5V0). Ist das Bild um 2 Pixel
horizontal verschoben, in der `.ino` `NONAME0` durch `NONAME2` ersetzen. Ist das Display kopfüber
eingebaut, im Webinterface „OLED um 180° drehen“ einschalten.

### Antenne
- Den ESP32-**WROOM-32U** nie ohne angeschlossene Antenne im WLAN-Betrieb laufen lassen.
- Die SMA-Buchse des Pigtails in die Gehäusewand schrauben. Die Antenne zeigt nach oben und sitzt
  möglichst weit weg von den BNC-Kabeln.

### Optional: Echtzeituhr DS3231 (z. B. Modul ZS-042)
Hält Datum und Uhrzeit mit Knopfzelle auch nach einem Stromausfall ohne Internet (DS3231: ±2 ppm, etwa
1 Minute pro Jahr). Im beschriebenen Aufbau nicht enthalten. Passendes Modul: [WINGONEER „Tiny DS3231 AT24C32“](https://www.amazon.de/dp/B01H5NAFUY).

| Modul | Breakout-Board |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | P21 |
| SCL | P22 |
| 32K, SQW | nicht anschließen |

Die 6-polige und die 4-polige Stiftleiste des Moduls sind durchverbunden – jede Leitung kann an
der Leiste angeschlossen werden, die besser passt.

- Einschalten im Webinterface: „Einstellungen“ → „Uhrzeit“ → „Echtzeituhr (RTC)“ (standardmäßig aus). Der Status
  zeigt dann „Läuft“, „Nicht gefunden“ oder „Zeit ungültig“.
- **Knopfzelle: normale CR2032**, kein Ladegerät und kein Löten nötig. Die Ladeschaltung des ZS-042
  (Diode und 200 Ω) ist für 5 V gedacht. An **3V3** kommen nach der Diode nur etwa 2,7 V an – weniger als
  die Zellspannung, es fließt praktisch kein Ladestrom. Die Zelle hält 5 Jahre und mehr, denn sie wird nur
  bei ausgesteckter Uhr genutzt (ca. 3 µA).
- **Modul nur an 3V3 anschließen, nicht an 5 V** – an 5 V würde die CR2032 geladen (dann die Diode
  auslöten). Eine aufladbare LIR2032 ist hier ungeeignet: An 3V3 würde sie nie geladen.
- Auf dem großen Chip sollte **DS3231SN** stehen – sehr billige Module haben manchmal ungenauere Nachbauten.
- Die RTC läuft in UTC; Zeitzone und Sommerzeit rechnet weiterhin die Firmware. Bei jedem NTP-Abgleich
  (stündlich) und beim Stellen von Hand wird sie nachgestellt.

### Stromversorgung & Masse
- Versorgung über USB-C (5 V). Der Regler auf dem DevKit liefert 3,3 V, die auch für das OLED reichen.
- **Brummschleife vermeiden:** Die BNC-Masse des Oszilloskops liegt am Schutzleiter. Ein ebenfalls
  geerdeter PC am USB kann Brumm und Zittern ins Bild bringen. Für den Dauerbetrieb ein normales
  (ungeerdetes) USB-Steckernetzteil verwenden.

## Gehäuse (Abzweigdose 150 × 100 × 70 mm)
- **Anschlussseite** (kurze Seite, 100 mm), von außen gesehen: Pigtail X bei 28 mm, **mittig die USB-C-Einbaubuchse** (Ø 16 mm bei 50 mm), Pigtail Y bei 72 mm. Kabelverschraubungen M6: Ø 5,0 bohren und M6-Gewinde schneiden (oder Ø 6,2 + Mutter).
- **Gegenüberliegende Seite:** Antenne (SMA, Ø 6,5 mm, mittig).
- DevKit richtig herum aufgesteckt (Pin „3V3“ an Klemme „3V3“), das Breakout-Board **samt DevKit um 180° gedreht** eingebaut (Modul zur Anschlussseite, USB-C zur Antennenseite, P25/P26 rechts). Das DevKit nie verdreht aufstecken, sonst stimmen die Klemmenaufdrucke nicht; Antennenkabel und USB-Flachbandkabel laufen einmal unter dem Board durch zur jeweils gegenüberliegenden Seite.
- Deckel: Fenster für das OLED (aktive Fläche ca. 55 × 28 mm), Modul mit M2,5-Abstandsbolzen befestigen.
- Pigtails: die DuPont-Stecker abschneiden (sie passen nicht durch die Verschraubung), das Kabel von außen durch die
  Kabelverschraubung führen, erst dann innen abisolieren und den 220-Ω-Widerstand an die rote Ader löten – mit Widerstand
  und Schrumpfschlauch passt das Kabel nicht mehr durch. Das freie Widerstandsbein und die braune Ader kommen in die Klemmen.
- Breakout-Board auf der Montageplatte; die Pigtails in die Klemmen P25/P26 (rot, über 220 Ω) und GND (braun).
- Bohrplan 1:1 zum Ausdrucken auf DIN A4: letzte Seite von `docs/ESP32_Oscilloscope_Clock_Dokumentation.pdf`.

## Oszilloskop-Einstellungen

Die Firmware kennt zwei Oszilloskop-Profile. Umgeschaltet wird im Betrieb mit dem **BOOT-Taster** des
ESP32: **3 s halten**. Das OLED zeigt dabei den Wechsel (z. B. „DIGITAL > ANALOG“) mit Countdown 3, 2, 1.
Wer vorher loslässt, bricht ab. Die Einstellung wird im Flash gespeichert und gilt auch nach einem Neustart.
Ein neues Hochladen ist nicht nötig. Den Taster **nicht beim Einschalten** drücken, sonst
startet der ESP32 im Download-Modus. Beim Umschalten kann einmal kurz ein Lichtpunkt auf dem Oszilloskop
aufblitzen (Schreibzugriff auf den Flash).
Das Profil lässt sich auch im Webinterface umschalten („Einstellungen“ → „Oszilloskop-Profil“).

**Werkseinstellungen:** Den BOOT-Taster **10 s halten**. Nach 3 s wechselt zunächst das Profil, ab 5 s zählt
das OLED 5 … 1 herunter. Dann werden alle Einstellungen und gespeicherten WLANs gelöscht und die Uhr startet
neu (danach Setup-WLAN „ScopeClock-Setup“). Vorher loslassen bricht ab.

**Optional – Dunkeltastung über den Z-Eingang:** Hat das Oszilloskop einen Z-Eingang („Z“, „Z-Mod“,
„Intensity“), kann ein drittes Kabel von **GPIO27** (Klemme P27) dorthin die Rücklaufspuren ganz
ausblenden. Im beschriebenen Aufbau ist es nicht vorhanden. Eingeschaltet wird es im Webinterface
(„Einstellungen“ → „Oszilloskop“ → „Dunkeltastung (Z-Eingang)“), dort auch der Pegel (High oder Low = dunkel,
siehe Handbuch des Oszilloskops). Ausgeschaltet ist GPIO27 hochohmig.

**Testbild:** „Einstellungen“ → „Oszilloskop“ → „Testbild“ zeigt statt der Uhr einen Rahmen mit
Marken, einen Kreis und ein Mittelkreuz. Damit lassen sich Position (Kreuz in der Bildmitte), Größe
(Rahmen füllt 8 × 8 Teilungen fast aus), gleiche Verstärkung von X und Y (der Kreis ist rund, nicht oval)
und Fokus einstellen. Das Testbild wird nicht gespeichert. **Nach jedem Einschalten erscheint es
automatisch für 10 s** (`BOOT_TEST_S`), danach läuft die Uhr – so sieht man gleich, ob alles sitzt.


| Profil | für | Sprünge zwischen Linien |
|---|---|---|
| DIGITAL (Standard) | digitale Oszilloskope, Speicher-Oszilloskope | hart, damit keine Abtastpunkte unterwegs entstehen |
| ANALOG | analoge Röhren-Oszilloskope | schnell und vor dem Ziel gebremst, schwache Rücklaufspur |

### Analoges Röhren-Oszilloskop (Profil ANALOG)
- Betriebsart **X/Y**, beide Kanäle **DC-Kopplung**.
- Kanal 1 (X) und Kanal 2 (Y): **0,5 V/Div**. Bei 8 Divisions ergibt das 4 V, das 3,2-V-Bild passt also hinein.
- Mit den Position-Reglern auf Bildmitte schieben (DAC-Mitte ≈ 1,6 V).
- Helligkeit/Intensität eher gering, Fokus nachstellen.
- **Röhre schonen:** Im X/Y-Betrieb brennt ein stehendes Bild schnell in die Leuchtschicht ein. Intensität nur so hoch
  wie nötig, die **Bildverschiebung gegen Einbrennen** (Webinterface → Oszilloskop, standardmäßig an) eingeschaltet lassen und
  das Oszilloskop ausschalten, wenn niemand hinsieht. Abdunkeln kann die Firmware das Bild nicht – nur der
  Intensitätsregler oder die Dunkeltastung über den Z-Eingang.
- Statt der Pigtails Tastköpfe? Kurze Masseleitungen, 10:1-Tastköpfe vorher am Kalibrierausgang abgleichen.

### Digitales Oszilloskop (Profil DIGITAL, Standard)
Ein digitales Oszilloskop zeigt im X/Y-Modus nur die Abtastwerte **einer Aufnahme**. Damit die ganze Uhr
zu sehen ist, muss das Aufnahmefenster (10 × Zeitbasis) mindestens ein Uhrenbild abdecken.
Ein Bild dauert ca. 4,9 ms, mit römischen Ziffern ca. 7 ms.

**Einbrennen:** Digitale Oszilloskope mit LCD brennen nicht ein – die Bildverschiebung ist dort unnötig, schadet aber nicht.
Ältere digitale Geräte mit Bildröhre (z. B. HP 54200A) sollten sie eingeschaltet lassen.

| Einstellung | Wert | Grund |
|---|---|---|
| Kopplung | DC, beide Kanäle | Die Signale haben einen Gleichanteil (0–3,2 V) |
| Empfindlichkeit | 0,5 V/Div, beide Kanäle gleich | sonst wird die Uhr verzerrt |
| Offset/Position | auf Bildmitte, ca. −1,6 V bzw. −3,2 Div | DAC-Mitte ≈ 1,6 V |
| Zeitbasis | **1 ms/Div** (Fenster 10 ms ≈ 2 Bilder) | ganze Uhr in jeder Aufnahme |
| Trigger | **Auto** (freilaufend) | Die Uhr liefert kein Triggerereignis |
| Nachleuchten (Persistence) | kurz bis mittel, falls vorhanden | glättet den Bildaufbau |
| Bandbreitenbegrenzung | ein (z. B. 20 MHz) | weniger Rauschen, die Signale sind langsam |

**Beispiele für einzelne Geräte** – andere Oszilloskope werden sinngemäß eingestellt.

**Tektronix TDS3012**
- CH1 = X (GPIO25), CH2 = Y (GPIO26). XY-Darstellung im Display-Menü einschalten.
- Zeitbasis 1 ms/Div, Trigger Auto.
- Das TDS3000 stuft die Intensität nach Häufigkeit ab (Digital Phosphor). Gleichmäßige
  Strahlgeschwindigkeit wirkt dort wieder direkt als gleichmäßige Helligkeit.
- Kurzes Nachleuchten (Display → Persist) beruhigt das Bild.

**HP 54200A**
- Ob und wie das Gerät eine X/Y-Darstellung bietet, konnte ich nicht sicher klären. Bitte im
  Handbuch (Display-Menü) nachsehen.
- Gut für die Uhr: Das Gerät hat variables/unendliches Nachleuchten. Damit sammelt sich das Bild
  über mehrere Aufnahmen, auch wenn eine einzelne Aufnahme nur wenige Punkte hat.

**Digilent Analog Discovery 2**
- Die Eingänge sind **differenziell**: 1+ an GPIO25 (X), **1− an GND**; 2+ an GPIO26 (Y),
  **2− an GND**. Zusätzlich das GND-Kabel des AD2 an GND des ESP32.
- WaveForms → Scope: Range 500 mV/Div, Offset −1,6 V, Zeitbasis 1 ms/Div, Trigger Auto,
  XY-Ansicht einschalten (View → XY).
- Standardpuffer 8.192 Abtastwerte pro Kanal: Bei 10 ms Fenster ist das ca. 1 Wert pro 1,2 µs,
  also 2–3 Werte pro Schritt der Uhr. Mehr Punkte liefert die Gerätekonfiguration mit
  2 × 16k Scope-Puffer (WaveForms → Device Manager).
- Verbindet die XY-Ansicht die Abtastwerte mit Linien, sind auch die Sprünge als Linien zu sehen.
  Dann in den Ansichtsoptionen auf Punktdarstellung umstellen, falls angeboten.
- Beide Geräte hängen am selben PC, damit haben sie gemeinsame Masse, das ist unkritisch.

### Helligkeit der Zeiger abgleichen
Stunden- und Minutenzeiger sind schmale Nadeln aus zwei dicht liegenden Kanten und wirken je nach Oszilloskop heller als der
Rest. Der Regler „Helligkeit Stunden-/Minutenzeiger“ (Webinterface → Oszilloskop, standardmäßig 55 %) gleicht das aus. Richtwerte:

- **Analoge Röhren-Oszilloskope:** 55 %, bei sehr scharfem, dünnem Strahl bis 85 %
- **Digitale Oszilloskope mit Helligkeitsabstufung** (z. B. Tektronix TDS3000): 50–65 %
- **Geräte ohne Helligkeitsabstufung** (z. B. HP 54200A, Digilent Analog Discovery 2): 100 % (alle Punkte gleich hell)
- **Andere Geräte:** mit 55 % beginnen und abgleichen

**Genau einstellen:** erst Fokus und Intensität am Oszilloskop einstellen. Dann auf die Kreuzungen von Zeigern und
Minutenstrichen schauen und den Regler in 5-%-Schritten schieben, bis beide dort gleich hell wirken. Kurz auf 100 % und zurück
zeigt, in welche Richtung es geht. Der Wert wird sofort übernommen und gespeichert.

## Feinabstimmung in `config.h`
Profil, Zifferblatt, Verweilzeit des Strahls, Helligkeit von Zeigern, Minutenstrichen und Ziffern, Rücklauf-Tempo, Zeitzone, Zeitserver (NTP)
und OLED lassen sich bequemer im **Webinterface unter „Einstellungen“** ändern, mit sofortiger Wirkung
am Oszilloskop. Die Werte in `config.h` sind dann nur die Startwerte.

| Parameter | Wirkung |
|---|---|
| `BEAM_NS_PER_UNIT` | Verweilzeit des Strahls pro Schritt in ns. Größer = hellere Linien, weniger Bilder/s (Standard 3000 ≈ 205 Bilder/s) |
| `SCOPE_PROFILE` | Start-Profil, solange keins gespeichert ist: 0 = ANALOG, 1 = DIGITAL (Standard). Im Betrieb per BOOT-Taster umschaltbar |
| `BUTTON_LONG_MS` | Haltedauer des BOOT-Tasters zum Umschalten (Standard 3000 ms) |
| `BUTTON_FACTORY_MS` | Haltedauer für die Werkseinstellungen (Standard 10000 ms; die letzten 5 s Countdown auf dem OLED) |
| `JUMP_SPEED` | Geschwindigkeit der Sprünge zwischen Linien im Profil ANALOG (Standard 24; DIGITAL springt immer hart). Größer = schwächere Rücklaufspur, kleiner (12) für sehr alte Oszilloskope, 0 = harter Sprung |
| `JUMP_LANDING`, `JUMP_RAMP` | Abbremsen vor dem Linienanfang (Standard 8 und 4). Kleinere `JUMP_LANDING` bei „Haken“ an Linienanfängen |
| `ZBLANK_PIN`, `ZBLANK_ENABLE`, `ZBLANK_ACTIVE_HIGH` | Optionale Dunkeltastung über den Z-Eingang: GPIO (Standard 27), Startwert aus/an und Pegel. Im beschriebenen Aufbau nicht verdrahtet; ein- und ausschalten im Webinterface (Einstellungen → Oszilloskop) |
| `HAND_BRIGHT_PCT` | Helligkeit von Stunden- und Minutenzeiger im Verhältnis zum Rest (Standard 55 %; Richtwerte: Röhre 55–85 %, digital mit Helligkeitsabstufung 50–65 %, ohne Abstufung 100 %) |
| `TICK_BRIGHT_PCT`, `DIGIT_BRIGHT_PCT` | Helligkeit von Minutenstrichen und Ziffern, 0–100 % (Standard 100 %, 0 = ausgeblendet). Im Webinterface regelbar |
| `OLED_BRIGHT_PCT` | OLED-Helligkeit 0–100 % (Standard 50 %, 0 = aus) |
| `OLED_SECONDS` | 1 = OLED-Uhrzeit mit Sekunden, 0 = nur HH:MM |
| `USE_FAST_DAC` | `1` = direkte Registerzugriffe (Standard). `0` = `dacWrite()`, nur als Notlösung |
| `DIAL_STYLE` | Start-Zifferblatt: 0 = 1–12, 1 = 12/3/6/9, 2 = römisch I–XII, 3 = römisch XII/III/VI/IX, 4 = Bahnhof (Stundenbalken statt Zahlen). Im Webinterface durch Wischen über die Vorschau umschaltbar, die Auswahl wird gespeichert |
| `TZ_INFO` | Zeitzone inkl. Sommerzeitregel (Standard: Mitteleuropa) |
| `DIAL_AUTO_MIN` | Zifferblatt automatisch wechseln alle n Minuten (0 = aus, Standard; 1, 5, 15, 60); läuft nach dem Einschalten weiter |
| `SCOPE_SHIFT`, `SCOPE_SHIFT_EVERY_S` | Bildverschiebung gegen Einbrennen (Standard an, alle 60 s weiter) |
| `RTC_ENABLE`, `RTC_SDA_PIN`, `RTC_SCL_PIN` | Optionale Echtzeituhr DS3231: Startwert aus, I²C an GPIO21/22 (im Webinterface umschaltbar) |
| `NTP_SERVER_1` … `_3` | Zeitserver (der erste ist im Webinterface änderbar) |
| `FALLBACK_H`, `FALLBACK_M`, `FALLBACK_S` | Startzeit, ab der die Uhr zählt, solange weder Zeitserver noch Handeingabe eine Uhrzeit geliefert haben |
| `OLED_SHIFT_EVERY_S` | Pixelverschiebung gegen OLED-Einbrennen: alle n Sekunden (0 = aus) |
| `OLED_FLIP` | 1 = OLED-Anzeige um 180° drehen (Display kopfüber eingebaut), auch im Webinterface umschaltbar |
| `CLOCK_12H` | Zeitformat für OLED und Webinterface: 0 = 24 Stunden (Standard), 1 = 12 Stunden mit AM/PM. Im Webinterface umschaltbar |
| `BOOT_TEST_S` | Testbild nach dem Einschalten in Sekunden (Standard 10, 0 = aus) |
| `UI_LANG` | Sprache für Webinterface, OLED und Protokoll: 0 = Deutsch (Standard), 1 = English. Im Webinterface umschaltbar |
