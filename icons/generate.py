#!/usr/bin/env python3
"""Regenerate the app icons from kurly.svg.

Requires `rsvg-convert` (librsvg). Writes:
  png/kurly-<size>.png   window icon (embedded via Qt resources) + Linux hicolor icons
  kurly.icns             macOS app bundle icon
  kurly.ico              Windows executable icon
"""
import os
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SVG = os.path.join(HERE, "kurly.svg")
SIZES = [16, 32, 48, 64, 128, 256, 512, 1024]


def render(size):
    return subprocess.run(
        ["rsvg-convert", "-w", str(size), "-h", str(size), SVG],
        check=True, capture_output=True).stdout


def main():
    pngs = {s: render(s) for s in SIZES}

    os.makedirs(os.path.join(HERE, "png"), exist_ok=True)
    for s, data in pngs.items():
        if s > 512:
            continue  # only needed inside the .icns
        with open(os.path.join(HERE, "png", f"kurly-{s}.png"), "wb") as f:
            f.write(data)

    # ICNS: a header followed by (type, length, PNG data) chunks.
    icns_types = [("icp4", 16), ("icp5", 32), ("icp6", 64), ("ic07", 128),
                  ("ic08", 256), ("ic09", 512), ("ic10", 1024),
                  ("ic11", 32), ("ic12", 64), ("ic13", 256), ("ic14", 512)]
    body = b"".join(t.encode() + struct.pack(">I", 8 + len(pngs[s])) + pngs[s]
                    for t, s in icns_types)
    with open(os.path.join(HERE, "kurly.icns"), "wb") as f:
        f.write(b"icns" + struct.pack(">I", 8 + len(body)) + body)

    # ICO: directory of PNG-compressed images (supported since Windows Vista).
    ico_sizes = [16, 32, 48, 64, 128, 256]
    header = struct.pack("<HHH", 0, 1, len(ico_sizes))
    offset = len(header) + 16 * len(ico_sizes)
    entries, blobs = b"", b""
    for s in ico_sizes:
        data = pngs[s]
        dim = 0 if s >= 256 else s  # 0 means 256
        entries += struct.pack("<BBBBHHII", dim, dim, 0, 0, 1, 32, len(data), offset)
        blobs += data
        offset += len(data)
    with open(os.path.join(HERE, "kurly.ico"), "wb") as f:
        f.write(header + entries + blobs)

    print("Icons written to", HERE)


if __name__ == "__main__":
    sys.exit(main())
