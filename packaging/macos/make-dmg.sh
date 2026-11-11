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

echo "==> Building Kurly $VERSION ($ARCH_NAME) with Qt from $QT_DIR"
cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release -DKURLY_BUILD_TESTS=OFF \
      -DKURLY_REQUIRE_QT_TRANSLATIONS=ON -DCMAKE_PREFIX_PATH="$QT_DIR" \
      -DCMAKE_OSX_ARCHITECTURES="$ARCHS" -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
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
if [[ -n "$IDENTITY" ]]; then
    "$QT_DIR/bin/macdeployqt" "$STAGE/Kurly.app" -always-overwrite \
        -codesign="$IDENTITY" -hardened-runtime -timestamp
else
    "$QT_DIR/bin/macdeployqt" "$STAGE/Kurly.app" -always-overwrite
    # Apple Silicon refuses to run unsigned code; an ad-hoc signature is enough
    # for locally built or unnotarized apps.
    codesign --force --deep --sign - "$STAGE/Kurly.app"
fi
codesign --verify --deep --strict "$STAGE/Kurly.app"

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
