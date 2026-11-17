# ESP32 Oscilloscope Clock – v2.0 (dual core, NTP, OLED)

[Deutsch](README.md) · **English**

A vector clock for an oscilloscope in X/Y mode, based on an idea by Mauro Pintus (see [Origin](#origin)),
adapted for this hardware: **ESP32-WROOM-32U DevKitC V4**, **Waveshare 2.42" OLED (SPI)**, external
2.4 GHz antenna, BNC outputs and an ABS enclosure.

**Full documentation with assembly guide, pictures and drilling template: [docs/ESP32_Oscilloscope_Clock_Documentation.pdf](docs/ESP32_Oscilloscope_Clock_Documentation.pdf)** (German version: [docs/ESP32_Oscilloscope_Clock_Dokumentation.pdf](docs/ESP32_Oscilloscope_Clock_Dokumentation.pdf))

**Build and wiring: [HARDWARE.en.md](HARDWARE.en.md)** · **Software design: [SOFTWARE.en.md](SOFTWARE.en.md)**

**Test page of the web interface without a clock: [web/demo/scope-clock-demo.html](web/demo/scope-clock-demo.html)** – download it and open it in a browser; all values are simulated.

Web interface, OLED and log can be switched between German and English („Sprache“ / “Language”).
This documentation quotes the German labels and adds the English label in parentheses, e.g.
„Einstellungen“ (Settings).

### Contents
- [Features](#features)
- [Quick start (Arduino IDE 2.x)](#quick-start-arduino-ide-2x)
- [Operation](#operation)
- [Status and tests](#status-and-tests)
- [Origin](#origin)

### Features
- **Core 1** does nothing but generate the X/Y signals (GPIO25/26) – no pauses, no locks.
- **Core 0** handles Wi-Fi, NTP and the OLED. Wi-Fi and the display bus are initialised there so
  that their interrupts never disturb the drawing path.
- NTP via the ESP32's built-in SNTP client (no `NTPtimeESP` library). Time zone with automatic
  daylight saving time, re-sync every hour.
- New drawing engine with constant beam speed: all lines and hands are equally bright, including
  diagonals. The speed is chosen for older CRT oscilloscopes (approx. 230 frames/s, adjustable).
- Hardly visible retrace lines: optimised drawing order, fast braked jumps, second hand drawn out
  and back.
- OLED as a pure text display: time (24 h) in 7-segment style, calendar week and date with small 7-segment digits, network name
  and signal strength (dBm), all centred, with pixel shifting against burn-in.
- Every NTP sync is reported with date and time on the serial console (115200 baud), as is a
  warning if syncs stop.
- Works without internet too: the time can be set by hand in the web interface (or taken over
  from the phone with one tap), optionally with summer or winter time fixed. Without any time source
  the clock counts from a fixed start time.
- **Optional DS3231 real-time clock** (I²C, GPIO21/22): keeps date and time with its coin cell even after a
  power failure without internet. Switch on in the web interface, off by default.
- Several Wi-Fi networks: the clock automatically connects to the strongest known network.
- **12- or 24-hour format** for OLED and web interface (12 h with AM/PM), switchable in the web interface.
- **German or English** for the web interface, OLED display and log, switchable in the web
  interface (side menu) and saved. Start value `UI_LANG` in `config.h`.
- **Web interface** at `http://scope-clock.local`, optimised for phones, no password, also as an app on the
  home screen. Light/dark/automatic appearance, log with filter, reset with “Undo”.
  At the top it shows a live preview of the oscilloscope picture (transparent background in the
  light theme), below that the status (time sync, Wi-Fi, signal strength, profile, frame rate,
  uptime, memory, CPU load, reason for the last restart) and the log („Protokoll“). It is also used
  to add Wi-Fi networks. A collapsible side menu (☰) jumps to the sections and highlights the current one in green; a thin bar at the top shows the scroll position. **Swiping left or right
  over the preview changes the dial**: 1–12, only 12/3/6/9, ticks only (no numbers), only Roman XII/III/VI/IX or Roman I–XII. This
  also applies to the oscilloscope and is saved. Under „Einstellungen“ (settings) you can change
  the oscilloscope profile, dial, beam dwell time, brightness of hands, tick marks and numerals, retrace speed, time zone, time
  server (NTP) and OLED (brightness 0–100 %, seconds, pixel shift, 180° rotation) online. In addition there is a **test pattern** for adjusting the oscilloscope, the **device name** and **back up/load settings** (as a file). Changes apply immediately
  and are saved. The web interface is always reachable as soon as the clock is on Wi-Fi. Without
  a known network the clock opens the setup network **“ScopeClock-Setup”**, and the phone shows
  the setup page automatically.
- One firmware for all oscilloscopes: the profile DIGITAL (default) or ANALOG is switched at run
  time with the BOOT button and saved. To be tested with Tektronix TDS3012, HP 54200A and Digilent
  Analog Discovery 2.

### Quick start (Arduino IDE 2.x)
1. Boards manager: install **esp32 by Espressif** (2.0.x or 3.x), board **ESP32 Dev Module**.
   Under **Tools → Partition Scheme** choose “**Huge APP (3MB No OTA/1MB SPIFFS)**”. This gives the
   program 3 MB instead of 1.25 MB (39 % used instead of 93 %). The clock uses neither OTA updates
   nor SPIFFS, and saved networks and settings survive the change.
2. Library manager: install **U8g2** (by oliver).
3. Open `ESP32_OscilloscopeClock_v2.0.ino` and upload. Set the serial monitor to 115200 baud.
4. Enter your Wi-Fi: select the network **“ScopeClock-Setup”** on your phone. The setup page opens
   automatically, otherwise open `http://192.168.4.1`. Enter your network under „Einstellungen“ → „WLAN“ (Settings → Wi-Fi)
   (up to 10 networks). Credentials are never stored in the source code.
   The clock automatically connects to the strongest known network in range.
5. Switch the oscilloscope to X/Y mode, 0.5 V/div, DC coupling. On digital oscilloscopes set the
   time base to approx. 1 ms/div and the trigger to Auto (details in HARDWARE.en.md).
6. **Oscilloscope profile via BOOT button:** the default profile is DIGITAL (digital
   oscilloscopes). For an analogue CRT oscilloscope hold the BOOT button for 3 s while running; the
   OLED shows “DIGITAL > ANALOG” with a countdown 3, 2, 1. The setting is saved, no new upload is
   needed. (Do not press the button while powering up, otherwise the ESP32 starts in download mode.)

Alternatively with PlatformIO: `pio run -t upload` (see `platformio.ini`, Huge APP is already set
there).

The start values of all settings (time zone, pins, dial, beam dwell time, oscilloscope profile)
are in `ESP32_OscilloscopeClock_v2.0/config.h`. At run time it is easier to change them in the web
interface.

### Operation

| Task | How |
|---|---|
| Open the web interface | `http://scope-clock.local` or the IP address from the OLED/log; without a known network via the setup network “ScopeClock-Setup” (`http://192.168.4.1`) |
| Change the dial | swipe left/right over the preview (on a PC: arrow keys), or „Einstellungen“ → „Zifferblatt“ (dial) |
| Oscilloscope profile DIGITAL/ANALOG | hold the BOOT button for 3 s, or „Einstellungen“ → „Oszilloskop-Profil“ |
| DS3231 real-time clock (optional) | „Einstellungen“ → „Uhrzeit“ → „Echtzeituhr (RTC)“ (real-time clock) |
| Factory reset (erase all settings and networks) | hold the BOOT button for 10 s – from 5 s the OLED counts down 5 … 1, releasing cancels |
| Z blanking via the Z input (optional, third wire to GPIO27) | „Einstellungen“ → „Oszilloskop“ → „Dunkeltastung (Z-Eingang)“ (Z blanking) |
| Set the time without internet | „Einstellungen“ → „Uhrzeit stellen“ (Set time; only shown while no time server has been reached); summer/winter time automatic or fixed |
| 12/24-hour format | „Einstellungen“ → „Zeitformat“ (Time format) |
| Language German/English | side menu (☰) → „Sprache“ (Language, drop-down list) |
| Add/delete Wi-Fi networks | „Einstellungen“ → „WLAN“ (Settings → Wi-Fi, also directly via the side menu) |
| Adjust the oscilloscope | „Einstellungen“ → „Oszilloskop“ → „Testbild“ (test pattern: frame, circle, centre cross; not saved). After power-on it appears automatically for 10 s |
| Device name, backup, reset everything | „Einstellungen“ → „System“ |
| Clear the log | „Protokoll“ section → „Leeren“ (Clear) |
| Troubleshooting | „Protokoll“ (log) section of the web interface, or the serial monitor (115200 baud) |

### Status and tests
**Real build** with Arduino CLI 1.6.0, board “ESP32 Dev Module”, default partition scheme
(1.25 MB program), all warnings enabled. It builds without errors and without warnings:

| Board package | Program memory | Global variables (RAM) |
|---|---|---|
| esp32 3.3.12 | 1,238,302 bytes (94 %) | 74,924 bytes (22 %) |
| esp32 2.0.17 | 866,053 bytes (66 %) | 61,048 bytes (18 %) – state before the menu and dial switching |

The configuration variants (profile ANALOG/DIGITAL, Z blanking, dial 0/1/2, `USE_FAST_DAC 0`,
EXCEL mode) were also built without warnings before the web interface was added. With the
recommended partition scheme **Huge APP** the same firmware uses approx. 1,235,000 of 3,145,728
bytes (**39 %**), with “Minimal SPIFFS” 62 %.

Also simulated on a PC: beam path (brightness, retrace lines, frame rate), OLED content, BOOT
button, calendar week, memory accesses (AddressSanitizer). The web page was tested in Chromium at
iPhone screen size against mocked clock interfaces: no JavaScript errors, no horizontal
scrolling, light and dark theme.
**Not yet tested on real hardware.** Typical adjustments for the first test are described in
[HARDWARE.en.md](HARDWARE.en.md) and [SOFTWARE.en.md](SOFTWARE.en.md).

---

## Origin

The idea comes from the **ESP32 Oscilloscope Clock by Mauro Pintus** (Milan 2018):
https://github.com/maurohh/ESP32_OscilloscopeClock – many thanks!

This version is an independent rewrite: firmware, web interface and all vector graphics (dial,
digits, hands – generated with `tools/gen_vectors.py`). No files of the original are included
any more.

The web interface font is a reduced version of **Raleway** (Copyright 2010 The Raleway Project
Authors, SIL Open Font License 1.1 – licence text in [fonts/OFL.txt](fonts/OFL.txt)). Because the
OFL requires a different name for modified versions, it is called “ScopeClock Sans” here.
