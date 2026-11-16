/******************************************************************************
  webpage.h – Webinterface der ESP32 Oscilloscope Clock (eine einzige Seite)

  Die Seite liegt als Text im Flash (PROGMEM) und wird unter "/" ausgeliefert.
  Sie lädt alle Daten per JavaScript von den JSON-Schnittstellen der Uhr
  (siehe Abschnitt "Webinterface" in der .ino und SOFTWARE.md):

    GET  /api/status        Uhrzeit, NTP, WLAN, Setup-WLAN, Profil, Bildrate …
    GET  /api/log           Log (Text, älteste Zeile zuerst)
    GET  /api/networks      gespeicherte WLANs (nur Namen)
    GET  /api/scan          WLANs in Reichweite (Suche läuft im Hintergrund)
    POST /api/wifi          WLAN speichern        (ssid, pw)
    POST /api/wifi/delete   WLAN löschen          (ssid) – Uhr startet neu
    POST /api/reconnect     WLAN neu verbinden

  Für die Mobilbedienung optimiert:
   - eine Spalte, max. 640 px breit, Abstände für Daumenbedienung
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
:root{--bg:#f2f2f5;--card:#fff;--fg:#111;--mute:#6b6b70;--line:#e3e3e8;--acc:#0a7d44;--warn:#b45309;--err:#b91c1c;--mono:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace}
@media (prefers-color-scheme:dark){:root{--bg:#000;--card:#1c1c1e;--fg:#f2f2f7;--mute:#9a9aa0;--line:#2c2c2e;--acc:#30d158;--warn:#ffb340;--err:#ff6961}}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--fg);font:16px/1.4 system-ui,-apple-system,Segoe UI,Roboto,sans-serif;
 padding:env(safe-area-inset-top) env(safe-area-inset-right) env(safe-area-inset-bottom) env(safe-area-inset-left);-webkit-text-size-adjust:100%}
main{max-width:640px;margin:0 auto;padding:12px 16px 32px}
header{padding:8px 4px 4px}
h1{font-size:15px;font-weight:600;color:var(--mute);margin:0;text-transform:uppercase;letter-spacing:.06em}
#clock{font:600 44px/1.1 var(--mono);margin:4px 0 0;font-variant-numeric:tabular-nums}
#date{color:var(--mute)}
section{background:var(--card);border-radius:14px;margin:14px 0;padding:4px 16px}
h2{font-size:13px;font-weight:600;color:var(--mute);text-transform:uppercase;letter-spacing:.06em;margin:14px 0 4px}
.row{display:flex;justify-content:space-between;align-items:center;gap:12px;min-height:44px;border-top:1px solid var(--line);padding:6px 0}
.row:first-of-type{border-top:0}
.row span:first-child{color:var(--mute)}
.row span:last-child{text-align:right;overflow-wrap:anywhere}
.ok{color:var(--acc)}.warn{color:var(--warn)}.err{color:var(--err)}
button{font:inherit;min-height:44px;border:0;border-radius:10px;padding:0 16px;background:var(--acc);color:#fff;font-weight:600;width:100%;margin:6px 0}
button.sec{background:transparent;color:var(--acc);border:1px solid var(--line)}
button.del{width:auto;min-height:36px;padding:0 12px;background:transparent;color:var(--err);border:1px solid var(--line);margin:0}
button:disabled{opacity:.5}
label{display:block;color:var(--mute);font-size:14px;margin:10px 0 4px}
input{font:inherit;font-size:16px;width:100%;min-height:44px;padding:0 12px;border:1px solid var(--line);border-radius:10px;background:var(--bg);color:var(--fg)}
.pw{display:flex;gap:8px}.pw button{width:auto;margin:0;white-space:nowrap}
ul{list-style:none;margin:0;padding:0}
li{display:flex;justify-content:space-between;align-items:center;gap:12px;min-height:44px;border-top:1px solid var(--line)}
li:first-child{border-top:0}
li.tap{cursor:pointer}
.bars{font-family:var(--mono);color:var(--mute);white-space:nowrap}
.hint{color:var(--mute);font-size:14px;margin:6px 0 12px}
#msg{min-height:20px;font-size:14px;margin:4px 0}
pre{font:12px/1.45 var(--mono);white-space:pre-wrap;overflow-wrap:anywhere;max-height:50vh;overflow:auto;margin:8px 0 12px;
 background:var(--bg);border-radius:10px;padding:10px;-webkit-overflow-scrolling:touch}
.tools{display:flex;gap:8px}.tools button{margin:0 0 12px}
</style></head>
<body><main>
<header><h1>ESP32 Oscilloscope Clock</h1><div id="clock">--:--:--</div><div id="date">&nbsp;</div></header>

<h2>Status</h2>
<section id="status"></section>

<h2>WLAN</h2>
<section>
 <label>Gespeicherte Netze</label>
 <ul id="saved"><li><span class="hint">wird geladen …</span></li></ul>
 <button class="sec" id="scanBtn">Netze in Reichweite suchen</button>
 <ul id="scan"></ul>
 <form id="wf" autocomplete="off">
  <label for="ssid">Netzwerkname (SSID)</label>
  <input id="ssid" name="ssid" maxlength="32" autocapitalize="none" autocorrect="off" spellcheck="false" required>
  <label for="pw">Passwort</label>
  <div class="pw"><input id="pw" name="pw" type="password" maxlength="63" autocapitalize="none" autocorrect="off" spellcheck="false">
  <button type="button" class="sec" id="eye">Zeigen</button></div>
  <div id="msg"></div>
  <button type="submit">Speichern</button>
 </form>
 <button class="sec" id="recon">Neu verbinden (stärkstes Netz)</button>
 <p class="hint">Gespeicherte Passwörter werden nie angezeigt. Netze aus secrets.h lassen sich hier nicht löschen.</p>
</section>

<h2>Log</h2>
<section>
 <pre id="log">wird geladen …</pre>
 <div class="tools"><button class="sec" id="logBtn">Aktualisieren</button><button class="sec" id="logAuto">Auto: an</button></div>
</section>
</main>
<script>
const $=id=>document.getElementById(id);
const get=(u)=>fetch(u,{cache:'no-store'}).then(r=>{if(!r.ok)throw r.status;return r});
const post=(u,d)=>fetch(u,{method:'POST',body:new URLSearchParams(d)}).then(r=>r.json());
function row(k,v,c){const d=document.createElement('div');d.className='row';
 const a=document.createElement('span');a.textContent=k;const b=document.createElement('span');b.textContent=v;if(c)b.className=c;d.append(a,b);return d}
function bars(r){return r>-55?'▂▄▆█':r>-67?'▂▄▆ ':r>-75?'▂▄  ':'▂   '}
function dur(s){const d=Math.floor(s/86400),h=Math.floor(s/3600)%24,m=Math.floor(s/60)%60;return (d?d+' d ':'')+h+' h '+m+' min'}
let online=true;
async function status(){
 try{const s=await (await get('/api/status')).json();online=true;
  $('clock').textContent=s.time;$('date').textContent=s.date?('KW '+s.kw+' – '+s.date):'Datum unbekannt';
  const st=$('status');st.replaceChildren(
   row('NTP',s.ntp.synced?('synchronisiert, '+s.ntp.last):(s.wifi.configured?'warte auf Sync …':'aus (kein WLAN)'),s.ntp.synced?(s.ntp.overdue?'warn':'ok'):'warn'),
   row('WLAN',s.wifi.connected?s.wifi.ssid:(s.wifi.configured?'sucht …':'kein Netz eingetragen'),s.wifi.connected?'ok':'warn'),
   ...(s.wifi.connected?[row('Signal',s.wifi.rssi+' dBm'),row('Adresse',s.wifi.ip+'  ·  '+s.wifi.host)]:[]),
   ...(s.ap.active?[row('Setup-WLAN',s.ap.ssid+' ('+s.ap.ip+')','warn')]:[]),
   row('Oszilloskop-Profil',s.profile),
   row('Bildrate',s.fps+' Bilder/s'),
   row('Laufzeit',dur(s.uptime)),
   row('Freier Speicher',Math.round(s.heap/1024)+' KB'),
   row('Firmware','v'+s.fw));
 }catch(e){if(online){$('status').replaceChildren(row('Verbindung','keine Antwort von der Uhr','err'))}online=false}
}
async function saved(){
 try{const l=await (await get('/api/networks')).json();const u=$('saved');u.replaceChildren();
  if(!l.length){const li=document.createElement('li');li.innerHTML='<span class="hint">noch keins</span>';u.append(li)}
  for(const n of l){const li=document.createElement('li');const a=document.createElement('span');a.textContent=n.ssid;li.append(a);
   if(n.source==='web'){const b=document.createElement('button');b.className='del';b.textContent='Löschen';
    b.onclick=async()=>{if(!confirm('„'+n.ssid+'“ löschen? Die Uhr startet danach neu.'))return;const r=await post('/api/wifi/delete',{ssid:n.ssid});msg(r)};li.append(b)}
   else{const c=document.createElement('span');c.className='hint';c.textContent='secrets.h';li.append(c)}
   u.append(li)}
 }catch(e){}
}
let scanTimer=0;
async function scan(){
 $('scanBtn').disabled=true;$('scanBtn').textContent='Suche läuft …';
 try{const r=await (await get('/api/scan')).json();
  if(r.running){scanTimer=setTimeout(scan,1500);return}
  const u=$('scan');u.replaceChildren();
  for(const n of r.networks){const li=document.createElement('li');li.className='tap';
   const a=document.createElement('span');a.textContent=n.ssid+(n.open?' (offen)':'');const b=document.createElement('span');b.className='bars';b.textContent=bars(n.rssi);
   li.append(a,b);li.onclick=()=>{$('ssid').value=n.ssid;$('pw').focus()};u.append(li)}
  if(!r.networks.length){const li=document.createElement('li');li.innerHTML='<span class="hint">keine Netze gefunden</span>';u.append(li)}
 }catch(e){}
 $('scanBtn').disabled=false;$('scanBtn').textContent='Netze in Reichweite suchen';
}
function msg(r){const m=$('msg');m.textContent=r.msg||'';m.className=r.ok?'ok':'err';saved()}
let logAuto=true;
async function log(){try{const t=await (await get('/api/log')).text();const p=$('log');const end=p.scrollTop+p.clientHeight>=p.scrollHeight-20;p.textContent=t||'(leer)';if(end)p.scrollTop=p.scrollHeight}catch(e){}}
$('wf').onsubmit=async e=>{e.preventDefault();const r=await post('/api/wifi',{ssid:$('ssid').value.trim(),pw:$('pw').value});msg(r);if(r.ok)$('pw').value=''};
$('eye').onclick=()=>{const p=$('pw');p.type=p.type==='password'?'text':'password';$('eye').textContent=p.type==='password'?'Zeigen':'Verbergen'};
$('scanBtn').onclick=scan;
$('recon').onclick=async()=>msg(await post('/api/reconnect',{}));
$('logBtn').onclick=log;
$('logAuto').onclick=()=>{logAuto=!logAuto;$('logAuto').textContent='Auto: '+(logAuto?'an':'aus')};
status();saved();log();
setInterval(()=>{if(document.visibilityState==='visible')status()},2000);
setInterval(()=>{if(logAuto&&document.visibilityState==='visible')log()},5000);
</script>
</body></html>)HTML";
