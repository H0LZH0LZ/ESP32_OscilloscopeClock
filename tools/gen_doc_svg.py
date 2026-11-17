#!/usr/bin/env python3
"""Zeichnet die Schaubilder der Dokumentation als SVG nach docs/images/:
  verdrahtung.svg  – Schaltplan ESP32 DevKitC ↔ Pigtails, OLED, Antenne (OLED-Inhalt aus
                     docs/images/oled_pixel.png, 128 × 64 Pixel aus der OLED-Simulation)
  bohrplan.svg     – Gehäuse-Bohrplan 1:1 (in mm, zum Ausdrucken mit 100 %)
  aufbau.svg       – Anordnung der Teile im Gehäuse (Draufsicht)
  software.svg     – Aufteilung der Firmware auf die beiden Kerne
Aufruf: python3 tools/gen_doc_svg.py
"""
import os
OUT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'docs', 'images')
FONT = "font-family='Helvetica,Arial,sans-serif'"

# Englische Fassung: gleiche Zeichnung, Texte übersetzt (Datei *_en.svg)
EN = {'47,5': '47.5', 'ESP32-Breakout-Board (Schraubklemmen), DevKit aufgesteckt': 'ESP32 breakout board (screw terminals), DevKit plugged in', 'USB-C ← Flachbandkabel ← USB-C-Einbaubuchse (Anschlussseite)': 'USB-C ← flat cable ← USB-C panel socket (connector side)', 'Gehäusewand': 'Enclosure wall', 'BNC-Stecker → Oszilloskop Kanal 1': 'BNC plug → oscilloscope channel 1', 'BNC-Stecker → Oszilloskop Kanal 2': 'BNC plug → oscilloscope channel 2', '⏚ = Klemme GND': '⏚ = GND terminal', 'Pigtail: rot = Innenleiter (Signal),': 'Pigtail: red = core (signal),', 'braun = Schirm (Masse)': 'brown = shield (ground)', 'SMA-Buchse + WLAN-Antenne (Bingfu, SMA)': 'SMA socket + WiFi antenna (Bingfu, SMA)', '(IPEX→SMA-Pigtail aus dem Antennen-Kit)': '(IPEX→SMA pigtail from the antenna kit)', 'GND (Masse)': 'GND (ground)', 'Beschriftung wie auf dem Breakout-Board (P25 = GPIO25 …).': 'Labels as printed on the breakout board (P25 = GPIO25 …).', 'P0 = BOOT-Taster – dort nichts anschließen.': 'P0 = BOOT button – do not connect anything there.', '220 Ω in die rote Ader jedes Pigtails eingelötet.': '220 Ω soldered into the red lead of each pigtail.', 'Bohrplan Abzweigdose 150 × 100 × 70 mm – Maßstab 1:1': 'Drilling template, junction box 150 × 100 × 70 mm – scale 1:1', 'DIN A4 mit 100 % drucken (nicht „an Seite anpassen“), dann das Prüfmaß nachmessen': 'Print on A4 at 100 % (not “fit to page”), then measure the test line', 'Deckel (Draufsicht)': 'Lid (top view)', 'Fenster OLED 55 × 28': 'OLED window 55 × 28', 'Befestigungslöcher des OLED-Moduls (61,5 × 39,5 mm) nach dem Modul anzeichnen (M2,5)': 'Mark the mounting holes of the OLED module (61.5 × 39.5 mm) from the module itself (M2.5)', 'Prüfmaß 50 mm': 'Test line 50 mm', 'Nach dem Drucken nachmessen: die Linie muss genau 50 mm lang sein.': 'Measure after printing: the line must be exactly 50 mm long.', 'Schablonen ausschneiden und mit Klebeband auf Deckel und Seiten kleben.': 'Cut out the templates and tape them onto the lid and the sides.', 'Anschlussseite (kurze Seite, von außen gesehen)': 'Connector side (short side, seen from outside)', 'Pigtails: Ø 5,0 bohren + M6-Gewinde schneiden (gestrichelt: M6) – oder Ø 6,2 + M6-Mutter innen': 'Pigtails: drill Ø 5.0 + tap M6 (dashed: M6) – or Ø 6.2 + M6 nut inside', 'Alle drei Löcher auf gleicher Höhe, etwa Mitte des Unterteils': 'All three holes at the same height, about the middle of the base', 'Gegenüberliegende Seite – Antenne': 'Opposite side – antenna', 'SMA  Ø 6,5': 'SMA  Ø 6.5', 'Mittig, gleiche Höhe wie die Anschlüsse': 'Centred, same height as the connectors', 'Anordnung in der Abzweigdose (Draufsicht, Deckel offen)': 'Layout in the junction box (top view, lid open)', 'Breakout-Board, DevKit um 180° gedreht': 'Breakout board, DevKit rotated by 180°', 'gestrichelt: USB- und Antennenkabel unter dem Board': 'dashed: USB and antenna cable under the board', 'Antenne (SMA)': 'Antenna (SMA)', 'Gegenüberliegende Seite': 'Opposite side', 'Pigtail X · USB-C · Pigtail Y (Kabelverschraubungen SKINDICHT MINI M6)': 'Pigtail X · USB-C · pigtail Y (cable glands SKINDICHT MINI M6)', 'Anschlussseite (kurze Seite)': 'Connector side (short side)', 'OLED (61,5 × 39,5 mm) sitzt im Scharnierdeckel –': 'OLED (61.5 × 39.5 mm) sits in the hinged lid –', 'Leitungen lang genug lassen': 'leave the wires long enough', 'Kern 1 – scopeTask': 'Core 1 – scopeTask', '• nur Strahlausgabe, ohne Pause': '• beam output only, without pause', '• DAC X/Y an GPIO25/26 (Register)': '• DAC X/Y on GPIO25/26 (registers)', '• Rundtour: Zifferblatt + Ziffern': '• round trip: dial + numerals', '• Zeiger nach der Uhrzeit': '• hands according to the time', '• gleich schnelle Linien = gleich hell': '• equally fast lines = equally bright', '• Profil ANALOG/DIGITAL, Testbild': '• profile ANALOG/DIGITAL, test pattern', '• ca. 230 Bilder pro Sekunde': '• approx. 230 frames per second', 'Kern 0 – uiTask + wifiTask': 'Core 0 – uiTask + wifiTask', '• WLAN (stärkstes bekanntes Netz)': '• WiFi (strongest known network)', '• Zeit per NTP, stündlich': '• time via NTP, hourly', '• Webserver, Setup-WLAN, mDNS': '• web server, setup network, mDNS', '• OLED-Anzeige (SPI)': '• OLED display (SPI)', '• BOOT-Taster, Einstellungen (NVS)': '• BOOT button, settings (NVS)', '• Protokoll (Ringpuffer 8 KB)': '• log (ring buffer 8 KB)', '• CPU-Last-Messung': '• CPU load measurement', 'Übergabe nur über wenige atomare Variablen (Uhrzeit, Zifferblatt, Helligkeit, Profil) – keine Sperren im Zeichenpfad': 'Hand-over only via a few atomic variables (time, dial, brightness, profile) – no locks in the drawing path'}

def save(name, w, h, body, extra=''):
    svg = (f"<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 {w} {h}' {extra} {FONT}>\n"
           f"<rect x='0' y='0' width='{w}' height='{h}' fill='#fff'/>\n{body}</svg>\n")
    open(os.path.join(OUT, name), 'w', encoding='utf-8').write(svg)
    en = svg
    for de, tr in EN.items():
        en = en.replace('>' + de + '<', '>' + tr + '<')
    if name == 'verdrahtung.svg':                                 # OLED-Inhalt auf Englisch
        import base64
        b64 = lambda f: base64.b64encode(open(os.path.join(OUT, f), 'rb').read()).decode()
        en = en.replace(b64('oled_pixel.png'), b64('oled_pixel_en.png'))
    open(os.path.join(OUT, name.replace('.svg', '_en.svg')), 'w', encoding='utf-8').write(en)

# ---------------------------------------------------------------- Verdrahtung
def verdrahtung():
    L = ['3V3','EN','VP','VN','P34','P35','P32','P33','P25','P26','P27','P14','P12','GND','P13','SD2','SD3','CMD','5V']
    R = ['GND','P23','P22','TX','RX','P21','GND','P19','P18','P5','P17','P16','P4','P0','P2','P15','SD1','SD0','CLK']
    X0, X1, Y0, P = 400, 600, 130, 24
    y = lambda i: Y0 + (i - 1) * P
    s = []
    a = s.append
    # --- Breakout-Board (grün) mit Schraubklemmen
    a(f"<rect x='{X0-24}' y='{Y0-56}' width='{X1-X0+48}' height='{18*P+118}' rx='5' fill='#2f8f46' stroke='#1d5e2c' stroke-width='2'/>")
    for hx, hy in ((X0-14, Y0-46), (X1+14, Y0-46), (X0-14, y(19)+52), (X1+14, y(19)+52)):
        a(f"<circle cx='{hx}' cy='{hy}' r='5' fill='#fff' stroke='#c9a227' stroke-width='2'/>")
    a(f"<text x='{(X0+X1)/2}' y='{y(19)+94}' text-anchor='middle' font-size='12' fill='#1d5e2c' font-weight='bold'>ESP32-Breakout-Board (Schraubklemmen), DevKit aufgesteckt</text>")
    # Buchsenleisten, in denen das DevKit steckt
    for hx in (X0+16, X1-22):
        a(f"<rect x='{hx}' y='{Y0-14}' width='6' height='{18*P+28}' fill='#111'/>")
    # --- DevKitC V4 (schwarze Platine)
    DX0, DX1 = X0+20, X1-20
    a(f"<rect x='{DX0}' y='{Y0-44}' width='{DX1-DX0}' height='{18*P+92}' rx='5' fill='#161616' stroke='#000' stroke-width='1.5'/>")
    # Modul ESP32-WROOM-32U: silbernes Schirmblech, IPEX-Buchse oben rechts
    MX0, MX1 = DX0+34, DX1-34
    a(f"<rect x='{MX0-4}' y='{Y0-38}' width='{MX1-MX0+8}' height='136' rx='2' fill='#2a2a2a'/>")
    a(f"<rect x='{MX0}' y='{Y0-30}' width='{MX1-MX0}' height='118' rx='3' fill='#cfd2d6' stroke='#9aa0a6'/>")
    a(f"<rect x='{MX0+5}' y='{Y0-25}' width='{MX1-MX0-10}' height='108' rx='2' fill='none' stroke='#b4b8bd' stroke-width='1'/>")
    a(f"<text x='{(MX0+MX1)/2}' y='{Y0+8}' text-anchor='middle' font-size='9' fill='#555' font-weight='bold'>ESP32-WROOM-32U</text>")
    a(f"<text x='{(MX0+MX1)/2}' y='{Y0+22}' text-anchor='middle' font-size='7' fill='#777'>CE · FCC · 211-16007</text>")
    for qx in range(4):
        for qy in range(4):
            if (qx + qy) % 2 == 0:
                a(f"<rect x='{MX1-30+qx*5}' y='{Y0+50+qy*5}' width='5' height='5' fill='#666'/>")
    a(f"<circle cx='{MX1-10}' cy='{Y0-18}' r='6' fill='#d8b14a' stroke='#8a6d1d' stroke-width='1.5'/><circle cx='{MX1-10}' cy='{Y0-18}' r='2' fill='#8a6d1d'/>")
    a(f"<text x='{MX1-20}' y='{Y0-30}' text-anchor='end' font-size='9' fill='#fff'>IPEX</text>")
    for k in range(10):                                           # Lötpads unter dem Modul
        a(f"<rect x='{MX0+6+k*((MX1-MX0-12)/10)}' y='{Y0+92}' width='6' height='5' fill='#c9a227'/>")
    # Spannungsregler AMS1117 und USB-Seriell-Wandler CP2102
    a(f"<rect x='{(DX0+DX1)/2-14}' y='{y(10)-8}' width='28' height='18' fill='#222' stroke='#444'/>")
    for k in range(3):
        a(f"<rect x='{(DX0+DX1)/2-12+k*10}' y='{y(10)+10}' width='4' height='6' fill='#bbb'/>")
    a(f"<text x='{(DX0+DX1)/2}' y='{y(10)+3}' text-anchor='middle' font-size='6' fill='#aaa'>1117</text>")
    cx_, cy_ = (DX0+DX1)/2, y(15)
    a(f"<rect x='{cx_-16}' y='{cy_-16}' width='32' height='32' fill='#2b2b2b' stroke='#555'/>")
    a(f"<text x='{cx_}' y='{cy_-2}' text-anchor='middle' font-size='6' fill='#aaa'>SILABS</text><text x='{cx_}' y='{cy_+7}' text-anchor='middle' font-size='6' fill='#aaa'>CP2102</text>")
    a(f"<text x='{(DX0+DX1)/2}' y='{y(7)+6}' text-anchor='middle' font-size='10' fill='#ddd' transform='rotate(-90 {(DX0+DX1)/2} {y(7)+2})'>ESP32_DevKitc_V4</text>")
    # Taster EN (links) und BOOT (rechts), USB-C-Buchse unten
    for bxk, lab, tx, anc in ((DX0+22, 'EN', DX0+38, 'start'), (DX1-22, 'BOOT', DX1-38, 'end')):
        by0 = y(19) + 24
        a(f"<rect x='{bxk-11}' y='{by0}' width='22' height='22' rx='2' fill='#d9d9d9' stroke='#888'/><circle cx='{bxk}' cy='{by0+11}' r='6' fill='#3a3a3a'/>")
        a(f"<text x='{bxk}' y='{by0-3}' text-anchor='middle' font-size='7' fill='#fff'>{lab}</text>")
    a(f"<rect x='{(DX0+DX1)/2-20}' y='{y(19)+22}' width='40' height='26' rx='7' fill='#d6d8db' stroke='#888' stroke-width='1.5'/>")
    a(f"<rect x='{(DX0+DX1)/2-14}' y='{y(19)+30}' width='28' height='10' rx='4' fill='#555'/>")
    a(f"<text x='{(DX0+DX1)/2}' y='{y(19)+74}' text-anchor='middle' font-size='12' fill='#333'>USB-C ← Flachbandkabel ← USB-C-Einbaubuchse (Anschlussseite)</text>")
    # Schraubklemmen links/rechts; benutzte Klemmen mit Farbmarke
    used = {('L',1):'#d62828',('L',9):'#e09f00',('L',10):'#1d6fd6',('L',14):'#111',
            ('R',1):'#111',('R',2):'#2a9d48',('R',9):'#7b3fb5',('R',10):'#d1495b',('R',11):'#00a6a6',('R',13):'#a0522d'}
    def terminal(x, yy, c):
        a(f"<rect x='{x-10}' y='{yy-11}' width='20' height='22' fill='#5cc06b' stroke='#2b7a38'/>")
        a(f"<circle cx='{x}' cy='{yy}' r='6' fill='#c8ccd0' stroke='#777'/><line x1='{x-4}' y1='{yy}' x2='{x+4}' y2='{yy}' stroke='#555' stroke-width='1.5'/>")
        if c: a(f"<circle cx='{x}' cy='{yy}' r='3' fill='{c}'/>")
    for i, n in enumerate(L, 1):
        c = used.get(('L', i)); terminal(X0, y(i), c)
        a(f"<text x='{DX0+6}' y='{y(i)+4}' font-size='10' fill='{'#fff' if c else '#9a9a9a'}' {'font-weight=\"bold\"' if c else ''}>{n}</text>")
    for i, n in enumerate(R, 1):
        c = used.get(('R', i)); terminal(X1, y(i), c)
        a(f"<text x='{DX1-6}' y='{y(i)+4}' text-anchor='end' font-size='10' fill='{'#fff' if c else '#9a9a9a'}' {'font-weight=\"bold\"' if c else ''}>{n}</text>")
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
        wire([(284, by), (cx, by), (cx, y(pin)), (X0 - 10, y(pin))], col, dash)
        wire([(212, by), (212, by + 18)], '#7a4a2a', dash); gnd(212, by + 18)                  # braune Ader = Schirm
    a(f"<text x='{WX+10}' y='212' font-size='10' fill='#555'>Lapp SKINDICHT MINI M6</text>")
    # GND-Klemme
    wire([(X0 - 10, y(14)), (372, y(14)), (372, y(14) + 16)], '#111'); gnd(372, y(14) + 16)
    a(f"<text x='10' y='590' font-size='11' fill='#111'>⏚ = Klemme GND</text>")
    a(f"<text x='10' y='606' font-size='11' fill='#111'>Pigtail: rot = Innenleiter (Signal),</text>")
    a(f"<text x='10' y='622' font-size='11' fill='#111'>braun = Schirm (Masse)</text>")
    # OLED: Waveshare 2,42" (blaue Platine 61,5 × 39,5 mm, schwarzes Glas, Flexkabel, Stiftleiste links)
    OX, OY = 800, 150
    ow, oh = 288, 185                                             # Seitenverhältnis wie 61,5 × 39,5
    oy0 = OY - 40
    a(f"<rect x='{OX}' y='{oy0}' width='{ow}' height='{oh}' rx='6' fill='#1f4fa0' stroke='#163a78' stroke-width='2'/>")
    for hx, hy in ((OX+12, oy0+12), (OX+ow-12, oy0+12), (OX+12, oy0+oh-12), (OX+ow-12, oy0+oh-12)):
        a(f"<circle cx='{hx}' cy='{hy}' r='7' fill='#fff' stroke='#c9a227' stroke-width='2.5'/>")
    gx0, gy0, gw, gh = OX+66, oy0+22, ow-96, oh-50               # Glas
    a(f"<rect x='{gx0}' y='{gy0}' width='{gw}' height='{gh}' rx='3' fill='#0b0d12' stroke='#3a3f4a'/>")
    aw, ah = gw-14, (gw-14)/2                                     # aktive Fläche 2:1 (128 × 64)
    import base64
    png = base64.b64encode(open(os.path.join(OUT, 'oled_pixel.png'), 'rb').read()).decode()
    a(f"<image href='data:image/png;base64,{png}' x='{gx0+7}' y='{gy0+(gh-ah)/2-6}' width='{aw}' height='{ah}' style='image-rendering:pixelated'/>")
    a(f"<path d='M{gx0+gw/2-28},{gy0+gh} L{gx0+gw/2+28},{gy0+gh} L{gx0+gw/2+20},{oy0+oh-4} L{gx0+gw/2-20},{oy0+oh-4} Z' fill='#d58a2a' stroke='#9a5e14'/>")
    a(f"<text x='{gx0+2}' y='{oy0+14}' font-size='9' fill='#fff' font-weight='bold'>Waveshare · 2.42inch OLED Module</text>")
    a(f"<text x='{OX+ow/2}' y='{oy0+oh+18}' text-anchor='middle' font-size='12' fill='#222'>Waveshare 2.42\" OLED · SSD1309 · SPI (4-Wire)</text>")
    names = ['VCC', 'GND', 'DIN', 'CLK', 'CS', 'DC', 'RST']
    pins = {}
    a(f"<rect x='{OX+4}' y='{OY-26}' width='14' height='{6*24+16}' fill='#111' rx='2'/>")
    for k, n in enumerate(names):
        py = OY - 18 + k * 24 + 6
        pins[n] = py
        a(f"<rect x='{OX-6}' y='{py-2}' width='20' height='4' fill='#d4af37'/><rect x='{OX+6}' y='{py-5}' width='10' height='10' fill='#222' stroke='#555'/>")
        a(f"<text x='{OX+24}' y='{py+4}' font-size='10' fill='#fff' font-weight='bold'>{n}</text>")
    # Leitungen zum OLED (rechte Stiftleiste)
    conn = [('DIN', 2, '#2a9d48', 640), ('CLK', 9, '#7b3fb5', 656), ('CS', 10, '#d1495b', 672),
            ('DC', 11, '#00a6a6', 688), ('RST', 13, '#a0522d', 704), ('GND', 1, '#111', 720)]
    for n, rp, col, cx in conn:
        wire([(X1 + 10, y(rp)), (cx, y(rp)), (cx, pins[n]), (OX - 6, pins[n])], col)
    # VCC von 3V3 (links oben) über die Platine
    wire([(X0 - 10, y(1)), (360, y(1)), (360, 62), (750, 62), (750, pins['VCC']), (OX - 6, pins['VCC'])], '#d62828')
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
    # DIN A4 hochkant (210 × 297 mm), Maßstab 1:1
    a("<text x='105' y='16' text-anchor='middle' font-size='4.6' font-weight='bold'>Bohrplan Abzweigdose 150 × 100 × 70 mm – Maßstab 1:1</text>")
    a("<text x='105' y='22' text-anchor='middle' font-size='3.2' fill='#444'>DIN A4 mit 100 % drucken (nicht „an Seite anpassen“), dann das Prüfmaß nachmessen</text>")
    # Deckel 150 x 100
    X, Y = 30, 35
    panel(X, Y, 150, 100, 'Deckel (Draufsicht)')
    a(f"<rect x='{X+75-27.5}' y='{Y+22}' width='55' height='28' fill='#eef' stroke='#c00' stroke-width='0.35'/>")
    a(f"<text x='{X+75}' y='{Y+37}' text-anchor='middle' font-size='3'>Fenster OLED 55 × 28</text>")
    dim(X+75-27.5, Y+18, X+75+27.5, Y+18, '55')
    dim(X+75+31, Y+22, X+75+31, Y+50, '28')
    dim(X, Y+14, X+75-27.5, Y+14, '47,5')
    dim(X+4, Y, X+4, Y+22, '22')
    a(f"<text x='{X+75}' y='{Y+62}' text-anchor='middle' font-size='2.8' fill='#444'>Befestigungslöcher des OLED-Moduls (61,5 × 39,5 mm) nach dem Modul anzeichnen (M2,5)</text>")
    # Prüfmaß
    a("<line x1='30' y1='150' x2='80' y2='150' stroke='#000' stroke-width='0.6'/><line x1='30' y1='147' x2='30' y2='153' stroke='#000' stroke-width='0.4'/><line x1='80' y1='147' x2='80' y2='153' stroke='#000' stroke-width='0.4'/>")
    a("<text x='55' y='145.5' text-anchor='middle' font-size='3.2'>Prüfmaß 50 mm</text>")
    a("<text x='88' y='149' font-size='2.8' fill='#444'>Nach dem Drucken nachmessen: die Linie muss genau 50 mm lang sein.</text>")
    a("<text x='88' y='153' font-size='2.8' fill='#444'>Schablonen ausschneiden und mit Klebeband auf Deckel und Seiten kleben.</text>")
    # Anschlussseite (kurze Seite, 100 mm), von außen gesehen: Pigtail X – USB-C – Pigtail Y
    X2, Y2 = 55, 172
    panel(X2, Y2, 100, 42, 'Anschlussseite (kurze Seite, von außen gesehen)')
    for cx, lab in [(X2+28, 'X'), (X2+72, 'Y')]:
        hole(cx, Y2+18, 5.0, lab)
        a(f"<circle cx='{cx}' cy='{Y2+18}' r='3' fill='none' stroke='#c00' stroke-width='0.15' stroke-dasharray='0.6 0.6'/>")
    hole(X2+50, Y2+18, 16, 'USB-C  Ø 16')
    dim(X2, Y2+36, X2+28, Y2+36, '28'); dim(X2+28, Y2+36, X2+50, Y2+36, '22'); dim(X2+50, Y2+36, X2+72, Y2+36, '22')
    a(f"<text x='{X2+50}' y='{Y2+48}' text-anchor='middle' font-size='2.8' fill='#444'>Pigtails: Ø 5,0 bohren + M6-Gewinde schneiden (gestrichelt: M6) – oder Ø 6,2 + M6-Mutter innen</text>")
    a(f"<text x='{X2+50}' y='{Y2+52}' text-anchor='middle' font-size='2.8' fill='#444'>Alle drei Löcher auf gleicher Höhe, etwa Mitte des Unterteils</text>")
    # Gegenüberliegende Seite: Antenne
    X3, Y3 = 55, 240
    panel(X3, Y3, 100, 42, 'Gegenüberliegende Seite – Antenne')
    hole(X3+50, Y3+18, 6.5, 'SMA  Ø 6,5')
    dim(X3, Y3+36, X3+50, Y3+36, '50')
    a(f"<text x='{X3+50}' y='{Y3+48}' text-anchor='middle' font-size='2.8' fill='#444'>Mittig, gleiche Höhe wie die Anschlüsse</text>")
    save('bohrplan.svg', 210, 297, '\n'.join(s), "width='210mm' height='297mm'")

# ---------------------------------------------------------------- Anordnung im Gehäuse
def aufbau():
    """Draufsicht, maßstäblich (4 px = 1 mm), wie im Bohrplan: Anschlussseite (100 mm) unten mit
    Pigtail X (28 mm), USB-C (50 mm) und Pigtail Y (72 mm); Antenne mittig auf der Gegenseite oben.
    Das DevKit ist um 180° gedreht eingebaut: Modul/IPEX zur Anschlussseite, USB-C zur Antennenseite.
    Antennen- und USB-Kabel sind zu lang und laufen deshalb einmal unter dem Breakout-Board durch."""
    k = 4; X, Y = 150, 120; W, H = 100 * k, 150 * k
    s = []; a = s.append
    mm = lambda v: v * k
    a(f"<text x='{X+W/2}' y='26' text-anchor='middle' font-size='17' font-weight='bold'>Anordnung in der Abzweigdose (Draufsicht, Deckel offen)</text>")
    a(f"<rect x='{X}' y='{Y}' width='{W}' height='{H}' rx='16' fill='#f2f2f0' stroke='#888' stroke-width='3'/>")
    yb = Y + H
    sx = X + mm(50)
    bx, by_, bw, bh = X + mm(20), Y + mm(40), mm(60), mm(75)
    dx, dw, dh = X + mm(50), mm(28.5), mm(48.5); dy = by_ + mm(13)
    ix, iy = dx - dw/2 + 14, dy + dh - 14                         # IPEX unten links (gedreht)
    ux, ax = dx - mm(14), dx + mm(14)                             # Spuren unter dem Board
    usb = '#d08a3a'; ant = '#777'
    # --- Breakout-Board mit DevKitC (um 180° gedreht)
    a(f"<rect x='{bx}' y='{by_}' width='{bw}' height='{bh}' rx='5' fill='#2f8f46' stroke='#1d5e2c' stroke-width='2'/>")
    for xx in (bx + 8, bx + bw - 26):
        a(f"<rect x='{xx}' y='{by_+10}' width='18' height='{bh-20}' fill='#7fd18f' stroke='#1d5e2c'/>")
    a(f"<rect x='{dx-dw/2}' y='{dy}' width='{dw}' height='{dh}' rx='5' fill='#1b1b1b'/>")
    a(f"<rect x='{dx-dw/2+8}' y='{dy+dh-6-mm(17)}' width='{dw-16}' height='{mm(17)}' rx='2' fill='#c9ccd1'/>")
    a(f"<text x='{dx}' y='{dy+dh/2-30}' text-anchor='middle' font-size='13' fill='#fff' transform='rotate(90 {dx} {dy+dh/2-30})'>DevKitC V4</text>")
    a(f"<rect x='{dx-12}' y='{dy-6}' width='24' height='10' rx='3' fill='#c9ccd1' stroke='#888'/>")
    a(f"<circle cx='{ix}' cy='{iy}' r='5' fill='#d8b14a' stroke='#8a6d1d'/>")
    # USB-Flachbandkabel: vom DevKit (oben) nach oben, Schleife, unter dem Board durch zur USB-C-Buchse unten
    a(f"<path d='M{dx},{dy-6} C{dx},{by_-45} {ux},{by_-45} {ux},{by_}' fill='none' stroke='{usb}' stroke-width='9'/>")
    a(f"<line x1='{ux}' y1='{by_}' x2='{ux}' y2='{by_+bh}' stroke='{usb}' stroke-width='9' stroke-dasharray='10 7' stroke-opacity='.55'/>")
    a(f"<path d='M{ux},{by_+bh} C{ux},{yb-60} {dx},{yb-60} {dx},{yb-10}' fill='none' stroke='{usb}' stroke-width='9'/>")
    # Antennenkabel: von der IPEX-Buchse (unten) nach unten, Schleife, unter dem Board durch zur Antenne oben
    a(f"<path d='M{ix},{iy} L{ix},{by_+bh} C{ix},{by_+bh+45} {ax},{by_+bh+45} {ax},{by_+bh}' fill='none' stroke='{ant}' stroke-width='3'/>")
    a(f"<line x1='{ax}' y1='{by_+bh}' x2='{ax}' y2='{by_}' stroke='{ant}' stroke-width='3' stroke-dasharray='8 6'/>")
    a(f"<path d='M{ax},{by_} C{ax},{Y+60} {sx},{Y+70} {sx},{Y+8}' fill='none' stroke='{ant}' stroke-width='3'/>")
    a(f"<text x='{bx+bw/2}' y='{by_-10}' text-anchor='middle' font-size='11' fill='#1d5e2c'>Breakout-Board, DevKit um 180° gedreht</text>")
    a(f"<text x='{bx+bw/2}' y='{by_+bh+16}' text-anchor='middle' font-size='10' fill='#555'>gestrichelt: USB- und Antennenkabel unter dem Board</text>")
    # Antenne mittig auf der Gegenseite (oben)
    a(f"<circle cx='{sx}' cy='{Y}' r='10' fill='#d8b14a' stroke='#8a6d1d'/><line x1='{sx}' y1='{Y}' x2='{sx}' y2='{Y-60}' stroke='#333' stroke-width='8' stroke-linecap='round'/>")
    a(f"<text x='{sx+16}' y='{Y-40}' font-size='12'>Antenne (SMA)</text>")
    a(f"<text x='{X+W/2}' y='{Y-72}' text-anchor='middle' font-size='13' fill='#555'>Gegenüberliegende Seite</text>")
    # USB-C-Einbaubuchse mittig unten
    a(f"<rect x='{dx-32}' y='{yb-10}' width='64' height='20' rx='5' fill='#333'/>")
    a(f"<text x='{dx}' y='{yb+40}' text-anchor='middle' font-size='12'>USB-C</text>")
    # Pigtails X (28 mm) und Y (72 mm) zu P25/P26 – durch die Drehung jetzt rechte Klemmenleiste
    tx = bx + bw - 17
    for n, (lab, d) in enumerate([('X', 28), ('Y', 72)]):
        cx = X + mm(d); ty = by_ + bh * 0.55 - n * 10; lx = X + W - mm(20) + 6 + n * 8
        a(f"<line x1='{cx}' y1='{yb+70}' x2='{cx}' y2='{yb-30}' stroke='#c98d6b' stroke-width='6'/>")
        a(f"<rect x='{cx-11}' y='{yb+62}' width='22' height='30' rx='4' fill='#d9d9d9' stroke='#555' stroke-width='2'/>")
        a(f"<text x='{cx}' y='{yb+110}' text-anchor='middle' font-size='13' font-weight='bold'>{lab}</text>")
        a(f"<rect x='{cx-10}' y='{yb-12}' width='20' height='24' rx='2' fill='#b5b5b5' stroke='#444'/>")
        a(f"<path d='M{cx},{yb-30} C{cx},{yb-70} {lx},{yb-60} {lx},{yb-100} L{lx},{ty}' fill='none' stroke='#c98d6b' stroke-width='3'/>")
        a(f"<line x1='{lx}' y1='{ty}' x2='{tx}' y2='{ty}' stroke='#d62828' stroke-width='3'/>")
    a(f"<text x='{tx+4}' y='{by_+bh*0.55+26}' font-size='10' fill='#1d5e2c' text-anchor='end'>P25/P26</text>")
    a(f"<text x='{X+W/2}' y='{yb+132}' text-anchor='middle' font-size='12'>Pigtail X · USB-C · Pigtail Y (Kabelverschraubungen SKINDICHT MINI M6)</text>")
    a(f"<text x='{X+W/2}' y='{yb+152}' text-anchor='middle' font-size='13' fill='#555'>Anschlussseite (kurze Seite)</text>")
    a(f"<text x='{X+W/2}' y='{Y+mm(14)}' text-anchor='middle' font-size='11' fill='#555'>OLED (61,5 × 39,5 mm) sitzt im Scharnierdeckel –</text>")
    a(f"<text x='{X+W/2}' y='{Y+mm(14)+15}' text-anchor='middle' font-size='11' fill='#555'>Leitungen lang genug lassen</text>")
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
