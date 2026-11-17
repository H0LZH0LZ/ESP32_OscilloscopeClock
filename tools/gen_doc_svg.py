#!/usr/bin/env python3
"""Zeichnet die Schaubilder der Dokumentation als SVG nach docs/images/:
  verdrahtung.svg  – Schaltplan ESP32 DevKitC ↔ BNC, OLED, Antenne
  bohrplan.svg     – Gehäuse-Bohrplan 1:1 (in mm, zum Ausdrucken mit 100 %)
  aufbau.svg       – Anordnung der Teile im Gehäuse (Draufsicht)
  software.svg     – Aufteilung der Firmware auf die beiden Kerne
Aufruf: python3 tools/gen_doc_svg.py
"""
import os
OUT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'docs', 'images')
FONT = "font-family='Helvetica,Arial,sans-serif'"

def save(name, w, h, body, extra=''):
    open(os.path.join(OUT, name), 'w', encoding='utf-8').write(
        f"<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 {w} {h}' {extra} {FONT}>\n"
        f"<rect x='0' y='0' width='{w}' height='{h}' fill='#fff'/>\n{body}</svg>\n")

# ---------------------------------------------------------------- Verdrahtung
def verdrahtung():
    L = ['3V3','EN','VP','VN','34','35','32','33','25','26','27','14','12','GND','13','D2','D3','CMD','5V']
    R = ['GND','23','22','TX','RX','21','GND','19','18','5','17','16','4','0','2','15','D1','D0','CLK']
    X0, X1, Y0, P = 400, 600, 130, 24
    y = lambda i: Y0 + (i - 1) * P
    s = []
    a = s.append
    # Platine
    a(f"<rect x='{X0-14}' y='{Y0-40}' width='{X1-X0+28}' height='{18*P+90}' rx='8' fill='#1f3b63' stroke='#13243d' stroke-width='2'/>")
    a(f"<rect x='{X0+28}' y='{Y0-28}' width='{X1-X0-56}' height='110' rx='4' fill='#c9ccd1' stroke='#8a8f96'/>")
    a(f"<text x='{(X0+X1)/2}' y='{Y0+20}' text-anchor='middle' font-size='12' fill='#333'>ESP32-WROOM-32U</text>")
    a(f"<text x='{(X0+X1)/2}' y='{Y0+38}' text-anchor='middle' font-size='11' fill='#555'>(Modul mit IPEX-Buchse)</text>")
    a(f"<circle cx='{X1-60}' cy='{Y0-14}' r='6' fill='#d8b14a' stroke='#8a6d1d'/>")
    a(f"<text x='{X1-50}' y='{Y0-26}' font-size='11' fill='#fff'>IPEX</text>")
    a(f"<text x='{(X0+X1)/2}' y='{Y0+230}' text-anchor='middle' font-size='16' font-weight='bold' fill='#fff'>DevKitC V4</text>")
    a(f"<rect x='{(X0+X1)/2-22}' y='{y(19)+28}' width='44' height='18' rx='6' fill='#c9ccd1' stroke='#8a8f96'/>")
    a(f"<text x='{(X0+X1)/2}' y='{y(19)+70}' text-anchor='middle' font-size='12' fill='#333'>USB-C (5 V Netzteil)</text>")
    a(f"<rect x='{X1-58}' y='{y(14)-9}' width='22' height='18' rx='3' fill='#eee' stroke='#666'/>")
    a(f"<text x='{X1-47}' y='{y(14)+4}' text-anchor='middle' font-size='9'>BOOT</text>")
    used = {('L',1):'#d62828',('L',9):'#e09f00',('L',10):'#1d6fd6',('L',11):'#8a8a8a',('L',14):'#111',
            ('R',1):'#111',('R',2):'#2a9d48',('R',9):'#7b3fb5',('R',10):'#d1495b',('R',11):'#00a6a6',('R',13):'#a0522d'}
    for i, n in enumerate(L, 1):
        c = used.get(('L', i))
        a(f"<circle cx='{X0}' cy='{y(i)}' r='5' fill='{c or '#d9d9d9'}' stroke='#555'/>")
        a(f"<text x='{X0+12}' y='{y(i)+4}' font-size='12' fill='{'#fff' if c else '#9fb3cf'}' {'font-weight=\"bold\"' if c else ''}>{n}</text>")
    for i, n in enumerate(R, 1):
        c = used.get(('R', i))
        a(f"<circle cx='{X1}' cy='{y(i)}' r='5' fill='{c or '#d9d9d9'}' stroke='#555'/>")
        a(f"<text x='{X1-12}' y='{y(i)+4}' text-anchor='end' font-size='12' fill='{'#fff' if c else '#9fb3cf'}' {'font-weight=\"bold\"' if c else ''}>{n}</text>")
    def wire(pts, col, dash=False):
        p = ' '.join(f'{x},{yy}' for x, yy in pts)
        a(f"<polyline points='{p}' fill='none' stroke='{col}' stroke-width='3' stroke-linejoin='round' {'stroke-dasharray=\"8 6\"' if dash else ''}/>")
    def res(x, yy, col):
        a(f"<rect x='{x-22}' y='{yy-8}' width='44' height='16' rx='3' fill='#f3e3c3' stroke='#7a5a1d'/>")
        a(f"<text x='{x}' y='{yy+4}' text-anchor='middle' font-size='10'>220 Ω</text>")
    def bnc(cx, cy, lab, sub, dash=False):
        d = 'stroke-dasharray="5 4"' if dash else ''
        a(f"<circle cx='{cx}' cy='{cy}' r='26' fill='#e6e6e6' stroke='#555' stroke-width='2' {d}/>")
        a(f"<circle cx='{cx}' cy='{cy}' r='9' fill='#fff' stroke='#555' stroke-width='2'/><circle cx='{cx}' cy='{cy}' r='3' fill='#c9a227'/>")
        a(f"<text x='{cx}' y='{cy-36}' text-anchor='middle' font-size='14' font-weight='bold'>{lab}</text>")
        a(f"<text x='{cx}' y='{cy+46}' text-anchor='middle' font-size='11' fill='#444'>{sub}</text>")
    # BNC-Buchsen links
    BX = 90
    for lab, sub, by, pin, col, dash in [('BNC X', '→ Kanal 1 (X)', 230, 9, '#e09f00', False),
                                        ('BNC Y', '→ Kanal 2 (Y)', 360, 10, '#1d6fd6', False),
                                        ('BNC Z (optional)', '→ Z-Eingang', 500, 11, '#8a8a8a', True)]:
        bnc(BX, by, lab, sub, dash)
        cx = 300 + (pin - 9) * 18
        wire([(X0, y(pin)), (cx, y(pin)), (cx, by), (BX + 9, by)], col, dash)
        res(200, by, col)
    # Masse-Sammelleitung
    wire([(X0, y(14)), (362, y(14)), (362, 600), (30, 600), (30, 230)], '#111')   # Masse-Sammelleitung
    for by in (230, 360, 500):
        wire([(30, by), (BX - 26, by)], '#111')                                       # Gehäuse der Buchse
    a(f"<text x='40' y='618' font-size='11' fill='#111'>Masse: Gehäuse aller BNC-Buchsen → GND</text>")
    # OLED
    OX, OY = 800, 150
    a(f"<rect x='{OX}' y='{OY-30}' width='190' height='210' rx='8' fill='#123' stroke='#0a1a26' stroke-width='2'/>")
    a(f"<rect x='{OX+40}' y='{OY-12}' width='138' height='78' fill='#05080c' stroke='#335'/>")
    a(f"<text x='{OX+109}' y='{OY+24}' text-anchor='middle' font-size='16' fill='#dfe8ff' font-family='monospace'>19:42:37</text>")
    a(f"<text x='{OX+109}' y='{OY+44}' text-anchor='middle' font-size='9' fill='#dfe8ff' font-family='monospace'>KW 47 - 16.11.2026</text>")
    a(f"<text x='{OX+109}' y='{OY+100}' text-anchor='middle' font-size='12' fill='#fff'>Waveshare 2.42\" OLED</text>")
    a(f"<text x='{OX+109}' y='{OY+116}' text-anchor='middle' font-size='11' fill='#9fb3cf'>SSD1309 · SPI (4-Wire)</text>")
    names = ['VCC', 'GND', 'DIN', 'CLK', 'CS', 'DC', 'RST']
    oy = lambda k: OY + 30 + k * 0  # Platzhalter
    pins = {}
    for k, n in enumerate(names):
        py = OY - 12 + k * 26 + 0
        py = OY - 10 + k * 24 + 0
        pins[n] = OY + 0 + k * 24 - 18 + 0
    for k, n in enumerate(names):
        py = OY - 18 + k * 24 + 6
        pins[n] = py
        a(f"<circle cx='{OX}' cy='{py}' r='5' fill='#d9d9d9' stroke='#555'/>")
        a(f"<text x='{OX+10}' y='{py+4}' font-size='11' fill='#fff' font-weight='bold'>{n}</text>")
    # Leitungen zum OLED (rechte Stiftleiste)
    conn = [('DIN', 2, '#2a9d48', 640), ('CLK', 9, '#7b3fb5', 656), ('CS', 10, '#d1495b', 672),
            ('DC', 11, '#00a6a6', 688), ('RST', 13, '#a0522d', 704), ('GND', 1, '#111', 720)]
    for n, rp, col, cx in conn:
        wire([(X1, y(rp)), (cx, y(rp)), (cx, pins[n]), (OX, pins[n])], col)
    # VCC von 3V3 (links oben) über die Platine
    wire([(X0, y(1)), (372, y(1)), (372, 62), (750, 62), (750, pins['VCC']), (OX, pins['VCC'])], '#d62828')
    # Antenne
    a(f"<path d='M{X1-60},{Y0-14} C{X1-60},40 {X1+90},70 {X1+120},40' fill='none' stroke='#777' stroke-width='4'/>")
    a(f"<rect x='{X1+112}' y='24' width='22' height='22' rx='3' fill='#d8b14a' stroke='#8a6d1d'/>")
    a(f"<line x1='{X1+123}' y1='24' x2='{X1+123}' y2='-0' stroke='#333' stroke-width='6' stroke-linecap='round'/>")
    a(f"<text x='{X1+142}' y='30' font-size='12'>SMA-Buchse + 2,4-GHz-Antenne</text>")
    a(f"<text x='{X1+142}' y='46' font-size='11' fill='#444'>(IPEX→SMA-Pigtail, Pflicht beim -32U)</text>")
    # Legende
    leg = [('#d62828', '3V3 → OLED VCC'), ('#111', 'GND (Masse)'), ('#e09f00', 'GPIO25 DAC1 → X'),
           ('#1d6fd6', 'GPIO26 DAC2 → Y'), ('#8a8a8a', 'GPIO27 → Z (optional)'), ('#2a9d48', 'GPIO23 → DIN'),
           ('#7b3fb5', 'GPIO18 → CLK'), ('#d1495b', 'GPIO5 → CS'), ('#00a6a6', 'GPIO17 → DC'), ('#a0522d', 'GPIO4 → RST')]
    for k, (c, t) in enumerate(leg):
        lx, ly = 760 + (k // 5) * 170, 420 + (k % 5) * 22
        a(f"<line x1='{lx}' y1='{ly}' x2='{lx+26}' y2='{ly}' stroke='{c}' stroke-width='4'/><text x='{lx+32}' y='{ly+4}' font-size='11'>{t}</text>")
    a("<text x='760' y='545' font-size='11' fill='#444'>Stiftleisten von oben gezählt (Antenne oben, USB-C unten).</text>")
    a("<text x='760' y='562' font-size='11' fill='#444'>GPIO0 = BOOT-Taster – dort nichts anschließen.</text>")
    a("<text x='760' y='579' font-size='11' fill='#444'>Optional: 1 nF vom BNC-Mittelkontakt nach GND.</text>")
    save('verdrahtung.svg', 1110, 660, '\n'.join(s))

# ---------------------------------------------------------------- Bohrplan 1:1
def bohrplan():
    s = []; a = s.append
    def panel(x, y, w, h, title):
        a(f"<rect x='{x}' y='{y}' width='{w}' height='{h}' fill='none' stroke='#000' stroke-width='0.4'/>")
        a(f"<text x='{x}' y='{y-3}' font-size='4.2' font-weight='bold'>{title}</text>")
    def hole(cx, cy, d, lab):
        a(f"<circle cx='{cx}' cy='{cy}' r='{d/2}' fill='none' stroke='#c00' stroke-width='0.35'/>")
        a(f"<line x1='{cx-d/2-2}' y1='{cy}' x2='{cx+d/2+2}' y2='{cy}' stroke='#c00' stroke-width='0.15'/><line x1='{cx}' y1='{cy-d/2-2}' x2='{cx}' y2='{cy+d/2+2}' stroke='#c00' stroke-width='0.15'/>")
        a(f"<text x='{cx}' y='{cy+d/2+4.5}' text-anchor='middle' font-size='3'>{lab}</text>")
    def dim(x1, y1, x2, y2, t):
        a(f"<line x1='{x1}' y1='{y1}' x2='{x2}' y2='{y2}' stroke='#06c' stroke-width='0.2' marker-start='url(#m)' marker-end='url(#m)'/>")
        a(f"<text x='{(x1+x2)/2}' y='{(y1+y2)/2-1}' text-anchor='middle' font-size='2.8' fill='#06c'>{t}</text>")
    a("<defs><marker id='m' viewBox='0 0 4 4' refX='2' refY='2' markerWidth='3' markerHeight='3' orient='auto'><path d='M0,2 L4,0 L4,4 z' fill='#06c'/></marker></defs>")
    a("<text x='10' y='8' font-size='5' font-weight='bold'>Bohrplan Gehäuse 150 × 100 mm – Maßstab 1:1 (mit 100 % drucken)</text>")
    # Deckel 150 x 100
    X, Y = 10, 20
    panel(X, Y, 150, 100, 'Deckel (Draufsicht)')
    a(f"<rect x='{X+75-27.5}' y='{Y+22}' width='55' height='28' fill='#eef' stroke='#c00' stroke-width='0.35'/>")
    a(f"<text x='{X+75}' y='{Y+37}' text-anchor='middle' font-size='3'>Fenster OLED 55 × 28</text>")
    dim(X+75-27.5, Y+18, X+75+27.5, Y+18, '55')
    dim(X+75+31, Y+22, X+75+31, Y+50, '28')
    dim(X, Y+14, X+75-27.5, Y+14, '47,5')
    dim(X+4, Y, X+4, Y+22, '22')
    a(f"<text x='{X+75}' y='{Y+62}' text-anchor='middle' font-size='2.8' fill='#444'>Befestigungslöcher des OLED-Moduls nach dem Modul anzeichnen (M2,5)</text>")
    # Stirnseite 100 breit
    X2, Y2 = 10, 140
    panel(X2, Y2, 100, 45, 'Stirnseite (vorne) – BNC-Buchsen')
    for k, (cx, lab) in enumerate([(X2+25, 'X  Ø 9,6'), (X2+50, 'Y  Ø 9,6'), (X2+75, 'Z (optional)')]):
        hole(cx, Y2+20, 9.6, lab)
    dim(X2+25, Y2+36, X2+50, Y2+36, '25'); dim(X2+50, Y2+36, X2+75, Y2+36, '25')
    dim(X2, Y2+40, X2+25, Y2+40, '25')
    a(f"<text x='{X2+50}' y='{Y2+52}' text-anchor='middle' font-size='2.8' fill='#444'>Höhe: Bohrungen auf halber Höhe der Seitenwand (Gehäusehöhe beliebig)</text>")
    # Rückseite
    X3, Y3 = 120, 140
    panel(X3, Y3, 100, 45, 'Rückseite – USB-C und Antenne')
    a(f"<rect x='{X3+30-6}' y='{Y3+20-2.5}' width='12' height='5' rx='2' fill='none' stroke='#c00' stroke-width='0.35'/>")
    a(f"<text x='{X3+30}' y='{Y3+28}' text-anchor='middle' font-size='3'>USB-C 12 × 5</text>")
    hole(X3+75, Y3+20, 6.5, 'SMA  Ø 6,5')
    dim(X3, Y3+36, X3+30, Y3+36, '30'); dim(X3+30, Y3+36, X3+75, Y3+36, '45')
    a(f"<text x='{X3+50}' y='{Y3+52}' text-anchor='middle' font-size='2.8' fill='#444'>USB-C-Ausschnitt genau auf Höhe der Buchse des DevKits anpassen</text>")
    # Prüfmaß
    a("<line x1='170' y1='30' x2='220' y2='30' stroke='#000' stroke-width='0.6'/><line x1='170' y1='27' x2='170' y2='33' stroke='#000' stroke-width='0.4'/><line x1='220' y1='27' x2='220' y2='33' stroke='#000' stroke-width='0.4'/>")
    a("<text x='195' y='26' text-anchor='middle' font-size='3.2'>Prüfmaß 50 mm</text>")
    a("<text x='170' y='42' font-size='2.8' fill='#444'>Nach dem Drucken nachmessen:</text><text x='170' y='46' font-size='2.8' fill='#444'>die Linie muss 50 mm lang sein.</text>")
    save('bohrplan.svg', 230, 200, '\n'.join(s), "width='230mm' height='200mm'")

# ---------------------------------------------------------------- Anordnung im Gehäuse
def aufbau():
    """Draufsicht, maßstäblich (4 px = 1 mm), gleiche Lage wie im Bohrplan:
    Stirnseite (100 mm) oben mit BNC bei 25/50/75 mm, Rückseite unten mit USB-C bei 30 mm
    und Antenne bei 75 mm."""
    k = 4; X, Y = 120, 70; W, H = 100 * k, 150 * k
    s = []; a = s.append
    mm = lambda v: v * k
    a(f"<text x='{X+W/2}' y='30' text-anchor='middle' font-size='17' font-weight='bold'>Anordnung im Gehäuse (Draufsicht, Deckel abgenommen)</text>")
    a(f"<text x='{X+W/2}' y='52' text-anchor='middle' font-size='13' fill='#555'>Stirnseite (vorne)</text>")
    a(f"<rect x='{X}' y='{Y}' width='{W}' height='{H}' rx='16' fill='#f4f1ea' stroke='#777' stroke-width='3'/>")
    for lab, d, opt in [('X', 25, False), ('Y', 50, False), ('Z', 75, True)]:
        cx = X + mm(d)
        a(f"<rect x='{cx-20}' y='{Y-14}' width='40' height='44' fill='#bbb' stroke='#555' {'stroke-dasharray=\"5 4\"' if opt else ''}/><circle cx='{cx}' cy='{Y+8}' r='12' fill='#ddd' stroke='#555'/>")
        a(f"<text x='{cx}' y='{Y+52}' text-anchor='middle' font-size='13'>BNC {lab}{' (opt.)' if opt else ''}</text>")
        if not opt:
            a(f"<rect x='{cx-9}' y='{Y+62}' width='18' height='40' fill='#f3e3c3' stroke='#7a5a1d'/><text x='{cx+14}' y='{Y+87}' font-size='11'>220 Ω</text>")
    # DevKitC (ca. 55 × 28 mm), längs, USB-C zur Rückseite bei 30 mm
    dx = X + mm(30); dw, dh = mm(28), mm(55); dy = Y + H - dh - 26
    a(f"<rect x='{dx-dw/2}' y='{dy}' width='{dw}' height='{dh}' rx='6' fill='#1f3b63'/>")
    a(f"<text x='{dx}' y='{dy+dh/2}' text-anchor='middle' font-size='14' fill='#fff' transform='rotate(-90 {dx} {dy+dh/2})'>ESP32 DevKitC</text>")
    a(f"<rect x='{dx-22}' y='{Y+H-26}' width='44' height='16' rx='5' fill='#c9ccd1' stroke='#888'/>")
    a(f"<circle cx='{dx}' cy='{dy+12}' r='6' fill='#d8b14a' stroke='#8a6d1d'/>")
    # Pigtail zur SMA-Buchse bei 75 mm
    sx = X + mm(75)
    a(f"<path d='M{dx},{dy+12} C{dx+80},{dy-40} {sx},{Y+H-160} {sx},{Y+H-8}' fill='none' stroke='#777' stroke-width='4'/>")
    a(f"<circle cx='{sx}' cy='{Y+H}' r='11' fill='#d8b14a' stroke='#8a6d1d'/><line x1='{sx}' y1='{Y+H}' x2='{sx}' y2='{Y+H+44}' stroke='#333' stroke-width='7' stroke-linecap='round'/>")
    # OLED im Deckel (gestrichelt)
    a(f"<rect x='{X+mm(22)}' y='{Y+mm(48)}' width='{mm(70)}' height='{mm(42)}' rx='6' fill='none' stroke='#123' stroke-width='2' stroke-dasharray='8 5'/>")
    a(f"<text x='{X+mm(57)}' y='{Y+mm(66)}' text-anchor='middle' font-size='13'>OLED-Modul im Deckel</text><text x='{X+mm(57)}' y='{Y+mm(66)+18}' text-anchor='middle' font-size='11' fill='#555'>(darunter frei lassen)</text>")
    a(f"<text x='{X+W/2}' y='{Y+H+70}' text-anchor='middle' font-size='13' fill='#555'>Rückseite: USB-C (links) · Antenne (rechts, weit weg von den BNC-Buchsen)</text>")
    # Maßangaben
    a(f"<text x='{X-12}' y='{Y+H/2}' text-anchor='middle' font-size='12' fill='#06c' transform='rotate(-90 {X-12} {Y+H/2})'>150 mm</text>")
    a(f"<text x='{X+W/2}' y='{Y+H+92}' text-anchor='middle' font-size='12' fill='#06c'>100 mm</text>")
    save('aufbau.svg', X * 2 + W, Y + H + 105, '\n'.join(s))

# ---------------------------------------------------------------- Software-Aufteilung
def software():
    s = []; a = s.append
    def box(x, y, w, h, title, lines, col):
        a(f"<rect x='{x}' y='{y}' width='{w}' height='{h}' rx='12' fill='{col}' stroke='#555' stroke-width='1.5'/>")
        a(f"<text x='{x+w/2}' y='{y+26}' text-anchor='middle' font-size='16' font-weight='bold'>{title}</text>")
        for k, l in enumerate(lines):
            a(f"<text x='{x+18}' y='{y+52+k*21}' font-size='13'>• {l}</text>")
    box(20, 20, 380, 250, 'Kern 1 – scopeTask', ['nur Strahlausgabe, ohne Pause', 'DAC X/Y an GPIO25/26 (Register)', 'Rundtour: Zifferblatt + Ziffern', 'Zeiger nach der Uhrzeit', 'gleich schnelle Linien = gleich hell', 'Profil ANALOG/DIGITAL, Testbild', 'ca. 230 Bilder pro Sekunde'], '#e8f5ec')
    box(440, 20, 380, 250, 'Kern 0 – uiTask + wifiTask', ['WLAN (stärkstes bekanntes Netz)', 'Zeit per NTP, stündlich', 'Webserver, Setup-WLAN, mDNS', 'OLED-Anzeige (SPI)', 'BOOT-Taster, Einstellungen (NVS)', 'Protokoll (Ringpuffer 8 KB)', 'CPU-Last-Messung'], '#e8eef8')
    a("<line x1='400' y1='145' x2='440' y2='145' stroke='#555' stroke-width='2'/>")
    a("<text x='420' y='300' text-anchor='middle' font-size='13' fill='#333'>Übergabe nur über wenige atomare Variablen (Uhrzeit, Zifferblatt, Helligkeit, Profil) – keine Sperren im Zeichenpfad</text>")
    save('software.svg', 840, 320, '\n'.join(s))

if __name__ == '__main__':
    os.makedirs(OUT, exist_ok=True)
    verdrahtung(); bohrplan(); aufbau(); software()
    print('SVG-Dateien in', OUT)
