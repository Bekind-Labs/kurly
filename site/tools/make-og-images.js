// Renders the social preview cards (static/assets/og-<lang>.png, 1200×630)
// from the page's own strings, the app icon and the screenshots.
//
//   node site/tools/make-og-images.js
//
// Needs Playwright with Chromium (npm i -D playwright && npx playwright install chromium).
// Re-run it after changing hero_title/hero_eyebrow or the screenshots.
const fs = require("fs");
const path = require("path");
const { chromium } = require("playwright");

const site = path.resolve(__dirname, "..");
const icon = fs.readFileSync(path.join(site, "..", "icons", "kurly.svg"), "utf8");

function card(strings, lang) {
  const shot = "data:image/webp;base64," +
    fs.readFileSync(path.join(site, "static", "assets", `screenshot-${lang}.webp`)).toString("base64");
  const text = (html) => html.replace(/<[^>]+>/g, "");
  return `<!doctype html><html lang="${lang}"><meta charset="utf-8"><style>
    * { margin: 0; box-sizing: border-box; }
    body { width: 1200px; height: 630px; overflow: hidden; position: relative;
      font-family: -apple-system, "Segoe UI", "Hiragino Sans", "Noto Sans CJK JP", "Noto Sans JP", sans-serif;
      background: radial-gradient(circle at 15% 20%, #3a2a8f 0, transparent 55%),
                  radial-gradient(circle at 85% 90%, #0d6c78 0, transparent 50%), #110f1c; color: #fff; }
    .copy { position: absolute; left: 72px; top: 0; bottom: 0; width: 560px;
      display: flex; flex-direction: column; justify-content: center; gap: 24px; }
    .brand { display: flex; align-items: center; gap: 18px; font-size: 44px; font-weight: 800; }
    .brand svg { width: 84px; height: 84px; }
    h1 { font-size: ${lang === "ja" ? 50 : 56}px; line-height: 1.25; font-weight: 800;
      letter-spacing: ${lang === "ja" ? 0 : -1}px; word-break: auto-phrase; }
    p { font-size: 24px; color: #c9c4e6; line-height: 1.5; }
    img { position: absolute; left: 660px; top: 90px; width: 760px; border-radius: 14px;
      border: 1px solid #3a3550; box-shadow: 0 30px 80px rgb(0 0 0 / 55%); }
  </style><body>
    <div class="copy">
      <div class="brand">${icon}<span>Kurly</span></div>
      <h1>${text(strings.hero_title)}</h1>
      <p>${text(strings.hero_eyebrow)}</p>
    </div>
    <img src="${shot}" alt="">
  </body></html>`;
}

(async () => {
  const browser = await chromium.launch();
  const page = await browser.newPage({ viewport: { width: 1200, height: 630 } });
  for (const lang of ["en", "ja"]) {
    const strings = JSON.parse(fs.readFileSync(path.join(site, "strings", `${lang}.json`), "utf8"));
    await page.setContent(card(strings, lang), { waitUntil: "load" });
    const out = path.join(site, "static", "assets", `og-${lang}.png`);
    await page.screenshot({ path: out });
    console.log("wrote", path.relative(process.cwd(), out));
  }
  await browser.close();
})();
