/******************************************************************************
  secrets_example.h – Vorlage für die WLAN-Zugangsdaten

  1. Diese Datei nach "secrets.h" kopieren (im selben Ordner).
  2. SSID und Passwort eintragen.
  secrets.h steht in .gitignore und wird nicht eingecheckt.

  Solange secrets.h fehlt, wird diese Vorlage verwendet: die Uhr läuft dann
  ohne WLAN ab der Startzeit aus config.h.
******************************************************************************/
#pragma once

#define WIFI_SSID     ""   // z.B. "MeinWLAN"
#define WIFI_PASSWORD ""
