# M2-POSE-FRAME-AUDIT-01 Evidence Index

Generated 2026-11-28. Base `e636b8c`. Host-only, no DQS/Android.
54 images losslessly recompressed (pixel-roundtrip verified).

## POSE_FRAME_AUDIT = PASS (math correctness; NOT final deformation quality)

## logs/
- tests-before-fix.log — 24 pass / 6 fail (oblique tests reproduce the world-axis bug).
- tests-after-fix.log / wp4-tests-after.log — 30/30 (fix + 14 non-axis-aligned tests).
- wp3-tests.log (33/33), ht-tests.log (65/65), engine-ctest.log (22/22).
- pose-regression-run.log — 22-pose Before/After run.

## data/
- pose-before-after.csv — 22 poses x {before,after}: flips/flipRate/selfInt/minNormalDot/edgeP95/P99/max/volumeRatio/cpuMs + 14 regions x (flips/edgeMax/areaRatio).
- pose-before-after.md — summary table.

## images/
9 key poses (bind/fist/finger_curl/elbow_90/elbow_120/shoulder_abduction_60/90/arms_raised/basic_squat)
x {before,after} x {clay,wire,closeup}.

## Key findings
- Bug: rl used rest WORLD axes (rig.ax/ay/az) right-multiplied -> skin rotated about R*axis. Fixed to local unit axes.
- elbow_120 flips 3138->1492 (-52%, coordinate error contributed).
- fist selfInt 130->1088, finger_curl 0->318: correct kinematics EXPOSES real inter-finger geometric penetration (needs M6 separation/limits, not coordinate fix).

See ../02a-m2-pose-frame-audit-report.md.