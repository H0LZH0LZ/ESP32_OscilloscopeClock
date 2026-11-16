/******************************************************************************
  webpage.h – Webinterface der ESP32 Oscilloscope Clock (eine einzige Seite)

  Die Seite liegt als Text im Flash (PROGMEM) und wird unter "/" ausgeliefert.
  Sie lädt alle Daten per JavaScript von den JSON-Schnittstellen der Uhr
  (siehe Abschnitt "Webinterface" in der .ino und SOFTWARE.md):

    GET  /api/status        Uhrzeit, NTP, WLAN, Setup-WLAN, Profil, Bildrate …
    GET  /api/log           Log (Text, älteste Zeile zuerst)
    GET  /api/networks      gespeicherte WLANs (nur Namen)
    GET  /api/vectors       Vektordaten für die Oszilloskop-Vorschau (einmal)
    GET  /api/scan          WLANs in Reichweite (Suche läuft im Hintergrund)
    POST /api/wifi          WLAN speichern        (ssid, pw)
    POST /api/wifi/delete   WLAN löschen          (ssid) – Uhr startet neu
    POST /api/reconnect     WLAN neu verbinden
    POST /api/dial          Zifferblatt umschalten (style 0–3) – auch am Oszilloskop
    GET  /api/settings      Einstellungen, Standardwerte, Zeitzonen
    POST /api/settings      Einstellungen ändern (beliebige Felder, sofort wirksam)
    POST /api/settings/reset  alle Einstellungen auf die Werte aus config.h

  Für die Mobilbedienung optimiert:
   - eine Spalte, max. 640 px breit, Abstände für Daumenbedienung
   - einklappbares Seitenmenü (☰ oben links) zum Springen in die Abschnitte
   - Uhr-Abschnitt zentriert; Abschnittsüberschriften bleiben beim Scrollen stehen
   - Einstellungen (Profil, Zifferblatt, Strahl, Zeitzone, NTP, OLED) wirken sofort
   - Tippen auf die Vorschau schaltet das Zifferblatt um (Webseite + Oszilloskop):
     1–12 → 12·3·6·9 → römisch I–XII → römisch XII·III·VI·IX
   - Schaltflächen und Listeneinträge mindestens 44 px hoch
   - Eingabefelder mit 16 px Schrift (iOS zoomt sonst beim Antippen)
   - helles/dunkles Design nach Systemeinstellung, Rand für Notch/Home-Balken
   - keine externen Dateien: funktioniert auch im Setup-WLAN ohne Internet
   - alle Texte von der Uhr werden als Text (textContent) eingesetzt, nie als
     HTML – WLAN-Namen mit Sonderzeichen können die Seite nicht verändern
******************************************************************************/
#pragma once

static const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="de"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="color-scheme" content="light dark">
<title>Scope Clock</title>
<style>
/* Schrift aus der Firmware (Raleway, verkleinert – siehe tools/gen_font.py): zwei feste Schnitte,
   normal (400) und halbfett (600). swap: bis sie geladen ist, wird die Systemschrift gezeigt. */
@font-face{font-family:'ScopeClock Sans';src:url(/font.woff2) format('woff2');font-weight:400;font-display:swap}
@font-face{font-family:'ScopeClock Sans';src:url(/font-600.woff2) format('woff2');font-weight:600;font-display:swap}
:root{--scope-bg:transparent;--scope-trace:#0b7a3e;--scope-glow:rgba(11,122,62,.12);--scope-halo:rgba(11,122,62,.35);--scope-grid:rgba(0,0,0,.07);
--bg:#f2f2f5;--card:#fff;--fg:#111;--mute:#6b6b70;--line:#e3e3e8;--acc:#0a7d44;--warn:#b45309;--err:#b91c1c;--mono:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace;
 --sans:'ScopeClock Sans',system-ui,-apple-system,Segoe UI,Roboto,sans-serif}
@media (prefers-color-scheme:dark){:root{--scope-bg:#050805;--scope-trace:#b6ffc4;--scope-glow:rgba(80,255,120,.18);--scope-halo:#3dff6a;--scope-grid:rgba(120,255,140,.08);--bg:#000;--card:#1c1c1e;--fg:#f2f2f7;--mute:#9a9aa0;--line:#2c2c2e;--acc:#30d158;--warn:#ffb340;--err:#ff6961}}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--fg);font:16px/1.4 var(--sans);
 font-feature-settings:'lnum' 1;   /* Raleway: Ziffern in Versalhöhe statt Mediävalziffern (vererbt sich überallhin) */
 padding:env(safe-area-inset-top) env(safe-area-inset-right) env(safe-area-inset-bottom) env(safe-area-inset-left);-webkit-text-size-adjust:100%}
main{max-width:640px;margin:0 auto;padding:0 16px 32px}
/* Titelleiste: bleibt beim Scrollen immer oben stehen (64 px, auf Höhe der
   Menütaste). Links und rechts gleich viel Abstand, damit die Mitte stimmt. */
h1{position:sticky;top:env(safe-area-inset-top);z-index:22;display:flex;align-items:center;justify-content:center;
 min-height:64px;margin:0;padding:0 46px;background:var(--bg);text-align:center;
 font-size:15px;font-weight:600;color:var(--mute);text-transform:uppercase;letter-spacing:.06em}
/* Oberster Abschnitt (Uhr): alles zentriert, die Oszilloskop-Vorschau nutzt die volle Breite. */
header{padding:0 0 4px;text-align:center;scroll-margin-top:calc(env(safe-area-inset-top) + 64px)}
#clock{font:600 44px/1.1 var(--sans);font-feature-settings:'lnum' 1;margin:4px 0 0}   /* 'font' setzt lnum zurück */
/* Raleway hat unterschiedlich breite Ziffern – jede Ziffer bekommt ein festes Feld,
   damit die Uhrzeit beim Weiterzählen nicht hin und her springt. */
#clock i{display:inline-block;width:.62em;font-style:normal;text-align:center}
#clock .ap{font-size:.4em;margin-left:.25em;letter-spacing:.04em}
#date{color:var(--mute)}
.scope canvas{margin:0 auto}
section{background:var(--card);border-radius:14px;margin:0 0 14px;padding:6px 16px}   /* oben/unten 6 px + Innenabstand der Zeilen ≈ 16 px wie links/rechts */
/* Abschnittsüberschriften bleiben beim Scrollen direkt unter der Titelleiste
   stehen. Die nächste Überschrift schiebt die vorige weg; so ist immer zu sehen,
   in welchem Abschnitt man ist. scroll-margin: Sprungziele aus dem Menü landen
   unter der Titelleiste statt dahinter. */
h2{position:sticky;top:calc(env(safe-area-inset-top) + 64px);scroll-margin-top:calc(env(safe-area-inset-top) + 64px);
 z-index:20;display:flex;align-items:center;min-height:44px;margin:0;
 padding:0 4px;background:var(--bg);font-size:13px;font-weight:600;color:var(--mute);text-transform:uppercase;letter-spacing:.06em}
/* Streifen unter Notch/Statusleiste deckt durchscrollenden Inhalt ab */
body::before{content:"";position:fixed;inset:0 0 auto;height:env(safe-area-inset-top);background:var(--bg);z-index:25}
.row{display:flex;justify-content:space-between;align-items:center;gap:12px;min-height:44px;border-top:1px solid var(--line);padding:6px 0}
.row:first-of-type{border-top:0}
.row span:first-child{color:var(--mute)}
.row span:last-child{text-align:right;overflow-wrap:anywhere}
.row .note{display:block;font-size:12px;line-height:1.3;color:var(--mute);opacity:.85;margin-top:2px}
/* Farbig hervorgehoben werden nur Probleme: Orange = Warnung, Rot = Fehler.
   Normale Zustände und Bestätigungen (.ok) stehen in normaler Schriftfarbe. */
.ok{color:inherit}.warn{color:var(--warn)}.err{color:var(--err)}
button{font:inherit;min-height:44px;border:0;border-radius:10px;padding:0 16px;background:var(--acc);color:#fff;font-weight:600;width:100%;margin:6px 0}
button.sec{background:transparent;color:var(--acc);border:1px solid var(--line)}
button.del{width:auto;min-height:36px;padding:0 12px;background:transparent;color:var(--err);border:1px solid var(--line);margin:0}
button:disabled{opacity:.5}
label{display:block;color:var(--mute);font-size:14px;margin:10px 0 4px}
.subhead{color:var(--fg);font-size:16px;margin:10px 0 2px}   /* Zwischenüberschrift wie die Einstellungen */
.desc{color:var(--mute);font-size:13px;margin:0 0 6px}.desc.top{margin-top:10px}   /* kurze Funktionsbeschreibung */
input{font:inherit;font-size:16px;width:100%;min-height:44px;padding:0 12px;border:1px solid var(--line);border-radius:10px;background:var(--bg);color:var(--fg)}
.pw{display:flex;gap:8px}.pw button{width:auto;margin:0;white-space:nowrap}
ul{list-style:none;margin:0;padding:0}
li{display:flex;justify-content:space-between;align-items:center;gap:12px;min-height:44px;border-top:1px solid var(--line)}
li:first-child{border-top:0}
li.tap{cursor:pointer}
.bars{font-family:var(--mono);color:var(--mute);white-space:nowrap}
.hint{color:var(--mute);font-size:14px;margin:6px 0 12px}
#msg{font-size:14px;margin:4px 0}
#tMsg{font-size:14px;margin:2px 0 6px}
#msg:empty,#setMsg:empty,#tMsg:empty{display:none}   /* kein Leerraum, solange keine Meldung da ist */
/* Log-Feld: feste Höhe, eigener Scrollbereich. Wischen im Feld scrollt nur das Log
   (overscroll-behavior), die Scrollleiste bleibt sichtbar. "Alles anzeigen" hebt die
   Höhenbegrenzung auf. */
/* margin-top 10px + 6px Abschnitts-Innenabstand = 16px, gleich wie links/rechts */
pre{font:12px/1.45 var(--mono);white-space:pre-wrap;overflow-wrap:anywhere;height:min(60vh,520px);overflow-y:auto;margin:10px 0 6px;
 background:var(--bg);border:1px solid var(--line);border-radius:10px;padding:10px;-webkit-overflow-scrolling:touch;
 overscroll-behavior:contain;scrollbar-width:thin;scrollbar-color:var(--mute) transparent}
pre::-webkit-scrollbar{width:6px}pre::-webkit-scrollbar-thumb{background:var(--mute);border-radius:3px}
pre.full{height:auto;overflow-y:visible}
.loginfo{color:var(--mute);font-size:13px;margin:0 0 8px;display:flex;justify-content:space-between;gap:8px}
.tools{display:flex;flex-wrap:wrap;gap:8px}.tools button{margin:0 0 10px;flex:1 1 40%}
.logsw{min-height:44px;border-top:1px solid var(--line);margin-bottom:8px}
/* Einstellungen */
.set{border-top:1px solid var(--line);padding:10px 0}
.set:first-of-type,#timeRow[hidden]+.set{border-top:0}   /* keine Linie über dem ersten sichtbaren Eintrag */
.set .head{display:flex;justify-content:space-between;align-items:baseline;gap:12px}
.set .head label{margin:0;color:var(--fg);font-size:16px}
.set .val{color:var(--mute);font-variant-numeric:tabular-nums;white-space:nowrap}
.set .sub{color:var(--mute);font-size:13px;margin-top:2px}
.set input[type=range]{width:100%;min-height:44px;padding:0;border:0;background:transparent;accent-color:var(--acc)}
.set select{font:inherit;font-size:16px;width:100%;min-height:44px;padding:0 10px;margin-top:6px;border:1px solid var(--line);border-radius:10px;background:var(--bg);color:var(--fg)}
.seg{display:flex;gap:6px;margin-top:6px}
.seg button{margin:0;background:transparent;color:var(--fg);border:1px solid var(--line);font-weight:400}
.seg button[aria-pressed="true"]{background:var(--acc);color:#fff;border-color:var(--acc);font-weight:600}
.dt{display:flex;gap:8px;margin-top:6px}
#tDst{margin-bottom:2px}#tDst button{padding:0 4px;font-size:15px}.dt input{flex:1 1 0;min-width:0}
.sw{display:flex;justify-content:space-between;align-items:center;gap:12px;margin:0;color:var(--fg);font-size:16px}
.sw input{width:24px;height:24px;min-height:0;margin:0;flex:none;accent-color:var(--acc)}
.set.off{opacity:.55}
#setMsg{font-size:14px;margin:6px 0}
/* Oszilloskop-Schirm: im hellen Design durchsichtig mit dunkelgrünen Linien,
   im dunklen Design schwarzer Schirm mit leuchtend grünen Linien (Farben --scope-*). */
.scope{background:var(--scope-bg);border-radius:14px;margin:12px 0 4px;padding:10px;display:flex;justify-content:center;forced-color-adjust:none}
.scope{position:relative}
.scope canvas:focus:not(:focus-visible){outline:none}
.scope canvas{width:100%;max-width:340px;aspect-ratio:1;display:block;cursor:grab;touch-action:pan-y;user-select:none;-webkit-user-select:none;-webkit-tap-highlight-color:transparent}
.dialtag{position:absolute;left:50%;bottom:14px;transform:translateX(-50%);background:rgba(0,0,0,.65);color:#b6ffc4;
 font-size:13px;padding:4px 10px;border-radius:999px;pointer-events:none;transition:opacity .4s;opacity:0;white-space:nowrap}
.dialtag.show{opacity:1}
.scopehint{color:var(--mute);font-size:13px;margin:0 0 6px}
html{scroll-behavior:smooth}
/* Jede Überschrift ist nur innerhalb ihres Abschnitts (.grp) angeheftet:
   endet der Abschnitt, schiebt die nächste Überschrift die vorige hinaus. */
.grp{position:relative}
.grp:last-child{min-height:100vh}   /* auch der letzte Abschnitt (Log) lässt sich ganz nach oben scrollen */
#menuBtn{position:fixed;top:calc(env(safe-area-inset-top) + 10px);left:calc(env(safe-area-inset-left) + 10px);z-index:30;
 width:44px;height:44px;min-height:44px;padding:0;margin:0;border-radius:12px;background:var(--card);color:var(--fg);
 border:1px solid var(--line);font-size:22px;line-height:1;box-shadow:0 2px 8px rgba(0,0,0,.15)}
#nav{position:fixed;top:0;bottom:0;left:0;z-index:40;width:min(78vw,280px);background:var(--card);
 padding:calc(env(safe-area-inset-top) + 16px) 12px 16px calc(env(safe-area-inset-left) + 12px);
 transform:translateX(-105%);transition:transform .25s ease;box-shadow:2px 0 16px rgba(0,0,0,.25);overflow-y:auto}
#nav.open{transform:none}
#nav h3{font-size:13px;color:var(--mute);text-transform:uppercase;letter-spacing:.06em;margin:4px 8px 12px}
#nav a{display:flex;align-items:center;min-height:48px;padding:0 12px;border-radius:10px;color:var(--fg);text-decoration:none;font-weight:500}
#nav a:active,#nav a:hover{background:var(--bg)}
#nav .navlang{margin:16px 8px 0}
#shade{position:fixed;inset:0;z-index:35;background:rgba(0,0,0,.35);opacity:0;pointer-events:none;transition:opacity .25s}
#shade.open{opacity:1;pointer-events:auto}

</style></head>
<body>
<button id="menuBtn" aria-label="Menü" aria-controls="nav" aria-expanded="false">☰</button>
<nav id="nav" aria-label="Abschnitte" aria-hidden="true">
 <h3>Abschnitte</h3>
 <a href="#uhr">Uhr</a>
 <a href="#status-h">Status</a>
 <a href="#set-h">Einstellungen</a>
 <a href="#wlan-h">WLAN</a>
 <a href="#log-h">Protokoll</a>
 <div class="seg navlang" role="group" aria-label="Sprache">
  <button type="button" data-lang="de" aria-pressed="true">Deutsch</button>
  <button type="button" data-lang="en" aria-pressed="false">English</button></div>
</nav>
<div id="shade"></div>
<main>
<h1>ESP32 Oscilloscope Clock</h1>
<header id="uhr">
<div class="scope"><canvas id="scope" width="512" height="512" role="img" tabindex="0"
 aria-label="Vorschau des Oszilloskop-Bildes – nach links oder rechts wischen (oder Pfeiltasten), um das Zifferblatt zu wechseln"></canvas><div class="dialtag" id="dialTag"></div></div>
<p class="scopehint">‹ Nach links oder rechts wischen, um das Zifferblatt zu wechseln ›</p>
<div id="clock">--:--:--</div><div id="date">&nbsp;</div></header>

<div class="grp"><h2 id="status-h">Status</h2>
<section id="status"></section></div>

<div class="grp"><h2 id="set-h">Einstellungen</h2>
<section id="settings">
 <!-- Nur sichtbar, solange kein Zeitserver erreichbar war (kein Internet, Setup-WLAN) -->
 <div class="set" id="timeRow" hidden><div class="head"><label>Uhrzeit stellen</label></div>
  <div class="sub">Kein Zeitserver erreichbar (kein Internet). Die Uhrzeit lässt sich hier von Hand stellen.</div>
  <button type="button" id="tPhone">Uhrzeit dieses Geräts übernehmen</button>
  <div class="dt"><input type="date" id="tDate" aria-label="Datum"><input type="time" id="tTime" step="1" aria-label="Uhrzeit"></div>
  <div id="tDstBox"><div class="seg" id="tDst" role="group" aria-label="Sommer-/Winterzeit">
   <button type="button" data-dst="-1" aria-pressed="true">Automatisch</button>
   <button type="button" data-dst="1" aria-pressed="false">Sommerzeit</button>
   <button type="button" data-dst="0" aria-pressed="false">Winterzeit</button></div>
  <div class="sub">Automatisch = nach den Regeln der Zeitzone. Sommer- oder Winterzeit fest wählen, falls die Uhr sonst eine Stunde falsch geht.</div></div>
  <button type="button" class="sec" id="tSave">Datum und Uhrzeit setzen</button>
  <div id="tMsg"></div>
  <div class="sub">Sobald ein Zeitserver erreichbar ist, stellt sich die Uhr wieder selbst. Nach einem Neustart muss die Zeit erneut gestellt werden.</div></div>
 <div class="set"><div class="head"><label>Sprache</label></div>
  <div class="seg" role="group" aria-label="Sprache">
   <button type="button" data-lang="de" aria-pressed="true">Deutsch</button>
   <button type="button" data-lang="en" aria-pressed="false">English</button></div>
  <div class="sub">Gilt für Webinterface, OLED-Anzeige und Protokoll.</div></div>
 <div class="set"><div class="head"><label>Zeitformat</label></div>
  <div class="seg" role="group" aria-label="Zeitformat">
   <button type="button" data-h12="0" aria-pressed="true">24 Stunden</button>
   <button type="button" data-h12="1" aria-pressed="false">12 Stunden (AM/PM)</button></div>
  <div class="sub">Für die Uhrzeit auf der OLED-Anzeige und auf dieser Seite.</div></div>
 <div class="set"><div class="head"><label>Oszilloskop-Profil</label></div>
  <div class="seg" role="group" aria-label="Oszilloskop-Profil">
   <button type="button" data-profile="1" aria-pressed="false">DIGITAL</button>
   <button type="button" data-profile="0" aria-pressed="false">ANALOG</button></div>
  <div class="sub">DIGITAL für digitale Oszilloskope, ANALOG für Röhren-Oszilloskope. Umschalten auch an der Uhr: BOOT-Taster 3 s halten.</div></div>
 <div class="set"><div class="head"><label for="sDial">Zifferblatt</label></div>
  <select id="sDial"><option value="0">Ziffern 1–12</option><option value="1">12 · 3 · 6 · 9</option>
   <option value="2">Römisch I–XII</option><option value="3">Römisch XII · III · VI · IX</option></select>
  <div class="sub">Ziffern auf dem Oszilloskop. Geht auch durch Wischen über die Vorschau oben.</div></div>
 <div class="set"><div class="head"><label for="sBeam">Verweilzeit des Strahls</label><span class="val" id="vBeam"></span></div>
  <input type="range" id="sBeam" min="1000" max="10000" step="250">
  <div class="sub">Länger = hellere Linien, aber weniger Bilder pro Sekunde (jetzt <span id="vFps">–</span>).</div></div>
 <div class="set"><div class="head"><label for="sHand">Helligkeit Stunden-/Minutenzeiger</label><span class="val" id="vHand"></span></div>
  <input type="range" id="sHand" min="30" max="100" step="1">
  <div class="sub">Unter 100 %, weil die schmalen Zeiger sonst heller wirken als der Rest.</div></div>
 <div class="set" id="jumpRow"><div class="head"><label for="sJump">Rücklauf-Tempo</label><span class="val" id="vJump"></span></div>
  <input type="range" id="sJump" min="0" max="48" step="1">
  <div class="sub" id="jumpHint"></div></div>
 <div class="set"><div class="head"><label for="sTz">Zeitzone</label></div><select id="sTz"></select>
  <div class="sub">Bestimmt die Ortszeit samt automatischer Sommer-/Winterzeit.</div></div>
 <div class="set"><div class="head"><label for="sNtp">Zeitserver (NTP)</label></div>
  <div class="pw"><input id="sNtp" maxlength="63" autocapitalize="none" autocorrect="off" spellcheck="false">
  <button type="button" class="sec" id="sNtpSave">Übernehmen</button></div>
  <div class="sub">Server im Internet, von dem die Uhr die genaue Zeit holt (Standard: de.pool.ntp.org).</div></div>
 <div class="set"><div class="head"><label for="sOled">OLED-Helligkeit</label><span class="val" id="vOled"></span></div>
  <input type="range" id="sOled" min="1" max="255" step="1">
  <div class="sub">Helligkeit des kleinen Displays an der Uhr.</div></div>
 <div class="set"><label class="sw"><span>OLED um 180° drehen</span><input type="checkbox" id="sFlip"></label>
  <div class="sub">Für ein kopfüber eingebautes Display.</div></div>
 <div class="set"><label class="sw"><span>OLED-Pixelverschiebung</span><input type="checkbox" id="sShift"></label>
  <div class="sub">Verschiebt das Bild jede Minute um wenige Pixel, gegen Einbrennen.</div></div>
 <div id="setMsg"></div>
 <button type="button" class="sec" id="sReset">Auf Standardwerte zurücksetzen</button>
 <p class="hint">Änderungen wirken sofort und bleiben gespeichert. Standardwerte stammen aus config.h.</p>
</section></div>

<div class="grp"><h2 id="wlan-h">WLAN</h2>
<section>
 <div class="subhead">Gespeicherte Netze</div>
 <div class="desc">Netze, mit denen sich die Uhr verbinden darf. Sie wählt automatisch das stärkste.</div>
 <ul id="saved"><li><span class="hint">wird geladen …</span></li></ul>
 <button class="sec" id="scanBtn">Netze in Reichweite suchen</button>
 <div class="desc">Zeigt die Netze in der Nähe. Ein Netz antippen übernimmt seinen Namen ins Formular.</div>
 <ul id="scan"></ul>
 <form id="wf" autocomplete="off">
  <div class="subhead">Netz hinzufügen</div>
  <div class="desc">Für offene Netze das Passwort leer lassen.</div>
  <label for="ssid">Netzwerkname (SSID)</label>
  <input id="ssid" name="ssid" maxlength="32" autocapitalize="none" autocorrect="off" spellcheck="false" required>
  <label for="pw">Passwort</label>
  <div class="pw"><input id="pw" name="pw" type="password" maxlength="63" autocapitalize="none" autocorrect="off" spellcheck="false">
  <button type="button" class="sec" id="eye">Anzeigen</button></div>
  <div id="msg"></div>
  <button type="submit">Netz speichern</button>
 </form>
 <button class="sec" id="recon">Neu verbinden (mit dem stärksten Netz)</button>
 <p class="hint">Trennt kurz und verbindet sich neu – etwa nach dem Hinzufügen eines Netzes. Gespeicherte Passwörter werden nie angezeigt. Netze aus secrets.h lassen sich hier nicht löschen.</p>
</section></div>

<div class="grp"><h2 id="log-h">Protokoll</h2>
<section>
 <div class="desc top">Meldungen der Uhr zu WLAN, Zeitabgleich und Einstellungen – hilfreich bei der Fehlersuche.</div>
 <pre id="log" tabindex="0" aria-label="Protokoll, im Feld scrollbar">wird geladen …</pre>
 <div class="loginfo"><span id="logInfo"></span><span>neueste Einträge unten</span></div>
 <label class="sw logsw"><span>Automatisch aktualisieren (alle 5 s)</span><input type="checkbox" id="logAuto" checked></label>
 <div class="tools"><button class="sec" id="logBtn">Jetzt aktualisieren</button>
 <button class="sec" id="logFull">Alles anzeigen</button></div>
 <p class="hint">„Alles anzeigen“ hebt die feste Höhe des Feldes auf. Die Uhr speichert die letzten 8 KB (ca. 120 Zeilen).</p>
</section></div>
</main>
<script>
const $=id=>document.getElementById(id);
// ---- Sprache (Deutsch/English) ----------------------------------------------
// Die Seite ist auf Deutsch geschrieben. EN übersetzt deutsche Texte ins Englische.
// t(): Text für Skript-Ausgaben; applyLang(): übersetzt alle festen Texte der Seite
// (Textknoten und aria-label) – über die Rückwärtstabelle DE auch wieder zurück.
// Die Sprache wird auf der Uhr gespeichert (gilt auch für OLED und Protokoll);
// localStorage merkt sie nur, damit die Seite beim Laden gleich richtig erscheint.
const EN={"Abschnitte":"Sections","Uhr":"Clock","Status":"Status","Einstellungen":"Settings","WLAN":"Wi-Fi","Protokoll":"Log","Menü":"Menu","Vorschau des Oszilloskop-Bildes – nach links oder rechts wischen (oder Pfeiltasten), um das Zifferblatt zu wechseln":"Preview of the oscilloscope picture – swipe left or right (or use the arrow keys) to change the dial","‹ Nach links oder rechts wischen, um das Zifferblatt zu wechseln ›":"‹ Swipe left or right to change the dial ›","Sprache":"Language","Gilt für Webinterface, OLED-Anzeige und Protokoll.":"Applies to the web interface, the OLED display and the log.","Uhrzeit stellen":"Set time","Kein Zeitserver erreichbar (kein Internet). Die Uhrzeit lässt sich hier von Hand stellen.":"No time server reachable (no internet). You can set the time by hand here.","Uhrzeit dieses Geräts übernehmen":"Use this device's time","Datum":"Date","Uhrzeit":"Time","Datum und Uhrzeit setzen":"Set date and time","Sobald ein Zeitserver erreichbar ist, stellt sich die Uhr wieder selbst. Nach einem Neustart muss die Zeit erneut gestellt werden.":"As soon as a time server is reachable, the clock sets itself again. After a restart the time has to be set again.","Oszilloskop-Profil":"Oscilloscope profile","DIGITAL für digitale Oszilloskope, ANALOG für Röhren-Oszilloskope. Umschalten auch an der Uhr: BOOT-Taster 3 s halten.":"DIGITAL for digital oscilloscopes, ANALOG for CRT oscilloscopes. Can also be switched on the clock: hold the BOOT button for 3 s.","Zifferblatt":"Dial","Ziffern 1–12":"Numbers 1–12","Römisch I–XII":"Roman I–XII","Römisch XII · III · VI · IX":"Roman XII · III · VI · IX","Ziffern auf dem Oszilloskop. Geht auch durch Wischen über die Vorschau oben.":"Numerals on the oscilloscope. You can also swipe over the preview above.","Verweilzeit des Strahls":"Beam dwell time","Länger = hellere Linien, aber weniger Bilder pro Sekunde (jetzt":"Longer = brighter lines, but fewer frames per second (now","Helligkeit Stunden-/Minutenzeiger":"Hour/minute hand brightness","Unter 100 %, weil die schmalen Zeiger sonst heller wirken als der Rest.":"Below 100 % because the narrow hands would otherwise look brighter than the rest.","Rücklauf-Tempo":"Retrace speed","Zeitzone":"Time zone","Bestimmt die Ortszeit samt automatischer Sommer-/Winterzeit.":"Sets the local time including automatic daylight saving time.","Zeitserver (NTP)":"Time server (NTP)","Übernehmen":"Apply","Server im Internet, von dem die Uhr die genaue Zeit holt (Standard: de.pool.ntp.org).":"Internet server from which the clock gets the exact time (default: de.pool.ntp.org).","OLED-Helligkeit":"OLED brightness","Helligkeit des kleinen Displays an der Uhr.":"Brightness of the small display on the clock.","OLED um 180° drehen":"Rotate OLED by 180°","Für ein kopfüber eingebautes Display.":"For a display mounted upside down.","OLED-Pixelverschiebung":"OLED pixel shift","Verschiebt das Bild jede Minute um wenige Pixel, gegen Einbrennen.":"Moves the picture by a few pixels every minute to prevent burn-in.","Auf Standardwerte zurücksetzen":"Reset to defaults","Änderungen wirken sofort und bleiben gespeichert. Standardwerte stammen aus config.h.":"Changes take effect immediately and are saved. Default values come from config.h.","Gespeicherte Netze":"Saved networks","Netze, mit denen sich die Uhr verbinden darf. Sie wählt automatisch das stärkste.":"Networks the clock may connect to. It automatically picks the strongest one.","wird geladen …":"loading …","Netze in Reichweite suchen":"Search for networks in range","Zeigt die Netze in der Nähe. Ein Netz antippen übernimmt seinen Namen ins Formular.":"Shows nearby networks. Tap a network to copy its name into the form.","Netz hinzufügen":"Add network","Für offene Netze das Passwort leer lassen.":"Leave the password empty for open networks.","Netzwerkname (SSID)":"Network name (SSID)","Passwort":"Password","Anzeigen":"Show","Ausblenden":"Hide","Netz speichern":"Save network","Neu verbinden (mit dem stärksten Netz)":"Reconnect (to the strongest network)","Trennt kurz und verbindet sich neu – etwa nach dem Hinzufügen eines Netzes. Gespeicherte Passwörter werden nie angezeigt. Netze aus secrets.h lassen sich hier nicht löschen.":"Briefly disconnects and reconnects – e.g. after adding a network. Saved passwords are never shown. Networks from secrets.h cannot be deleted here.","Meldungen der Uhr zu WLAN, Zeitabgleich und Einstellungen – hilfreich bei der Fehlersuche.":"Messages from the clock about Wi-Fi, time sync and settings – helpful for troubleshooting.","Protokoll, im Feld scrollbar":"Log, scrollable within the field","neueste Einträge unten":"newest entries at the bottom","Automatisch aktualisieren (alle 5 s)":"Refresh automatically (every 5 s)","Jetzt aktualisieren":"Refresh now","Alles anzeigen":"Show all","Weniger anzeigen":"Show less","„Alles anzeigen“ hebt die feste Höhe des Feldes auf. Die Uhr speichert die letzten 8 KB (ca. 120 Zeilen).":"“Show all” removes the fixed height of the field. The clock keeps the last 8 KB (approx. 120 lines).","Wechsel fehlgeschlagen":"Change failed","µs pro Schritt":"µs per step","sofortiger Sprung":"instant jump","× Zeichentempo":"× drawing speed","Wie schnell der Strahl zwischen zwei Linien springt. Höher = schwächere Rücklaufspuren, 0 = sofortiger Sprung.":"How fast the beam jumps between two lines. Higher = fainter retrace lines, 0 = instant jump.","Im Profil DIGITAL springt der Strahl immer sofort. Der ANALOG-Wert":"In profile DIGITAL the beam always jumps instantly. The ANALOG value","bleibt gespeichert.":"is kept.","Keine Antwort von der Uhr":"No answer from the clock","Bitte Datum und Uhrzeit eintragen":"Please enter date and time","Alle Einstellungen auf die Standardwerte aus config.h zurücksetzen? (gespeicherte WLANs bleiben erhalten)":"Reset all settings to the default values from config.h? (saved networks are kept)","Bilder/s":"frames/s","KW":"Week","Datum unbekannt":"Date unknown","Zeitabgleich (NTP)":"Time sync (NTP)","zuletzt":"last","aus – Uhrzeit von Hand gestellt":"off – time set by hand","wartet auf ersten Abgleich …":"waiting for the first sync …","aus (kein WLAN)":"off (no Wi-Fi)","sucht Netz …":"searching …","kein Netz eingetragen":"no network configured","Signalstärke":"Signal strength","über −67 dBm gut, unter −80 dBm schwach":"above −67 dBm good, below −80 dBm weak","Netzwerkadresse":"Network address","unter dieser Adresse ist die Uhr im Browser erreichbar":"the clock can be reached in the browser at this address","Setup-WLAN":"Setup network","Bildrate":"Frame rate","so oft pro Sekunde wird das Bild am Oszilloskop gezeichnet":"how often per second the picture is drawn on the oscilloscope","Laufzeit":"Uptime","seit dem letzten Neustart":"since the last restart","Arbeitsspeicher":"Memory","von":"of","KB frei":"KB free","Arbeitsspeicher, Tiefstwert":"Memory, minimum","seit dem Neustart":"since the restart","Größter freier Speicherblock":"Largest free memory block","Prozessorlast Kern 0":"CPU load core 0","WLAN, NTP, Webinterface, OLED, Uhrzeit, BOOT‑Taster":"Wi-Fi, NTP, web interface, OLED, time, BOOT button","Prozessorlast Kern 1":"CPU load core 1","Strahlausgabe an GPIO25/26 – läuft absichtlich ohne Pause":"beam output on GPIO25/26 – runs without pause on purpose","Grund des letzten Neustarts":"Reason for the last restart","Firmware-Version":"Firmware version","Verbindung":"Connection","keine Antwort von der Uhr":"no answer from the clock","Tag":"day","Tage":"days","Std.":"h","Min.":"min","noch keine gespeichert":"none saved yet","Löschen":"Delete","aus secrets.h":"from secrets.h","Suche läuft …":"Searching …","(ohne Passwort)":"(no password)","keine Netze gefunden":"no networks found","Zeilen":"lines","(leer)":"(empty)"};
Object.assign(EN,{'Sommer-/Winterzeit':'Summer/winter time','Automatisch':'Automatic','Sommerzeit':'Summer time','Winterzeit':'Winter time',
 'Automatisch = nach den Regeln der Zeitzone. Sommer- oder Winterzeit fest wählen, falls die Uhr sonst eine Stunde falsch geht.':'Automatic = according to the rules of the time zone. Choose summer or winter time explicitly if the clock is otherwise off by one hour.'});
Object.assign(EN,{'Zeitformat':'Time format','24 Stunden':'24 hours','12 Stunden (AM/PM)':'12 hours (AM/PM)',
 'Für die Uhrzeit auf der OLED-Anzeige und auf dieser Seite.':'For the time on the OLED display and on this page.'});
const DE={};for(const k in EN)DE[EN[k]]=k;
let lang='de';try{if(localStorage.getItem('lang')==='en')lang='en'}catch(e){}
const t=s=>lang==='en'&&EN[s]||s;
function trText(v){const k=v.trim();if(!k)return v;const de=DE[k]||k;const n=lang==='en'?(EN[de]||de):de;return n===k?v:v.replace(k,n)}
function applyLang(){document.documentElement.lang=lang;
 const w=document.createTreeWalker(document.body,NodeFilter.SHOW_TEXT,{acceptNode:n=>/^(SCRIPT|STYLE|PRE)$/.test(n.parentNode.nodeName)?2:1});
 for(let n;(n=w.nextNode());){const v=trText(n.nodeValue);if(v!==n.nodeValue)n.nodeValue=v}
 document.querySelectorAll('[aria-label]').forEach(e=>{const v=trText(e.getAttribute('aria-label'));if(v!==e.getAttribute('aria-label'))e.setAttribute('aria-label',v)});
 document.querySelectorAll('[data-lang]').forEach(b=>b.setAttribute('aria-pressed',String(b.dataset.lang===lang)))}
// Uhrzeit anzeigen: Ziffern in <i> mit fester Breite (siehe CSS #clock i)
// 12-Stunden-Format (h12): Stunde 1–12, dahinter klein AM/PM
let h12=false;
function showClock(t){let s=String(t),ap='';const m=/^(\d\d):(\d\d:\d\d)$/.exec(s);
 if(h12&&m){const h=+m[1];ap=h>=12?'PM':'AM';s=(h%12||12)+':'+m[2]}
 $('clock').innerHTML=s.replace(/[0-9-]/g,'<i>$&</i>')+(ap?'<small class="ap">'+ap+'</small>':'')}
const get=(u)=>fetch(u,{cache:'no-store'}).then(r=>{if(!r.ok)throw r.status;return r});
const post=(u,d)=>fetch(u,{method:'POST',body:new URLSearchParams(d)}).then(r=>r.json());
// Statuszeile: Bezeichnung links, Wert rechts, optional Klasse (warn/err) und
// eine kleine Erläuterung unter der Bezeichnung.
function row(k,v,c,note){const d=document.createElement('div');d.className='row';
 const a=document.createElement('span');a.textContent=k;
 if(note){const n=document.createElement('small');n.className='note';n.textContent=note;a.append(n)}
 const b=document.createElement('span');b.textContent=v;if(c)b.className=c;
 if(note&&String(v).length<16)b.style.whiteSpace='nowrap';   // kurzer Wert neben der Erläuterung nicht umbrechen
 d.append(a,b);return d}
// WLAN-Signal (dBm) als Balkenanzeige für die Netzsuche
function bars(r){return r>-55?'▂▄▆█':r>-67?'▂▄▆ ':r>-75?'▂▄  ':'▂   '}
// Sekunden als "x Tage y Std. z Min."
function dur(s){const d=Math.floor(s/86400),h=Math.floor(s/3600)%24,m=Math.floor(s/60)%60;return (d?d+' '+t(d===1?'Tag':'Tage')+' ':'')+h+' '+t('Std.')+' '+m+' '+t('Min.')}
// ---- Oszilloskop-Vorschau ---------------------------------------------------
// Zeichnet dasselbe Bild wie die Uhr (Vektordaten von /api/vectors) auf ein
// Canvas im Look eines Oszilloskop-Schirms. Die Zeigerstellung wird wie im
// scopeTask berechnet: Stunde = (h%12)*5 + m/12, Minute = m, Sekunde = s.
// Zwischen den Status-Abfragen läuft die Zeit lokal weiter (jede Sekunde).
let V=null,tBase=null,tAt=0,dial=0;
// Schirmfarben aus den CSS-Variablen – bei jedem Zeichnen neu gelesen, damit ein
// Wechsel zwischen hellem und dunklem Design sofort greift.
function scopeColors(){const c=getComputedStyle(document.documentElement),v=n=>c.getPropertyValue(n).trim();
 return {bg:v('--scope-bg')||'transparent',trace:v('--scope-trace')||'#0b7a3e',glow:v('--scope-glow')||'rgba(11,122,62,.12)',
  halo:v('--scope-halo')||'rgba(11,122,62,.35)',grid:v('--scope-grid')||'rgba(0,0,0,.07)'}}
const DIALS=['Ziffern 1–12','12 · 3 · 6 · 9','Römisch I–XII','Römisch XII · III · VI · IX'];
// Oszilloskop-Vorschau zeichnen: Raster, Zifferblatt, Ziffern, drei Zeiger
function drawScope(){
 const c=$('scope'),g=c.getContext('2d'),W=c.width,k=W/256;
 const SC=scopeColors();
 g.clearRect(0,0,W,W);if(SC.bg!=='transparent'&&!/rgba\(.*,\s*0\)$/.test(SC.bg)){g.fillStyle=SC.bg;g.fillRect(0,0,W,W)}
 g.strokeStyle=SC.grid;g.lineWidth=1;            // Raster 8x8 wie am Oszilloskop
 for(let i=1;i<8;i++){const p=Math.round(i*W/8)+.5;g.beginPath();g.moveTo(p,0);g.lineTo(p,W);g.moveTo(0,p);g.lineTo(W,p);g.stroke()}
 if(!V||tBase===null)return;
 const sec=(tBase+Math.floor((performance.now()-tAt)/1000))%86400;
 const h=Math.floor(sec/3600),m=Math.floor(sec/60)%60,s=sec%60;
 const polys=[...V.dial,...(V.digits[dial]||[]),V.hr[(h%12)*5+Math.floor(m/12)],V.min[m],V.sec[s]];
 g.lineCap='round';g.lineJoin='round';
 for(const [w,col,blur] of [[5,SC.glow,10],[1.8,SC.trace,3]]){   // Leuchten + Spur (SC: Farben s.o.)
  g.lineWidth=w*k/2;g.strokeStyle=col;g.shadowColor=SC.halo;g.shadowBlur=blur;
  g.beginPath();
  for(const p of polys){if(!p||p.length<4)continue;g.moveTo(p[0]*k,(255-p[1])*k);for(let i=2;i<p.length;i+=2)g.lineTo(p[i]*k,(255-p[i+1])*k)}
  g.stroke()}
 g.shadowBlur=0;
}
// Zifferblatt wechseln – sofort in der Vorschau,
// per POST /api/dial auch am Oszilloskop (wird dort gespeichert).
let tagTimer=0,dialBusy=false;
// Namen des Zifferblatts kurz unten in der Vorschau einblenden
function showTag(t){const e=$('dialTag');e.textContent=t;e.classList.add('show');clearTimeout(tagTimer);tagTimer=setTimeout(()=>e.classList.remove('show'),1600)}
// step = +1 (nächstes) oder -1 (voriges Zifferblatt), reihum
async function nextDial(step=1){
 if(dialBusy)return;dialBusy=true;const prev=dial;dial=(dial+step+DIALS.length)%DIALS.length;drawScope();showTag(t(DIALS[dial]));
 try{const r=await post('/api/dial',{style:dial});if(!r.ok)throw 0;if(S){S.dial=dial;renderSettings()}}catch(e){dial=prev;drawScope();showTag(t('Wechsel fehlgeschlagen'))}
 dialBusy=false}
// Wischen über die Vorschau: nach links = nächstes, nach rechts = voriges Zifferblatt
// (wie beim Blättern in Fotos). Pointer-Events decken Finger und Maus ab.
// Senkrechtes Wischen bleibt Seiten-Scrollen (CSS touch-action:pan-y).
// Tastatur: Pfeil links/rechts, wenn die Vorschau den Fokus hat.
let swX=null,swY=0;
$('scope').addEventListener('pointerdown',e=>{swX=e.clientX;swY=e.clientY});
$('scope').addEventListener('pointerup',e=>{if(swX===null)return;const dx=e.clientX-swX,dy=e.clientY-swY;swX=null;
 if(Math.abs(dx)>=40&&Math.abs(dx)>Math.abs(dy)*1.5)nextDial(dx<0?1:-1)});
$('scope').addEventListener('pointercancel',()=>{swX=null});
$('scope').addEventListener('keydown',e=>{if(e.key==='ArrowLeft'||e.key==='ArrowRight'){e.preventDefault();nextDial(e.key==='ArrowRight'?1:-1)}});
// Seitenmenü
function menu(open){$('nav').classList.toggle('open',open);$('shade').classList.toggle('open',open);
 $('menuBtn').setAttribute('aria-expanded',open);$('nav').setAttribute('aria-hidden',!open);$('menuBtn').textContent=open?'✕':'☰'}
$('menuBtn').onclick=()=>menu(!$('nav').classList.contains('open'));
$('shade').onclick=()=>menu(false);
document.addEventListener('keydown',e=>{if(e.key==='Escape')menu(false)});
document.querySelectorAll('#nav a').forEach(a=>a.addEventListener('click',()=>menu(false)));
// Uhrzeit der Uhr (hh:mm:ss) übernehmen; die Seite zählt danach selbst weiter
function setTime(t){const a=t.split(':').map(Number);if(a.length===3&&!a.some(isNaN)){tBase=a[0]*3600+a[1]*60+a[2];tAt=performance.now()}}
drawScope();                                           // Schirm/Raster sofort zeichnen, noch vor den Daten
matchMedia('(prefers-color-scheme: dark)').addEventListener('change',()=>drawScope());
get('/api/vectors').then(r=>r.json()).then(v=>{V=v;drawScope()}).catch(()=>{});
let lastSec=-1;
setInterval(()=>{if(tBase===null)return;const n=Math.floor((performance.now()-tAt)/1000);
 if(n!==lastSec){lastSec=n;const x=(tBase+n)%86400;showClock([x/3600|0,(x/60|0)%60,x%60].map(v=>String(v).padStart(2,'0')).join(':'));drawScope()}},200);

// ---- Einstellungen ---------------------------------------------------------
// Laden von /api/settings, jede Änderung sofort per POST übernehmen (Schieberegler
// beim Loslassen, damit nicht bei jeder Bewegung in den Flash geschrieben wird).
let S=null,setBusy=false;
const fmtBeam=v=>{const x=(v/1000).toFixed(2);return (lang==='en'?x:x.replace('.',','))+' '+t('µs pro Schritt')};
const fmtJump=v=>+v===0?t('sofortiger Sprung'):v+t('× Zeichentempo');
const fmtOled=v=>Math.round(v/255*100)+' %';
// Alle Einstellungs-Elemente aus S (Antwort von /api/settings) füllen
function renderSettings(){
 if(!S)return;
 document.querySelectorAll('[data-profile]').forEach(b=>b.setAttribute('aria-pressed',String(+b.dataset.profile===S.profile)));
 $('sDial').value=S.dial;
 h12=!!S.h12; document.querySelectorAll('[data-h12]').forEach(b=>b.setAttribute('aria-pressed',String(+b.dataset.h12===(S.h12?1:0))));
 $('sBeam').value=S.beamNs;$('vBeam').textContent=fmtBeam(S.beamNs);
 $('sHand').value=S.handPct;$('vHand').textContent=S.handPct+' %';
 // Rücklauf-Tempo: im Profil DIGITAL wird immer hart gesprungen – der Regler zeigt dann
 // ausgegraut 0. Der gespeicherte ANALOG-Wert bleibt erhalten und erscheint wieder,
 // sobald auf ANALOG umgeschaltet wird.
 const ana=S.profile===0,jv=ana?S.jumpSpeed:0;
 $('sJump').value=jv;$('vJump').textContent=fmtJump(jv);
 $('jumpRow').classList.toggle('off',!ana);$('sJump').disabled=!ana;
 $('jumpHint').textContent=ana?t('Wie schnell der Strahl zwischen zwei Linien springt. Höher = schwächere Rücklaufspuren, 0 = sofortiger Sprung.'):t('Im Profil DIGITAL springt der Strahl immer sofort. Der ANALOG-Wert')+' ('+fmtJump(S.jumpSpeed)+') '+t('bleibt gespeichert.');
 const tz=$('sTz'),tzk=S.timezones.map(z=>z.label).join('|');if(tz.dataset.k!==tzk){tz.replaceChildren();tz.dataset.k=tzk;   // neu aufbauen, wenn sich die Namen (Sprache) ändern
 for(const z of S.timezones){const o=document.createElement('option');o.value=z.tz;o.textContent=z.label;tz.append(o)}}
 tz.value=S.tz;
 if(document.activeElement!==$('sNtp'))$('sNtp').value=S.ntp;
 $('sOled').value=S.oledContrast;$('vOled').textContent=fmtOled(S.oledContrast);
 $('sShift').checked=S.oledShift;
 $('sFlip').checked=S.oledFlip;
}
// Meldung unter den Einstellungen anzeigen (Erfolg verschwindet nach 2,5 s)
function setMsg(r){const m=$('setMsg');m.textContent=r.msg||'';m.className=r.ok?'ok':'err';clearTimeout(setMsg.t);if(r.ok)setMsg.t=setTimeout(()=>m.textContent='',2500)}
// Einstellungen von der Uhr laden
async function loadSettings(){try{S=(await (await get('/api/settings')).json()).settings;renderSettings()}catch(e){}}
// Eine oder mehrere Einstellungen an die Uhr senden und die Antwort übernehmen
async function setSetting(d){
 setBusy=true;
 try{const r=await post('/api/settings',d);if(r.settings){S=r.settings;renderSettings()}setMsg(r);
  if('dial' in d&&r.ok){dial=S.dial;drawScope()}}
 catch(e){setMsg({ok:false,msg:t('Keine Antwort von der Uhr')});loadSettings()}
 setBusy=false}
document.querySelectorAll('[data-profile]').forEach(b=>b.onclick=()=>setSetting({profile:b.dataset.profile}));
document.querySelectorAll('[data-h12]').forEach(b=>b.onclick=()=>{h12=b.dataset.h12==='1';lastSec=-1;setSetting({h12:b.dataset.h12})});
$('sDial').onchange=e=>setSetting({dial:e.target.value});
$('sBeam').oninput=e=>$('vBeam').textContent=fmtBeam(e.target.value);
$('sBeam').onchange=e=>setSetting({beamNs:e.target.value});
$('sHand').oninput=e=>$('vHand').textContent=e.target.value+' %';
$('sHand').onchange=e=>setSetting({handPct:e.target.value});
$('sJump').oninput=e=>$('vJump').textContent=fmtJump(e.target.value);
$('sJump').onchange=e=>setSetting({jumpSpeed:e.target.value});
$('sTz').onchange=e=>setSetting({tz:e.target.value});
$('sNtpSave').onclick=()=>setSetting({ntp:$('sNtp').value.trim()});
$('sOled').oninput=e=>$('vOled').textContent=fmtOled(e.target.value);
$('sOled').onchange=e=>setSetting({oledContrast:e.target.value});
$('sShift').onchange=e=>setSetting({oledShift:e.target.checked?1:0});
$('sFlip').onchange=e=>setSetting({oledFlip:e.target.checked?1:0});
// ---- Uhrzeit von Hand stellen (nur ohne Zeitserver sichtbar)
const pad=v=>String(v).padStart(2,'0');
// Block "Uhrzeit stellen" ein-/ausblenden, beim Einblenden mit der Gerätezeit vorbelegen
// Sommer-/Winterzeit für die manuelle Eingabe: -1 = automatisch, 1 = Sommer-, 0 = Winterzeit
let dstMode=-1;
function showDst(){document.querySelectorAll('[data-dst]').forEach(b=>b.setAttribute('aria-pressed',String(+b.dataset.dst===dstMode)))}
document.querySelectorAll('[data-dst]').forEach(b=>b.onclick=()=>{dstMode=+b.dataset.dst;showDst()});
function showTimeRow(on,dst){const r=$('timeRow');
 $('tDstBox').hidden=!(S&&S.tz&&S.tz.includes(','));            // nur bei Zeitzonen mit Sommerzeit
 if(r.hidden===!on)return;r.hidden=!on;
 if(on&&typeof dst==='number'){dstMode=dst;showDst()}
 if(on){const d=new Date();$('tDate').value=d.getFullYear()+'-'+pad(d.getMonth()+1)+'-'+pad(d.getDate());
  $('tTime').value=pad(d.getHours())+':'+pad(d.getMinutes())+':'+pad(d.getSeconds())}}
// Meldung im Block "Uhrzeit stellen" anzeigen
function tMsg(r){const m=$('tMsg');m.textContent=r.msg||'';m.className=r.ok?'ok':'err'}
// Uhrzeit an die Uhr senden (epoch oder local, siehe POST /api/time)
async function setClockTime(d){d.dst=dstMode;try{const r=await post('/api/time',d);tMsg(r);if(r.ok)status()}catch(e){tMsg({ok:false,msg:t('Keine Antwort von der Uhr')})}}
$('tPhone').onclick=()=>setClockTime({epoch:Math.round(Date.now()/1000)});
$('tSave').onclick=()=>{const d=$('tDate').value,tm=$('tTime').value;
 if(!d||!tm){tMsg({ok:false,msg:t('Bitte Datum und Uhrzeit eintragen')});return}
 setClockTime({local:d+'T'+(tm.length===5?tm+':00':tm)})};
// ---- Sprache umschalten: sofort auf der Seite, dann auf der Uhr speichern (OLED, Protokoll)
let langBusy=false;
function setLangLocal(l){lang=l;try{localStorage.setItem('lang',l)}catch(e){}applyLang();
 ['tMsg','msg','setMsg'].forEach(i=>$(i).textContent='');   // alte Meldungen in der anderen Sprache entfernen

 if(S)renderSettings();status();saved();log()}
document.querySelectorAll('[data-lang]').forEach(b=>b.onclick=async()=>{const l=b.dataset.lang;if(l===lang)return;
 langBusy=true;setLangLocal(l);await setSetting({lang:l});langBusy=false});
$('sReset').onclick=async()=>{if(!confirm(t('Alle Einstellungen auf die Standardwerte aus config.h zurücksetzen? (gespeicherte WLANs bleiben erhalten)')))return;
 setBusy=true;try{const r=await post('/api/settings/reset',{});if(r.settings){S=r.settings;renderSettings();dial=S.dial;drawScope()}setMsg(r)}catch(e){}setBusy=false};

let online=true;
// Status holen (alle 2 s): Uhrzeit, Datum, Statusliste, Vorschau, Einstellungen abgleichen
async function status(){
 try{const s=await (await get('/api/status')).json();online=true;
  showClock(s.time);setTime(s.time);lastSec=-1;
  if(!dialBusy&&typeof s.dial==='number'&&s.dial!==dial){dial=s.dial;drawScope()}
  if(s.lang&&s.lang!==lang&&!langBusy)setLangLocal(s.lang);
  if(typeof s.h12==='boolean'&&s.h12!==h12&&!setBusy){h12=s.h12;lastSec=-1}
  $('vFps').textContent=s.fps+' '+t('Bilder/s');
  showTimeRow(!s.ntp.synced,s.ntp.dst);
  if(S&&!setBusy){const p=s.profile==='DIGITAL'?1:0;if(p!==S.profile||s.dial!==S.dial){S.profile=p;S.dial=s.dial;renderSettings()}}$('date').textContent=s.date?(t('KW')+' '+s.kw+' – '+(lang==='en'?s.date.split('.').reverse().join('-'):s.date)):t('Datum unbekannt');
  const st=$('status');st.replaceChildren(
   row(t('Zeitabgleich (NTP)'),s.ntp.synced?(t('zuletzt')+' '+s.ntp.last):s.ntp.manual?t('aus – Uhrzeit von Hand gestellt'):(s.wifi.configured?t('wartet auf ersten Abgleich …'):t('aus (kein WLAN)')),s.ntp.synced?(s.ntp.overdue?'warn':''):(s.ntp.manual?'':'warn')),
   row(t('WLAN'),s.wifi.connected?s.wifi.ssid:(s.wifi.configured?t('sucht Netz …'):t('kein Netz eingetragen')),s.wifi.connected?'':'warn'),
   ...(s.wifi.connected?[row(t('Signalstärke'),s.wifi.rssi+' dBm',s.wifi.rssi<-80?'warn':'',t('über −67 dBm gut, unter −80 dBm schwach')),row(t('Netzwerkadresse'),s.wifi.ip+'  ·  '+s.wifi.host,'',t('unter dieser Adresse ist die Uhr im Browser erreichbar'))]:[]),
   ...(s.ap.active?[row(t('Setup-WLAN'),s.ap.ssid+' ('+s.ap.ip+')','warn')]:[]),
   row(t('Oszilloskop-Profil'),s.profile),
   row(t('Bildrate'),s.fps+' '+t('Bilder/s'),'',t('so oft pro Sekunde wird das Bild am Oszilloskop gezeichnet')),
   row(t('Laufzeit'),dur(s.uptime),'',t('seit dem letzten Neustart')),
   row(t('Arbeitsspeicher'),Math.round(s.heap/1024)+' '+t('von')+' '+Math.round(s.heapTotal/1024)+' '+t('KB frei')),
   row(t('Arbeitsspeicher, Tiefstwert'),Math.round(s.heapMin/1024)+' '+t('KB frei'),s.heapMin<20480?'warn':'',t('seit dem Neustart')),
   row(t('Größter freier Speicherblock'),Math.round(s.heapBlock/1024)+' KB',s.heapBlock<8192?'warn':''),
   row(t('Prozessorlast Kern 0'),s.cpu0+' %',s.cpu0>85?'warn':'',t('WLAN, NTP, Webinterface, OLED, Uhrzeit, BOOT\u2011Taster')),
   row(t('Prozessorlast Kern 1'),'100 %','',t('Strahlausgabe an GPIO25/26 – läuft absichtlich ohne Pause')),
   row(t('Grund des letzten Neustarts'),s.reset,/Absturz|Watchdog|Unterspannung|Crash|Brown-out/.test(s.reset)?'warn':''),
   row(t('Firmware-Version'),'v'+s.fw));
 }catch(e){if(online){$('status').replaceChildren(row(t('Verbindung'),t('keine Antwort von der Uhr'),'err'))}online=false}
}
// Liste der gespeicherten WLANs laden (Netze aus dem Webinterface mit "Löschen")
async function saved(){
 try{const l=await (await get('/api/networks')).json();const u=$('saved');u.replaceChildren();
  if(!l.length){const li=document.createElement('li');li.innerHTML='<span class="hint">'+t('noch keine gespeichert')+'</span>';u.append(li)}
  for(const n of l){const li=document.createElement('li');const a=document.createElement('span');a.textContent=n.ssid;li.append(a);
   if(n.source==='web'){const b=document.createElement('button');b.className='del';b.textContent=t('Löschen');
    b.onclick=async()=>{if(!confirm(lang==='en'?'Delete “'+n.ssid+'”? The clock restarts afterwards.':'„'+n.ssid+'“ löschen? Die Uhr startet danach neu.'))return;const r=await post('/api/wifi/delete',{ssid:n.ssid});msg(r)};li.append(b)}
   else{const c=document.createElement('span');c.className='hint';c.textContent=t('aus secrets.h');li.append(c)}
   u.append(li)}
 }catch(e){}
}
let scanTimer=0;
// WLAN-Suche starten und das Ergebnis abfragen, bis sie fertig ist
async function scan(){
 $('scanBtn').disabled=true;$('scanBtn').textContent=t('Suche läuft …');
 try{const r=await (await get('/api/scan')).json();
  if(r.running){scanTimer=setTimeout(scan,1500);return}
  const u=$('scan');u.replaceChildren();
  for(const n of r.networks){const li=document.createElement('li');li.className='tap';
   const a=document.createElement('span');a.textContent=n.ssid+(n.open?' '+t('(ohne Passwort)'):'');const b=document.createElement('span');b.className='bars';b.textContent=bars(n.rssi);
   li.append(a,b);li.onclick=()=>{$('ssid').value=n.ssid;$('pw').focus()};u.append(li)}
  if(!r.networks.length){const li=document.createElement('li');li.innerHTML='<span class="hint">'+t('keine Netze gefunden')+'</span>';u.append(li)}
 }catch(e){}
 $('scanBtn').disabled=false;$('scanBtn').textContent=t('Netze in Reichweite suchen');
}
// Meldung im WLAN-Formular anzeigen und die Netzliste neu laden
function msg(r){const m=$('msg');m.textContent=r.msg||'';m.className=r.ok?'ok':'err';saved()}
let logAuto=true;
// Log neu laden. Stand das Feld unten, bleibt es unten (neue Zeilen folgen);
// liest man weiter oben, bleibt die Position erhalten.
let logFirst=true;
// Protokoll laden; bleibt unten, wenn man unten war, sonst an der Leseposition
async function log(){try{const txt=await (await get('/api/log')).text();const p=$('log');
 const end=logFirst||p.scrollTop+p.clientHeight>=p.scrollHeight-20,top=p.scrollTop;
 p.textContent=txt||t('(leer)');p.scrollTop=end?p.scrollHeight:top;logFirst=false;
 const n=txt?txt.replace(/\n$/,'').split('\n').length:0;$('logInfo').textContent=n+' '+t('Zeilen')}catch(e){}}
$('wf').onsubmit=async e=>{e.preventDefault();const r=await post('/api/wifi',{ssid:$('ssid').value.trim(),pw:$('pw').value});msg(r);if(r.ok)$('pw').value=''};
$('eye').onclick=()=>{const p=$('pw');p.type=p.type==='password'?'text':'password';$('eye').textContent=t(p.type==='password'?'Anzeigen':'Ausblenden')};
$('scanBtn').onclick=scan;
$('recon').onclick=async()=>msg(await post('/api/reconnect',{}));
$('logBtn').onclick=log;
$('logAuto').onchange=e=>{logAuto=e.target.checked;if(logAuto)log()};
$('logFull').onclick=()=>{const p=$('log'),f=p.classList.toggle('full');$('logFull').textContent=t(f?'Weniger anzeigen':'Alles anzeigen');
 if(!f){p.scrollTop=p.scrollHeight;$('log-h').scrollIntoView()}};
applyLang();status();saved();log();loadSettings();
setInterval(()=>{if(document.visibilityState==='visible')status()},2000);
setInterval(()=>{if(logAuto&&document.visibilityState==='visible')log()},5000);
</script>
</body></html>)HTML";
