#!/usr/bin/env python3
"""Erzeugt die Testseite (Demo) des Webinterfaces: web/demo/scope-clock-demo.html

Die Testseite ist genau die Webseite, die auf der Uhr liegt (aus webpage.h entpackt),
ergänzt um eine Demo-Schicht, die die Schnittstellen der Uhr im Browser simuliert.
Sie ist eine einzelne HTML-Datei ohne externe Abhängigkeiten und lässt sich direkt
im Browser öffnen – ganz ohne ESP32.

Bestandteile:
  web/demo/mock-api.js       simuliert /api/status, /api/log, /api/wifi, /api/time, /api/vectors …
  web/demo/mock-settings.js  simuliert /api/settings und /api/settings/reset
  web/demo/demo-ui.js        Knopf "Setup-WLAN simulieren" im Demo-Hinweis
Die Vektordaten kommen aus tools/gen_vectors.py (wie vectors.h), die Schriften aus font.h.

Aufruf (nach tools/gen_webpage.py):  python3 tools/gen_demo.py
"""
import base64
import gzip
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FW = os.path.join(ROOT, 'ESP32_OscilloscopeClock_v2.0')
DEMO = os.path.join(ROOT, 'web', 'demo')
OUT = os.path.join(DEMO, 'scope-clock-demo.html')
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import gen_webpage  # noqa: E402


def read(p):
    with open(p, encoding='utf-8') as f:
        return f.read()


def c_bytes(text):
    return bytes(int(x, 16) for x in re.findall(r'0x([0-9a-fA-F]{2})', text))


def vectors():
    """Vektordaten für die Vorschau – dieselben wie in vectors.h (GET /api/vectors)."""
    import gen_vectors
    return gen_vectors.preview()


def main():
    # 1. Webseite genau so, wie sie auf der Uhr liegt
    hdr = read(os.path.join(FW, 'webpage.h'))
    html = gzip.decompress(c_bytes(hdr.split('PROGMEM = {')[1])).decode('utf-8')
    if html != gen_webpage.minify(read(os.path.join(ROOT, 'web', 'index.html'))):
        sys.exit('webpage.h ist veraltet – zuerst tools/gen_webpage.py ausführen')

    style = re.search(r'<style>(.*?)</style>', html, re.S).group(1)
    body = re.search(r'<body>(.*)</body>', html, re.S).group(1)
    script = re.search(r'<script>(.*?)</script>', body, re.S).group(1)
    body = re.sub(r'<script>.*?</script>', '', body, flags=re.S)

    # 2. Farbschema auch per data-theme umschaltbar (wie im Seitenmenü der Uhr)
    style = style.replace('@media (prefers-color-scheme:dark){:root{',
                          '@media (prefers-color-scheme:dark){:root:not([data-theme="light"]){color-scheme:dark;')
    dark = re.search(r'@media \(prefers-color-scheme:dark\)\{:root:not\(\[data-theme="light"\]\)\{(.*?)\}\}', style).group(1)
    style += ':root[data-theme="dark"]{' + dark + '}\n'
    style += ('.demo{background:var(--card);border:1px dashed var(--line);border-radius:12px;padding:10px 14px;'
              'margin:10px 0 0;font-size:14px;color:var(--mute);display:flex;flex-wrap:wrap;gap:8px 12px;'
              'align-items:center;justify-content:center;text-align:center}\n'
              '.demo b{color:var(--fg)}.demo button{width:auto;min-height:36px;margin:0;font-size:14px}\n')

    # 3. Schriften aus font.h einbetten (auf der Uhr: /font.woff2, /font-600.woff2)
    parts = read(os.path.join(FW, 'font.h')).split('PROGMEM = {')
    for i, url in ((1, '/font.woff2'), (2, '/font-600.woff2')):
        data = c_bytes(parts[i].split('};')[0])
        style = style.replace('url(' + url + ')', 'url(data:font/woff2;base64,' + base64.b64encode(data).decode() + ')')

    # 4. Demo-Hinweis unter der Kopfzeile, App-Links (Manifest, Icon) entfernen
    banner = ('<div class="demo" id="demoBar"><span><b>Demo-Vorschau</b> – keine Verbindung zu einer Uhr, '
              'alle Werte sind simuliert. Die Uhrzeit ist die deines Geräts.</span>\n'
              '<button type="button" class="sec" id="demoSetup">Setup-WLAN simulieren</button></div>')
    assert '</header>' in body
    body = body.replace('</header>', '</header>\n' + banner, 1)
    body = re.sub(r'<link rel="(manifest|icon|apple-touch-icon)"[^>]*>\n?', '', body)

    # 5. Demo-Schicht mit aktuellen Vektordaten
    api = read(os.path.join(DEMO, 'mock-api.js'))
    assert '/*VEC*/null' in api
    api = api.replace('/*VEC*/null', json.dumps(vectors(), separators=(',', ':')), 1)
    settings = read(os.path.join(DEMO, 'mock-settings.js'))
    ui = read(os.path.join(DEMO, 'demo-ui.js'))

    page = ('<!doctype html>\n<html lang="de"><head><meta charset="utf-8">\n'
            '<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">\n'
            '<title>Scope Clock Webinterface – Demo</title>\n'
            f'<style>{style}</style></head><body>\n{body}\n'
            f'<script>{api}</script>\n<script>{settings}</script>\n<script>{script}\n{ui}</script>\n'
            '</body></html>\n')
    with open(OUT, 'w', encoding='utf-8') as f:
        f.write(page)
    print(os.path.relpath(OUT, ROOT), len(page.encode()), 'Byte')


if __name__ == '__main__':
    main()
