#!/usr/bin/env python3
"""Setzt die Kapitel der Dokumentation in die Projektseite docs/index.html (GitHub Pages) ein.

Quelle sind dieselben Dateien wie für die PDFs: docs/dokumentation.html (Deutsch) und
docs/documentation.en.html (Englisch). Übernommen werden die Kapitel 1–12, das Quellenverzeichnis und Anhang A (ab der ersten
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
SITE = 'https://holzholz123.github.io/ESP32_OscilloscopeClock/'

# Knopf zum Installieren im Browser (ESP Web Tools); die Dateien in docs/firmware/ baut GitHub
# (tools/gen_firmware.py, .github/workflows/firmware.yml). Im PDF steht an dieser Stelle nichts.
FLASH_BUTTON = {
    'de': ('<esp-web-install-button manifest="firmware/manifest.json">\n'
           ' <button slot="activate" class="flashbtn">Firmware installieren</button>\n'
           ' <span slot="unsupported" class="flashmsg">Dieser Browser kann nicht auf USB-Geräte zugreifen. Bitte Chrome oder Edge am PC '
           'verwenden – oder den Sketch mit Arduino Cloud bzw. der Arduino IDE hochladen (siehe unten).</span>\n'
           ' <span slot="not-allowed" class="flashmsg">Die Installation geht nur über eine sichere Verbindung (https).</span>\n'
           '</esp-web-install-button>'),
    'en': ('<esp-web-install-button manifest="firmware/manifest.json">\n'
           ' <button slot="activate" class="flashbtn">Install firmware</button>\n'
           ' <span slot="unsupported" class="flashmsg">This browser cannot access USB devices. Please use Chrome or Edge on a PC – '
           'or upload the sketch with Arduino Cloud or the Arduino IDE (see below).</span>\n'
           ' <span slot="not-allowed" class="flashmsg">Installing only works over a secure connection (https).</span>\n'
           '</esp-web-install-button>'),
}


def label_cells(m):
    """Schreibt die Texte der Kopfzeile als data-th in die Zellen der übrigen Zeilen."""
    t = m.group(0)
    rows = re.findall(r'<tr[^>]*>.*?</tr>', t, flags=re.S)
    if not rows or '<th' not in rows[0]:
        return t
    heads = [html.escape(html.unescape(re.sub(r'<[^>]+>', '', h)).strip(), quote=True)
             for h in re.findall(r'<th[^>]*>(.*?)</th>', rows[0], flags=re.S)]
    for r in rows[1:]:
        i = 0

        def tag(_m):
            nonlocal i
            h = heads[i] if i < len(heads) else ''
            i += 1
            return '<td data-th="%s"' % h
        nr = re.sub(r'<td(?=[ >])', tag, r)
        # Teileliste: die Anzahl steht auf dem Handy vor dem Teil („2 ×“) statt als eigener Eintrag
        if heads and heads[0] in ('Anz.', 'Qty'):
            q = re.search(r'<td[^>]*>(.*?)</td>', nr, flags=re.S)
            nr = nr.replace('<td data-th="%s"' % heads[0], '<td class="qty" data-th="%s"' % heads[0], 1)
            n = re.sub(r'<[^>]+>', '', q.group(1)).strip()
            if n not in ('', '–', '-'):
                nr = nr.replace('<td data-th="%s"' % heads[1], '<td data-qty="%s ×" data-th="%s"'
                                % (html.escape(n, quote=True), heads[1]), 1)
        t = t.replace(r, nr, 1)
    return t


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
        if not re.match(r'(\d+|[A-Z]) ', title) and title not in ('Quellenverzeichnis', 'References'):
            continue
        # Tabellen: jede Zelle bekommt die Spaltenbezeichnung (data-th) – auf dem Handy stehen die
        # Zeilen dann als Einträge untereinander, jeweils mit Bezeichnung
        content = re.sub(r'<table[^>]*>.*?</table>', label_cells, content, flags=re.S)
        # feste Breiten der PDF-Bilder entfernen
        content = re.sub(r'<table([^>]*)>', r'<div class="tw"><table\1>', content)
        content = content.replace('</table>', '</table></div>')
        content = re.sub(r'(<img [^>]*?) style="width:\d+%"', r'\1', content)
        content = re.sub(r'<img (?![^>]*loading=)', '<img loading="lazy" ', content)
        # Links auf die Projektseite selbst werden relativ (kein neuer Tab), z. B. Sketch-ZIP
        content = content.replace('href="' + SITE, 'href="')
        content = content.replace('href=""', 'href="./"')
        # Verweise aus dem PDF auf die Projektseite: hier ist man schon auf der Projektseite
        content = re.sub(r'von der <a href="#kap\d+">Projektseite</a>', 'von dieser Seite', content)
        content = re.sub(r'from the <a href="#kap\d+">project page</a>', 'from this page', content)
        # Kapitel 4: Knopf "Firmware installieren" (ESP Web Tools, Skript am Ende von docs/index.html)
        content = content.replace('<!-- FIRMWARE-KNOPF -->', FLASH_BUTTON['en' if path.endswith('.en.html') else 'de'])
        # externe Links (Händler, GitHub …) in einem neuen Tab öffnen
        content = re.sub(r'<a href="(https?://[^"]*)"', r'<a href="\1" target="_blank" rel="noopener"', content)
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
    # PDF-Links mit Versionskennung (Prüfsumme der Datei): Browser und Handys laden nach jeder
    # neuen PDF-Fassung die aktuelle Datei statt einer zwischengespeicherten alten
    import hashlib
    for pdf in ('ESP32_Oscilloscope_Clock_Dokumentation.pdf', 'ESP32_Oscilloscope_Clock_Documentation.pdf'):
        v = hashlib.md5(open(os.path.join(DOCS, pdf), 'rb').read()).hexdigest()[:8]
        s = re.sub(r'href="' + re.escape(pdf) + r'(\?v=[0-9a-f]+)?"', f'href="{pdf}?v={v}"', s)
    open(f, 'w', encoding='utf-8').write(s)
    print(f'docs/index.html: {len(de)} Kapitel eingesetzt')


if __name__ == '__main__':
    main()
