# M2 Quality Improvement Evidence Index

Generated 2026-11-28 by `pae_wp4_q2` (host-only, no DQS). Base `7282baa`.
18 images losslessly recompressed (pixel-roundtrip verified), 0.27 MB.

## data/
- shoulder-geometry.md — acromion vs helper centroid vs humerus axis (measured).
- shoulder-offset.csv — 5 offsets × abduction 30/60/90 axilla metrics.
- extreme-attribution.csv / .md — per-region flips/edge/area/weightPurity for fist/elbow_120/arms_raised/squat.
- selfint-localization.md — self-intersection pairs localized to region×region.
- per-finger.csv — fist flips per finger (L/R symmetric).

## images/
- shoulder-{baseline,medial-15mm,inferior-15mm,medial-inferior-15mm,proximal-10mm}-{clay,closeup}.png (abduction 90°).
- extreme-{fist,elbow_120,arms_raised,basic_squat}-{clay,closeup}.png.

## logs/
experiment-run.log, wp4-tests.log (16/16), wp3-tests.log (33/33), engine-ctest.log (22/22).

## Key findings
- Shoulder proximal-10mm offset halves abduction axilla edge stretch (edgeMax 12.75x -> 5.87x).
- Weight purity 0.98-1.00 rules out binding contamination; extreme flips = geometric penetration + fold.
- fist self-int all hand x hand; elbow_120 self-int 0 (pure fold); arms_raised shoulder x upper-arm; squat lower-leg x lower-leg.

See ../03-m2-quality-improvement-report.md.