"""Per-row silhouette width profile of a rendered front view.

For each screen row (sampled), find the leftmost/rightmost body pixel
(non-background) and report width + center. This reproduces the 2D silhouette
a user sees in the front screenshot, in pixels, so proportions can be checked
objectively (shoulder vs waist vs hip widths, head size, arm spread).
"""
import sys
sys.path.insert(0, "C:/Users/Giant/Desktop/GITCODE/Procedural Avatar Engine/tools")
from png_probe import read_png


def profile(path, surface_top=0, surface_bottom=None, bg_tol=20, step=8):
    w, h, ch, rows = read_png(path)
    if surface_bottom is None:
        surface_bottom = h
    xs = [3, w - 4, w // 6, 5 * w // 6]
    ys = range(surface_top, surface_bottom)
    bg = []
    for y in ys:
        row = rows[y]
        samples = [[row[x * ch + i] for i in range(3)] for x in xs]
        bg.append([max(s[i] for s in samples) for i in range(3)])
    out = []
    for idx, y in enumerate(ys):
        row = rows[y]
        ref = bg[idx]
        lo, hi = -1, -1
        for x in range(w):
            off = x * ch
            d = max(abs(row[off + i] - ref[i]) for i in range(3))
            if d > bg_tol:
                if lo < 0:
                    lo = x
                hi = x
        if lo >= 0:
            out.append((y, lo, hi, hi - lo + 1, (lo + hi) / 2.0))
        if idx % step == 0 or idx == len(list(ys)) - 1:
            pass
    return w, h, out


if __name__ == "__main__":
    path = sys.argv[1]
    top = int(sys.argv[2]) if len(sys.argv) > 2 else 0
    bot = int(sys.argv[3]) if len(sys.argv) > 3 else None
    w, h, prof = profile(path, surface_top=top, surface_bottom=bot)
    if not prof:
        print("no body pixels")
        sys.exit(0)
    ymin, ymax = prof[0][0], prof[-1][0]
    span = ymax - ymin
    print(f"image {w}x{h}, body rows y[{ymin}..{ymax}] span={span}px")
    print(f"{'y%':>5} {'y':>5} {'lo':>5} {'hi':>5} {'width':>6} {'center':>7}")
    for (y, lo, hi, wid, c) in prof:
        if (y - ymin) % max(1, span // 40) == 0 or y == ymax:
            print(f"{100.0*(y-ymin)/span:5.1f} {y:5d} {lo:5d} {hi:5d} {wid:6d} {c:7.1f}")
    # key widths
    def width_at(frac):
        target = ymin + frac * span
        best = min(prof, key=lambda r: abs(r[0] - target))
        return best
    for name, frac in [("head-top", 0.02), ("neck", 0.10), ("shoulder", 0.16),
                       ("chest", 0.28), ("waist", 0.55), ("hip", 0.75),
                       ("lowest(arm)", 0.98)]:
        r = width_at(frac)
        print(f"{name:>14} @y={r[0]} width={r[3]}px center={r[4]:.1f}")