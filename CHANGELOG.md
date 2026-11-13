# Changelog

## 0.3.0

### New

- **Flatpak.** Linux users can now install Kurly as a Flatpak
  (`Kurly-0.3.0-x86_64.flatpak` on the release page). It runs on KDE's
  Flatpak runtime, which includes curl. Files you save with a relative
  output path go to your Downloads folder.
- Kurly shows up properly in Linux software centers (GNOME Software,
  Discover), with screenshots and a description in English and Japanese.

### Changed

- On Linux, Kurly's launcher entry and icon are now named
  `io.github.Bekind_Labs.Kurly` instead of `kurly`. Your saved requests and
  settings are unaffected. If you installed with `sudo cmake --install`,
  you can delete the old `/usr/local/share/applications/kurly.desktop`.
- The Arch package no longer lists `qt6-wayland` as an optional dependency;
  Qt's Wayland support is part of `qt6-base`.

### Fixed

- The AppImage now runs natively on Wayland instead of through XWayland.
- The AppImage now reads KDE Plasma 6's font settings, so it uses your
  desktop's fonts (including Japanese ones) instead of Qt's defaults.
- Buttons in the same row could have different heights, mostly in Japanese.
- Running the tests (for example while building the Arch package) no longer
  leaves a curl core dump behind.

## 0.2.0

### New

- **Library of saved requests.** Kurly now keeps your work: the current
  request is still there next time you open it. Click **Library**
  (Ctrl/⌘+B) to keep several requests in a sidebar: add, duplicate, rename,
  delete, reorder and filter them. Each request keeps its own response, and
  a running request carries on in the background while you look at another.
- When the library is open, importing a command adds it as a new request
  instead of replacing the current one.
- The "long option names" and "split across lines" choices are remembered.
- If curl fails, the status line names the curl program that ran (hover over
  it to see the full path).

### Fixed

- With **GET** selected and a request body filled in, curl actually sent a
  POST. Kurly now always sends the method the form shows.
- The "Done" and error messages were hard to read in dark mode.
- The panes now fill the window, with dividers running edge to edge.

### Website

- A new [guide](https://bekind-labs.github.io/kurly/docs/) in English and
  Japanese: what Kurly is for, an introduction to curl, and how to use every
  part of the app.
- Screenshots match your light or dark theme.

## 0.1.0

First release: build curl commands with a form, import them from your
browser, run them and read formatted JSON/XML responses, in English and
Japanese, on macOS, Windows and Linux.
