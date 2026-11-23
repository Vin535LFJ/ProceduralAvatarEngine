# 02 — Skeleton Contract（P0 冻结）

## 坐标与矩阵

- 右手系；+Y Up；+Z Forward；+X Character Left；单位米；角度弧度；列主序矩阵，`v' = M * v`。
- 骨骼矩阵链：`GlobalPose = ParentGlobal * Local`；`SkinPalette = GlobalPose * InverseBind`。当前 `bindLocal` 仅含平移，不表达旋转 A-Pose；如需旋转 Bind，必须先写 ADR。
- 时间戳：纳秒整型；JNI 传递不得用 float 承载时间戳。

## BoneId ABI（0–21，冻结）

| ID | Name | Parent |
|---|---|---|
| 0 | Root | -1 |
| 1 | Hips | 0 |
| 2 | Spine | 1 |
| 3 | Chest | 2 |
| 4 | Neck | 3 |
| 5 | Head | 4 |
| 6 | LeftShoulder | 3 |
| 7 | LeftUpperArm | 6 |
| 8 | LeftLowerArm | 7 |
| 9 | LeftHand | 8 |
| 10 | RightShoulder | 3 |
| 11 | RightUpperArm | 10 |
| 12 | RightLowerArm | 11 |
| 13 | RightHand | 12 |
| 14 | LeftUpperLeg | 1 |
| 15 | LeftLowerLeg | 14 |
| 16 | LeftFoot | 15 |
| 17 | LeftToe | 16 |
| 18 | RightUpperLeg | 1 |
| 19 | RightLowerLeg | 18 |
| 20 | RightFoot | 19 |
| 21 | RightToe | 20 |

## 语义修正（P0）

- 修正前：Shoulder 与 UpperArm 共享 `head=leftShoulder`、`tail=leftElbow`，导致 Shoulder 骨长度为 UpperArm 全长的重复段，语义重叠。
- 修正后：
  - `LeftShoulder = chest → leftShoulder`（短骨，代表锁骨/肩带）
  - `LeftUpperArm = leftShoulder → leftElbow`（上臂）
  - Right 侧镜像同理。
- BoneId、名称、parent 链保持不变，故 ABI 不变；权重与 retargeting 需回归验证。
- 回归测试：`tests/unit/humanoid_tests.cpp` 与 `tests/unit/skinning_tests.cpp` 断言 head/tail 不重叠、骨长>0 且在合理范围、parent 正确、权重合法。

## A-Pose 与 Bind Pose

- Bind Pose 使用 A-Pose：双臂相对躯干约 24°/10° 下垂，肘微屈，双腿分开，头中立。
- Bind Pose ≠ Idle Pose；Idle 是动作集合中的一种确定性姿势，不能替代 Bind 定义。
- 统一来源：`BodyJoints`/P1 `AnatomicalLandmarks` 同时驱动骨架、几何与语义区域。

## 验证

- 中性旋转下 `GlobalPose == BindGlobal`，`SkinPalette ≈ I`（容差 1e-3）。
- 单骨旋转满足 `T(head) * R * T(-head)`。
- 所有骨骼 `inverseBind` 可逆；`bindGlobal * inverseBind ≈ I`。
- Retargeting 的 UpperArm 目标方向仍由 `LkLeftShoulder → LkLeftElbow` 构造，不因 Shoulder 定义修正而改变。
