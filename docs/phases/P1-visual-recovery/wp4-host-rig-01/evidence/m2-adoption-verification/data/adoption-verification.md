# M2 shoulder offset adoption verification (generated)

- legacy rig version: pae-rig-v2.1
- adopted rig version: pae-rig-v2.2
- authored proximal offset: 0.0100 m along humerus axis
- measured upper_arm.L head shift (legacy->adopted): 0.01000 m
- adopted upper_arm.L head: (0.1613, 0.5323, 0.0147)
- offset magnitude correct (within 0.1mm): 1

## 9-shape joint sync (adopted v2.2)

| shape | joints | rig | bindErr | unbound | L/R asym | shoulder offset(m) | offset ok |
|---|---|---|---|---|---|---|---|
| baseline | 1 | 1 | 3.67e-08 | 0 | 0.000000 | 0.01000 | 1 |
| broader-shoulder | 1 | 1 | 3.67e-08 | 0 | 0.000000 | 0.01000 | 1 |
| broader-ribcage | 1 | 1 | 3.67e-08 | 0 | 0.000000 | 0.01000 | 1 |
| narrower-waist | 1 | 1 | 3.67e-08 | 0 | 0.000000 | 0.01000 | 1 |
| wider-hip | 1 | 1 | 3.67e-08 | 0 | 0.000000 | 0.01000 | 1 |
| taller | 1 | 1 | 3.69e-08 | 0 | 0.000000 | 0.01000 | 1 |
| shorter | 1 | 1 | 3.67e-08 | 0 | 0.000000 | 0.01000 | 1 |
| combo-athletic | 1 | 1 | 3.67e-08 | 0 | 0.000000 | 0.01000 | 1 |
| combo-pear | 1 | 1 | 3.67e-08 | 0 | 0.000000 | 0.01000 | 1 |

## Full-pose regression (legacy v2.1 vs adopted v2.2)

| pose | flips L->A | edgeP99 L->A | axilla edgeMax L->A | axilla flips L->A |
|---|---|---|---|---|
| bind | 0->0 | 1.000->1.000 | 1.000->1.000 | 0->0 |
| neutral_standing | 0->0 | 1.000->1.000 | 1.000->1.000 | 0->0 |
| shoulder_forward_flexion | 18->20 | 1.062->1.055 | 4.474->4.768 | 9->10 |
| shoulder_abduction_30 | 8->2 | 1.028->1.017 | 5.344->6.115 | 4->1 |
| shoulder_abduction_60 | 24->14 | 1.047->1.040 | 9.362->10.834 | 10->5 |
| shoulder_abduction_90 | 64->70 | 1.079->1.071 | 12.745->14.816 | 18->24 |
| elbow_0 | 0->0 | 1.000->1.000 | 1.000->1.000 | 0->0 |
| elbow_45 | 0->0 | 1.076->1.076 | 1.000->1.000 | 0->0 |
| elbow_90 | 28->26 | 1.268->1.268 | 1.000->1.000 | 0->0 |
| elbow_120 | 3138->3138 | 1.376->1.376 | 1.000->1.000 | 0->0 |
| hip_flexion | 19->19 | 1.000->1.000 | 1.000->1.000 | 0->0 |
| hip_abduction | 12->12 | 1.000->1.000 | 1.000->1.000 | 0->0 |
| knee_45 | 0->0 | 1.108->1.108 | 1.000->1.000 | 0->0 |
| knee_90 | 32->32 | 1.346->1.346 | 1.000->1.000 | 0->0 |
| wrist_flexion | 16->16 | 1.000->1.000 | 1.000->1.000 | 0->0 |
| wrist_extension | 20->20 | 1.000->1.000 | 1.000->1.000 | 0->0 |
| finger_curl | 34->34 | 1.300->1.300 | 1.000->1.000 | 0->0 |
| fist | 1056->1056 | 1.675->1.675 | 1.000->1.000 | 0->0 |
| arms_raised | 3112->3124 | 1.132->1.107 | 16.737->19.551 | 30->36 |
| one_arm_raised | 1556->1562 | 1.000->1.000 | 16.737->19.551 | 30->36 |
| basic_squat | 1517->1517 | 1.435->1.435 | 1.000->1.000 | 0->0 |
| asymmetric | 53->56 | 1.129->1.132 | 12.745->14.816 | 18->24 |
