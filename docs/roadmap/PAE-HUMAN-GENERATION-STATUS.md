# PAE Human Generation — STATUS BOARD

Authority: [`PAE-PROCEDURAL-HUMAN-GENERATION-MASTER-PLAN.md`](PAE-PROCEDURAL-HUMAN-GENERATION-MASTER-PLAN.md)
Updated: 2026-11-28 · Branch `poc-sm/shape-model-v2` @ `d40e213`（本地=远端）

状态枚举：NOT STARTED / DESIGN / IMPLEMENTED / TECHNICAL PARTIAL / VISUAL BLOCKED / ACCEPTED。
规则：Host 通过 ≠ Android 通过；静态完整人体 ≠ 最终半写实人体；技术指标通过 ≠ 人眼视觉通过。

## 里程碑总览

| 里程碑 | 状态 | 说明 |
|---|---|---|
| M0 模板基础 | **ACCEPTED** | 真机静态全身 + 损坏拒绝 + 生命周期 + Legacy 回归 |
| M1 形态参数化 | **IMPLEMENTED** | WP3-HOST 测量 + WP3-R2 逆求解器已实现；ANSUR 口径对齐/最终视觉待负责人 |
| M2 骨骼与蒙皮 | **TECHNICAL PARTIAL** | WP4 Host 已实现；极端姿态变形质量未达标 |
| M3 整合实例 | NOT STARTED | 需 M1+M2 通过 + 新授权 |
| M4 Android 动态 | NOT AUTHORIZED | M3 通过后单独授权 |
| M5 动画/IK | NOT AUTHORIZED | M4 稳定后 |
| M6 解剖细化 | PLANNED | |
| M7 材质/头发/服装 | PLANNED | |
| M8 生产验收 | PLANNED | 负责人人眼最终签署 |

## 工作包明细

| WP | 状态 | 代码 | 测试 | 证据 | 未解决 |
|---|---|---|---|---|---|
| WP1 模板编译器 | ACCEPTED | `wp1_compiler_main.cpp` + native human_template | ctest 22/22 | 16 号报告 | — |
| WP2 Android 静态 | ACCEPTED | RenderThread mode-4 | 真机 15 图 | 16 号报告/evidence | — |
| WP2.1 APK 加载 | ACCEPTED | `nativeLoadTemplateAsset` JNI | 真机独立加载 | 17 号报告 | — |
| WP3-HOST-01 测量 | IMPLEMENTED | `wp3_measure/quality/experiment` | 33/33 | 18 号 + evidence | 围度口径已修 |
| WP3-H-R1 修正 | IMPLEMENTED | 锚点/联动/截面 | 33/33 | 19 号 + r1 evidence | 退化绝对阈值（native，白名单外） |
| WP3-R2 求解器 | **IMPLEMENTED** | `wp3_r2.h/.cpp` + `wp3_r2_main.cpp` | known/holdout/unreachable/非加性/确定性 全过 | VR-1B/01a 报告 + evidence/wp3-r2 | armShape 零空间（正则化处理）；ANSUR 口径对齐 |
| WP4-HOST-RIG-01 | **TECHNICAL PARTIAL** | `wp4_rig/skin/weights/poses/quality` | 16/16 | 02 号报告 + 184 图 | 握拳/举臂/深蹲翻面（LBS+网格限制） |
| M2 变形质量改进 | NOT STARTED | — | — | — | DQS/corrective 或 M6 手指细化 |
| M3 CharacterInstance | NOT STARTED | — | — | VR-1B/04 计划 | 缓存失效/权重复用规则 |
| M4 Android 动态 | NOT AUTHORIZED | — | — | VR-1B/05 计划 | 设备 palette/UBO 能力 |
| M5 动画/IK | NOT AUTHORIZED | — | — | VR-1B/06 计划 | — |
| M6 解剖 | PLANNED | — | — | VR-1B/07 计划 | — |
| M7 表现 | PLANNED | — | — | VR-1B/08 计划 | Saltline 借鉴范围待研究 |
| M8 验收 | PLANNED | — | — | VR-1B/09 计划 | — |

## 最终状态字段（MASTER-PLAN-01 + WP3-R2 实现后）

```
MASTER_PLAN      = READY_FOR_REVIEW
M1_HOST          = IMPLEMENTED       (WP3-R2 求解器已实现；ANSUR 口径/最终视觉待负责人)
M2_HOST          = TECHNICAL PARTIAL (WP4 已实现；极端姿态质量未达标)
M3_INTEGRATION   = NOT_STARTED
M4_ANDROID       = NOT_AUTHORIZED
M5_ANIMATION     = NOT_AUTHORIZED
M6_ANATOMY       = PLANNED
M7_APPEARANCE    = PLANNED
M8_PRODUCTION    = PLANNED
```

## 代码授权来源

- M1 Host 代码授权：WP3-HOST-01 / WP3-H-R1 任务书白名单（`tools/human-template-poc/host/**` + 对应 docs）。WP3-R2 沿用同一 Host 白名单。
- M2 Host 代码授权：WP4-HOST-RIG-01（`wp4-host-rig-01/01-wp4-host-rig-plan.md` §1 白名单）。
- M3+ 需新授权；M4 Android 必须在 M3 Host 通过后单独授权白名单。**不得把 M1–M8 全部标为已授权实施。**

## 冻结边界（全程）

LegacyRigV1 / BoneId 0–21 / UBO[22] / 64B Vertex Packing / JNI ABI / mode-3 Canonical / mode-4 静态默认行为 / PAHT v3 字段语义。生产路径写入单一权威写入者。