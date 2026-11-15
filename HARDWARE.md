# Hardware & Aufbau – ESP32 Oscilloscope Clock v2.0

## Komponenten

| Teil | Zweck |
|---|---|
| ESP32-WROOM-32U DevKitC V4 (USB-C, IPEX/U.FL) | Klassischer Dual-Core-ESP32 mit zwei 8-Bit-DACs (GPIO25/26) |
| Waveshare 2.42" OLED (SSD1309, 128×64) | Lokale Textanzeige: Uhrzeit 24h (7-Segment-Stil), Datum, KW, SSID, Signal |
| 2,4-GHz-Stabantenne + IPEX→SMA-Pigtail | Pflicht beim *-32U*: das Modul hat **keine** PCB-Antenne |
| 2× BNC-Einbaubuchse (Lötanschluss) | X- und Y-Ausgang zum Oszilloskop |
| ABS-Abzweigdose 100 × 150 mm | Gehäuse (funkdurchlässig) |
| Kleinteile | 2× 220 Ω (Serienwiderstand DAC), optional 2× 1 nF, optional 2× 4,7 kΩ (I2C-Pull-ups), Litze, Abstandsbolzen |

## Verdrahtung

```
ESP32 DevKitC            Ziel
─────────────            ──────────────────────────────────────────────
GPIO25 (DAC1) ──[220Ω]── BNC "X"  Mittelkontakt   ──> Oszi Kanal 1
GPIO26 (DAC2) ──[220Ω]── BNC "Y"  Mittelkontakt   ──> Oszi Kanal 2
GND ──────────────────── BNC "X" + "Y" Masse (Gehäuse der Buchse)

3V3  ─────────────────── OLED VCC
GND  ─────────────────── OLED GND
GPIO21 (SDA) ─────────── OLED DIN   (im I2C-Modus = SDA)
GPIO22 (SCL) ─────────── OLED CLK   (im I2C-Modus = SCL)
GPIO4  ───────────────── OLED RES
GND  ─────────────────── OLED CS
GND  ─────────────────── OLED DC    (DC = GND → I2C-Adresse 0x3C, DC = 3V3 → 0x3D)

IPEX/U.FL ── Pigtail ── SMA-Buchse in der Gehäusewand ── Stabantenne

Optional, Dunkeltastung:
GPIO27 ──[220Ω]──────── BNC "Z"  Mittelkontakt   ──> Z-Eingang des Oszis (Rückseite)
GND ──────────────────── BNC "Z" Masse
```

Die Pins stehen zentral in `ESP32_OscilloscopeClock_v2.0/config.h`. Nur GPIO25/26 sind fest vorgegeben,
weil die DACs nur an diesen beiden Pins herausgeführt sind.

### Optional: Dunkeltastung über den Z-Eingang
Der ESP32 kann den Strahl nicht selbst abschalten. Zwischen zwei Linien bleibt deshalb eine sehr
schwache Rücklaufspur (die Firmware hält sie mit ca. 1/24 der Linienhelligkeit so klein wie möglich).
Viele Röhrenoszilloskope haben auf der Rückseite eine BNC-Buchse **„Z“, „Z-Mod“, „Z-Input“** oder
**„Intensity“**. Mit einem dritten Ausgang lässt sich der Strahl während der Sprünge ganz ausschalten:
1. Dritte BNC-Buchse ins Gehäuse, Verdrahtung wie oben (GPIO27 über 220 Ω).
2. Im Handbuch des Oszis nachsehen, welcher Pegel **dunkel** schaltet. Bei vielen Geräten macht
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

### OLED von SPI auf I2C umstellen (Waveshare 2.42")
Das Modul ist ab Werk auf 4-Wire-SPI eingestellt.
1. Auf der Rückseite die 0-Ω-Brücken entsprechend dem Platinenaufdruck von **SPI** auf **I2C**
   umlöten. Die genaue Brückenbelegung steht im Waveshare-Wiki („2.42inch OLED Module“); bitte
   die Platinenrevision dort vergleichen.
2. Im I2C-Modus: DIN = SDA, CLK = SCL, **CS auf GND**, **DC wählt die Adresse** (GND = 0x3C).
3. RES an GPIO4. Alternativ RES fest an 3V3 legen und in `config.h` `OLED_RST_PIN` auf
   `U8X8_PIN_NONE` setzen. Mit GPIO-Reset startet das Display aber zuverlässiger.
4. Bleibt das Display dunkel: prüfen, ob das Modul I2C-Pull-ups hat. Falls nicht,
   **4,7 kΩ von SDA und SCL nach 3V3** nachrüsten.
5. Ist das Bild um 2 Pixel horizontal verschoben, in der `.ino` `NONAME0` durch `NONAME2` ersetzen.

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

### Analoges Röhren-Oszi (`SCOPE_PROFILE 0`)
- Betriebsart **X/Y**, beide Kanäle **DC-Kopplung**.
- Kanal 1 (X) und Kanal 2 (Y): **0,5 V/Div**. Bei 8 Divisions ergibt das 4 V, das 3,2-V-Bild passt also hinein.
- Mit den Position-Reglern auf Bildmitte schieben (DAC-Mitte ≈ 1,6 V).
- Helligkeit/Intensität eher gering, Fokus nachstellen.

### Digitales Oszi / USB-Oszi (`SCOPE_PROFILE 1`, Standard)
Ein digitales Oszi zeigt im X/Y-Modus nur die Abtastwerte **einer Aufnahme**. Damit die ganze Uhr
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
| Parameter | Wirkung |
|---|---|
| `BEAM_NS_PER_UNIT` | Strahlgeschwindigkeit. Größer = hellere Linien, weniger Bilder/s (Standard 3000 ≈ 230 Bilder/s) |
| `SCOPE_PROFILE` | 0 = analoges Röhren-Oszi, 1 = digitales Oszi / USB-Oszi (Standard). Wählt die passende Sprungart |
| `JUMP_SPEED` | Geschwindigkeit der Sprünge zwischen Linien (analog 24, digital 0). Größer = schwächere Rücklaufspur, kleiner (12) für sehr alte Oszis, 0 = harter Sprung |
| `JUMP_LANDING`, `JUMP_RAMP` | Abbremsen vor dem Linienanfang (Standard 8 und 4). Kleinere `JUMP_LANDING` bei „Haken“ an Linienanfängen |
| `ZBLANK_PIN` | optionaler Ausgang zur Dunkeltastung (Z-Eingang), -1 = aus |
| `HAND_BRIGHT_PCT` | Helligkeit von Stunden- und Minutenzeiger im Verhältnis zum Rest (Standard 67 %, wirkt dann gleich hell) |
| `USE_FAST_DAC` | `1` = direkte Registerzugriffe (Standard). `0` = `dacWrite()`, nur als Notlösung |
| `DIAL_STYLE` | Zifferblatt: 0 = 1–12, 1 = 13–24, 2 = römisch |
| `TZ_INFO` | Zeitzone inkl. Sommerzeitregel (Standard: Mitteleuropa) |
| `OLED_SHIFT_EVERY_S` | Pixel-Shift gegen OLED-Einbrennen |
