#!/usr/bin/env python3
"""
number_docs.py – nummeriert die Unterkapitel der Dokumentation und baut das Inhaltsverzeichnis.

  docs/dokumentation.html, docs/documentation.en.html
    <h2>6 Oszilloskop einstellen</h2>  →  Unterkapitel  <h3>6.1 …</h3>, <h3>6.2 …</h3>, …
    Inhaltsverzeichnis (<section class="toc">): je Kapitel eine Zeile, darunter klein die
    Unterkapitel mit Nummer (zwischen <!-- TOC --> und <!-- /TOC -->).

Das Skript ist wiederholbar: vorhandene Nummern werden vorher entfernt. Es läuft automatisch
mit node tools/gen_pdf.js; die Projektseite übernimmt die Nummern über tools/gen_site.py.
Verweise im Text („Abschnitt 6.3“) werden von Hand gepflegt.
"""
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FILES = ['docs/dokumentation.html', 'docs/documentation.en.html']


def number(path):
    s = open(os.path.join(ROOT, path), encoding='utf-8').read()
    start = s.index('<!-- /TOC -->')
    end = s.index('<section class="bohrseite">')
    body = s[start:end]
    toc, chap, sub = [], None, 0

    def h(m):
        nonlocal chap, sub
        level, attrs, inner = m.group(1), m.group(2), m.group(3)
        if level == '2':
            mm = re.match(r'(\d+) (.*)', inner, re.S)
            chap, sub = (mm.group(1), 0) if mm else (None, 0)
            if mm:
                toc.append([chap, mm.group(2).strip(), []])
            return m.group(0)
        if chap is None:
            return m.group(0)
        sub += 1
        inner = re.sub(r'^\d+\.\d+\s+', '', inner.strip())
        num = f'{chap}.{sub}'
        toc[-1][2].append((num, inner))
        return f'<h3{attrs}>{num} {inner}</h3>'

    body = re.sub(r'<h([23])([^>]*)>(.*?)</h\1>', h, body, flags=re.S)
    s = s[:start] + body + s[end:]

    # Inhaltsverzeichnis: Kapitel als Liste, Unterkapitel klein in einer Zeile darunter
    items = []
    for n, title, subs in toc:
        line = f'  <li><span class="tn">{n}</span>{title}'
        if subs:
            line += '<div class="tsub">' + ' · '.join(f'{num} {t}' for num, t in subs) + '</div>'
        items.append(line + '</li>')
    a, b = s.index('<!-- TOC -->') + len('<!-- TOC -->'), s.index('<!-- /TOC -->')
    s = s[:a] + '\n <ol class="tl">\n' + '\n'.join(items) + '\n' + s[s.index('  <li class="bp">', a):b] + s[b:]
    open(os.path.join(ROOT, path), 'w', encoding='utf-8').write(s)
    return sum(len(t[2]) for t in toc)


if __name__ == '__main__':
    for f in FILES:
        print(f'{f}: {number(f)} Unterkapitel nummeriert')
