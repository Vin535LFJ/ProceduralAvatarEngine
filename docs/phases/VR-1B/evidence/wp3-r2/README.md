# WP3-R2 (M1) Evidence Index

Generated 2026-11-28 by `pae_wp3_r2_experiment` (host-only). Base `d40e213`.
18 images losslessly recompressed (pixel-roundtrip verified), 0.37 MB.

## data/
- solve-cases.csv — every solve case: reachable / rmsRelResidual / iters / forwardEvals / flips / minNormalDot / edgeP99 / volumeRatio / 8 targets / 8 achieved / 6 alpha.
- base-measures.md — base semantic measurements.
- known-solution.md — recovery rms + maxAlphaErr.
- non-additivity.md — shoulder×ribcage coupling (small, honest) + solver recovery.
- determinism.md — identical alpha across two solves.
- profiles.csv — 8 profiles + hash + achieved measures.

## profiles/
8 saveable profiles (.txt): templateSha / shapeTargetHash / profileHash / alpha / stature / achieved.

## images/
profile-{baseline,broad-shoulder,broad-ribcage,narrow-waist,wide-hip,tall,short,combo-athletic,combo-pear}-{clay,wire}.png.

## logs/
experiment-run.log, wp3-tests.log (33/33), wp4-tests.log (16/16), ht-tests.log (65/65), engine-ctest.log (22/22).

## Key findings
- Known-solution rms ≤0.45%, maxAlphaErr ≤0.077; holdout rms ≤0.30%; unreachable correctly flagged; determinism exact; 0 flips.
- Identifiability: armShape is null-space wrt the 8 measures; Tikhonov regularization keeps it near 0.
- shape-shape non-additivity small (≤0.1mm); dominant coupling is stature×shape (handled by forward model).

See ../01a-wp3-r2-implementation-report.md.