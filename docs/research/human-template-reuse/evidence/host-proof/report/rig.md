# Rig V2 (generated)

Source: `makehuman/data/rigs/default.mhskel` (`MakeHuman default skeleton`, format v110, license declared in file: CC0; NOT VERIFIED - the file is not enumerated in LICENSE.md, status PENDING owner confirmation).

Source bones: 163. PAE role census (PAE's own name-based classification):

| PAE role | Bones | In deform rig | In core rig |
|---|---:|---|---|
| root | 1 | 1 | 1 |
| pelvis | 2 | 2 | - |
| spine | 5 | 5 | 3 |
| neck | 3 | 3 | 1 |
| head | 1 | 1 | 1 |
| clavicle | 2 | 2 | 2 |
| shoulder | 2 | 2 | - |
| upper-arm | 4 | 4 | 2 |
| forearm | 4 | 4 | 2 |
| hand | 10 | 10 | 2 |
| finger | 30 | 30 | - |
| thigh | 4 | 4 | 2 |
| shin | 4 | 4 | 2 |
| foot | 2 | 2 | 2 |
| toe | 28 | 28 | - |
| breast | 2 | - | - |
| face | 59 | - | - |

| Variant | Bones | Notes |
|---|---:|---|
| PAEHumanRigV2-deform | 102 | every non-facial source bone; fingers (3 x 5 per side), toes, twist bones (upperarm02, lowerarm02, upperleg02, lowerleg02), clavicle + shoulder, 5 spine, 3 neck |
| PAEHumanRigV2-core | 20 | root, spine01/03/05, neck01, head, clavicle, upperarm01, lowerarm01, wrist, upperleg01, lowerleg01, foot per side; no digits, no twist split |
| LegacyRigV1 (production, untouched) | 22 | Root, Hips, Spine, Chest, Neck, Head, L/R Shoulder, UpperArm, LowerArm, Hand, UpperLeg, LowerLeg, Foot, Toe |

Bones with a roll-reference plane normal: 102 of 102 (deform rig). Bone head/tail come from the rig's joint definitions resolved as the mean of the base.obj joint-helper cube vertices on the CURRENT surface, so the rig follows every shape change.
