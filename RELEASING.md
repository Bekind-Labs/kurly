# Releasing Kurly

## Packages

| Platform | File | Built by |
|---|---|---|
| Linux (x86_64) | `Kurly-<version>-x86_64.AppImage` | `packaging/linux/make-appimage.sh` |
| Windows (x64) | `Kurly-<version>-windows-x64.zip` (portable, no installer) | `packaging/windows/make-zip.ps1` |
| macOS (Apple Silicon + Intel) | `Kurly-<version>-macOS-universal.dmg` | `packaging/macos/make-dmg.sh` |

Each package bundles Qt, Kurly's translations and Qt's own Japanese
translation, so users don't need Qt installed. Kurly uses the system's
`curl` to run requests, which macOS, Windows 10 1803+ and practically every
Linux distribution already have.

The AppImage is built on Ubuntu 22.04, so it runs on distributions at least
that new (glibc 2.35+).

## Making a release

1. Update `project(kurly VERSION x.y.z)` in `CMakeLists.txt`, commit, and merge
   to `main`.
2. Tag the commit and push the tag:

   ```bash
   git tag v0.1.0 && git push origin v0.1.0
   ```

3. The **Release** workflow builds and smoke-tests every package, then creates
   a **draft** release with them and a `SHA256SUMS.txt`. The tag must match
   the version in `CMakeLists.txt`, or the workflow stops.
4. Review the draft on GitHub (edit the generated notes if you like) and
   publish it.

The workflow also runs, without releasing anything, when packaging files
change or when started by hand from the Actions tab. Its packages are then
available as workflow artifacts.

### If the macOS job can't run

The draft release is still created when only the macOS job fails or is
cancelled (for example when the organization can't use macOS runners). Build
the dmg on a Mac and attach it to the draft by hand:

```bash
KURLY_UNIVERSAL=1 QT_DIR=~/Qt/6.8.3/macos packaging/macos/make-dmg.sh
```

`KURLY_UNIVERSAL=1` needs a universal Qt from the official Qt installer. With
Homebrew's Qt, leave it out to build for your Mac's architecture only:

```bash
packaging/macos/make-dmg.sh
```

Then add the dmg's line to `SHA256SUMS.txt` (`shasum -a 256 dist/*.dmg`).

## macOS signing and notarization

Without signing, the app is ad-hoc signed. The first time users open it,
macOS blocks it and they have to allow it under **System Settings → Privacy &
Security → Open Anyway**. To avoid that, sign and notarize with an Apple
Developer account by adding these repository secrets (Settings → Secrets and
variables → Actions):

| Secret | Value |
|---|---|
| `MACOS_CERTIFICATE` | Your "Developer ID Application" certificate exported as `.p12`, base64-encoded (`base64 -i cert.p12`) |
| `MACOS_CERTIFICATE_PASSWORD` | The `.p12` export password |
| `MACOS_SIGN_IDENTITY` | The identity name, e.g. `Developer ID Application: Bekind Labs (TEAMID)` |
| `APPLE_ID` | The Apple ID used for notarization |
| `APPLE_TEAM_ID` | Your team ID |
| `APPLE_APP_PASSWORD` | An app-specific password from appleid.apple.com |

The same values work as environment variables for a local
`packaging/macos/make-dmg.sh` run (the certificate must be in your keychain).

Windows builds are unsigned, so SmartScreen may warn on first run ("More info →
Run anyway") until the app builds up reputation or is signed.
