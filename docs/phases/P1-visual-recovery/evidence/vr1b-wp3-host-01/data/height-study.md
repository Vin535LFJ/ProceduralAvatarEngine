# WP3 stature special study (generated)

## Baseline difference

- MakeHuman neutral base mesh stature: **1.6659 m** (this study's reference).
- CharacterDefinition statureM default: **1.7549 m** = ANSUR II male 50th percentile (SRC-ANTH-01), a different reference population. The 89 mm gap is a caliber difference, not a defect.
- Macro height +1 alone gives ~2.3884 m (+0.7225 m): the maxheight target encodes MakeHuman's full height-slider range inside its macro combination (muscle/weight/age/proportion), NOT a calibrated metre scale. Standalone anthropometric correctness is **NOT VERIFIED**.

## A. MakeHuman macro height curve (standalone, research only)

| alpha | stature (m) | flips | selfInt | edgeP99 | upperArm/stature | thigh/stature |
|---:|---:|---:|---:|---:|---:|---:|
| -1.00 | 1.2995 | 0 | 0 | 1.000 | 0.1330 | 0.2346 |
| -0.50 | 1.4827 | 0 | 0 | 1.000 | 0.1324 | 0.2431 |
| -0.25 | 1.5743 | 0 | 0 | 1.000 | 0.1321 | 0.2467 |
| +0.00 | 1.6659 | 0 | 0 | 1.000 | 0.1319 | 0.2498 |
| +0.25 | 1.8465 | 0 | 0 | 1.000 | 0.1366 | 0.2551 |
| +0.50 | 2.0271 | 0 | 0 | 1.000 | 0.1406 | 0.2595 |
| +1.00 | 2.3884 | 0 | 0 | 1.000 | 0.1467 | 0.2662 |

## B. PAE own height transforms (full OBJ incl. helper/joint, joints re-resolved)

Uniform scale about the sole plane preserves all proportions and scales joint heights exactly; vertical-only scale stretches y without x/z (distorts proportions).

| Method | Target (m) | Achieved | upperArm/stature | thigh/stature | shoulderJointH | hipJointH | flips | selfInt | edgeP99 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| uniform | 1.5500 | 1.5500 | 0.1319 | 0.2498 | 1.2279 | 0.7568 | 0 | 0 | 1.000 |
| uniform | 1.6659 | 1.6659 | 0.1319 | 0.2498 | 1.3197 | 0.8133 | 0 | 0 | 1.000 |
| uniform | 1.7549 | 1.7549 | 0.1319 | 0.2498 | 1.3902 | 0.8568 | 0 | 0 | 1.000 |
| uniform | 1.8500 | 1.8500 | 0.1319 | 0.2498 | 1.4655 | 0.9032 | 0 | 0 | 1.000 |
| vertical | 1.5500 | 1.5500 | 0.1362 | 0.2501 | 1.2279 | 0.7568 | 0 | 0 | 1.000 |
| vertical | 1.6659 | 1.6659 | 0.1319 | 0.2498 | 1.3197 | 0.8133 | 0 | 0 | 1.000 |
| vertical | 1.7549 | 1.7549 | 0.1291 | 0.2497 | 1.3902 | 0.8568 | 0 | 0 | 1.000 |
| vertical | 1.8500 | 1.8500 | 0.1265 | 0.2495 | 1.4655 | 0.9032 | 0 | 0 | 1.000 |

## Conclusion

- Macro height standalone: NOT VERIFIED for anthropometric stature control (non-linear, proportion-shifting, authored for macro combinations). Do NOT wire statureM to it.
- PAE uniform scale about the sole: deterministic, proportion-preserving, exact joint sync, clean quality at 1.55-1.85 m. Recommended host-verified stature method for WP3-Production (with helper geometry scaled together).
- Vertical-only scale: changes limb/head proportions; recorded for comparison, not recommended.
