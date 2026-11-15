# Hardware & Aufbau – ESP32 Oscilloscope Clock v2.0

## Komponenten

| Teil | Zweck |
|---|---|
| ESP32-WROOM-32U DevKitC V4 (USB-C, IPEX/U.FL) | Klassischer Dual-Core-ESP32 mit zwei 8-Bit-DACs (GPIO25/26) |
| Waveshare 2.42" OLED (SSD1309, 128×64) | Lokale Anzeige: Digitalzeit, Datum, WLAN/NTP-Status |
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
```

Die Pins stehen zentral in `ESP32_OscilloscopeClock_v2.0/config.h`. Nur GPIO25/26 sind fest vorgegeben,
weil die DACs nur an diesen beiden Pins herausgeführt sind.

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
- Betriebsart **X/Y**, beide Kanäle **DC-Kopplung**.
- Kanal 1 (X) und Kanal 2 (Y): **0,5 V/Div**. Bei 8 Divisions ergibt das 4 V, das 3,2-V-Bild passt also hinein.
- Mit den Position-Reglern auf Bildmitte schieben (DAC-Mitte ≈ 1,6 V).
- Helligkeit/Intensität eher gering, Fokus nachstellen.

## Feinabstimmung in `config.h`
| Parameter | Wirkung |
|---|---|
| `USE_FAST_DAC` | `1` = direkte Registerzugriffe (schnell). `0` = `dacWrite()` wie im Original (langsamer, flimmert eher) |
| `DOT_DELAY_CYCLES` | Wartezeit pro Bildpunkt. Erhöhen, wenn Ziffern verzerrt aussehen. Verringern, wenn das Bild flimmert |
| `DIAL_STYLE` | Zifferblatt: 0 = 1–12, 1 = 24 h, 2 = römisch |
| `TZ_INFO` | Zeitzone inkl. Sommerzeitregel (Standard: Mitteleuropa) |
| `OLED_SHIFT_EVERY_S` | Pixel-Shift gegen OLED-Einbrennen |
