// Demo: Setup-WLAN-Zustand umschalten
document.getElementById('demoSetup').onclick=function(){const st=window.__demo;st.setup=!st.setup;
 this.textContent=st.setup?'Normalbetrieb simulieren':'Setup-WLAN simulieren';
 window.__demoLog(st.setup?'[Web] Setup-WLAN "ScopeClock-Setup" aktiv – Einrichtung unter http://192.168.4.1':'[WiFi] verbunden mit "'+st.ssid+'", IP 192.168.178.42, RSSI -61 dBm');
 status();saved();log();};
