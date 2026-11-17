# Hardware & build – ESP32 Oscilloscope Clock v2.0

[Deutsch](HARDWARE.md) · **English** · back to the [README](README.en.md)

## Components

| Part | Purpose |
|---|---|
| ESP32-WROOM-32U DevKitC V4 (USB-C, IPEX/U.FL) | Classic dual-core ESP32 with two 8-bit DACs (GPIO25/26) |
| Waveshare 2.42" OLED (SSD1309, 128×64) | Local text display: time 24 h (7-segment style), date, calendar week, SSID, signal |
| 2.4 GHz rod antenna + IPEX→SMA pigtail | Mandatory with the *-32U*: the module has **no** PCB antenna |
| ESP32 breakout board with screw terminals | plug in the DevKit, all wires go into screw terminals (labelled P25, P26 …) – no soldering on the DevKit |
| 2× BNC pigtail, BNC plug to 2.54 mm DuPont pin, RG316 | X and Y outputs; the BNC plug goes straight into the oscilloscope (red = signal, brown = shield) |
| 2× cable gland Lapp SKINDICHT MINI M6×1 (52001860), clamping range 2–3 mm | leads the pigtails through the enclosure wall |
| USB-C panel-mount extension (IP67, 0.2 m, 16 mm cut-out) | power and programming from outside |
| ABS junction box 150 × 100 × 70 mm, IP67, hinged lid, mounting plate | Enclosure (RF-transparent) |
| Small parts | 2× 220 Ω (in the red lead of the pigtails), standoffs M2.5/M3 |

Detailed assembly guide (German) with drilling template: [docs/ESP32_Oscilloscope_Clock_Dokumentation.pdf](docs/ESP32_Oscilloscope_Clock_Dokumentation.pdf).

## Wiring

```
Terminal (breakout board) Target
─────────────            ──────────────────────────────────────────────
P25 (DAC1) ──[220Ω]── pigtail "X" red (core)   ──> BNC plug → oscilloscope channel 1
P26 (DAC2) ──[220Ω]── pigtail "Y" red (core)   ──> BNC plug → oscilloscope channel 2
GND ─────────────────── pigtail "X" + "Y" brown (shield)

3V3  ─────────────────── OLED VCC
GND  ─────────────────── OLED GND
GPIO18 (VSPI SCK) ────── OLED CLK
GPIO23 (VSPI MOSI) ───── OLED DIN
GPIO5  ───────────────── OLED CS
GPIO17 ───────────────── OLED DC
GPIO4  ───────────────── OLED RST

IPEX/U.FL ── pigtail ── SMA socket in the enclosure wall ── rod antenna

Optional, blanking:
P27    ──[220Ω]──────── pigtail "Z" red (core)   ──> Z input of the oscilloscope (rear panel)
GND ─────────────────── pigtail "Z" brown (shield)
```

All pins are defined in `ESP32_OscilloscopeClock_v2.0/config.h`. Fixed are GPIO25/26 (the only
DAC pins) and GPIO18/23 (hardware SPI for the display).

### Pin positions on the DevKitC (counted from the top, antenna socket at the top, USB-C at the bottom)

| Header | Pos. | Pin | Connection |
|---|---|---|---|
| left | 1 | 3V3 | OLED VCC |
| left | 9 | GPIO25 (DAC_1) | via 220 Ω → BNC “X” centre |
| left | 10 | GPIO26 (DAC_2) | via 220 Ω → BNC “Y” centre |
| left | 11 | GPIO27 | optional via 220 Ω → BNC “Z” centre |
| left | 14 | GND | brown leads (shield) of all pigtails |
| right | 1 | GND | OLED GND |
| right | 2 | GPIO23 | OLED DIN |
| right | 9 | GPIO18 | OLED CLK |
| right | 10 | GPIO5 | OLED CS |
| right | 11 | GPIO17 | OLED DC |
| right | 13 | GPIO4 | OLED RST |

GPIO0 (right, pos. 14) is the BOOT button on the board – do not connect anything there.

### Optional: blanking via the Z input
The ESP32 cannot switch the beam off by itself. A very faint retrace line therefore remains
between two lines (the firmware keeps it as small as possible, approx. 1/24 of the line
brightness). Many CRT oscilloscopes have a BNC socket **“Z”, “Z-Mod”, “Z-Input”** or
**“Intensity”** on the rear. With a third output the beam can be switched off completely during
jumps:
1. Add a third pigtail with cable gland, wiring as above (P27 via 220 Ω).
2. Check in the oscilloscope manual which level makes the beam **dark**. On many devices high
   (TTL) blanks. On some it is the other way round, or the input needs more than 3.3 V.
3. Set `ZBLANK_PIN 27` in `config.h`, and `ZBLANK_ACTIVE_HIGH 0` if needed.
4. If the picture stays completely dark, the polarity is wrong. If bright dots appear at the start
   of lines, increase `ZBLANK_SETTLE_NS`.

### Notes on the DAC outputs
- Output range approx. 0 … 3.2 V, picture centre approx. 1.6 V.
- **220 Ω in series** protect the DAC against a short on the BNC cable and dampen ringing caused
  by the cable capacitance.
- Optional **1 nF from the BNC centre pin to GND**: this forms an RC low-pass (≈ 720 kHz) that
  smooths the DAC steps. If digits look “rounded” as a result, leave the capacitor out.
- No level shifter and no amplifier needed: the oscilloscope has 1 MΩ input resistance.

### OLED connection (Waveshare 2.42")
The module is used **as delivered (4-wire SPI)**, no resoldering needed. Wire the 7-pin
connector as above (VCC to 3V3, not 5V0). If the picture is shifted horizontally by 2 pixels,
replace `NONAME0` with `NONAME2` in the `.ino`. If the display is mounted upside down, enable
„OLED um 180° drehen“ (rotate OLED by 180°) in the web interface.

### Antenna
- Never run the ESP32-**WROOM-32U** with Wi-Fi enabled without an antenna connected.
- Screw the pigtail's SMA socket into the enclosure wall. The antenna points upwards and sits as
  far away from the BNC cables as possible.

### Power supply & ground
- Power via USB-C (5 V). The regulator on the DevKit provides 3.3 V, which is enough for the OLED.
- **Avoid ground loops:** the BNC ground of the oscilloscope is connected to protective earth. An
  earthed PC on USB can add hum and jitter to the picture. For permanent use, take a normal
  (unearthed) USB power adapter.
- Wi-Fi transmit bursts draw short current peaks. Slight jitter in the picture can be reduced with
  an electrolytic capacitor (e.g. 100 µF) between 3V3 and GND close to the DevKit.

## Enclosure (junction box 150 × 100 × 70 mm)
- Front: 2 (3) M6 cable glands for the pigtails, spacing 20 mm – drill Ø 5.0 and tap M6 (or Ø 6.2 + nut).
- Rear: USB-C panel socket (Ø 16 mm) and SMA antenna socket (Ø 6.5 mm).
- Lid: window for the OLED (active area approx. 55 × 28 mm), mount the module on M2.5 standoffs.
- Breakout board on the mounting plate; pigtails into terminals P25/P26 (red, via 220 Ω) and GND (brown).
- Drilling template 1:1: `docs/images/bohrplan.svg`.

## Oscilloscope settings

The firmware knows two oscilloscope profiles. Switch at run time with the **BOOT button** of the
ESP32: **hold for 3 s**. The OLED shows the change (e.g. “DIGITAL > ANALOG”) with a countdown
3, 2, 1. Releasing earlier cancels. The setting is stored in flash and survives a restart. No new
upload is needed. Do **not** press the button **while powering up**, otherwise the ESP32 starts in
download mode. When switching, a bright dot may flash briefly on the oscilloscope (flash write).
The profile can also be changed in the web interface („Einstellungen“ → „Oszilloskop-Profil“).

**Test pattern:** „Einstellungen“ → „Oszilloskop“ → „Testbild“ shows a frame with marks, a circle
and a centre cross instead of the clock. Use it to adjust position (cross in the centre of the screen),
size (the frame almost fills 8 × 8 divisions), equal X and Y gain (the circle is round, not oval) and
focus. The test pattern is not saved. **After every power-on it appears automatically for 10 s**
(`BOOT_TEST_S`), then the clock runs – so you see right away whether everything fits.


| Profile | for | Jumps between lines |
|---|---|---|
| DIGITAL (default) | digital oscilloscopes, storage oscilloscopes | instant, so no sample points appear along the way |
| ANALOG | analogue CRT oscilloscopes | fast and braked before the target, faint retrace line |

### Analogue CRT oscilloscope (profile ANALOG)
- Mode **X/Y**, both channels **DC coupling**.
- Channel 1 (X) and channel 2 (Y): **0.5 V/div**. With 8 divisions this gives 4 V, so the 3.2 V
  picture fits.
- Move to the centre with the position controls (DAC centre ≈ 1.6 V).
- Rather low brightness/intensity, adjust focus.

### Digital oscilloscope (profile DIGITAL, default)
In X/Y mode a digital oscilloscope only shows the samples of **one acquisition**. To see the whole
clock, the acquisition window (10 × time base) must cover at least one clock frame. One frame
takes approx. 4.3 ms, with Roman numerals approx. 6.2 ms.

| Setting | Value | Reason |
|---|---|---|
| Coupling | DC, both channels | The signals have a DC component (0–3.2 V) |
| Sensitivity | 0.5 V/div, both channels the same | otherwise the clock is distorted |
| Offset/position | centred, approx. −1.6 V or −3.2 div | DAC centre ≈ 1.6 V |
| Time base | **1 ms/div** (window 10 ms ≈ 2 frames) | whole clock in every acquisition |
| Trigger | **Auto** (free running) | The clock provides no trigger event |
| Persistence | short to medium, if available | smooths the picture build-up |
| Bandwidth limit | on (e.g. 20 MHz) | less noise, the signals are slow |

**Tektronix TDS3012**
- CH1 = X (GPIO25), CH2 = Y (GPIO26). Enable XY display in the Display menu.
- Time base 1 ms/div, trigger Auto.
- The TDS3000 grades intensity by frequency of hits (Digital Phosphor). A constant beam speed
  therefore directly gives an even brightness there as well.
- Short persistence (Display → Persist) calms the picture.

**HP 54200A**
- I could not verify whether and how this device offers an X/Y display. Please check the manual
  (Display menu).
- Good for the clock: the device has variable/infinite persistence. The picture then accumulates
  over several acquisitions, even if a single acquisition only contains few points.

**Digilent Analog Discovery 2**
- The inputs are **differential**: 1+ to GPIO25 (X), **1− to GND**; 2+ to GPIO26 (Y),
  **2− to GND**. Also connect the AD2's GND wire to the ESP32's GND.
- WaveForms → Scope: range 500 mV/div, offset −1.6 V, time base 1 ms/div, trigger Auto, enable
  the XY view (View → XY).
- Default buffer 8,192 samples per channel: with a 10 ms window that is approx. 1 sample per
  1.2 µs, i.e. 2–3 samples per clock step. More points are available with the device
  configuration with 2 × 16k scope buffer (WaveForms → Device Manager).
- If the XY view connects the samples with lines, the jumps are visible as lines too. Then switch
  to point display in the view options, if offered.
- Both devices are connected to the same PC and therefore share ground, which is fine.

## Fine tuning in `config.h`
Profile, dial, beam dwell time, brightness of hands, tick marks and numerals, retrace speed, time zone, time server (NTP) and
OLED are easier to change in the **web interface under „Einstellungen“**, with immediate effect on
the oscilloscope. The values in `config.h` are then only the start values.

| Parameter | Effect |
|---|---|
| `BEAM_NS_PER_UNIT` | Beam dwell time per step in ns. Larger = brighter lines, fewer frames/s (default 3000 ≈ 230 frames/s) |
| `SCOPE_PROFILE` | Start profile as long as none is saved: 0 = ANALOG, 1 = DIGITAL (default). Switchable at run time with the BOOT button |
| `BUTTON_LONG_MS` | Hold time of the BOOT button for switching (default 3000 ms) |
| `JUMP_SPEED` | Speed of the jumps between lines in profile ANALOG (default 24; DIGITAL always jumps instantly). Larger = fainter retrace line, smaller (12) for very old oscilloscopes, 0 = instant jump |
| `JUMP_LANDING`, `JUMP_RAMP` | Braking before the start of a line (default 8 and 4). Smaller `JUMP_LANDING` if lines start with small “hooks” |
| `ZBLANK_PIN` | optional output for blanking (Z input), -1 = off |
| `HAND_BRIGHT_PCT` | Brightness of hour and minute hand relative to the rest (default 67 %, then they look equally bright) |
| `TICK_BRIGHT_PCT`, `DIGIT_BRIGHT_PCT` | Brightness of tick marks and numerals, 0–100 % (default 100 %, 0 = hidden). Adjustable in the web interface |
| `OLED_BRIGHT_PCT` | OLED brightness 0–100 % (default 50 %, 0 = off) |
| `OLED_SECONDS` | 1 = OLED time with seconds, 0 = HH:MM only |
| `USE_FAST_DAC` | `1` = direct register access (default). `0` = `dacWrite()`, only as a fallback |
| `DIAL_STYLE` | Start dial: 0 = 1–12, 1 = 12/3/6/9, 2 = Roman I–XII, 3 = Roman XII/III/VI/IX, 4 = ticks only (no numbers). Switchable in the web interface by swiping over the preview, the choice is saved |
| `TZ_INFO` | Time zone incl. daylight saving rule (default: Central Europe) |
| `NTP_SERVER_1` … `_3` | Time servers (the first one can be changed in the web interface) |
| `FALLBACK_H`, `FALLBACK_M`, `FALLBACK_S` | Start time from which the clock counts as long as neither a time server nor manual input has provided the time |
| `OLED_SHIFT_EVERY_S` | Pixel shift against OLED burn-in: every n seconds (0 = off) |
| `OLED_FLIP` | 1 = rotate the OLED display by 180° (display mounted upside down), also switchable in the web interface |
| `CLOCK_12H` | Time format for OLED and web interface: 0 = 24 hours (default), 1 = 12 hours with AM/PM. Switchable in the web interface |
| `BOOT_TEST_S` | test pattern after power-on in seconds (default 10, 0 = off) |
| `UI_LANG` | Language for web interface, OLED and log: 0 = German (default), 1 = English. Switchable in the web interface |
