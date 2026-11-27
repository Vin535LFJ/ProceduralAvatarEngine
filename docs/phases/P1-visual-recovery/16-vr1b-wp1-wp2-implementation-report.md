# VR-1B WP1/WP2 实施报告 — PAE 人体模板第一次进入 Android 真机

> 日期：2026-11-27
> 任务：PAE-CONSOLIDATION-03 授权的第一阶段（WP1 模板编译器产品化 + WP2 Android 静态模板加载）
> 授权依据：负责人 2026-11-27 会话指示；ADR-014（ACCEPTED）；15 号决策包 §4 白名单
> 性质：**工程里程碑**——本轮有代码、真机视觉证据、验收日志三类产出（AGENT.md §3）。
> 证据：`evidence/vr1b-wp1-wp2/`（device/ 15 张真机截图，host-views/ 7 张 host 预览，logs/ 12 份）

## 0. 一句话结论

**PAE 自有 `.paht` 人体模板第一次在 Android 真机（nunai_max，Mali-T860，GLES 3.2）上被正确加载并渲染为完整静态人体**：13,380 顶点 / 26,756 三角形，8 个相机预设 + 线框 + 法线视图全部出图，53–56 fps；损坏模板被哈希校验拒绝且进程不崩溃；Surface 重建走 CPU 缓存；Legacy mode-0/mode-3 回归正常。

**这不是**最终自然半写实成人视觉验收（那是后续 Gate），也**不是**动画验证（WP2 明确静态）。

## 1. 交付代码（全部在白名单内，加法接入）

### WP1 — 模板格式与编译器

| 文件 | 内容 |
|---|---|
| `native/procedural/human_template/PAEHumanTemplateFormat.h/.cpp` | PAHT **v3**：magic + version + **payload SHA-256**（32 B）+ 载荷。相对 v2：移除 `convertedAtUtc`（D1 可复现修复）、`sizeBytes` 升 i64。全部 v2 字段校验保留 |
| `native/procedural/human_template/HumanTemplateLoader.h/.cpp` | 只读文件/字节加载 + 完整校验（magic/版本/哈希/索引/权重块） |
| `native/procedural/human_template/BaseHumanRenderMeshBuilder.h/.cpp` | 四边形 → 三角形（D6 质量剖分：最大化最小角，平局取 0-2 对角线）；面积加权顶点法线；UV；f64→f32；静态蒙皮字段 joints=0/weights=1 |
| `tools/human-template-poc/host/src/wp1_compiler_main.cpp` + CMake 目标 `pae_ht_compiler` | host-only 编译器：复用原型加载链，**PAE 自建 20 骨 core rig + PAE 距离法权重**（ADR-014 决定 7；PENDING 文件仅作 host-only 参考，不进分发数据） |
| `app/src/main/assets/human_template/pae-human-base-mh-a8bc2d54.paht` + `PROVENANCE.md` | 版本化分发模板资产 |

三个新模块 cpp 加入 `PAE_CORE_SOURCES`（host 与 Android 共用，纯数据无 GL）。原型工具与 v2 证据**未改动**。

### WP2 — Android 静态加载（加法，无 ABI 变更）

| 文件 | 改动 |
|---|---|
| `native/renderer/RenderThread.h/.cpp` | 新增 mode-4：`ensureTemplateGeometry()`（加载→校验→建网→落地平移→上传）、`templateMesh_/templateLines_` + CPU 缓存、`activatePreviewGpuResources(4)`、`releaseGpuResources` 释放、`drawFrame` mode-4 分支（灰模/法线/线框，复用现有 shader）、单位矩阵调色板（mode-1 先例）、stats 附加模板状态。`postPreviewMode` clamp 0..3 → 0..4 |
| `app/.../MainActivity.kt` | BodyMode 循环 %4 → %5；`debug_generator` 范围 0..3 → 0..4 |
| `native/CMakeLists.txt`、`tests/CMakeLists.txt` | 新模块源 + 2 个测试可执行 + 1 个证据探针 |

**未触碰**：LegacyRigV1 / BoneId 0–21 / UBO[22] / 64B 打包 / 现有 JNI 签名 / mode-3 canonical / RenderThread 既有分支逻辑。**未新增 JNI 函数**（模板经 adb push 到 `/data/local/tmp/pae-human-base.paht` 只读加载；APK 已打包 assets 副本，AAssetManager 接入需另行授权，属后续步骤）。

## 2. 验收结果（日志原文在 evidence/logs/）

### Host

| 项 | 结果 |
|---|---|
| 编译器可复现性（D1） | 3 次运行（不同时刻）产出**逐字节相同**文件：4,054,264 B，SHA-256 `d3bcbbece3db11c0df79cb7202fdf1730b325108509d60ad9faa209a8a5e417b` |
| 模板往返 | serialize → parse → serialize 逐字节一致（真实资产 + 合成用例） |
| ctest 全套 | **22/22 通过**（含全部 Legacy 回归）；新增 `pae_human_template_format_tests` **14 cases / 0 failures**、`pae_base_human_canonical_tests` **7 cases / 0 failures** |
| 拒绝用例 | bad magic / bad version / payload 篡改 / 截断 / 尾部多余字节 / 面索引越界 / 区域越界 / 权重不覆盖 / 未知骨引用 — 全部拒绝 |
| 七视角 host 预览 | 探针 7/7 出图（`host-views/`，PAE ClayRenderer + CameraProtocol，与设备截图同相机协议） |
| 构建警告 | host 工具与测试构建 0 警告；APK native 构建仅既有警告（AnatomyField.cpp switch，与本轮无关） |

### 真机（nunai_max MAX0019111000104，Mali-T860，OpenGL ES 3.2，1920×1008）

| 项 | 结果 |
|---|---|
| APK | `app-debug.apk` 62,113,422 B，SHA-256 `c5e06e5d6c3d611f01e5afb72e886b36fa91378ea11422e4298ac560b5e4ee40` |
| 模板加载 | `human template ready: verts=13380 tris=26756 quads=13378 degenerate=0 ground_offset=0.8168 load_ms≈990 sha256=d3bcbbe…417b`（与编译资产哈希一致） |
| 渲染 | mode-4 灰模 8 预设（front/back/left/right/45-left/45-right/shoulder/face）+ 线框 + 法线，**15 张截图全部有内容**（覆盖率 4.5–15.4%）；53–56 fps，draw_calls=5，buffer_bytes=1,178,800 |
| 异常模板拒绝 | 推送篡改文件 → `human template rejected: payload hash mismatch`，进程存活（pid 可查），回退到先前模式继续渲染 |
| 生命周期 | HOME→resume：surfaceDestroyed/Created 后 `geometry cache hit: human template (26756 tris)`；force-stop 冷启动重新加载成功 |
| Legacy 回归 | mode-0 avatar 正常（59,408 tris，48 fps）；mode-3 canonical 正常（93,344 tris，49 fps）。注：mode-3 的 canonical 生成在该设备约 70 s（既有行为，慢 CPU），截图轮询需等待 |
| 内存 | dumpsys meminfo：native heap PSS 约 102–161 MB（随模式变化，含全部缓存网格）；模板 GPU 缓冲 1.18 MB |

### 过程性事实（如实记录）

- 首轮截图全黑：PowerShell `>` 重定向损坏 PNG 二进制 + 黑帧重试阈值错误。改为设备端 screencap + pull + 覆盖率轮询后正常（与历史 capture 脚本同法）。
- 图像描述模型对 45° 图的姿态描述不可靠（把静态 A-pose 描述成动态姿势）；**所有视觉结论以负责人人眼验收为准**，本报告只陈述"有内容、完整人体轮廓、无黑块"级别的客观事实。

## 3. 已知限制与 NOT RUN

| 项 | 状态 |
|---|---|
| 模板经 AAssetManager 从 APK 直接加载 | NOT RUN — 需加法 JNI 授权；当前 adb push 路径加载，assets 已打包备用 |
| Rig V2 动画 / 动态骨骼调色板 / 权重启用 | NOT RUN — WP2 静态；模板内权重为 PAE 距离法，肩外展拉伸缺陷未解决，留待后续 WP |
| 握拳 / DQS / corrective / LOD / 材质 | NOT RUN — 非本阶段 |
| 最终自然半写实成人视觉 Gate | 未通过 — 基础拓扑基线已 ACCEPTED_FOR_VR1B，最终形象另验 |
| 负责人对 15 张真机截图的人眼验收 | **PENDING** — 请查看 `evidence/vr1b-wp1-wp2/device/` |
| 其他设备（手机）验证 | NOT RUN — 本轮仅 nunai_max 电视盒子 |
| CPU/GPU skinning parity | 不适用 — 静态路径单位矩阵；parity 属 WP6 |

## 4. 下一步（需授权）

1. 负责人人眼验收本轮真机截图。
2. WP3（形态参数）或 AAssetManager 接入（加法 JNI）——按 15 号决策包，WP3–WP8 另行批准。
3. PAE 距离法权重的肩外展改进（动画前置）。