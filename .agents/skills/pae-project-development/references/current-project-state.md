# PAE Current Project State (动态快照)

> **这是快照，不替代正式状态板。** 权威：`docs/roadmap/PAE-HUMAN-GENERATION-STATUS.md`。每轮必须重新读仓库核对，快照过期须提醒复核。

- Updated: 2026-11-28
- Repository: ProceduralAvatarEngine（工作树 `E:\AI-Tools\ProceduralAvatarEngine-poc-sm`）
- Branch: `poc-sm/shape-model-v2`
- Commit SHA（快照时 HEAD）: `e636b8cefb7af8d86d46a0eb4440336ea17f146b`
- 核实状态: 快照时已核对；**M2-POSE-FRAME-AUDIT-01 成果当时尚未提交（工作区修改），后续提交后 SHA 会前进，须重核。**

## 里程碑概况（须以最新 STATUS + 测试重核）

| 阶段 | 状态 | 核心 |
|---|---|---|
| M0 | ACCEPTED | PAHT v3、模板编译、Android 静态加载、APK Asset |
| M1 | IMPLEMENTED | WP3 测量 + WP3-R2 逆求解（ANSUR 口径/视觉待确认） |
| M2 | TECHNICAL PARTIAL | Rig V2 / JointResolver / 权重 / CPU LBS / 坐标审计；极端姿态质量未达标 |
| M3 | NOT STARTED | CharacterInstance 整合 |
| M4 | NOT AUTHORIZED | Android Rig V2 动态路径 |
| M5 | NOT AUTHORIZED | 程序化动画 / IK |
| M6 | PLANNED | 解剖/结构化手足（细分负结果已记录） |
| M7 | PLANNED | 材质/头发/服装 |
| M8 | PLANNED | 生产质量/最终验收 |

## 关键技术事实（须重核）

- Base Mesh：13,380 顶点 / 13,378 四边形面。PAHT v3 SHA-256 `d3bcbbece3db11c0df79cb7202fdf1730b325108509d60ad9faa209a8a5e417b`。
- 历史真机静态约 53–56 FPS（仅代表已测设备）。
- M2 极端姿态（修复前基线）：fist 1056 翻面、elbow_120 3138、arms_raised 3112、basic_squat 1517、shoulder_abduction_90 腋窝边拉伸约 12.7×。
- M2 坐标审计修复后：elbow_120 翻面 3138→1492；fist 自交 130→1088（正确运动学暴露指间穿透）；WP4 测试 30/30。
- M6 统一细分负结果：13,380→53,514 顶点，握拳翻面率 7.89%→7.99%，自交 130→260，细分权重纯度 0.9279。单纯细分非解。

## 已知文档漂移

MASTER-PLAN/STATUS 曾记录 `d40e213`，代码已前进。每轮核对最新报告/提交/状态板/总计划/测试，历史与当前分开记录，不擅自改写正式路线图/ADR。

## 测试基线（须以当前 CMake/ctest 重核，不沿用历史数字）

WP4 30/30、WP3 33/33、Host 65/65、Engine CTest 22/22（快照时）。