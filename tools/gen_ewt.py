#!/usr/bin/env python3
"""
gen_ewt.py – legt ESP Web Tools (Knopf „Firmware installieren“ auf der Projektseite) mit
deutschen Texten unter docs/ewt/ ab.

ESP Web Tools (https://github.com/esphome/esp-web-tools, Apache-2.0) kennt nur Englisch.
Dieses Skript lädt das Paket in einer festen Version von npm, kopiert dist/web/ nach docs/ewt/
und ersetzt in den Dialogen (Installation, „No port selected“) die englischen Texte durch
ewtT("Deutsch", "English"). ewtT() wählt beim Anzeigen nach <html lang>: auf der
Projektseite also Deutsch oder Englisch je nach gewählter Sprache.

Jede Ersetzung muss genau passen – ändert sich der Text in einer neuen Version, bricht das
Skript mit einer Meldung ab, statt halb übersetzte Dateien zu schreiben.

Aufruf:  python3 tools/gen_ewt.py            (lädt von registry.npmjs.org)
         python3 tools/gen_ewt.py PAKET.tgz   (vorhandenes Paket verwenden)
"""
import io
import os
import re
import shutil
import sys
import tarfile
import urllib.request

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VERSION = '10.4.0'
URL = f'https://registry.npmjs.org/esp-web-tools/-/esp-web-tools-{VERSION}.tgz'
OUT = os.path.join(ROOT, 'docs', 'ewt')
HELPER = 'const ewtT=(d,e)=>document.documentElement.lang==="en"?e:d;'


def q(t):
    return '"' + t.replace('\\', '\\\\').replace('"', '\\"') + '"'


def T(de, en):                     # Ausdruck für Template-Text: ${ewtT("…","…")}
    return '${ewtT(' + q(de) + ',' + q(en) + ')}'


# Textknoten in lit-Templates: englischer Text (Zeilenumbrüche egal) → deutscher Text
DIALOG_TEXT = [
    ('Close', 'Schließen'),
    ('Visit Device', 'Gerät öffnen'),
    ('Add to Home Assistant', 'Zu Home Assistant hinzufügen'),
    ('Logs & Console', 'Protokoll & Konsole'),
    ('Fund Development', 'Entwicklung unterstützen'),
    ('Erase User Data', 'Benutzerdaten löschen'),
    ('Skip', 'Überspringen'),
    ('Back', 'Zurück'),
    ('Next', 'Weiter'),
    ('Install', 'Installieren'),
    ('Connect', 'Verbinden'),
    ('Continue', 'Weiter'),
    ('Reset Device', 'Gerät neu starten'),
    ('Download Logs', 'Protokoll herunterladen'),
    ('Join other…', 'Anderes Netz …'),
    ('Erase device', 'Gerät löschen'),
    ('Connect your device to the network to start using it.',
     'Gerät mit dem WLAN verbinden, um es zu benutzen.'),
    ('Do you want to erase the device before installing ${this._manifest.name}? All data on the device will be lost.',
     None),   # Sonderfall unten (enthält ${…})
    ('Do you want to reset your device and erase all user data from your device?',
     'Gerät zurücksetzen und alle Benutzerdaten löschen?'),
    ('Keep this page visible to prevent slow down',
     'Diese Seite dabei sichtbar lassen, sonst wird es langsamer.'),
    ('This will take', 'Das dauert etwa'),
]

# Stringliterale im Dialog-Code (Überschriften, Fortschritt, Fehlermeldungen)
DIALOG_STR = [
    ('Error', 'Fehler'),
    ('Disconnected', 'Verbindung getrennt'),
    ('Connecting', 'Verbinden …'),
    ('Installing', 'Installation läuft'),
    ('Preparing installation', 'Installation wird vorbereitet'),
    ('Erasing', 'Speicher wird gelöscht'),
    ('Wrapping up', 'Wird abgeschlossen'),
    ('Confirm Installation', 'Installation bestätigen'),
    ('Erase User Data', 'Benutzerdaten löschen'),
    ('Erase device', 'Gerät löschen'),
    ('Installation failed', 'Installation fehlgeschlagen'),
    ('Configure Wi-Fi', 'WLAN einrichten'),
    ('Change Wi-Fi', 'WLAN ändern'),
    ('Connect to Wi-Fi', 'Mit WLAN verbinden'),
    ('Trying to connect', 'Verbindung wird hergestellt'),
    ('Scanning for networks', 'Netze werden gesucht'),
    ('Unable to connect', 'Verbindung nicht möglich'),
    ('Timeout', 'Zeitüberschreitung'),
    ('Skip', 'Überspringen'),
    ('Back', 'Zurück'),
    ('a minute', 'eine Minute'),
    ('2 minutes', '2 Minuten'),
    ('Failed to download manifest', 'Firmware-Beschreibung (manifest.json) konnte nicht geladen werden'),
    ('Serial port is not readable/writable. Close any other application using it and try again.',
     'Die serielle Schnittstelle ist belegt. Programme schließen, die sie nutzen (z. B. seriellen Monitor), und erneut versuchen.'),
    ('Serial port is not ready. Close any other application using it and try again.',
     'Die serielle Schnittstelle ist nicht bereit. Programme schließen, die sie nutzen (z. B. seriellen Monitor), und erneut versuchen.'),
    ('Failed to initialize. Try resetting your device or holding the BOOT button while clicking INSTALL.',
     'Verbindung zum ESP32 fehlgeschlagen. ESP32 neu starten oder beim Klick auf „Installieren“ den BOOT-Taster gedrückt halten.'),
]

# Feste Ersetzungen (Stellen mit ${…} oder Attributen)
DIALOG_RAW = [
    ('`Update ${this._manifest.name}`', 'ewtT(`${this._manifest.name} aktualisieren`,`Update ${this._manifest.name}`)'),
    ('`Install ${this._manifest.name}`', 'ewtT(`${this._manifest.name} installieren`,`Install ${this._manifest.name}`)'),
    ('Connected to ${this._info.name}', T('Verbunden mit', 'Connected to') + ' ${this._info.name}'),
    ('s`Your device is running', 's`${ewtT("Auf dem Gerät läuft","Your device is running")}'),
    ('All data on the device will be erased.', T('Alle Daten auf dem Gerät werden gelöscht.', 'All data on the device will be erased.')),
    ('label="Installation complete!"', '.label=${ewtT("Installation abgeschlossen!","Installation complete!")}'),
    ('label="Device connected to the network!"', '.label=${ewtT("Gerät ist mit dem Netz verbunden!","Device connected to the network!")}'),
    ('label="Network Name"', '.label=${ewtT("Netzwerkname (SSID)","Network Name")}'),
    ('label="Network"', '.label=${ewtT("Netz","Network")}'),
    ('label="Password"', '.label=${ewtT("Passwort","Password")}'),
    ('.label=${this._installState.message}',
     '.label=${ewtT(this._installState.message+" – Fenster schließen, USB-Kabel kurz abziehen (oder EN-Taster drücken) und erneut „Firmware installieren“ klicken; dabei den BOOT-Taster gedrückt halten, Kabel zum Oszilloskop vorher abziehen.",'
     'this._installState.message+" – close this window, unplug the USB cable briefly (or press the EN button) and click “Install firmware” again while holding the BOOT button; unplug the cables to the oscilloscope first.")}'),
]

PORT_TEXT = [
    ('No port selected', 'Kein Anschluss gewählt'),
    ("If you didn't select a port because you didn't see your device listed, try the following steps:",
     'Wurde kein Anschluss gewählt, weil der ESP32 nicht in der Liste stand, hilft meist eines davon:'),
    ('Make sure that the device is connected to this computer (the one that runs the browser that shows this website)',
     'Der ESP32 ist mit diesem Computer verbunden (dem, auf dem dieser Browser läuft).'),
    ('Most devices have a tiny light when it is powered on. If yours has one, make sure it is on.',
     'Die meisten Boards haben eine kleine Leuchtdiode, die bei Stromversorgung leuchtet – sie muss an sein.'),
    ('Make sure that the USB cable you use can be used for data and is not a power-only cable.',
     'Das USB-Kabel überträgt Daten und ist kein reines Ladekabel.'),
    ('If you are using a Linux flavor, make sure that your user is part of the',
     'Unter Linux muss der eigene Benutzer zur Gruppe'),
    ('group so it has permission to access the device.',
     'gehören, damit er auf den Anschluss zugreifen darf.'),
    ('You may need to log out & back in or reboot to activate the new group access.',
     'Danach ab- und wieder anmelden oder neu starten.'),
    ('Make sure you have the right drivers installed. Below are the drivers for common chips used in ESP devices:',
     'Der passende USB-Treiber ist installiert. Treiber für die üblichen USB-Chips auf ESP32-Boards:'),
    ('CP2102 drivers:', 'CP2102-Treiber:'),
    ('CH342, CH343, CH9102 drivers:', 'CH342-, CH343-, CH9102-Treiber:'),
    ('CH340, CH341 drivers:', 'CH340-, CH341-Treiber:'),
    ('Cancel', 'Abbrechen'),
    ('Try Again', 'Erneut versuchen'),
    ('Close', 'Schließen'),
]
PORT_RAW = [
    ('(download via blue button with ${s} icon)', T('(Download über den blauen Knopf mit dem Symbol', '(download via blue button with') + ' ${s} ' + T(')', 'icon)')),
]


def text_pattern(en):
    """Regex für einen Textknoten: zwischen > und <, Leerraum beliebig."""
    parts = [re.escape(w) for w in en.split()]
    return re.compile(r'(>\s*)' + r'\s+'.join(parts) + r'(\s*<)')


def phrase_pattern(en):
    parts = [re.escape(w) for w in en.split()]
    return re.compile(r'\s+'.join(parts))


def patch(seg, texts, strs, raws, name):
    # Reihenfolge: erst Stringliterale, dann feste Stellen, dann Textknoten – so trifft keine
    # Ersetzung den englischen Text, den eine frühere schon in ewtT(…) gesetzt hat
    for en, de in strs:
        lit = q(en)
        if lit not in seg:
            sys.exit(f'{name}: String nicht gefunden: {lit}')
        seg = seg.replace(lit, 'ewtT(' + q(de) + ',' + lit + ')')
    for en, de in raws:
        n = seg.count(en)
        if not n:
            sys.exit(f'{name}: Stelle nicht gefunden: {en!r}')
        seg = seg.replace(en, de)
    for en, de in texts:
        if de is None:
            continue
        pat = text_pattern(en)
        seg, n = pat.subn(lambda m: m.group(1) + T(de, ' '.join(en.split())) + m.group(2), seg)
        if not n:
            # Text steht nicht direkt zwischen > und < (z. B. vor <code> oder hinter </code>)
            pat = phrase_pattern(en)
            seg, n = pat.subn(lambda m: T(de, ' '.join(en.split())), seg)
        if not n:
            sys.exit(f'{name}: Text nicht gefunden: {en!r}')
    return seg


def main():
    if len(sys.argv) > 1:
        data = open(sys.argv[1], 'rb').read()
    else:
        with urllib.request.urlopen(URL) as r:
            data = r.read()
    tar = tarfile.open(fileobj=io.BytesIO(data), mode='r:gz')
    files = {}
    for m in tar.getmembers():
        if m.isfile() and m.name.startswith('package/dist/web/') and m.name.endswith('.js'):
            files[m.name.rsplit('/', 1)[1]] = tar.extractfile(m).read().decode('utf-8')
    lic = tar.extractfile('package/LICENSE').read().decode('utf-8') if 'package/LICENSE' in tar.getnames() else ''

    dlg = [n for n in files if n.startswith('install-dialog-')]
    idx = [n for n in files if n.startswith('index-') and 'No port selected' in files[n]]
    if len(dlg) != 1 or len(idx) != 1:
        sys.exit('Dateien des Installationsdialogs nicht eindeutig gefunden')

    # Installationsdialog: nur die Dialog-Klasse ändern, nicht den Flash-Code davor
    s = files[dlg[0]]
    a = s.index('console.log("ESP Web Tools')
    seg = s[a:]
    erase = re.compile(r'Do you want to erase the device before installing\s+\$\{this\._manifest\.name\}\? All data on the device will be lost\.')
    seg, n = erase.subn('${ewtT(s`Soll das Gerät vor der Installation von ${this._manifest.name} gelöscht werden? Alle Daten darauf gehen verloren.`,'
                        's`Do you want to erase the device before installing ${this._manifest.name}? All data on the device will be lost.`)}', seg)
    if n != 1:
        sys.exit('Installationsdialog: Lösch-Frage nicht gefunden')
    ask = re.compile(r'Do you want to \$\{r\}\s+\$\{this\._manifest\.name\}&nbsp;\$\{this\._manifest\.version\}\?')
    seg, n = ask.subn('${ewtT(s`Soll ${this._manifest.name}&nbsp;${this._manifest.version} ${i?"aktualisiert":"installiert"} werden?`,'
                      's`Do you want to ${r} ${this._manifest.name}&nbsp;${this._manifest.version}?`)}', seg)
    if n != 1:
        sys.exit('Installationsdialog: Installations-Frage nicht gefunden')
    # Nach einem Fehler hängt der ESP32 meist im Lademodus: „Back“ (zurück zur Übersicht) hilft dann
    # nicht – stattdessen „Schließen“; neu starten und „Firmware installieren“ erklärt die Meldung
    i = seg.index('e="Installation failed"')
    old = '@click=${async()=>{this._initialize(),this._state="DASHBOARD"}}\n          >\n            Back\n'
    if seg.find(old, i, i + 600) < 0:
        sys.exit('Installationsdialog: Knopf nach Fehler nicht gefunden')
    seg = seg[:i] + seg[i:].replace(old, '@click=${this._closeDialog}\n          >\n            Close\n', 1)
    seg = patch(seg, DIALOG_TEXT, DIALOG_STR, DIALOG_RAW, 'Installationsdialog')
    seg = seg.replace('e=`Logs`', 'e=ewtT("Protokoll","Logs")')
    # Konsole (ewt-console, vor der Dialog-Klasse): Meldung beim Trennen
    head = s[:a]
    for en, de in (('"Terminal disconnected"', 'ewtT("Verbindung getrennt","Terminal disconnected")'),
                   ('`Terminal disconnected: ${e}`', 'ewtT(`Verbindung getrennt: ${e}`,`Terminal disconnected: ${e}`)')):
        if head.count(en) != 1:
            sys.exit(f'Konsole: Stelle nicht gefunden: {en}')
        head = head.replace(en, de)
    files[dlg[0]] = HELPER + head + seg
    # Einstieg für die serielle Konsole der Projektseite (Kapitel 11.3): lädt den Dialog-Code,
    # der das Element <ewt-console> definiert – der Dateiname mit Prüfsumme ändert sich je Version
    files['console.js'] = f'import "./{dlg[0]}";\n'

    # Hinweis „No port selected“
    s = files[idx[0]]
    a = s.index('No port selected')
    a = s.rfind('return', 0, a)
    files[idx[0]] = HELPER + s[:a] + patch(s[a:], PORT_TEXT, [], PORT_RAW, 'No-port-Dialog')

    if os.path.isdir(OUT):
        shutil.rmtree(OUT)
    os.makedirs(OUT)
    for n, c in files.items():
        with open(os.path.join(OUT, n), 'w', encoding='utf-8') as f:
            f.write(c)
    with open(os.path.join(OUT, 'LICENSE'), 'w', encoding='utf-8') as f:
        f.write(f'ESP Web Tools {VERSION} – https://github.com/esphome/esp-web-tools\n'
                'Texte der Dialoge ins Deutsche übersetzt (tools/gen_ewt.py), sonst unverändert.\n\n' + lic)
    print(f'docs/ewt: ESP Web Tools {VERSION}, {len(files)} Dateien, Dialoge Deutsch/Englisch')


if __name__ == '__main__':
    main()
