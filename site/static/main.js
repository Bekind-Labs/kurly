// Kurly landing page: language preference, OS detection and links to the
// latest release's files. The page works without this script; it only
// makes the download buttons more specific.
(function () {
  "use strict";

  var REPO = "Bekind-Labs/kurly";
  var LANG_KEY = "kurly-lang";
  var RELEASE_KEY = "kurly-latest-release";

  var strings = {};
  try {
    strings = JSON.parse(document.getElementById("strings").textContent);
  } catch (e) { /* keep the static text */ }

  function t(key, values) {
    var text = strings[key] || "";
    Object.keys(values || {}).forEach(function (name) {
      text = text.split("{" + name + "}").join(values[name]);
    });
    return text;
  }

  function storageGet(store, key) {
    try { return window[store].getItem(key); } catch (e) { return null; }
  }
  function storageSet(store, key, value) {
    try { window[store].setItem(key, value); } catch (e) { /* private mode etc. */ }
  }

  // ---- Language ------------------------------------------------------------

  var pageLang = document.documentElement.lang;
  var chosen = storageGet("localStorage", LANG_KEY);
  if (!chosen && pageLang === "en") {
    var preferred = (navigator.languages && navigator.languages[0]) || navigator.language || "";
    if (/^ja\b/i.test(preferred)) {
      var japanese = document.querySelector('.lang-switch a[data-lang="ja"]');
      if (japanese) {
        window.location.replace(japanese.href + window.location.hash);
        return;
      }
    }
  }

  document.addEventListener("DOMContentLoaded", function () {
    document.querySelectorAll(".lang-switch a").forEach(function (link) {
      link.addEventListener("click", function () {
        storageSet("localStorage", LANG_KEY, link.dataset.lang);
      });
    });
    setUpDownloads();
    setUpTableOfContents();
  });

  // ---- Guide: table of contents ------------------------------------------------

  function setUpTableOfContents() {
    var toc = document.querySelector(".toc");
    if (!toc) return;
    var details = toc.querySelector("details");
    // On narrow screens the contents start collapsed above the article.
    if (details && window.matchMedia && window.matchMedia("(max-width: 900px)").matches)
      details.open = false;
    var links = {};
    toc.querySelectorAll("a[href^='#']").forEach(function (a) {
      links[a.getAttribute("href").slice(1)] = a;
    });
    var headings = Array.prototype.filter.call(
      document.querySelectorAll(".prose h2[id], .prose h3[id]"),
      function (h) { return links[h.id]; });
    // Highlight the last heading that has scrolled past the top area.
    function update() {
      var current = headings[0];
      headings.forEach(function (h) {
        if (h.getBoundingClientRect().top < 140) current = h;
      });
      // At the very bottom, the last sections may never reach the top.
      var root = document.documentElement;
      if (window.innerHeight + window.scrollY >= root.scrollHeight - 2)
        current = headings[headings.length - 1];
      Object.keys(links).forEach(function (id) {
        if (current && id === current.id) links[id].setAttribute("aria-current", "true");
        else links[id].removeAttribute("aria-current");
      });
    }
    var pending = false;
    window.addEventListener("scroll", function () {
      if (pending) return;
      pending = true;
      window.requestAnimationFrame(function () { pending = false; update(); });
    }, { passive: true });
    update();
  }

  // ---- Downloads -----------------------------------------------------------

  function detectOs() {
    var platform = (navigator.userAgentData && navigator.userAgentData.platform) ||
                   navigator.platform || "";
    var ua = navigator.userAgent || "";
    if (/android|iphone|ipad|ipod/i.test(ua)) return null; // no mobile version
    if (/mac/i.test(platform) || /Macintosh/.test(ua)) return "mac";
    if (/win/i.test(platform) || /Windows/.test(ua)) return "windows";
    if (/linux/i.test(platform) || /Linux|X11/.test(ua)) return "linux";
    return null;
  }

  var PATTERNS = {
    mac: /macOS-(universal|arm64|x86_64)\.dmg$/,
    windows: /windows-x64\.zip$/,
    linux: /x86_64\.flatpak$/,
    linux_appimage: /x86_64\.AppImage$/
  };

  function setUpDownloads() {
    var os = detectOs();
    var hero = document.getElementById("hero-download");
    if (os) {
      var card = document.querySelector('.download-card[data-os="' + os + '"]');
      if (card) card.classList.add("is-recommended");
      if (hero) hero.textContent = t("download_for", { os: t("os_" + os) });
    }

    loadLatestRelease(function (release) {
      if (!release) return;
      var version = String(release.tag_name || "").replace(/^v/, "");
      var assets = release.assets || [];
      var urls = {};
      Object.keys(PATTERNS).forEach(function (key) {
        var asset = assets.filter(function (a) { return PATTERNS[key].test(a.name); })[0];
        if (!asset) return;
        urls[key] = asset.browser_download_url;
        var button = document.getElementById("download-" + key);
        if (button) {
          button.href = asset.browser_download_url;
          button.setAttribute("download", "");
        }
        if (key === "mac") {
          var arch = PATTERNS.mac.exec(asset.name)[1];
          var req = document.getElementById("mac-req");
          var label = { universal: "mac_universal", arm64: "mac_arm", x86_64: "mac_intel" }[arch];
          if (req && label) req.textContent = t(label);
        }
      });
      var sums = assets.filter(function (a) { return /SHA256SUMS/.test(a.name); })[0];
      var sumsLink = document.getElementById("download-checksums");
      if (sums && sumsLink) sumsLink.href = sums.browser_download_url;
      if (version) {
        var meta = document.getElementById("hero-meta");
        var lead = document.getElementById("download-version");
        if (meta) meta.textContent = t("version_meta", { version: version });
        if (lead) lead.textContent = t("download_lead", { version: version });
      }
      if (hero && os && urls[os]) {
        hero.href = urls[os];
        hero.setAttribute("download", "");
      }
    });
  }

  function loadLatestRelease(done) {
    var cached = storageGet("sessionStorage", RELEASE_KEY);
    if (cached) {
      try { return done(JSON.parse(cached)); } catch (e) { /* refetch */ }
    }
    if (!window.fetch) return done(null);
    fetch("https://api.github.com/repos/" + REPO + "/releases/latest", {
      headers: { Accept: "application/vnd.github+json" }
    })
      .then(function (r) { return r.ok ? r.json() : null; })
      .then(function (release) {
        if (release) {
          // Keep only what the page uses, to stay small in storage.
          var slim = {
            tag_name: release.tag_name,
            assets: (release.assets || []).map(function (a) {
              return { name: a.name, browser_download_url: a.browser_download_url };
            })
          };
          storageSet("sessionStorage", RELEASE_KEY, JSON.stringify(slim));
          done(slim);
        } else {
          done(null);
        }
      })
      .catch(function () { done(null); });
  }
})();
