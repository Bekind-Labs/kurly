# kurly

A Qt-based native desktop app with a friendly UI for building cURL commands.

Fill in the form on the left (URL, method, headers, body, auth, common
options) and the matching `curl` command updates live on the right. Click
**Copy command** to put it on the clipboard. You can switch the output to long
option names (`--location` vs `-L`) or split it across multiple lines.

The UI is available in **English** and **Japanese (日本語)**. It follows the
system language on first launch, and you can switch at any time from the
language selector in the top-right corner. The choice is remembered.

## Building

Requirements: CMake ≥ 3.21, a C++17 compiler, and Qt 6.2+ with the Widgets
and LinguistTools (Qt Linguist) modules.

```bash
cmake -S . -B build
```

```bash
cmake --build build
```

```bash
./build/kurly
```

On Debian/Ubuntu the dependencies are `qt6-base-dev qt6-tools-dev qt6-l10n-tools`.

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

## Layout

- `src/curlcommand.*` – GUI-independent command builder and shell quoting
- `src/mainwindow.*` – the form, result pane, and language switching
- `tests/` – unit tests for the command builder
