#!/usr/bin/env python3
"""Zeichnet die Schaubilder der Dokumentation als SVG nach docs/images/:
  verdrahtung.svg  – Schaltplan ESP32 DevKitC ↔ Pigtails, OLED, Antenne (OLED-Inhalt aus
                     docs/images/oled_pixel.png, 128 × 64 Pixel aus der OLED-Simulation)
  bohrplan.svg     – Gehäuse-Bohrplan 1:1, Blatt 1/2: Deckel (in mm, zum Ausdrucken mit 100 %)
  bohrplan_seiten.svg – Blatt 2/2: beide Seitenteile
  aufbau.svg       – Anordnung der Teile im Gehäuse (Draufsicht)
  software.svg     – Aufteilung der Firmware auf die beiden Kerne
Aufruf: python3 tools/gen_doc_svg.py
"""
import os
import re
OUT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'docs', 'images')
FONT = "font-family='Helvetica,Arial,sans-serif'"

# Englische Fassung: gleiche Zeichnung, Texte übersetzt (Datei *_en.svg)
EN_BOHRPLAN2 = {'Bohrplan Deckel – Abzweigdose 150 × 100 × 70': 'Drilling template lid – junction box 150 × 100 × 70', 'Bohrplan Seitenteile – Abzweigdose 150 × 100 × 70': 'Drilling template side walls – junction box 150 × 100 × 70', 'ESP32 Oscilloscope Clock v2.0 · ABS-Gehäuse': 'ESP32 Oscilloscope Clock v2.0 · ABS enclosure', 'Fenster mit Bohrungen in den Ecken vorbohren und mit dem Cuttermesser ausschneiden.': 'Pre-drill the window with holes in the corners and cut it out with a utility knife.', 'Anschlussseite – Ansicht von außen': 'Connector side – view from outside', 'Gegenüberliegende Seite – Antenne, Ansicht von außen': 'Opposite side – antenna, view from outside', 'h = Höhe des Unterteils (ohne Deckel), am Gehäuse messen – gezeichnet nicht maßstäblich.': 'h = height of the base (without lid), measure it on the enclosure – not drawn to scale.', 'Schablone mit der Mittellinie auf h/2 ausrichten; alle Löcher liegen auf dieser Linie.': 'Align the template with the centre line at h/2; all holes lie on this line.'}
EN_BOHRPLAN = {'Deckel – Draufsicht': 'Lid – top view', 'Fenster OLED': 'OLED window', 'Befestigungslöcher M2,5 des OLED-Moduls': 'M2.5 mounting holes of the OLED module', '(Lochabstand 61,5 × 39,5) nach dem Modul anreißen': '(hole spacing 61.5 × 39.5): mark them from the module', 'Prüfmaß 50': 'Test line 50', 'Mit 100 % drucken (nicht „an Seite anpassen“),': 'Print at 100 % (not “fit to page”),', 'Prüfmaß nachmessen, Schablonen ausschneiden.': 'measure the test line, cut out the templates.', 'Anschlussseite – Ansicht von außen (Ausschnitt)': 'Connector side – view from outside (detail)', '2× M6 (X, Y)': '2× M6 (X, Y)', 'M6: Kernloch Ø5 bohren und Gewinde schneiden – oder Ø6,2 mit Mutter M6 innen.': 'M6: drill tap hole Ø5 and cut the thread – or Ø6.2 with an M6 nut inside.', 'Höhe der Lochmitten: Mitte des Unterteils, am Gehäuse anreißen.': 'Height of the hole centres: middle of the base, mark it on the enclosure.', 'Gegenüberliegende Seite – Ansicht von außen (Ausschnitt)': 'Opposite side – view from outside (detail)', 'Lochmitte auf gleicher': 'Hole centre at same', 'Höhe wie Anschlussseite': 'height as connector side', 'Ø6,5': 'Ø6.5', '47,5': '47.5', 'Benennung': 'Title', 'Maßstab': 'Scale', 'Blatt': 'Sheet', 'Bohrplan Abzweigdose 150 × 100 × 70': 'Drilling template, junction box 150 × 100 × 70', 'Projekt · Werkstoff': 'Project · material', 'ESP32 Oscilloscope Clock v2.0 · Kunststoffgehäuse': 'ESP32 Oscilloscope Clock v2.0 · plastic enclosure', 'Allgemeintoleranz': 'General tolerance', 'Maße': 'Dimensions', 'in mm': 'in mm'}
EN = {'Stiftleiste auf der Rückseite': 'Pin header on the back', 'DS3231 (optional) – SDA → P21, SCL → P22': 'DS3231 (optional) – SDA → P21, SCL → P22', 'Optional: Echtzeituhr DS3231 (ZS-042)': 'Optional: real-time clock DS3231 (ZS-042)', 'Knopfzelle CR2032 auf der Rückseite': 'Coin cell CR2032 on the back', 'P21 → SDA (RTC)': 'P21 → SDA (RTC)', 'optional': 'optional', 'OLED sitzt im Scharnierdeckel –': 'OLED sits in the hinged lid –', 'Das OLED sitzt im Scharnierdeckel – Leitungen lang genug lassen': 'The OLED sits in the hinged lid – leave the wires long enough', 'Leitungen lang genug lassen': 'leave the wires long enough', '47,5': '47.5', 'ESP32-Breakout-Board (Schraubklemmen), DevKit aufgesteckt': 'ESP32 breakout board (screw terminals), DevKit plugged in', 'USB-C ← Flachbandkabel ← USB-C-Einbaubuchse (Anschlussseite)': 'USB-C ← flat cable ← USB-C panel socket (connector side)', 'Gehäusewand': 'Enclosure wall', 'BNC-Stecker': 'BNC plug', '→ Oszilloskop Kanal 1': '→ oscilloscope channel 1', '→ Oszilloskop Kanal 2': '→ oscilloscope channel 2', '⏚ = Klemme GND': '⏚ = GND terminal', 'Pigtail: rot = Innenleiter (Signal),': 'Pigtail: red = core (signal),', 'schwarz = Schirm (Masse)': 'black = shield (ground)', 'SMA-Buchse + WLAN-Antenne (Bingfu, SMA)': 'SMA socket + WiFi antenna (Bingfu, SMA)', '(IPEX→SMA-Pigtail aus dem Antennen-Kit)': '(IPEX→SMA pigtail from the antenna kit)', 'GND (Masse)': 'GND (ground)', 'Beschriftung wie auf dem Breakout-Board (P25 = GPIO25 …).': 'Labels as printed on the breakout board (P25 = GPIO25 …).', 'P0 = BOOT-Taster – dort nichts anschließen.': 'P0 = BOOT button – do not connect anything there.', '220 Ω in die rote Ader jedes Pigtails eingelötet.': '220 Ω soldered into the red lead of each pigtail.', 'Bohrplan Abzweigdose 150 × 100 × 70 mm – Maßstab 1:1': 'Drilling template, junction box 150 × 100 × 70 mm – scale 1:1', 'DIN A4 mit 100 % drucken (nicht „an Seite anpassen“), dann das Prüfmaß nachmessen': 'Print on A4 at 100 % (not “fit to page”), then measure the test line', 'Deckel (Draufsicht)': 'Lid (top view)', 'Fenster OLED 55 × 28': 'OLED window 55 × 28', 'Befestigungslöcher des OLED-Moduls (61,5 × 39,5 mm) nach dem Modul anzeichnen (M2,5)': 'Mark the mounting holes of the OLED module (61.5 × 39.5 mm) from the module itself (M2.5)', 'Prüfmaß 50 mm': 'Test line 50 mm', 'Nach dem Drucken nachmessen: die Linie muss genau 50 mm lang sein.': 'Measure after printing: the line must be exactly 50 mm long.', 'Schablonen ausschneiden und mit Klebeband auf Deckel und Seiten kleben.': 'Cut out the templates and tape them onto the lid and the sides.', 'Anschlussseite (kurze Seite, von außen gesehen)': 'Connector side (short side, seen from outside)', 'Pigtails: Ø 5,0 bohren + M6-Gewinde schneiden (gestrichelt: M6) – oder Ø 6,2 + M6-Mutter innen': 'Pigtails: drill Ø 5.0 + tap M6 (dashed: M6) – or Ø 6.2 + M6 nut inside', 'Alle drei Löcher auf gleicher Höhe, etwa Mitte des Unterteils': 'All three holes at the same height, about the middle of the base', 'Gegenüberliegende Seite – Antenne': 'Opposite side – antenna', 'SMA  Ø 6,5': 'SMA  Ø 6.5', 'Mittig, gleiche Höhe wie die Anschlüsse': 'Centred, same height as the connectors', 'Anordnung in der Abzweigdose (Draufsicht, Deckel offen)': 'Layout in the junction box (top view, lid open)', 'Breakout-Board samt DevKit um 180° gedreht': 'Breakout board with DevKit rotated by 180°', 'gestrichelt: USB- und Antennenkabel unter dem Board': 'dashed: USB and antenna cable under the board', 'Antenne (SMA)': 'Antenna (SMA)', 'Gegenüberliegende Seite': 'Opposite side', 'Pigtail X · USB-C · Pigtail Y (Kabelverschraubungen SKINDICHT MINI M6)': 'Pigtail X · USB-C · pigtail Y (cable glands SKINDICHT MINI M6)', 'Anschlussseite (kurze Seite)': 'Connector side (short side)', 'OLED (61,5 × 39,5 mm) sitzt im Scharnierdeckel –': 'OLED (61.5 × 39.5 mm) sits in the hinged lid –', 'Leitungen lang genug lassen': 'leave the wires long enough', 'Kern 1 – scopeTask': 'Core 1 – scopeTask', '• nur Strahlausgabe, ohne Pause': '• beam output only, without pause', '• DAC X/Y an GPIO25/26 (Register)': '• DAC X/Y on GPIO25/26 (registers)', '• Rundtour: Zifferblatt + Ziffern': '• round trip: dial + numerals', '• Zeiger nach der Uhrzeit': '• hands according to the time', '• gleich schnelle Linien = gleich hell': '• equally fast lines = equally bright', '• Profil ANALOG/DIGITAL, Testbild': '• profile ANALOG/DIGITAL, test pattern', '• ca. 200 Bilder pro Sekunde': '• approx. 200 frames per second', 'Kern 0 – uiTask + wifiTask': 'Core 0 – uiTask + wifiTask', '• WLAN (stärkstes bekanntes Netz)': '• WiFi (strongest known network)', '• Zeit per NTP, stündlich': '• time via NTP, hourly', '• Webserver, Setup-WLAN, mDNS': '• web server, setup network, mDNS', '• OLED-Anzeige (SPI)': '• OLED display (SPI)', '• BOOT-Taster, Einstellungen (NVS)': '• BOOT button, settings (NVS)', '• Protokoll (Ringpuffer 8 KB)': '• log (ring buffer 8 KB)', '• CPU-Last-Messung': '• CPU load measurement', 'Übergabe nur über wenige atomare Variablen (Uhrzeit, Zifferblatt, Helligkeit, Profil) – keine Sperren im Zeichenpfad': 'Hand-over only via a few atomic variables (time, dial, brightness, profile) – no locks in the drawing path'}

# Grafiken mit themed=True: ohne Hintergrund, Texte/Rahmen über Klassen (th-fg, th-mu, th-ac, th-enc, th-ol).
#  - Datei (README, GitHub): feste helle Farben, dunkle per prefers-color-scheme
#  - zusätzlich direkt in dokumentation.html / documentation.en.html eingebettet (Marke <!-- SVG name -->):
#    Schrift und Farben kommen von der Seite (Projektseite hell/dunkel, im PDF die Rückfallwerte)
TH_CSS = ('.{c} .th-fg{{fill:{fg}}}.{c} .th-mu{{fill:{mu}}}.{c} .th-ac{{fill:{ac}}}'
          '.{c} .th-enc{{fill:{fg};fill-opacity:.04;stroke:{fg};stroke-opacity:.45}}'
          '.{c} .th-ol{{stroke:{mu};stroke-opacity:.6;stroke-width:1}}')
TH_FILE = (TH_CSS.format(c='th', fg='#111', mu='#5f6368', ac='#0a7d44') + '@media (prefers-color-scheme:dark){' +
           TH_CSS.format(c='th', fg='#eee', mu='#a0a0a8', ac='#30d158') + '}')
TH_PAGE = TH_CSS.format(c='thfig', fg='currentColor', mu='var(--mute,#5f6368)', ac='var(--acc,#0a7d44)')
SVG_FONT = "font-family='Raleway,Helvetica Neue,Helvetica,Arial,sans-serif'"


def embed(name, svg_inline):
    """Setzt die Grafik in die Doku ein (ersetzt <img src=\"images/name\" …> bzw. den Inhalt der Marke)."""
    lang_file = 'documentation.en.html' if name.endswith('_en.svg') else 'dokumentation.html'
    doc = os.path.join(os.path.dirname(OUT), lang_file)
    h = open(doc, encoding='utf-8').read()
    a_, b_ = f'<!-- SVG {name} -->', '<!-- /SVG -->'
    if f'<!-- SVG {name} ' not in h:
        m = re.search(r'<img src="images/' + re.escape(name) + r'"(?: style="([^"]*)")?>', h)
        if not m:
            raise SystemExit(f'{doc}: Grafik {name} nicht gefunden')
        h = h[:m.start()] + f'<!-- SVG {name} {m.group(1) or ""} -->' + b_ + h[m.end():]
        a_ = f'<!-- SVG {name} {m.group(1) or ""} -->'
    else:
        k = h.index(f'<!-- SVG {name} ')
        a_ = h[k:h.index('-->', k) + 3]
    style = a_[len(f'<!-- SVG {name} '):-4].strip()
    i = h.index(a_) + len(a_); j = h.index(b_, i)
    h = h[:i] + svg_inline.replace('<svg ', f"<svg style='{style};height:auto' ", 1) + h[j:]
    open(doc, 'w', encoding='utf-8').write(h)


def save(name, w, h, body, extra='', themed=False):
    if themed:
        svg = (f"<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 {w} {h}' class='th' {extra} {SVG_FONT}>\n"
               f"<style>{TH_FILE}</style>\n{body}</svg>\n")
    else:
        svg = (f"<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 {w} {h}' {extra} {FONT}>\n"
               f"<rect x='0' y='0' width='{w}' height='{h}' fill='#fff'/>\n{body}</svg>\n")
    open(os.path.join(OUT, name), 'w', encoding='utf-8').write(svg)
    en = svg
    for de, tr in {**EN, **EN_BOHRPLAN, **EN_BOHRPLAN2}.items():
        en = en.replace('>' + de + '<', '>' + tr + '<')
    if name == 'verdrahtung.svg':                                 # OLED-Inhalt auf Englisch
        import base64
        b64 = lambda f: base64.b64encode(open(os.path.join(OUT, f), 'rb').read()).decode()
        en = en.replace(b64('oled_pixel.png'), b64('oled_pixel_en.png'))
    open(os.path.join(OUT, name.replace('.svg', '_en.svg')), 'w', encoding='utf-8').write(en)
    if themed:
        for n, v in ((name, svg), (name.replace('.svg', '_en.svg'), en)):
            body_ = v[v.index('</style>') + 9:v.rindex('</svg>')]
            embed(n, f"<svg class='thfig' viewBox='0 0 {w} {h}' role='img'><style>{TH_PAGE}</style>{body_}</svg>")

# ---------------------------------------------------------------- Verdrahtung
def zs042(a, RX, RY, rw, rh, pinlen=30):
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
        a(f"<rect x='{RX-pinlen+8}' y='{py-1.6}' width='{pinlen}' height='3.2' fill='#c9ccd0'/><rect x='{RX+4}' y='{py-4.5}' width='9' height='9' fill='#1a1a1a' stroke='#333'/>")
        a(f"<text x='{fx(148)}' y='{py+4}' font-size='10' fill='{W_}' font-weight='bold'>{n}</text>")
    return rpins


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
    used = {('L',1):'#d62828',('L',9):'#d62828',('L',10):'#d62828',('L',14):'#111',
            ('R',1):'#111',('R',2):'#1f5fd1',('R',9):'#f2c200',('R',10):'#f07b1d',('R',11):'#1f9d55',('R',13):'#f7f7f7'}   # Farben wie an den Kabeln (OLED)
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
        if c: a(f"<circle cx='{x}' cy='{yy}' r='3' fill='{c}' stroke='#5b6066' stroke-width='.8'/>")
    for i in range(1, 20):
        terminal(X0, y(i), used.get(('L', i))); terminal(X1, y(i), used.get(('R', i)))
    WHITE = '#f7f7f7'                                            # weiße Ader: mit grauem Rand, sonst unsichtbar
    def wire(pts, col, dash=False):
        p = ' '.join(f'{x},{yy}' for x, yy in pts)
        if col == WHITE:
            a(f"<polyline points='{p}' fill='none' stroke='#8a8f94' stroke-width='5' stroke-linejoin='round' {'stroke-dasharray=\"8 6\"' if dash else ''}/>")
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
    for lab, sub, by, pin, col, dash in [('Pigtail X', '→ Oszilloskop Kanal 1', 250, 9, '#d62828', False),
                                        ('Pigtail Y', '→ Oszilloskop Kanal 2', 370, 10, '#d62828', False)]:
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
        a(f"<text x='10' y='{by+30}' font-size='11' fill='#444'>BNC-Stecker</text>")               # links der Gehäusewand,
        a(f"<text x='10' y='{by+44}' font-size='11' fill='#444'>{sub}</text>")                       # zweizeilig (keine Überlappung)
        cx = 300 + (pin - 9) * 18
        res(262, by, col)
        wire([(212, by), (240, by)], '#d62828', dash)                                           # rote Ader
        wire([(284, by), (cx, by), (cx, y(pin)), (X0 - 10, y(pin))], col, dash)
        wire([(212, by), (212, by + 18)], '#111', dash); gnd(212, by + 18)                     # schwarze Ader = Schirm
    a(f"<text x='{WX+10}' y='212' font-size='10' fill='#555'>Lapp SKINDICHT MINI M6</text>")
    # GND-Klemme
    wire([(X0 - 10, y(14)), (318, y(14)), (318, y(14) + 16)], '#111'); gnd(318, y(14) + 16)
    a(f"<text x='10' y='590' font-size='11' fill='#111'>⏚ = Klemme GND</text>")
    a(f"<text x='10' y='606' font-size='11' fill='#111'>Pigtail: rot = Innenleiter (Signal),</text>")
    a(f"<text x='10' y='622' font-size='11' fill='#111'>schwarz = Schirm (Masse)</text>")
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
    # Farben wie am Kabel des Waveshare-OLED: RST weiß, DC grün, CS orange, CLK gelb, DIN blau, GND schwarz, VCC rot
    order = [('RST', 13, WHITE), ('DC', 11, '#1f9d55'), ('CS', 10, '#f07b1d'), ('CLK', 9, '#f2c200'),
             ('DIN', 2, '#1f5fd1'), ('GND', 1, '#111')]
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
    SDA, SCL = '#8b5a2b', '#9a9ea3'                               # DuPont-Kabel: SDA braun, SCL grau, VCC rot, GND schwarz
    RX, RY, rw, rh = 906, 668, 232, 134
    a(f"<rect x='{RX-28}' y='{RY-46}' width='{rw+42}' height='{rh+62}' rx='8' fill='none' stroke='#888' stroke-dasharray='6 5'/>")
    a(f"<text x='{RX-18}' y='{RY-28}' font-size='12' font-weight='bold' fill='#444'>Optional: Echtzeituhr DS3231 (ZS-042)</text>")
    a(f"<text x='{RX-18}' y='{RY-12}' font-size='11' fill='#444'>Knopfzelle CR2032 auf der Rückseite</text>")
    rpins = zs042(a, RX, RY, rw, rh)
    # Klemmen P22 (rechts 3), P21 (rechts 6), GND (rechts 7): erst neben dem Board nach unten,
    # unterhalb der OLED-Leitungen nach rechts, dann links an der Legende vorbei zum Modul
    for n, rp, col, vx, hy, dx2 in (('SCL', 3, SCL, 734, 450, 824), ('SDA', 6, SDA, 726, 462, 816), ('GND', 7, '#111', 718, 474, 808)):
        terminal(X1, y(rp), col)
        wire([(X1 + 10, y(rp)), (vx, y(rp)), (vx, hy), (dx2, hy), (dx2, rpins[n]), (RX - 6, rpins[n])], col, True)
    # VCC von 3V3: von der roten Leitung oben gerade nach unten (zwischen Board und OLED),
    # dann von links an VCC – wie die übrigen RTC-Leitungen
    wire([(840, vb), (846, vb), (846, rpins['VCC']), (RX - 6, rpins['VCC'])], '#d62828', True)
    leg = [('#d62828', '3V3 → OLED VCC'), ('#111', 'GND (Masse)'), ('#d62828', 'P25 (DAC1) → X'),
           ('#d62828', 'P26 (DAC2) → Y'), ('#1f5fd1', 'P23 → DIN'), ('#f2c200', 'P18 → CLK'),
           ('#f07b1d', 'P5 → CS'), ('#1f9d55', 'P17 → DC'), (WHITE, 'P4 → RST'),
           (SDA, 'P21 → SDA (RTC)'), (SCL, 'P22 → SCL (RTC)')]
    for k, (c, t) in enumerate(leg):
        lx, ly = 862 + (k // 6) * 166, 420 + (k % 6) * 22
        if c == WHITE: a(f"<line x1='{lx}' y1='{ly}' x2='{lx+26}' y2='{ly}' stroke='#8a8f94' stroke-width='6'/>")
        a(f"<line x1='{lx}' y1='{ly}' x2='{lx+26}' y2='{ly}' stroke='{c}' stroke-width='4'/><text x='{lx+32}' y='{ly+4}' font-size='11'>{t}</text>")
    a("<line x1='1028' y1='552' x2='1054' y2='552' stroke='#555' stroke-width='3' stroke-dasharray='6 4'/><text x='1060' y='556' font-size='11'>optional</text>")
    a("<text x='862' y='576' font-size='11' fill='#444'>Beschriftung wie auf dem Breakout-Board (P25 = GPIO25 …).</text>")
    a("<text x='862' y='592' font-size='11' fill='#444'>P0 = BOOT-Taster – dort nichts anschließen.</text>")
    a("<text x='862' y='608' font-size='11' fill='#444'>220 Ω in die rote Ader jedes Pigtails eingelötet.</text>")
    save('verdrahtung.svg', 1250, 860, '\n'.join(s))

# ---------------------------------------------------------------- Bohrplan 1:1
def bohrplan():
    """Bohrplan als technische Zeichnung, Maßstab 1:1 auf DIN A4 (Maße in mm, viewBox = mm).
    Regeln nach „Grundlagen des Technischen Zeichnens“ (TUM) bzw. DIN ISO 128 / DIN ISO 129:
      - sichtbare Kanten breite Volllinie (0,5), Maß-, Maßhilfs- und Hinweislinien schmale Volllinie (0,25)
      - Maßhilfslinien ragen 2 mm über die Maßlinie, Maßpfeile ausgefüllt, Maßzahlen 3,5 mm hoch,
        über der Maßlinie, lesbar von unten bzw. rechts, ohne Einheit (mm), nicht von Linien gekreuzt
      - Bemaßung von Bezugskanten aus (linke und obere Kante), keine Überbemaßung
      - Bohrungen mit Ø und Mittellinienkreuz (schmale Strich-Punkt-Linie, bei kleinen Bohrungen schmale Volllinie)
      - Innengewinde in der Ansicht: Kernloch breite Volllinie, Gewinde-Außendurchmesser schmaler ¾-Kreis
      - zwei Blätter: 1/2 Deckel, 2/2 beide Seitenteile komplett. Die Höhe h des Unterteils steht im
        Datenblatt nicht – sie wird am Gehäuse gemessen; die Lochmitten liegen auf h/2 (Mittellinie zum Ausrichten)
      - Zeichnungsrahmen (links 20 mm Heftrand, sonst 10 mm) und Schriftfeld unten rechts"""
    import math
    pages = []
    a = lambda t: pages[-1].append(t)                           # zeichnet auf die aktuelle Seite
    B, N = 0.5, 0.25                                              # Linienbreiten: breit / schmal
    TXT = 3.5                                                     # Schrifthöhe Maßzahlen und Texte
    def line(x1, y1, x2, y2, w=N, dash=None):
        d = f" stroke-dasharray='{dash}'" if dash else ''
        a(f"<line x1='{x1:.2f}' y1='{y1:.2f}' x2='{x2:.2f}' y2='{y2:.2f}' stroke='#000' stroke-width='{w}'{d}/>")
    def text(x, y, t, size=TXT, anchor='middle', bold=False, rot=None):
        r = f" transform='rotate({rot} {x:.2f} {y:.2f})'" if rot is not None else ''
        b = " font-weight='bold'" if bold else ''
        a(f"<text x='{x:.2f}' y='{y:.2f}' text-anchor='{anchor}' font-size='{size}'{b}{r}>{t}</text>")
    def arrow(x, y, ang):                                         # ausgefüllter Maßpfeil, Spitze bei (x, y), zeigt in Richtung ang
        L, h = 2.5, 0.67                                          # 2,5 mm lang, Öffnungswinkel 15°
        c, sn = math.cos(ang), math.sin(ang)
        bx, by = x - L * c, y - L * sn
        a(f"<path d='M{x:.2f},{y:.2f} L{bx - h*sn:.2f},{by + h*c:.2f} L{bx + h*sn:.2f},{by - h*c:.2f} Z' fill='#000'/>")
    def dim_h(xa, xb, y, ya, yb, t):                              # waagerechtes Maß zwischen xa und xb auf Höhe y
        for x, yo in ((xa, ya), (xb, yb)):
            line(x, yo, x, y + (2 if y > yo else -2))             # Maßhilfslinie, 2 mm über die Maßlinie hinaus
        line(xa, y, xb, y); arrow(xa, y, math.pi); arrow(xb, y, 0)
        text((xa + xb) / 2, y - 1, t)
    def dim_v(ya, yb, x, xa, xb, t):                              # senkrechtes Maß, Maßzahl von rechts lesbar
        for yy, xo in ((ya, xa), (yb, xb)):
            line(xo, yy, x + (2 if x > xo else -2), yy)
        line(x, ya, x, yb); arrow(x, ya, -math.pi / 2); arrow(x, yb, math.pi / 2)
        text(x - 1, (ya + yb) / 2, t, rot=-90)
    def centre(cx, cy, r, big):                                   # Mittellinienkreuz
        e = r + (3 if big else 2)
        dash = '6 1 0.5 1' if big else None                       # Strich-Punkt-Linie bei größeren Bohrungen
        line(cx - e, cy, cx + e, cy, N, dash); line(cx, cy - e, cx, cy + e, N, dash)
    def leader(cx, cy, r, ang_deg, kx, ky, t, right=True):        # Hinweislinie mit Pfeil auf den Kreis
        ang = math.radians(ang_deg)
        px, py = cx + r * math.cos(ang), cy - r * math.sin(ang)
        line(px, py, kx, ky)
        arrow(px, py, math.atan2(py - ky, px - kx))
        ex = kx + (8 if right else -8)
        line(kx, ky, ex, ky)
        text(kx + (0.5 if right else -0.5), ky - 1, t, anchor='start' if right else 'end')
    def breakline(x1, x2, y):                                     # Bruchkante als schmale Freihandlinie
        pts = []; n = 24
        for q in range(n + 1):
            x = x1 - 2 + (x2 - x1 + 4) * q / n
            pts.append(f"{x:.2f},{y + 0.8 * math.sin(q * 1.7) + 0.4 * math.sin(q * 3.1):.2f}")
        a(f"<polyline points='{' '.join(pts)}' fill='none' stroke='#000' stroke-width='{N}'/>")

    def sheet(title, nr):                                         # neues Blatt: Rahmen, Schriftfeld, Prüfmaß
        pages.append([])
        a(f"<rect x='20' y='10' width='180' height='277' fill='none' stroke='#000' stroke-width='0.7'/>")
        TY = 267
        a(f"<rect x='20' y='{TY}' width='180' height='20' fill='none' stroke='#000' stroke-width='0.7'/>")
        line(20, TY + 10, 200, TY + 10, N); line(130, TY, 130, TY + 20, N); line(165, TY, 165, TY + 20, N)
        def cell(x, y, label, value, bold=False):
            text(x + 1.5, y + 3, label, 2.5, 'start')
            text(x + 1.5, y + 8.3, value, 3.5, 'start', bold)
        cell(20, TY, 'Benennung', title, True)
        cell(130, TY, 'Maßstab', '1:1')
        cell(165, TY, 'Blatt', nr)
        cell(20, TY + 10, 'Projekt · Werkstoff', 'ESP32 Oscilloscope Clock v2.0 · ABS-Gehäuse')
        cell(130, TY + 10, 'Allgemeintoleranz', 'ISO 2768-m')
        cell(165, TY + 10, 'Maße', 'in mm')
        # Prüfmaß 50 mm über dem Schriftfeld
        py = 258
        line(40, py, 90, py, B); line(40, py - 2.5, 40, py + 2.5, B); line(90, py - 2.5, 90, py + 2.5, B)
        text(65, py - 2, 'Prüfmaß 50')
        text(98, py - 1.5, 'Mit 100 % drucken (nicht „an Seite anpassen“),', anchor='start')
        text(98, py + 3.5, 'Prüfmaß nachmessen, Schablonen ausschneiden.', anchor='start')

    # ======== Blatt 1/2: Deckel, Draufsicht (150 × 100), Bezugskanten links und oben
    sheet('Bohrplan Deckel – Abzweigdose 150 × 100 × 70', '1 / 2')
    X, Y = 40, 50
    text(22, 18, 'Deckel – Draufsicht', 4.2, 'start', True)
    a(f"<rect x='{X}' y='{Y}' width='150' height='100' fill='none' stroke='#000' stroke-width='{B}'/>")
    wx, wy = X + 47.5, Y + 22
    a(f"<rect x='{wx}' y='{wy}' width='55' height='28' fill='none' stroke='#000' stroke-width='{B}'/>")
    dim_h(X, X + 150, Y - 12, Y, Y, '150')
    dim_h(X, wx, Y - 6, Y, wy, '47,5'); dim_h(wx, wx + 55, Y - 6, wy, wy, '55')
    dim_v(Y, Y + 100, X - 12, X, X, '100')
    vx = wx - 8                                                   # innen links neben dem Fenster
    line(wx, wy, vx - 2, wy); line(wx, wy + 28, vx - 2, wy + 28)
    line(vx, Y, vx, wy); arrow(vx, Y, -math.pi / 2); arrow(vx, wy, math.pi / 2); text(vx - 1, (Y + wy) / 2, '22', rot=-90)
    line(vx, wy, vx, wy + 28); arrow(vx, wy, -math.pi / 2); arrow(vx, wy + 28, math.pi / 2); text(vx - 1, wy + 14, '28', rot=-90)
    text(X + 75, wy + 14 + 1.2, 'Fenster OLED')
    text(X + 75, Y + 76, 'Befestigungslöcher M2,5 des OLED-Moduls')
    text(X + 75, Y + 81, '(Lochabstand 61,5 × 39,5) nach dem Modul anreißen')
    text(22, Y + 120, 'Fenster mit Bohrungen in den Ecken vorbohren und mit dem Cuttermesser ausschneiden.', anchor='start')
    save('bohrplan.svg', 210, 297, '\n'.join(pages[-1]), "width='210mm' height='297mm'")

    # ======== Blatt 2/2: beide Seitenteile komplett (100 × h), Ansicht von außen
    sheet('Bohrplan Seitenteile – Abzweigdose 150 × 100 × 70', '2 / 2')
    HD = 50                                                       # gezeichnete Höhe des Unterteils (h wird gemessen)
    def wall(X2, Y2, heading, holes, leaders):
        text(22, Y2 - 22, heading, 4.2, 'start', True)
        a(f"<rect x='{X2}' y='{Y2}' width='100' height='{HD}' fill='none' stroke='#000' stroke-width='{B}'/>")
        cy = Y2 + HD / 2
        line(X2 - 4, cy, X2 + 104, cy, N, '6 1 0.5 1')            # Mittellinie der Bohrungen (h/2) zum Ausrichten
        dim_h(X2, X2 + 100, Y2 - 8, Y2, Y2, '100')
        dim_v(Y2, Y2 + HD, X2 - 14, X2, X2, 'h')
        dim_v(cy, Y2 + HD, X2 - 7, X2 - 4, X2, 'h/2')
        for d, kind in holes:
            cx = X2 + d
            if kind == 'M6':                                      # Kernloch Ø5 breit, Gewinde M6 als schmaler ¾-Kreis
                a(f"<circle cx='{cx}' cy='{cy}' r='2.5' fill='none' stroke='#000' stroke-width='{B}'/>")
                a(f"<path d='M{cx + 3:.2f},{cy:.2f} A3,3 0 1 0 {cx:.2f},{cy + 3:.2f}' fill='none' stroke='#000' stroke-width='{N}'/>")
                line(cx, cy - 5, cx, cy + 5)
            else:
                r = kind / 2
                a(f"<circle cx='{cx}' cy='{cy}' r='{r}' fill='none' stroke='#000' stroke-width='{B}'/>")
                line(cx, cy - r - (3 if r > 4 else 2), cx, cy + r + (3 if r > 4 else 2), N, '6 1 0.5 1' if r > 4 else None)
        for k, (d, kind) in enumerate(holes):                     # Lochabstände von der linken Kante
            rr = 3 if kind == 'M6' else kind / 2
            dim_h(X2, X2 + d, Y2 + HD + 7 + k * 6, Y2 + HD, Y2 + HD, str(d))
            line(X2 + d, cy + rr + 2, X2 + d, Y2 + HD)            # Mittellinie bis zur Kante verlängert
        for args in leaders:
            leader(*args)

    X2 = 55
    wall(X2, 52, 'Anschlussseite – Ansicht von außen',
         [(28, 'M6'), (50, 16), (72, 'M6')],
         [(X2 + 28, 52 + HD / 2, 3, 135, X2 + 6, 52 + 9, '2× M6 (X, Y)', True),
          (X2 + 50, 52 + HD / 2, 8, 30, X2 + 80, 52 + 6, 'Ø16')])
    wall(X2, 168, 'Gegenüberliegende Seite – Antenne, Ansicht von außen',
         [(50, 6.5)],
         [(X2 + 50, 168 + HD / 2, 3.25, 45, X2 + 70, 168 + 8, 'Ø6,5')])
    nb = 168 + HD + 18
    text(22, nb, 'h = Höhe des Unterteils (ohne Deckel), am Gehäuse messen – gezeichnet nicht maßstäblich.', anchor='start')
    text(22, nb + 5, 'Schablone mit der Mittellinie auf h/2 ausrichten; alle Löcher liegen auf dieser Linie.', anchor='start')
    text(22, nb + 10, 'M6: Kernloch Ø5 bohren und Gewinde schneiden – oder Ø6,2 mit Mutter M6 innen.', anchor='start')
    save('bohrplan_seiten.svg', 210, 297, '\n'.join(pages[-1]), "width='210mm' height='297mm'")

# ---------------------------------------------------------------- Anordnung im Gehäuse
def devkit_top(a, x0, y0, k):
    """ESP32 DevKitC V4 (28,5 × 48,5 mm) von oben wie auf dem Produktfoto: Modul oben, USB-C unten.
    Ursprung oben links, k Pixel je mm."""
    m = lambda v: v * k
    a(f"<rect x='{x0}' y='{y0}' width='{m(28.5)}' height='{m(48.5)}' rx='{m(.6)}' fill='#151515' stroke='#000'/>")
    for i in range(19):                                           # verzinnte Lötaugen
        for px in (1.55, 26.95):
            a(f"<circle cx='{x0+m(px)}' cy='{y0+m(1.4+i*2.54)}' r='{m(.85)}' fill='#c3c7cb' stroke='#555' stroke-width='.6'/>")
    mx0, mx1, my0, my1 = 5.25, 23.25, .5, 19.7                   # Modul 18 × 19,2 mm
    a(f"<rect x='{x0+m(mx0)}' y='{y0+m(my0)}' width='{m(mx1-mx0)}' height='{m(my1-my0)}' fill='#1e1e1e'/>")
    for i in range(12):
        for px in (mx0+.1, mx1-1.1):
            a(f"<rect x='{x0+m(px)}' y='{y0+m(4.2+i*1.27)}' width='{m(1)}' height='{m(.7)}' fill='#d9dcdf'/>")
    sx0, sx1, sy0, sy1, nx, ny = mx0+1.3, mx1-1.3, my0+.4, my1-1.4, mx1-5.6, my0+4
    a(f"<path d='M{x0+m(sx0)},{y0+m(sy0)} L{x0+m(nx)},{y0+m(sy0)} L{x0+m(nx)},{y0+m(ny)} L{x0+m(sx1)},{y0+m(ny)} L{x0+m(sx1)},{y0+m(sy1)} L{x0+m(sx0)},{y0+m(sy1)} Z' fill='#c8b88f' stroke='#8d7f5c'/>")
    a(f"<text x='{x0+m(sx0+1)}' y='{y0+m(sy0+2.6)}' font-size='{m(1.25)}' fill='#4d4535' font-weight='bold'>ESP32-WROOM-32U</text>")
    for qx in range(5):                                           # QR-Code (angedeutet)
        for qy in range(5):
            if (qx * 7 + qy * 3 + qx * qy) % 3 or qx in (0, 4) and qy in (0, 4):
                a(f"<rect x='{x0+m(sx1-5+qx)}' y='{y0+m(sy1-5+qy)}' width='{m(1)}' height='{m(1)}' fill='#4d4535'/>")
    ix, iy = x0 + m(nx + 2.2), y0 + m(my0 + 2)                    # IPEX-Buchse in der Aussparung
    a(f"<rect x='{ix-m(1.2)}' y='{iy-m(1.2)}' width='{m(2.4)}' height='{m(2.4)}' fill='#b8bcc0'/><circle cx='{ix}' cy='{iy}' r='{m(.8)}' fill='#c9a227' stroke='#8a6d1d'/>")
    a(f"<rect x='{x0+m(8.5)}' y='{y0+m(25.5)}' width='{m(5.5)}' height='{m(6.5)}' fill='#1b1b1b' stroke='#444'/><rect x='{x0+m(14)}' y='{y0+m(26.8)}' width='{m(1.8)}' height='{m(3.8)}' fill='#d9dcdf'/>")  # AMS1117
    a(f"<rect x='{x0+m(11.8)}' y='{y0+m(33.5)}' width='{m(5)}' height='{m(5)}' fill='#232323' stroke='#999' stroke-width='.6'/>")  # CP2102
    for jy in (26, 36):                                           # Transistoren J3Y
        a(f"<rect x='{x0+m(20)}' y='{y0+m(jy)}' width='{m(3.4)}' height='{m(2.2)}' fill='#1b1b1b' stroke='#ddd' stroke-width='.6'/>")
    for cx_, cy_ in ((6, 26), (8, 26), (6, 29), (8, 29)):         # Kondensatoren
        a(f"<rect x='{x0+m(cx_-.6)}' y='{y0+m(cy_)}' width='{m(1.2)}' height='{m(2)}' fill='#c9b07a'/>")
    for bx_ in (5.3, 23.2):                                       # Taster EN / Boot
        a(f"<rect x='{x0+m(bx_-1.8)}' y='{y0+m(43.3)}' width='{m(3.6)}' height='{m(3.8)}' fill='#d6d8db' stroke='#888' stroke-width='.6'/><circle cx='{x0+m(bx_)}' cy='{y0+m(45.2)}' r='{m(1)}' fill='#2e2e2e'/>")
    a(f"<text x='{x0+m(4)}' y='{y0+m(42.3)}' font-size='{m(1.5)}' fill='#fff' font-weight='bold'>EN</text><text x='{x0+m(24.6)}' y='{y0+m(42.3)}' text-anchor='end' font-size='{m(1.5)}' fill='#fff' font-weight='bold'>Boot</text>")
    a(f"<rect x='{x0+m(10.5)}' y='{y0+m(42.5)}' width='{m(7.5)}' height='{m(7)}' rx='{m(1)}' fill='#cfd2d5' stroke='#888' stroke-width='.8'/>")  # USB-C, ragt über den Rand
    return {'ipex': (ix, iy), 'usb': (x0 + m(14.25), y0 + m(49.5))}


def breakout_top(a, x0, y0, k, L, R, used):
    """Breakout-Board (63 × 77 mm) von oben wie das Original, mit aufgestecktem DevKit.
    Gibt die Lage der Klemmen zurück: {('L', i): (x, y), ('R', i): (x, y)}."""
    m = lambda v: v * k
    W, H = 63, 77
    a(f"<rect x='{x0}' y='{y0}' width='{m(W)}' height='{m(H)}' rx='{m(1)}' fill='#22a54a' stroke='#17803a' stroke-width='2'/>")
    for hx, hy in ((2.6, 2.6), (W-2.6, 2.6), (2.6, H-2.6), (W-2.6, H-2.6)):
        a(f"<circle cx='{x0+m(hx)}' cy='{y0+m(hy)}' r='{m(1.4)}' fill='#fff' stroke='#d7efd9'/>")
    pos = {}
    t0 = (H - 18 * 3.5) / 2
    for side, cx_, names in (('L', 8.5, L), ('R', W-8.5, R)):
        a(f"<rect x='{x0+m(cx_-3.4)}' y='{y0+m(t0-2.2)}' width='{m(6.8)}' height='{m(18*3.5+4.4)}' rx='2' fill='#93e09f' stroke='#4aa35a'/>")
        for i, n in enumerate(names, 1):
            ty = y0 + m(t0 + (i - 1) * 3.5)
            pos[(side, i)] = (x0 + m(cx_), ty)
            a(f"<circle cx='{x0+m(cx_)}' cy='{ty}' r='{m(1.25)}' fill='#dfe3e6' stroke='#7b8187' stroke-width='.8'/>")
            a(f"<path d='M{x0+m(cx_)-3},{ty} h6 M{x0+m(cx_)},{ty-3} v6' stroke='#5b6066' stroke-width='1.2'/>")
            c = used.get((side, i))
            if c: a(f"<circle cx='{x0+m(cx_)}' cy='{ty}' r='2.2' fill='{c}'/>")
            pos[(side, i, 'label')] = (n, bool(c))                # Beschriftung zeichnet der Aufrufer (lesbar)
    for hx in (18.8 - 1.3, 44.2 - 1.3):                           # Buchsenleisten unter dem DevKit
        a(f"<rect x='{x0+m(hx)}' y='{y0+m(13.2)}' width='{m(2.6)}' height='{m(50.6)}' fill='#111'/>")
    dk = devkit_top(a, x0 + m((W - 28.5) / 2), y0 + m((H - 48.5) / 2 - 1), k)
    return pos, dk


def aufbau():
    """Draufsicht, maßstäblich (4 px = 1 mm), wie im Bohrplan: Anschlussseite (100 mm) unten mit
    Pigtail X (28 mm), USB-C (50 mm) und Pigtail Y (72 mm); Antenne mittig auf der Gegenseite oben.
    Das Breakout-Board ist samt (richtig aufgestecktem) DevKit um 180° gedreht eingebaut:
    Modul/IPEX zur Anschlussseite, USB-C zur Antennenseite, Klemmen P25/P26 rechts.
    Antennen- und USB-Kabel sind zu lang und laufen deshalb einmal unter dem Breakout-Board durch.
    Bauteile wie auf den Produktfotos (Breakout-Board XTVTX, DevKitC V4, Bingfu-Antenne). Die optionale
    Echtzeituhr ist hier bewusst nicht eingezeichnet (nur im Verdrahtungsplan)."""
    k = 4; X, Y = 150, 165; W, H = 100 * k, 150 * k
    s = []; a = s.append
    mm = lambda v: v * k
    a(f"<text class='th-fg' x='{X+W/2}' y='26' text-anchor='middle' font-size='17' font-weight='bold'>Anordnung in der Abzweigdose (Draufsicht, Deckel offen)</text>")
    a(f"<rect x='{X}' y='{Y}' width='{W}' height='{H}' rx='16' class='th-enc' stroke-width='3'/>")
    yb = Y + H
    sx = X + mm(50)
    # Breakout-Board 63 × 77 mm mittig, samt DevKit um 180° gedreht
    bw, bh = mm(63), mm(77)
    bx, by_ = X + (W - bw) / 2, Y + mm(40)
    cx, cy = bx + bw / 2, by_ + bh / 2
    rot = lambda p: (2 * cx - p[0], 2 * cy - p[1])
    L = ['3V3','EN','SVP','SVN','P34','P35','P32','P33','P25','P26','P27','P14','P12','GND','P13','SD2','SD3','CMD','5V']
    R = ['GND','P23','P22','TX','RX','P21','GND','P19','P18','P5','P17','P16','P4','P0','P2','P15','SD1','SD0','CLK']
    used = {('L', 9): '#d62828', ('L', 10): '#d62828'}        # P25/P26 (Pigtails); die optionale RTC zeigt nur der Schaltplan
    g = []
    pos, dk = breakout_top(g.append, bx, by_, k, L, R, used)
    a(f"<g transform='rotate(180 {cx} {cy})'>" + '\n'.join(g) + "</g>")
    for (side, i), (tx, ty) in [(kk, v) for kk, v in pos.items() if len(kk) == 2]:   # Klemmenaufdruck lesbar
        n, bold = pos[(side, i, 'label')]
        if bold:
            continue                                              # P25/P26: die rote Ader läge über der Schrift – Beschriftung rechts außen
        px, py = rot((tx, ty))
        anc, dx_ = ('start', mm(3.9)) if side == 'L' else ('end', -mm(3.9))     # L-Leiste liegt jetzt rechts
        a(f"<text x='{px+dx_}' y='{py+2.5}' text-anchor='{anc}' font-size='7' fill='#fff' {'font-weight=\"bold\"' if bold else ''}>{n}</text>")
    ix, iy = rot(dk['ipex']); ux0, uy0 = rot(dk['usb'])
    usb = '#c98a2e'; ant = '#777'
    ux, ax = cx - mm(14), cx + mm(14)                             # Spuren unter dem Board
    # USB-Flachbandkabel (FPC, bernsteinfarben): von der DevKit-Buchse (oben) Schleife, unter dem Board zur Einbaubuchse
    a(f"<path d='M{ux0},{uy0} C{ux0},{by_-45} {ux},{by_-45} {ux},{by_}' fill='none' stroke='{usb}' stroke-width='10' stroke-opacity='.9'/>")
    a(f"<line x1='{ux}' y1='{by_}' x2='{ux}' y2='{by_+bh}' stroke='{usb}' stroke-width='10' stroke-dasharray='10 7' stroke-opacity='.5'/>")
    a(f"<path d='M{ux},{by_+bh} C{ux},{yb-60} {sx},{yb-60} {sx},{yb-12}' fill='none' stroke='{usb}' stroke-width='10' stroke-opacity='.9'/>")
    # Antennenkabel: von der IPEX-Buchse (unten) Schleife, unter dem Board zur SMA-Buchse oben
    a(f"<path d='M{ix},{iy} L{ix},{by_+bh} C{ix},{by_+bh+45} {ax},{by_+bh+45} {ax},{by_+bh}' fill='none' stroke='{ant}' stroke-width='3'/>")
    a(f"<line x1='{ax}' y1='{by_+bh}' x2='{ax}' y2='{by_}' stroke='{ant}' stroke-width='3' stroke-dasharray='8 6'/>")
    a(f"<path d='M{ax},{by_} C{ax},{Y+60} {sx},{Y+70} {sx},{Y+8}' fill='none' stroke='{ant}' stroke-width='3'/>")
    # SMA-Buchse in der Wand, außen die Bingfu-Antenne (flach, mit Gelenk und geriffelter Mutter)
    a(f"<path d='M{sx-10},{Y-4} h20 v12 h-20 Z' fill='#d8b14a' stroke='#8a6d1d'/>")
    a(f"<rect x='{sx-9}' y='{Y-22}' width='18' height='18' rx='2' fill='#222' class='th-ol'/>")
    for q in range(5):
        a(f"<line x1='{sx-6+q*3}' y1='{Y-22}' x2='{sx-6+q*3}' y2='{Y-4}' stroke='#444'/>")
    a(f"<circle cx='{sx}' cy='{Y-30}' r='9' fill='#262626' class='th-ol'/>")
    a(f"<path d='M{sx-8},{Y-36} L{sx-10},{Y-60} L{sx-10},{Y-88} Q{sx-10},{Y-98} {sx},{Y-98} Q{sx+10},{Y-98} {sx+10},{Y-88} L{sx+10},{Y-60} L{sx+8},{Y-36} Z' fill='#1f1f1f' class='th-ol'/>")
    a(f"<text class='th-fg' x='{sx+18}' y='{Y-50}' font-size='12'>Antenne (SMA)</text>")
    a(f"<text class='th-mu' x='{X+W/2-30}' y='{Y-50}' text-anchor='end' font-size='13'>Gegenüberliegende Seite</text>")
    # USB-C-Einbaubuchse mittig in der Anschlussseite: Mutter innen, Flansch außen
    a(f"<rect x='{sx-30}' y='{yb-14}' width='60' height='10' rx='2' fill='#2b2b2b' class='th-ol'/>")
    a(f"<rect x='{sx-36}' y='{yb+2}' width='72' height='8' rx='2' fill='#2b2b2b' class='th-ol'/><rect x='{sx-18}' y='{yb+10}' width='36' height='14' rx='3' fill='#3a3a3a' class='th-ol'/><rect x='{sx-10}' y='{yb+14}' width='20' height='6' rx='3' fill='#999'/>")
    a(f"<text class='th-fg' x='{sx}' y='{yb+40}' text-anchor='middle' font-size='12'>USB-C</text>")
    # Pigtails X (28 mm) und Y (72 mm) zu P25/P26 – nach der Drehung rechte Klemmenleiste
    for n, (lab, d, i) in enumerate([('X', 28, 9), ('Y', 72, 10)]):
        px = X + mm(d)
        tx, ty = rot(pos[('L', i)])
        lx = bx + bw + 14 + n * 10
        # Kabelverschraubung: Hutmutter außen, Sechskantmutter innen
        a(f"<path d='M{px-10},{yb+4} L{px-7},{yb+18} Q{px},{yb+24} {px+7},{yb+18} L{px+10},{yb+4} Z' fill='#cfd3d7' stroke='#6d7378'/>")
        a(f"<path d='M{px-11},{yb-12} h22 l-3,-6 h-16 Z' fill='#bfc4c8' stroke='#6d7378'/><rect x='{px-11}' y='{yb-12}' width='22' height='10' fill='#b3b8bd' stroke='#6d7378'/>")
        a(f"<line x1='{px}' y1='{yb+22}' x2='{px}' y2='{yb+56}' stroke='#b57452' stroke-width='6'/>")
        # BNC-Stecker (von oben): Knickschutz, Crimphülse, geriffelte Überwurfhülse, Steckerkopf
        a(f"<path d='M{px-6},{yb+56} L{px-8},{yb+70} L{px+8},{yb+70} L{px+6},{yb+56} Z' fill='#2a2a2a' class='th-ol'/>")
        a(f"<rect x='{px-8}' y='{yb+70}' width='16' height='14' rx='2' fill='#c3c7cb' stroke='#6d7378'/>")
        a(f"<rect x='{px-12}' y='{yb+84}' width='24' height='24' rx='3' fill='#d5d9dd' stroke='#6d7378' stroke-width='1.5'/>")
        for q in range(5):
            a(f"<line x1='{px-11}' y1='{yb+87+q*4}' x2='{px+11}' y2='{yb+87+q*4}' stroke='#a3a9ae'/>")
        a(f"<rect x='{px-8}' y='{yb+108}' width='16' height='10' fill='#c9cdd1' stroke='#6d7378'/>")
        a(f"<text class='th-fg' x='{px+18}' y='{yb+100}' font-size='13' font-weight='bold'>{lab}</text>")
        a(f"<path d='M{px},{yb-18} C{px},{yb-60} {lx},{yb-50} {lx},{yb-90} L{lx},{ty}' fill='none' stroke='#b57452' stroke-width='3'/>")
        a(f"<line x1='{lx}' y1='{ty}' x2='{tx}' y2='{ty}' stroke='#d62828' stroke-width='3'/>")
    a(f"<text x='{bx+bw+40}' y='{rot(pos[('L', 10)])[1]-6}' font-size='10' class='th-ac'>P26 (Y)</text>")
    a(f"<text x='{bx+bw+40}' y='{rot(pos[('L', 9)])[1]+12}' font-size='10' class='th-ac'>P25 (X)</text>")
    a(f"<text class='th-fg' x='{X+W/2}' y='{yb+140}' text-anchor='middle' font-size='12'>Pigtail X · USB-C · Pigtail Y (Kabelverschraubungen SKINDICHT MINI M6)</text>")
    a(f"<text x='{X+W/2}' y='{yb+160}' text-anchor='middle' font-size='13' class='th-mu'>Anschlussseite (kurze Seite)</text>")
    a(f"<text x='{X+W/2}' y='46' text-anchor='middle' font-size='11' class='th-mu'>Das OLED sitzt im Scharnierdeckel – Leitungen lang genug lassen</text>")
    a(f"<text x='{X-14}' y='{Y+H/2}' text-anchor='middle' font-size='12' class='th-ac' transform='rotate(-90 {X-14} {Y+H/2})'>150 mm</text>")
    a(f"<text x='{X+W/2}' y='{yb+180}' text-anchor='middle' font-size='12' class='th-ac'>100 mm</text>")
    # Hinweise unter der Zeichnung (nicht über Kabeln und Board)
    a(f"<text x='{X+W/2}' y='{yb+206}' text-anchor='middle' font-size='12' class='th-ac'>Breakout-Board samt DevKit um 180° gedreht</text>")
    a(f"<text x='{X+W/2}' y='{yb+224}' text-anchor='middle' font-size='12' class='th-mu'>gestrichelt: USB- und Antennenkabel unter dem Board</text>")
    save('aufbau.svg', X * 2 + W, yb + 236, '\n'.join(s), themed=True)


# ---------------------------------------------------------------- Software-Aufteilung
# Ohne Hintergrund, Farben über CSS-Klassen:
#  - docs/images/software(_en).svg für README/GitHub: helle Farben, dunkle per prefers-color-scheme
#  - in dokumentation.html / documentation.en.html direkt eingebettet (zwischen <!-- SVG software -->
#    und <!-- /SVG -->): Schrift und Farben (--fg, --acc, --mute) kommen von der Seite, so passt die
#    Grafik auf der Projektseite zu hellem und dunklem Design und zur Seitenschrift (Raleway)
SW_TEXT = {
    'de': dict(k1='Kern 1', t1='scopeTask', k0='Kern 0', t0='uiTask + wifiTask',
               l1=['nur Strahlausgabe, ohne Pause', 'DAC X/Y an GPIO25/26 (Register)', 'Rundtour: Zifferblatt + Ziffern',
                   'Zeiger nach der Uhrzeit', 'gleich schnelle Linien = gleich hell', 'Profil ANALOG/DIGITAL, Testbild',
                   'ca. 200 Bilder pro Sekunde'],
               l0=['WLAN (stärkstes bekanntes Netz)', 'Zeit per NTP, stündlich', 'Webserver, Setup-WLAN, mDNS',
                   'OLED-Anzeige (SPI)', 'BOOT-Taster, Einstellungen (NVS)', 'Protokoll (Ringpuffer 8 KB)', 'CPU-Last-Messung'],
               mid='atomare Variablen',
               foot='Übergabe nur über wenige atomare Variablen (Uhrzeit, Zifferblatt, Helligkeit, Profil) – keine Sperren im Zeichenpfad',
               aria='Aufteilung der Firmware auf die beiden Prozessorkerne'),
    'en': dict(k1='Core 1', t1='scopeTask', k0='Core 0', t0='uiTask + wifiTask',
               l1=['beam output only, without pause', 'DAC X/Y on GPIO25/26 (registers)', 'round trip: dial + numerals',
                   'hands according to the time', 'equally fast lines = equally bright', 'profile ANALOG/DIGITAL, test pattern',
                   'approx. 200 frames per second'],
               l0=['Wi-Fi (strongest known network)', 'time via NTP, hourly', 'web server, setup network, mDNS',
                   'OLED display (SPI)', 'BOOT button, settings (NVS)', 'log (ring buffer 8 KB)', 'CPU load measurement'],
               mid='atomic variables',
               foot='Hand-over only via a few atomic variables (time, dial, brightness, profile) – no locks in the drawing path',
               aria='Distribution of the firmware over the two processor cores'),
}


def software_body(t):
    s = []; a = s.append
    def box(x, title, task, lines):
        a(f"<rect class='sw-box' x='{x}' y='6' width='370' height='252' rx='8'/>")
        a(f"<text class='sw-h' x='{x+22}' y='40'>{title}</text>")
        a(f"<text class='sw-c' x='{x+22+len(title)*11.5+14}' y='40'>{task}</text>")
        a(f"<line class='sw-l' x1='{x+22}' y1='54' x2='{x+348}' y2='54'/>")
        for k, l in enumerate(lines):
            y = 82 + k * 25
            a(f"<circle class='sw-d' cx='{x+27}' cy='{y-5}' r='3'/>")
            a(f"<text class='sw-t' x='{x+40}' y='{y}'>{l}</text>")
    box(6, t['k1'], t['t1'], t['l1'])
    box(464, t['k0'], t['t0'], t['l0'])
    # Pfeil in beide Richtungen zwischen den Kernen
    a("<line class='sw-a' x1='386' y1='132' x2='454' y2='132'/>")
    a("<path class='sw-p' d='M384,132 l10,-6 v12 z M456,132 l-10,-6 v12 z'/>")
    a(f"<text class='sw-m' x='420' y='118' text-anchor='middle' font-size='11'>{t['mid'].split()[0]}</text>")
    a(f"<text class='sw-m' x='420' y='154' text-anchor='middle' font-size='11'>{t['mid'].split()[1]}</text>")
    a(f"<text class='sw-m' x='420' y='290' text-anchor='middle'>{t['foot']}</text>")
    return '\n'.join(s)


SW_W, SW_H = 840, 300
SW_FONT = "font-family='Raleway,Helvetica Neue,Helvetica,Arial,sans-serif'"
SW_CSS = ('.{c} text{{font-size:15px;font-weight:400}}.{c} .sw-h{{font-size:19px;font-weight:700;fill:{acc}}}'
          '.{c} .sw-c{{font-size:15px;font-family:Menlo,Consolas,monospace;fill:{mute}}}'
          '.{c} .sw-t{{fill:{fg}}}.{c} .sw-m{{fill:{mute};font-size:13px}}'
          '.{c} .sw-box{{fill:{acc};fill-opacity:.07;stroke:{acc};stroke-opacity:.6;stroke-width:1.5}}'
          '.{c} .sw-l{{stroke:{acc};stroke-opacity:.35;stroke-width:1}}.{c} .sw-d,.{c} .sw-p{{fill:{acc}}}'
          '.{c} .sw-a{{stroke:{acc};stroke-width:2}}')


def software():
    for lang, t in SW_TEXT.items():
        body = software_body(t)
        # Datei (README, GitHub): eigene Farben, dunkles Design über prefers-color-scheme
        css = (SW_CSS.format(c='sw', acc='#0a7d44', mute='#5f6368', fg='#111') +
               '@media (prefers-color-scheme:dark){' + SW_CSS.format(c='sw', acc='#30d158', mute='#a0a0a8', fg='#eee') + '}')
        svg = (f"<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 {SW_W} {SW_H}' class='sw' {SW_FONT}>\n"
               f"<style>{css}</style>\n{body}\n</svg>\n")
        name = 'software.svg' if lang == 'de' else 'software_en.svg'
        open(os.path.join(OUT, name), 'w', encoding='utf-8').write(svg)
        # eingebettet in die Doku: Farben der Seite (Projektseite: hell/dunkel), im PDF die Rückfallwerte
        c = 'swfig'
        css = SW_CSS.format(c=c, acc='var(--acc,#0a7d44)', mute='var(--mute,#5f6368)', fg='currentColor')
        inl = (f"<svg class='{c}' viewBox='0 0 {SW_W} {SW_H}' role='img' aria-label='{t['aria']}' style='width:96%;height:auto;overflow:visible'>"
               f"<style>{css}</style>{body}</svg>")
        doc = os.path.join(os.path.dirname(OUT), 'dokumentation.html' if lang == 'de' else 'documentation.en.html')
        h = open(doc, encoding='utf-8').read()
        a_, b_ = '<!-- SVG software -->', '<!-- /SVG -->'
        if a_ not in h:
            img = f'<img src="images/{name}" style="width:96%">'
            if img not in h:
                raise SystemExit(f'{doc}: Grafik software nicht gefunden')
            h = h.replace(img, a_ + b_)
        i, j = h.index(a_) + len(a_), h.index(b_)
        h = h[:i] + inl + h[j:]
        open(doc, 'w', encoding='utf-8').write(h)


if __name__ == '__main__':
    os.makedirs(OUT, exist_ok=True)
    verdrahtung(); bohrplan(); aufbau(); software()
    print('SVG-Dateien in', OUT)
