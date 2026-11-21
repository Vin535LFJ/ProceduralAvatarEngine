# M11 — 04 任务拆解

| 任务 | 阶段 | 文件范围 | DoD |
|---|---|---|---|
| T1 CatmullRom + SplineSweep + Loft 增强 | B | `geometry/CatmullRom.h`, `surfaces/ParametricSurfaces.{h,cpp}` | 新用例：样条有效性/法线连续性/winding/UV 接缝；旧 API 兼容 |
| T2 SdfFusion + PoC 三法对比 | B | `fusion/SdfFusion.{h,cpp}`, `tests/unit/fusion_tests.cpp` | 三样本数据表（法线偏差/三角形/耗时）；ADR-008 定案 |
| T3 人体生成器重写 | C | `humanoid/Humanoid.{h,cpp}` | BodyShapeParams 全参数生效；ctest 全绿；真机前后对比 |
| T4 WeightGenerator | D | `skeleton/weights/WeightGenerator.{h,cpp}` | 区域边界连续；热力图保留；运动回归 |
| T5 FaceGenerator | E | `humanoid/FaceGenerator.{h,cpp}` | 面部参数全生效；正/侧特写 |
| T6 HairGenerator | E | `humanoid/HairGenerator.{h,cpp}` | 3 发型切换；发束几何 |
| T7 ClothingGenerator | F | `humanoid/ClothingGenerator.{h,cpp}` | T 恤+裤外壳；厚度/领/袖口 |
| T8 材质 Shader + 调色板 | G | `renderer/RenderThread.{h,cpp}`, `NativeBridge.kt`, `MainActivity.kt` | 6 色；三档；特写对比 |
| T9 重定向修复 | H | `retargeting/Retargeting.{h,cpp}`, `MotionRuntime.{h,cpp}`, `PoseProviderAdapter.kt` | 置信度生效/缺失保持/坐标实证 |
| T10 相机预设 + 视觉证据 | I | `RenderThread`, JNI, `MainActivity.kt` | 10 预设固定相机；矩阵评分 |
| T11 整合验收 | I | 全部 | ctest 全绿 + 构建 + 真机 + 性能报告 + 06 报告 |

依赖：T1→T2→T3→T4→T5/T6/T7（并行）→T8→T9（可与 T5 并行）→T10→T11。
每任务完成即提交分支并跑 ctest+构建；阶段门（B/C/D/E/F/G/H/I）产出 05/06 记录。

## 阶段门

- M11-B：PoC 数据表 + ADR-008（几何选型）+ 真机融合样张
- M11-C/D：完整人体（旧脸临时保留）通过 Visual Matrix 1/2/3/4/9/10/11 维
- M11-E/F/G：面部/发型/服装/材质维度 ≥4
- M11-H：置信度/保持/镜像实证记录
- M11-I：全矩阵 + 性能报告 + 合回 main
