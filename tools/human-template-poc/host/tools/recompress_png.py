"""Re-compress the evidence PNGs written by PAE's ImageWriter.

PAE's dependency-free writer stores DEFLATE "stored" blocks (no compression),
which is fine for a handful of PoC images but too large for a evidence folder
of ~80 images. This host-only tooling re-encodes them with zlib level 9 and
verifies pixel-exact round trips. It uses only the Python standard library and
is NOT part of the engine or of any runtime.

usage: python recompress_png.py <src-dir> <dst-dir>
"""

import hashlib
import os
import struct
import sys
import zlib


def read_png(path):
    data = open(path, "rb").read()
    assert data[:8] == b"\x89PNG\r\n\x1a\n", path
    pos, idat, ihdr = 8, [], None
    while pos < len(data):
        length, ctype = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + length]
        if ctype == b"IHDR":
            ihdr = struct.unpack(">IIBBBBB", body)
        elif ctype == b"IDAT":
            idat.append(body)
        pos += 12 + length
    width, height, depth, ctype_, comp, filt, interlace = ihdr
    assert (depth, ctype_, comp, filt, interlace) == (8, 2, 0, 0, 0), ihdr
    raw = zlib.decompress(b"".join(idat))
    stride = width * 3 + 1
    assert len(raw) == stride * height
    rows = []
    for y in range(height):
        row = raw[y * stride:(y + 1) * stride]
        assert row[0] == 0, "only filter type 0 is expected from PAE's writer"
        rows.append(row[1:])
    return width, height, b"".join(rows)


def chunk(ctype, body):
    return struct.pack(">I", len(body)) + ctype + body + struct.pack(">I", zlib.crc32(ctype + body) & 0xFFFFFFFF)


def write_png(path, width, height, pixels):
    raw = b"".join(b"\x00" + pixels[y * width * 3:(y + 1) * width * 3] for y in range(height))
    body = chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
    body += chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    open(path, "wb").write(b"\x89PNG\r\n\x1a\n" + body)


def main():
    src, dst = sys.argv[1], sys.argv[2]
    os.makedirs(dst, exist_ok=True)
    total_in = total_out = 0
    count = 0
    for name in sorted(os.listdir(src)):
        if not name.lower().endswith(".png"):
            continue
        width, height, pixels = read_png(os.path.join(src, name))
        out = os.path.join(dst, name)
        write_png(out, width, height, pixels)
        w2, h2, pixels2 = read_png(out)
        assert (w2, h2, pixels2) == (width, height, pixels), name
        a, b = os.path.getsize(os.path.join(src, name)), os.path.getsize(out)
        total_in += a
        total_out += b
        count += 1
        print("%-48s %7d -> %6d bytes  pixels-sha256 %s" % (name, a, b, hashlib.sha256(pixels).hexdigest()[:16]))
    print("files=%d total %d -> %d bytes (%.1f%%)" % (count, total_in, total_out, 100.0 * total_out / total_in))


if __name__ == "__main__":
    main()
