#!/usr/bin/env python3
"""
gen_vectors.py – erzeugt alle Vektortabellen für DataTable.h in eigener Gestaltung

  DialData          Minutenstriche (60), Stunden länger, Viertelstunden am längsten
  DialDigits12      Ziffern 1–12        (eigene Strichschrift, abgeschrägte Ecken)
  DialDigitsMin     nur 12, 3, 6, 9
  DialDigitsRoman   römisch I–XII       (serifenlos, 4 als "IIII")
  DialDigitsRoman4  nur XII, III, VI, IX
  HrPtrData         Stundenzeiger, 60 Stellungen × 9 Byte (Dreieck, 4 Punkte + 0xFF)
  MinPtrData        Minutenzeiger, 60 Stellungen × 9 Byte
  SecPtrData        Sekundenzeiger mit Gegengewicht, 60 Stellungen × 5 Byte

Format wie in DataTable.h: Punkte (x, y) je 1 Byte, (128,128) = Mitte, y nach
oben; 0x00 trennt Linienzüge, 0xFF beendet Tabelle bzw. Zeigerstellung.
Koordinaten liegen immer in 2…253, damit 0x00/0xFF nie als Wert vorkommen.

Aufruf:  python3 tools/gen_vectors.py            → C-Block für DataTable.h auf stdout
         python3 tools/gen_vectors.py --json     → dieselben Daten als JSON (Vorschau)
"""
import json
import math
import sys

C = 128                                   # Bildmitte

# ---------------------------------------------------------------- Hilfen
def pt(r, deg):
    """Punkt auf Radius r, Winkel deg (0° = oben, im Uhrzeigersinn)."""
    a = math.radians(deg)
    return (round(C + r * math.sin(a)), round(C + r * math.cos(a)))


def check(polys):
    for p in polys:
        for x, y in p:
            assert 2 <= x <= 253 and 2 <= y <= 253, (x, y)
    return polys


def table(polys):
    """Linienzüge → Bytefolge mit 0x00-Trennern und 0xFF am Ende."""
    out = []
    for p in check(polys):
        for x, y in p:
            out += [x, y]
        out.append(0x00)
    out[-1] = 0xFF
    return out


# ---------------------------------------------------------------- Zifferblatt
def dial():
    polys = []
    for i in range(60):
        deg = i * 6
        if i % 15 == 0:   r0 = 108          # Viertelstunden
        elif i % 5 == 0:  r0 = 113          # Stunden
        else:             r0 = 121          # Minuten
        polys.append([pt(125, deg), pt(r0, deg)])
    return polys


# ---------------------------------------------------------------- Strichschrift 1–12
H = 16                                    # Ziffernhöhe
DIGIT = {                                 # Breite 8, y nach oben, Ecken abgeschrägt
    '0': [[(2, 0), (6, 0), (8, 2), (8, 14), (6, 16), (2, 16), (0, 14), (0, 2), (2, 0)]],
    '1': [[(1, 13), (4, 16), (4, 0)]],
    '2': [[(0, 14), (2, 16), (6, 16), (8, 14), (8, 10), (0, 2), (0, 0), (8, 0)]],
    '3': [[(0, 14), (2, 16), (6, 16), (8, 14), (8, 10), (6, 8), (3, 8), (6, 8),
           (8, 6), (8, 2), (6, 0), (2, 0), (0, 2)]],
    '4': [[(6, 0), (6, 16), (0, 5), (8, 5)]],
    '5': [[(8, 16), (0, 16), (0, 9), (6, 9), (8, 7), (8, 2), (6, 0), (2, 0), (0, 2)]],
    '6': [[(7, 16), (3, 16), (0, 12), (0, 2), (2, 0), (6, 0), (8, 2), (8, 7), (6, 9), (0, 9)]],
    '7': [[(0, 16), (8, 16), (8, 14), (3, 0)]],
    '8': [[(2, 8), (0, 10), (0, 14), (2, 16), (6, 16), (8, 14), (8, 10), (6, 8), (2, 8),
           (0, 6), (0, 2), (2, 0), (6, 0), (8, 2), (8, 6), (6, 8)]],
    '9': [[(8, 7), (2, 7), (0, 9), (0, 14), (2, 16), (6, 16), (8, 14), (8, 4), (5, 0), (1, 0)]],
}
DIGIT_W = {c: (5 if c == '1' else 8) for c in DIGIT}

# ---------------------------------------------------------------- römisch, serifenlos
RH = 14
ROMAN = {
    'I': (0, [[(0, 0), (0, RH)]]),
    'V': (8, [[(0, RH), (4, 0), (8, RH)]]),
    'X': (8, [[(0, 0), (8, RH)], [(0, RH), (8, 0)]]),
}
ROMAN_NUM = {1: 'I', 2: 'II', 3: 'III', 4: 'IIII', 5: 'V', 6: 'VI', 7: 'VII',
             8: 'VIII', 9: 'IX', 10: 'X', 11: 'XI', 12: 'XII'}


def place(glyphs, text, cx, cy, height, gap):
    """Zeichenkette zentriert auf (cx, cy) setzen (ganzzahlig)."""
    width = sum(glyphs[c][0] for c in text) + gap * (len(text) - 1)
    x0 = round(cx - width / 2)
    y0 = round(cy - height / 2)
    polys = []
    for c in text:
        w, ps = glyphs[c]
        for p in ps:
            polys.append([(x0 + x, y0 + y) for x, y in p])
        x0 += w + gap
    return polys


def arabic(hours, radius=94):
    g = {c: (DIGIT_W[c], DIGIT[c]) for c in DIGIT}
    polys = []
    for h in hours:
        cx, cy = pt(radius, h * 30)
        polys += place(g, str(h), cx, cy, H, 3)
    return polys


def roman(hours, radius=95):
    polys = []
    for h in hours:
        cx, cy = pt(radius, h * 30)
        polys += place(ROMAN, ROMAN_NUM[h], cx, cy, RH, 3)
    return polys


# ---------------------------------------------------------------- Zeiger
def hand_triangle(length, half_width, tail):
    """Schlankes Dreieck: Basis hinter der Mitte (tail), Spitze bei length.
    4 Punkte (Basis links, Spitze, Basis rechts, Basis links) = 8 Byte + 0xFF."""
    out = []
    for pos in range(60):
        deg = pos * 6
        bl = pt(math.hypot(tail, half_width), deg + 180 - math.degrees(math.atan2(half_width, tail)))
        br = pt(math.hypot(tail, half_width), deg + 180 + math.degrees(math.atan2(half_width, tail)))
        tip = pt(length, deg)
        out.append([bl, tip, br, bl])
    return out


def hand_line(length, tail):
    """Sekundenzeiger: Spitze → Gegengewicht (2 Punkte = 4 Byte + 0xFF)."""
    return [[pt(length, pos * 6), pt(tail, pos * 6 + 180)] for pos in range(60)]


def hand_table(polys_per_pos):
    out = []
    for p in check(polys_per_pos):
        for x, y in p:
            out += [x, y]
        out.append(0xFF)
    return out


# ---------------------------------------------------------------- Ausgabe
TABLES = {
    'DialData':         table(dial()),
    'DialDigits12':     table(arabic(range(1, 13))),
    'DialDigitsMin':    table(arabic([12, 3, 6, 9])),
    'DialDigitsRoman':  table(roman(range(1, 13))),
    'DialDigitsRoman4': table(roman([12, 3, 6, 9])),
    'HrPtrData':        hand_table(hand_triangle(62, 4, 8)),
    'MinPtrData':       hand_table(hand_triangle(98, 3, 10)),
    'SecPtrData':       hand_table(hand_line(108, 20)),
}

if __name__ == '__main__':
    if '--json' in sys.argv:
        print(json.dumps(TABLES))
        sys.exit()
    print('// ---- Erzeugt mit tools/gen_vectors.py – nicht von Hand ändern ----')
    for name, data in TABLES.items():
        print(f'byte {name}[] = {{')
        for i in range(0, len(data), 20):
            print('                ' + ','.join(f'0x{v:02x}' for v in data[i:i + 20]) + ',')
        print('};\n')
