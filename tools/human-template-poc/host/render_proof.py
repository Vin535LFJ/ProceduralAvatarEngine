import csv
import math
import os
import struct
import sys
import zlib
from pathlib import Path

import numpy as np

# Optional argv[1]: the template_proof output directory. Defaults to
# build-host/human-template-poc/out next to the repository's own host build.
ROOT = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parents[3] / "build-host" / "human-template-poc" / "out"
OUT = ROOT / "renders"
OUT.mkdir(exist_ok=True)
W, H = 900, 1200


def load_obj(path):
    vertices = []
    faces = []
    with open(path, encoding="ascii") as f:
        for line in f:
            if line.startswith("v "):
                vertices.append([float(x) for x in line.split()[1:4]])
            elif line.startswith("f "):
                ids = [int(x.split("/")[0]) - 1 for x in line.split()[1:]]
                faces.append(ids)
    return np.asarray(vertices, dtype=np.float64), np.asarray(faces, dtype=np.int32)


def load_weights():
    rows = []
    with open(ROOT / "weights.csv", encoding="ascii") as f:
        for row in csv.DictReader(f):
            colors = []
            weights = []
            for i in range(4):
                bone = int(row[f"bone{i}"])
                weight = float(row[f"weight{i}"])
                if weight <= 0:
                    continue
                hues = [0.02, 0.10, 0.20, 0.32, 0.45, 0.58, 0.70, 0.82]
                h = hues[bone % len(hues)]
                colors.append(hsv(h, 0.75, 1.0))
                weights.append(weight)
            c = np.asarray(colors)
            w = np.asarray(weights)
            rows.append((c * w[:, None]).sum(axis=0) / w.sum())
    return np.asarray(rows)


def hsv(h, s, v):
    i = int(h * 6) % 6
    f = h * 6 - int(h * 6)
    p = v * (1 - s)
    q = v * (1 - f * s)
    t = v * (1 - (1 - f) * s)
    return [(v, t, p), (q, v, p), (p, v, t), (p, q, v), (t, p, v), (v, p, q)][i]


def write_png(path, rgb):
    rgb = np.clip(rgb * 255, 0, 255).astype(np.uint8)
    raw = b"".join(b"\x00" + row.tobytes() for row in rgb)
    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xffffffff)
    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 6))
    png += chunk(b"IEND", b"")
    path.write_bytes(png)


def camera(eye, target, up=(0, 1, 0), scale=2.05):
    eye = np.asarray(eye, dtype=float)
    target = np.asarray(target, dtype=float)
    up = np.asarray(up, dtype=float)
    forward = (target - eye)
    forward /= np.linalg.norm(forward)
    right = np.cross(forward, up)
    right /= np.linalg.norm(right)
    true_up = np.cross(right, forward)
    def project(points):
        q = points - target
        x = q @ right
        y = q @ true_up
        z = q @ forward
        sx = W * 0.5 + x / scale * H
        sy = H * 0.53 - y / scale * H
        return sx, sy, z
    return project


def render(name, obj_name, eye, target=(0, 0.8, 0), mode="clay", scale=2.05):
    vertices, faces = load_obj(ROOT / obj_name)
    project = camera(eye, target, scale=scale)
    sx, sy, depth = project(vertices)
    tri2 = np.stack([sx[faces], sy[faces]], axis=2)
    tri3 = vertices[faces]
    normals = np.cross(tri3[:, 1] - tri3[:, 0], tri3[:, 2] - tri3[:, 0])
    normals /= np.maximum(np.linalg.norm(normals, axis=1, keepdims=True), 1e-12)
    light = np.asarray([0.35, 0.65, 0.9])
    light /= np.linalg.norm(light)
    shade = 0.40 + 0.60 * np.maximum(0, normals @ light)
    face_areas = np.linalg.norm(np.cross(tri3[:, 1] - tri3[:, 0], tri3[:, 2] - tri3[:, 0]), axis=1)
    vertex_normals = np.zeros_like(vertices)
    for column in range(3):
        np.add.at(vertex_normals, faces[:, column], normals * face_areas[:, None])
    vertex_normals /= np.maximum(np.linalg.norm(vertex_normals, axis=1, keepdims=True), 1e-12)
    vertex_shade = 0.32 + 0.68 * np.maximum(0, vertex_normals @ light)
    if mode == "normal":
        colors = (normals * 0.5 + 0.5)
    elif mode == "weight":
        colors = load_weights()[faces].mean(axis=1)
    else:
        colors = np.repeat((shade * 0.86)[:, None], 3, axis=1)
    vertex_clay = np.repeat((vertex_shade * 0.86)[:, None], 3, axis=1)
    image = np.zeros((H, W, 3), dtype=np.float64)
    image[:] = [0.16, 0.18, 0.20]
    zbuffer = np.full((H, W), 1e9, dtype=np.float64)
    for ti, face in enumerate(faces):
        p = tri2[ti]
        minx = max(0, int(math.floor(p[:, 0].min())))
        maxx = min(W - 1, int(math.ceil(p[:, 0].max())))
        miny = max(0, int(math.floor(p[:, 1].min())))
        maxy = min(H - 1, int(math.ceil(p[:, 1].max())))
        if minx > maxx or miny > maxy:
            continue
        x0, y0 = p[0]
        x1, y1 = p[1]
        x2, y2 = p[2]
        denom = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2)
        if abs(denom) < 1e-9:
            continue
        for y in range(miny, maxy + 1):
            py = y + 0.5
            for x in range(minx, maxx + 1):
                px = x + 0.5
                w0 = ((y1 - y2) * (px - x2) + (x2 - x1) * (py - y2)) / denom
                w1 = ((y2 - y0) * (px - x2) + (x0 - x2) * (py - y2)) / denom
                w2 = 1.0 - w0 - w1
                if w0 < 0 or w1 < 0 or w2 < 0:
                    continue
                z = w0 * depth[face[0]] + w1 * depth[face[1]] + w2 * depth[face[2]]
                if z < zbuffer[y, x]:
                    zbuffer[y, x] = z
                    if mode == "clay":
                        image[y, x] = (w0 * vertex_clay[face[0]] +
                                       w1 * vertex_clay[face[1]] +
                                       w2 * vertex_clay[face[2]])
                    else:
                        image[y, x] = colors[ti]
    if mode == "wire":
        image[:] = [0.16, 0.18, 0.20]
        for face in faces:
            pts = [(sx[i], sy[i]) for i in face]
            for a, b in zip(pts, pts[1:] + pts[:1]):
                x0, y0 = map(int, map(round, a))
                x1, y1 = map(int, map(round, b))
                dx, sxstep = abs(x1 - x0), 1 if x0 < x1 else -1
                dy, systep = -abs(y1 - y0), 1 if y0 < y1 else -1
                err = dx + dy
                while True:
                    if 0 <= x0 < W and 0 <= y0 < H:
                        image[y0, x0] = [0.12, 0.35, 0.16]
                    if x0 == x1 and y0 == y1:
                        break
                    e2 = 2 * err
                    if e2 >= dy:
                        err += dy
                        x0 += sxstep
                    if e2 <= dx:
                        err += dx
                        y0 += systep
    write_png(OUT / f"{name}.png", image)


views = {
    "front": ((0, 0.05, 5), (0, 0, 0.05)),
    "back": ((0, 0.05, -5), (0, 0, 0.05)),
    "left": ((5, 0.05, 0.05), (0, 0, 0.05)),
    "right": ((-5, 0.05, 0.05), (0, 0, 0.05)),
    "left45": ((3.5, 0.05, 3.5), (0, 0, 0.05)),
    "right45": ((-3.5, 0.05, 3.5), (0, 0, 0.05)),
}
for view, (eye, target) in views.items():
    render(f"base-{view}-clay", "base.obj", eye, target)
for mode in ("wire", "normal", "weight"):
    render(f"base-front-{mode}", "base.obj", views["front"][0], views["front"][1], mode=mode)
render("shoulder-closeup-clay", "base.obj", (2.6, 0.72, 2.4), (0.18, 0.43, 0.04), scale=0.60)
render("axilla-closeup-clay", "base.obj", (2.2, 0.50, 1.5), (0.18, 0.38, 0.03), scale=0.45)
render("both-abduct-front-clay", "both_abduct.obj", views["front"][0], views["front"][1])
render("both-abduct-left-clay", "both_abduct.obj", views["left"][0], views["left"][1])
for target in ("torso-scale-horiz-incr.target", "measure-waist-circ-incr.target",
               "measure-shoulder-dist-incr.target", "hip-scale-horiz-incr.target"):
    render(f"target-{target[:-7]}-front-clay", target + ".obj", views["front"][0], views["front"][1])
print("rendered", len(list(OUT.glob("*.png"))), "images")
