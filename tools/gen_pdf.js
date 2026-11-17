// Erzeugt docs/ESP32_Oscilloscope_Clock_Dokumentation.pdf aus docs/dokumentation.html.
// Aufruf: node tools/gen_pdf.js   (benötigt Playwright mit Chromium)
const path = require('path');
const { chromium } = require('playwright');
(async () => {
  const root = path.resolve(__dirname, '..');
  const b = await chromium.launch();
  const p = await b.newPage();
  await p.goto('file://' + path.join(root, 'docs', 'dokumentation.html'), { waitUntil: 'networkidle' });
  await p.pdf({
    path: path.join(root, 'docs', 'ESP32_Oscilloscope_Clock_Dokumentation.pdf'),
    format: 'A4', printBackground: true, preferCSSPageSize: true,
    displayHeaderFooter: true,
    headerTemplate: '<div></div>',
    footerTemplate: '<div style="width:100%;font:8px Helvetica,Arial,sans-serif;color:#777;padding:0 17mm;display:flex;justify-content:space-between">' +
      '<span>ESP32 Oscilloscope Clock v2.0 – Dokumentation</span><span>Seite <span class="pageNumber"></span> von <span class="totalPages"></span></span></div>',
  });
  await b.close();
})();
