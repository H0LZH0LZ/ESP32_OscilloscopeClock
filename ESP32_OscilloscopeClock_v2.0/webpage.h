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

  Für die Mobilbedienung optimiert:
   - eine Spalte, max. 640 px breit, Abstände für Daumenbedienung
   - einklappbares Seitenmenü (☰ oben links) zum Springen in die Abschnitte
   - Uhr-Abschnitt zentriert; Abschnittsüberschriften bleiben beim Scrollen stehen
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
:root{--bg:#f2f2f5;--card:#fff;--fg:#111;--mute:#6b6b70;--line:#e3e3e8;--acc:#0a7d44;--warn:#b45309;--err:#b91c1c;--mono:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace}
@media (prefers-color-scheme:dark){:root{--bg:#000;--card:#1c1c1e;--fg:#f2f2f7;--mute:#9a9aa0;--line:#2c2c2e;--acc:#30d158;--warn:#ffb340;--err:#ff6961}}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--fg);font:16px/1.4 system-ui,-apple-system,Segoe UI,Roboto,sans-serif;
 padding:env(safe-area-inset-top) env(safe-area-inset-right) env(safe-area-inset-bottom) env(safe-area-inset-left);-webkit-text-size-adjust:100%}
main{max-width:640px;margin:0 auto;padding:12px 16px 32px}
/* Oberster Abschnitt (Uhr): alles zentriert. Die Überschrift steht auf Höhe der
   Menütaste und hält links und rechts gleich viel Abstand, damit die Mitte stimmt;
   die Oszilloskop-Vorschau nutzt die volle Breite. */
header{padding:21px 0 4px;text-align:center}
header h1{padding:0 46px}
h1{font-size:15px;font-weight:600;color:var(--mute);margin:0;text-transform:uppercase;letter-spacing:.06em}
#clock{font:600 44px/1.1 var(--mono);margin:4px 0 0;font-variant-numeric:tabular-nums}
#date{color:var(--mute)}
.scope canvas{margin:0 auto}
section{background:var(--card);border-radius:14px;margin:0 0 14px;padding:4px 16px}
/* Abschnittsüberschriften bleiben beim Scrollen oben stehen – auf Höhe der
   Menütaste (64 px Leiste, Text rechts neben der Taste). Die nächste Überschrift
   schiebt die vorige weg; so ist immer zu sehen, in welchem Abschnitt man ist. */
h2{position:sticky;top:env(safe-area-inset-top);z-index:20;display:flex;align-items:center;min-height:64px;margin:0;
 padding:0 0 0 46px;background:var(--bg);font-size:13px;font-weight:600;color:var(--mute);text-transform:uppercase;letter-spacing:.06em}
/* Streifen unter Notch/Statusleiste deckt durchscrollenden Inhalt ab */
body::before{content:"";position:fixed;inset:0 0 auto;height:env(safe-area-inset-top);background:var(--bg);z-index:25}
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
.scope{background:#050805;border-radius:14px;margin:12px 0 4px;padding:10px;display:flex;justify-content:center}
.scope{position:relative}
.scope canvas{width:100%;max-width:340px;aspect-ratio:1;display:block;cursor:pointer;-webkit-tap-highlight-color:transparent}
.dialtag{position:absolute;left:50%;bottom:14px;transform:translateX(-50%);background:rgba(0,0,0,.65);color:#b6ffc4;
 font-size:13px;padding:4px 10px;border-radius:999px;pointer-events:none;transition:opacity .4s;opacity:0;white-space:nowrap}
.dialtag.show{opacity:1}
.scopehint{color:var(--mute);font-size:13px;margin:0 0 6px}
html{scroll-behavior:smooth;scroll-padding-top:64px}
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
#shade{position:fixed;inset:0;z-index:35;background:rgba(0,0,0,.35);opacity:0;pointer-events:none;transition:opacity .25s}
#shade.open{opacity:1;pointer-events:auto}

</style></head>
<body>
<button id="menuBtn" aria-label="Menü" aria-controls="nav" aria-expanded="false">☰</button>
<nav id="nav" aria-label="Abschnitte" aria-hidden="true">
 <h3>Scope Clock</h3>
 <a href="#uhr">Uhr</a>
 <a href="#status-h">Status</a>
 <a href="#wlan-h">WLAN</a>
 <a href="#log-h">Log</a>
</nav>
<div id="shade"></div>
<main>
<header id="uhr"><h1>ESP32 Oscilloscope Clock</h1>
<div class="scope"><canvas id="scope" width="512" height="512" role="button" tabindex="0"
 aria-label="Vorschau des Oszilloskop-Bildes – tippen, um das Zifferblatt umzuschalten"></canvas><div class="dialtag" id="dialTag"></div></div>
<p class="scopehint">Tippen, um das Zifferblatt umzuschalten</p>
<div id="clock">--:--:--</div><div id="date">&nbsp;</div></header>

<h2 id="status-h">Status</h2>
<section id="status"></section>

<h2 id="wlan-h">WLAN</h2>
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

<h2 id="log-h">Log</h2>
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
// ---- Oszilloskop-Vorschau ---------------------------------------------------
// Zeichnet dasselbe Bild wie die Uhr (Vektordaten von /api/vectors) auf ein
// Canvas im Look eines Oszilloskop-Schirms. Die Zeigerstellung wird wie im
// scopeTask berechnet: Stunde = (h%12)*5 + m/12, Minute = m, Sekunde = s.
// Zwischen den Status-Abfragen läuft die Zeit lokal weiter (jede Sekunde).
let V=null,tBase=null,tAt=0,dial=0;
const DIALS=['Ziffern 1–12','12 · 3 · 6 · 9','Römisch I–XII','Römisch XII · III · VI · IX'];
function drawScope(){
 const c=$('scope'),g=c.getContext('2d'),W=c.width,k=W/256;
 g.fillStyle='#050805';g.fillRect(0,0,W,W);
 g.strokeStyle='rgba(120,255,140,.08)';g.lineWidth=1;            // Raster 8x8 wie am Oszilloskop
 for(let i=1;i<8;i++){const p=Math.round(i*W/8)+.5;g.beginPath();g.moveTo(p,0);g.lineTo(p,W);g.moveTo(0,p);g.lineTo(W,p);g.stroke()}
 if(!V||tBase===null)return;
 const sec=(tBase+Math.floor((performance.now()-tAt)/1000))%86400;
 const h=Math.floor(sec/3600),m=Math.floor(sec/60)%60,s=sec%60;
 const polys=[...V.dial,...(V.digits[dial]||[]),V.hr[(h%12)*5+Math.floor(m/12)],V.min[m],V.sec[s]];
 g.lineCap='round';g.lineJoin='round';
 for(const [w,col,blur] of [[5,'rgba(80,255,120,.18)',10],[1.8,'#b6ffc4',3]]){   // Leuchten + Spur
  g.lineWidth=w*k/2;g.strokeStyle=col;g.shadowColor='#3dff6a';g.shadowBlur=blur;
  g.beginPath();
  for(const p of polys){if(!p||p.length<4)continue;g.moveTo(p[0]*k,(255-p[1])*k);for(let i=2;i<p.length;i+=2)g.lineTo(p[i]*k,(255-p[i+1])*k)}
  g.stroke()}
 g.shadowBlur=0;
}
// Tippen auf die Vorschau: nächstes Zifferblatt – sofort in der Vorschau,
// per POST /api/dial auch am Oszilloskop (wird dort gespeichert).
let tagTimer=0,dialBusy=false;
function showTag(t){const e=$('dialTag');e.textContent=t;e.classList.add('show');clearTimeout(tagTimer);tagTimer=setTimeout(()=>e.classList.remove('show'),1600)}
async function nextDial(){
 if(dialBusy)return;dialBusy=true;const prev=dial;dial=(dial+1)%DIALS.length;drawScope();showTag(DIALS[dial]);
 try{const r=await post('/api/dial',{style:dial});if(!r.ok)throw 0}catch(e){dial=prev;drawScope();showTag('Umschalten fehlgeschlagen')}
 dialBusy=false}
$('scope').addEventListener('click',nextDial);
$('scope').addEventListener('keydown',e=>{if(e.key==='Enter'||e.key===' '){e.preventDefault();nextDial()}});
// Seitenmenü
function menu(open){$('nav').classList.toggle('open',open);$('shade').classList.toggle('open',open);
 $('menuBtn').setAttribute('aria-expanded',open);$('nav').setAttribute('aria-hidden',!open);$('menuBtn').textContent=open?'✕':'☰'}
$('menuBtn').onclick=()=>menu(!$('nav').classList.contains('open'));
$('shade').onclick=()=>menu(false);
document.addEventListener('keydown',e=>{if(e.key==='Escape')menu(false)});
document.querySelectorAll('#nav a').forEach(a=>a.addEventListener('click',()=>menu(false)));
function setTime(t){const a=t.split(':').map(Number);if(a.length===3&&!a.some(isNaN)){tBase=a[0]*3600+a[1]*60+a[2];tAt=performance.now()}}
get('/api/vectors').then(r=>r.json()).then(v=>{V=v;drawScope()}).catch(()=>{});
let lastSec=-1;
setInterval(()=>{if(tBase===null)return;const n=Math.floor((performance.now()-tAt)/1000);
 if(n!==lastSec){lastSec=n;const x=(tBase+n)%86400;$('clock').textContent=[x/3600|0,(x/60|0)%60,x%60].map(v=>String(v).padStart(2,'0')).join(':');drawScope()}},200);

let online=true;
async function status(){
 try{const s=await (await get('/api/status')).json();online=true;
  $('clock').textContent=s.time;setTime(s.time);lastSec=-1;
  if(!dialBusy&&typeof s.dial==='number'&&s.dial!==dial){dial=s.dial;drawScope()}$('date').textContent=s.date?('KW '+s.kw+' – '+s.date):'Datum unbekannt';
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
