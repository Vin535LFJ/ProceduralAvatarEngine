# Project Status

> 本文件是项目状态的**唯一**入口。不要再创建 `PROJECT_STATUS.md` / `STATUS.md` 等重复文件。
> 本页是**一页式当前状态**；§A/§B/§C 为 P1-A 及更早阶段的**历史记录，原样保留**，
> 其 Gate 结论不代表当前阶段。

## 当前状态（2026-11-24，PoC-SM 第 2 轮）

| 项 | 当前事实 |
|---|---|
| **项目方向** | Anatomy-driven Geometry PoC：以单一 `CharacterDefinition` 语义驱动几何，验证语义是否真正进入最终网格。旧 P1-A 路线视觉 `REJECTED`，仅作历史，不是当前阶段。 |
| **工作区 / 分支** | `E:/AI-Tools/ProceduralAvatarEngine-poc-sm`，分支 `poc-sm/shape-model-v2`，本地 HEAD `8d1377e`。记录的基线 `9cdde4c` 已核实为 HEAD 的**祖先**（`git merge-base --is-ancestor` = 0）。HEAD 比记录值多 1 个仅含治理文档的提交 `8d1377e`。G2 冻结基线 `a32251b` 亦在历史中。 |
| **远端核对** | 本地 `origin/poc-sm/shape-model-v2` 缓存 ref = 本地 HEAD = `8d1377e`。`git fetch origin` 两次失败（环境 SSL/schannel 错误），**远端实时状态 UNVERIFIED**；未做任何覆盖/恢复。 |
| **PoC-SM 实现状态** | v2 五层契约 + 单一连续 Shared Skin Envelope + OQ-03 历史环带测量 + 新增实验性跨缝测量 + 全参数差分工具。Landmark 仅**声明未放置**（`placed` 为空）；`spine-curve` 为 `DeclaredOnly`；`shoulderBreadthRatio` / `biacromialRatio` / `abdominalDepthRatio` / `headHeightRatio` 有测量值但**未进入生成链**。 |
| **测试状态** | 修改前基线 `14/14`；本轮 `17/17 suites, 0 failures`（新增跨缝 9 例、AL-6 差分 9 例、语义契约 7 例）。完整原文见 `evidence/poc-sm/round-2/final-ctest.log`。 |
| **接缝口径** | OQ-03 已签署定义未改动。历史环带实测 **21.7622°**（32 段）；新实验证明该统计随段数按 360/S 变化，且不随真实跨缝折角变化。跨缝实验（0/5/15/30° × 32/64/96/128 段）读出真实折角，但**不是**正式验收定义，等待负责人裁定。 |
| **OQ-01 / DEC-05** | `OPEN`。灰模已产出（含本轮 9 张，7 固定视角 + 2 近景），等待负责人视觉审查与 DEC-05 签署；**任何指标不得判 PASS**，无 Gate 通过。 |
| **本轮视觉变化** | 几何生成代码未改（只改注释、加工具与测试）；本轮图像为新生成，但与上一版 PoC-SM 在几何上**一致**。未声称视觉改进。 |
| **下一阶段** | ① 负责人裁定 OQ-03 正式测量定义；② 负责人完成 DEC-05 / Canonical Acceptance Sheet；③ 裁定 4 个测量字段的解剖映射后再决定是否接线；④ 裁定后按统一口径重算 G2 三路线。 |

### 五个权威文档入口（唯一集合，不另建）

| 用途 | 位置 |
|---|---|
| 决策记录（2026-11-24 签署） | `docs/phases/anatomy-driven-geometry-poc/10-decision-records/00-INDEX.md` |
| 待决问题单一 Register | `docs/phases/anatomy-driven-geometry-poc/07-open-issues.md` |
| 实施 SPEC（含 Register / 验收标准） | `docs/phases/anatomy-driven-geometry-poc/00-implementation-spec.md` |
| 负责人待办一页纸 | `docs/phases/anatomy-driven-geometry-poc/06-owner-signoff-checklist.md` |
| ADR 索引 | `docs/ADR_INDEX.md` |

> 轮次验收契约：`AGENT.md` + ADR-013（要求本轮新代码 / 新图 / 新日志三者齐备）。
> PoC-SM 实现与证据：`docs/phases/anatomy-driven-geometry-poc/12-poc-sm-vertical-slice.md`
> 及 `evidence/poc-sm/`。以上是补充入口，不增加第六个状态文件。

---

## 历史记录（P1-A 及更早，保留不删；下列 Gate 状态不是当前阶段）

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
| P1-A | REJECTED | 项目方判定效果特别差；工程指标通过但视觉不达标。缺陷档案 `docs/phases/P1A-geometry-poc/06-visual-defect-report.md`；不进入 P1-B |
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
