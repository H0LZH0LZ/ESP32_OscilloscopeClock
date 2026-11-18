# Hardware & build – ESP32 Oscilloscope Clock v2.0

[Deutsch](HARDWARE.md) · **English** · back to the [README](README.en.md)

## Components

| Part | Purpose |
|---|---|
| ESP32-WROOM-32U DevKitC V4, USB-C, with antenna kit (DUBEUYEW), 48.5 × 28.5 mm | Classic dual-core ESP32 with two 8-bit DACs (GPIO25/26); the kit contains the IPEX→SMA pigtail |
| Waveshare 2.42inch OLED display module, white (SSD1309, 128×64, SPI/I²C), 61.5 × 39.5 mm – **optional**, fitted in the prototype | Local text display (the clock also runs without it): time 24 h and date/calendar week (7-segment style), SSID, signal |
| WiFi antenna Bingfu 2.4/5.8 GHz, 8 dBi, SMA plug | Mandatory with the *-32U*: the module has **no** PCB antenna; do not mix SMA and RP-SMA |
| ESP32 breakout board with screw terminals | plug in the DevKit, all wires go into screw terminals (labelled P25, P26 …) – no soldering on the DevKit |
| 2× BNC pigtail, BNC plug to 2.54 mm DuPont pin, RG316 | X and Y outputs; the BNC plug goes straight into the oscilloscope (red = signal, brown = shield) |
| 2× cable gland Lapp SKINDICHT MINI M6×1 (52001860), clamping range 2–3 mm | leads the pigtails through the enclosure wall |
| USB-C panel-mount extension (IP67, 0.2 m, 16 mm cut-out) | power and programming from outside |
| ABS junction box 150 × 100 × 70 mm, IP67, hinged lid, mounting plate | Enclosure (RF-transparent) |
| Small parts | 2× 220 Ω (in the red lead of the pigtails), standoffs M2.5/M3 |

Detailed assembly guide with drilling template: [docs/ESP32_Oscilloscope_Clock_Documentation.pdf](docs/ESP32_Oscilloscope_Clock_Documentation.pdf).

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

optional – DS3231 real-time clock (ZS-042):
3V3  ─────────────────── RTC VCC
GND  ─────────────────── RTC GND
GPIO21 (I²C SDA) ─────── RTC SDA
GPIO22 (I²C SCL) ─────── RTC SCL

IPEX/U.FL ── pigtail (antenna kit) ── SMA socket in the opposite side ── Bingfu antenna (SMA)
```

All pins are defined in `ESP32_OscilloscopeClock_v2.0/config.h`. Fixed are GPIO25/26 (the only
DAC pins) and GPIO18/23 (hardware SPI for the display).

### Pin positions on the DevKitC (counted from the top, antenna socket at the top, USB-C at the bottom)

| Header | Pos. | Pin | Connection |
|---|---|---|---|
| left | 1 | 3V3 | OLED VCC |
| left | 9 | GPIO25 (DAC_1) | via 220 Ω → BNC “X” centre |
| left | 10 | GPIO26 (DAC_2) | via 220 Ω → BNC “Y” centre |
| left | 14 | GND | brown leads (shield) of all pigtails |
| right | 1 | GND | OLED GND |
| right | 2 | GPIO23 | OLED DIN |
| right | 9 | GPIO18 | OLED CLK |
| right | 10 | GPIO5 | OLED CS |
| right | 11 | GPIO17 | OLED DC |
| right | 13 | GPIO4 | OLED RST |
| right | 3 | GPIO22 | optional: RTC SCL |
| right | 6 | GPIO21 | optional: RTC SDA |
| right | 7 | GND | optional: RTC GND (RTC VCC to 3V3, left pos. 1) |

GPIO0 (right, pos. 14) is the BOOT button on the board – do not connect anything there.

### Notes on the DAC outputs
- Output range approx. 0 … 3.2 V, picture centre approx. 1.6 V.
- **220 Ω in series** protect the DAC against a short on the BNC cable and dampen ringing caused
  by the cable capacitance.
- Optional **1 nF from the BNC centre pin to GND**: this forms an RC low-pass (≈ 720 kHz) that
  smooths the DAC steps. If digits look “rounded” as a result, leave the capacitor out.
- No level shifter and no amplifier needed: the oscilloscope has 1 MΩ input resistance.

### OLED connection (Waveshare 2.42", optional)
The module is used **as delivered (4-wire SPI)**, no resoldering needed. Wire the 7-pin
connector as above (VCC to 3V3, not 5V0). If the picture is shifted horizontally by 2 pixels,
replace `NONAME0` with `NONAME2` in the `.ino`. If the display is mounted upside down, enable
„OLED um 180° drehen“ (rotate OLED by 180°) in the web interface.

### Antenna
- Never run the ESP32-**WROOM-32U** with Wi-Fi enabled without an antenna connected.
- Screw the pigtail's SMA socket into the enclosure wall. The antenna points upwards and sits as
  far away from the BNC cables as possible.

### Optional: DS3231 real-time clock (e.g. module ZS-042)
Keeps date and time with its coin cell even after a power failure without internet (DS3231: ±2 ppm, about
1 minute per year). Not part of the described build.

| Module | Breakout board |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | P21 |
| SCL | P22 |
| 32K, SQW | do not connect |

The module's 6-pin and 4-pin headers are connected through – each wire can go to whichever header
fits better.

- Switch on in the web interface: „Einstellungen“ → „Uhrzeit“ → „Echtzeituhr (RTC)“ (off by default). The status
  then shows “Running”, “Not found” or “Time invalid”.
- **Coin cell: normal CR2032**, no charger and no soldering needed. The ZS-042's charging circuit (diode
  and 200 Ω) is meant for 5 V. At **3V3** only about 2.7 V reach the cell after the diode – less than the cell
  voltage, so practically no charging current flows. The cell lasts 5 years and more, because it is only used
  while the clock is unplugged (approx. 3 µA).
- **Connect the module to 3V3 only, not to 5 V** – at 5 V the CR2032 would be charged (then desolder the
  diode). A rechargeable LIR2032 is unsuitable here: at 3V3 it would never be charged.
- The large chip should read **DS3231SN** – very cheap modules sometimes contain less accurate copies.
- The RTC runs in UTC; time zone and daylight saving time are still handled by the firmware. It is updated
  at every NTP sync (hourly) and when the time is set by hand.

### Power supply & ground
- Power via USB-C (5 V). The regulator on the DevKit provides 3.3 V, which is enough for the OLED.
- **Avoid ground loops:** the BNC ground of the oscilloscope is connected to protective earth. An
  earthed PC on USB can add hum and jitter to the picture. For permanent use, take a normal
  (unearthed) USB power adapter.

## Enclosure (junction box 150 × 100 × 70 mm)
- **Connector side** (short side, 100 mm), seen from outside: pigtail X at 28 mm, **the USB-C panel socket in the middle** (Ø 16 mm at 50 mm), pigtail Y at 72 mm. M6 cable glands: drill Ø 5.0 and tap M6 (or Ø 6.2 + nut).
- **Opposite side:** antenna (SMA, Ø 6.5 mm, centred).
- DevKit rotated by 180° in the breakout board (module towards the connector side, USB-C towards the antenna side); the antenna cable and the USB flat cable run once underneath the board to the opposite side.
- Lid: window for the OLED (active area approx. 55 × 28 mm), mount the module on M2.5 standoffs.
- Breakout board on the mounting plate; pigtails into terminals P25/P26 (red, via 220 Ω) and GND (brown).
- Drilling template 1:1 for printing on A4: last page of `docs/ESP32_Oscilloscope_Clock_Documentation.pdf`.

## Oscilloscope settings

The firmware knows two oscilloscope profiles. Switch at run time with the **BOOT button** of the
ESP32: **hold for 3 s**. The OLED shows the change (e.g. “DIGITAL > ANALOG”) with a countdown
3, 2, 1. Releasing earlier cancels. The setting is stored in flash and survives a restart. No new
upload is needed. Do **not** press the button **while powering up**, otherwise the ESP32 starts in
download mode. When switching, a bright dot may flash briefly on the oscilloscope (flash write).
The profile can also be changed in the web interface („Einstellungen“ → „Oszilloskop-Profil“).

**Factory reset:** hold the BOOT button **for 10 s**. After 3 s the profile switches first, from 5 s the
OLED counts down 5 … 1. Then all settings and saved networks are erased and the clock restarts (afterwards
setup network “ScopeClock-Setup”). Releasing earlier cancels.

**Optional – blanking via the Z input:** if the oscilloscope has a Z input (“Z”, “Z-Mod”, “Intensity”), a
third wire from **GPIO27** (terminal P27) to it can hide the retrace lines completely. It is not part of the
described build. Switch it on in the web interface („Einstellungen“ → „Oszilloskop“ → „Dunkeltastung
(Z-Eingang)“, Z blanking), together with the level (high or low = dark, see the oscilloscope manual).
When switched off, GPIO27 is high impedance.

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
| `BUTTON_FACTORY_MS` | Hold time for the factory reset (default 10000 ms; countdown on the OLED during the last 5 s) |
| `JUMP_SPEED` | Speed of the jumps between lines in profile ANALOG (default 24; DIGITAL always jumps instantly). Larger = fainter retrace line, smaller (12) for very old oscilloscopes, 0 = instant jump |
| `JUMP_LANDING`, `JUMP_RAMP` | Braking before the start of a line (default 8 and 4). Smaller `JUMP_LANDING` if lines start with small “hooks” |
| `ZBLANK_PIN`, `ZBLANK_ENABLE`, `ZBLANK_ACTIVE_HIGH` | Optional blanking via the Z input: GPIO (default 27), start value off/on and level. Not wired in the described build; switch on and off in the web interface (Settings → Oscilloscope) |
| `HAND_BRIGHT_PCT` | Brightness of hour and minute hand relative to the rest (default 67 %, then they look equally bright) |
| `TICK_BRIGHT_PCT`, `DIGIT_BRIGHT_PCT` | Brightness of tick marks and numerals, 0–100 % (default 100 %, 0 = hidden). Adjustable in the web interface |
| `OLED_BRIGHT_PCT` | OLED brightness 0–100 % (default 50 %, 0 = off) |
| `OLED_SECONDS` | 1 = OLED time with seconds, 0 = HH:MM only |
| `USE_FAST_DAC` | `1` = direct register access (default). `0` = `dacWrite()`, only as a fallback |
| `DIAL_STYLE` | Start dial: 0 = 1–12, 1 = 12/3/6/9, 2 = Roman I–XII, 3 = Roman XII/III/VI/IX, 4 = ticks only (no numbers). Switchable in the web interface by swiping over the preview, the choice is saved |
| `TZ_INFO` | Time zone incl. daylight saving rule (default: Central Europe) |
| `RTC_ENABLE`, `RTC_SDA_PIN`, `RTC_SCL_PIN` | Optional DS3231 real-time clock: start value off, I²C on GPIO21/22 (switchable in the web interface) |
| `NTP_SERVER_1` … `_3` | Time servers (the first one can be changed in the web interface) |
| `FALLBACK_H`, `FALLBACK_M`, `FALLBACK_S` | Start time from which the clock counts as long as neither a time server nor manual input has provided the time |
| `OLED_SHIFT_EVERY_S` | Pixel shift against OLED burn-in: every n seconds (0 = off) |
| `OLED_FLIP` | 1 = rotate the OLED display by 180° (display mounted upside down), also switchable in the web interface |
| `CLOCK_12H` | Time format for OLED and web interface: 0 = 24 hours (default), 1 = 12 hours with AM/PM. Switchable in the web interface |
| `BOOT_TEST_S` | test pattern after power-on in seconds (default 10, 0 = off) |
| `UI_LANG` | Language for web interface, OLED and log: 0 = German (default), 1 = English. Switchable in the web interface |
