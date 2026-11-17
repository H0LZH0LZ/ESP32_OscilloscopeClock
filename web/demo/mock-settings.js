// ===== Demo-Schicht, Teil 2: Einstellungen =====
(function(){
const TZ=[["Mitteleuropa (Berlin, Wien, Zürich)","CET-1CEST,M3.5.0,M10.5.0/3"],["Westeuropa (London, Lissabon)","GMT0BST,M3.5.0/1,M10.5.0"],["Osteuropa (Helsinki, Athen)","EET-2EEST,M3.5.0/3,M10.5.0/4"],["Moskau","MSK-3"],["UTC","UTC0"],["USA Ost (New York)","EST5EDT,M3.2.0,M11.1.0"],["USA Zentral (Chicago)","CST6CDT,M3.2.0,M11.1.0"],["USA Mountain (Denver)","MST7MDT,M3.2.0,M11.1.0"],["USA West (Los Angeles)","PST8PDT,M3.2.0,M11.1.0"],["Indien (Delhi)","IST-5:30"],["China (Peking)","CST-8"],["Japan (Tokio)","JST-9"],["Australien Ost (Sydney)","AEST-10AEDT,M10.1.0,M4.1.0/3"]];
const D={profile:1,dial:0,beamNs:3000,handPct:67,tickPct:100,digitPct:100,jumpSpeed:24,zblank:false,zblankHigh:true,tz:TZ[0][1],ntp:"de.pool.ntp.org",oledPct:50,oledSec:true,oledShift:true,oledFlip:false,lang:'de',h12:false,host:'scope-clock',rtc:false};
const S=Object.assign({},D);
S.test=true;let testTouched=false;setTimeout(()=>{if(!testTouched){S.test=false;window.__demoLog&&window.__demoLog('[Scope] Start-Testbild beendet, Uhr laeuft')}},10000);   // wie die Uhr: Testbild 10 s nach dem Einschalten
const TZEN=['Central Europe (Berlin, Vienna, Zurich)','Western Europe (London, Lisbon)','Eastern Europe (Helsinki, Athens)','Moscow','UTC','US Eastern (New York)','US Central (Chicago)','US Mountain (Denver)','US Pacific (Los Angeles)','India (Delhi)','China (Beijing)','Japan (Tokyo)','Australia Eastern (Sydney)'];
const out=()=>Object.assign({},S,{zblankPin:27,rtcSda:21,rtcScl:22,defaults:D,timezones:TZ.map(([label,tz],i)=>({label:S.lang==='en'?TZEN[i]:label,tz}))});
const J=o=>new Response(JSON.stringify(o),{status:200,headers:{'Content-Type':'application/json'}});
const prev=window.fetch;
window.fetch=async function(url,init){
 const u=String(url).replace(/^https?:\/\/[^/]+/,'').split('?')[0];
 const p=init&&init.body?new URLSearchParams(init.body):new URLSearchParams();
 if(u==='/api/settings'&&(!init||!init.method||init.method==='GET'))return J({ok:true,msg:'',settings:out()});
 if(u==='/api/settings'){const e=[];let n=0;
  const num=(k,lo,hi,m)=>{if(!p.has(k))return;const v=+p.get(k);if(Number.isInteger(v)&&v>=lo&&v<=hi){S[k]=v;n++}else e.push(m)};
  num('profile',0,1,'Profil ungültig');num('dial',0,4,'Zifferblatt ungültig');num('beamNs',1000,10000,'Strahlgeschwindigkeit: 1000 bis 10000 ns');
  num('handPct',0,100,'Zeigerhelligkeit: 0 bis 100 %');num('tickPct',0,100,'Helligkeit Minutenstriche: 0 bis 100 %');num('digitPct',0,100,'Helligkeit Ziffern: 0 bis 100 %');num('oledPct',0,100,'OLED-Helligkeit: 0 bis 100 %');for(const k of ['oledSec'])if(p.has(k)){S[k]=p.get(k)==='1';n++}num('jumpSpeed',0,48,'Rücklauf-Tempo: 0 bis 48');
  if(p.has('tz')){if(TZ.some(z=>z[1]===p.get('tz'))){S.tz=p.get('tz');n++}else e.push('Zeitzone unbekannt')}
  if(p.has('ntp')){const h=p.get('ntp');if(/^[A-Za-z0-9.-]{1,63}$/.test(h)){S.ntp=h;n++}else e.push('NTP-Server: nur Buchstaben, Ziffern, Punkt und Bindestrich (max. 63)')}
  if(p.has('oledShift')){S.oledShift=p.get('oledShift')==='1';n++}
  for(const k of ['zblank','zblankHigh','rtc'])if(p.has(k)){S[k]=p.get(k)==='1'||p.get(k)==='true';n++}
  if(p.has('oledFlip')){S.oledFlip=p.get('oledFlip')==='1';n++}
  if(p.has('lang')){S.lang=p.get('lang')==='en'?'en':'de';n++}
  if(p.has('h12')){S.h12=p.get('h12')==='1';n++}
  if(p.has('test')){S.test=p.get('test')==='1';testTouched=true;n++}
  if(p.has('host')){const h=p.get('host');if(/^[a-z0-9]([a-z0-9-]{0,30}[a-z0-9])?$/.test(h)){S.host=h;n++}else e.push(S.lang==='en'?'Device name: a–z, 0–9 and hyphen (max. 32)':'Gerätename: a–z, 0–9 und Bindestrich (max. 32)')}
  if(p.has('dial'))await prev('/api/dial',{method:'POST',body:new URLSearchParams({style:S.dial})});
  for(const k of p.keys())window.__demoLog&&window.__demoLog('[Web] Einstellung '+k+' = '+p.get(k));
  const en=S.lang==='en';return J({ok:!e.length,msg:e.length?e.join('; '):(n?(en?'Saved':'Gespeichert'):(en?'No change':'Keine Änderung')),settings:out()})}
 if(u==='/api/settings/reset'){const G={scope:['profile','dial','beamNs','handPct','tickPct','digitPct','jumpSpeed','zblank','zblankHigh'],time:['tz','ntp','h12','rtc'],display:['oledPct','oledSec','oledShift','oledFlip']};const g=p.get('group');if(g&&G[g])for(const k of G[g])S[k]=D[k];else {Object.assign(S,D);S.test=false}if(!g||g==='scope')await prev('/api/dial',{method:'POST',body:new URLSearchParams({style:S.dial})});
  window.__demoLog&&window.__demoLog('[Web] Einstellungen auf Standard zurueckgesetzt');return J({ok:true,msg:S.lang==='en'?'Reset to defaults':'Auf Standardwerte zurückgesetzt',settings:out()})}
 if(u==='/api/time'){const r=await prev(url,init);const o=await r.json();if(o.ok){const d=new Date(Date.now()+((window.__demo&&window.__demo.off)||0)),z=v=>String(v).padStart(2,'0');o.msg=S.lang==='en'?'Time set: '+d.getFullYear()+'-'+z(d.getMonth()+1)+'-'+z(d.getDate())+' '+z(d.getHours())+':'+z(d.getMinutes())+':'+z(d.getSeconds()):'Uhrzeit gestellt: '+z(d.getDate())+'.'+z(d.getMonth()+1)+'.'+d.getFullYear()+' '+z(d.getHours())+':'+z(d.getMinutes())+':'+z(d.getSeconds())}window.__dst=p.has('dst')?+p.get('dst'):-1;return J(o)}
 const r=await prev(url,init);
 if(u==='/api/dial'&&p.has('style'))S.dial=+p.get('style');
 if(u==='/api/status'){const s=await r.json();s.profile=S.profile?'DIGITAL':'ANALOG';S.dial=s.dial;
  s.fps=Math.round(1e9/(1420*S.beamNs))+Math.round(Math.random()*4-2);s.heapTotal=301400;s.heapMin=171200;s.heapBlock=110580;s.cpu0=6+Math.round(Math.random()*6);s.ntp.dst=window.__dst??-1;s.reset=S.lang==='en'?'Power on':'Einschalten';s.lang=S.lang;s.oled=S.oledPct;s.rtc={enabled:!!S.rtc,found:!!S.rtc,valid:!!S.rtc,tempQ:S.rtc?94:-32768};s.h12=S.h12;s.test=!!S.test;s.wifi.host=S.host+'.local';return J(s)}
 return r};
})();
