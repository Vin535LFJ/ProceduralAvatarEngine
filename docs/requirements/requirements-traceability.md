# Requirements Traceability Matrix (RTM)

状态：proposed / approved / designed / implemented / verified / blocked / deferred。没有证据不得标 verified。

| Requirement ID | 描述 | 设计文档/模块 | 实现位置 | 测试用例 | 状态 | 证据 |
|---|---|---|---|---|---|---|
| FUN-AV-001 | 程序化完整卡通人体 | `docs/architecture/target-architecture.md`；M4 设计 | M4 `native/procedural/humanoid` | M4 geometry/humanoid tests | verified | docs/phases/M4-humanoid-skeleton/evidence/ctest-utf8.txt, docs/phases/M5-skinning-materials/evidence/pose-arm-raise.png |
| FUN-AV-002 | 体型/外观参数与预设 | M4/M5 设计 | `native/procedural/humanoid`, `native/materials` | M5 参数/上传计数测试 | verified | docs/phases/M5-skinning-materials/evidence/logcat-stats.txt,colors-and-gradient.png |
| FUN-RIG-001 | 22 骨 Humanoid Skeleton | `docs/architecture/module-contracts.md`；ADR-002 | `native/skeleton` | M4 skeleton tests | verified | docs/phases/M4-humanoid-skeleton/evidence/ctest-utf8.txt |
| FUN-RIG-002 | 自动蒙皮权重 | M4 设计；ADR-004 | `native/skeleton/weights` | M4 weight tests | verified | docs/phases/M4-humanoid-skeleton/evidence/ctest-utf8.txt |
| FUN-RND-001 | GLES 3.0 Native 渲染 | M2 设计；ADR-001 | `native/renderer`, `app/src/main/cpp` | M2 真机探针与截图 | verified | docs/phases/M2-gles-renderer/evidence/frame.png,logcat-recreate.txt |
| FUN-RND-002 | GPU LBS | M5 设计；ADR-005 | `native/renderer/shaders`, `native/skeleton` | M5 UBO/CPU-GPU 对照 | verified | docs/phases/M5-skinning-materials/evidence/pose-arm-raise.png,ctest-utf8.txt |
| FUN-MOT-001 | SyntheticPoseProvider | M6 设计 | `native/motion/providers` | M6 六类动作测试 | verified | docs/phases/M6-motion-runtime/evidence/ctest-utf8.txt,motion-pose-2.png |
| FUN-MOT-002 | Retargeting/滤波/约束 | M6 设计 | `native/motion` | M6 motion tests | verified | docs/phases/M6-motion-runtime/evidence/ctest-utf8.txt |
| FUN-POSE-001 | IPoseProvider/PoseFrame | `docs/architecture/module-contracts.md` | `native/motion/poseframe` | M6 数据契约测试 | verified | docs/phases/M6-motion-runtime/evidence/dependency-scan.txt,ctest-utf8.txt |
| FUN-POSE-002 | ML Kit 真实姿态 Adapter | M7 设计；ADR-003 | `app/src/main/java/com/pae/host/PoseProviderAdapter.kt` | M7 静态图链路验证（真机推理 238ms/帧） | verified | docs/phases/M7-pose-provider-camera/04-test-and-acceptance.md, evidence/logcat-m7-final.txt |
| FUN-IN-001 | 摄像头实时输入 | M7 设计 | `app/src/main/java/com/pae/host/PoseProviderAdapter.kt` | M7 真机/替代验证 | implemented | docs/phases/M7-pose-provider-camera/04-test-and-acceptance.md（摄像头真机项未验证：设备无摄像头，三选择器回退日志） |
| FUN-IN-002 | 本地视频输入 | M8 设计；ADR-006/ADR-007 | `app/src/main/java/com/pae/host/VideoPoseSource.kt` | M8 真机解码/PTS/状态机/**真人驱动**验证 | verified | docs/phases/M8-video-input/04-test-and-acceptance.md, evidence/logcat-person-drive.txt, evidence/frame-person-pose-1..3.png |
| FUN-SCN-001 | 程序化背景 | M5 设计 | `native/procedural/scene`, `native/renderer` | M5 切换测试 | verified | docs/phases/M5-skinning-materials/evidence/colors-and-gradient.png,frame-initial.png |
| PERF-RT-001 | 30 FPS 目标与测量 | M9 设计；`docs/performance/performance-test-plan.md` | `native/renderer/stats` | M9 设备报告 | verified | docs/performance/reports/mvp-device-report.md（实测 ~48 FPS > 30 目标） |
| PERF-RT-002 | 延迟与资源统计 | M7/M9 设计 | `native/renderer/stats`, Kotlin stats | M9 设备报告 | verified | docs/performance/reports/mvp-device-report.md（推理 208–240ms、P50/P95/P99、内存/温度） |
| COMP-003 | Low/Medium/High 质量等级 | M9 设计（Spec §12.2） | `native/core/config/EngineConfig.cpp`, `native/renderer/RenderThread.cpp` | M9 真机三档切换 + 宿主单测 | verified | docs/phases/M9-performance-delivery/evidence/logcat-quality-*.txt |
| REL-002 | 稳定性长测与生命周期压力 | M9 设计 | 全运行时 | M9 12 分钟 soak + 冷重启 ×3 | verified | docs/phases/M9-performance-delivery/evidence/soak-12min.log |
| COMP-001 | GLES 3.0 能力检测 | M2 设计；ADR-001 | `native/renderer/egl` | M2 真机 logcat | verified | docs/phases/M2-gles-renderer/evidence/logcat-recreate.txt |
| COMP-002 | Android 生命周期 | `docs/architecture/threading-and-lifecycle.md` | JNI + Kotlin Surface 回调 | M2/M9 重建测试 | verified | docs/phases/M2-gles-renderer/evidence/logcat-recreate.txt |
| ART-001 | 零外部 3D 美术资产 | ADR-004 | 全工程 | 目录扫描 + 构建产物审计 | verified | M9 审计：仓库无模型/贴图/媒体命中；APK 仅 assets/mlkit_pose/（已披露 SDK 模型） |
| REL-001 | 线程与资源所有权 | `docs/architecture/threading-and-lifecycle.md` | `native/runtime`, RenderThread | M2 测试 | verified | docs/phases/M2-gles-renderer/04-test-and-acceptance.md |
| OBS-001 | 日志与 RenderStats | M2 设计 | `native/core/diagnostics`, `native/renderer/stats` | M2 logcat/测试 | verified | docs/phases/M2-gles-renderer/evidence/logcat-recreate.txt |
| TEST-001 | 单元与集成测试 | 各阶段设计 | `tests/unit` | `ctest` | verified | docs/phases/M1-architecture/evidence/ctest-cygwin.txt |
| M11-GEOM | 样条几何/SDF融合/解剖参数 | M11 设计；ADR-008 | `procedural/geometry/CatmullRom.h`, `surfaces/ParametricSurfaces.*`, `fusion/SdfFusion.*`, `humanoid/Humanoid.*` | fusion_tests + geometry_tests（样条/椭圆/锥度法线/winding） | verified | docs/phases/M11-anatomy-visual-rebuild/, ADR-008 |
| M11-SKIN | 区域化蒙皮权重 | M11 设计 | `skeleton/weights/WeightGenerator.*` | skinning_tests（髋部混合/头部主导） | verified | docs/phases/M11-anatomy-visual-rebuild/05 |
| M11-FACE | 参数化面部/头发/服装 | M11-E/F 设计 | `humanoid/FaceGenerator.*`, `HairGenerator.*`, `ClothingGenerator.*` | face_hair_tests（头形/特征/参数/3发型/服装覆盖） | verified | docs/phases/M11-anatomy-visual-rebuild/05 |
| M11-RETARGET | 置信度门限/关键点保持 | M11-H 设计 | `motion/retargeting/Retargeting.*`, `MotionRuntime.*` | motion_tests（保持/置信度门限） | verified | docs/phases/M11-anatomy-visual-rebuild/05 |
| TEST-002 | 视觉/设备验收 | 各阶段 04-test-and-acceptance | `docs/phases/Mx.../evidence` | 截图/日志 | verified | M2–M9 各阶段 evidence 目录（截图/日志/命令输出） |
| P0-AUDIT | v1.0 基线审计与路径差异记录 | `docs/foundation/00-product-scope.md`, `01-current-repository-audit.md` | 文档层 | ctest 8/8 + assembleDebug 基线 | implemented | docs/phases/P0-foundation-contracts/evidence/ctest-baseline.txt, gradle-assemble-baseline.txt |
| P0-CONTRACT | Skeleton/Mesh/Skinning/Visual/Thread 契约冻结 | `docs/foundation/02..06` | 文档层 | 契约审阅 + P0 Gate | implemented | docs/foundation/02-skeleton-contract.md, 03-geometry-mesh-contract.md, 04-skinning-contract.md, 05-visual-acceptance-protocol.md, 06-thread-and-resource-lifecycle.md |
| P0-SHOULDER | Shoulder/UpperArm 语义修正与回归 | `docs/foundation/02-skeleton-contract.md` | `native/skeleton/Skeleton.cpp`, `native/motion/providers/SyntheticPoseProvider.cpp`, `native/procedural/humanoid/anatomy/AnatomyField.cpp` | humanoid_tests + skinning_tests + motion_tests + 真机截图 | tested | docs/phases/P0-foundation-contracts/evidence/ctest-after-shoulder-fix.txt, frame-shoulder-fix-2.png |
| P0-ADR | SDF 主线定位修正 | ADR-010 | 文档层 | P0 Gate 审阅 | implemented | docs/architecture-decisions/ADR-010-p1-surface-patch-mainline.md |
