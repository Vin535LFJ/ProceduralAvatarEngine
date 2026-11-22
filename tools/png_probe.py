"""Minimal PNG reader (zlib + struct, no third-party deps).

Used only to produce objective pixel statistics for M12-A anatomy preview
evidence: coverage, brightness distribution, dominant body colours. Not a
general image library -- 8-bit non-interlaced RGB/RGBA PNG only, which is
what `adb screencap -p` emits.
"""
import struct
import sys
import zlib


def read_png(path):
    with open(path, "rb") as fh:
        data = fh.read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("not a PNG")
    pos = 8
    width = height = bitdepth = colortype = interlace = None
    idat = bytearray()
    while pos + 8 <= len(data):
        length, kind = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + length]
        if kind == b"IHDR":
            width, height, bitdepth, colortype, _comp, _filt, interlace = \
                struct.unpack(">IIBBBBB", body)
        elif kind == b"IDAT":
            idat += body
        elif kind == b"IEND":
            break
        pos += 12 + length
    if interlace:
        raise ValueError("interlaced PNG not supported")
    if bitdepth != 8:
        raise ValueError("only 8-bit PNG supported")
    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[colortype]
    stride = width * channels
    raw = zlib.decompress(bytes(idat))
    rows = []
    prev = bytearray(stride)
    p = 0
    for _y in range(height):
        ftype = raw[p]
        p += 1
        line = bytearray(raw[p:p + stride])
        p += stride
        if ftype == 1:      # Sub
            for i in range(channels, stride):
                line[i] = (line[i] + line[i - channels]) & 0xFF
        elif ftype == 2:    # Up
            for i in range(stride):
                line[i] = (line[i] + prev[i]) & 0xFF
        elif ftype == 3:    # Average
            for i in range(stride):
                left = line[i - channels] if i >= channels else 0
                line[i] = (line[i] + ((left + prev[i]) >> 1)) & 0xFF
        elif ftype == 4:    # Paeth
            for i in range(stride):
                left = line[i - channels] if i >= channels else 0
                up = prev[i]
                ul = prev[i - channels] if i >= channels else 0
                pa = abs(left + up - ul - left)
                pb = abs(left + up - ul - up)
                pc = abs(left + up - ul - ul)
                pred = left if pa <= pb and pa <= pc else (up if pb <= pc else ul)
                line[i] = (line[i] + pred) & 0xFF
        rows.append(bytes(line))
        prev = line
    return width, height, channels, rows


def stats(path, bg_tol=20, surface_top=0, surface_bottom=None):
    w, h, ch, rows = read_png(path)
    if surface_bottom is None:
        surface_bottom = h
    # The renderer's background is a vertical gradient (function of y only),
    # so sample a column near the left edge to recover the per-row profile.
    xs = [3, w - 4, (w // 6), (5 * w // 6)]
    ys = range(surface_top, surface_bottom)
    bg = []
    for y in ys:
        row = rows[y]
        samples = []
        for x in xs:
            off = x * ch
            samples.append([row[off], row[off + 1], row[off + 2]])
        bg.append([max(s[i] for s in samples) for i in range(3)])
    total = 0
    bg_count = 0
    body = []
    bright_hist = [0] * 8
    for idx, y in enumerate(ys):
        row = rows[y]
        ref = bg[idx]
        for x in range(w):
            off = x * ch
            px = [row[off], row[off + 1], row[off + 2]]
            d = max(abs(px[i] - ref[i]) for i in range(3))
            total += 1
            if d <= bg_tol:
                bg_count += 1
                continue
            lum = (px[0] * 299 + px[1] * 587 + px[2] * 114) // 1000
            bright_hist[min(lum >> 5, 7)] += 1
            body.append(tuple(px))
    covered = total - bg_count
    mean = [0.0, 0.0, 0.0]
    if body:
        n = len(body)
        for px in body:
            for i in range(3):
                mean[i] += px[i]
        mean = [v / n for v in mean]
    # Dominant colour bucket (10-level quantisation).
    buckets = {}
    for px in body:
        key = ((px[0] // 26) * 26, (px[1] // 26) * 26, (px[2] // 26) * 26)
        buckets[key] = buckets.get(key, 0) + 1
    top = sorted(buckets.items(), key=lambda kv: -kv[1])[:5]
    return {
        "size": f"{w}x{h}",
        "coverage_pct": round(100.0 * covered / total, 2),
        "body_pixels": covered,
        "mean_rgb": [round(v, 1) for v in mean],
        "bright_hist_0_255": bright_hist,
        "distinct_colors": len(buckets),
        "top_colors": [(k, round(100.0 * v / max(len(body), 1), 1)) for k, v in top],
    }


if __name__ == "__main__":
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument("paths", nargs="+")
    ap.add_argument("--top", type=int, default=0)
    ap.add_argument("--bottom", type=int, default=None)
    args = ap.parse_args()
    for p in args.paths:
        s = stats(p, surface_top=args.top, surface_bottom=args.bottom)
        print(p)
        for k, v in s.items():
            print(f"  {k}: {v}")