# Third-party notices

Kurly itself is licensed under the MIT License (see `LICENSE`). The
distributed packages also include the following third-party software.

## Qt

Kurly uses the [Qt framework](https://www.qt.io/) (Qt Core, Qt GUI, Qt
Widgets, Qt SVG and their plugins), copyright The Qt Company Ltd.
and other contributors, under the GNU Lesser General Public License version 3
(`licenses/LGPL-3.0.txt`, which builds on `licenses/GPL-3.0.txt`).

Qt is linked dynamically: the Qt libraries in the package are separate files
that you may replace with your own builds of the same Qt version. The source
code of Qt is available from <https://download.qt.io/official_releases/qt/>
and <https://code.qt.io/>. The packages are built with the Qt version noted
in `.github/workflows/release.yml` (Linux packages built from source use your
distribution's Qt).

Qt includes further third-party components under their own (permissive)
licenses; see <https://doc.qt.io/qt-6/licenses-used-in-qt.html>.

## Linux AppImage system libraries

The AppImage also bundles libraries from Ubuntu 22.04 that Qt depends on (for
example ICU, GLib, libxkbcommon, libpng, zstd, Brotli and D-Bus), each under its
own license. Their licenses and source code are available from
<https://packages.ubuntu.com/jammy/>.

## Microsoft Visual C++ runtime (Windows)

The Windows package includes the Microsoft Visual C++ runtime DLLs
(`msvcp140*.dll`, `vcruntime140*.dll`, ...), redistributed as permitted by the
Visual Studio license terms for redistributable code.
