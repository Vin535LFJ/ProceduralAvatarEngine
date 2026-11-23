# G1 route metrics (host probe)

Target reference: anatomy SDF isosurface, torso + shoulder-root region, 2066 samples.
Anatomy resolution: 44x66x26.

## Template resolution 10 rings x 16 segments (coarse, unit-test spec)

| Route | Status | Verts | Tris | Quads | Components | Boundary | NonManifold | Degenerate | MeanEdge(m) | E1 mean(m) | E1 max(m) |
|---|---|---|---|---|---|---|---|---|---|---|---|
| R1 SDF + Surface Nets | FEASIBLE | 5108 | 10212 | 5106 | 1 | 0 | 0 | 0 | 0.017 | 0.0000 | 0.0000 |
| R2 quad template projected on SDF | FEASIBLE | 161 | 304 | 144 | 1 | 16 | 0 | 0 | 0.066 | 0.0252 | 0.0710 |
| R3 quad template on B-spline control lattice | FEASIBLE | 161 | 304 | 144 | 1 | 16 | 0 | 0 | 0.060 | 0.0432 | 0.1379 |

- R1 SDF + Surface Nets: generation 196 ms
- R2 quad template projected on SDF: generation 10 ms
- R3 quad template on B-spline control lattice: generation 0 ms

## Template resolution 26 rings x 32 segments (production PoC-A spec)

| Route | Status | Verts | Tris | Quads | Components | Boundary | NonManifold | Degenerate | MeanEdge(m) | E1 mean(m) | E1 max(m) |
|---|---|---|---|---|---|---|---|---|---|---|---|
| R1 SDF + Surface Nets | FEASIBLE | 5108 | 10212 | 5106 | 1 | 0 | 0 | 0 | 0.017 | 0.0000 | 0.0000 |
| R2 quad template projected on SDF | FEASIBLE | 833 | 1632 | 800 | 1 | 32 | 0 | 0 | 0.031 | 0.0116 | 0.0387 |
| R3 quad template on B-spline control lattice | FEASIBLE | 833 | 1632 | 800 | 1 | 32 | 0 | 0 | 0.026 | 0.0350 | 0.1331 |

- R1 SDF + Surface Nets: generation 213 ms
- R2 quad template projected on SDF: generation 56 ms
- R3 quad template on B-spline control lattice: generation 0 ms

## Topology stability under a shape change

Shape change: chestWidth 1.00 -> 1.15, pelvisWidth 1.00 -> 1.12.

| Route | Verts before | Verts after | Vertex set fixed | Index-wise delta well defined |
|---|---|---|---|---|
| R1 SDF + Surface Nets | 5108 | 5342 | NO | NO |
| R2 quad template projected on SDF | 833 | 833 | yes | yes |
| R3 quad template on B-spline control lattice | 833 | 833 | yes | yes |

## E1 error distribution by height band (production template)

A DISCRETISATION error shrinks as the template resolution goes up; a MODEL error (the route's shape model cannot represent the anatomy) does not.

**R2 quad template projected on SDF** - E1 by height band (metres):

| Height band | Samples | E1 mean | E1 max |
|---|---|---|---|
| y=[0.85, 0.95) | 440 | 0.0093 | 0.0210 |
| y=[0.95, 1.05) | 350 | 0.0082 | 0.0170 |
| y=[1.05, 1.15) | 552 | 0.0124 | 0.0365 |
| y=[1.15, 1.25) | 584 | 0.0143 | 0.0387 |
| y=[1.25, 1.35) | 92 | 0.0143 | 0.0359 |

**R3 quad template on B-spline control lattice** - E1 by height band (metres):

| Height band | Samples | E1 mean | E1 max |
|---|---|---|---|
| y=[0.85, 0.95) | 440 | 0.0142 | 0.0304 |
| y=[0.95, 1.05) | 350 | 0.0115 | 0.0300 |
| y=[1.05, 1.15) | 552 | 0.0454 | 0.1331 |
| y=[1.15, 1.25) | 584 | 0.0549 | 0.1208 |
| y=[1.25, 1.35) | 92 | 0.0483 | 0.1035 |
