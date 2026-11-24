#!/usr/bin/env python3
"""
gen_firmware.py – legt die fertige Firmware und den Sketch für die Projektseite ab.

  docs/firmware/bootloader.bin     \
  docs/firmware/partitions.bin      |  Firmware in vier Teilen für die Installation im
  docs/firmware/boot_app0.bin       |  Browser (ESP Web Tools, Knopf auf der Projektseite)
  docs/firmware/firmware.bin       /
  docs/firmware/manifest.json      Beschreibung für ESP Web Tools (Teile und Adressen)
  docs/firmware/ESP32_OscilloscopeClock_v2.0.zip
                                   Sketch-Ordner als ZIP, z. B. zum Import in den
                                   Arduino Cloud Editor (app.arduino.cc)

Die Teile werden einzeln geschrieben – NICHT das "merged"-Image: das ist 4 MB groß und
würde den NVS-Bereich (gespeicherte Einstellungen und WLANs) mit überschreiben. So
bleiben die Einstellungen bei einem Update erhalten; nur bei einer Neuinstallation
bietet ESP Web Tools an, den Flash vorher zu löschen.

Aufruf (nach dem Kompilieren mit Standard-Partitionsschema, Board "ESP32 Dev Module"):
    arduino-cli compile --fqbn esp32:esp32:esp32 --build-path BUILD ESP32_OscilloscopeClock_v2.0
    python3 tools/gen_firmware.py BUILD
"""
import json
import os
import shutil
import sys
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SKETCH = 'ESP32_OscilloscopeClock_v2.0'
OUT = os.path.join(ROOT, 'docs', 'firmware')

# Teil: (Datei im Build-Ordner, Name auf der Webseite, Adresse im Flash)
PARTS = [
    (SKETCH + '.ino.bootloader.bin', 'bootloader.bin', 0x1000),
    (SKETCH + '.ino.partitions.bin', 'partitions.bin', 0x8000),
    ('boot_app0.bin',                'boot_app0.bin',  0xE000),
    (SKETCH + '.ino.bin',            'firmware.bin',   0x10000),
]


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    build = sys.argv[1]
    os.makedirs(OUT, exist_ok=True)
    parts = []
    for src, name, offset in PARTS:
        shutil.copyfile(os.path.join(build, src), os.path.join(OUT, name))
        parts.append({'path': name, 'offset': offset})

    # Standard-Partitionsschema: App-Bereich 0x140000 (1,25 MB) – Firmware muss hineinpassen
    size = os.path.getsize(os.path.join(OUT, 'firmware.bin'))
    if size > 0x140000:
        sys.exit(f'firmware.bin ist {size} Byte groß – passt nicht ins Standard-Partitionsschema')

    manifest = {
        'name': 'ESP32 Oscilloscope Clock',
        'version': '2.0',
        'new_install_prompt_erase': True,
        'builds': [{'chipFamily': 'ESP32', 'parts': parts}],
    }
    with open(os.path.join(OUT, 'manifest.json'), 'w') as f:
        json.dump(manifest, f, indent=2)
        f.write('\n')

    # Sketch-ZIP mit festem Zeitstempel: gleiche Quellen ergeben die gleiche Datei
    zpath = os.path.join(OUT, SKETCH + '.zip')
    src_dir = os.path.join(ROOT, SKETCH)
    with zipfile.ZipFile(zpath, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for name in sorted(os.listdir(src_dir)):
            if name.endswith(('.ino', '.h')):
                info = zipfile.ZipInfo(f'{SKETCH}/{name}', date_time=(2026, 1, 1, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                info.external_attr = 0o644 << 16
                with open(os.path.join(src_dir, name), 'rb') as f:
                    z.writestr(info, f.read())

    print(f'docs/firmware: firmware.bin {size} Byte, {os.path.getsize(zpath)} Byte Sketch-ZIP')


if __name__ == '__main__':
    main()
