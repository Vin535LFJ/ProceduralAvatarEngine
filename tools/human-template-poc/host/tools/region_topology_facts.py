"""Objective region-topology facts for the base mesh (host analysis tooling).

Standard library + numpy only. NOT part of the engine. Reads the pinned
MakeHuman base.obj and default.mhskel / default_weights.mhw and prints facts
that support (or refute) what the wire-frame close-ups appear to show.

usage: python region_topology_facts.py <makehuman/data>
"""

import collections
import json
import os
import sys

import numpy as np


def seg_dist(p, a, b):
    ab = b - a
    t = np.clip(((p - a) @ ab) / (ab @ ab), 0.0, 1.0)
    return np.linalg.norm(p - (a + t[:, None] * ab), axis=1)


def main():
    data = sys.argv[1]
    V, F, grp, cur = [], [], [], None
    for line in open(os.path.join(data, "3dobjs", "base.obj"), encoding="utf-8", errors="replace"):
        if line.startswith("v "):
            V.append([float(x) for x in line.split()[1:4]])
        elif line.startswith("f "):
            F.append([int(t.split("/")[0]) - 1 for t in line.split()[1:]])
            grp.append(cur)
        elif line.startswith("g "):
            cur = line[2:].strip()
    V = np.array(V) * 0.1  # mesh units -> metres
    F = np.array(F)
    grp = np.array(grp)
    body = F[grp == "body"]
    P = V[np.unique(body)]
    rig = json.load(open(os.path.join(data, "rigs", "default.mhskel"), encoding="utf-8"))
    J = {k: (V[np.array(v) - 1].mean(0) if isinstance(v, list) else V[int(v) - 1]) for k, v in rig["joints"].items()}
    print("OBJECTIVE REGION TOPOLOGY FACTS (region_topology_facts.py, numpy %s; metres)" % np.__version__)
    print("body vertices %d, body quads %d" % (len(P), len(body)))

    print("\n[1] local mesh density around joints: quads with at least one vertex within 40 mm of the joint centre")
    for jn in ["upperarm01.L____head", "lowerarm01.L____head", "wrist.L____head", "upperleg01.L____head",
               "lowerleg01.L____head", "foot.L____head", "neck01____head", "head____head", "eye.L____head"]:
        c = J[jn]
        close = np.linalg.norm(P - c, axis=1) < 0.04
        near = set(np.flatnonzero(close).tolist())
        n = sum(1 for f in body if any((i in near) for i in f))
        print("    %-22s joint %s  quads within 40 mm: %d" % (jn, np.round(c, 3), n))
    print("\n[2] vertices per digit (nearest digit bone; vertices within 3 cm of the digit skeleton)")
    for prefix, side in [("finger", ".L"), ("toe", ".L")]:
        names = [n for n in rig["bones"] if n.startswith(prefix) and n.endswith(side)]
        A = np.array([J[rig["bones"][n]["head"]] for n in names])
        C = np.array([J[rig["bones"][n]["tail"]] for n in names])
        pts = np.concatenate([A, C])
        sel = P[np.all((P > pts.min(0) - 0.03) & (P < pts.max(0) + 0.03), axis=1)]
        D = np.stack([seg_dist(sel, A[i], C[i]) for i in range(len(names))], 1)
        lab = D.argmin(1)
        digits = sorted(set(n.split("-")[0] for n in names))
        parts = []
        for dg in digits:
            ph = [sum(1 for x in lab if names[x].startswith(dg + "-"))]
            parts.append("%s=%d" % (dg, ph[0]))
        print("    %s%s: %d bones; %s; vertices in the box %d" % (prefix, side, len(names), ", ".join(parts), len(sel)))

    print("\n[3] face landmarks")
    for jn, radius in [("eye.L____head", 0.025), ("eye.L____head", 0.040), ("jaw____head", 0.030)]:
        c = J[jn]
        n = int((np.linalg.norm(P - c, axis=1) < radius).sum())
        print("    vertices within %.0f mm of %s %s: %d" % (radius * 1000, jn, np.round(c, 3), n))

    print("\n[4] MakeHuman weights versus the non-facial skeleton")
    W = json.load(open(os.path.join(data, "rigs", "default_weights.mhw"), encoding="utf-8"))["weights"]
    roles_face = [n for n in rig["bones"] if n.startswith(("eye.", "jaw", "tongue", "levator", "oculi", "orbicularis", "oris",
                                                             "risorius", "temporalis", "special", "breast"))]
    def role_unused(n):
        return None
    any_w = set(i for v in W.values() for i, _ in v)
    deform_w = set(i for k, v in W.items() if k not in roles_face for i, _ in v)
    print("    weight groups %d, of which facial/breast groups %d" % (len(W), len(roles_face)))
    print("    body vertices with ANY weight: %d of %d" % (sum(1 for i in np.unique(body) if i in any_w), len(np.unique(body))))
    un = [i for i in np.unique(body) if i not in deform_w]
    print("    body vertices with NO non-facial weight (unweighted once facial bones are dropped): %d" % len(un))
    if un:
        U = V[np.array(un)]
        touched = sum(1 for f in body if any(i in set(un) for i in f))
        print("      bbox x %.3f..%.3f  y %.3f..%.3f  z %.3f..%.3f m, quads touching them %d"
              % (U[:, 0].min(), U[:, 0].max(), U[:, 1].min(), U[:, 1].max(), U[:, 2].min(), U[:, 2].max(), touched))

    print("\n[5] OBJ groups")
    c = collections.Counter("joint" if g.startswith("joint-") else "helper" if g.startswith("helper-") else g for g in grp)
    print("    faces per group class:", dict(c))
    print("    helper groups:", ", ".join(sorted(set(g[7:] for g in grp if g.startswith("helper-")))))
    print("    joint groups:", len(set(g for g in grp if g.startswith("joint-"))))


if __name__ == "__main__":
    main()
