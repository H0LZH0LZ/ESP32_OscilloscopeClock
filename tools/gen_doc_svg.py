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
    L = ['3V3','EN','VP','VN','P34','P35','P32','P33','P25','P26','P27','P14','P12','GND','P13','SD2','SD3','CMD','5V']
    R = ['GND','P23','P22','TX','RX','P21','GND','P19','P18','P5','P17','P16','P4','P0','P2','P15','SD1','SD0','CLK']
    X0, X1, Y0, P = 400, 600, 130, 24
    y = lambda i: Y0 + (i - 1) * P
    s = []
    a = s.append
    # Platine
    a(f"<rect x='{X0-22}' y='{Y0-52}' width='{X1-X0+44}' height='{18*P+112}' rx='6' fill='#2f8f46' stroke='#1d5e2c' stroke-width='2'/>")
    a(f"<text x='{(X0+X1)/2}' y='{y(19)+94}' text-anchor='middle' font-size='12' fill='#1d5e2c' font-weight='bold'>ESP32-Breakout-Board (Schraubklemmen), DevKit aufgesteckt</text>")
    a(f"<rect x='{X0+18}' y='{Y0-40}' width='{X1-X0-36}' height='{18*P+80}' rx='6' fill='#1f3b63' stroke='#13243d' stroke-width='2'/>")
    a(f"<rect x='{X0+58}' y='{Y0-28}' width='{X1-X0-116}' height='110' rx='4' fill='#c9ccd1' stroke='#8a8f96'/>")
    a(f"<text x='{(X0+X1)/2}' y='{Y0+18}' text-anchor='middle' font-size='11' fill='#333'>ESP32-</text>")
    a(f"<text x='{(X0+X1)/2}' y='{Y0+32}' text-anchor='middle' font-size='11' fill='#333'>WROOM-32U</text>")
    a(f"<circle cx='{X1-60}' cy='{Y0-14}' r='6' fill='#d8b14a' stroke='#8a6d1d'/>")
    a(f"<text x='{X1-50}' y='{Y0-26}' font-size='11' fill='#fff'>IPEX</text>")
    a(f"<text x='{(X0+X1)/2}' y='{Y0+230}' text-anchor='middle' font-size='15' font-weight='bold' fill='#fff'>DevKitC</text>")
    a(f"<rect x='{(X0+X1)/2-22}' y='{y(19)+28}' width='44' height='18' rx='6' fill='#c9ccd1' stroke='#8a8f96'/>")
    a(f"<text x='{(X0+X1)/2}' y='{y(19)+74}' text-anchor='middle' font-size='12' fill='#333'>USB-C ← Flachbandkabel ← USB-C-Einbaubuchse (Anschlussseite)</text>")
    a(f"<rect x='{X1-90}' y='{y(14)-9}' width='26' height='18' rx='3' fill='#eee' stroke='#666'/>")
    a(f"<text x='{X1-77}' y='{y(14)+4}' text-anchor='middle' font-size='8'>BOOT</text>")
    used = {('L',1):'#d62828',('L',9):'#e09f00',('L',10):'#1d6fd6',('L',14):'#111',
            ('R',1):'#111',('R',2):'#2a9d48',('R',9):'#7b3fb5',('R',10):'#d1495b',('R',11):'#00a6a6',('R',13):'#a0522d'}
    for i, n in enumerate(L, 1):
        c = used.get(('L', i))
        a(f"<rect x='{X0-7}' y='{y(i)-7}' width='14' height='14' rx='2' fill='{c or '#d9d9d9'}' stroke='#555'/>")
        a(f"<text x='{X0+26}' y='{y(i)+4}' font-size='11' fill='{'#fff' if c else '#9fb3cf'}' {'font-weight=\"bold\"' if c else ''}>{n}</text>")
    for i, n in enumerate(R, 1):
        c = used.get(('R', i))
        a(f"<rect x='{X1-7}' y='{y(i)-7}' width='14' height='14' rx='2' fill='{c or '#d9d9d9'}' stroke='#555'/>")
        a(f"<text x='{X1-26}' y='{y(i)+4}' text-anchor='end' font-size='11' fill='{'#fff' if c else '#9fb3cf'}' {'font-weight=\"bold\"' if c else ''}>{n}</text>")
    def wire(pts, col, dash=False):
        p = ' '.join(f'{x},{yy}' for x, yy in pts)
        a(f"<polyline points='{p}' fill='none' stroke='{col}' stroke-width='3' stroke-linejoin='round' {'stroke-dasharray=\"8 6\"' if dash else ''}/>")
    def res(x, yy, col):
        a(f"<rect x='{x-22}' y='{yy-8}' width='44' height='16' rx='3' fill='#f3e3c3' stroke='#7a5a1d'/>")
        a(f"<text x='{x}' y='{yy+4}' text-anchor='middle' font-size='10'>220 Ω</text>")
    def gnd(x, yy):
        a(f"<line x1='{x}' y1='{yy}' x2='{x}' y2='{yy+8}' stroke='#111' stroke-width='2'/>")
        for k, w in enumerate((12, 8, 4)):
            a(f"<line x1='{x-w}' y1='{yy+8+k*4}' x2='{x+w}' y2='{yy+8+k*4}' stroke='#111' stroke-width='2'/>")
    # Gehäusewand mit Kabelverschraubungen, außen die BNC-Stecker der Pigtails
    WX = 175
    a(f"<line x1='{WX}' y1='190' x2='{WX}' y2='440' stroke='#999' stroke-width='6' stroke-dasharray='14 6'/>")
    a(f"<text x='{WX}' y='182' text-anchor='middle' font-size='11' fill='#555'>Gehäusewand</text>")
    for lab, sub, by, pin, col, dash in [('Pigtail X', '→ Oszilloskop Kanal 1', 250, 9, '#e09f00', False),
                                        ('Pigtail Y', '→ Oszilloskop Kanal 2', 370, 10, '#1d6fd6', False)]:
        d = 'stroke-dasharray="6 4"' if dash else ''
        a(f"<line x1='60' y1='{by}' x2='212' y2='{by}' stroke='#c98d6b' stroke-width='7' {d}/>")         # RG316
        a(f"<rect x='18' y='{by-12}' width='44' height='24' rx='4' fill='#d9d9d9' stroke='#555' stroke-width='2'/>")  # BNC-Stecker
        a(f"<rect x='8' y='{by-8}' width='12' height='16' fill='#bbb' stroke='#555'/>")
        a(f"<rect x='{WX-9}' y='{by-13}' width='18' height='26' rx='2' fill='#b5b5b5' stroke='#444' stroke-width='1.5'/>")  # Verschraubung
        a(f"<text x='40' y='{by-22}' font-size='13' font-weight='bold'>{lab}</text>")
        a(f"<text x='10' y='{by+30}' font-size='11' fill='#444'>BNC-Stecker {sub}</text>")
        cx = 300 + (pin - 9) * 18
        res(262, by, col)
        wire([(212, by), (240, by)], '#d62828', dash)                                           # rote Ader
        wire([(284, by), (cx, by), (cx, y(pin)), (X0 - 7, y(pin))], col, dash)
        wire([(212, by), (212, by + 18)], '#7a4a2a', dash); gnd(212, by + 18)                  # braune Ader = Schirm
    a(f"<text x='{WX+10}' y='212' font-size='10' fill='#555'>Lapp SKINDICHT MINI M6</text>")
    # GND-Klemme
    wire([(X0 - 7, y(14)), (372, y(14)), (372, y(14) + 16)], '#111'); gnd(372, y(14) + 16)
    a(f"<text x='10' y='590' font-size='11' fill='#111'>⏚ = Klemme GND</text>")
    a(f"<text x='10' y='606' font-size='11' fill='#111'>Pigtail: rot = Innenleiter (Signal),</text>")
    a(f"<text x='10' y='622' font-size='11' fill='#111'>braun = Schirm (Masse)</text>")
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
    a(f"<text x='{X1+142}' y='30' font-size='12'>SMA-Buchse + WLAN-Antenne (Bingfu, SMA)</text>")
    a(f"<text x='{X1+142}' y='46' font-size='11' fill='#444'>(IPEX→SMA-Pigtail aus dem Antennen-Kit)</text>")
    # Legende
    leg = [('#d62828', '3V3 → OLED VCC'), ('#111', 'GND (Masse)'), ('#e09f00', 'P25 (DAC1) → X'),
           ('#1d6fd6', 'P26 (DAC2) → Y'), ('#2a9d48', 'P23 → DIN'),
           ('#7b3fb5', 'P18 → CLK'), ('#d1495b', 'P5 → CS'), ('#00a6a6', 'P17 → DC'), ('#a0522d', 'P4 → RST')]
    for k, (c, t) in enumerate(leg):
        lx, ly = 760 + (k // 5) * 170, 420 + (k % 5) * 22 if k < 5 else 420 + (k - 5) * 22
        a(f"<line x1='{lx}' y1='{ly}' x2='{lx+26}' y2='{ly}' stroke='{c}' stroke-width='4'/><text x='{lx+32}' y='{ly+4}' font-size='11'>{t}</text>")
    a("<text x='760' y='545' font-size='11' fill='#444'>Beschriftung wie auf dem Breakout-Board (P25 = GPIO25 …).</text>")
    a("<text x='760' y='562' font-size='11' fill='#444'>P0 = BOOT-Taster – dort nichts anschließen.</text>")
    a("<text x='760' y='579' font-size='11' fill='#444'>220 Ω in die rote Ader jedes Pigtails eingelötet.</text>")
    save('verdrahtung.svg', 1110, 680, '\n'.join(s))

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
    a("<text x='10' y='8' font-size='5' font-weight='bold'>Bohrplan Abzweigdose 150 × 100 × 70 mm – Maßstab 1:1 (mit 100 % drucken)</text>")
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
    # Anschlussseite (kurze Seite, 100 mm), von außen gesehen: Pigtail X – USB-C – Pigtail Y
    X2, Y2 = 10, 140
    panel(X2, Y2, 100, 45, 'Anschlussseite (kurze Seite, von außen gesehen)')
    for cx, lab in [(X2+28, 'X'), (X2+72, 'Y')]:
        hole(cx, Y2+20, 5.0, lab)
        a(f"<circle cx='{cx}' cy='{Y2+20}' r='3' fill='none' stroke='#c00' stroke-width='0.15' stroke-dasharray='0.6 0.6'/>")
    hole(X2+50, Y2+20, 16, 'USB-C  Ø 16')
    dim(X2, Y2+38, X2+28, Y2+38, '28'); dim(X2+28, Y2+38, X2+50, Y2+38, '22'); dim(X2+50, Y2+38, X2+72, Y2+38, '22')
    a(f"<text x='{X2+50}' y='{Y2+52}' text-anchor='middle' font-size='2.8' fill='#444'>Pigtails: Ø 5,0 bohren + M6-Gewinde schneiden (gestrichelt: M6) – oder Ø 6,2 + M6-Mutter innen</text>")
    a(f"<text x='{X2+50}' y='{Y2+56}' text-anchor='middle' font-size='2.8' fill='#444'>Alle drei Löcher auf gleicher Höhe, etwa Mitte des Unterteils</text>")
    # Gegenüberliegende Seite: Antenne
    X3, Y3 = 120, 140
    panel(X3, Y3, 100, 45, 'Gegenüberliegende Seite – Antenne')
    hole(X3+50, Y3+20, 6.5, 'SMA  Ø 6,5')
    dim(X3, Y3+38, X3+50, Y3+38, '50')
    a(f"<text x='{X3+50}' y='{Y3+52}' text-anchor='middle' font-size='2.8' fill='#444'>Mittig, gleiche Höhe wie die Anschlüsse</text>")
    # Prüfmaß
    a("<line x1='170' y1='30' x2='220' y2='30' stroke='#000' stroke-width='0.6'/><line x1='170' y1='27' x2='170' y2='33' stroke='#000' stroke-width='0.4'/><line x1='220' y1='27' x2='220' y2='33' stroke='#000' stroke-width='0.4'/>")
    a("<text x='195' y='26' text-anchor='middle' font-size='3.2'>Prüfmaß 50 mm</text>")
    a("<text x='170' y='42' font-size='2.8' fill='#444'>Nach dem Drucken nachmessen:</text><text x='170' y='46' font-size='2.8' fill='#444'>die Linie muss 50 mm lang sein.</text>")
    save('bohrplan.svg', 230, 205, '\n'.join(s), "width='230mm' height='200mm'")

# ---------------------------------------------------------------- Anordnung im Gehäuse
def aufbau():
    """Draufsicht, maßstäblich (4 px = 1 mm), wie im Bohrplan: Anschlussseite (100 mm) unten mit
    Pigtail X (28 mm), USB-C (50 mm) und Pigtail Y (72 mm); Antenne mittig auf der Gegenseite oben."""
    k = 4; X, Y = 150, 120; W, H = 100 * k, 150 * k
    s = []; a = s.append
    mm = lambda v: v * k
    a(f"<text x='{X+W/2}' y='26' text-anchor='middle' font-size='17' font-weight='bold'>Anordnung in der Abzweigdose (Draufsicht, Deckel offen)</text>")
    a(f"<rect x='{X}' y='{Y}' width='{W}' height='{H}' rx='16' fill='#f2f2f0' stroke='#888' stroke-width='3'/>")
    yb = Y + H
    # Antenne mittig auf der Gegenseite (oben)
    sx = X + mm(50)
    a(f"<circle cx='{sx}' cy='{Y}' r='10' fill='#d8b14a' stroke='#8a6d1d'/><line x1='{sx}' y1='{Y}' x2='{sx}' y2='{Y-60}' stroke='#333' stroke-width='8' stroke-linecap='round'/>")
    a(f"<text x='{sx+16}' y='{Y-40}' font-size='12'>Antenne (SMA)</text>")
    a(f"<text x='{X+W/2}' y='{Y-72}' text-anchor='middle' font-size='13' fill='#555'>Gegenüberliegende Seite</text>")
    # Breakout-Board mit DevKitC (USB-C zur Anschlussseite, Modul/IPEX zur Antenne)
    bx, by_, bw, bh = X + mm(20), Y + mm(42), mm(60), mm(75)
    a(f"<rect x='{bx}' y='{by_}' width='{bw}' height='{bh}' rx='5' fill='#2f8f46' stroke='#1d5e2c' stroke-width='2'/>")
    for xx in (bx + 8, bx + bw - 26):
        a(f"<rect x='{xx}' y='{by_+10}' width='18' height='{bh-20}' fill='#7fd18f' stroke='#1d5e2c'/>")
    dx, dw, dh = X + mm(50), mm(28.5), mm(48.5); dy = by_ + mm(13)
    a(f"<rect x='{dx-dw/2}' y='{dy}' width='{dw}' height='{dh}' rx='5' fill='#1b1b1b'/>")
    a(f"<rect x='{dx-dw/2+8}' y='{dy+6}' width='{dw-16}' height='{mm(17)}' rx='2' fill='#c9ccd1'/>")
    a(f"<text x='{dx}' y='{dy+dh/2+30}' text-anchor='middle' font-size='13' fill='#fff' transform='rotate(-90 {dx} {dy+dh/2+30})'>DevKitC V4</text>")
    ix, iy = dx + dw/2 - 14, dy + 14
    a(f"<circle cx='{ix}' cy='{iy}' r='5' fill='#d8b14a' stroke='#8a6d1d'/>")
    a(f"<path d='M{ix},{iy} C{ix+40},{iy-60} {sx+20},{Y+80} {sx},{Y+8}' fill='none' stroke='#777' stroke-width='3'/>")
    a(f"<text x='{bx+bw/2}' y='{by_+bh+18}' text-anchor='middle' font-size='12' fill='#1d5e2c'>Breakout-Board auf der Montageplatte</text>")
    # USB-C mittig
    a(f"<rect x='{dx-12}' y='{dy+dh-4}' width='24' height='10' rx='3' fill='#c9ccd1' stroke='#888'/>")
    a(f"<path d='M{dx},{dy+dh+6} C{dx-25},{yb-90} {dx+25},{yb-50} {dx},{yb-10}' fill='none' stroke='#d08a3a' stroke-width='9' stroke-opacity='.85'/>")
    a(f"<rect x='{dx-32}' y='{yb-10}' width='64' height='20' rx='5' fill='#333'/>")
    a(f"<text x='{dx}' y='{yb+40}' text-anchor='middle' font-size='12'>USB-C</text>")
    # Pigtails X (28 mm) und Y (72 mm) zu den Klemmen P25/P26 (linke Klemmenleiste)
    for n, (lab, d) in enumerate([('X', 28), ('Y', 72)]):
        cx = X + mm(d); ty = by_ + bh * 0.45 + n * 10; lx = X + mm(8) + n * 8
        a(f"<line x1='{cx}' y1='{yb+70}' x2='{cx}' y2='{yb-30}' stroke='#c98d6b' stroke-width='6'/>")
        a(f"<rect x='{cx-11}' y='{yb+62}' width='22' height='30' rx='4' fill='#d9d9d9' stroke='#555' stroke-width='2'/>")
        a(f"<text x='{cx}' y='{yb+110}' text-anchor='middle' font-size='13' font-weight='bold'>{lab}</text>")
        a(f"<rect x='{cx-10}' y='{yb-12}' width='20' height='24' rx='2' fill='#b5b5b5' stroke='#444'/>")
        if lab == 'X':
            path = f"M{cx},{yb-30} C{cx},{yb-60} {lx},{yb-60} {lx},{yb-90} L{lx},{ty}"
        else:   # Y: rechts am Board vorbei nach oben, über das Board zur linken Klemmenleiste
            rx_ = X + W - mm(8)
            path = f"M{cx},{yb-30} C{cx},{yb-60} {rx_},{yb-60} {rx_},{yb-90} L{rx_},{by_-14} L{lx},{by_-14} L{lx},{ty}"
        a(f"<path d='{path}' fill='none' stroke='#c98d6b' stroke-width='3'/>")
        a(f"<line x1='{lx}' y1='{ty}' x2='{bx+17}' y2='{ty}' stroke='#d62828' stroke-width='3'/>")
    a(f"<text x='{X+W/2}' y='{yb+132}' text-anchor='middle' font-size='12'>Pigtail X · USB-C · Pigtail Y (Kabelverschraubungen SKINDICHT MINI M6)</text>")
    a(f"<text x='{X+W/2}' y='{yb+152}' text-anchor='middle' font-size='13' fill='#555'>Anschlussseite (kurze Seite)</text>")
    a(f"<text x='{X+W/2}' y='{Y+mm(22)}' text-anchor='middle' font-size='11' fill='#555'>OLED (61,5 × 39,5 mm) sitzt im Scharnierdeckel –</text>")
    a(f"<text x='{X+W/2}' y='{Y+mm(22)+15}' text-anchor='middle' font-size='11' fill='#555'>Leitungen lang genug lassen</text>")
    a(f"<text x='{X-14}' y='{Y+H/2}' text-anchor='middle' font-size='12' fill='#06c' transform='rotate(-90 {X-14} {Y+H/2})'>150 mm</text>")
    a(f"<text x='{X+W/2}' y='{yb+172}' text-anchor='middle' font-size='12' fill='#06c'>100 mm</text>")
    save('aufbau.svg', X * 2 + W, yb + 184, '\n'.join(s))

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
