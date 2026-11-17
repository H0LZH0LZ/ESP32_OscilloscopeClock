# Software – ESP32 Oscilloscope Clock v2.0

[Deutsch](SOFTWARE.md) · **English**

This document describes how the firmware is structured. For wiring and enclosure see
[HARDWARE.en.md](HARDWARE.en.md), for the quick start the [README](README.en.md).
Web interface, OLED and log can be switched between German and English. Labels are quoted in
German with the English label in parentheses. Code comments are in German.

## Files

| File | Content |
|---|---|
| `ESP32_OscilloscopeClock_v2.0/ESP32_OscilloscopeClock_v2.0.ino` | Firmware (tasks, DAC, time, Wi-Fi, OLED, web server) |
| `ESP32_OscilloscopeClock_v2.0/config.h` | All settings: time zone, NTP, pins, OLED, beam dwell time, oscilloscope profile, dial |
| `ESP32_OscilloscopeClock_v2.0/DataTable.h` | Vector data for dial, digits and hands – generated with `tools/gen_vectors.py` |
| `tools/gen_vectors.py` | Generates `DataTable.h` (Python 3, no extra packages) |
| `ESP32_OscilloscopeClock_v2.0/webpage.h` | Web interface: one page (HTML, CSS, JavaScript) in flash |
| `web/index.html` | Source of the web page – after changes run `python3 tools/gen_webpage.py` (generates `webpage.h`, gzip) |
| `web/demo/scope-clock-demo.html` | Test page: the same web page with simulated interfaces, runs in a browser without an ESP32 – generated with `python3 tools/gen_demo.py` from `webpage.h`, `font.h` and `web/demo/mock-api.js`, `mock-settings.js`, `demo-ui.js` |
| `ESP32_OscilloscopeClock_v2.0/font.h` | Web interface font (“ScopeClock Sans” = reduced Raleway, two static styles Light (300) for text and SemiBold (600) for headings/buttons, WOFF2, approx. 30 KB in total), served at `/font.woff2` and `/font-600.woff2` – generated with `tools/gen_font.py` |
| `tools/gen_font.py` | Generates `font.h` from `Raleway[wght].ttf` (needs `pip install fonttools brotli`) |
| `fonts/OFL.txt` | Font licence (SIL Open Font License 1.1) |
| `ESP32_OscilloscopeClock_v2.0/icon.h` | App icon (`/icon.svg`, `/icon.png` 192 px) and `/manifest.json` for “Add to home screen” – generated with `tools/gen_icon.py` (needs Pillow) |
| `platformio.ini` | Alternative to the Arduino IDE |

## Division of work between the two cores

```
            Core 1                                   Core 0
 ┌──────────────────────────┐          ┌────────────────────────────────────────┐
 │ scopeTask                │          │ uiTask (every 10 ms)                   │
 │  forever:                │  g_time  │  publishTime()  → g_timePacked         │
 │   read time   ◄──────────┼──Packed──┤  buttonPoll()   → g_scopeProfile       │
 │   hour, minute hand      │ (32 bit) │  oledUpdate()   every 200 ms (SPI)     │
 │   second hand out        │          │  wifiLogChanges() every second         │
 │   dial round trip        │ g_scope- │  webLoop()      web server             │
 │   second hand back       │◄─Profile─┤                                        │
 │   → DAC GPIO25 / GPIO26  │ (1 byte) ├────────────────────────────────────────┤
 └──────────────────────────┘          │ wifiTask: WiFiMulti, keep connection   │
                                       │ system:   Wi-Fi stack, SNTP client     │
                                       └────────────────────────────────────────┘
```

- **Core 1** draws without any pause so that the picture on the oscilloscope does not flicker.
  The task never waits for a lock, never calls `vTaskDelay()` and reads exactly one 32-bit value
  per frame. The idle watchdog of core 1 is therefore disabled (`disableCore1WDT()`).
- **Core 0** handles everything else. Wi-Fi and the display bus (SPI) are initialised in the
  `uiTask` so that their interrupts run on core 0.
- The functions of the drawing path (`beamPoint`, `beamMove`, `PlotTable`, DAC access) are placed
  in internal RAM with `IRAM_ATTR`, as are the tables in `DataTable.h`. There are therefore no
  flash cache misses that would make the timing – and thus the brightness – uneven.

### Shared data

| Variable | Writer | Reader | Protection |
|---|---|---|---|
| `g_timePacked` (h/min/s) | uiTask | scopeTask | 32-bit access is atomic |
| `g_scopeProfile` (ANALOG/DIGITAL) | uiTask (BOOT button, web), `setup()` | scopeTask (`beamJump`) | 1 byte, atomic |
| `g_dialStyle` (dial 0–3) | uiTask (web interface), `setup()` | scopeTask (rebuilds the round trip) | 1 byte, atomic |
| `g_frameCount` (frame counter) | scopeTask | uiTask (frame rate) | 32-bit access is atomic |
| Log ring buffer | `logMsg()` on both cores | web interface | spinlock |
| `g_ntpSynced` | SNTP callback | uiTask | `bool`, atomic |
| `g_timeManual` | web interface (`POST /api/time`) | uiTask | `bool`, atomic |
| `s_lang` (language), `s_dstForced` (summer/winter time set by hand) | uiTask (web interface) | uiTask | core 0 only |
| `g_lastSyncEpoch` | SNTP callback | uiTask | spinlock (`time_t` is 64 bit) |

## Time and NTP

- `configTzTime()` starts the built-in SNTP client of ESP-IDF and sets the time zone (`TZ_INFO`,
  incl. daylight saving rule). It waits by itself until Wi-Fi is up and then re-syncs every
  `NTP_SYNC_INTERVAL_MS` (default: 1 h).
- After every sync it calls `onNtpSync()`. The function only records the timestamp; console
  output is done by `wifiLogChanges()`.
- Before the first sync `currentTime()` counts up from `FALLBACK_H:M:S`. Date and calendar week are
  unknown then. After the first sync the ESP32 system clock keeps running by itself, even without
  Wi-Fi.
- **Without internet** the time can be set in the web interface (`POST /api/time`): either as
  Unix time from the phone (`epoch`, „Uhrzeit dieses Geräts übernehmen“ – take over this device's
  time) or as date and time from the input fields (`local`, interpreted in the configured time zone,
  converted with `mktime()`). `settimeofday()` sets the system clock, `g_timeManual` marks it as
  valid (`timeValid()`). A later NTP sync overwrites it. The ESP32 has no battery-backed real-time
  clock: after a restart the time has to be set again.
- **Summer/winter time for manual input:** „Automatisch“ (Automatic, rules of the time zone),
  „Sommerzeit“ (Summer time) or „Winterzeit“ (Winter time) (`dst` = -1/1/0). When fixed,
  `applyDstOverride()` sets a time zone without switching rule, e.g. `CEST-2:00` instead of
  `CET-1CEST,M3.5.0,M10.5.0/3` (`tzPart()` reads name and offset from the POSIX time zone). With the
  next NTP sync or a time zone change the normal time zone applies again. The choice is only shown
  for time zones with daylight saving time.

## Language (German/English)

- `s_lang` (NVS key `lang`, start value `UI_LANG`) applies to web interface messages, the OLED and
  the log. `tr("german", "english")` picks the text. Log lines keep the language they were written
  in; `loadSettings()` therefore runs before the first output in `setup()`.
- OLED in English: `WiFi …`, `Week 46 - 2026-11-15` (ISO date), “Oscilloscope profile”.
- Web page: written in German, the table `EN` in the script translates. `applyLang()` translates all
  fixed texts (text nodes, `aria-label`) – and back via the reverse table –, `t()` the texts from the
  script. Switch in the side menu with the drop-down list „Sprache“ (Language); the page changes immediately and
  stores the language on the clock (`POST /api/settings`, `lang`). A change from another device is
  picked up with the next status request.

## Wi-Fi

- Wi-Fi networks are only entered in the web interface (no credentials in the source code). At
  start-up `webNetsLoad()` registers all saved networks with `WiFiMulti`.
- `wifiTask` checks the connection every 2 s. If it is lost, `WiFiMulti::run()` scans and connects
  to the **strongest known** network. The scan blocks for a few seconds, which is why it runs in its
  own task and the OLED keeps running.

## Web interface

Reachable at `http://scope-clock.local` or the clock's IP address (shown in the log), without a
password. This applies whenever the clock is connected to a network, not only in the setup
network. The `WebServer` runs in the `uiTask` on core 0 (`webLoop()` every 10 ms) and does not
affect the beam output on core 1.

**Page (`webpage.h`):** one page with a **collapsible side menu** (☰ top left, stays visible while
scrolling): „Uhr“ (clock), Status, „Einstellungen“ (settings), with the sub-entries „Oszilloskop“, „Uhrzeit“, „Display“, „WLAN“ and „System“ (jump directly to that group of
the settings), „Protokoll“ (log), below them
the language selection. The section taking up most of the screen is highlighted in green in the menu (scroll spy, `spy()`;
for a settings group „Einstellungen“ is green as well), and a 3 px green bar at the top edge shows how far the page
is scrolled. The settings are split into five groups: **„Oszilloskop“** (Oscilloscope: profile first, dial,
test pattern in second place, dwell time, brightness, retrace speed), **„Uhrzeit“** (Time: “Set time” if shown, time format,
time zone, time server), **„Display“** (OLED brightness, seconds, rotation, pixel shift)
and **„WLAN“** (Wi-Fi: saved networks, network scan, add network, reconnect, reset Wi-Fi) and **„System“** (device name, back up/load settings).
The top section (title, preview, time, date) is centred. The title bar “ESP32 Oscilloscope Clock”
always stays at the top next to the menu button, the section headings directly below it
(`position: sticky`). Each section heading is only pinned within its section (`.grp`), so the
next one pushes the previous one out. `scroll-margin-top` makes jump targets from the menu land
below the title bar. Tapping a menu entry scrolls there smoothly and closes the menu again, as does
tapping beside it or Esc. At the very top there is a **live preview of the oscilloscope picture**
(canvas with an 8×8 grid as on the oscilloscope). In the **dark theme** this is a black screen
with glowing green lines, in the **light theme** the background is transparent with dark green
lines and a faint grid. The colours are CSS variables `--scope-*`; the canvas reads them on every
redraw, so a theme change takes effect immediately. For this the page loads the vector data once
from `/api/vectors` – the same tables the `scopeTask` draws – and positions the hands itself
according to the time (hour = `(h%12)*5 + m/12`, as in the firmware). Between status requests the
time keeps running in the browser to the second.

**Changing the dial:** swiping over the preview cycles through four dials – left for the next,
right for the previous one (pointer events, at least 40 px horizontally; vertical swiping still
scrolls the page, `touch-action: pan-y`). On a PC the arrow keys work too when the preview has
focus. A plain tap does not change the dial. Order when swiping and in the drop-down list:
numbers 1–12 → numbers 12 · 3 · 6 · 9 → ticks only → Roman XII · III · VI · IX → Roman I–XII (style 0, 1, 4, 3, 2 – “ticks only” sits in the middle):

| Style | Digits | Table |
|---|---|---|
| 0 | 1–12, 7-segment style | `DialDigits12` |
| 1 | only 12, 3, 6, 9, 7-segment style | `DialDigitsMin` |
| 2 | Roman I–XII, sans serif, 4 as IIII | `DialDigitsRoman` |
| 3 | only XII, III, VI, IX | `DialDigitsRoman4` |
| 4 | no numbers, tick marks only | `DialDigitsNone` (empty) |

The preview shows the new style immediately. `POST /api/dial` sets
`g_dialStyle`. The `scopeTask` detects the change before the next frame and recomputes the round
trip. This takes a few tens of milliseconds, during which the beam stands still (short bright
dot). The choice is stored in NVS (key `dial`); `DIAL_STYLE` in `config.h` is only the start
value. All four digit sets are therefore kept in RAM (approx. 0.7 KB in total).

Below follow three sections. Every setting and every button has a short description:
- **Status:** large time, calendar week and date, Wi-Fi, signal
  strength, network address, below it time sync (NTP) with last sync, uptime, below it the reason for the last restart, memory (free/total, below it the minimum since restart and the
  largest free block), CPU usage of core 0 and core 1 (one row each with a short note; core 1 draws
  all the time and always shows 100 %), firmware version. Updated every 2 s. Oscilloscope
  profile and frame rate are not duplicated in the status but only shown in the settings (profile
  selection, frame rate under „Verweilzeit des Strahls“ – beam dwell time).
  Only problems are highlighted in colour: orange = warning (NTP waiting or overdue, Wi-Fi
  searching, signal below −80 dBm, setup network active, minimum below 20 KB, largest block below
  8 KB, core 0 above 85 %, restart after crash, watchdog or brown-out), red = error (no answer from
  the clock, rejected input). Normal states and confirmations use the normal text colour.
- **„Einstellungen“ (settings):** see [Settings in the web interface](#settings-in-the-web-interface).
  As long as no time server has been reached, „Uhrzeit stellen“ (set time) appears at the top
  (see [Time and NTP](#time-and-ntp)).
- **„WLAN“ (Wi-Fi, group in the settings):** saved networks (up to 10, each with “Delete”), network
  scan (strongest network first, bars and dBm, the connected network is marked; tapping a network takes over its name), form „Netz hinzufügen“ (add network; collapsed at first – open while no network is saved and when tapping a found network), „Neu verbinden“ (reconnect), „WLAN zurücksetzen“ (reset Wi-Fi: delete all networks).
- **„Protokoll“ (log):** the last `LOG_BUFFER_SIZE` bytes (8 KB, approx. 120 lines), refreshed every
  5 s while „Automatisch aktualisieren“ (auto refresh) is on („Aktualisieren“ – refresh –
  loads immediately). The field has a fixed height (max. 60 % of the screen) with its own scroll
  bar. Swiping inside the field only scrolls the log, not the page. If you are at the bottom, new
  lines follow automatically. If you are reading further up, the position is kept on refresh.
  The line count is shown below. „Herunterladen“ (download) saves the whole log (unfiltered) as a text
  file `<device-name>-log-YYYYMMDD-HHMM.txt`. „Leeren“ (clear) empties the clock's ring buffer after a confirmation
  (`POST /api/log/clear`); the first new line is “log cleared”.
- **„System“ (group in the settings):** device name (`host`, a–z, 0–9, hyphen, max. 32; applies at once
  to mDNS `http://<name>.local` and to the router on the next connection), back up
  settings (JSON file with all settings except Wi-Fi networks) and load them (sent to
  `POST /api/settings`), „Alle Einstellungen zurücksetzen“ (reset all settings: all groups, language and
  device name to `config.h`, test pattern off; Wi-Fi networks are kept; “Undo” for 8 s).
- **Status** and **„Protokoll“** are collapsible (V arrow): status starts open, log closed (on every visit: status open, everything else closed – nothing is saved; a jump from the menu opens them).

**Usability aids of the page:**
- **Dots below the preview** show the active dial; tapping a dot or the arrows ‹ › changes it (in addition to swiping).
  No name is shown when switching.
- **Settings groups** are collapsed (V arrow) and open by tapping the heading (closed again on every visit, nothing is saved). Menu sub-entries
  open their group; when “Set time” appears, the time group opens by itself and closes again once the time server is reached again.
- **“✓ Saved”** briefly appears right at the changed setting (errors in red).
- **Preview while dragging:** the dwell time changes the preview brightness and shows the estimated frame rate (~);
  in profile ANALOG the retrace speed shows the retrace lines in the preview (fainter the faster).
- **Resetting** a group needs no confirmation; instead “Undo” is offered for 8 s (sends the previous values).
  “Reset Wi-Fi” still asks (restart, cannot be undone).
- **Signal strength** in the status with bars.
- **Appearance** in the side menu: automatic, light or dark (saved only in the browser, `data-theme`).
- **Log filter** (drop-down list): all, Wi-Fi, time, web, oscilloscope, warnings.
- **As an app:** manifest and icon allow “Add to home screen”; the page then opens without browser bar.
- **Preview:** redrawn once per second; the second hand steps from second to second like on the oscilloscope.
  Larger on tablet/PC (up to 460 or 560 px); on a phone in landscape, preview and time sit side by side.
- **Font:** Raleway Light/SemiBold with Raleway's original old-style figures, text in black.

Optimised for phones: one column, buttons at least 44 px high, input fields with 16 px text (iOS
zooms otherwise), light/dark theme following the system setting, margins for notch and home bar,
no external files – even the font comes from the clock (works in the setup network without
internet). Requests pause while the page is in the background. Text from the clock is always
inserted as text (`textContent`), never as HTML.

**Interfaces:**

| Method and path | Function |
|---|---|
| `GET /` | the page |
| `GET /manifest.json`, `/icon.svg`, `/icon.png` | app manifest and icon for the home screen |
| `GET /font.woff2`, `GET /font-600.woff2` | font of the page, Light and SemiBold (from `font.h`), cached by the browser for one year. Two static styles instead of a variable font, because some Android browsers otherwise show bold text of a variable font in the system font |
| `GET /api/status` | JSON: `time`, `date`, `kw`, `ntp{synced,overdue,manual,dst,last,server}`, `wifi{configured,connected,ssid,rssi,ip,host}`, `ap{active,ssid,ip}`, `profile`, `dial`, `fps`, `heap`, `heapTotal`, `heapMin`, `heapBlock`, `cpu0`, `reset`, `uptime`, `fw`, `lang`, `h12`, `test` (test pattern on) |
| `GET /api/log` | log as text, oldest line first |
| `POST /api/log/clear` | empty the log ring buffer (serial output unaffected) |
| `GET /api/networks` | saved networks `[{ssid, source:"web"}]`, never passwords |
| `GET /api/vectors` | vector data for the preview (the preview does not show the test pattern, only the oscilloscope does) (approx. 9 KB): `dial`, `digits` (all dials, index = style), `hr`, `min`, `sec` (60 positions each). Polylines as `[x0,y0,x1,y1,…]`, 0–255, y upwards |
| `GET /api/scan` | network scan in the background: first `{"running":true}`, then `{"networks":[{ssid,rssi,open}]}` |
| `POST /api/wifi` | save `ssid`, `pw` or change the password (SSID 1–32, password empty or 8–63 characters) |
| `POST /api/wifi/delete` | delete `ssid`, then restart (WiFiMulti cannot forget networks) |
| `POST /api/wifi/reset` | delete all saved networks, then restart (the setup network opens) |
| `POST /api/reconnect` | disconnect, `wifiTask` connects to the strongest network |
| `GET /api/settings` | current settings, default values (`defaults`) and time zone list |
| `POST /api/settings` | any of the fields `profile`, `dial`, `beamNs`, `handPct`, `tickPct`, `digitPct`, `jumpSpeed`, `tz`, `ntp`, `oledPct`, `oledSec`, `oledShift`, `oledFlip`, `lang` (`de`/`en`), `h12` (0/1), `host` (device name), `test` (0/1, test pattern, not saved): validate, apply immediately, save. Answer `{ok, msg, settings}` |
| `POST /api/settings/reset` | `group` = `scope` (oscilloscope), `time` or `display`: reset only this group to the values from `config.h`; without `group` everything incl. language and device name, test pattern off. Wi-Fi networks are always kept |
| `POST /api/dial` | `style` 0 = 1–12, 1 = 12/3/6/9, 2 = Roman I–XII, 3 = Roman XII/III/VI/IX, 4 = ticks only: change the dial (oscilloscope and preview), saved |
| `POST /api/time` | `epoch` (Unix seconds, UTC) **or** `local` (`YYYY-MM-DDTHH:MM[:SS]`, local time), optional `dst` (-1 automatic, 1 summer time, 0 winter time): set the time by hand, 2024–2099. The answer `msg` contains the time that was set; the page shows it with ✓ right below the pressed button |

Messages returned by the interfaces (`msg`) are in the selected language.

**Saved networks:** up to `WEB_MAX_NETWORKS` (10) networks in NVS (namespace `scopeclock`, keys
`netn`, `net_s0…`, `net_p0…`). They survive a firmware update.

**Setup network (captive portal):** if no network is configured or no connection has been possible
for `WEB_SETUP_AP_AFTER_S` (60 s), `webApManage()` opens the open network `WEB_SETUP_AP_SSID`
(“ScopeClock-Setup”) in AP+STA mode. A `DNSServer` answers every name with 192.168.4.1, unknown
addresses are redirected to the start page. This makes phones open the setup page automatically.
Once the connection to a known network has been stable for 30 s, the setup network closes again.
During this time the OLED shows “WiFi ScopeClock-Setup” and “http://192.168.4.1”.

**Log:** `logMsg()` replaces direct `Serial` output. Every line gets a timestamp (`hh:mm:ss` once
the time is valid – via NTP or set by hand –, before that `+<s>s` since start) and goes to the
console and into a ring buffer (`LOG_BUFFER_SIZE`, 8 KB). A spinlock protects the buffer because
both cores can write. Log lines avoid umlauts because they also go to the serial console.

**Frame rate:** `scopeTask` counts frames (`g_frameCount`), the `uiTask` calculates the frame rate
for the status display every second.

**CPU load core 0:** an idle hook (`esp_register_freertos_idle_hook_for_cpu`) adds up the time in
which the idle task of core 0 runs. If two calls follow closely (< 200 µs), the core was free in
between. Load = 100 % minus idle share, recalculated every second. Core 1 is intentionally always
100 % busy with the beam output.

**Memory and restart reason:** `ESP.getFreeHeap()`, `getHeapSize()`, `getMinFreeHeap()`,
`getMaxAllocHeap()` and `esp_reset_reason()`. The restart reason is also written to the log at
start-up.

## Settings in the web interface

The „Einstellungen“ (settings) section changes values at run time. Every change takes effect
immediately and is stored in NVS. Sliders send when released, so that flash is not written on every
movement. `config.h` only provides the start values. Each group (oscilloscope, time, display) has its own
button „Auf Standardwerte zurücksetzen“ (Reset to defaults): it deletes only the stored values of
that group; Wi-Fi networks are kept.

| Setting (German label) | NVS key | Range | Effect |
|---|---|---|---|
| Oscilloscope profile („Oszilloskop-Profil“) | `profile` | DIGITAL / ANALOG | `g_scopeProfile`, also via BOOT button |
| Dial („Zifferblatt“) | `dial` | 5 styles | `g_dialStyle`, in sync with swiping over the preview |
| Beam dwell time („Verweilzeit des Strahls“) | `beam` | 1,000–10,000 ns per step | `g_beamNs`, `scopeTask` recalculates the timing between two frames (`beamTiming()`) |
| Hour/minute hand brightness („Helligkeit Stunden-/Minutenzeiger“) | `hand` | 0–100 % (0 = off) | `g_handPct` |
| Tick mark brightness („Helligkeit Minutenstriche“) | `tick` | 0–100 % (0 = off) | `g_tickPct`: dwell time of the marks (table `DialData`) in the round trip, in percent of the normal dwell time |
| Numeral brightness („Helligkeit Ziffern“) | `digit` | 0–100 % (0 = off) | `g_digitPct`: dwell time of the numerals in the round trip |
| Retrace speed („Rücklauf-Tempo“) | `jump` | 0 (instant jump) – 48 (× drawing speed) | `g_jumpSpeed`, only effective in profile ANALOG. With DIGITAL the slider shows a greyed-out 0 (instant jump); the ANALOG value stays saved and reappears when switching back |
| Time zone („Zeitzone“) | `tz` | list `TIMEZONES[]` (or `TZ_INFO`) | `configTzTime()` again |
| Time server („Zeitserver (NTP)“) | `ntp` | host name, max. 63 characters | first server, `configTzTime()` again |
| OLED brightness („OLED-Helligkeit“) | `oledp` | 0–100 % (0 = off) | `oledApplyBrightness()`: 0 % = `setPowerSave(1)`, otherwise contrast 1–255. An old value `oled` (contrast) is converted on first start |
| Show seconds („Sekunden anzeigen“) | `osec` | on/off | OLED time HH:MM:SS or HH:MM |
| OLED pixel shift („OLED-Pixelverschiebung“) | `shift` | on/off | offset against burn-in |
| Rotate OLED by 180° („OLED um 180° drehen“) | `flip` | on/off | `u8g2.setDisplayRotation(U8G2_R2)`, for a display mounted upside down |
| Device name („Gerätename“) | `host` | a–z, 0–9, hyphen, max. 32 | `WiFi.setHostname()`, `MDNS.begin()` again |
| Test pattern („Testbild“) | – | on/off | `g_testPattern`: `scopeTask` draws `TestData` instead of the clock; not saved |
| Language („Sprache“) | `lang` | Deutsch / English | `s_lang`: web interface, OLED, log |
| Time format („Zeitformat“) | `h12` | 24 hours / 12 hours (AM/PM) | `s_h12`: time on OLED and web page (log stays 24 h) |

The firmware validates every value (`handleSettingsPost()`) and reports invalid input with a
reason. `loadSettings()` reads all values in `setup()` before the tasks start.


## BOOT button: oscilloscope profile

**Test pattern at start-up:** `g_testPattern` starts as `BOOT_TEST_S > 0`; the `uiTask` switches it off
after `BOOT_TEST_S` seconds (log “start-up test pattern finished”) unless it was switched by hand in the
web interface before.

`buttonPoll()` in the `uiTask` polls the BOOT button (GPIO0, pressed = LOW) every 10 ms. After
holding it for `BUTTON_LONG_MS` (3 s), `g_scopeProfile` is switched. It is one byte and therefore
atomic; `beamJump()` on core 1 reads it at the next jump. The profile is then stored in NVS
(`Preferences`, namespace `scopeclock`, key `profile`). Each press switches only once, even if the
button is held longer. Releasing earlier cancels. `setup()` loads the profile before the
`scopeTask` starts. If nothing is stored yet, `SCOPE_PROFILE` from `config.h` applies.

While the button is held (from 0.3 s), `oledProfileOverlay()` replaces the normal display:

```
 Oszilloskop-Profil       font 6x10
  DIGITAL > ANALOG        font 7x14 bold: current > new profile
         3                countdown 3, 2, 1 as a 7-segment digit (9×16 px, drawSegDigit)
```

A “1” consists only of the right-hand segments and is therefore moved towards the centre in the
countdown. After switching, the normal display returns immediately.

## Console output (115200 baud) and log in the web interface

Both show the same lines, with a timestamp (`+<s>s` until the time is valid):

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

(Shown in German here; with the language set to English the lines read e.g. “[WiFi] connected to …”,
“[WiFi] connection lost – retrying”, “[Time] set by hand …”.)
Passwords are never printed.

## OLED display

```
      22:15:06            7-segment, digits 11×20 px (drawn by the firmware); 12 h: "10:15:06 PM" without leading zero, AM/PM small (font 5x8)
 KW 46 - 15.11.2026       digits, dots, hyphens as small 7-segment digits 5×9 px (open corners), "KW" font 6x10; calendar week (KW) according to ISO 8601
WLAN Gastzugang           font 5x8, max. 25 characters (otherwise shortened with "..")
   Signal -61 dBm         font 5x8
```

In the setup network the two lower lines show “WiFi ScopeClock-Setup” and “http://192.168.4.1”.

- **7-segment digits:** the ready-made 7-segment fonts of U8g2 are too wide for `HH:MM:SS` on
  128 px. `drawSegDigit()` therefore assembles each digit from up to 7 rectangles. Which segments
  are lit is defined in `SEG_MAP`. The date line uses the same digits in small size (5×9 px,
  1 px segments): `drawSegTextCentered()` draws digits, `.` and `-` as segments and all other
  characters (“KW”, “Week”) in font 6x10. Without NTP time the line shows normal text.
- **Calendar week:** `isoWeek()` calculates it according to ISO 8601: the week starts on Monday,
  week 1 contains 4 January. Checked against `strftime("%V")` for every day from 2000 to 2040.
- **Pixel shift:** every `OLED_SHIFT_EVERY_S` seconds the whole picture moves by 0–3 px
  horizontally and 0–1 px vertically. This prevents burn-in of static content.
- **Adjusting positions:** `Y_TIME`, `Y_DATE`, `Y_SSID`, `Y_RSSI` at the beginning of
  `oledUpdate()`. U8g2 positions text at the **baseline**, not the top edge.

## Vector engine: equal brightness for all lines

How bright a trace glows on the screen depends on how long the beam dwells per distance. The
drawing routine of v1.0 (Bresenham, same time per raster point) therefore showed visible
brightness differences. v2.0 replaces it with a vector engine with constant beam speed:

| Rule | Implementation | Original v1.0 |
|---|---|---|
| Equal distances | Every line split into steps of exactly 1 DAC unit **length** (Euclidean) | Bresenham: only 0.71 points per unit at 45°, diagonal lines darker |
| Equal dwell time | Every step exactly `BEAM_NS_PER_UNIT`, using the CPU cycle counter. If a step is delayed by an interrupt, the following ones are shorter | time per point depending on the driver call |
| Corners | End point of one line = start point of the next, output only **once** | twice, bright dots at corners |
| Short lines | The rounding remainder is passed on to the next line, a polyline gets exactly its length in time | each line rounded on its own |
| Oscilloscope profile | DIGITAL: instant jumps, otherwise a sampling oscilloscope shows intermediate points. ANALOG: braked jumps (see next row). Switched at run time via BOOT button (3 s) or web interface, stored in NVS (`Preferences`, namespace `scopeclock`) | – |
| Jumps (ANALOG) | in 1-unit steps at 24× speed (`JUMP_SPEED`), braking over the last 4 units (`JUMP_LANDING`, `JUMP_RAMP`). The retrace line has approx. 1/24 of the line brightness, old amplifiers still land accurately | instant jump |
| Order | Dial and digits are sorted once at start-up into a round trip with minimal jumps (nearest neighbour + 2-opt), entered where the second hand ends | table order |
| Second hand | drawn out and back (centre ↔ tip) at half brightness each, which removes the long jump from the edge to the centre | – |
| Direction | every polyline is drawn from the nearer end | always from the start of the table |
| Blanking | optional via the oscilloscope's Z input (`ZBLANK_PIN`), then there is no retrace line at all | – |
| Needle hands | hour and minute hands at `HAND_BRIGHT_PCT` (67 %), because their two closely spaced edges merge | twice as bright |
| Table bounds | always checked | reads past the end of the table if an end marker is missing |

Simulation result (trace with approx. 2.4 units beam diameter, brightness relative to the dial
ticks, several times; measured with the earlier vector data and 67 % hand brightness – not yet
repeated for the new graphics):

| | Original v1.0 | v2.0 |
|---|---|---|
| Second hand | 0.80 – 1.00 | 0.92 – 1.07 |
| Minute hand | 1.23 – 1.65 | 0.95 – 1.07 |
| Hour hand | 1.25 – 1.71 | 0.99 – 1.19 |
| Dwell time per length, second hand in all 60 positions | 0.73 – 1.00 | 0.98 – 1.00 |
| Retrace energy per frame (step equivalents, without Z blanking) | – (instant jump) | 56 (previously with simple jumps: 190) |

Where two hands lie exactly on top of each other, the spot is naturally brighter.

**Speed and old CRTs:** one frame takes approx. 1,400 step times; at 3 µs per step that is about
230 frames/s (Roman approx. 160). While writing, the beam moves at approx. 4 mV/µs, during jumps
at up to approx. 100 mV/µs, when arriving at approx. 33 mV/µs. In addition there is a full step
(3 µs) of dwell time at the start of each line in which the beam can settle. This is sufficient
for oscilloscopes from about 0.5 MHz X/Y bandwidth. For very old devices lower `JUMP_SPEED` to 12,
and if needed increase `BEAM_NS_PER_UNIT` (brighter trace, fewer frames/s).

## Oscilloscope picture: table format (`DataTable.h`)

- A point consists of 2 bytes `(x, y)`, range 0–255 = DAC value. `(0x80, 0x80)` is the centre,
  y grows upwards.
- Consecutive points are connected with lines (`beamMove()`, see vector engine).
- `0x00` separates two polylines in dial and digit tables, so `x = 0` never occurs as a
  coordinate there. `0xFF` ends a hand position.
- Hands: 60 precomputed positions each, starting at 12 o'clock, clockwise. Second hand 5 bytes per
  position (tip, centre), minute and hour hand 9 bytes (narrow needle of 4 points). The offset
  `position × length` selects the position. The hour hand advances one step every 12 minutes:
  `hpos = (h % 12) * 5 + m / 12`.
- All tables are generated by `tools/gen_vectors.py`: dial, digits (font selectable with
  `--font segment|kante|raleway`, default 7-segment) and hands. To change shapes, edit the script
  and run `python3 tools/gen_vectors.py > ESP32_OscilloscopeClock_v2.0/DataTable.h`.
- Your own graphics can be added as another table in the same format and drawn in the `scopeTask`
  with `PlotTable(table, sizeof(table), 0x00, 0)`. For checking without an oscilloscope there is
  the `EXCEL` mode in `config.h`: it prints all points to the serial port, and the list can be
  plotted as an XY chart.

## Fine tuning

| Symptom | Adjustment (`config.h` or web interface) |
|---|---|
| Picture flickers | decrease `BEAM_NS_PER_UNIT` (e.g. 2000) |
| Lines too dark | increase `BEAM_NS_PER_UNIT` (e.g. 5000) or increase intensity on the oscilloscope |
| Rounded corners, blurred digits (oscilloscope too slow) | increase `BEAM_NS_PER_UNIT` |
| Small “hooks” at the start of lines | decrease `JUMP_SPEED` (e.g. 12) or `JUMP_LANDING` (e.g. 4) |
| Retrace lines visible between lines | increase `JUMP_SPEED` (32), `0` (instant jump, for fast oscilloscopes) or use Z blanking (`ZBLANK_PIN`) |
| Hands brighter/darker than the dial | adjust `HAND_BRIGHT_PCT` |
| Time wrong by whole hours | check the time zone in the web interface or `TZ_INFO` |
| No internet, time runs from 10:08:37 (start time `FALLBACK_H:M:S`) | web interface → „Einstellungen“ → „Uhrzeit stellen“ (set time) |
| OLED picture shifted by 2 px | in the `.ino` `NONAME0` → `NONAME2` |
| OLED stays dark | check wiring: CLK GPIO18, DIN GPIO23, CS GPIO5, DC GPIO17, RST GPIO4, VCC to 3V3 (see HARDWARE.en.md) |
