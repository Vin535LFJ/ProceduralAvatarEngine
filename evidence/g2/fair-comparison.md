# G2 fair comparison (PoC-A)

All routes share the same Shape Model, landmarks, reference surface, sampling rule, sample count, distance metric, coordinate system, parameter groups and camera protocol (SPEC 8.1).

Anatomy resolution 44x66x26; template 26 rings x 32 segments.

## Error layers (metres)

| Group | Route | E1 samples | E2 samples | E3 samples | E1 mean | E1 RMS | E1 Hausdorff | E2 mean | E2 max | E3 mean |
|---|---|---|---|---|---|---|---|---|---|---|
| Baseline | R1 SDF + Surface Nets | 5408 | 5408 | 2066 | 0.00440 | 0.00506 | 0.01724 | 0.00098 | 0.02310 | 0.00149 |
| Baseline | R2 quad template projected on SDF | 5408 | 5408 | 2066 | 0.00440 | 0.00506 | 0.01724 | 0.00098 | 0.02310 | 0.00149 |
| Baseline | R3 quad template on B-spline control lattice | 5408 | 5408 | 2066 | 0.03272 | 0.04783 | 0.13310 | 0.00040 | 0.00100 | 0.03211 |
| Chest Width-Depth | R1 SDF + Surface Nets | 5408 | 5408 | 2236 | 0.00459 | 0.00541 | 0.02069 | 0.00102 | 0.01683 | 0.00163 |
| Chest Width-Depth | R2 quad template projected on SDF | 5408 | 5408 | 2236 | 0.00459 | 0.00541 | 0.02069 | 0.00102 | 0.01683 | 0.00163 |
| Chest Width-Depth | R3 quad template on B-spline control lattice | 5408 | 5408 | 2236 | 0.03297 | 0.04472 | 0.12391 | 0.00041 | 0.00100 | 0.03243 |
| Waist-Pelvis Ratio | R1 SDF + Surface Nets | 5408 | 5408 | 2282 | 0.00465 | 0.00540 | 0.02005 | 0.00118 | 0.03177 | 0.00198 |
| Waist-Pelvis Ratio | R2 quad template projected on SDF | 5408 | 5408 | 2282 | 0.00465 | 0.00540 | 0.02005 | 0.00118 | 0.03177 | 0.00198 |
| Waist-Pelvis Ratio | R3 quad template on B-spline control lattice | 5408 | 5408 | 2282 | 0.03623 | 0.05173 | 0.14514 | 0.00041 | 0.00131 | 0.03542 |

**Reading the layers**

- E1 = route reference surface vs the shared target reference (topology independent). For R1 and R2 this is the distance between the continuous SDF and R1's discretised mesh - a REFERENCE FIDELITY number, not a route-quality number, because R1 and R2 share the same surface.
- E2 = route reference surface vs its own canonical mesh. This is the cost of the fixed-topology projection ALONE and must not be mixed into E1 or E3.
- E3 = canonical mesh vs the shared target reference: the final shape error of the route that would actually be shipped.

Grey-clay fixed-view evidence for the owner's independent visual Gate: [clay/index.md](clay/index.md) (3 groups x 3 routes x 7 views). Camera, light and band are shared; the framing is shared by every route within a view. No image is marked VISUALLY ACCEPTED by the AI.

## Topology audit (Layer B)

| Group | Route | Verts | Tris | Quads | Components | Boundary (crop) | Unregistered | NonManifold | Degenerate | Clean |
|---|---|---|---|---|---|---|---|---|---|---|
| Baseline | R1 SDF + Surface Nets | 5108 | 10212 | 5106 | 1 | 0 | 0 | 0 | 0 | yes |
| Baseline | R2 quad template projected on SDF | 833 | 1632 | 800 | 1 | 32 | 0 | 0 | 0 | yes |
| Baseline | R3 quad template on B-spline control lattice | 833 | 1632 | 800 | 1 | 32 | 0 | 0 | 0 | yes |
| Chest Width-Depth | R1 SDF + Surface Nets | 5234 | 10464 | 5232 | 1 | 0 | 0 | 0 | 0 | yes |
| Chest Width-Depth | R2 quad template projected on SDF | 833 | 1632 | 800 | 1 | 32 | 0 | 0 | 0 | yes |
| Chest Width-Depth | R3 quad template on B-spline control lattice | 833 | 1632 | 800 | 1 | 32 | 0 | 0 | 0 | yes |
| Waist-Pelvis Ratio | R1 SDF + Surface Nets | 5314 | 10632 | 5316 | 1 | 0 | 0 | 0 | 0 | yes |
| Waist-Pelvis Ratio | R2 quad template projected on SDF | 833 | 1632 | 800 | 1 | 32 | 0 | 0 | 0 | yes |
| Waist-Pelvis Ratio | R3 quad template on B-spline control lattice | 833 | 1632 | 800 | 1 | 32 | 0 | 0 | 0 | yes |

## Face quality and edge flow (Layer B, canonical quad grid)

| Group | Route | Mean aspect | Max aspect | Mean min angle | Slivers | Edge flow circum. p95 | Edge flow long. p95 | Orthogonal quad ratio |
|---|---|---|---|---|---|---|---|---|
| Baseline | R1 SDF + Surface Nets | 1.705 | 39.983 | 70.1 deg | 30 | 89.6 deg | 90.0 deg | 0.637 |
| Baseline | R2 quad template projected on SDF | 2.013 | 6.868 | 83.0 deg | 0 | 58.0 deg | 30.8 deg | 0.935 |
| Baseline | R3 quad template on B-spline control lattice | 2.103 | 11.161 | 86.4 deg | 20 | 20.9 deg | 13.2 deg | 1.000 |
| Chest Width-Depth | R1 SDF + Surface Nets | 1.673 | 26.736 | 69.7 deg | 22 | 89.6 deg | 90.0 deg | 0.632 |
| Chest Width-Depth | R2 quad template projected on SDF | 2.073 | 7.505 | 82.2 deg | 0 | 59.8 deg | 36.0 deg | 0.904 |
| Chest Width-Depth | R3 quad template on B-spline control lattice | 2.191 | 12.728 | 85.2 deg | 28 | 21.5 deg | 19.9 deg | 1.000 |
| Waist-Pelvis Ratio | R1 SDF + Surface Nets | 1.736 | 60.778 | 69.9 deg | 36 | 89.6 deg | 90.0 deg | 0.628 |
| Waist-Pelvis Ratio | R2 quad template projected on SDF | 2.155 | 8.577 | 81.6 deg | 0 | 64.8 deg | 41.1 deg | 0.897 |
| Waist-Pelvis Ratio | R3 quad template on B-spline control lattice | 2.141 | 11.873 | 83.8 deg | 28 | 20.4 deg | 24.4 deg | 1.000 |

## Layer A: anatomical landmark error (SPEC 13 #5)

Fifteen named PoC-A landmarks, identical for every route. Each landmark is resolved on the SAME (height, azimuth) grid cell of the route's reference surface and of the target reference. Euclidean distance in metres; threshold TBD (§13 #5).

| Group | Route | compared | mean | p50 | p95 | max |
|---|---|---|---|---|---|---|
| Baseline | R1 SDF + Surface Nets | 15 | 0.00000 | 0.00000 | 0.00000 | 0.00000 |
| Baseline | R2 quad template projected on SDF | 15 | 0.00000 | 0.00000 | 0.00000 | 0.00000 |
| Baseline | R3 quad template on B-spline control lattice | 15 | 0.05443 | 0.03926 | 0.11503 | 0.12684 |
| Chest Width-Depth | R1 SDF + Surface Nets | 15 | 0.00000 | 0.00000 | 0.00000 | 0.00000 |
| Chest Width-Depth | R2 quad template projected on SDF | 15 | 0.00000 | 0.00000 | 0.00000 | 0.00000 |
| Chest Width-Depth | R3 quad template on B-spline control lattice | 15 | 0.05550 | 0.04607 | 0.10375 | 0.11271 |
| Waist-Pelvis Ratio | R1 SDF + Surface Nets | 15 | 0.00000 | 0.00000 | 0.00000 | 0.00000 |
| Waist-Pelvis Ratio | R2 quad template projected on SDF | 15 | 0.00000 | 0.00000 | 0.00000 | 0.00000 |
| Waist-Pelvis Ratio | R3 quad template on B-spline control lattice | 15 | 0.06194 | 0.04320 | 0.12648 | 0.13308 |

Per-landmark detail, Baseline group (metres):

| Landmark | R1 SDF + Surface Nets | R2 quad template projected on SDF | R3 quad template on B-spline control lattice |
|---|---|---|---|
| iliac-crest-left | 0.00000 | 0.00000 | 0.02611 |
| iliac-crest-front | 0.00000 | 0.00000 | 0.03512 |
| waist-left | 0.00000 | 0.00000 | 0.02253 |
| waist-back | 0.00000 | 0.00000 | 0.03904 |
| lower-rib-left | 0.00000 | 0.00000 | 0.01975 |
| lower-rib-front | 0.00000 | 0.00000 | 0.02640 |
| sternum | 0.00000 | 0.00000 | 0.04084 |
| upper-chest-left | 0.00000 | 0.00000 | 0.12684 |
| scapula-back | 0.00000 | 0.00000 | 0.06804 |
| shoulder-root-left | 0.00000 | 0.00000 | 0.10997 |
| shoulder-root-right | 0.00000 | 0.00000 | 0.10997 |
| shoulder-root-left-front | 0.00000 | 0.00000 | 0.01104 |
| shoulder-root-left-back | 0.00000 | 0.00000 | 0.07812 |
| shoulder-root-right-front | 0.00000 | 0.00000 | 0.06346 |
| shoulder-root-right-back | 0.00000 | 0.00000 | 0.03926 |

## Layer A: normal field consistency (SPEC 13 #7)

Angle between each sample's normal and the reference normal, in degrees. `reference` measures the route's reference surface; `shipped mesh` measures the canonical mesh that would actually be rendered. Both use the same reference normals and the same grid cells. Smooth-seam contract value: <=5 degrees; all other thresholds TBD.

| Group | Route | surface | compared | flipped | mean | p50 | p95 | max |
|---|---|---|---|---|---|---|---|---|
| Baseline | R1 SDF + Surface Nets | reference | 5408 | 0 | 0.01 | 0.00 | 0.03 | 0.04 |
| Baseline | R1 SDF + Surface Nets | shipped mesh | 5408 | 144 | 9.23 | 3.32 | 32.57 | 114.78 |
| Baseline | R2 quad template projected on SDF | reference | 5408 | 0 | 0.01 | 0.00 | 0.03 | 0.04 |
| Baseline | R2 quad template projected on SDF | shipped mesh | 5408 | 144 | 9.23 | 3.32 | 32.57 | 114.78 |
| Baseline | R3 quad template on B-spline control lattice | reference | 5408 | 19 | 25.73 | 21.90 | 58.26 | 122.17 |
| Baseline | R3 quad template on B-spline control lattice | shipped mesh | 5408 | 152 | 28.19 | 22.62 | 70.29 | 121.98 |
| Chest Width-Depth | R1 SDF + Surface Nets | reference | 5408 | 0 | 0.01 | 0.00 | 0.03 | 0.04 |
| Chest Width-Depth | R1 SDF + Surface Nets | shipped mesh | 5408 | 144 | 9.37 | 3.44 | 32.48 | 114.78 |
| Chest Width-Depth | R2 quad template projected on SDF | reference | 5408 | 0 | 0.01 | 0.00 | 0.03 | 0.04 |
| Chest Width-Depth | R2 quad template projected on SDF | shipped mesh | 5408 | 144 | 9.37 | 3.44 | 32.48 | 114.78 |
| Chest Width-Depth | R3 quad template on B-spline control lattice | reference | 5408 | 19 | 26.88 | 23.13 | 57.38 | 122.17 |
| Chest Width-Depth | R3 quad template on B-spline control lattice | shipped mesh | 5408 | 152 | 29.37 | 23.99 | 71.08 | 121.98 |
| Waist-Pelvis Ratio | R1 SDF + Surface Nets | reference | 5408 | 0 | 0.01 | 0.00 | 0.03 | 0.04 |
| Waist-Pelvis Ratio | R1 SDF + Surface Nets | shipped mesh | 5408 | 130 | 10.10 | 3.55 | 46.13 | 149.55 |
| Waist-Pelvis Ratio | R2 quad template projected on SDF | reference | 5408 | 0 | 0.01 | 0.00 | 0.03 | 0.04 |
| Waist-Pelvis Ratio | R2 quad template projected on SDF | shipped mesh | 5408 | 130 | 10.10 | 3.55 | 46.13 | 149.55 |
| Waist-Pelvis Ratio | R3 quad template on B-spline control lattice | reference | 5408 | 26 | 28.14 | 24.85 | 60.00 | 131.79 |
| Waist-Pelvis Ratio | R3 quad template on B-spline control lattice | shipped mesh | 5408 | 134 | 30.30 | 25.69 | 69.02 | 147.89 |

Flipped-normal cells by ring (shipped meshes, Baseline; only rings with a flip are listed):

A flip is a hard normal-field disagreement and is recorded rather than hidden by folding it into an acute angle. Every flip is confined to rings 0-1 (y ~ 0.844-0.852 m), where the pelvis fan cap meets the side wall: the cap's downward faces dominate the accumulated vertex normal there while the reference normal is radial. Rings 2-51 have zero flips.

| Ring | y (m) | R1 SDF + Surface Nets | R2 quad template projected on SDF | R3 quad template on B-spline control lattice |
|---|---|---|---|---|
| 0 | 0.844 | 73 | 73 | 77 |
| 1 | 0.852 | 71 | 71 | 75 |

## Layer A: cross-section contour deviation (SPEC 13 #1)

Signed horizontal-radius deviation at matched (height, azimuth) cells, over ten height bands of the PoC-A torso. Positive means the route is wider than the target at that cell.

| Group | Route | |deviation| mean | p95 | max | band range (signed) |
|---|---|---|---|---|---|---|
| Baseline | R1 SDF + Surface Nets | 0.00000 | 0.00000 | 0.00000 | +0.00000 .. +0.00000 |
| Baseline | R2 quad template projected on SDF | 0.00000 | 0.00000 | 0.00000 | +0.00000 .. +0.00000 |
| Baseline | R3 quad template on B-spline control lattice | 0.02679 | 0.10222 | 0.13678 | -0.01665 .. +0.01012 |
| Chest Width-Depth | R1 SDF + Surface Nets | 0.00000 | 0.00000 | 0.00000 | +0.00000 .. +0.00000 |
| Chest Width-Depth | R2 quad template projected on SDF | 0.00000 | 0.00000 | 0.00000 | +0.00000 .. +0.00000 |
| Chest Width-Depth | R3 quad template on B-spline control lattice | 0.03016 | 0.09358 | 0.13752 | -0.01732 .. +0.01098 |
| Waist-Pelvis Ratio | R1 SDF + Surface Nets | 0.00000 | 0.00000 | 0.00000 | +0.00000 .. +0.00000 |
| Waist-Pelvis Ratio | R2 quad template projected on SDF | 0.00000 | 0.00000 | 0.00000 | +0.00000 .. +0.00000 |
| Waist-Pelvis Ratio | R3 quad template on B-spline control lattice | 0.03154 | 0.11214 | 0.15166 | -0.02354 .. +0.01560 |

## Layer B: seam smoothness and T2 crop ring (SPEC 13 #7)

Dihedral deflection between adjacent quad face normals. `smooth` = <=5 degrees (contract value); `hard` = above it. The crop ring is the T2 shoulder-root attachment boundary.

| Group | Route | interior edges | smooth | hard | mean | p95 | max | crop edges | crop mean | crop max | crop planarity (mm) | crop smooth |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Baseline | R1 SDF + Surface Nets | 1568 | 591 | 977 | 9.84 | 41.36 | 72.22 | 32 | 14.52 | 47.66 | 0.00 | NO |
| Baseline | R2 quad template projected on SDF | 1568 | 591 | 977 | 9.84 | 41.36 | 72.22 | 32 | 14.52 | 47.66 | 0.00 | NO |
| Baseline | R3 quad template on B-spline control lattice | 1568 | 704 | 864 | 6.26 | 15.18 | 16.57 | 32 | 11.10 | 16.13 | 0.00 | NO |
| Chest Width-Depth | R1 SDF + Surface Nets | 1568 | 588 | 980 | 10.12 | 39.66 | 88.26 | 32 | 14.84 | 42.33 | 0.00 | NO |
| Chest Width-Depth | R2 quad template projected on SDF | 1568 | 588 | 980 | 10.12 | 39.66 | 88.26 | 32 | 14.84 | 42.33 | 0.00 | NO |
| Chest Width-Depth | R3 quad template on B-spline control lattice | 1568 | 692 | 876 | 6.32 | 15.18 | 16.57 | 32 | 10.92 | 16.28 | 0.00 | NO |
| Waist-Pelvis Ratio | R1 SDF + Surface Nets | 1568 | 516 | 1052 | 11.79 | 44.26 | 149.12 | 32 | 14.52 | 47.66 | 0.00 | NO |
| Waist-Pelvis Ratio | R2 quad template projected on SDF | 1568 | 516 | 1052 | 11.79 | 44.26 | 149.12 | 32 | 14.52 | 47.66 | 0.00 | NO |
| Waist-Pelvis Ratio | R3 quad template on B-spline control lattice | 1568 | 652 | 916 | 6.70 | 14.22 | 22.10 | 32 | 11.06 | 15.74 | 0.00 | NO |

## Weight legality and CPU deformation (SPEC 11.1)

GPU numerical parity remains `[UNVERIFIED]` at G2 and is NOT a kill condition (SPEC 11.1). The test pose is a 20-degree chest twist plus a 15-degree left-shoulder raise.

| Group | Route | legal | max influences | negative | sum violations | bad IDs | rigid probes | max rigid error | bind residual | max move | vol change |
|---|---|---|---|---|---|---|---|---|---|---|---|
| Baseline | R1 SDF + Surface Nets | yes | 2 | 0 | 0 | 0 | 22 | 0.000e+00 | 2.389e-07 | 0.21647 | 1.684% |
| Baseline | R2 quad template projected on SDF | yes | 2 | 0 | 0 | 0 | 22 | 0.000e+00 | 1.231e-07 | 0.13306 | 8.301% |
| Baseline | R3 quad template on B-spline control lattice | yes | 2 | 0 | 0 | 0 | 22 | 0.000e+00 | 1.204e-07 | 0.07899 | 3.909% |
| Chest Width-Depth | R1 SDF + Surface Nets | yes | 2 | 0 | 0 | 0 | 22 | 0.000e+00 | 2.389e-07 | 0.21647 | 2.108% |
| Chest Width-Depth | R2 quad template projected on SDF | yes | 2 | 0 | 0 | 0 | 22 | 0.000e+00 | 1.229e-07 | 0.13530 | 9.443% |
| Chest Width-Depth | R3 quad template on B-spline control lattice | yes | 2 | 0 | 0 | 0 | 22 | 0.000e+00 | 1.229e-07 | 0.09084 | 6.752% |
| Waist-Pelvis Ratio | R1 SDF + Surface Nets | yes | 2 | 0 | 0 | 0 | 22 | 0.000e+00 | 2.389e-07 | 0.21647 | 2.709% |
| Waist-Pelvis Ratio | R2 quad template projected on SDF | yes | 2 | 0 | 0 | 0 | 22 | 0.000e+00 | 1.231e-07 | 0.18518 | 8.246% |
| Waist-Pelvis Ratio | R3 quad template on B-spline control lattice | yes | 2 | 0 | 0 | 0 | 22 | 0.000e+00 | 1.204e-07 | 0.07899 | 2.968% |

## Silhouette deviation vs R1, seven fixed views (metres)

Orthographic projection, identical for every route (SPEC 8.4). R1 is the reference because it defines the target reference surface.

| Group | Route | View | Compared bins | p50 | p95 | max abs | mean abs |
|---|---|---|---|---|---|---|---|
| Baseline | R2 quad template projected on SDF | front | 170 | -0.02230 | 0.02659 | 0.25706 | 0.06680 |
| Baseline | R3 quad template on B-spline control lattice | front | 152 | -0.07058 | 0.01521 | 0.25440 | 0.09452 |
| Baseline | R2 quad template projected on SDF | front-45-left | 166 | -0.02274 | 0.00650 | 0.19573 | 0.04610 |
| Baseline | R3 quad template on B-spline control lattice | front-45-left | 168 | -0.08829 | 0.00409 | 0.16619 | 0.07210 |
| Baseline | R2 quad template projected on SDF | side-left | 121 | -0.00238 | 0.01501 | 0.10137 | 0.01431 |
| Baseline | R3 quad template on B-spline control lattice | side-left | 143 | -0.00003 | 0.05787 | 0.09029 | 0.02759 |
| Baseline | R2 quad template projected on SDF | side-left-30up | 164 | -0.00208 | 0.00591 | 0.23186 | 0.01236 |
| Baseline | R3 quad template on B-spline control lattice | side-left-30up | 176 | -0.00343 | 0.04651 | 0.23535 | 0.03295 |
| Baseline | R2 quad template projected on SDF | back | 170 | -0.02230 | 0.02659 | 0.25706 | 0.06680 |
| Baseline | R3 quad template on B-spline control lattice | back | 152 | -0.07058 | 0.01521 | 0.25440 | 0.09452 |
| Baseline | R2 quad template projected on SDF | back-45-right | 166 | -0.02274 | 0.00650 | 0.19573 | 0.04614 |
| Baseline | R3 quad template on B-spline control lattice | back-45-right | 168 | -0.08829 | 0.00409 | 0.16619 | 0.07210 |
| Baseline | R2 quad template projected on SDF | front-30up | 170 | -0.01642 | 0.02467 | 0.23651 | 0.06148 |
| Baseline | R3 quad template on B-spline control lattice | front-30up | 168 | -0.08684 | 0.02869 | 0.20909 | 0.08866 |
| Chest Width-Depth | R2 quad template projected on SDF | front | 166 | -0.01599 | 0.02984 | 0.25699 | 0.06694 |
| Chest Width-Depth | R3 quad template on B-spline control lattice | front | 152 | -0.08621 | 0.00878 | 0.25392 | 0.09344 |
| Chest Width-Depth | R2 quad template projected on SDF | front-45-left | 164 | -0.01605 | 0.00766 | 0.19598 | 0.04604 |
| Chest Width-Depth | R3 quad template on B-spline control lattice | front-45-left | 168 | -0.08661 | 0.00353 | 0.16979 | 0.07387 |
| Chest Width-Depth | R2 quad template projected on SDF | side-left | 126 | -0.00408 | 0.01451 | 0.10137 | 0.01601 |
| Chest Width-Depth | R3 quad template on B-spline control lattice | side-left | 146 | -0.00147 | 0.05789 | 0.11168 | 0.02871 |
| Chest Width-Depth | R2 quad template projected on SDF | side-left-30up | 166 | -0.00330 | 0.00585 | 0.23186 | 0.01524 |
| Chest Width-Depth | R3 quad template on B-spline control lattice | side-left-30up | 174 | -0.00611 | 0.05806 | 0.24812 | 0.03300 |
| Chest Width-Depth | R2 quad template projected on SDF | back | 166 | -0.01599 | 0.02984 | 0.25699 | 0.06694 |
| Chest Width-Depth | R3 quad template on B-spline control lattice | back | 152 | -0.08621 | 0.00878 | 0.25392 | 0.09344 |
| Chest Width-Depth | R2 quad template projected on SDF | back-45-right | 164 | -0.01605 | 0.00766 | 0.19598 | 0.04604 |
| Chest Width-Depth | R3 quad template on B-spline control lattice | back-45-right | 168 | -0.08661 | 0.00353 | 0.16979 | 0.07387 |
| Chest Width-Depth | R2 quad template projected on SDF | front-30up | 176 | -0.02006 | 0.02278 | 0.23007 | 0.06441 |
| Chest Width-Depth | R3 quad template on B-spline control lattice | front-30up | 170 | -0.07452 | 0.04731 | 0.20909 | 0.08563 |
| Waist-Pelvis Ratio | R2 quad template projected on SDF | front | 168 | -0.01409 | 0.02659 | 0.27034 | 0.05696 |
| Waist-Pelvis Ratio | R3 quad template on B-spline control lattice | front | 148 | -0.07881 | 0.00916 | 0.25793 | 0.10202 |
| Waist-Pelvis Ratio | R2 quad template projected on SDF | front-45-left | 164 | -0.01662 | 0.00733 | 0.18771 | 0.03860 |
| Waist-Pelvis Ratio | R3 quad template on B-spline control lattice | front-45-left | 165 | -0.08844 | 0.00291 | 0.21488 | 0.07644 |
| Waist-Pelvis Ratio | R2 quad template projected on SDF | side-left | 121 | -0.00288 | 0.01481 | 0.14447 | 0.01467 |
| Waist-Pelvis Ratio | R3 quad template on B-spline control lattice | side-left | 141 | 0.00066 | 0.06361 | 0.09346 | 0.03037 |
| Waist-Pelvis Ratio | R2 quad template projected on SDF | side-left-30up | 165 | -0.00328 | 0.00613 | 0.30350 | 0.01454 |
| Waist-Pelvis Ratio | R3 quad template on B-spline control lattice | side-left-30up | 176 | 0.00089 | 0.05829 | 0.24802 | 0.03361 |
| Waist-Pelvis Ratio | R2 quad template projected on SDF | back | 168 | -0.01409 | 0.02659 | 0.27034 | 0.05696 |
| Waist-Pelvis Ratio | R3 quad template on B-spline control lattice | back | 148 | -0.07881 | 0.00916 | 0.25793 | 0.10202 |
| Waist-Pelvis Ratio | R2 quad template projected on SDF | back-45-right | 164 | -0.01662 | 0.00733 | 0.18771 | 0.03864 |
| Waist-Pelvis Ratio | R3 quad template on B-spline control lattice | back-45-right | 165 | -0.08844 | 0.00291 | 0.21488 | 0.07644 |
| Waist-Pelvis Ratio | R2 quad template projected on SDF | front-30up | 172 | -0.01755 | 0.02127 | 0.19191 | 0.05232 |
| Waist-Pelvis Ratio | R3 quad template on B-spline control lattice | front-30up | 166 | -0.09960 | 0.01854 | 0.19397 | 0.09618 |

## Topology stability across parameter groups (SPEC 13 #10)

Same vertex ID across parameter groups must keep pointing at the same anatomical position; face adjacency must stay constant.

| Route | ID defined | Face adjacency constant | Mean vertex displacement | Max vertex displacement | Vertices moved > 1 mm |
|---|---|---|---|---|---|
| R1 SDF + Surface Nets (Chest Width-Depth) | NO | NO | 0.00000 | 0.00000 | 0/0 |
| R2 quad template projected on SDF (Chest Width-Depth) | yes | yes | 0.00594 | 0.12714 | 433/833 |
| R3 quad template on B-spline control lattice (Chest Width-Depth) | yes | yes | 0.00599 | 0.02757 | 416/833 |
| R1 SDF + Surface Nets (Waist-Pelvis Ratio) | NO | NO | 0.00000 | 0.00000 | 0/0 |
| R2 quad template projected on SDF (Waist-Pelvis Ratio) | yes | yes | 0.00602 | 0.15703 | 364/833 |
| R3 quad template on B-spline control lattice (Waist-Pelvis Ratio) | yes | yes | 0.00898 | 0.02534 | 724/833 |

## Parameter-group proportions (SPEC 13 #3 / #4)

Ring extents are measured on the shared target reference grid, at the same anatomical heights in every group. All thresholds are TBD until the owner approves them against DEC-05.

| Group | waist width (m) | chest width (m) | waist/chest | iliac crest width (m) | pelvic depth (m) | width/depth |
|---|---|---|---|---|---|---|
| Baseline | 0.2780 | 0.5766 | 0.4821 | 0.3356 | 0.2101 | 1.5977 |
| Chest Width-Depth | 0.2782 | 0.5835 | 0.4767 | 0.3356 | 0.2101 | 1.5977 |
| Waist-Pelvis Ratio | 0.2582 | 0.5766 | 0.4478 | 0.6002 | 0.2249 | 2.6687 |

## Parameter region impact (SPEC 13 #9)

Displacement against the Baseline group. `stable region` is the part of the torso the changed parameters do NOT act on: the lower torso (below the spine landmark) for Chest Width-Depth, and the upper chest (above spine + 50 mm) for Waist-Pelvis Ratio. A displacement >1 mm there is an illegal cross-region influence. The 1 mm guard is a measurement guard, NOT an approved threshold. R1 shows 0/0 because its vertex count changes between parameter groups, so vertex-ID comparison is UNDEFINED - this is NOT a claim that R1 did not move.

| Group | Route | moved | total | mean (m) | max (m) | stable max (m) | stable >1 mm |
|---|---|---|---|---|---|---|---|
| Chest Width-Depth | R1 SDF + Surface Nets | 0 | 0 | 0.00000 | 0.00000 | 0.00000 | 0 |
| Chest Width-Depth | R2 quad template projected on SDF | 514 | 833 | 0.00594 | 0.12714 | 0.00450 | 33 |
| Chest Width-Depth | R3 quad template on B-spline control lattice | 480 | 833 | 0.00599 | 0.02757 | 0.00002 | 0 |
| Waist-Pelvis Ratio | R1 SDF + Surface Nets | 0 | 0 | 0.00000 | 0.00000 | 0.00000 | 0 |
| Waist-Pelvis Ratio | R2 quad template projected on SDF | 483 | 833 | 0.00602 | 0.15703 | 0.00007 | 0 |
| Waist-Pelvis Ratio | R3 quad template on B-spline control lattice | 798 | 833 | 0.00898 | 0.02534 | 0.01486 | 250 |

## Volume (SPEC 13 #2 / #8)

Signed triangle sum. A mesh with a registered crop boundary is open, so the value is the volume of the surface closed by the crop plane - usable for relative comparison, not as an absolute body volume.

| Group | Route | Volume (m3) | Delta vs R1 |
|---|---|---|---|
| Baseline | R1 SDF + Surface Nets | 0.044832 | +0.00% |
| Baseline | R2 quad template projected on SDF | 0.023452 | -47.69% |
| Baseline | R3 quad template on B-spline control lattice | 0.026083 | -41.82% |
| Chest Width-Depth | R1 SDF + Surface Nets | 0.047088 | +0.00% |
| Chest Width-Depth | R2 quad template projected on SDF | 0.022718 | -51.75% |
| Chest Width-Depth | R3 quad template on B-spline control lattice | 0.020037 | -57.45% |
| Waist-Pelvis Ratio | R1 SDF + Surface Nets | 0.046654 | +0.00% |
| Waist-Pelvis Ratio | R2 quad template projected on SDF | 0.037980 | -18.59% |
| Waist-Pelvis Ratio | R3 quad template on B-spline control lattice | 0.037661 | -19.28% |
