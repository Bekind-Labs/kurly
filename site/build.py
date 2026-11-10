#!/usr/bin/env python3
"""Builds the Kurly website into site/_site/.

Every page in pages/ is rendered once per language:

    pages/index.html  ->  /            and  /ja/
    pages/docs.html   ->  /docs/       and  /ja/docs/

Templates can include shared pieces from partials/ with {{> name}}, and use
placeholders like {{key}}, filled from strings/<lang>.json plus a few values
computed here (links, the language switch, ...). String values may contain
inline HTML.

Long-form pages take their text from content/<page>.<lang>.html, inserted as
{{content}}, with a table of contents ({{toc}}) generated from its <h2>/<h3>
headings.

The build fails if the languages' strings differ, a placeholder is
undefined, the languages' content doesn't have the same sections in the same
order, or an in-page link points to a section that doesn't exist, so the
translations can't drift apart.

Usage: python3 site/build.py [--out DIR]
"""
import argparse
import html
import json
import re
import shutil
import sys
from pathlib import Path

SITE = Path(__file__).resolve().parent
ROOT = SITE.parent
SITE_URL = "https://bekind-labs.github.io/kurly/"
REPO_URL = "https://github.com/Bekind-Labs/kurly"

# Language -> directory, and page -> directory below it.
LANGUAGES = {"en": "", "ja": "ja/"}
PAGES = {"index": "", "docs": "docs/"}

PLACEHOLDER = re.compile(r"\{\{\s*([a-zA-Z0-9_.]+)\s*\}\}")
INCLUDE = re.compile(r"\{\{>\s*([a-zA-Z0-9_]+)\s*\}\}")
HEADING = re.compile(r'<h([23]) id="([^"]+)"[^>]*>(.*?)</h\1>', re.S)
ANY_ID = re.compile(r'\sid="([^"]+)"')
IN_PAGE_LINK = re.compile(r'href="#([^"]+)"')


def load_strings():
    strings = {}
    for lang in LANGUAGES:
        with open(SITE / "strings" / f"{lang}.json", encoding="utf-8") as f:
            strings[lang] = json.load(f)
    reference = set(strings["en"])
    errors = []
    for lang, values in strings.items():
        keys = set(values)
        if missing := sorted(reference - keys):
            errors.append(f"strings/{lang}.json is missing: {', '.join(missing)}")
        if extra := sorted(keys - reference):
            errors.append(f"strings/{lang}.json has keys English doesn't: {', '.join(extra)}")
        for key, value in values.items():
            if not isinstance(value, str) or not value.strip():
                errors.append(f"strings/{lang}.json: '{key}' is empty")
    return strings, errors


def expand_includes(text, seen=()):
    def include(match):
        name = match.group(1)
        if name in seen:
            raise ValueError(f"partial '{name}' includes itself")
        part = (SITE / "partials" / f"{name}.html").read_text(encoding="utf-8")
        return expand_includes(part, seen + (name,))
    return INCLUDE.sub(include, text)


def render(template, values, where):
    missing = set()

    def substitute(match):
        key = match.group(1)
        if key not in values:
            missing.add(key)
            return match.group(0)
        return values[key]

    text = PLACEHOLDER.sub(substitute, template)
    return text, [f"{where}: uses undefined '{key}'" for key in sorted(missing)]


def table_of_contents(content):
    """A nested list of links to the <h2>/<h3> sections."""
    items, open_sub = [], False
    for level, anchor, title in HEADING.findall(content):
        text = html.escape(re.sub(r"<[^>]+>", "", title).strip(), quote=False)
        link = f'<a href="#{anchor}">{text}</a>'
        if level == "2":
            if open_sub:
                items.append("</ul></li>")
                open_sub = False
            elif items:
                items.append("</li>")
            items.append(f"<li>{link}")
        else:
            if not open_sub:
                items.append("<ul>")
                open_sub = True
            items.append(f"<li>{link}</li>")
    items.append("</ul></li>" if open_sub else "</li>")
    return "<ul>" + "".join(items) + "</ul>"


def check_content(page, contents):
    """Same sections in every language; in-page links resolve."""
    errors = []
    sections = {lang: [a for _, a, _ in HEADING.findall(c)] for lang, c in contents.items()}
    for lang, ids in sections.items():
        if ids != sections["en"]:
            errors.append(f"content/{page}.{lang}.html sections differ from English:\n"
                          f"      en: {sections['en']}\n      {lang}: {ids}")
    for lang, content in contents.items():
        ids = set(ANY_ID.findall(content))
        for target in sorted(set(IN_PAGE_LINK.findall(content)) - ids):
            errors.append(f"content/{page}.{lang}.html links to missing section '#{target}'")
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--out", type=Path, default=SITE / "_site")
    args = parser.parse_args()

    strings, errors = load_strings()
    outputs = {}

    for page, page_dir in PAGES.items():
        template = expand_includes((SITE / "pages" / f"{page}.html").read_text(encoding="utf-8"))
        content_files = {lang: SITE / "content" / f"{page}.{lang}.html" for lang in LANGUAGES}
        has_content = all(f.exists() for f in content_files.values())
        if has_content:
            errors += check_content(page, {lang: f.read_text(encoding="utf-8")
                                           for lang, f in content_files.items()})

        for lang, lang_dir in LANGUAGES.items():
            path = lang_dir + page_dir
            prefix = "../" * path.count("/")  # from this page back to the site root

            def link(target):  # a relative link to another page
                return (prefix + target) or "./"

            values = dict(strings[lang])
            values.update({
                "lang": lang,
                "root": prefix,
                "site_url": SITE_URL,
                "repo_url": REPO_URL,
                "page_url": SITE_URL + path,
                "url_en": SITE_URL + LANGUAGES["en"] + page_dir,
                "url_ja": SITE_URL + LANGUAGES["ja"] + page_dir,
                # The language switch stays on the same page.
                "switch_en": link(LANGUAGES["en"] + page_dir),
                "switch_ja": link(LANGUAGES["ja"] + page_dir),
                "en_current": 'aria-current="page"' if lang == "en" else "",
                "ja_current": 'aria-current="page"' if lang == "ja" else "",
                "home": link(lang_dir),
                "docs_link": link(lang_dir + PAGES["docs"]),
                "nav_docs_current": 'aria-current="page"' if page == "docs" else "",
                # Strings the page script needs at runtime.
                "js_strings": json.dumps(
                    {k[3:]: v for k, v in strings[lang].items() if k.startswith("js_")},
                    ensure_ascii=False).replace("</", "<\\/"),
            })
            for key in ("meta_title", "meta_description"):
                page_key = f"{page}_{key}"
                if page_key in strings[lang]:
                    values[key] = strings[lang][page_key]
                else:
                    errors.append(f"strings/{lang}.json is missing '{page_key}'")
            if has_content:
                content, render_errors = render(content_files[lang].read_text(encoding="utf-8"),
                                                values, f"content/{page}.{lang}.html")
                errors += render_errors
                values["content"] = content
                values["toc"] = table_of_contents(content)
            text, render_errors = render(template, values, f"pages/{page}.html ({lang})")
            errors += render_errors
            outputs[path + "index.html"] = text

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
    for path, text in outputs.items():
        (out / path).parent.mkdir(parents=True, exist_ok=True)
        (out / path).write_text(text, encoding="utf-8")
    (out / ".nojekyll").touch()  # serve files as-is
    print(f"Built {len(outputs)} pages into {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
