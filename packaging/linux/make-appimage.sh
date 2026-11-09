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

echo "==> Bundling"
mkdir -p "$OUT"
export APPIMAGE_EXTRACT_AND_RUN=1 # works without FUSE (e.g. in CI containers)
export PATH="$TOOLS:$PATH"        # lets linuxdeploy find the Qt plugin
export LINUXDEPLOY_OUTPUT_VERSION="$VERSION"
(cd "$OUT" && "$LINUXDEPLOY" --appdir "$APPDIR" --plugin qt --output appimage)

APPIMAGE="$OUT/Kurly-$VERSION-$ARCH.AppImage"
if [[ ! -f "$APPIMAGE" ]]; then
    # linuxdeploy names the file after the .desktop Name; normalise it.
    mv "$OUT"/Kurly*"$ARCH".AppImage "$APPIMAGE"
fi
echo "==> Built $APPIMAGE"
