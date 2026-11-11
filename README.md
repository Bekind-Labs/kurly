# kurly

A Qt-based native desktop app with a friendly UI for building cURL commands.

**Website:** <https://bekind-labs.github.io/kurly/> ([日本語](https://bekind-labs.github.io/kurly/ja/))

Fill in the form on the left (URL, method, headers, body, auth, common
options) and the matching `curl` command updates live on the right. Click
**Copy command** to put it on the clipboard, or **Run** (Ctrl/⌘+Return) to
send the request with your installed `curl` and see the output below the
command. JSON and XML responses are shown indented (toggle **Format
JSON/XML** to see the raw output). You can switch the command to long option names (`--location` vs
`-L`) or split it across multiple lines.

Already have a command? Click **Import curl command…** (Ctrl/⌘+Shift+I) and
paste it, for example from your browser's developer tools (Network tab →
right-click a request → Copy → Copy as cURL). Both the bash and Windows cmd
formats work. If a curl command is already on the clipboard, it's filled in
for you. Options Kurly can't represent are listed and skipped.

### Saving requests

Kurly keeps your work automatically: close it and the request is still there
next time. To keep several requests, click **Library** (Ctrl/⌘+B) to show
the sidebar, where you can:

- add (**New**, Ctrl/⌘+N), **Duplicate** and **Delete** requests;
- switch between them, each keeping its own response (a running request
  carries on in the background);
- rename one by double-clicking it or pressing F2 (clear the name to go back
  to the automatic "GET example.com/path" one), and drag to reorder;
- filter the list by name or URL.

When the library is showing, importing a command adds it as a new request
instead of replacing the current one.

The library is a JSON file, `library.json`, in Kurly's data folder:

| System | Location |
|---|---|
| Linux | `~/.local/share/Bekind Labs/Kurly/` |
| macOS | `~/Library/Application Support/Bekind Labs/Kurly/` |
| Windows | `%APPDATA%\Bekind Labs\Kurly\` |

Requests can contain passwords and tokens, which are stored as plain text, so
the file is readable only by your user account. If the file is ever damaged,
Kurly keeps a copy next to it (`library.json.corrupt-…`) and starts a new one.

The UI is available in **English** and **Japanese (日本語)**. It follows the
system language on first launch, and you can switch at any time from the
language selector in the top-right corner. The choice is remembered.

## Download

Get the latest version from the
[Releases page](https://github.com/Bekind-Labs/kurly/releases):

- **macOS:** open the `.dmg` and drag Kurly to Applications. If macOS says it
  can't check the app for malicious software, allow it once under
  **System Settings → Privacy & Security → Open Anyway**.
- **Windows:** unzip and run `Kurly.exe`. If SmartScreen warns, choose
  **More info → Run anyway**.
- **Linux:** make the `.AppImage` executable (`chmod +x Kurly-*.AppImage`) and
  run it. On Arch Linux you can also install it as a package; see below.

See [RELEASING.md](RELEASING.md) for how releases are made.

### Arch Linux

Kurly isn't on the AUR yet. Until it is, build and install the package from
the PKGBUILD in this repository; it builds the current release, runs the tests,
and installs Kurly through pacman with its launcher entry and icons:

```bash
git clone https://github.com/Bekind-Labs/kurly.git
```

```bash
cd kurly/packaging/arch && makepkg -si
```

To update, pull the repository and run `makepkg -si` again. To uninstall:

```bash
sudo pacman -R kurly
```

## Building

Requirements: CMake ≥ 3.21, a C++17 compiler, Qt 6.2+ with the Widgets and
LinguistTools (Qt Linguist) modules, and `curl` on your `PATH` to use **Run**.

### macOS

```bash
xcode-select --install
```

```bash
brew install qt cmake
```

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
```

```bash
cmake --build build
```

```bash
open build/kurly.app
```

### Linux

On Debian/Ubuntu:

```bash
sudo apt install cmake g++ qt6-base-dev qt6-tools-dev qt6-l10n-tools curl
```

On Arch Linux (add `qt6-wayland` for native Wayland support):

```bash
sudo pacman -S --needed base-devel cmake qt6-base qt6-tools curl
```

If CMake can't find Qt right after installing it, open a new terminal and
try again.

```bash
cmake -S . -B build
```

```bash
cmake --build build
```

```bash
./build/kurly
```

To add Kurly to your desktop's app launcher (with its icon), run
`sudo cmake --install build`.

## Tests

```bash
ctest --test-dir build --output-on-failure
```

| Test | Covers |
|---|---|
| `tst_curlcommand` | Building commands, shell quoting |
| `tst_curlparser` | Importing commands: bash/cmd quoting, browser "Copy as cURL" output, every supported option, build→parse round trips |
| `tst_responseformatter` | JSON/XML pretty-printing, `--include` header handling |
| `tst_requeststore` | The saved-request library: saving and loading, autosave, file permissions, damaged files, files from newer versions, editing |
| `tst_requestrunner` | Running curl and collecting its output, stopping, missing curl |
| `tst_importdialog` | The import dialog: clipboard pre-fill, error messages |
| `tst_library` | The library sidebar: showing it, new/duplicate/delete/rename/reorder/filter, keyboard shortcuts, import, per-request responses, persistence across restarts, file problems |
| `tst_mainwindow` | The window end to end: form ↔ command, headers, copy, language switching, import, scrolling, and **Run** against a local test server (success, HTTP errors, timeouts, stop, huge responses, formatting) |
| `tst_translations` | Every Japanese string is translated, with matching `%1` placeholders |
| `translations_up_to_date` | No UI strings were added without updating `translations/kurly_ja.ts` |

The widget tests run with Qt's `offscreen` platform, so no windows open. The
**Run** tests need `curl` installed (they're skipped otherwise) but no
internet connection.

To measure coverage (GCC/Clang), install [gcovr](https://gcovr.com) and:

```bash
cmake -S . -B build-cov -DKURLY_COVERAGE=ON && cmake --build build-cov
```

```bash
ctest --test-dir build-cov && gcovr -r . build-cov --filter src/ --exclude src/main.cpp
```

GitHub Actions (`.github/workflows/ci.yml`) builds and runs all tests on
Linux, macOS and Windows for every push and pull request, and reports
coverage.

## Translations

Source strings are English. Japanese lives in `translations/kurly_ja.ts` and is
compiled and embedded into the binary at build time. After changing UI
strings, refresh the `.ts` file and translate new entries with Qt Linguist:

```bash
cmake --build build --target update_translations
```

## Icon

`icons/kurly.svg` is the source. Running `icons/generate.py` (needs
`rsvg-convert` from librsvg) regenerates the PNGs, the macOS `kurly.icns`,
and the Windows `kurly.ico`.

## Website

The website lives in `site/` and is published to GitHub Pages by
`.github/workflows/pages.yml` whenever it changes on `main`. It has a landing
page and a guide, each in English (`/`, `/docs/`) and Japanese (`/ja/`,
`/ja/docs/`).

- `site/pages/` holds a template per page, and `site/partials/` the pieces
  they share (`<head>`, header, footer).
- Short texts come from `site/strings/en.json` and `site/strings/ja.json`.
  The guide's text is in `site/content/docs.en.html` and `docs.ja.html`; its
  table of contents is generated from the headings.
- The build fails if the two languages' strings differ, a template uses one
  that doesn't exist, the guides don't have the same sections in the same
  order, or a link points to a section that doesn't exist.
- `site/static/` holds the CSS, JavaScript, screenshots and social preview
  images. The download buttons fetch the latest release from GitHub, so the
  site doesn't need changing for new versions.

To build and preview it locally:

```bash
python3 site/build.py && python3 -m http.server -d site/_site 8000
```

Then open <http://localhost:8000>.

**Screenshots.** Every screenshot comes in a light and a dark version, and
the site shows the one matching the visitor's theme. After changing the
interface, retake them all (both themes, both languages, with and without the
library) with the screenshot tool, which drives Kurly's real window with a
stand-in curl (Linux/macOS):

```bash
cmake -S . -B build -DKURLY_BUILD_SCREENSHOT_TOOL=ON && cmake --build build --target kurly-screenshots
```

```bash
QT_QPA_PLATFORM=offscreen QT_SCALE_FACTOR=2 ./build/tools/screenshots/kurly-screenshots shots
```

Convert them into `site/static/assets/` as `screenshot-<lang>[-dark].webp`
(from `main-*`) and `library-<lang>[-dark].webp` (from `library-*`), for
example with `convert shots/main-en-dark.png -quality 88 site/static/assets/screenshot-en-dark.webp`.
Then regenerate the social preview images with
`node site/tools/make-og-images.js` (needs Playwright).

## Layout

- `src/curlcommand.*` – GUI-independent command builder and shell quoting
- `src/curlparser.*` – GUI-independent parser for importing existing curl commands
- `src/responseformatter.*` – GUI-independent JSON/XML pretty-printing
- `src/requeststore.*` – GUI-independent library of saved requests and its JSON file
- `src/requestrunner.*` – GUI-independent runner for one request's curl process
- `src/importdialog.*` – the import dialog
- `src/librarypanel.*` – the library sidebar
- `src/mainwindow.*` – the form, command/response panes, and language switching
- `packaging/` – scripts that build the release packages
- `site/` – the website (see above)
- `icons/` – app icon sources, generated platform icons, and the Linux `.desktop` file
- `tests/` – unit and widget tests (see above)
- `tools/screenshots/` – the tool that takes the website's screenshots

## License

Kurly is released under the [MIT License](LICENSE). The packaged apps also
include Qt (LGPL-3.0) and other third-party libraries; see
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
