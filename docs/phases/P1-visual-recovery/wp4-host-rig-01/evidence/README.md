# WP4-HOST-RIG-01 Evidence Index

Generated 2026-11-28 by `pae_wp4_experiment` (host-only). Base `bfcada7`.
All images losslessly recompressed from PAE's uncompressed PNG writer (pixel-roundtrip verified); 184 files, 4.4 MB.

## data/
| file | content |
|---|---|
| skeleton-definitions.md | core20 + full92 semantic tables (id/parent/role/group/head/tail/length) |
| joint-table.csv | resolved joint XYZ (metres) + root anchor |
| joint-deviation-vs-reference.csv | PAE resolver vs PENDING mhskel reference (host research only) |
| weight-diagnostics.md | Distance / Heat / Region comparison |
| weight-table.csv | per-bone weight mass (distance vs region) |
| pose-inputs.md | every pose as Bone Local Transforms |
| deformation-metrics.csv | per-pose global flips/selfInt/edge/volume/cpu |
| region-metrics.csv | per-pose PER-JOINT-REGION metrics (the honest picture) |
| shape-sync.csv | 9 shapes × joint/rig/bind/weight/pose |
| core-vs-full.csv | core20 vs full92 comparison |
| assertions.md | direction assertions (0 fail) |

## images/
- `base-<pose>-{clay,wire,normal,heat,overlay}.png` — 22 poses, front view; overlay = skeleton on clay.
- `base-<pose>-closeup.png` — shoulder/elbow/fist joint close-ups.
- `shape-<name>-bind-clay.png` — 9 shapes at bind.
- `combo-athletic-<pose>-clay.png`, `wider-hip-<pose>-clay.png` — 2 extra shapes × 22 poses.
- `cmp-{core20,full92}-<pose>-clay.png` — profile comparison × 6 poses.

## logs/
experiment-run.log, wp4-tests.log (16/16), wp3-tests.log (33/33), ht-tests.log (65/65), engine-ctest.log (22/22).

## Key honest findings
- Bind recovery 3.675e-08 m; L/R symmetry 0.0; Region weights 0 unbound/0 cross-side/0 far-bound.
- Moderate poses usable with minor joint artifacts; EXTREME poses fail (fist 1056 flips, elbow_120 3138, arms_raised 3112, squat 1517) — LBS + base-mesh limit, stop condition #5.
- Per-region: shoulder abduction axilla edge stretch 12.7×; elbow_90 area ratio 0.72; knee_90 0.67; fist fingers 528 flips/side.
- core20 cannot express fist (no finger bones); its "0 flips" for fist is an artifact, not quality.

See ../02-wp4-host-rig-report.md for the full report and final status fields.