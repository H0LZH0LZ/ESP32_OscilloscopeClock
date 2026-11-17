#!/usr/bin/env python3
"""
gen_font.py – erzeugt ESP32_OscilloscopeClock_v2.0/font.h (Schrift des Webinterfaces)

Grundlage ist Raleway (SIL Open Font License 1.1, siehe fonts/OFL.txt):
  https://fonts.google.com/specimen/Raleway  ·  Datei "Raleway[wght].ttf" aus google/fonts,
  im Projekt als fonts/Raleway-VF.ttf (dieselbe Datei nutzt auch die PDF-Dokumentation)

Die Schrift wird für die Uhr verkleinert, damit sie in den Flash passt und die
Seite auch ohne Internet (Setup-WLAN) richtig aussieht:
  - zwei feste Schnitte: Light (300) für Text und große Uhrzeit, SemiBold (600)
    für Überschriften und Knöpfe – der schlanke Light-Schnitt ist typisch für Raleway. Keine variable Schrift –
    manche Android-Browser zeigen fette Texte einer variablen Schrift sonst in
    der Systemschrift an.
  - fester Zeichenvorrat (CHARSET: ASCII, Umlaute, einige Akzente, Einheiten,
    gängige Satzzeichen). Dadurch hängt font.h nur von der Schriftdatei ab und ist
    reproduzierbar – Textänderungen an der Webseite ändern die Schrift nicht.
    Kommt in web/index.html ein Zeichen vor, das Raleway hat, CHARSET aber
    nicht, bricht das Skript mit einem Hinweis ab.
  - WOFF2-komprimiert
Weil das eine veränderte Fassung ist, verlangt die OFL einen anderen Namen als
den reservierten Namen "Raleway": Die Schrift heißt hier "ScopeClock Sans".

Benötigt: pip install fonttools brotli
Aufruf:   python3 tools/gen_font.py > ESP32_OscilloscopeClock_v2.0/font.h
          (optional mit anderer Quelldatei: python3 tools/gen_font.py "Raleway[wght].ttf" > …)
"""
import io
import sys
from pathlib import Path

from fontTools.ttLib import TTFont
from fontTools.varLib import instancer
from fontTools import subset

ROOT = Path(__file__).resolve().parent.parent
NAME = 'ScopeClock Sans'


SRC = ROOT / 'fonts' / 'Raleway-VF.ttf'
# Fester Zeichenvorrat: ASCII, Umlaute und einige Akzente, Einheiten und gängige Satzzeichen
# (bewusst klein – jedes Zeichen kostet Flash). Neue Zeichen hier ergänzen; check() meldet sie.
CHARSET = (''.join(chr(i) for i in range(0x20, 0x7F)) + '\u00a0'
           + 'äöüÄÖÜßéèàçñ€°µ²³±§«»‹›‐‑–—‘’‚“”„•…·×−≈≤≥←→✓')


def chars():
    return ''.join(sorted(set(CHARSET)))


def check(src):
    """Alle Zeichen der Webseite, die die Schrift kennt, müssen im Zeichenvorrat sein."""
    cmap = TTFont(src).getBestCmap()
    used = set((ROOT / 'web' / 'index.html').read_text(encoding='utf-8'))
    missing = sorted(c for c in used if ord(c) >= 32 and ord(c) in cmap and c not in CHARSET)
    if missing:
        sys.exit('gen_font.py: Zeichen fehlen in CHARSET: ' + ' '.join(missing))


def build(src, weight):
    font = instancer.instantiateVariableFont(TTFont(src), {'wght': weight})
    opt = subset.Options()
    opt.layout_features = ['kern', 'liga', 'lnum']
    opt.flavor = 'woff2'
    opt.name_IDs = [0, 1, 2, 3, 4, 5, 6, 13, 14, 16, 17]
    sub = subset.Subsetter(opt)
    sub.populate(text=chars())
    sub.subset(font)
    style = 'Light' if weight == 300 else 'SemiBold'
    for rec in font['name'].names:                 # neuer Name (OFL: Reserved Font Name)
        if rec.nameID in (1, 16):
            rec.string = NAME
        elif rec.nameID == 4:
            rec.string = NAME + ' ' + style
        elif rec.nameID == 6:
            rec.string = NAME.replace(' ', '') + '-' + style
        elif rec.nameID == 3:
            rec.string = NAME + ' ' + style + ' (Raleway subset)'
    buf = io.BytesIO()
    font.flavor = 'woff2'
    font.recalcTimestamp = False                   # kein Zeitstempel → gleiche Eingabe, gleiche Datei
    font['head'].modified = font['head'].created
    font.save(buf)
    return buf.getvalue()


if __name__ == '__main__':
    out = ['// ---- Erzeugt mit tools/gen_font.py – nicht von Hand ändern ----',
           f'// "{NAME}": verkleinerte Fassung von Raleway, Copyright 2010 The Raleway',
           '// Project Authors, SIL Open Font License 1.1 (Text: fonts/OFL.txt).',
           '// Zwei Schnitte als WOFF2: /font.woff2 (300, Light) und /font-600.woff2 (600, SemiBold).',
           '#pragma once']
    src = sys.argv[1] if len(sys.argv) > 1 else str(SRC)
    check(src)
    for name, w in (('FONT_WOFF2', 300), ('FONT_WOFF2_600', 600)):
        data = build(src, w)
        out.append(f'// Gewicht {w}: {len(data)} Byte')
        out.append(f'static const uint8_t {name}[{len(data)}] PROGMEM = {{')
        for i in range(0, len(data), 24):
            out.append('  ' + ','.join(f'0x{b:02x}' for b in data[i:i + 24]) + ',')
        out.append('};')
    print('\n'.join(out))
