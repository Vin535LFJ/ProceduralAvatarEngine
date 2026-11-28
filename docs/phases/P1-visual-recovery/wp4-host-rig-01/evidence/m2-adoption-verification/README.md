# M2 Offset Adoption Verification Evidence Index

Generated 2026-11-28 by `pae_wp4_adopt` (host-only, no DQS). Base `47d462d`.
37 images losslessly recompressed (pixel-roundtrip verified), 0.71 MB.

## CORRECTED CONCLUSION
The shoulder proximal-10mm offset is NOT adopted by default. Fixed-mask verification
(same skin patch for legacy v2.1 vs offset v2.2) shows a trade-off, not improvement;
the earlier "halves stretch" (12.75x->5.87x) was a measurement-mask artifact.

## data/
- adoption-sync.csv — 9 shapes: joints/rig/bindErr/unbound/L-R asym/offset magnitude (all 0.01000m, correct).
- pose-regression.csv — 22 poses legacy vs offset, FIXED mask: flips / edgeP99 / axilla edgeMax / axilla flips.
- adoption-verification.md — offset magnitude + 9-shape sync + full-pose regression tables.

## images/
- shape-{9 shapes}-clay.png — adopted rig bind per shape.
- adopted-{22 poses}-clay.png (+ shoulder closeups).
- legacy-shoulder_abduction_90-{clay,closeup}.png — comparison baseline.

## logs/
experiment-run.log, wp4-tests.log (16/16), wp3-tests.log (33/33), ht-tests.log (65/65), engine-ctest.log (22/22).

## Key numbers (fixed mask, abduction 90)
flips 64->70 (worse), axilla flips 18->24 (worse), axilla edgeMax 12.75x->14.82x (worse).
Abduction 30/60: flips reduce (8->2, 24->14) but peak stretch rises. Net: not adopted.

See ../04-m2-offset-verification-report.md.