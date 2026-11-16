#!/usr/bin/env python3
"""
gen_icon.py – erzeugt ESP32_OscilloscopeClock_v2.0/icon.h: App-Symbol und Manifest
für „Zum Startbildschirm hinzufügen“ (Android/iOS).

  /icon.svg        Vektor-Symbol (Browser-Tab, Android)
  /icon.png        192 × 192 PNG (iOS „apple-touch-icon“, Android)
  /manifest.json   Name, Farben, Symbole, Start-Adresse, Vollbild ohne Browserleiste

Motiv: grüner Oszilloskop-Schirm mit Minutenstrichen und zwei Zeigern (10:08).
Benötigt: Pillow (pip install pillow)
Aufruf:   python3 tools/gen_icon.py > ESP32_OscilloscopeClock_v2.0/icon.h
"""
import io
import math
from PIL import Image, ImageDraw

BG, TRACE = (8, 20, 12), (80, 255, 130)

def svg():
    c, r = 50, 38
    ticks = []
    for i in range(12):
        a = math.radians(i * 30)
        r0 = r - (7 if i % 3 == 0 else 4)
        ticks.append(f'M{c + r0 * math.sin(a):.1f} {c - r0 * math.cos(a):.1f}L{c + r * math.sin(a):.1f} {c - r * math.cos(a):.1f}')
    def hand(deg, ln):
        a = math.radians(deg)
        return f'M{c} {c}L{c + ln * math.sin(a):.1f} {c - ln * math.cos(a):.1f}'
    path = ''.join(ticks) + hand(304, 20) + hand(48, 30)
    return ('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">'
            f'<rect width="100" height="100" rx="22" fill="#08140c"/>'
            f'<path d="{path}" stroke="#50ff82" stroke-width="3.2" stroke-linecap="round" fill="none"/>'
            f'<circle cx="{c}" cy="{c}" r="2.6" fill="#50ff82"/></svg>')

def png(size=192):
    S = 4 * size                                   # 4-fach zeichnen, dann verkleinern (Kantenglättung)
    im = Image.new('RGB', (S, S), BG)
    d = ImageDraw.Draw(im)
    k = S / 100
    c, r, w = 50 * k, 38 * k, int(3.4 * k)
    for i in range(12):
        a = math.radians(i * 30)
        r0 = r - (7 if i % 3 == 0 else 4) * k
        d.line([(c + r0 * math.sin(a), c - r0 * math.cos(a)), (c + r * math.sin(a), c - r * math.cos(a))], fill=TRACE, width=w)
    for deg, ln in ((304, 20), (48, 30)):
        a = math.radians(deg)
        d.line([(c, c), (c + ln * k * math.sin(a), c - ln * k * math.cos(a))], fill=TRACE, width=w)
    d.ellipse([c - 2.8 * k, c - 2.8 * k, c + 2.8 * k, c + 2.8 * k], fill=TRACE)
    im = im.resize((size, size), Image.LANCZOS).quantize(colors=32)
    b = io.BytesIO(); im.save(b, 'PNG', optimize=True)
    return b.getvalue()

MANIFEST = ('{"name":"ESP32 Oscilloscope Clock","short_name":"Scope Clock","start_url":"/","scope":"/",'
            '"display":"standalone","background_color":"#000000","theme_color":"#0a7d44",'
            '"icons":[{"src":"/icon.svg","sizes":"any","type":"image/svg+xml"},'
            '{"src":"/icon.png","sizes":"192x192","type":"image/png"}]}')

if __name__ == '__main__':
    data = png()
    out = ['// ---- Erzeugt mit tools/gen_icon.py – nicht von Hand ändern ----',
           '// App-Symbol und Manifest für „Zum Startbildschirm hinzufügen“.',
           '#pragma once',
           f'static const char ICON_SVG[] PROGMEM = R"SVG({svg()})SVG";',
           f'static const char MANIFEST_JSON[] PROGMEM = R"JSON({MANIFEST})JSON";',
           f'// icon.png, 192 x 192, {len(data)} Byte',
           f'static const uint8_t ICON_PNG[{len(data)}] PROGMEM = {{']
    for i in range(0, len(data), 24):
        out.append('  ' + ','.join(f'0x{x:02x}' for x in data[i:i + 24]) + ',')
    out.append('};')
    print('\n'.join(out))
