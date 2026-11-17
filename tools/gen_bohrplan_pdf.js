// Erzeugt docs/Bohrplan_A4.pdf (DIN A4, Maßstab 1:1) aus docs/images/bohrplan.svg.
// Aufruf: node tools/gen_bohrplan_pdf.js   (benötigt Playwright mit Chromium)
const path = require('path'); const fs = require('fs'); const { chromium } = require('playwright');
(async () => {
  const docs = path.resolve(__dirname, '..', 'docs');
  const svg = fs.readFileSync(path.join(docs, 'images', 'bohrplan.svg'), 'utf8');
  const b = await chromium.launch(); const p = await b.newPage();
  await p.setContent(`<html><head><style>@page{size:A4;margin:0}html,body{margin:0}svg{display:block;width:210mm;height:297mm}</style></head><body>${svg}</body></html>`);
  await p.pdf({ path: path.join(docs, 'Bohrplan_A4.pdf'), format: 'A4', printBackground: true, preferCSSPageSize: true });
  await b.close();
})();
