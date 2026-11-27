# WP3 measurement protocol (PAE-defined surface measurements)

ALL values below are PAE custom surface measurements on the pinned MakeHuman base mesh. They are NOT ANSUR or MakeHuman official measurements. Units: metres (m), precision 1e-5 m in CSV. Circumferences are true plane-cut perimeters (triangle-plane intersection segments summed; loops chained and closure verified).

| Measurement | Plane / position | Sampling | Regions | Direction | Formula |
|---|---|---|---|---|---|
| stature | bbox over all body vertices | whole surface | all | y | max(y)-min(y) |
| biacromial_proxy | shoulder joint centres (upperarm01.L/R head) | rig joints re-resolved on deformed surface | rig | 3D distance | |L-R|; NOT anatomical acromion distance |
| shoulder_yoke | y = sole + shoulderJointH - 2.5% stature | band half-width 1.5% stature | Neck+Thorax+ShoulderGirdle | x | max(x)-min(x) of band vertices |
| outer_shoulder | same plane | same band | +UpperArm | x | max(x)-min(x) |
| chest_breadth/depth/circumference | y = sole + 72% stature | exact plane cut, TORSO loop (largest area; arm loops excluded) | whole body section | x/z/perimeter | section extents; sum of cut segments of the torso loop |
| waist_breadth/depth/circumference | y = sole + 60% stature | exact plane cut, TORSO loop | whole body section | x/z/perimeter | same |
| hip_breadth/depth/circumference | y = sole + hipJointH | exact plane cut, TORSO loop | whole body section | x/z/perimeter | same |
| upperarm_circumference_L | plane normal = upperarm02.L axis, through bone midpoint | exact oblique cut, loop NEAREST the midpoint | whole body section | in-plane | perimeter of that loop |
| limb lengths | joint-to-joint (01 head / 02 tail / wrist / foot) | rig joints | rig | 3D distance | mean over L/R |

Plane fractions follow the PAE proof caliber (ht_measure) for comparability with R3; alignment to the ANSUR acceptance caliber is a WP3-Production prerequisite.
