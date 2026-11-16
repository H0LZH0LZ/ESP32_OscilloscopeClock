#!/usr/bin/env python3
"""Erzeugt ESP32_OscilloscopeClock_v2.0/webpage.h aus web/index.html.

Aufruf (im Projektordner):   python3 tools/gen_webpage.py

Schritte:
  1. Kommentare entfernen: HTML <!-- … -->, CSS /* … */ (nur im <style>) und
     ganze JavaScript-Kommentarzeilen (// am Zeilenanfang). Code bleibt unverändert.
  2. Mit gzip (Stufe 9) komprimieren.
  3. Als Byte-Feld INDEX_HTML_GZ (PROGMEM) in webpage.h schreiben.

Die Uhr schickt die Seite mit "Content-Encoding: gzip"; jeder Browser entpackt sie
selbst. Das spart rund 55 KB Flash und macht das Laden schneller.
"""
import gzip, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, 'web', 'index.html')
DST = os.path.join(ROOT, 'ESP32_OscilloscopeClock_v2.0', 'webpage.h')


def minify(html):
    """Kommentare entfernen (sicher: keine Änderung an Code oder Texten)."""
    html = re.sub(r'<!--.*?-->\n?', '', html, flags=re.S)
    def css(m):
        return re.sub(r'/\*.*?\*/', '', m.group(0), flags=re.S)
    html = re.sub(r'<style>.*?</style>', css, html, flags=re.S)
    def js(m):
        lines = [l for l in m.group(0).split('\n') if not l.lstrip().startswith('//')]
        return '\n'.join(lines)
    html = re.sub(r'<script>.*?</script>', js, html, flags=re.S)
    html = re.sub(r'[ \t]+\n', '\n', html)          # Leerzeichen am Zeilenende
    html = re.sub(r'\n{2,}', '\n', html)            # Leerzeilen
    return html


def main():
    src = open(SRC, encoding='utf-8').read()
    page = minify(src).encode('utf-8')
    gz = gzip.compress(page, 9, mtime=0)            # mtime=0: gleiche Eingabe → gleiche Datei
    rows = [', '.join('0x%02x' % b for b in gz[i:i + 20]) for i in range(0, len(gz), 20)]
    out = ['/******************************************************************************',
           '  webpage.h – ERZEUGT von tools/gen_webpage.py aus web/index.html.',
           '  Nicht von Hand bearbeiten: web/index.html ändern und das Skript ausführen.',
           '',
           '  Webinterface der Uhr, gzip-komprimiert: %d Byte (unkomprimiert %d Byte,' % (len(gz), len(page)),
           '  Quelle mit Kommentaren %d Byte). Ausgeliefert unter "/" mit' % len(src.encode('utf-8')),
           '  Content-Encoding: gzip.',
           '******************************************************************************/',
           '#pragma once',
           '',
           'static const uint8_t INDEX_HTML_GZ[] PROGMEM = {',
           ',\n'.join('  ' + r for r in rows),
           '};',
           '']
    open(DST, 'w', encoding='utf-8').write('\n'.join(out))
    print('webpage.h: %d -> %d Byte (gzip)' % (len(page), len(gz)), file=sys.stderr)


if __name__ == '__main__':
    main()
