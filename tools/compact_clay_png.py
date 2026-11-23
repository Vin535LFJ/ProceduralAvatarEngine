#!/usr/bin/env python3
"""Losslessly recompress the grey-clay PNG evidence produced by the PoC probe.

The host PNG writer intentionally uses stored (uncompressed) DEFLATE blocks so
it needs no zlib dependency. That makes each 480x480 image ~676 KB. This
utility replaces the IDAT stream with a normally compressed one; the decoded
pixels are byte-for-byte identical, so the images remain valid evidence.

Usage: python tools/compact_clay_png.py evidence/g2/clay
"""

import binascii
import pathlib
import struct
import sys
import zlib


def chunks(data: bytes):
    pos = 8
    while pos < len(data):
        length = struct.unpack(">I", data[pos:pos + 4])[0]
        kind = data[pos + 4:pos + 8]
        payload = data[pos + 8:pos + 8 + length]
        yield kind, payload
        pos += 12 + length


def pack_chunk(kind: bytes, payload: bytes) -> bytes:
    return (struct.pack(">I", len(payload)) + kind + payload +
            struct.pack(">I", binascii.crc32(kind + payload) & 0xFFFFFFFF))


def compact(path: pathlib.Path) -> tuple[int, int]:
    raw = path.read_bytes()
    if not raw.startswith(b"\x89PNG\r\n\x1a\n"):
        return 0, 0
    parts = list(chunks(raw))
    idat = b"".join(p for k, p in parts if k == b"IDAT")
    if not idat:
        return 0, 0
    scanlines = zlib.decompress(idat)
    compressed = zlib.compress(scanlines, 9)

    out = bytearray(b"\x89PNG\r\n\x1a\n")
    emitted = False
    for kind, payload in parts:
        if kind == b"IDAT":
            # Collapse all original IDAT chunks into one replacement.
            if not emitted:
                out += pack_chunk(b"IDAT", compressed)
                emitted = True
            continue
        out += pack_chunk(kind, payload)
    if not emitted:
        raise RuntimeError(f"no IDAT emitted for {path}")

    if zlib.decompress(compressed) != scanlines:
        raise RuntimeError(f"lossless check failed for {path}")
    before = len(raw)
    path.write_bytes(bytes(out))
    return before, len(out)


def main() -> int:
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    root = pathlib.Path(sys.argv[1])
    images = sorted(root.rglob("*.png"))
    if not images:
        print(f"no PNG files under {root}")
        return 1
    before_total = after_total = 0
    for image in images:
        before, after = compact(image)
        before_total += before
        after_total += after
    print(f"{len(images)} images: {before_total / 1048576:.1f} MiB -> "
          f"{after_total / 1048576:.1f} MiB (lossless)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
