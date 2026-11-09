# kurly

A Qt-based native desktop app with a friendly UI for building cURL commands.

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
  run it.

See [RELEASING.md](RELEASING.md) for how releases are made.

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
| `tst_importdialog` | The import dialog: clipboard pre-fill, error messages |
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

## Layout

- `src/curlcommand.*` – GUI-independent command builder and shell quoting
- `src/curlparser.*` – GUI-independent parser for importing existing curl commands
- `src/responseformatter.*` – GUI-independent JSON/XML pretty-printing
- `src/importdialog.*` – the import dialog
- `src/mainwindow.*` – the form, command/response panes, running curl, and language switching
- `packaging/` – scripts that build the release packages
- `icons/` – app icon sources, generated platform icons, and the Linux `.desktop` file
- `tests/` – unit and widget tests (see above)
