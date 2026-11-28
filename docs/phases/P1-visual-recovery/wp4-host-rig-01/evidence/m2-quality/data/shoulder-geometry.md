# M2 shoulder joint-center geometry (generated)

- joint-l-shoulder helper centroid (m): (0.1677, 0.5246, 0.0146)
- acromion surface landmark (m): (0.1537, 0.5742, 0.0109) vertex 8226
- acromion -> helper centroid offset (m): (+0.0140, -0.0496, +0.0037)  [helper is 49.6 mm inferior, 14.0 mm lateral to acromion]
- humerus axis (shoulder->elbow) unit: (0.638, -0.770, -0.006)
- Finding: the CC0 helper centroid is already internal/inferior to the acromion surface landmark, NOT naively at the surface. The offset sweep below tests whether moving the rotation center further improves abduction axilla quality.

| offset | abduction | axilla flips | axilla edgeP99 | axilla edgeMax | axilla areaRatio | global flips |
|---|---|---|---|---|---|---|
| baseline | shoulder_abduction_30 | 4 | 3.941 | 5.344 | 1.030 | 8 |
| baseline | shoulder_abduction_60 | 10 | 6.703 | 9.362 | 1.106 | 24 |
| baseline | shoulder_abduction_90 | 18 | 9.018 | 12.745 | 1.185 | 64 |
| medial-15mm | shoulder_abduction_30 | 5 | 2.972 | 4.176 | 1.042 | 11 |
| medial-15mm | shoulder_abduction_60 | 11 | 4.947 | 7.308 | 1.135 | 33 |
| medial-15mm | shoulder_abduction_90 | 12 | 6.613 | 9.963 | 1.234 | 74 |
| inferior-15mm | shoulder_abduction_30 | 6 | 3.173 | 4.322 | 1.031 | 12 |
| inferior-15mm | shoulder_abduction_60 | 10 | 5.222 | 7.392 | 1.114 | 26 |
| inferior-15mm | shoulder_abduction_90 | 18 | 6.928 | 9.961 | 1.196 | 66 |
| medial-inferior-15mm | shoulder_abduction_30 | 3 | 3.463 | 4.834 | 1.039 | 16 |
| medial-inferior-15mm | shoulder_abduction_60 | 6 | 5.864 | 8.481 | 1.105 | 36 |
| medial-inferior-15mm | shoulder_abduction_90 | 9 | 7.913 | 11.560 | 1.163 | 74 |
| proximal-10mm | shoulder_abduction_30 | 1 | 2.449 | 2.717 | 1.020 | 2 |
| proximal-10mm | shoulder_abduction_60 | 4 | 4.269 | 4.328 | 1.056 | 14 |
| proximal-10mm | shoulder_abduction_90 | 17 | 5.657 | 5.866 | 1.115 | 70 |
