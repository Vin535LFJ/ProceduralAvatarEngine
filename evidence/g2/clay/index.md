# G2 grey-clay fixed-view evidence (PoC-A)

Software Lambert-grey render, NOT the production GLES renderer. The camera is the same `viewFrame()` used by the numeric silhouette protocol; the light, band and resolution are identical for every route and parameter group, and the framing is identical for every route within a view. The owner keeps an independent visual veto - no image here is marked ACCEPTED.

- Band: y = 0.8344 .. 1.2640 m
- Image: 480 x 480 px
- Light in view frame (right, up, towards camera): (0.55, 0.78, 0.45)
- Lambert: 0.28 ambient + 0.72 * max(0, N.L); background (28,28,31)

The frame is shared by every route and group WITHIN a view, but may differ between views so each view is framed tightly.

| View | u range (m) | v range (m) |
|---|---|---|
| front | [-0.3572, 0.3572] | [0.7962, 1.3021] |
| front-45-left | [-0.2644, 0.2705] | [0.8058, 1.2925] |
| side-left | [-0.1624, 0.1753] | [0.8087, 1.2896] |
| side-left-30up | [-0.1763, 0.1892] | [0.5321, 1.2727] |
| back | [-0.3572, 0.3572] | [0.7962, 1.3021] |
| back-45-right | [-0.2705, 0.2644] | [0.8058, 1.2925] |
| front-30up | [-0.3572, 0.3572] | [0.6249, 1.1925] |

| Group | Route | View | Image |
|---|---|---|---|
| Baseline | R1 SDF + Surface Nets | front | [clay/Baseline/r1/front.png](clay/Baseline/r1/front.png) |
| Baseline | R1 SDF + Surface Nets | front-45-left | [clay/Baseline/r1/front-45-left.png](clay/Baseline/r1/front-45-left.png) |
| Baseline | R1 SDF + Surface Nets | side-left | [clay/Baseline/r1/side-left.png](clay/Baseline/r1/side-left.png) |
| Baseline | R1 SDF + Surface Nets | side-left-30up | [clay/Baseline/r1/side-left-30up.png](clay/Baseline/r1/side-left-30up.png) |
| Baseline | R1 SDF + Surface Nets | back | [clay/Baseline/r1/back.png](clay/Baseline/r1/back.png) |
| Baseline | R1 SDF + Surface Nets | back-45-right | [clay/Baseline/r1/back-45-right.png](clay/Baseline/r1/back-45-right.png) |
| Baseline | R1 SDF + Surface Nets | front-30up | [clay/Baseline/r1/front-30up.png](clay/Baseline/r1/front-30up.png) |
| Baseline | R2 quad template projected on SDF | front | [clay/Baseline/r2/front.png](clay/Baseline/r2/front.png) |
| Baseline | R2 quad template projected on SDF | front-45-left | [clay/Baseline/r2/front-45-left.png](clay/Baseline/r2/front-45-left.png) |
| Baseline | R2 quad template projected on SDF | side-left | [clay/Baseline/r2/side-left.png](clay/Baseline/r2/side-left.png) |
| Baseline | R2 quad template projected on SDF | side-left-30up | [clay/Baseline/r2/side-left-30up.png](clay/Baseline/r2/side-left-30up.png) |
| Baseline | R2 quad template projected on SDF | back | [clay/Baseline/r2/back.png](clay/Baseline/r2/back.png) |
| Baseline | R2 quad template projected on SDF | back-45-right | [clay/Baseline/r2/back-45-right.png](clay/Baseline/r2/back-45-right.png) |
| Baseline | R2 quad template projected on SDF | front-30up | [clay/Baseline/r2/front-30up.png](clay/Baseline/r2/front-30up.png) |
| Baseline | R3 quad template on B-spline control lattice | front | [clay/Baseline/r3/front.png](clay/Baseline/r3/front.png) |
| Baseline | R3 quad template on B-spline control lattice | front-45-left | [clay/Baseline/r3/front-45-left.png](clay/Baseline/r3/front-45-left.png) |
| Baseline | R3 quad template on B-spline control lattice | side-left | [clay/Baseline/r3/side-left.png](clay/Baseline/r3/side-left.png) |
| Baseline | R3 quad template on B-spline control lattice | side-left-30up | [clay/Baseline/r3/side-left-30up.png](clay/Baseline/r3/side-left-30up.png) |
| Baseline | R3 quad template on B-spline control lattice | back | [clay/Baseline/r3/back.png](clay/Baseline/r3/back.png) |
| Baseline | R3 quad template on B-spline control lattice | back-45-right | [clay/Baseline/r3/back-45-right.png](clay/Baseline/r3/back-45-right.png) |
| Baseline | R3 quad template on B-spline control lattice | front-30up | [clay/Baseline/r3/front-30up.png](clay/Baseline/r3/front-30up.png) |
| Chest Width-Depth | R1 SDF + Surface Nets | front | [clay/Chest-Width-Depth/r1/front.png](clay/Chest-Width-Depth/r1/front.png) |
| Chest Width-Depth | R1 SDF + Surface Nets | front-45-left | [clay/Chest-Width-Depth/r1/front-45-left.png](clay/Chest-Width-Depth/r1/front-45-left.png) |
| Chest Width-Depth | R1 SDF + Surface Nets | side-left | [clay/Chest-Width-Depth/r1/side-left.png](clay/Chest-Width-Depth/r1/side-left.png) |
| Chest Width-Depth | R1 SDF + Surface Nets | side-left-30up | [clay/Chest-Width-Depth/r1/side-left-30up.png](clay/Chest-Width-Depth/r1/side-left-30up.png) |
| Chest Width-Depth | R1 SDF + Surface Nets | back | [clay/Chest-Width-Depth/r1/back.png](clay/Chest-Width-Depth/r1/back.png) |
| Chest Width-Depth | R1 SDF + Surface Nets | back-45-right | [clay/Chest-Width-Depth/r1/back-45-right.png](clay/Chest-Width-Depth/r1/back-45-right.png) |
| Chest Width-Depth | R1 SDF + Surface Nets | front-30up | [clay/Chest-Width-Depth/r1/front-30up.png](clay/Chest-Width-Depth/r1/front-30up.png) |
| Chest Width-Depth | R2 quad template projected on SDF | front | [clay/Chest-Width-Depth/r2/front.png](clay/Chest-Width-Depth/r2/front.png) |
| Chest Width-Depth | R2 quad template projected on SDF | front-45-left | [clay/Chest-Width-Depth/r2/front-45-left.png](clay/Chest-Width-Depth/r2/front-45-left.png) |
| Chest Width-Depth | R2 quad template projected on SDF | side-left | [clay/Chest-Width-Depth/r2/side-left.png](clay/Chest-Width-Depth/r2/side-left.png) |
| Chest Width-Depth | R2 quad template projected on SDF | side-left-30up | [clay/Chest-Width-Depth/r2/side-left-30up.png](clay/Chest-Width-Depth/r2/side-left-30up.png) |
| Chest Width-Depth | R2 quad template projected on SDF | back | [clay/Chest-Width-Depth/r2/back.png](clay/Chest-Width-Depth/r2/back.png) |
| Chest Width-Depth | R2 quad template projected on SDF | back-45-right | [clay/Chest-Width-Depth/r2/back-45-right.png](clay/Chest-Width-Depth/r2/back-45-right.png) |
| Chest Width-Depth | R2 quad template projected on SDF | front-30up | [clay/Chest-Width-Depth/r2/front-30up.png](clay/Chest-Width-Depth/r2/front-30up.png) |
| Chest Width-Depth | R3 quad template on B-spline control lattice | front | [clay/Chest-Width-Depth/r3/front.png](clay/Chest-Width-Depth/r3/front.png) |
| Chest Width-Depth | R3 quad template on B-spline control lattice | front-45-left | [clay/Chest-Width-Depth/r3/front-45-left.png](clay/Chest-Width-Depth/r3/front-45-left.png) |
| Chest Width-Depth | R3 quad template on B-spline control lattice | side-left | [clay/Chest-Width-Depth/r3/side-left.png](clay/Chest-Width-Depth/r3/side-left.png) |
| Chest Width-Depth | R3 quad template on B-spline control lattice | side-left-30up | [clay/Chest-Width-Depth/r3/side-left-30up.png](clay/Chest-Width-Depth/r3/side-left-30up.png) |
| Chest Width-Depth | R3 quad template on B-spline control lattice | back | [clay/Chest-Width-Depth/r3/back.png](clay/Chest-Width-Depth/r3/back.png) |
| Chest Width-Depth | R3 quad template on B-spline control lattice | back-45-right | [clay/Chest-Width-Depth/r3/back-45-right.png](clay/Chest-Width-Depth/r3/back-45-right.png) |
| Chest Width-Depth | R3 quad template on B-spline control lattice | front-30up | [clay/Chest-Width-Depth/r3/front-30up.png](clay/Chest-Width-Depth/r3/front-30up.png) |
| Waist-Pelvis Ratio | R1 SDF + Surface Nets | front | [clay/Waist-Pelvis-Ratio/r1/front.png](clay/Waist-Pelvis-Ratio/r1/front.png) |
| Waist-Pelvis Ratio | R1 SDF + Surface Nets | front-45-left | [clay/Waist-Pelvis-Ratio/r1/front-45-left.png](clay/Waist-Pelvis-Ratio/r1/front-45-left.png) |
| Waist-Pelvis Ratio | R1 SDF + Surface Nets | side-left | [clay/Waist-Pelvis-Ratio/r1/side-left.png](clay/Waist-Pelvis-Ratio/r1/side-left.png) |
| Waist-Pelvis Ratio | R1 SDF + Surface Nets | side-left-30up | [clay/Waist-Pelvis-Ratio/r1/side-left-30up.png](clay/Waist-Pelvis-Ratio/r1/side-left-30up.png) |
| Waist-Pelvis Ratio | R1 SDF + Surface Nets | back | [clay/Waist-Pelvis-Ratio/r1/back.png](clay/Waist-Pelvis-Ratio/r1/back.png) |
| Waist-Pelvis Ratio | R1 SDF + Surface Nets | back-45-right | [clay/Waist-Pelvis-Ratio/r1/back-45-right.png](clay/Waist-Pelvis-Ratio/r1/back-45-right.png) |
| Waist-Pelvis Ratio | R1 SDF + Surface Nets | front-30up | [clay/Waist-Pelvis-Ratio/r1/front-30up.png](clay/Waist-Pelvis-Ratio/r1/front-30up.png) |
| Waist-Pelvis Ratio | R2 quad template projected on SDF | front | [clay/Waist-Pelvis-Ratio/r2/front.png](clay/Waist-Pelvis-Ratio/r2/front.png) |
| Waist-Pelvis Ratio | R2 quad template projected on SDF | front-45-left | [clay/Waist-Pelvis-Ratio/r2/front-45-left.png](clay/Waist-Pelvis-Ratio/r2/front-45-left.png) |
| Waist-Pelvis Ratio | R2 quad template projected on SDF | side-left | [clay/Waist-Pelvis-Ratio/r2/side-left.png](clay/Waist-Pelvis-Ratio/r2/side-left.png) |
| Waist-Pelvis Ratio | R2 quad template projected on SDF | side-left-30up | [clay/Waist-Pelvis-Ratio/r2/side-left-30up.png](clay/Waist-Pelvis-Ratio/r2/side-left-30up.png) |
| Waist-Pelvis Ratio | R2 quad template projected on SDF | back | [clay/Waist-Pelvis-Ratio/r2/back.png](clay/Waist-Pelvis-Ratio/r2/back.png) |
| Waist-Pelvis Ratio | R2 quad template projected on SDF | back-45-right | [clay/Waist-Pelvis-Ratio/r2/back-45-right.png](clay/Waist-Pelvis-Ratio/r2/back-45-right.png) |
| Waist-Pelvis Ratio | R2 quad template projected on SDF | front-30up | [clay/Waist-Pelvis-Ratio/r2/front-30up.png](clay/Waist-Pelvis-Ratio/r2/front-30up.png) |
| Waist-Pelvis Ratio | R3 quad template on B-spline control lattice | front | [clay/Waist-Pelvis-Ratio/r3/front.png](clay/Waist-Pelvis-Ratio/r3/front.png) |
| Waist-Pelvis Ratio | R3 quad template on B-spline control lattice | front-45-left | [clay/Waist-Pelvis-Ratio/r3/front-45-left.png](clay/Waist-Pelvis-Ratio/r3/front-45-left.png) |
| Waist-Pelvis Ratio | R3 quad template on B-spline control lattice | side-left | [clay/Waist-Pelvis-Ratio/r3/side-left.png](clay/Waist-Pelvis-Ratio/r3/side-left.png) |
| Waist-Pelvis Ratio | R3 quad template on B-spline control lattice | side-left-30up | [clay/Waist-Pelvis-Ratio/r3/side-left-30up.png](clay/Waist-Pelvis-Ratio/r3/side-left-30up.png) |
| Waist-Pelvis Ratio | R3 quad template on B-spline control lattice | back | [clay/Waist-Pelvis-Ratio/r3/back.png](clay/Waist-Pelvis-Ratio/r3/back.png) |
| Waist-Pelvis Ratio | R3 quad template on B-spline control lattice | back-45-right | [clay/Waist-Pelvis-Ratio/r3/back-45-right.png](clay/Waist-Pelvis-Ratio/r3/back-45-right.png) |
| Waist-Pelvis Ratio | R3 quad template on B-spline control lattice | front-30up | [clay/Waist-Pelvis-Ratio/r3/front-30up.png](clay/Waist-Pelvis-Ratio/r3/front-30up.png) |
