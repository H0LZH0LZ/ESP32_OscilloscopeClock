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
EN = {'Stiftleiste auf der Rückseite': 'Pin header on the back', 'Optional: Echtzeituhr DS3231 (ZS-042)': 'Optional: real-time clock DS3231 (ZS-042)', 'Knopfzelle CR2032 auf der Rückseite': 'Coin cell CR2032 on the back', 'P21 → SDA (RTC)': 'P21 → SDA (RTC)', 'optional': 'optional', 'OLED sitzt im Scharnierdeckel –': 'OLED sits in the hinged lid –', 'Leitungen lang genug lassen': 'leave the wires long enough', '47,5': '47.5', 'ESP32-Breakout-Board (Schraubklemmen), DevKit aufgesteckt': 'ESP32 breakout board (screw terminals), DevKit plugged in', 'USB-C ← Flachbandkabel ← USB-C-Einbaubuchse (Anschlussseite)': 'USB-C ← flat cable ← USB-C panel socket (connector side)', 'Gehäusewand': 'Enclosure wall', 'BNC-Stecker → Oszilloskop Kanal 1': 'BNC plug → oscilloscope channel 1', 'BNC-Stecker → Oszilloskop Kanal 2': 'BNC plug → oscilloscope channel 2', '⏚ = Klemme GND': '⏚ = GND terminal', 'Pigtail: rot = Innenleiter (Signal),': 'Pigtail: red = core (signal),', 'braun = Schirm (Masse)': 'brown = shield (ground)', 'SMA-Buchse + WLAN-Antenne (Bingfu, SMA)': 'SMA socket + WiFi antenna (Bingfu, SMA)', '(IPEX→SMA-Pigtail aus dem Antennen-Kit)': '(IPEX→SMA pigtail from the antenna kit)', 'GND (Masse)': 'GND (ground)', 'Beschriftung wie auf dem Breakout-Board (P25 = GPIO25 …).': 'Labels as printed on the breakout board (P25 = GPIO25 …).', 'P0 = BOOT-Taster – dort nichts anschließen.': 'P0 = BOOT button – do not connect anything there.', '220 Ω in die rote Ader jedes Pigtails eingelötet.': '220 Ω soldered into the red lead of each pigtail.', 'Bohrplan Abzweigdose 150 × 100 × 70 mm – Maßstab 1:1': 'Drilling template, junction box 150 × 100 × 70 mm – scale 1:1', 'DIN A4 mit 100 % drucken (nicht „an Seite anpassen“), dann das Prüfmaß nachmessen': 'Print on A4 at 100 % (not “fit to page”), then measure the test line', 'Deckel (Draufsicht)': 'Lid (top view)', 'Fenster OLED 55 × 28': 'OLED window 55 × 28', 'Befestigungslöcher des OLED-Moduls (61,5 × 39,5 mm) nach dem Modul anzeichnen (M2,5)': 'Mark the mounting holes of the OLED module (61.5 × 39.5 mm) from the module itself (M2.5)', 'Prüfmaß 50 mm': 'Test line 50 mm', 'Nach dem Drucken nachmessen: die Linie muss genau 50 mm lang sein.': 'Measure after printing: the line must be exactly 50 mm long.', 'Schablonen ausschneiden und mit Klebeband auf Deckel und Seiten kleben.': 'Cut out the templates and tape them onto the lid and the sides.', 'Anschlussseite (kurze Seite, von außen gesehen)': 'Connector side (short side, seen from outside)', 'Pigtails: Ø 5,0 bohren + M6-Gewinde schneiden (gestrichelt: M6) – oder Ø 6,2 + M6-Mutter innen': 'Pigtails: drill Ø 5.0 + tap M6 (dashed: M6) – or Ø 6.2 + M6 nut inside', 'Alle drei Löcher auf gleicher Höhe, etwa Mitte des Unterteils': 'All three holes at the same height, about the middle of the base', 'Gegenüberliegende Seite – Antenne': 'Opposite side – antenna', 'SMA  Ø 6,5': 'SMA  Ø 6.5', 'Mittig, gleiche Höhe wie die Anschlüsse': 'Centred, same height as the connectors', 'Anordnung in der Abzweigdose (Draufsicht, Deckel offen)': 'Layout in the junction box (top view, lid open)', 'Breakout-Board, DevKit um 180° gedreht': 'Breakout board, DevKit rotated by 180°', 'gestrichelt: USB- und Antennenkabel unter dem Board': 'dashed: USB and antenna cable under the board', 'Antenne (SMA)': 'Antenna (SMA)', 'Gegenüberliegende Seite': 'Opposite side', 'Pigtail X · USB-C · Pigtail Y (Kabelverschraubungen SKINDICHT MINI M6)': 'Pigtail X · USB-C · pigtail Y (cable glands SKINDICHT MINI M6)', 'Anschlussseite (kurze Seite)': 'Connector side (short side)', 'OLED (61,5 × 39,5 mm) sitzt im Scharnierdeckel –': 'OLED (61.5 × 39.5 mm) sits in the hinged lid –', 'Leitungen lang genug lassen': 'leave the wires long enough', 'Kern 1 – scopeTask': 'Core 1 – scopeTask', '• nur Strahlausgabe, ohne Pause': '• beam output only, without pause', '• DAC X/Y an GPIO25/26 (Register)': '• DAC X/Y on GPIO25/26 (registers)', '• Rundtour: Zifferblatt + Ziffern': '• round trip: dial + numerals', '• Zeiger nach der Uhrzeit': '• hands according to the time', '• gleich schnelle Linien = gleich hell': '• equally fast lines = equally bright', '• Profil ANALOG/DIGITAL, Testbild': '• profile ANALOG/DIGITAL, test pattern', '• ca. 230 Bilder pro Sekunde': '• approx. 230 frames per second', 'Kern 0 – uiTask + wifiTask': 'Core 0 – uiTask + wifiTask', '• WLAN (stärkstes bekanntes Netz)': '• WiFi (strongest known network)', '• Zeit per NTP, stündlich': '• time via NTP, hourly', '• Webserver, Setup-WLAN, mDNS': '• web server, setup network, mDNS', '• OLED-Anzeige (SPI)': '• OLED display (SPI)', '• BOOT-Taster, Einstellungen (NVS)': '• BOOT button, settings (NVS)', '• Protokoll (Ringpuffer 8 KB)': '• log (ring buffer 8 KB)', '• CPU-Last-Messung': '• CPU load measurement', 'Übergabe nur über wenige atomare Variablen (Uhrzeit, Zifferblatt, Helligkeit, Profil) – keine Sperren im Zeichenpfad': 'Hand-over only via a few atomic variables (time, dial, brightness, profile) – no locks in the drawing path'}

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
    L = ['3V3','EN','SVP','SVN','P34','P35','P32','P33','P25','P26','P27','P14','P12','GND','P13','SD2','SD3','CMD','5V']
    R = ['GND','P23','P22','TX','RX','P21','GND','P19','P18','P5','P17','P16','P4','P0','P2','P15','SD1','SD0','CLK']
    X0, X1, Y0, P = 370, 670, 130, 24
    y = lambda i: Y0 + (i - 1) * P
    s = []
    a = s.append
    # Beschriftung auf dem DevKit (wie aufgedruckt) – die Klemmen tragen die Namen des Breakout-Boards (L, R)
    DL = ['3V3','EN','VP','VN','34','35','32','33','25','26','27','14','12','GND','13','D2','D3','CMD','5V']
    DR = ['GND','23','22','TX','RX','21','GND','19','18','5','17','16','4','0','2','15','D1','D0','CLK']
    used = {('L',1):'#d62828',('L',9):'#e09f00',('L',10):'#1d6fd6',('L',14):'#111',
            ('R',1):'#111',('R',2):'#2a9d48',('R',9):'#7b3fb5',('R',10):'#d1495b',('R',11):'#00a6a6',('R',13):'#a0522d'}
    wired = set(used) | {('R',3), ('R',6), ('R',7)}                # dazu die optionalen RTC-Klemmen
    # --- Breakout-Board wie das Original: hellgrün, Klemmleisten außen, Beschriftung am Rand
    BX0, BX1 = X0-40, X1+40
    a(f"<rect x='{BX0}' y='{Y0-56}' width='{BX1-BX0}' height='{18*P+118}' rx='3' fill='#22a54a' stroke='#17803a' stroke-width='2'/>")
    for hx, hy in ((BX0+12, Y0-44), (BX1-12, Y0-44), (BX0+12, y(19)+50), (BX1-12, y(19)+50)):
        a(f"<circle cx='{hx}' cy='{hy}' r='5.5' fill='#fff' stroke='#d7efd9' stroke-width='2'/>")
    for kx in (X0, X1):
        a(f"<rect x='{kx-13}' y='{Y0-16}' width='26' height='{18*P+32}' rx='2' fill='#93e09f' stroke='#4aa35a' stroke-width='1.5'/>")
        a(f"<rect x='{kx+(9 if kx == X0 else -13)}' y='{Y0-16}' width='4' height='{18*P+32}' fill='#6fc77e'/>")   # Kabeleinführung
    a(f"<text x='{(X0+X1)/2}' y='{y(19)+94}' text-anchor='middle' font-size='12' fill='#1d5e2c' font-weight='bold'>ESP32-Breakout-Board (Schraubklemmen), DevKit aufgesteckt</text>")
    for i, n in enumerate(L, 1):                                  # Klemmen-Beschriftung außen (weiß)
        w = ('L', i) in wired
        a(f"<text x='{X0-15}' y='{y(i)-5 if w else y(i)+3}' text-anchor='end' font-size='8' fill='#fff' {'font-weight=\"bold\"' if w else ''}>{n}</text>")
    for i, n in enumerate(R, 1):
        w = ('R', i) in wired
        a(f"<text x='{X1+15}' y='{y(i)-5 if w else y(i)+3}' font-size='8' fill='#fff' {'font-weight=\"bold\"' if w else ''}>{n}</text>")
    # --- ESP32 DevKitC V4 wie auf dem Produktfoto (Vorderseite, 28,5 × 48,5 mm, USB-C unten)
    DX0, DX1 = X0+18, X1-18
    DY0, DY1 = y(1)-17, y(19)+17
    DC = (DX0+DX1)/2
    a(f"<rect x='{DX0}' y='{DY0}' width='{DX1-DX0}' height='{DY1-DY0}' rx='3' fill='#151515' stroke='#000' stroke-width='1.5'/>")
    for i in range(1, 20):                                        # Stifte: verzinnte Lötaugen
        for px in (DX0+12, DX1-12):
            a(f"<circle cx='{px}' cy='{y(i)}' r='8' fill='#2a2a2a'/><circle cx='{px}' cy='{y(i)}' r='6' fill='#c3c7cb' stroke='#6d7378'/><circle cx='{px-1.5}' cy='{y(i)-1.5}' r='2' fill='#eef0f2'/>")
    for i, n in enumerate(DL, 1):                                 # Aufdruck senkrecht wie auf dem Board
        a(f"<text x='{DX0+31}' y='{y(i)}' text-anchor='middle' font-size='10.5' fill='#fff' font-weight='bold' transform='rotate(90 {DX0+31} {y(i)})' dy='4'>{n}</text>")
    for i, n in enumerate(DR, 1):
        a(f"<text x='{DX1-31}' y='{y(i)}' text-anchor='middle' font-size='10.5' fill='#fff' font-weight='bold' transform='rotate(90 {DX1-31} {y(i)})' dy='4'>{n}</text>")
    # Modul ESP32-WROOM-32U: Modulplatine mit Lötpads, goldfarbenes Schirmblech mit Aussparung für
    # die IPEX-Buchse oben rechts, Aufdruck und QR-Code
    MX0, MX1, MY0 = DX0+44, DX1-44, DY0+3
    MY1 = MY0 + 178
    a(f"<rect x='{MX0}' y='{MY0}' width='{MX1-MX0}' height='{MY1-MY0}' rx='2' fill='#1e1e1e' stroke='#3a3a3a'/>")
    for k in range(13):
        a(f"<rect x='{MX0+2}' y='{MY0+8+k*12.6}' width='9' height='8' fill='#d9dcdf' stroke='#888'/>")
    for k in range(10):
        a(f"<rect x='{MX1-11}' y='{MY0+46+k*12.6}' width='9' height='8' fill='#d9dcdf' stroke='#888'/>")
    for k in range(10):
        a(f"<rect x='{MX0+22+k*((MX1-MX0-44)/9)-4}' y='{MY1-10}' width='8' height='9' fill='#d9dcdf' stroke='#888'/>")
    sx0, sx1, sy0, sy1, nx, ny = MX0+14, MX1-14, MY0+4, MY1-14, MX1-52, MY0+36
    a(f"<path d='M{sx0},{sy0} L{nx},{sy0} L{nx},{ny} L{sx1},{ny} L{sx1},{sy1} L{sx0},{sy1} Z' fill='#c8b88f' stroke='#8d7f5c' stroke-width='1.5'/>")
    a(f"<path d='M{sx0+4},{sy0+4} L{nx-4},{sy0+4} L{nx-4},{ny+4} L{sx1-4},{ny+4} L{sx1-4},{sy1-4} L{sx0+4},{sy1-4} Z' fill='none' stroke='#b3a37b'/>")
    tc = '#4d4535'
    a(f"<text x='{sx0+8}' y='{sy0+22}' font-size='9.5' fill='{tc}' font-weight='bold'>ESP32-WROOM-32U</text>")
    a(f"<text x='{sx0+8}' y='{sy0+44}' font-size='8' fill='{tc}'>Wi-Fi  CE  ((·))  Ⓒ  [R] 211-16007</text>")
    a(f"<text x='{sx0+8}' y='{sy0+62}' font-size='8' fill='{tc}'>ESP32-WROOM-32U</text>")
    a(f"<text x='{sx0+8}' y='{sy0+78}' font-size='6.5' fill='{tc}'>FCC ID: 2AC7Z-ESPWROOM32U</text>")
    a(f"<text x='{sx0+8}' y='{sy0+90}' font-size='6.5' fill='{tc}'>IC: 21098-ESPWROOM32U</text>")
    a(f"<text x='{sx0+8}' y='{sy0+102}' font-size='6.5' fill='{tc}'>CMIIT ID: 2019DP2463</text>")
    q = 4; qx0, qy0 = sx1-56, sy1-58                              # QR-Code mit drei Suchmustern
    a(f"<rect x='{qx0-2}' y='{qy0-2}' width='{13*q+4}' height='{13*q+4}' fill='#d8cba8'/>")
    for fx, fy in ((0, 0), (9, 0), (0, 9)):
        a(f"<rect x='{qx0+fx*q}' y='{qy0+fy*q}' width='{4*q}' height='{4*q}' fill='{tc}'/><rect x='{qx0+fx*q+q*.7}' y='{qy0+fy*q+q*.7}' width='{2.6*q}' height='{2.6*q}' fill='#d8cba8'/><rect x='{qx0+fx*q+q*1.3}' y='{qy0+fy*q+q*1.3}' width='{1.4*q}' height='{1.4*q}' fill='{tc}'/>")
    for cx, cy in [(5,1),(6,2),(7,1),(5,3),(4,5),(6,5),(8,5),(10,5),(12,6),(5,7),(7,7),(9,8),(11,8),(10,10),(12,10),(8,10),(6,9),(5,11),(7,12),(9,12),(11,12),(12,12),(10,7),(6,11)]:
        a(f"<rect x='{qx0+cx*q}' y='{qy0+cy*q}' width='{q}' height='{q}' fill='{tc}'/>")
    IX, IY = nx+20, MY0+18                                        # IPEX-Buchse in der Aussparung
    a(f"<rect x='{IX-11}' y='{IY-11}' width='22' height='22' rx='2' fill='#b8bcc0' stroke='#6d7378'/><circle cx='{IX}' cy='{IY}' r='7' fill='#c9a227' stroke='#8a6d1d'/><circle cx='{IX}' cy='{IY}' r='2.5' fill='#8a6d1d'/>")
    # Bereich unter dem Modul: je drei Lötflächen links und rechts, weiße Umrisslinie
    for k in range(3):
        for px in (MX0+2, MX1-20):
            a(f"<rect x='{px}' y='{MY1+8+k*16}' width='18' height='11' fill='#d9dcdf' stroke='#888'/>")
    a(f"<polyline points='{MX0-4},{MY1+50} {MX0-4},{MY1+60} {MX1+4},{MY1+60} {MX1+4},{MY1+50}' fill='none' stroke='#ddd' stroke-width='1.5'/>")
    # Bauteile wie auf dem Foto
    yb = MY1 + 72
    def smd(x, y0, w, h, col='#c9b07a'):
        a(f"<rect x='{x-2}' y='{y0-2}' width='{w+4}' height='{h+4}' fill='none' stroke='#ddd' stroke-width='1'/><rect x='{x}' y='{y0}' width='{w}' height='{h}' fill='{col}'/><rect x='{x}' y='{y0}' width='{w}' height='3' fill='#d9dcdf'/><rect x='{x}' y='{y0+h-3}' width='{w}' height='3' fill='#d9dcdf'/>")
    for k in range(2):                                            # Kondensatoren links (2 × 2)
        smd(MX0-2, yb+4+k*30, 12, 20); smd(MX0+20, yb+4+k*30, 12, 20)
    a(f"<rect x='{DC-30}' y='{yb}' width='50' height='62' fill='#1b1b1b' stroke='#444'/>")           # AMS1117 (SOT-223)
    for k in range(3):
        a(f"<rect x='{DC-44}' y='{yb+6+k*19}' width='14' height='10' fill='#d9dcdf' stroke='#888'/>")
    a(f"<rect x='{DC+20}' y='{yb+14}' width='18' height='34' fill='#d9dcdf' stroke='#888'/>")
    a(f"<text x='{DC-5}' y='{yb+31}' text-anchor='middle' font-size='8' fill='#9a9a9a' transform='rotate(90 {DC-5} {yb+31})'>1117C 33 C352</text>")
    for k in range(2):                                            # Transistoren J3Y (SOT-23) rechts
        jy = yb + 4 + k * 86
        a(f"<rect x='{MX1-40}' y='{jy}' width='40' height='34' fill='none' stroke='#ddd'/><rect x='{MX1-34}' y='{jy+8}' width='28' height='16' fill='#1b1b1b' stroke='#555'/>")
        a(f"<text x='{MX1-20}' y='{jy+19}' text-anchor='middle' font-size='6' fill='#9a9a9a'>J3Y</text>")
        for lx in (MX1-30, MX1-14):
            a(f"<rect x='{lx}' y='{jy+24}' width='4' height='6' fill='#d9dcdf'/>")
        a(f"<rect x='{MX1-22}' y='{jy+2}' width='4' height='6' fill='#d9dcdf'/>")
    ly = yb + 78
    a(f"<rect x='{MX0-2}' y='{ly}' width='12' height='26' fill='none' stroke='#ddd'/><rect x='{MX0}' y='{ly+6}' width='8' height='14' fill='#e8e8e8'/>")   # LED
    for k in range(4):                                            # Widerstände
        a(f"<rect x='{MX0+18}' y='{ly-6+k*12}' width='14' height='8' fill='#1b1b1b' stroke='#bbb'/>")
    cy_ = ly + 18                                                 # CP2102 (QFN)
    a(f"<rect x='{DC-26}' y='{cy_-26}' width='52' height='52' fill='none' stroke='#ddd'/>")
    a(f"<rect x='{DC-22}' y='{cy_-22}' width='44' height='44' rx='2' fill='#232323' stroke='#555'/>")
    for k in range(7):
        for (px, py, w, h) in ((DC-25, cy_-15+k*5, 3, 2), (DC+22, cy_-15+k*5, 3, 2), (DC-15+k*5, cy_-25, 2, 3), (DC-15+k*5, cy_+22, 2, 3)):
            a(f"<rect x='{px}' y='{py}' width='{w}' height='{h}' fill='#bbb'/>")
    a(f"<text x='{DC}' y='{cy_}' text-anchor='middle' font-size='6.5' fill='#aaa' transform='rotate(90 {DC} {cy_})'><tspan x='{DC}' dy='-7'>SILABs</tspan><tspan x='{DC}' dy='7'>CP2102</tspan><tspan x='{DC}' dy='7'>2230+</tspan></text>")
    smd(DC+36, ly+20, 10, 18, '#1b1b1b'); smd(DC+36, ly-8, 10, 18, '#1b1b1b')
    # Taster EN/Boot und USB-C-Buchse (ragt unten über den Rand)
    a(f"<text x='{MX0+8}' y='{y(17)-4}' font-size='13' fill='#fff' font-weight='bold'>EN</text>")
    a(f"<text x='{MX1-8}' y='{y(17)-4}' text-anchor='end' font-size='13' fill='#fff' font-weight='bold'>Boot</text>")
    for bxk in (MX0+14, MX1-14):
        a(f"<rect x='{bxk-15}' y='{y(18)-10}' width='30' height='34' rx='2' fill='#d6d8db' stroke='#8a8a8a'/><circle cx='{bxk}' cy='{y(18)+7}' r='8' fill='#2e2e2e'/>")
        a(f"<rect x='{bxk-6}' y='{y(18)-16}' width='12' height='6' fill='#bfc3c7'/><rect x='{bxk-6}' y='{y(18)+24}' width='12' height='6' fill='#bfc3c7'/>")
    a(f"<rect x='{DC-30}' y='{y(17)+2}' width='60' height='{DY1-y(17)+16}' rx='7' fill='#cfd2d5' stroke='#8a8a8a' stroke-width='1.5'/>")
    for k in range(12):
        a(f"<rect x='{DC-27+k*4.7}' y='{y(17)-10}' width='2.6' height='12' fill='#c9a227'/>")
    for hx in (DC-11, DC+11):
        a(f"<rect x='{hx-3}' y='{y(18)+4}' width='6' height='8' rx='2' fill='#8a8d90'/>")
    a(f"<text x='{DC}' y='{y(19)+74}' text-anchor='middle' font-size='12' fill='#333'>USB-C ← Flachbandkabel ← USB-C-Einbaubuchse (Anschlussseite)</text>")
    # Schraubklemmen; benutzte Klemmen mit Farbmarke
    def terminal(x, yy, c):
        a(f"<line x1='{x-13}' y1='{yy-12}' x2='{x+13}' y2='{yy-12}' stroke='#4aa35a' stroke-width='1'/>")
        a(f"<circle cx='{x}' cy='{yy}' r='8' fill='#7fd18c' stroke='#4aa35a'/><circle cx='{x}' cy='{yy}' r='6' fill='#dfe3e6' stroke='#7b8187'/>")
        a(f"<line x1='{x-4}' y1='{yy}' x2='{x+4}' y2='{yy}' stroke='#5b6066' stroke-width='1.6'/><line x1='{x}' y1='{yy-4}' x2='{x}' y2='{yy+4}' stroke='#5b6066' stroke-width='1.6'/>")
        if c: a(f"<circle cx='{x}' cy='{yy}' r='3' fill='{c}'/>")
    for i in range(1, 20):
        terminal(X0, y(i), used.get(('L', i))); terminal(X1, y(i), used.get(('R', i)))
    def wire(pts, col, dash=False):
        p = ' '.join(f'{x},{yy}' for x, yy in pts)
        a(f"<polyline points='{p}' fill='none' stroke='{col}' stroke-width='3' stroke-linejoin='round' {'stroke-dasharray=\"8 6\"' if dash else ''}/>")
    def res(x, yy, col):
        a(f"<line x1='{x-24}' y1='{yy}' x2='{x+24}' y2='{yy}' stroke='#9aa0a6' stroke-width='2'/>")
        a(f"<rect x='{x-16}' y='{yy-7}' width='32' height='14' rx='6' fill='#e6cf9e' stroke='#9c7c45'/>")
        for k, c in enumerate(('#d62828', '#d62828', '#7a4a2a')):
            a(f"<rect x='{x-11+k*6}' y='{yy-7}' width='3.5' height='14' fill='{c}'/>")
        a(f"<rect x='{x+8}' y='{yy-7}' width='3.5' height='14' fill='#c9a227'/>")
        a(f"<text x='{x}' y='{yy+21}' text-anchor='middle' font-size='10'>220 Ω</text>")
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
        a(f"<line x1='76' y1='{by}' x2='212' y2='{by}' stroke='#b57452' stroke-width='6' {d}/>")         # RG316 (braun)
        # BNC-Stecker: Steckerkopf mit Bajonett-Schlitz, geriffelte Überwurfhülse, Crimphülse, Knickschutz
        a(f"<rect x='6' y='{by-9}' width='12' height='18' rx='1' fill='#c9cdd1' stroke='#6d7378'/><rect x='9' y='{by-9}' width='3' height='7' fill='#6d7378'/>")
        a(f"<rect x='18' y='{by-13}' width='26' height='26' rx='3' fill='#d5d9dd' stroke='#6d7378' stroke-width='1.5'/>")
        for k in range(6):
            a(f"<line x1='{21+k*4}' y1='{by-12}' x2='{21+k*4}' y2='{by+12}' stroke='#a3a9ae' stroke-width='1'/>")
        a(f"<rect x='44' y='{by-8}' width='16' height='16' rx='2' fill='#c3c7cb' stroke='#6d7378'/>")
        a(f"<path d='M60,{by-7} L78,{by-4} L78,{by+4} L60,{by+7} Z' fill='#2a2a2a'/>")
        # Kabelverschraubung Lapp SKINDICHT MINI M6 (Messing vernickelt): Hutmutter außen, Mutter innen
        a(f"<rect x='{WX-4}' y='{by-12}' width='8' height='24' fill='#bfc4c8' stroke='#6d7378'/>")
        a(f"<path d='M{WX-4},{by-10} L{WX-16},{by-7} Q{WX-22},{by} {WX-16},{by+7} L{WX-4},{by+10} Z' fill='#cfd3d7' stroke='#6d7378'/>")
        a(f"<rect x='{WX+4}' y='{by-11}' width='7' height='22' rx='1' fill='#b3b8bd' stroke='#6d7378'/>")
        a(f"<text x='40' y='{by-22}' font-size='13' font-weight='bold'>{lab}</text>")
        a(f"<text x='10' y='{by+30}' font-size='11' fill='#444'>BNC-Stecker {sub}</text>")
        cx = 300 + (pin - 9) * 18
        res(262, by, col)
        wire([(212, by), (240, by)], '#d62828', dash)                                           # rote Ader
        wire([(284, by), (cx, by), (cx, y(pin)), (X0 - 10, y(pin))], col, dash)
        wire([(212, by), (212, by + 18)], '#7a4a2a', dash); gnd(212, by + 18)                  # braune Ader = Schirm
    a(f"<text x='{WX+10}' y='212' font-size='10' fill='#555'>Lapp SKINDICHT MINI M6</text>")
    # GND-Klemme
    wire([(X0 - 10, y(14)), (318, y(14)), (318, y(14) + 16)], '#111'); gnd(318, y(14) + 16)
    a(f"<text x='10' y='590' font-size='11' fill='#111'>⏚ = Klemme GND</text>")
    a(f"<text x='10' y='606' font-size='11' fill='#111'>Pigtail: rot = Innenleiter (Signal),</text>")
    a(f"<text x='10' y='622' font-size='11' fill='#111'>braun = Schirm (Masse)</text>")
    # OLED: Waveshare 2,42" von vorn wie das Original – schwarzes Glas fast über die ganze blaue
    # Platine (61,5 × 39,5 mm), Flexkabel unten mittig. Die Stiftleiste sitzt auf der Rückseite am
    # rechten Rand (von vorn gesehen), Reihenfolge von oben: RST, DC, CS, CLK, DIN, GND, VCC.
    OX, oy0 = 860, 132
    ow, oh = 270, 173                                             # Seitenverhältnis 61,5 × 39,5
    a(f"<rect x='{OX}' y='{oy0}' width='{ow}' height='{oh}' rx='4' fill='#1b4f9c' stroke='#143d78' stroke-width='2'/>")
    gx0, gy0, gw, gh = OX+3, oy0+4, ow-6, oh*37/39.5             # Glas 60,5 × 37 mm
    a(f"<rect x='{gx0}' y='{gy0}' width='{gw}' height='{gh}' rx='2' fill='#07080b' stroke='#2b2f38'/>")
    aw = gw*55/60.5; ah = aw/2                                    # aktive Fläche 55 × 27,5 mm (128 × 64)
    import base64
    png = base64.b64encode(open(os.path.join(OUT, 'oled_pixel.png'), 'rb').read()).decode()
    a(f"<image href='data:image/png;base64,{png}' x='{gx0+(gw-aw)/2}' y='{gy0+(gh-ah)/2-4}' width='{aw}' height='{ah}' style='image-rendering:pixelated'/>")
    fx = OX + ow/2                                                # Flexkabel (orange) unten mittig
    a(f"<path d='M{fx-26},{gy0+gh} L{fx+26},{gy0+gh} L{fx+22},{oy0+oh+6} L{fx-22},{oy0+oh+6} Z' fill='#d98a26' stroke='#9a5e14'/>")
    a(f"<rect x='{fx-12}' y='{oy0+oh-6}' width='24' height='10' fill='#1a1a1a'/>")
    for hx, hy in ((OX+6, oy0+6), (OX+ow-6, oy0+6), (OX+6, oy0+oh-6), (OX+ow-6, oy0+oh-6)):
        a(f"<circle cx='{hx}' cy='{hy}' r='3' fill='#d7dde6' stroke='#9aa6b8'/>")
    a(f"<text x='{OX}' y='{oy0+oh+24}' font-size='12' fill='#222'>Waveshare 2.42\" OLED · SSD1309 · SPI (4-Wire) – optional</text>")
    a(f"<text x='{OX+ow+116}' y='{oy0+oh-4}' text-anchor='end' font-size='9' fill='#555'>Stiftleiste auf der Rückseite</text>")
    names = ['RST', 'DC', 'CS', 'CLK', 'DIN', 'GND', 'VCC']
    pins = {}
    a(f"<rect x='{OX+ow-12}' y='{oy0+12}' width='10' height='{6*22+16}' fill='#111' fill-opacity='.85' rx='1'/>")
    for k, n in enumerate(names):
        py = oy0 + 20 + k * 22
        pins[n] = py
        a(f"<rect x='{OX+ow-10}' y='{py-3}' width='6' height='6' fill='#333'/><rect x='{OX+ow-4}' y='{py-1.5}' width='16' height='3' fill='#d4af37'/>")
        a(f"<text x='{OX+ow+16}' y='{py-4}' font-size='9' fill='#222' font-weight='bold'>{n}</text>")
    # Leitungen zum OLED: über das Modul hinweg zur Stiftleiste rechts – ineinander geschachtelt,
    # damit sich keine Leitungen kreuzen (unterste Klemme → oberster Pin, VCC ganz außen)
    order = [('RST', 13, '#a0522d'), ('DC', 11, '#00a6a6'), ('CS', 10, '#d1495b'), ('CLK', 9, '#7b3fb5'),
             ('DIN', 2, '#2a9d48'), ('GND', 1, '#111')]
    for k, (n, rp, col) in enumerate(order):
        cx, by, dx = 826 - k * 16, oy0 - 10 - k * 10, OX + ow + 50 + k * 10
        wire([(X1 + 10, y(rp)), (cx, y(rp)), (cx, by), (dx, by), (dx, pins[n]), (OX + ow + 12, pins[n])], col)
    # VCC von 3V3 (links oben) ganz außen herum
    vb, vdx = oy0 - 10 - 6 * 10, OX + ow + 50 + 6 * 10
    wire([(X0 - 10, y(1)), (322, y(1)), (322, vb), (vdx, vb), (vdx, pins['VCC']), (OX + ow + 12, pins['VCC'])], '#d62828')
    a(f"<circle cx='840' cy='{vb}' r='4' fill='#d62828'/>")                   # Abzweig 3V3 → RTC VCC
    # Antenne: IPEX-Kabel zur SMA-Buchse, Bingfu-Antenne (schwarz, abwinkelbar) waagerecht
    a(f"<path d='M{IX},{IY} C{IX},10 {X1+60},26 {X1+108},26' fill='none' stroke='#777' stroke-width='4'/>")
    a(f"<path d='M{X1+104},20 L{X1+108},16 L{X1+116},16 L{X1+120},20 L{X1+120},32 L{X1+116},36 L{X1+108},36 L{X1+104},32 Z' fill='#d8b14a' stroke='#8a6d1d'/>")  # SMA-Buchse (Sechskant)
    a(f"<rect x='{X1+120}' y='15' width='18' height='22' rx='2' fill='#222'/>")                       # SMA-Mutter, geriffelt
    for k in range(5):
        a(f"<line x1='{X1+123+k*3.2}' y1='15' x2='{X1+123+k*3.2}' y2='37' stroke='#444' stroke-width='1'/>")
    a(f"<rect x='{X1+138}' y='19' width='10' height='14' fill='#1c1c1c'/><circle cx='{X1+152}' cy='26' r='9' fill='#262626' stroke='#111'/>")  # Gelenk
    a(f"<path d='M{X1+160},18 L{X1+190},16 L{X1+250},16 Q{X1+262},16 {X1+262},26 Q{X1+262},36 {X1+250},36 L{X1+190},36 L{X1+160},34 Z' fill='#1f1f1f' stroke='#000'/>")
    for k in range(3):
        a(f"<path d='M{X1+166+k*6},20 Q{X1+169+k*6},26 {X1+166+k*6},32' fill='none' stroke='#444' stroke-width='1'/>")
    a(f"<text x='{X1+272}' y='24' font-size='12'>SMA-Buchse + WLAN-Antenne (Bingfu, SMA)</text>")
    a(f"<text x='{X1+272}' y='40' font-size='11' fill='#444'>(IPEX→SMA-Pigtail aus dem Antennen-Kit)</text>")
    # Legende
    # --- Optional: Echtzeituhr DS3231 (Modul ZS-042, 38 × 22 mm), gestrichelt angeschlossen
    SDA, SCL = '#c99a00', '#5d6d7e'
    RX, RY, rw, rh = 906, 668, 232, 134
    a(f"<rect x='{RX-28}' y='{RY-46}' width='{rw+42}' height='{rh+62}' rx='8' fill='none' stroke='#888' stroke-dasharray='6 5'/>")
    a(f"<text x='{RX-18}' y='{RY-28}' font-size='12' font-weight='bold' fill='#444'>Optional: Echtzeituhr DS3231 (ZS-042)</text>")
    a(f"<text x='{RX-18}' y='{RY-12}' font-size='11' fill='#444'>Knopfzelle CR2032 auf der Rückseite</text>")
    # Modul ZS-042 wie auf dem Produktfoto (Vorderseite, 38 × 22 mm; Knopfzellenhalter auf der Rückseite).
    # Lage der Bauteile vom Foto übernommen: fx/fy rechnen Foto-Koordinaten (Platine 590 × 330) um.
    fx = lambda v: RX + (v - 100) * rw / 590
    fy = lambda v: RY + (v - 5) * rh / 330
    a(f"<rect x='{RX}' y='{RY}' width='{rw}' height='{rh}' rx='5' fill='#1c63bf' stroke='#124a91' stroke-width='2'/>")
    for hx, hy in ((235, 35), (630, 35), (235, 305)):             # Befestigungslöcher
        a(f"<circle cx='{fx(hx)}' cy='{fy(hy)}' r='5.5' fill='#fff' stroke='#4f8fda' stroke-width='1.5'/>")
    W_ = '#fff'
    a(f"<text x='{fx(140)}' y='{fy(42)}' font-size='7' fill='{W_}' font-family='serif' font-weight='bold'>SBX</text>")
    a(f"<text x='{fx(262)}' y='{fy(34)}' font-size='7' fill='{W_}' font-weight='bold'>POWER</text>")
    def box(x0, y0, x1, y1, fill, txt=''):
        X_, Y_, w, h = fx(x0), fy(y0), fx(x1) - fx(x0), fy(y1) - fy(y0)
        a(f"<rect x='{X_-1.5}' y='{Y_-1.5}' width='{w+3}' height='{h+3}' fill='none' stroke='#fff' stroke-width='.8'/><rect x='{X_}' y='{Y_}' width='{w}' height='{h}' fill='{fill}'/>")
        a(f"<rect x='{X_}' y='{Y_}' width='2.5' height='{h}' fill='#d9dcdf'/><rect x='{X_+w-2.5}' y='{Y_}' width='2.5' height='{h}' fill='#d9dcdf'/>")
        if txt: a(f"<text x='{X_+w/2}' y='{Y_+h/2+2}' text-anchor='middle' font-size='4.5' fill='#ddd'>{txt}</text>")
    box(278, 48, 318, 64, '#f2f2f2'); a(f"<rect x='{fx(292)}' y='{fy(50)}' width='3' height='{fy(62)-fy(50)}' fill='#e05050'/>")  # LED POWER
    box(278, 86, 322, 104, '#1a1a1a', '102')
    box(278, 124, 322, 142, '#b08a5a'); box(278, 164, 322, 182, '#b08a5a')                  # Kondensatoren
    for y0 in (200, 258):                                          # Widerstandsnetzwerke 472
        X_, Y_ = fx(272), fy(y0)
        a(f"<rect x='{X_}' y='{Y_}' width='{fx(318)-X_}' height='{fy(y0+44)-Y_}' fill='#1a1a1a' stroke='#fff' stroke-width='.8'/><text x='{(X_+fx(318))/2}' y='{(Y_+fy(y0+44))/2+2}' text-anchor='middle' font-size='4.5' fill='#ddd'>472</text>")
    # DS3231 (SOIC-16)
    cx0, cx1, cy0, cy1 = fx(358), fx(518), fy(58), fy(165)
    a(f"<rect x='{cx0}' y='{cy0}' width='{cx1-cx0}' height='{cy1-cy0}' rx='1.5' fill='#1b1b1b'/>")
    for k in range(8):
        lx = cx0 + 3 + k * (cx1 - cx0 - 9) / 7
        a(f"<rect x='{lx}' y='{fy(30)}' width='3' height='{cy0-fy(30)}' fill='#c9ccd0'/><rect x='{lx}' y='{cy1}' width='3' height='{fy(192)-cy1}' fill='#c9ccd0'/>")
    a(f"<text x='{(cx0+cx1)/2}' y='{(cy0+cy1)/2+3}' text-anchor='middle' font-size='7' fill='#bbb'>DS3231SN</text>")
    a(f"<circle cx='{cx0+5}' cy='{cy1-5}' r='1.3' fill='#666'/>")
    # AT24C32 (SOIC-8)
    ex0, ex1, ey0, ey1 = fx(358), fx(432), fy(228), fy(288)
    a(f"<rect x='{ex0}' y='{ey0}' width='{ex1-ex0}' height='{ey1-ey0}' rx='1.5' fill='#1b1b1b'/>")
    for k in range(4):
        lx = ex0 + 2 + k * (ex1 - ex0 - 7) / 3
        a(f"<rect x='{lx}' y='{fy(212)}' width='3' height='{ey0-fy(212)}' fill='#c9ccd0'/><rect x='{lx}' y='{ey1}' width='3' height='{fy(304)-ey1}' fill='#c9ccd0'/>")
    a(f"<text x='{(ex0+ex1)/2}' y='{(ey0+ey1)/2}' text-anchor='middle' font-size='3.8' fill='#bbb'>ATMTC086</text><text x='{(ex0+ex1)/2}' y='{(ey0+ey1)/2+5}' text-anchor='middle' font-size='3.8' fill='#bbb'>24C32N</text>")
    # Ladediode (rot) und Widerstand 102 oben rechts
    a(f"<rect x='{fx(530)}' y='{fy(60)}' width='{fx(560)-fx(530)}' height='{fy(140)-fy(60)}' fill='none' stroke='#fff' stroke-width='.8'/><rect x='{fx(536)}' y='{fy(66)}' width='{fx(554)-fx(536)}' height='{fy(134)-fy(66)}' fill='#e8e8e8'/><rect x='{fx(536)}' y='{fy(92)}' width='{fx(554)-fx(536)}' height='{fy(108)-fy(92)}' fill='#d62828'/>")
    box(572, 70, 636, 92, '#1a1a1a', '102')
    a(f"<text x='{fx(445)}' y='{fy(262)}' font-size='9' fill='{W_}' font-weight='bold'>ZS-042.</text>")
    for k, n in enumerate(['A0', 'A1', 'A2']):
        px = fx(540 + k * 32)
        a(f"<rect x='{px}' y='{fy(276)}' width='5' height='4' fill='#c9ccd0'/><rect x='{px}' y='{fy(296)}' width='5' height='4' fill='#c9ccd0'/>")
        a(f"<text x='{px+2.5}' y='{fy(330)}' text-anchor='middle' font-size='6' fill='{W_}'>{n}</text>")
    # zweite Anschlussreihe rechts (SCL, SDA, VCC, GND) im weißen Rahmen
    a(f"<rect x='{fx(652)}' y='{fy(100)}' width='{fx(688)-fx(652)}' height='{fy(258)-fy(100)}' fill='none' stroke='#fff' stroke-width='1'/>")
    for k, n in enumerate(['SCL', 'SDA', 'VCC', 'GND']):
        py = fy(120 + k * 40)
        a(f"<circle cx='{fx(670)}' cy='{py}' r='4' fill='#fff'/><circle cx='{fx(670)}' cy='{py}' r='1.8' fill='#1c63bf'/>")
        a(f"<text x='{fx(645)}' y='{py+3}' text-anchor='end' font-size='8' fill='{W_}'>{n}</text>")
    # Winkel-Stiftleiste links: Pins zeigen nach außen, Beschriftung daneben
    rpins = {}
    a(f"<rect x='{RX+2}' y='{fy(68)}' width='{fx(138)-RX-2}' height='{fy(298)-fy(68)}' fill='#111' rx='1'/>")
    for k, n in enumerate(['32K', 'SQW', 'SCL', 'SDA', 'VCC', 'GND']):      # Reihenfolge wie auf dem Modul
        py = fy(85 + k * 39)
        rpins[n] = py
        a(f"<rect x='{RX-22}' y='{py-1.6}' width='30' height='3.2' fill='#c9ccd0'/><rect x='{RX+4}' y='{py-4.5}' width='9' height='9' fill='#1a1a1a' stroke='#333'/>")
        a(f"<text x='{fx(148)}' y='{py+4}' font-size='10' fill='{W_}' font-weight='bold'>{n}</text>")
    # Klemmen P22 (rechts 3), P21 (rechts 6), GND (rechts 7): erst neben dem Board nach unten,
    # unterhalb der OLED-Leitungen nach rechts, dann links an der Legende vorbei zum Modul
    for n, rp, col, vx, hy, dx2 in (('SCL', 3, SCL, 734, 450, 824), ('SDA', 6, SDA, 726, 462, 816), ('GND', 7, '#111', 718, 474, 808)):
        terminal(X1, y(rp), col)
        wire([(X1 + 10, y(rp)), (vx, y(rp)), (vx, hy), (dx2, hy), (dx2, rpins[n]), (RX - 6, rpins[n])], col, True)
    # VCC von 3V3: von der roten Leitung oben gerade nach unten (zwischen Board und OLED),
    # dann von links an VCC – wie die übrigen RTC-Leitungen
    wire([(840, vb), (846, vb), (846, rpins['VCC']), (RX - 6, rpins['VCC'])], '#d62828', True)
    leg = [('#d62828', '3V3 → OLED VCC'), ('#111', 'GND (Masse)'), ('#e09f00', 'P25 (DAC1) → X'),
           ('#1d6fd6', 'P26 (DAC2) → Y'), ('#2a9d48', 'P23 → DIN'), ('#7b3fb5', 'P18 → CLK'),
           ('#d1495b', 'P5 → CS'), ('#00a6a6', 'P17 → DC'), ('#a0522d', 'P4 → RST'),
           (SDA, 'P21 → SDA (RTC)'), (SCL, 'P22 → SCL (RTC)')]
    for k, (c, t) in enumerate(leg):
        lx, ly = 862 + (k // 6) * 166, 420 + (k % 6) * 22
        a(f"<line x1='{lx}' y1='{ly}' x2='{lx+26}' y2='{ly}' stroke='{c}' stroke-width='4'/><text x='{lx+32}' y='{ly+4}' font-size='11'>{t}</text>")
    a("<line x1='1028' y1='552' x2='1054' y2='552' stroke='#555' stroke-width='3' stroke-dasharray='6 4'/><text x='1060' y='556' font-size='11'>optional</text>")
    a("<text x='862' y='576' font-size='11' fill='#444'>Beschriftung wie auf dem Breakout-Board (P25 = GPIO25 …).</text>")
    a("<text x='862' y='592' font-size='11' fill='#444'>P0 = BOOT-Taster – dort nichts anschließen.</text>")
    a("<text x='862' y='608' font-size='11' fill='#444'>220 Ω in die rote Ader jedes Pigtails eingelötet.</text>")
    save('verdrahtung.svg', 1250, 860, '\n'.join(s))

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
    a(f"<rect x='{bx}' y='{by_}' width='{bw}' height='{bh}' rx='3' fill='#22a54a' stroke='#17803a' stroke-width='2'/>")
    for xx in (bx + 8, bx + bw - 26):
        a(f"<rect x='{xx}' y='{by_+10}' width='18' height='{bh-20}' rx='2' fill='#79d68a' stroke='#3f9b50'/>")
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
    a(f"<text x='{bx+bw}' y='{by_-10}' text-anchor='end' font-size='11' fill='#1d5e2c'>Breakout-Board, DevKit um 180° gedreht</text>")
    a(f"<text x='{bx+bw/2}' y='{by_+bh+16}' text-anchor='middle' font-size='10' fill='#555'>gestrichelt: USB- und Antennenkabel unter dem Board</text>")
    # Antenne mittig auf der Gegenseite (oben)
    a(f"<circle cx='{sx}' cy='{Y}' r='10' fill='#d8b14a' stroke='#8a6d1d'/><circle cx='{sx}' cy='{Y-14}' r='7' fill='#1c1c1c'/>")
    a(f"<path d='M{sx-6},{Y-16} L{sx-3},{Y-66} Q{sx},{Y-72} {sx+3},{Y-66} L{sx+6},{Y-16} Z' fill='#1c1c1c'/>")
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
    a(f"<text x='{X+mm(73)}' y='{Y+mm(9)}' text-anchor='middle' font-size='10' fill='#555'>OLED sitzt im Scharnierdeckel –</text>")
    a(f"<text x='{X+mm(73)}' y='{Y+mm(9)+13}' text-anchor='middle' font-size='10' fill='#555'>Leitungen lang genug lassen</text>")
    # Optional: Echtzeituhr DS3231 (ZS-042, 38 × 22 mm) oben links, 4 Leitungen zur linken Klemmenleiste
    rx, ry = X + mm(5), Y + mm(7)
    a(f"<rect x='{rx}' y='{ry}' width='{mm(38)}' height='{mm(22)}' rx='4' fill='#1e6cc8' fill-opacity='.85' stroke='#14508f' stroke-width='2' stroke-dasharray='7 5'/>")
    a(f"<rect x='{rx+mm(13)}' y='{ry+mm(6)}' width='{mm(13)}' height='{mm(8)}' fill='#1a1a1a'/>")
    a(f"<text x='{rx+mm(19)}' y='{ry+mm(19)}' text-anchor='middle' font-size='10' fill='#fff'>DS3231 (optional)</text>")
    for n in range(4):                                        # GND, 3V3, SDA, SCL senkrecht in die linke Klemmenleiste
        px = bx + 11 + n * 4
        a(f"<line x1='{px}' y1='{ry+mm(22)}' x2='{px}' y2='{by_+24}' stroke='{['#111','#d62828','#c99a00','#5d6d7e'][n]}' stroke-width='2' stroke-dasharray='5 4'/>")
    a(f"<text x='{rx}' y='{ry-6}' font-size='10' fill='#1f5fbf'>RTC: SDA → P21, SCL → P22</text>")
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
