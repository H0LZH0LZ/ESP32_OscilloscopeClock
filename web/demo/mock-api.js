// ===== Demo-Schicht: ersetzt die Schnittstellen der Uhr (nur in dieser Vorschau) =====
(function(){
const VEC=/*VEC*/null;
const t0=Date.now();
const pad=n=>String(n).padStart(2,'0');
function isoWeek(d){const t=new Date(Date.UTC(d.getFullYear(),d.getMonth(),d.getDate()));const day=t.getUTCDay()||7;t.setUTCDate(t.getUTCDate()+4-day);const y=new Date(Date.UTC(t.getUTCFullYear(),0,1));return Math.ceil(((t-y)/864e5+1)/7)}
const hms=d=>pad(d.getHours())+':'+pad(d.getMinutes())+':'+pad(d.getSeconds());
let dial=0;try{dial=+(localStorage.getItem('demoDial')||0)%5}catch(e){}
const st={setup:false,ssid:'Heimnetz',rssi:-61,nets:[{ssid:'Heimnetz',source:'web'},{ssid:'Werkstatt',source:'web'}],scanN:0,lastSync:new Date(t0-17*60000)};
const log=[];
function L(msg){const up=Math.floor((Date.now()-t0)/1000);log.push(hms(new Date())+' '+msg)}
const b=new Date(t0-180000), f=s=>'+'+s+'s ';
log.push(f(0)+'ESP32 Oscilloscope Clock v2.0 (Idee nach Mauro Pintus 2018)',f(0)+'CPU: 240 MHz',f(0)+'[Scope] Profil: DIGITAL – BOOT-Taster 3 s halten zum Umschalten',
 f(0)+'[UI] laeuft auf Core 0',f(0)+'[Scope] laeuft auf Core 1',f(0)+'[Scope] Rundtour: 76 Linienzuege, Spruenge gesamt 1033 DAC-Einheiten',
 f(0)+'[WiFi] bekanntes Netz: "Heimnetz"',f(0)+'[WiFi] bekanntes Netz: "Werkstatt"',f(0)+'[Web] Webserver gestartet (Port 80)',
 f(3)+'[WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -61 dBm',f(3)+'[Web] erreichbar unter http://scope-clock.local und http://192.168.178.42',
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
...(()=>{const a=[];for(let h=40;h>=1;h--){const d=new Date(t0-h*3600000+137000);
 a.push(hms(d)+' [NTP] Sync '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
 if(h%9===0){a.push(hms(new Date(d.getTime()+600000))+' [WiFi] Verbindung verloren – versuche erneut');
  a.push(hms(new Date(d.getTime()+612000))+' [WiFi] verbunden mit "Heimnetz", IP 192.168.178.42, RSSI -63 dBm')}
 if(h===20)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: I-XII');
 if(h===19)a.push(hms(new Date(d.getTime()+900000))+' [Web] Zifferblatt: 1-12')}return a})(),
 hms(st.lastSync)+' [NTP] Sync '+pad(st.lastSync.getDate())+'.'+pad(st.lastSync.getMonth()+1)+'.'+st.lastSync.getFullYear()+' '+hms(st.lastSync));
const DN=['1-12','12/3/6/9','I-XII','XII/III/VI/IX','Bahnhof'];
function status(){const d=new Date(Date.now()+(st.off||0));const conn=!st.setup;const ls=st.lastSync;const ok=conn||!!st.manual;
 return {time:hms(d),date:ok?pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear():null,kw:ok?isoWeek(d):null,
  ntp:{synced:conn,overdue:false,manual:!conn&&!!st.manual,last:conn?pad(ls.getDate())+'.'+pad(ls.getMonth()+1)+'. '+hms(ls):null,server:'de.pool.ntp.org'},
  wifi:{configured:conn,connected:conn,ssid:conn?st.ssid:'',rssi:conn?st.rssi+Math.round(Math.random()*4-2):0,ip:conn?'192.168.178.42':'',host:'scope-clock.local'},
  ap:{active:st.setup,ssid:'ScopeClock-Setup',ip:'192.168.4.1'},profile:'DIGITAL',dial,fps:230+Math.round(Math.random()*8),
  uptime:Math.floor((Date.now()-t0)/1000)+180,heap:186000+Math.round(Math.random()*3000),fw:'2.0'}}
const J=(o)=>Promise.resolve(new Response(JSON.stringify(o),{status:200,headers:{'Content-Type':'application/json'}}));
const R=(ok,msg)=>J({ok,msg});
const realFetch=window.fetch.bind(window);
window.fetch=function(url,init){
 const u=String(url).replace(/^https?:\/\/[^/]+/,'').split('?')[0];
 if(!u.startsWith('/api/'))return realFetch(url,init);
 const p=init&&init.body?new URLSearchParams(init.body):new URLSearchParams();
 return new Promise(res=>setTimeout(res,120)).then(()=>{
  switch(u){
   case '/api/status':return J(status());
   case '/api/time':{let ms;if(p.has('epoch'))ms=+p.get('epoch')*1000;else{const m=/^(\d+)-(\d+)-(\d+)T(\d+):(\d+)(?::(\d+))?$/.exec(p.get('local')||'');
     if(!m)return R(false,'Datum oder Uhrzeit ungültig');ms=new Date(+m[1],m[2]-1,+m[3],+m[4],+m[5],+(m[6]||0)).getTime()}
    if(!(ms>=1704067200000&&ms<=4102444800000))return R(false,'Datum muss zwischen 2024 und 2099 liegen');
    st.off=ms-Date.now();st.manual=true;const d=new Date(ms);
    L('[Zeit] von Hand gestellt ('+(p.has('epoch')?'vom Handy':'Eingabe')+'): '+pad(d.getDate())+'.'+pad(d.getMonth()+1)+'.'+d.getFullYear()+' '+hms(d));
    return R(true,'Uhrzeit gestellt')}
   case '/api/log/clear':log.length=0;L('[Web] Protokoll geloescht');return R(true,'Protokoll gelöscht');
   case '/api/vectors':return J(VEC);
   case '/api/log':return Promise.resolve(new Response(log.join('\n')+'\n',{status:200,headers:{'Content-Type':'text/plain'}}));
   case '/api/networks':return J(st.setup?[]:st.nets);
   case '/api/scan':st.scanN++;if(st.scanN%2)return J({running:true});
    return J({running:false,networks:[{ssid:'Heimnetz',rssi:-52,open:false},{ssid:'Werkstatt',rssi:-66,open:false},{ssid:'Nachbar 5G',rssi:-79,open:false},{ssid:'Hotspot Café',rssi:-84,open:true}]});
   case '/api/wifi':{const s=(p.get('ssid')||'').trim(),pw=p.get('pw')||'';
    if(s.length<1||s.length>32)return R(false,'Netzwerkname: 1 bis 32 Zeichen');
    if(pw.length>0&&(pw.length<8||pw.length>63))return R(false,'Passwort: 8 bis 63 Zeichen (oder leer für offene Netze)');
    const ex=st.nets.find(n=>n.ssid===s&&n.source==='web');
    if(!ex){if(st.nets.filter(n=>n.source==='web').length>=10)return R(false,'Speicher voll – bitte zuerst ein Netz löschen');st.nets.push({ssid:s,source:'web'})}
    L('[Web] WLAN "'+s+'" '+(ex?'aktualisiert':'gespeichert'));
    if(st.setup){setTimeout(()=>{st.setup=false;st.ssid=s;L('[WiFi] verbunden mit "'+s+'", IP 192.168.178.42, RSSI -58 dBm')},3000);return R(true,'Gespeichert. Die Uhr verbindet sich jetzt …')}
    return R(true,'Gespeichert. Wird beim nächsten Verbindungsaufbau genutzt – oder „Neu verbinden“ antippen.')}
   case '/api/wifi/reset':st.nets=[];L('[Web] alle WLANs geloescht – Neustart');return R(true,'Alle Netze gelöscht. Die Uhr startet neu und öffnet das Setup-WLAN „ScopeClock-Setup“.');
   case '/api/wifi/delete':{const s=p.get('ssid');const i=st.nets.findIndex(n=>n.ssid===s&&n.source==='web');
    if(i<0)return R(false,'Dieses Netz ist nicht im Webinterface gespeichert');st.nets.splice(i,1);L('[Web] WLAN "'+s+'" geloescht – Neustart');
    return R(true,'Gelöscht. Die Uhr startet neu – Seite in ca. 10 s neu laden.')}
   case '/api/reconnect':L('[Web] Neu verbinden angefordert');setTimeout(()=>L('[WiFi] verbunden mit "'+st.ssid+'", IP 192.168.178.42, RSSI -59 dBm'),2500);
    return R(true,'Verbinde neu … die Seite ist kurz nicht erreichbar.');
   case '/api/dial':{const v=+p.get('style');if(!(v>=0&&v<5))return R(false,'style muss 0 bis 4 sein');
    if(v!==dial){dial=v;try{localStorage.setItem('demoDial',v)}catch(e){}L('[Scope] Zifferblatt: '+DN[v]+', gespeichert')}return J({ok:true,dial:v})}
  }
  return Promise.resolve(new Response('',{status:404}));
 })};
window.confirm=()=>true;   // in der Vorschau gibt es keine Bestätigungsdialoge
document.addEventListener('DOMContentLoaded',()=>{});
window.__demo=st;window.__demoLog=L;
})();
