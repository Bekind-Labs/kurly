# kurly

A Qt-based native desktop app with a friendly UI for building cURL commands.

Fill in the form on the left (URL, method, headers, body, auth, common
options) and the matching `curl` command updates live on the right. Click
**Copy command** to put it on the clipboard, or **Run** (Ctrl/⌘+Return) to
send the request with your installed `curl` and see the output below the
command. You can switch the command to long option names (`--location` vs
`-L`) or split it across multiple lines.

The UI is available in **English** and **Japanese (日本語)**. It follows the
system language on first launch, and you can switch at any time from the
language selector in the top-right corner. The choice is remembered.

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
ctest --test-dir build
```

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
- `src/mainwindow.*` – the form, command/response panes, running curl, and language switching
- `icons/` – app icon sources, generated platform icons, and the Linux `.desktop` file
- `tests/` – unit tests for the command builder
