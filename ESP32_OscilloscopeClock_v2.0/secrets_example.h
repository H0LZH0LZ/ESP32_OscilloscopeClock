/******************************************************************************
  secrets_example.h – Vorlage für die WLAN-Zugangsdaten

  1. Diese Datei nach "secrets.h" kopieren (im selben Ordner).
  2. Beliebig viele Netze eintragen (eine Zeile pro Netz).
  secrets.h steht in .gitignore und wird nicht eingecheckt.

  Die Uhr sucht beim Start und nach jedem Verbindungsverlust alle Netze
  in Reichweite und verbindet sich mit dem stärksten bekannten Netz.
  Die Reihenfolge der Einträge spielt keine Rolle.

  Solange secrets.h fehlt (oder alle SSIDs leer sind), wird diese Vorlage
  verwendet. Die Uhr öffnet dann das Setup-WLAN "ScopeClock-Setup": Damit
  verbinden und unter http://192.168.4.1 das WLAN im Webinterface eintragen.
  secrets.h ist also optional – beide Wege lassen sich auch kombinieren.
******************************************************************************/
#pragma once
#define WIFI_NETWORKS_DEFINED

static const WifiCredential WIFI_NETWORKS[] = {
  // { "SSID",          "Passwort"   },
  {    "",              ""           },
  // { "Werkstatt",     "geheim2"    },
  // { "Handy-Hotspot", "geheim3"    },
};
