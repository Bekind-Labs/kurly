#!/usr/bin/env python3
"""Builds the Kurly landing page into site/_site/.

One template (template.html) is rendered once per language using the strings
in strings/<lang>.json:

    English  -> _site/index.html
    Japanese -> _site/ja/index.html

Placeholders look like {{key}}. String values may contain inline HTML. Every
language must define exactly the same keys, and every placeholder must be
defined, otherwise the build fails, so the translations can't drift apart.

Usage: python3 site/build.py [--out DIR]
"""
import argparse
import json
import re
import shutil
import sys
from pathlib import Path

SITE = Path(__file__).resolve().parent
ROOT = SITE.parent
SITE_URL = "https://bekind-labs.github.io/kurly/"
REPO_URL = "https://github.com/Bekind-Labs/kurly"

# Language code -> output directory, relative to the site root.
LANGUAGES = {"en": "", "ja": "ja/"}
PLACEHOLDER = re.compile(r"\{\{\s*([a-zA-Z0-9_.]+)\s*\}\}")


def load_strings():
    strings = {}
    for lang in LANGUAGES:
        with open(SITE / "strings" / f"{lang}.json", encoding="utf-8") as f:
            strings[lang] = json.load(f)
    keys = {lang: set(s) for lang, s in strings.items()}
    reference = keys["en"]
    errors = []
    for lang, k in keys.items():
        if missing := sorted(reference - k):
            errors.append(f"strings/{lang}.json is missing: {', '.join(missing)}")
        if extra := sorted(k - reference):
            errors.append(f"strings/{lang}.json has keys English doesn't: {', '.join(extra)}")
        for key, value in strings[lang].items():
            if not isinstance(value, str) or not value.strip():
                errors.append(f"strings/{lang}.json: '{key}' is empty")
    return strings, errors


def render(template, values, lang):
    missing = set()

    def substitute(match):
        key = match.group(1)
        if key not in values:
            missing.add(key)
            return match.group(0)
        return values[key]

    html = PLACEHOLDER.sub(substitute, template)
    errors = [f"{lang}: template uses undefined '{key}'" for key in sorted(missing)]
    return html, errors


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--out", type=Path, default=SITE / "_site")
    args = parser.parse_args()

    strings, errors = load_strings()
    template = (SITE / "template.html").read_text(encoding="utf-8")

    pages = {}
    for lang, subdir in LANGUAGES.items():
        prefix = "../" * subdir.count("/")  # path from the page back to the site root
        values = dict(strings[lang])
        values.update({
            "lang": lang,
            "root": prefix,
            "page_url": SITE_URL + subdir,
            "site_url": SITE_URL,
            "repo_url": REPO_URL,
            "url_en": SITE_URL + LANGUAGES["en"],
            "url_ja": SITE_URL + LANGUAGES["ja"],
            "home_path": subdir,
            "en_current": 'aria-current="page"' if lang == "en" else "",
            "ja_current": 'aria-current="page"' if lang == "ja" else "",
            # Strings the download script needs at runtime.
            "js_strings": json.dumps(
                {k[3:]: v for k, v in strings[lang].items() if k.startswith("js_")},
                ensure_ascii=False).replace("</", "<\\/"),
        })
        html, render_errors = render(template, values, lang)
        errors += render_errors
        pages[subdir + "index.html"] = html

    if errors:
        print("Site build failed:\n  " + "\n  ".join(errors), file=sys.stderr)
        return 1

    out = args.out
    if out.exists():
        shutil.rmtree(out)
    shutil.copytree(SITE / "static", out)
    assets = out / "assets"
    shutil.copy(ROOT / "icons" / "kurly.svg", assets / "kurly.svg")
    shutil.copy(ROOT / "icons" / "png" / "kurly-32.png", assets / "favicon-32.png")
    shutil.copy(ROOT / "icons" / "png" / "kurly-256.png", assets / "kurly-256.png")
    for path, html in pages.items():
        (out / path).parent.mkdir(parents=True, exist_ok=True)
        (out / path).write_text(html, encoding="utf-8")
    (out / ".nojekyll").touch()  # serve files as-is
    print(f"Built {len(pages)} pages into {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
