#!/usr/bin/env python3
"""
gen_dial_quarters.py – erzeugt die Tabelle DialDigitsRoman4 (römisches
Zifferblatt nur mit XII, III, VI, IX) aus der vorhandenen Tabelle
DialDigitsRoman in DataTable.h.

Jeder Linienzug der römischen Tabelle wird über die Lage seines Schwerpunkts
(Winkel zur Bildmitte) einer Stunde zugeordnet; übernommen werden nur die
Linienzüge der Stunden 12, 3, 6 und 9 – unverändert, damit die Ziffern genau
wie im vollen römischen Zifferblatt aussehen.

Das arabische Gegenstück (12, 3, 6, 9) ist die Originaltabelle DialDigitsMin.

Aufruf:  python3 tools/gen_dial_quarters.py [--check]
         gibt den C-Block auf stdout aus (in DataTable.h einfügen/ersetzen);
         --check zeigt nur die Zuordnung Linienzug → Stunde.
"""
import math, re, sys, pathlib

TABLE = pathlib.Path(__file__).resolve().parent.parent / 'ESP32_OscilloscopeClock_v2.0' / 'DataTable.h'
KEEP = {12, 3, 6, 9}


def load(name):
    src = TABLE.read_text()
    m = re.search(r'byte\s+' + name + r'\s*\[\]\s*=\s*\{(.*?)\};', src, re.S)
    return [int(v, 16) for v in re.findall(r'0x[0-9a-fA-F]+', m.group(1))]


def polylines(t):
    """Linienzüge wie nextStroke() in der Firmware (Trenner 0x00, Ende 0xFF)."""
    out, i = [], 0
    while i + 1 < len(t) and t[i] != 0xFF:
        if t[i] == 0x00:
            i += 1
            continue
        e = i
        while e + 1 < len(t) and t[e] not in (0x00, 0xFF):
            e += 2
        out.append(t[i:e])
        i = e
    return out


def hour_of(poly):
    xs, ys = poly[0::2], poly[1::2]
    cx, cy = sum(xs) / len(xs) - 128, sum(ys) / len(ys) - 128
    ang = math.degrees(math.atan2(cx, cy)) % 360          # 0° = oben, im Uhrzeigersinn
    h = round(ang / 30) % 12
    return 12 if h == 0 else h


def main():
    polys = polylines(load('DialDigitsRoman'))
    if '--check' in sys.argv:
        for p in polys:
            print(hour_of(p), p)
        return
    data = []
    for p in polys:
        if hour_of(p) in KEEP:
            data += p + [0x00]
    data[-1] = 0xFF
    print('// Erzeugt mit tools/gen_dial_quarters.py aus DialDigitsRoman – nur XII, III, VI, IX')
    print('byte DialDigitsRoman4[] = {')
    for i in range(0, len(data), 20):
        print('                ' + ','.join(f'0x{v:02x}' for v in data[i:i + 20]) + ',')
    print('};')


if __name__ == '__main__':
    main()
