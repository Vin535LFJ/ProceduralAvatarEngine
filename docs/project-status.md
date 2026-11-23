# Project Status

> 本文件是项目状态的**唯一**入口。不要再创建 `PROJECT_STATUS.md` 等仅大小写/下划线不同的副本。

- 更新：2026-11-23
- 当前阶段：**P1-A 证据已提交，等待用户视觉 Gate**（P0 Gate 已由用户确认进入 P1-A；P1-A Gate 状态 NOT VERIFIED）
- P0 提交：`835dc71 docs+fix: freeze P0 contracts and correct shoulder bone semantics`，已推送 `feature/procedural-character-foundation`；基线提交 `399967f`。
- 权威基线：`01_Procedural_Character_Foundation_Specification_v1.0.md`、`02_Procedural_Character_Development_Roadmap_v1.0.md`、`03_AI_Engineering_Execution_Prompt_v1.0.md`、`README.md`（位于仓库根目录；用户引用的同名子目录当前不存在，仅有 zip）。

---

## A. Current Foundation（M13 当前工程基线）

- Git：基线分支 `m13-procedural-retopology` @ `bf5415f`；v1.0 工作分支 `feature/procedural-character-foundation`，基线提交 `399967f` 已推送 origin；未合并 main、未 force push。
- 基线提交范围：提交前已核对 8 项（6 untracked + 2 modified）与批准清单一致；提交时用了 `git add -A`，因当时工作区已确认干净且仅这 8 项变更，实际提交范围正确。**后续每次提交改为显式列出文件路径**，不再使用 `git add -A`；并在 push 前核对远程同名分支。
- 基线构建：`ctest --test-dir build/host-cygwin --output-on-failure` → 8/8 套件通过（7.60s）；`./gradlew :app:assembleDebug --no-daemon` → BUILD SUCCESSFUL in 12s。证据在 `docs/phases/P0-foundation-contracts/evidence/`。
- 基线稳定化：M13 WIP 中 `AnatomyField.cpp` 写入 `RegionPrimitives::trapRbBelly` 但结构体缺字段，host 编译失败；已补字段使基线可编译，未改变 M12 已验证几何语义。此项按 **PRELIMINARY FINDING → CODE AUDIT → REPRODUCTION → CONFIRMED ISSUE → FIX** 流程记录于 `docs/foundation/01-current-repository-audit.md`。

## B. New Product Roadmap（P0–P5 新目标）

| Gate | 状态 | 说明 |
|---|---|---|
| P0 | NOT VERIFIED | 契约/审计/ADR 已写入，待用户确认 |
| P1-A | NOT VERIFIED（证据已提交） | 新几何 PoC 已在真机运行并提交 8 视角灰模/线框/法线/动作证据；仍有肩部细暗线、臂部圆管感、肘部轻微折痕；未进入 P1-B |
| P1-B/C/D + P1 最终 | NOT VERIFIED | 严格串行，不提前开始 |
| P2/P3 | NOT VERIFIED | 等 P1 Gate |
| P4 | 分项状态见下表 | 视频链路与实时摄像头分开判定 |
| P5 | NOT VERIFIED | 等 P4 与设备条件 |

### P4/P5 分项状态（不得合并为“摄像头已交付”）

| 测试项 | 当前状态 |
|---|---|
| 视频解码 | 依实际测试 |
| 视频 Pose 推理 | 依实际测试 |
| Pose → Avatar 驱动 | 依实际测试 |
| 实时摄像头输入（nunai_max） | BLOCKED（无可用摄像头） |
| 手机摄像头真机验证 | NOT VERIFIED |

- 实时摄像头在盒子上 CameraX FRONT/BACK/ANY 均无法解析；用户计划后续在手机上验证。视频证据可用于视频路径验收，但不能代表实时摄像头 DEVICE VERIFIED。
- `rotationDegrees` 未应用、mirror 硬编码：按 **代码追踪 → 输入测试（0/90/180/270°、前后摄像头镜像只执行一次）→ 确认缺陷 → 修复 → 回归** 处理，不直接当作既定实现缺陷。
- P5 时间契约：Video PTS=µs；PoseFrame/Animation/Render timestamp=ns；统一时间映射层转换。Seek/重播时更新 `PlaybackEpoch`，过期异步 Pose 结果必须丢弃，避免动作回跳。

### P1-A 证据口径（按实际适用视角，不为凑数制造 8 个无意义视角）

- P1-A PoC 范围：躯干、肩胸连接、单臂+肘（随后镜像）；适用视角为正面、侧面、45°、背面、肩部特写、肘部特写。
- 所有适用视角提供 gray Lambert；代表视角提供 wireframe / normals；不要求每个视角都输出三种模式。
- P1-B 再做完整人体六方向 + 肩/肘/髋/膝四关节特写。
- Patch Boundary 验收分层：C0 位置连续（数值误差内重合）；G1 切平面/法线连续（平滑缝）；同时区分 Smooth seam / Hard seam / Open seam，不要求硬边法线相等。

### P2 规划资产（P2 正式设计时展开）

| 类型 | 目标资产 |
|---|---|
| 上衣 | T-shirt |
| 上衣 | Hoodie / Jacket |
| 下装 | Trousers |
| 下装 | Shorts |
| 鞋 | Shoes |

Garment Generation 必须同时解决人体遮挡、衣物厚度与轮廓、服装贴合、独立蒙皮和动作穿模；切换服装不重建整个角色。

## C. Historical Status（M0–M9 历史完成记录，保留不删）

- 更新：2026-11-21（M9 阶段门关闭，M0–M9 全路线闭环）
- 当时阶段：**MVP 交付完成**（PASS WITH FOLLOW-UPS）
- 当时上一阶段：M9 — PASS WITH FOLLOW-UPS（质量三档/百分位统计/12 分钟长测/冷重启压力/资产审计全部通过）

### 历史最终状态

- 需求：23 项 ID 化，17 项 verified、1 项 implemented（FUN-IN-001 摄像头路径——设备无摄像头）、其余按子项归并。
- 构建/测试：ctest 5 套件 24 用例全绿；`./gradlew :app:assembleDebug` 可复现；APK ~56MB（含 ML Kit bundled 模型）。
- 真机（nunai_max TV 盒子）验证：渲染 ~48 FPS（P50 20.8ms / P95 21–24ms）、合成动作六类、ML Kit 推理 208–240ms/帧（静态图/1080p 视频）、真人姿态驱动端到端（Shaka "Angel One" 480p 短片）、视频链路、质量三档切换、12 分钟 soak、冷重启 ×3 零崩溃、零外部美术资产审计。
- 未验证：摄像头实时路径、跨设备兼容性。
- M10 视觉提升：卡通描边、卡通脸、刘海、躯干截面平滑、四肢锥度、拇指、比例卡通化、细分提升、接触阴影、暖色调；见 `docs/phases/M10-visual-polish/`。

### M11 程序化人体解剖与视觉质量重构（历史）

- A–H 完成，I 收尾中：CatmullRom 样条扫掠/放掠、SDF 融合 PoC、人体生成器重写、WeightGenerator 区域权重、FaceGenerator、HairGenerator、ClothingGenerator、调色板、重定向修复。
- 视觉矩阵：P0 维度全部 ≥4；面部/发型/服装/材质 3–4 分。
- 性能：Medium 14065 三角形、50–53 FPS、P95 21.5ms。详见 `docs/phases/M11-anatomy-visual-rebuild/`。

### 历史阶段门记录

- M0/M1 复核：ctest 5/5 + assembleDebug 证据。
- M7：PASS WITH FOLLOW-UPS——ML Kit 静态图验证；相机三选择器回退；盒子无摄像头未验证。
- M8：PASS WITH FOLLOW-UPS——视频链路真机验证；EOS 修复；播放控制带滤波重置。
- M9：PASS WITH FOLLOW-UPS——质量三档、P50/P95/P99、12 分钟 soak、冷重启 ×3、资产审计通过。

> 以上历史声明只描述旧 M 路线，**不代表 v1.0 P0–P5 新目标已完成**。
