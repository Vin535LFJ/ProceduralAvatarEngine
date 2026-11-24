# Deliverable 1 — Visual Direction Decision

> 状态：**草案 / DRAFT — 待负责人批准**。本文件不修改项目文档、不签署 DEC-05。
> 证据纪律：每条结论标注 `[FACT]` / `[INFERENCE]` / `[RECOMMENDATION]` / `[OPEN QUESTION]`
> 与 `VERIFIED` / `PARTIALLY VERIFIED` / `DECLARED ONLY` / `UNVERIFIED` / `REJECTED`。

## 1. 新的最终视觉方向

`[RECOMMENDATION]` 采用 **High-quality Stylized Realism（高品质风格化写实 3D 人物）** 作为项目最终视觉方向：

- 人体比例自然可信，具备真实解剖结构；
- 肌肉与骨骼轮廓有解剖依据，不是球体/胶囊/圆管拼接；
- 曲面连续，肩根、腋下、胯部、肘、膝无明显拼接痕迹；
- 有较高 3D 完成度，不是低模、玩具感、过度简化；
- 保留卡通化/艺术化特征，但不牺牲结构可信度；
- 可程序化生成、可实时驱动，不是只好看的静态图。

`[FACT/VERIFIED]` 这与负责人本轮给出的方向描述一致，也与项目现有硬约束一致：C++17 / Android Kotlin+JNI+NDK / EGL / GLES3 / 自研 Renderer，不使用 Unity/Unreal/Godot/Filament，不依赖外部模型资产。

## 2. 为什么不再使用 Vincent 作为主视觉标杆

| 理由 | 证据等级 |
|---|---|
| 负责人明确指示"不得继续沿用 Vincent 作为主要视觉标杆，也不得把旧方案简单换几个链接后重新提交" | `[FACT/VERIFIED]` — 本轮任务书 |
| Vincent 当前呈现的简化、粗糙风格被负责人列入"明确排除的视觉方向" | `[FACT/VERIFIED]` — 本轮任务书 |
| 本项目需要"高品质 3D 完成度"与"自然人体比例"，与简化粗糙风格目标冲突 | `[INFERENCE/PARTIALLY VERIFIED]` — 基于任务书描述，未做像素级图像审查 |

`[RECOMMENDATION]` Vincent 从"主视觉标杆"降为"历史失败/不足案例"，不再进入正式参考矩阵。此判断需要负责人确认后写入 DEC-05。

## 3. 为什么选择 High-quality Stylized Realism

1. `[FACT/VERIFIED]` 它满足任务书第八节全部工程约束：不要求外部模型、不要求高面数、可由程序化几何实现。
2. `[INFERENCE/PARTIALLY VERIFIED]` 它给出清晰的中间地带：比"粗略卡通"高完成度，但不要求照片级扫描（后者与实时 Android 和低面数预算冲突）。
3. `[FACT/VERIFIED]` G2 实证了当前几何的问题不是网格数量，而是形态语义缺失：R3 上胸缺口 13.5 cm、有效参数 7/20、模板加密几乎不收敛；R2 后端 E2 仅 0.98 mm 却继承同一不够好的目标表面。`[RECOMMENDATION]` 因此视觉目标必须落在"形态关系可信"，而不是"面数更高"。
4. `[OPEN QUESTION]` 高端风格化写实的具体完成度阈值尚未批准（`TBD`）。

## 4. Level 1 / Level 2 / Level 3 的区别

`[FACT/VERIFIED]` 三级定义沿用负责人任务书；**目标层级已由负责人明确指示**（见 §4.1）。下表补充"可观察指标 / 必须视角 / 通过条件 / 一票否决 / 当前阶段是否要求"，阈值未经批准处保持 `TBD`。

| 级别 | 定位 | 可观察指标（草案） | 必须提交视角 | 通过条件 | 一票否决 | 当前阶段要求 |
|---|---|---|---|---|---|---|
| **Level 1 — Structurally Valid** | 工程基础门槛，非最终目标 | 比例基本合理；主要区域可辨；曲面连续；无明显球体/圆管拼接；主要关节结构正确；可完成基础骨骼驱动 | Front / Side / Back 灰模 + 基础 wireframe | 三段视角无严重形态缺陷；拓扑 components=1、无非登记 boundary/non-manifold；≤5° 接缝合同值通过 | 独立肩球、圆管四肢、明显拼接、三视角之一严重失真 | **是 — PoC-SM 必须达到 Level 1** |
| **Level 2 — High-quality Stylized Human** | **项目核心视觉目标** | 解剖比例可信；肌肉体积与骨骼关系合理；肩/胸/腹/盆/四肢自然连续；侧背同样有体积；动作中关节保持体积；不依赖贴图掩盖几何；支持程序化比例调整 | Front / Front 3/4 / Side / Back / Back 3/4 + A-Pose/T-Pose + 至少一个动作姿态 | 由 DEC-05 授权的 Canonical Reference Sheet 对照判定；阈值 `TBD` | 静态好看但侧面/背面失真；贴图掩盖几何缺陷；无法程序化调参 | **是 — 核心目标（P1 结构/曲面/驱动质量，P2 换装与模块化外观）** |
| **Level 3 — Production Visual Quality** | **长期方向** | 接近高品质商业风格化角色完成度；多体型多外观；动作表现好；目标 Android 设备性能可接受 | 同 Level 2 + 实机录屏/FPS/帧时间 | `TBD`（负责人 + 设备实测） | 为静态视觉破坏 ABI/性能/工程边界 | **否 — 长期方向，不作为当前 Gate 判据** |

`[RECOMMENDATION]` 三个等级**不得平均成一个总分**；Level 1 是门禁，Level 2 是核心目标，Level 3 是长期方向。

### 4.1 负责人已明确的层级与阶段分工（2026-11-24）

`[FACT/VERIFIED — 负责人指示]` 以下三项不再是待决问题，AI 按此执行：

1. **Level 2 — High-quality Stylized Human 设为项目的核心视觉目标**；
2. **Level 3 作为长期方向**，不在当前阶段作为判据；
3. **阶段分工**：**P1 先证明人体结构、曲面和驱动质量；P2 再证明换装和模块化外观。**

`[INFERENCE]` 该指示的目的是**防止为了追求一张漂亮的渲染图而牺牲程序化数字人引擎本身**。它与项目既有约束在结构上同向：C++17 / NDK / GLES3 / 自研 Renderer / 低端 Android / 无外部模型资产，天然排斥"用渲染品质掩盖几何缺陷"。

`[RECOMMENDATION]` 据此，本轮所有 rubric 与 Gate 建议都按 §5 的**五 Gate 分离**设计，而不是单一"视觉 Gate"。

---

## 4.2 五 Gate 分离：几何验收不得与最终渲染品质混成一个 Gate

`[FACT/VERIFIED — 负责人指示]` 验收对象与各自的替代禁区：

| 验收对象（Gate） | 应该验证什么 | **不应该用什么代替** |
|---|---|---|
| **G-P1-GEO** — P1 人体几何 | 解剖比例、曲面连续性、人体体积 | 高质量皮肤贴图 |
| **G-P1-TOPO** — P1 拓扑 | 肩根、腋下、胯部、关节结构 | 单纯统计面数 |
| **G-P1-ANIM** — P1 动画 | 关节弯曲、体积保持、变形连续性 | 静态 T-Pose |
| **G-P2-MOD** — P2 模块化 | 服装、发型、配件独立切换 | 单张穿着完整服装的效果图 |
| **G-FINAL-VIS** — 最终视觉 | 整体角色完成度、材质、光照 | 单纯几何测试通过 |

`[RECOMMENDATION]` 这条分离对当前项目有**直接且可量化的后果**，见下表（全部为 G2 实测值，非阈值）：

| Gate | 当前证据 | 结论 |
|---|---|---|
| G-P1-TOPO | R2：sliver 0、未登记 boundary 0、non-manifold 0、degenerate 0、components 1、正交 quad 比 0.935、顶点 ID 跨参数组恒定 | **后端基本成立** —— 这是 R2 可以保留的技术依据 |
| G-P1-ANIM | 权重全部合法（≤4/非负/和=1±1e-3/ID∈[0,21]）；CPU 变形刚体探针 22/22 零误差；GPU parity `UNVERIFIED` | **CPU 侧成立，GPU 侧待补测** |
| G-P1-GEO | R3 上胸缺口 13.5 cm；R1/R2 肩根接缝 47.66°（合同 ≤5°）；R3 16.13° | **不成立 —— 这是当前真正的阻塞点** |
| G-FINAL-VIS | 无任何实机渲染/性能数据 | **NOT RUN** |
| G-P2-MOD | 未开始 | **NOT RUN** |

`[INFERENCE]` 这组数据**独立验证了负责人的整体判断**：先做 Shape Model（P1-GEO）而不是继续加 R2/R3 代码量是对的；而且 R2 作为固定拓扑后端之所以值得保留，正是因为它唯一已经过关的是拓扑与动画两关，恰恰不是形态关。若把五 Gate 混成一个"视觉 Gate"，R2 会因为形态不达标而连同其合格的拓扑能力一起被否掉——这是必须分开验收的实质原因，而不只是流程洁癖。

`[RECOMMENDATION]` 建议把这五 Gate 写进 SPEC 的 Gate 链，作为 GATE-04（G3 路线决策）之后的细分验收序列；但这属于 SPEC 结构变更，需负责人批准后再改文档（见 Deliverable 5 Q-06）。

---

## 5. 当前阶段应达到哪一级

`[FACT/VERIFIED — 负责人指示]` **核心目标是 Level 2**；**P1 先证明人体结构、曲面和驱动质量**。

`[RECOMMENDATION]` 对 PoC-SM 的具体判据建议（**这是"如何逼近 Level 2"的路径设计，不是降低目标**）：

- PoC-SM 必须达到 **Level 1 全部**（工程基础门槛），并且必须让 **G-P1-GEO 从不成立变为成立**；
- Level 2 的判定需要 Canonical Reference Sheet（DEC-05 授权参考图），在负责人提供前 `NOT RUN`；
- 因此 **Level 2 是设计方向 + 验收目标，不是当前能判 PASS 的 Gate**；PoC-SM 的实际判据是"Level 1 达成 + G-P1-GEO 达成 + G-P1-TOPO 维持 + G-P1-ANIM 的 CPU 侧维持"。

理由：
- `[FACT/VERIFIED]` 现有 §13 阈值除 ≤5° 外全为 `TBD`，且 DEC-05 参考图未提供；在 Canonical Reference Sheet 存在前，无法对 Level 2 做可核验判定。
- `[FACT/VERIFIED]` G2 已暴露 ≤5° 接缝合同失败（R1/R2 47.66°、R3 16.13°）；该项是 Level 1 的一票否决，必须先清。
- `[INFERENCE]` 若现在要求 Level 2 判 PASS，等于让 AI 用未授权参考图自判视觉合格，违反 DEC-05。
- `[INFERENCE]` 但若 PoC-SM 只按 Level 1 验收，则可能交付一个"结构合法但形态平庸"的人体，达不到负责人要的 Level 2 方向。因此判据必须是 **Level 1 达成 ∩ G-P1-GEO 实质改善**，而不是"Level 1 达成即可"。

## 6. 与工程架构的关系（必须保留）

`[FACT/VERIFIED]` 视觉目标调整**不能**成为推翻以下约束的理由：C++17、Android Kotlin+JNI+NDK、EGL、GLES3/GLSL ES 3.00、自研 Renderer；不用 Unity/Unreal/Godot/Filament；不依赖 Blender/Maya/ZBrush 外部模型资产；不导入 GLB/GLTF/FBX/OBJ 作为生产角色资产；不使用外部美术贴图与动画作为生产依赖；通过代码、数学规则、几何算法生成资产；面向低性能 Android 设备。

`[FACT/VERIFIED]` "高品质视觉目标 ≠ 必须使用高面数网格，也 ≠ 必须使用外部模型"。

`[RECOMMENDATION]` 实现路径按任务书第八节：Anatomy-driven Geometry → Rig-aware Surface Construction → Anatomical Landmarks → Procedural Surface Patches → Topology Templates → Variable-profile Sweeps → 受控法线与局部细节。`[FACT/VERIFIED]` SDF Smooth Union 不得作为最终人体几何的默认解决方案；G2 的 R3 失败正是包络表达力不足的实证。

## 7. 本轮未决（已随负责人指示更新）

`[FACT/VERIFIED — 负责人已答复，2026-11-24]`

1. ~~是否批准 High-quality Stylized Realism 为最终视觉方向？~~ → **已定：Level 2 为核心目标，Level 3 为长期方向**（§4.1）。
2. ~~是否批准 Level 1 为 PoC-SM 当前门禁、Level 2 为项目核心目标？~~ → **已定：Level 2 核心 / Level 3 长期；P1 证明结构·曲面·驱动，P2 证明换装·模块化**（§4.1、§4.2）。
3. ~~Vincent 是否正式降级为"历史不足案例"？~~ → **已定：负责人明确要求不再沿用 Vincent 作为主视觉标杆**，降级为历史不足案例。

`[OPEN QUESTION]` 剩余：

4. Vincent 是否需要写入 DEC-05 的"明确排除"清单（而非仅"不作主标杆"）？
5. 五 Gate 分离是否写入 SPEC Gate 链作为 GATE-04 之后的细分？
6. Canonical Reference Sheet 由谁制作、何时交付（见 Deliverable 6）。
