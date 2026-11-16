# Hardware & Aufbau – ESP32 Oscilloscope Clock v2.0

**Deutsch** · [English](HARDWARE.en.md) · zurück zur [README](README.md)

## Komponenten

| Teil | Zweck |
|---|---|
| ESP32-WROOM-32U DevKitC V4 (USB-C, IPEX/U.FL) | Klassischer Dual-Core-ESP32 mit zwei 8-Bit-DACs (GPIO25/26) |
| Waveshare 2.42" OLED (SSD1309, 128×64) | Lokale Textanzeige: Uhrzeit 24h (7-Segment-Stil), Datum, KW, SSID, Signal |
| 2,4-GHz-Stabantenne + IPEX→SMA-Pigtail | Pflicht beim *-32U*: das Modul hat **keine** PCB-Antenne |
| 2× BNC-Einbaubuchse (Lötanschluss) | X- und Y-Ausgang zum Oszilloskop |
| ABS-Abzweigdose 100 × 150 mm | Gehäuse (funkdurchlässig) |
| Kleinteile | 2× 220 Ω (Serienwiderstand DAC), optional 2× 1 nF, Litze, Abstandsbolzen |

## Verdrahtung

```
ESP32 DevKitC            Ziel
─────────────            ──────────────────────────────────────────────
GPIO25 (DAC1) ──[220Ω]── BNC "X"  Mittelkontakt   ──> Oszilloskop Kanal 1
GPIO26 (DAC2) ──[220Ω]── BNC "Y"  Mittelkontakt   ──> Oszilloskop Kanal 2
GND ──────────────────── BNC "X" + "Y" Masse (Gehäuse der Buchse)

3V3  ─────────────────── OLED VCC
GND  ─────────────────── OLED GND
GPIO18 (VSPI SCK) ────── OLED CLK
GPIO23 (VSPI MOSI) ───── OLED DIN
GPIO5  ───────────────── OLED CS
GPIO17 ───────────────── OLED DC
GPIO4  ───────────────── OLED RST

IPEX/U.FL ── Pigtail ── SMA-Buchse in der Gehäusewand ── Stabantenne

Optional, Dunkeltastung:
GPIO27 ──[220Ω]──────── BNC "Z"  Mittelkontakt   ──> Z-Eingang des Oszilloskops (Rückseite)
GND ──────────────────── BNC "Z" Masse
```

Die Pins stehen zentral in `ESP32_OscilloscopeClock_v2.0/config.h`. Fest vorgegeben sind GPIO25/26
(nur dort liegen die DACs) und GPIO18/23 (Hardware-SPI für das Display).

### Pin-Positionen am DevKitC (gezählt von oben, Antennenbuchse oben, USB-C unten)

| Leiste | Pos. | Pin | Anschluss |
|---|---|---|---|
| links | 1 | 3V3 | OLED VCC |
| links | 9 | GPIO25 (DAC_1) | über 220 Ω → BNC „X“ Mitte |
| links | 10 | GPIO26 (DAC_2) | über 220 Ω → BNC „Y“ Mitte |
| links | 11 | GPIO27 | optional über 220 Ω → BNC „Z“ Mitte |
| links | 14 | GND | Masse aller BNC-Buchsen |
| rechts | 1 | GND | OLED GND |
| rechts | 2 | GPIO23 | OLED DIN |
| rechts | 9 | GPIO18 | OLED CLK |
| rechts | 10 | GPIO5 | OLED CS |
| rechts | 11 | GPIO17 | OLED DC |
| rechts | 13 | GPIO4 | OLED RST |

GPIO0 (rechts, Pos. 14) ist der BOOT-Taster auf dem Board – dort nichts anschließen.

### Optional: Dunkeltastung über den Z-Eingang
Der ESP32 kann den Strahl nicht selbst abschalten. Zwischen zwei Linien bleibt deshalb eine sehr
schwache Rücklaufspur (die Firmware hält sie mit ca. 1/24 der Linienhelligkeit so klein wie möglich).
Viele Röhrenoszilloskope haben auf der Rückseite eine BNC-Buchse **„Z“, „Z-Mod“, „Z-Input“** oder
**„Intensity“**. Mit einem dritten Ausgang lässt sich der Strahl während der Sprünge ganz ausschalten:
1. Dritte BNC-Buchse ins Gehäuse, Verdrahtung wie oben (GPIO27 über 220 Ω).
2. Im Handbuch des Oszilloskops nachsehen, welcher Pegel **dunkel** schaltet. Bei vielen Geräten macht
   High (TTL) dunkel. Bei manchen ist es umgekehrt, oder der Eingang braucht mehr als 3,3 V.
3. In `config.h` `ZBLANK_PIN 27` setzen, bei Bedarf `ZBLANK_ACTIVE_HIGH 0`.
4. Bleibt das Bild ganz dunkel, ist die Polarität falsch. Zeigen sich Lichtpunkte an den
   Linienanfängen, `ZBLANK_SETTLE_NS` erhöhen.

### Hinweise zu den DAC-Ausgängen
- Ausgangsbereich ca. 0 … 3,2 V, Bildmitte ca. 1,6 V.
- **220 Ω in Serie** schützen den DAC bei Kurzschluss am BNC-Kabel und dämpfen Schwingneigung
  durch die Kabelkapazität.
- Optional **1 nF vom BNC-Mittelkontakt nach GND**: so entsteht ein RC-Tiefpass (≈ 720 kHz),
  der die DAC-Stufen glättet. Wenn Ziffern dadurch „verrundet“ aussehen, den Kondensator weglassen.
- Kein Pegelwandler und kein Verstärker nötig: das Oszilloskop hat 1 MΩ Eingangswiderstand.

### OLED-Anschluss (Waveshare 2.42")
Das Modul wird im **Auslieferungszustand (4-Wire-SPI)** betrieben, es muss nichts umgelötet werden.
Der 7-polige Stecker wird wie oben verdrahtet (VCC an 3V3, nicht an 5V0). Ist das Bild um 2 Pixel
horizontal verschoben, in der `.ino` `NONAME0` durch `NONAME2` ersetzen. Ist das Display kopfüber
eingebaut, im Webinterface „OLED um 180° drehen“ einschalten.

### Antenne
- Den ESP32-**WROOM-32U** nie ohne angeschlossene Antenne im WLAN-Betrieb laufen lassen.
- Die SMA-Buchse des Pigtails in die Gehäusewand schrauben. Die Antenne zeigt nach oben und sitzt
  möglichst weit weg von den BNC-Kabeln.

### Stromversorgung & Masse
- Versorgung über USB-C (5 V). Der Regler auf dem DevKit liefert 3,3 V, die auch für das OLED reichen.
- **Brummschleife vermeiden:** Die BNC-Masse des Oszilloskops liegt am Schutzleiter. Ein ebenfalls
  geerdeter PC am USB kann Brumm und Zittern ins Bild bringen. Für den Dauerbetrieb ein normales
  (ungeerdetes) USB-Steckernetzteil verwenden.
- WLAN-Sendepulse ziehen kurzzeitig Strom. Leichtes Zittern im Bild lässt sich mit einem Elko
  (z. B. 100 µF) zwischen 3V3 und GND nahe am DevKit reduzieren.

## Gehäuse (ABS 100 × 150 mm) – Vorschlag
- Stirnseite: 2× BNC (X, Y), Abstand ≥ 20 mm, beschriften.
- Rückseite: USB-C-Ausschnitt für das DevKit **oder** eine USB-C-Einbaubuchse mit kurzem Kabel.
- Deckel: Fenster für das OLED (aktive Fläche ca. 55 × 28 mm), Modul mit M2/M2,5-Abstandsbolzen befestigen.
- Seite/Deckel: SMA-Bohrung (6,5 mm) für die Antenne.
- DevKit auf Abstandsbolzen oder Lochrasterplatine. Die Leitungen GPIO25/26 → BNC kurz halten.

## Oszilloskop-Einstellungen

Die Firmware kennt zwei Oszilloskop-Profile. Umgeschaltet wird im Betrieb mit dem **BOOT-Taster** des
ESP32: **3 s halten**. Das OLED zeigt dabei den Wechsel (z. B. „DIGITAL > ANALOG“) mit Countdown 3, 2, 1.
Wer vorher loslässt, bricht ab. Die Einstellung wird im Flash gespeichert und gilt auch nach einem Neustart.
Ein neues Hochladen ist nicht nötig. Den Taster **nicht beim Einschalten** drücken, sonst
startet der ESP32 im Download-Modus. Beim Umschalten kann einmal kurz ein Lichtpunkt auf dem Oszilloskop
aufblitzen (Schreibzugriff auf den Flash).
Das Profil lässt sich auch im Webinterface umschalten („Einstellungen“ → „Oszilloskop-Profil“).

**Testbild:** „Einstellungen“ → „Oszilloskop“ → „Testbild“ zeigt statt der Uhr einen Rahmen mit
Marken, einen Kreis und ein Mittelkreuz. Damit lassen sich Position (Kreuz in der Bildmitte), Größe
(Rahmen füllt 8 × 8 Teilungen fast aus), gleiche Verstärkung von X und Y (der Kreis ist rund, nicht oval)
und Fokus einstellen. Das Testbild wird nicht gespeichert, nach einem Neustart läuft wieder die Uhr.

| Profil | für | Sprünge zwischen Linien |
|---|---|---|
| DIGITAL (ab Werk) | digitale Oszilloskope, Speicher-Oszilloskope | hart, damit keine Abtastpunkte unterwegs entstehen |
| ANALOG | analoge Röhren-Oszilloskope | schnell und vor dem Ziel gebremst, schwache Rücklaufspur |

### Analoges Röhren-Oszilloskop (Profil ANALOG)
- Betriebsart **X/Y**, beide Kanäle **DC-Kopplung**.
- Kanal 1 (X) und Kanal 2 (Y): **0,5 V/Div**. Bei 8 Divisions ergibt das 4 V, das 3,2-V-Bild passt also hinein.
- Mit den Position-Reglern auf Bildmitte schieben (DAC-Mitte ≈ 1,6 V).
- Helligkeit/Intensität eher gering, Fokus nachstellen.

### Digitales Oszilloskop (Profil DIGITAL, ab Werk)
Ein digitales Oszilloskop zeigt im X/Y-Modus nur die Abtastwerte **einer Aufnahme**. Damit die ganze Uhr
zu sehen ist, muss das Aufnahmefenster (10 × Zeitbasis) mindestens ein Uhrenbild abdecken.
Ein Bild dauert ca. 4,3 ms, mit römischen Ziffern ca. 6,2 ms.

| Einstellung | Wert | Grund |
|---|---|---|
| Kopplung | DC, beide Kanäle | Die Signale haben einen Gleichanteil (0–3,2 V) |
| Empfindlichkeit | 0,5 V/Div, beide Kanäle gleich | sonst wird die Uhr verzerrt |
| Offset/Position | auf Bildmitte, ca. −1,6 V bzw. −3,2 Div | DAC-Mitte ≈ 1,6 V |
| Zeitbasis | **1 ms/Div** (Fenster 10 ms ≈ 2 Bilder) | ganze Uhr in jeder Aufnahme |
| Trigger | **Auto** (freilaufend) | Die Uhr liefert kein Triggerereignis |
| Nachleuchten (Persistence) | kurz bis mittel, falls vorhanden | glättet den Bildaufbau |
| Bandbreitenbegrenzung | ein (z. B. 20 MHz) | weniger Rauschen, die Signale sind langsam |

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

## Feinabstimmung in `config.h`
Profil, Zifferblatt, Verweilzeit des Strahls, Helligkeit von Zeigern, Minutenstrichen und Ziffern, Rücklauf-Tempo, Zeitzone, Zeitserver (NTP)
und OLED lassen sich bequemer im **Webinterface unter „Einstellungen“** ändern, mit sofortiger Wirkung
am Oszilloskop. Die Werte in `config.h` sind dann nur die Startwerte.

| Parameter | Wirkung |
|---|---|
| `BEAM_NS_PER_UNIT` | Verweilzeit des Strahls pro Schritt in ns. Größer = hellere Linien, weniger Bilder/s (Standard 3000 ≈ 230 Bilder/s) |
| `SCOPE_PROFILE` | Start-Profil, solange keins gespeichert ist: 0 = ANALOG, 1 = DIGITAL (Standard). Im Betrieb per BOOT-Taster umschaltbar |
| `BUTTON_LONG_MS` | Haltedauer des BOOT-Tasters zum Umschalten (Standard 3000 ms) |
| `JUMP_SPEED` | Geschwindigkeit der Sprünge zwischen Linien im Profil ANALOG (Standard 24; DIGITAL springt immer hart). Größer = schwächere Rücklaufspur, kleiner (12) für sehr alte Oszilloskope, 0 = harter Sprung |
| `JUMP_LANDING`, `JUMP_RAMP` | Abbremsen vor dem Linienanfang (Standard 8 und 4). Kleinere `JUMP_LANDING` bei „Haken“ an Linienanfängen |
| `ZBLANK_PIN` | optionaler Ausgang zur Dunkeltastung (Z-Eingang), -1 = aus |
| `HAND_BRIGHT_PCT` | Helligkeit von Stunden- und Minutenzeiger im Verhältnis zum Rest (Standard 67 %, wirkt dann gleich hell) |
| `TICK_BRIGHT_PCT`, `DIGIT_BRIGHT_PCT` | Helligkeit von Minutenstrichen und Ziffern, 0–100 % (Standard 100 %, 0 = ausgeblendet). Im Webinterface regelbar |
| `OLED_BRIGHT_PCT` | OLED-Helligkeit 0–100 % (Standard 50 %, 0 = aus) |
| `OLED_SECONDS` | 1 = OLED-Uhrzeit mit Sekunden, 0 = nur HH:MM |
| `USE_FAST_DAC` | `1` = direkte Registerzugriffe (Standard). `0` = `dacWrite()`, nur als Notlösung |
| `DIAL_STYLE` | Start-Zifferblatt: 0 = 1–12, 1 = 12/3/6/9, 2 = römisch I–XII, 3 = römisch XII/III/VI/IX, 4 = nur Striche (ohne Zahlen). Im Webinterface durch Wischen über die Vorschau umschaltbar, die Auswahl wird gespeichert |
| `TZ_INFO` | Zeitzone inkl. Sommerzeitregel (Standard: Mitteleuropa) |
| `NTP_SERVER_1` … `_3` | Zeitserver (der erste ist im Webinterface änderbar) |
| `FALLBACK_H`, `FALLBACK_M`, `FALLBACK_S` | Startzeit, ab der die Uhr zählt, solange weder Zeitserver noch Handeingabe eine Uhrzeit geliefert haben |
| `OLED_SHIFT_EVERY_S` | Pixelverschiebung gegen OLED-Einbrennen: alle n Sekunden (0 = aus) |
| `OLED_FLIP` | 1 = OLED-Anzeige um 180° drehen (Display kopfüber eingebaut), auch im Webinterface umschaltbar |
| `CLOCK_12H` | Zeitformat für OLED und Webinterface: 0 = 24 Stunden (Standard), 1 = 12 Stunden mit AM/PM. Im Webinterface umschaltbar |
| `WEB_PASSWORD_CLEAR` | `1` löscht beim Start ein vergessenes Web-Passwort (einmal mit 1 hochladen, danach wieder 0). Ab Werk ist kein Passwort gesetzt |
| `UI_LANG` | Sprache für Webinterface, OLED und Protokoll: 0 = Deutsch (Standard), 1 = English. Im Webinterface umschaltbar |
