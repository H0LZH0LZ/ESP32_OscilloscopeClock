#!/usr/bin/env python3
"""
gen_vectors.py – erzeugt alle Vektortabellen für DataTable.h in eigener Gestaltung

  DialData          Minutenstriche (60), Stunden länger, Viertelstunden am längsten
  DialDigits12      Ziffern 1–12        (7-Segment-Stil; Alternativen: --font)
  DialDigitsMin     nur 12, 3, 6, 9
  DialDigitsRoman   römisch I–XII       (serifenlos, 4 als "IIII")
  DialDigitsRoman4  nur XII, III, VI, IX
  HrPtrData         Stundenzeiger, 60 Stellungen × 9 Byte (Nadel, 4 Punkte + 0xFF)
  MinPtrData        Minutenzeiger, 60 Stellungen × 9 Byte
  SecPtrData        Sekundenzeiger, 60 Stellungen × 5 Byte

Format wie in DataTable.h: Punkte (x, y) je 1 Byte, (128,128) = Mitte, y nach
oben; 0x00 trennt Linienzüge, 0xFF beendet Tabelle bzw. Zeigerstellung.
Koordinaten liegen immer in 2…253, damit 0x00/0xFF nie als Wert vorkommen.

Aufruf:  python3 tools/gen_vectors.py > ESP32_OscilloscopeClock_v2.0/DataTable.h
         python3 tools/gen_vectors.py --json     → dieselben Daten als JSON (Vorschau)
         --font kante|raleway|segment            → Schrift der Ziffern 1–12 wählen
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


# ---------------------------------------------------------------- Ziffern 1–12
# Drei Schriften zur Auswahl (Aufruf mit --font kante|raleway|segment).
# Alle Glyphen: Höhe 16, y nach oben; Koordinaten dürfen Kommazahlen sein,
# gerundet wird erst beim Platzieren auf dem Zifferblatt.
H = 16


def arc(cx, cy, rx, ry, a0, a1, n):
    """Ellipsenbogen von a0 nach a1 (Grad, 0° = rechts, gegen den Uhrzeigersinn)."""
    return [(cx + rx * math.cos(math.radians(a0 + (a1 - a0) * i / n)),
             cy + ry * math.sin(math.radians(a0 + (a1 - a0) * i / n))) for i in range(n + 1)]


# Schrift "kante": eckig, abgeschrägte Ecken, Breite 8
KANTE = {
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
KANTE_W = {c: (5 if c == '1' else 8) for c in KANTE}

# Schrift "raleway": geometrische Einstrich-Nachbildung im Stil von Raleway
# (runde Bögen, Fähnchen an der 1), Breite 11
_six = [(8.5, 16), (5.2, 13.2), (2.4, 9.6), (0.8, 6.2)] + arc(5.5, 5, 5, 5, 200, 200 - 360, 16)
RALEWAY = {
    '0': [arc(5.5, 8, 5.5, 8, 90, 450, 20)],
    '1': [[(1.5, 13), (5, 16), (5, 0)]],
    '2': [arc(5.5, 11.3, 5, 4.7, 155, -40, 8) + [(0.5, 0), (11, 0)]],
    '3': [arc(5.3, 12.2, 4.6, 3.8, 155, -90, 8) + arc(5.5, 4.4, 5.5, 4.4, 90, -200, 10)],
    '4': [[(8, 0), (8, 16), (0.3, 4.5), (11, 4.5)]],
    '5': [[(10, 16), (1.6, 16), (0.9, 8.4)] + arc(5.6, 5, 5.2, 5, 140, -150, 10)],
    '6': [_six],
    '7': [[(0.5, 16), (11, 16), (3.5, 0)]],
    '8': [arc(5.5, 12.1, 4.4, 3.9, 270, -90, 14) + arc(5.5, 4.2, 5.3, 4.2, 90, 450, 16)],
    '9': [[(11 - x, 16 - y) for x, y in _six]],
}
RALEWAY_W = {c: (6 if c == '1' else 11) for c in RALEWAY}

# Schrift "segment": 7-Segment-Anzeige mit kleinen Lücken, Breite 8
_SEG = {                                  # Höhe 20, Breite 10 (größer als die anderen Schriften)
    'a': [(1, 20), (9, 20)], 'b': [(10, 19), (10, 11)], 'c': [(10, 9), (10, 1)],
    'd': [(9, 0), (1, 0)],   'e': [(0, 1), (0, 9)],     'f': [(0, 11), (0, 19)],
    'g': [(1, 10), (9, 10)],
}
_SEGS = {'0': 'abcdef', '1': 'bc', '2': 'abged', '3': 'abgcd', '4': 'fgbc',
         '5': 'afgcd', '6': 'afgedc', '7': 'abc', '8': 'abcdefg', '9': 'gfabcd'}
SEGMENT = {c: [_SEG[s] for s in segs] for c, segs in _SEGS.items()}
SEGMENT['1'] = [[(0, 19), (0, 11)], [(0, 9), (0, 1)]]          # schmale 1, ohne Leerraum
SEGMENT_W = {c: (0 if c == '1' else 10) for c in SEGMENT}

# Schrift: (Glyphen, Breiten, Abstand, Höhe, Radius der Ziffernmitte)
FONTS = {'kante': (KANTE, KANTE_W, 3, 16, 94), 'raleway': (RALEWAY, RALEWAY_W, 2, 16, 94),
         'segment': (SEGMENT, SEGMENT_W, 5, 20, 91)}
FONT = 'segment'                          # gewählt: 7-Segment-Stil
for i, a in enumerate(sys.argv):
    if a == '--font':
        FONT = sys.argv[i + 1]

# ---------------------------------------------------------------- römisch, serifenlos
RH = 20                                   # gleiche Höhe wie die 7-Segment-Ziffern
ROMAN = {
    'I': (0, [[(0, 0), (0, RH)]]),
    'V': (10, [[(0, RH), (5, 0), (10, RH)]]),
    'X': (10, [[(0, 0), (10, RH)], [(0, RH), (10, 0)]]),
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
            polys.append([(round(x0 + x), round(y0 + y)) for x, y in p])
        x0 += w + gap
    return polys


def arabic(hours):
    glyphs, widths, gap, height, radius = FONTS[FONT]
    g = {c: (widths[c], glyphs[c]) for c in glyphs}
    polys = []
    for h in hours:
        cx, cy = pt(radius, h * 30)
        polys += place(g, str(h), cx, cy, height, gap)
    return polys


def roman(hours, radius=91):                 # gleicher Radius wie die 7-Segment-Ziffern
    polys = []
    for h in hours:
        cx, cy = pt(radius, h * 30)
        polys += place(ROMAN, ROMAN_NUM[h], cx, cy, RH, 3)
    return polys


# ---------------------------------------------------------------- Zeiger
def hand_needle(length, half_width):
    """Schmale Nadel: Basis quer durch die Mitte (±half_width), Spitze bei length.
    4 Punkte (Basis links, Spitze, Basis rechts, Basis links) = 8 Byte + 0xFF.
    Beginnt und endet an der Mitte – kein Überstand, keine Sprünge zwischen den Zeigern."""
    return [[pt(half_width, pos * 6 - 90), pt(length, pos * 6), pt(half_width, pos * 6 + 90),
             pt(half_width, pos * 6 - 90)] for pos in range(60)]


def hand_line(length):
    """Sekundenzeiger: Spitze → Mitte (2 Punkte = 4 Byte + 0xFF)."""
    return [[pt(length, pos * 6), (C, C)] for pos in range(60)]


def hand_table(polys_per_pos):
    out = []
    for p in check(polys_per_pos):
        for x, y in p:
            out += [x, y]
        out.append(0xFF)
    return out


# ---------------------------------------------------------------- Testbild
def test_pattern():
    """Testbild zum Einstellen des Oszilloskops: Rahmen (Bildgrenzen), Kreis
    (Seitenverhältnis X/Y: muss rund sein), Mittelkreuz (Position) und je eine
    kurze Marke an den Viertelpunkten des Rahmens."""
    polys = [[(8, 8), (247, 8), (247, 247), (8, 247), (8, 8)]]              # Rahmen
    polys.append([pt(100, a * 5) for a in range(73)])                       # Kreis r = 100
    polys += [[(98, 128), (158, 128)], [(128, 98), (128, 158)]]             # Mittelkreuz
    for v in (68, 188):                                                     # Marken auf dem Rahmen
        polys += [[(v, 8), (v, 20)], [(v, 247), (v, 235)], [(8, v), (20, v)], [(247, v), (235, v)]]
    return polys


# ---------------------------------------------------------------- Ausgabe
TABLES = {
    'DialData':         table(dial()),
    'DialDigits12':     table(arabic(range(1, 13))),
    'DialDigitsMin':    table(arabic([12, 3, 6, 9])),
    'DialDigitsRoman':  table(roman(range(1, 13))),
    'DialDigitsRoman4': table(roman([12, 3, 6, 9])),
    'DialDigitsNone':   [0xFF],                       # Zifferblatt nur mit Strichen (keine Ziffern)
    'TestData':         table(test_pattern()),
    'HrPtrData':        hand_table(hand_needle(60, 2)),
    'MinPtrData':       hand_table(hand_needle(100, 2)),
    'SecPtrData':       hand_table(hand_line(108)),
}

HEADER = """/*********************************************************************************
  DataTable.h – Vektordaten für das Oszilloskop-Bild

  ERZEUGT mit tools/gen_vectors.py – nicht von Hand ändern, sondern das Skript
  anpassen und neu ausführen:
      python3 tools/gen_vectors.py > ESP32_OscilloscopeClock_v2.0/DataTable.h

  Gezeichnet werden die Tabellen mit PlotTable() bzw. der Rundtour in der .ino.

  Koordinaten
  -----------
  Jeder Punkt besteht aus 2 Bytes (x, y), 0…255 = DAC-Ausgangswert.
  (128, 128) ist die Bildmitte, y wächst nach oben (12 Uhr oben).
  Aufeinanderfolgende Punkte werden durch Linien verbunden. Alle Werte liegen
  in 2…253, damit 0x00 und 0xFF nie als Koordinate vorkommen.

  Trenner
  -------
  0x00  beendet bei Zifferblatt/Ziffern einen Linienzug, danach beginnt der
        nächste (der Strahl springt dorthin).
  0xFF  beendet bei den Zeigern eine Zeigerstellung bzw. die Tabelle.

  Tabellen
  --------
  DialData         60 Minutenstriche, Stunden länger, Viertelstunden am längsten.
                   Wird immer gezeichnet.
  DialDigits12     Ziffern 1–12, 7-Segment-Stil              (DIAL_STYLE 0, Standard)
  DialDigitsMin    nur 12, 3, 6, 9, 7-Segment-Stil           (DIAL_STYLE 1)
  DialDigitsRoman  römisch I–XII, serifenlos, 4 als "IIII"   (DIAL_STYLE 2)
  DialDigitsRoman4 nur XII, III, VI, IX                      (DIAL_STYLE 3)
  DialDigitsNone   keine Ziffern, nur Minutenstriche          (DIAL_STYLE 4)
  TestData         Testbild: Rahmen, Kreis, Mittelkreuz, Marken (Einstellungen → Testbild)
  HrPtrData        Stundenzeiger: 60 Stellungen × 9 Byte
                   = schmale Nadel, 4 Punkte (Basis links an der Mitte, Spitze,
                   Basis rechts, Basis links) + 0xFF
  MinPtrData       Minutenzeiger: wie HrPtrData, länger
  SecPtrData       Sekundenzeiger: 60 Stellungen × 5 Byte
                   = 2 Punkte (Spitze, Mitte) + 0xFF

  Stellung n (0 = 12 Uhr, im Uhrzeigersinn) beginnt bei Byte-Offset n × 9
  (Stunden/Minuten) bzw. n × 5 (Sekunden).

  Die Tabellen sind bewusst NICHT const/PROGMEM und liegen damit im internen
  RAM (DRAM). Lesezugriffe dauern so immer gleich lang (keine Flash-Cache-
  Fehlzugriffe) – wichtig für das exakte Timing der Strahlausgabe.
*********************************************************************************/
"""

if __name__ == '__main__':
    if '--json' in sys.argv:
        print(json.dumps(TABLES))
        sys.exit()
    print(HEADER)
    for name, data in TABLES.items():
        print(f'byte {name}[] = {{')
        for i in range(0, len(data), 20):
            print('  ' + ','.join(f'0x{v:02x}' for v in data[i:i + 20]) + ',')
        print('};\n')
