#!/usr/bin/env bash
# Builds Kurly.app and packs it into a .dmg.
#
#   packaging/macos/make-dmg.sh [build-dir] [output-dir]
#
# Environment:
#   QT_DIR              Qt prefix. Defaults to $QT_ROOT_DIR (set by CI) or
#                       `brew --prefix qt`.
#   KURLY_UNIVERSAL=1   Build for both Apple Silicon and Intel. Needs a
#                       universal Qt (the official Qt installer's; Homebrew's
#                       is single-architecture).
#
# Optional signing (otherwise the app is ad-hoc signed and macOS will ask
# users to approve it in System Settings → Privacy & Security on first open):
#   MACOS_SIGN_IDENTITY "Developer ID Application: Your Name (TEAMID)", from a
#                       certificate in the keychain.
#   APPLE_ID, APPLE_TEAM_ID, APPLE_APP_PASSWORD
#                       Also notarize the .dmg (APPLE_APP_PASSWORD is an
#                       app-specific password from appleid.apple.com).
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD="${1:-$ROOT/build-release}"
OUT="${2:-$ROOT/dist}"
VERSION=$(sed -n 's/^project(kurly VERSION \([0-9.]*\).*/\1/p' "$ROOT/CMakeLists.txt")

# Refuse to package a tagged commit whose version doesn't match its tag
# (e.g. a v0.2.0 tag on a commit that still says 0.1.0).
TAG=$(git -C "$ROOT" describe --tags --exact-match 2>/dev/null || true)
if [[ "$TAG" == v* && "$TAG" != "v$VERSION" ]]; then
    echo "error: this commit is tagged $TAG, but CMakeLists.txt says version $VERSION." >&2
    echo "       Check out the commit that sets the version, or fix the tag." >&2
    exit 1
fi

QT_DIR=${QT_DIR:-${QT_ROOT_DIR:-$(brew --prefix qt 2>/dev/null || true)}}
if [[ -z "$QT_DIR" || ! -x "$QT_DIR/bin/macdeployqt" ]]; then
    echo "error: Qt not found; set QT_DIR to your Qt prefix (the folder containing bin/macdeployqt)" >&2
    exit 1
fi

if [[ "${KURLY_UNIVERSAL:-}" == 1 ]]; then
    ARCHS="arm64;x86_64"
    ARCH_NAME=universal
else
    ARCHS=$(uname -m)
    ARCH_NAME=$ARCHS
fi
IDENTITY=${MACOS_SIGN_IDENTITY:-}

# The oldest macOS the app runs on: 12, or newer if this Qt needs it
# (Homebrew's Qt is built for the macOS it was bottled on).
min_macos() { # prints the minimum macOS a Mach-O binary was built for
    otool -l "$1" 2>/dev/null | awk '
        /LC_BUILD_VERSION/ { b = 1 } b && $1 == "minos" { print $2; exit }
        /LC_VERSION_MIN_MACOSX/ { v = 1 } v && $1 == "version" { print $2; exit }'
}
QT_LIBS=$("$QT_DIR/bin/qmake" -query QT_INSTALL_LIBS 2>/dev/null || echo "$QT_DIR/lib")
QT_MIN=$(min_macos "$QT_LIBS/QtCore.framework/QtCore")
MACOS_MIN=12.0
if [[ -n "$QT_MIN" ]] && [[ "$(printf '%s\n' "$MACOS_MIN" "$QT_MIN" | sort -V | tail -1)" == "$QT_MIN" ]]; then
    MACOS_MIN=$QT_MIN
fi

echo "==> Building Kurly $VERSION ($ARCH_NAME, macOS $MACOS_MIN or later) with Qt from $QT_DIR"
cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release -DKURLY_BUILD_TESTS=OFF \
      -DKURLY_REQUIRE_QT_TRANSLATIONS=ON -DCMAKE_PREFIX_PATH="$QT_DIR" \
      -DCMAKE_OSX_ARCHITECTURES="$ARCHS" -DCMAKE_OSX_DEPLOYMENT_TARGET="$MACOS_MIN"
cmake --build "$BUILD" --parallel

echo "==> Bundling Qt into the app"
STAGE="$BUILD/dmg"
rm -rf "$STAGE"
mkdir -p "$STAGE" "$OUT"
cp -R "$BUILD/kurly.app" "$STAGE/Kurly.app"
# License notices travel inside the app (signed with it) and sit in the dmg.
mkdir -p "$STAGE/Licenses"
for dest in "$STAGE/Kurly.app/Contents/Resources" "$STAGE/Licenses"; do
    cp "$ROOT/LICENSE" "$ROOT/THIRD_PARTY_NOTICES.md" "$ROOT"/licenses/*.txt "$dest/"
done
APP="$STAGE/Kurly.app"
# Homebrew's `qt` includes every Qt module, so macdeployqt also copies plugins
# Kurly doesn't use (PDF and SVG images, the virtual keyboard) whose libraries
# live elsewhere and can't be bundled. Hide its complaints about those; the
# check below removes such plugins and fails if anything else is missing.
deployqt() {
    # (pipefail still stops the script if macdeployqt itself fails)
    "$QT_DIR/bin/macdeployqt" "$APP" -always-overwrite "$@" 2>&1 \
        | { grep -v -e 'Cannot resolve rpath' -e '^ERROR:  using QList' || true; }
}
if [[ -n "$IDENTITY" ]]; then
    deployqt -codesign="$IDENTITY" -hardened-runtime -timestamp
else
    deployqt
fi

# Every @rpath library a bundled binary uses must be in Contents/Frameworks.
missing_libs() {
    otool -L "$1" | tail -n +2 | awk '$1 ~ /^@rpath\// { print $1 }' | while read -r dep; do
        [[ -e "$APP/Contents/Frameworks/${dep#@rpath/}" ]] || echo "${dep#@rpath/}"
    done
}
while IFS= read -r -d '' plugin; do
    missing=$(missing_libs "$plugin")
    if [[ -n "$missing" ]]; then
        echo "    leaving out ${plugin#"$APP/Contents/PlugIns/"} (needs $(echo "$missing" | sed 's|\.framework/.*||' | sort -u | paste -sd, -))"
        rm "$plugin"
    fi
done < <(find "$APP/Contents/PlugIns" -name '*.dylib' -print0)
broken=0
while IFS= read -r -d '' bin; do
    file "$bin" | grep -q 'Mach-O' || continue
    missing=$(missing_libs "$bin")
    if [[ -n "$missing" ]]; then
        echo "error: ${bin#"$STAGE/"} needs libraries that weren't bundled: $missing" >&2
        broken=1
    fi
done < <(find "$APP/Contents/MacOS" "$APP/Contents/Frameworks" -type f -print0)
[[ $broken == 0 ]] || exit 1

# Sign again now the bundle's contents are final. Apple Silicon refuses to run
# unsigned code; an ad-hoc signature is enough for local or unnotarized apps.
if [[ -n "$IDENTITY" ]]; then
    codesign --force --options runtime --timestamp --sign "$IDENTITY" "$APP"
else
    codesign --force --deep --sign - "$APP"
fi
codesign --verify --deep --strict "$APP"

echo "==> Creating disk image"
ln -s /Applications "$STAGE/Applications"
DMG="$OUT/Kurly-$VERSION-macOS-$ARCH_NAME.dmg"
rm -f "$DMG"
hdiutil create -volname "Kurly $VERSION" -srcfolder "$STAGE" -ov -format UDZO "$DMG"

if [[ -n "$IDENTITY" ]]; then
    codesign --sign "$IDENTITY" --timestamp "$DMG"
    if [[ -n "${APPLE_ID:-}" && -n "${APPLE_TEAM_ID:-}" && -n "${APPLE_APP_PASSWORD:-}" ]]; then
        echo "==> Notarizing (this can take a few minutes)"
        xcrun notarytool submit "$DMG" --apple-id "$APPLE_ID" --team-id "$APPLE_TEAM_ID" \
            --password "$APPLE_APP_PASSWORD" --wait
        xcrun stapler staple "$DMG"
    else
        echo "note: signed but not notarized (APPLE_ID/APPLE_TEAM_ID/APPLE_APP_PASSWORD not set)"
    fi
fi
echo "==> Built $DMG"
