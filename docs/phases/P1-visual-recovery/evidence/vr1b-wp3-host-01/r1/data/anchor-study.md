# WP3-H-R1 scaling-anchor verification (generated)

Anchor pivot = (rootX=0.000000, soleY=-0.816760, rootZ=-0.077920) m.
- rootX = body bbox x-centre = 0.000000 m; vertex mean x = 6.632e-16 m; mirror-symmetric: YES.
- rootZ = rig Root bone head z (authoritative character root node).
- soleY = min body y (feet planted).
- WP3-HOST-01 bug: pivot was (sole,sole,sole)=(-0.8168,-0.8168,-0.8168); horizontal drift at 1.55 m = -0.0568 m (x), -0.0568 m (z).

| Target (m) | achieved | sole fixed | rootZ fixed | meanX drift | height err | upperRatio | thighRatio | L/R sym err | helper sync | repeat diff | PASS |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| 1.5500 | 1.5500 | 0.00e+00 | 0.00e+00 | 6.39e-16 | 0.00e+00 | 0.13188 | 0.24984 | 0.00e+00 | 0.00e+00 | 0.0e+00 | YES |
| 1.6659 | 1.6659 | 0.00e+00 | 0.00e+00 | 9.82e-16 | 0.00e+00 | 0.13188 | 0.24984 | 0.00e+00 | 0.00e+00 | 0.0e+00 | YES |
| 1.7549 | 1.7549 | 0.00e+00 | 0.00e+00 | 8.09e-16 | 0.00e+00 | 0.13188 | 0.24984 | 0.00e+00 | 0.00e+00 | 0.0e+00 | YES |
| 1.8500 | 1.8500 | 0.00e+00 | 0.00e+00 | 8.47e-16 | 4.44e-16 | 0.13188 | 0.24984 | 0.00e+00 | 0.00e+00 | 0.0e+00 | YES |

All nine checks: sole anchor fixed, root node fixed, X/Z centreline no drift, height exact, limb proportions preserved, L/R joint symmetry preserved, helper/joint synced, repeat bit-exact, 1.55/1.6659/1.7549/1.85 retested.
