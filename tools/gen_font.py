#!/usr/bin/env python3
"""
gen_font.py – erzeugt ESP32_OscilloscopeClock_v2.0/font.h (Schrift des Webinterfaces)

Grundlage ist Raleway (SIL Open Font License 1.1, siehe fonts/OFL.txt):
  https://fonts.google.com/specimen/Raleway  ·  Datei "Raleway[wght].ttf" aus google/fonts

Die Schrift wird für die Uhr verkleinert, damit sie in den Flash passt und die
Seite auch ohne Internet (Setup-WLAN) richtig aussieht:
  - nur Strichstärken 400–700 (variable Schrift, eine Datei für alle Stärken)
  - nur die Zeichen, die in webpage.h und der Firmware vorkommen, plus
    ASCII, Umlaute und gängige Satzzeichen
  - WOFF2-komprimiert
Weil das eine veränderte Fassung ist, verlangt die OFL einen anderen Namen als
den reservierten Namen "Raleway": Die Schrift heißt hier "ScopeClock Sans".

Benötigt: pip install fonttools brotli
Aufruf:   python3 tools/gen_font.py "Raleway[wght].ttf" > ESP32_OscilloscopeClock_v2.0/font.h
"""
import io
import sys
from pathlib import Path

from fontTools.ttLib import TTFont
from fontTools.varLib import instancer
from fontTools import subset

ROOT = Path(__file__).resolve().parent.parent
NAME = 'ScopeClock Sans'


def chars():
    c = set()
    for f in ('webpage.h', 'ESP32_OscilloscopeClock_v2.0.ino'):
        c |= set((ROOT / 'ESP32_OscilloscopeClock_v2.0' / f).read_text(encoding='utf-8'))
    c |= {chr(i) for i in range(32, 127)} | set('äöüÄÖÜßéèàçñ€°–—„“”‚‘’…·×')
    return ''.join(sorted(ch for ch in c if ord(ch) >= 32))


def build(src):
    font = instancer.instantiateVariableFont(TTFont(src), {'wght': (400, 700)})
    opt = subset.Options()
    opt.layout_features = ['kern', 'liga', 'lnum']
    opt.flavor = 'woff2'
    opt.name_IDs = [0, 1, 2, 3, 4, 5, 6, 13, 14, 16, 17]
    sub = subset.Subsetter(opt)
    sub.populate(text=chars())
    sub.subset(font)
    for rec in font['name'].names:                 # neuer Name (OFL: Reserved Font Name)
        if rec.nameID in (1, 4, 16):
            rec.string = NAME
        elif rec.nameID == 6:
            rec.string = NAME.replace(' ', '')
        elif rec.nameID == 3:
            rec.string = NAME + ' (Raleway subset)'
    buf = io.BytesIO()
    font.flavor = 'woff2'
    font.save(buf)
    return buf.getvalue()


if __name__ == '__main__':
    data = build(sys.argv[1])
    out = ['// ---- Erzeugt mit tools/gen_font.py – nicht von Hand ändern ----',
           f'// "{NAME}": verkleinerte Fassung von Raleway, Copyright 2010 The Raleway',
           '// Project Authors, SIL Open Font License 1.1 (Text: fonts/OFL.txt).',
           f'// {len(data)} Byte WOFF2, ausgeliefert unter /font.woff2',
           '#pragma once',
           f'static const uint8_t FONT_WOFF2[{len(data)}] PROGMEM = {{']
    for i in range(0, len(data), 24):
        out.append('  ' + ','.join(f'0x{b:02x}' for b in data[i:i + 24]) + ',')
    out.append('};')
    print('\n'.join(out))
