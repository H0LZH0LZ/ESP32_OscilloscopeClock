#!/usr/bin/env python3
"""Setzt die Kapitel der Dokumentation in die Projektseite docs/index.html (GitHub Pages) ein.

Quelle sind dieselben Dateien wie für die PDFs: docs/dokumentation.html (Deutsch) und
docs/documentation.en.html (Englisch). Übernommen werden die Kapitel 1–11 (ab der ersten
<h2>-Überschrift nach dem Inhaltsverzeichnis bis vor die Bohrplan-Seite). Jedes Kapitel wird ein
aufklappbarer Abschnitt im Stil des Webinterfaces, beide Sprachen stehen nebeneinander
(data-l="de" / data-l="en"), das Seitenmenü bekommt einen Eintrag je Kapitel.

In docs/index.html ersetzt das Skript nur, was zwischen diesen Markierungen steht:
  <!-- KAPITEL-MENUE --> … <!-- /KAPITEL-MENUE -->   Einträge im Seitenmenü
  <!-- KAPITEL -->       … <!-- /KAPITEL -->          die Kapitel selbst
Alles andere (Überblick oben, Downloads, Links, Stil) wird in docs/index.html von Hand gepflegt.

Aufruf (nach Änderungen an der Dokumentation):  python3 tools/gen_site.py
"""
import html
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DOCS = os.path.join(ROOT, 'docs')


def chapters(path):
    """Liefert [(Titel, HTML-Inhalt), …] der nummerierten Kapitel."""
    s = open(path, encoding='utf-8').read()
    start = s.index('</section>', s.index('<section class="toc">')) + len('</section>')
    end = s.index('<section class="bohrseite">')
    body = s[start:end]
    parts = re.split(r'<h2>(.*?)</h2>', body)
    out = []
    for i in range(1, len(parts), 2):
        title, content = parts[i].strip(), parts[i + 1].strip()
        if not re.match(r'\d+ ', title):
            continue
        # Tabellen auf dem Handy seitlich scrollbar machen, feste Breiten der PDF-Bilder entfernen
        content = re.sub(r'<table([^>]*)>', r'<div class="tw"><table\1>', content)
        content = content.replace('</table>', '</table></div>')
        content = re.sub(r'(<img [^>]*?) style="width:\d+%"', r'\1', content)
        content = re.sub(r'<img (?![^>]*loading=)', '<img loading="lazy" ', content)
        out.append((title, content))
    return out


def replace_between(s, tag, new):
    a, b = f'<!-- {tag} -->', f'<!-- /{tag} -->'
    i, j = s.index(a) + len(a), s.index(b)
    return s[:i] + '\n' + new + '\n' + s[j:]


def main():
    de = chapters(os.path.join(DOCS, 'dokumentation.html'))
    en = chapters(os.path.join(DOCS, 'documentation.en.html'))
    if len(de) != len(en):
        sys.exit(f'gen_site.py: {len(de)} deutsche, aber {len(en)} englische Kapitel')
    nav, body = [], []
    for k, ((td, cd), (te, ce)) in enumerate(zip(de, en), 1):
        cid = f'kap{k}'
        strip = lambda t: html.unescape(re.sub(r'<[^>]+>', '', t))
        nav.append(f' <a href="#{cid}"><span data-l="de">{html.escape(strip(td))}</span>'
                   f'<span data-l="en">{html.escape(strip(te))}</span></a>')
        body.append(f'<div class="grp closed"><h2 id="{cid}" class="coll" role="button" tabindex="0" aria-expanded="false">'
                    f'<span data-l="de">{td}</span><span data-l="en">{te}</span></h2>\n'
                    f'<section class="doc"><div data-l="de">\n{cd}\n</div><div data-l="en">\n{ce}\n</div></section></div>')
    f = os.path.join(DOCS, 'index.html')
    s = open(f, encoding='utf-8').read()
    s = replace_between(s, 'KAPITEL-MENUE', '\n'.join(nav))
    s = replace_between(s, 'KAPITEL', '\n\n'.join(body))
    open(f, 'w', encoding='utf-8').write(s)
    print(f'docs/index.html: {len(de)} Kapitel eingesetzt')


if __name__ == '__main__':
    main()
