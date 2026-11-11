# Changelog

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
