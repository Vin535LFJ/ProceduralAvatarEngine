# M6 Finger Refinement Evidence Index

Generated 2026-11-28 by `pae_wp6_finger` (host-only, no DQS). Base `6f4e750`.
4 images losslessly recompressed (pixel-roundtrip verified).

## NEGATIVE RESULT
Uniform subdivision (adds finger loops) does NOT fix fist: flip RATE unchanged
(7.89% -> 7.99%), per-finger flips scale ~4x with face count. Root cause is
inter-finger geometric interference (penetration), resolution-independent.

## data/
- finger-topology.md — per-finger vertex/face counts (base mesh).
- fist-refinement.csv / .md — fist + finger_curl, base vs subdiv, flip RATE normalized + per-finger.
- subdiv-purity.md — subdivided finger weight purity 0.9279 (confound check; base ~1.00).

## images/
fist-{base,subdiv}-closeup.png, finger_curl-{base,subdiv}-closeup.png (hand region).

## logs/
experiment-run.log, wp4-tests.log (16/16), wp3-tests.log (33/33), ht-tests.log (65/65), engine-ctest.log (22/22).

See ../07a-m6-finger-refinement-report.md.