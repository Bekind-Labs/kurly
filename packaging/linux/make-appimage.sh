#!/usr/bin/env bash
# Builds a Kurly AppImage for the current architecture.
#
#   packaging/linux/make-appimage.sh [build-dir] [output-dir]
#
# Requirements: CMake, a C++ compiler, Qt 6 (with Linguist tools and
# translations), curl, and qmake6/qmake on PATH (or QMAKE=/path/to/qmake).
# Downloads linuxdeploy and its Qt plugin into the build directory.
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD=$(realpath -m "${1:-$ROOT/build-release}")
OUT=$(realpath -m "${2:-$ROOT/dist}")
VERSION=$(sed -n 's/^project(kurly VERSION \([0-9.]*\).*/\1/p' "$ROOT/CMakeLists.txt")
ARCH=$(uname -m)

# Refuse to package a tagged commit whose version doesn't match its tag
# (e.g. a v0.2.0 tag on a commit that still says 0.1.0).
TAG=$(git -C "$ROOT" describe --tags --exact-match 2>/dev/null || true)
if [[ "$TAG" == v* && "$TAG" != "v$VERSION" ]]; then
    echo "error: this commit is tagged $TAG, but CMakeLists.txt says version $VERSION." >&2
    echo "       Check out the commit that sets the version, or fix the tag." >&2
    exit 1
fi

# linuxdeploy only publishes rolling "continuous" builds; override these to
# pin a specific copy. Their checksums are printed below for traceability.
LINUXDEPLOY_URL=${LINUXDEPLOY_URL:-https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-$ARCH.AppImage}
LINUXDEPLOY_QT_URL=${LINUXDEPLOY_QT_URL:-https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-$ARCH.AppImage}

if [[ -z "${QMAKE:-}" ]]; then
    QMAKE=$(command -v qmake6 || command -v qmake || true)
    if [[ -z "$QMAKE" && -x /usr/lib/qt6/bin/qmake ]]; then
        QMAKE=/usr/lib/qt6/bin/qmake
    fi
fi
if [[ -z "$QMAKE" ]]; then
    echo "error: qmake not found; set QMAKE=/path/to/qt6/bin/qmake" >&2
    exit 1
fi
export QMAKE

echo "==> Building Kurly $VERSION ($ARCH)"
cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release -DKURLY_BUILD_TESTS=OFF \
      -DKURLY_REQUIRE_QT_TRANSLATIONS=ON -DCMAKE_INSTALL_PREFIX=/usr
cmake --build "$BUILD" --parallel

echo "==> Staging AppDir"
APPDIR="$BUILD/AppDir"
rm -rf "$APPDIR"
DESTDIR="$APPDIR" cmake --install "$BUILD"

# appimagetool wants a PNG named after the .desktop Icon= at the top level
# (it becomes .DirIcon, the icon file managers show for the AppImage).
cp "$APPDIR/usr/share/icons/hicolor/256x256/apps/kurly.png" "$APPDIR/kurly.png"

echo "==> Fetching linuxdeploy"
TOOLS="$BUILD/tools"
mkdir -p "$TOOLS"
LINUXDEPLOY="$TOOLS/$(basename "$LINUXDEPLOY_URL")"
for url in "$LINUXDEPLOY_URL" "$LINUXDEPLOY_QT_URL"; do
    file="$TOOLS/$(basename "$url")"
    [[ -f "$file" ]] || curl -fsSL --retry 3 -o "$file" "$url"
    chmod +x "$file"
done
(cd "$TOOLS" && sha256sum ./*.AppImage)

# Bundle Qt's Wayland support too, so the AppImage runs natively on Wayland
# instead of through XWayland (blurry with fractional scaling). The plugin's
# file names differ between Qt versions, so take whatever this Qt has.
QT_PLATFORMS="$("$QMAKE" -query QT_INSTALL_PLUGINS)/platforms"
WAYLAND_PLUGINS=$(cd "$QT_PLATFORMS" 2>/dev/null && find . -maxdepth 1 -name 'libqwayland*.so' -printf '%f;' | sed 's/;$//')
if [[ -n "$WAYLAND_PLUGINS" ]]; then
    echo "==> Including Wayland support: $WAYLAND_PLUGINS"
    export EXTRA_PLATFORM_PLUGINS="$WAYLAND_PLUGINS"
else
    echo "warning: this Qt has no Wayland plugin in $QT_PLATFORMS; the AppImage will use X11 only" >&2
fi

echo "==> Bundling"
mkdir -p "$OUT"
export APPIMAGE_EXTRACT_AND_RUN=1 # works without FUSE (e.g. in CI containers)
export PATH="$TOOLS:$PATH"        # lets linuxdeploy find the Qt plugin
export LINUXDEPLOY_OUTPUT_VERSION="$VERSION"
(cd "$OUT" && "$LINUXDEPLOY" --appdir "$APPDIR" \
    --desktop-file "$APPDIR/usr/share/applications/kurly.desktop" \
    --plugin qt --output appimage)

APPIMAGE="$OUT/Kurly-$VERSION-$ARCH.AppImage"
if [[ ! -f "$APPIMAGE" ]]; then
    # linuxdeploy names the file after the .desktop Name; normalise it.
    mv "$OUT"/Kurly*"$ARCH".AppImage "$APPIMAGE"
fi
echo "==> Bundled display plugins:"
find "$APPDIR/usr/plugins" -maxdepth 2 \( -path '*/platforms/*' -o -path '*/wayland-*' \) | sed "s|$APPDIR/usr/plugins/|    |" | sort
echo "==> Built $APPIMAGE"
