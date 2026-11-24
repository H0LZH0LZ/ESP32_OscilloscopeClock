// Erzeugt die Dokumentation als PDF (A4) aus den HTML-Quellen in docs/.
// Aufruf: node tools/gen_pdf.js        -> deutsch und englisch
//         node tools/gen_pdf.js de|en  -> nur eine Sprache
// Benötigt Playwright mit Chromium.
const path = require('path');
const { chromium } = require('playwright');
const DOCS = {
  de: { src: 'dokumentation.html', out: 'ESP32_Oscilloscope_Clock_Dokumentation.pdf',
        title: 'ESP32 Oscilloscope Clock v2.0 – Dokumentation', page: 'Seite', of: 'von' },
  en: { src: 'documentation.en.html', out: 'ESP32_Oscilloscope_Clock_Documentation.pdf',
        title: 'ESP32 Oscilloscope Clock v2.0 – Documentation', page: 'Page', of: 'of' },
};
(async () => {
  const root = path.resolve(__dirname, '..');
  // Unterkapitel nummerieren und Inhaltsverzeichnis aktualisieren (tools/number_docs.py)
  require('child_process').execFileSync('python3', [path.join(__dirname, 'number_docs.py')], { stdio: 'inherit' });
  const langs = process.argv[2] ? [process.argv[2]] : Object.keys(DOCS);
  const b = await chromium.launch();
  for (const l of langs) {
    const d = DOCS[l];
    const p = await b.newPage();
    await p.goto('file://' + path.join(root, 'docs', d.src), { waitUntil: 'networkidle' });
    // Umbruchregeln: Absätze, Kästen, Schritte und Abbildungen nie teilen (CSS). Tabellen nur dann
    // zwischen ihren Zeilen bzw. Punkten umbrechen, wenn sie höher als 40 % einer Seite sind (Kopfzeile wird wiederholt) – sonst bleiben
    // sie zusammen. Gemessen wird bei Druckbreite (A4 abzüglich der Ränder).
    await p.emulateMedia({ media: 'print' });
    await p.setViewportSize({ width: Math.round(176 / 25.4 * 96), height: 1000 });
    await p.evaluate(() => {
      const page = 259 / 25.4 * 96;
      for (const e of document.querySelectorAll('table'))
        e.style.breakInside = e.getBoundingClientRect().height > 0.4 * page ? 'auto' : 'avoid';
      // Listen dürfen zwischen zwei Punkten umbrechen
      // (die ersten beiden und der letzte Punkt bleiben bei ihren Nachbarn, siehe CSS)
      for (const e of document.querySelectorAll('ul,ol')) e.style.breakInside = 'auto';
    });
    await p.pdf({
      path: path.join(root, 'docs', d.out),
      format: 'A4', printBackground: true, preferCSSPageSize: true,
      displayHeaderFooter: true,
      headerTemplate: '<div></div>',
      footerTemplate: '<div style="width:100%;font:8px Helvetica,Arial,sans-serif;color:#777;padding:0 17mm;display:flex;justify-content:space-between">' +
        `<span>${d.title}</span><span>${d.page} <span class="pageNumber"></span> ${d.of} <span class="totalPages"></span></span></div>`,
    });
    await p.close();
    console.log(d.out);
  }
  await b.close();
})();
