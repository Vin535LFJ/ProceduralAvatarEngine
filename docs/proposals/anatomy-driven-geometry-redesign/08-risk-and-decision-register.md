# 08 — Risk and Decision Register

## 1. 风险登记

| ID | 风险 | 概率/影响 | 证据或原因 | 缓解策略 | 状态 |
|---|---|---|---|---|---|
| R-01 | Anatomy Shape Model 继续退化为大量独立几何参数 | 高/高 | 既有 BodyParameters 与 HumanAnatomyParameters 有大量乘数，参数耦合不足 | 分层比例/体积/轮廓参数；Derived Controls；参数依赖图；限制 UI 暴露底层半径 | OPEN |
| R-02 | 统一 SDF 仍只生成 blob / balloon | 高/高 | M12 水密但场主要是椭球、胶囊、圆锥胶囊 | 真正接入 pectoral/trap/deltoid heads/biceps/triceps/flexor-extensor/rectus 等 anatomy features；独立视觉 Gate | OPEN |
| R-03 | Smooth Union 填平腋窝或抹掉区域层次 | 高/高 | 正体积 union 对凹面表达弱，blend radius 可改变局部轮廓 | axilla concavity / negative envelope；joint-aware field；多姿态验证 | OPEN |
| R-04 | Hybrid 在几何边界再次出现 seam | 中/高 | 局部 SDF/patch 输出分开 mesh 会重新遇到边界焊接问题 | 所有贡献进入同一个 skin-envelope；否则必须共用模板和真实 C0/G1 boundary contract | OPEN |
| R-05 | Surface Nets 拓扑无动画 Edge Flow | 高/高 | 体素 lattice 驱动顶点布局，无关节环线语义 | 固定 anatomy quad template 投影/优化；Surface Nets 只作 reference surface | OPEN |
| R-06 | 自动 Retopo 产生不稳定 topology | 中/高 | Quad remesh 目标与角色变形不同 | 固定模板 + semantic field alignment；锁定模板版本；避免每次随机 remesh | OPEN |
| R-07 | 参数极端组合造成 self-intersection / 负体积 | 中/高 | Landmark/feature field 互相作用和局部参数依赖 | ShapeValidator、约束投影、参数边界、极端组合测试 | OPEN |
| R-08 | PTF/截面 frame 在反向或退化切线翻转 | 中/中 | 现有 PTF 没有显式 anti-parallel case；局部 fallback 不统一 | 去重采样、稳定 quaternion fallback、frame continuity tests | OPEN |
| R-09 | 22 Bone LBS 无法满足肩/肘变形 | 高/高 | LBS 固有体积收缩/扭转，当前只有四权重 | 在不改变 ABI 下先做 semantic weights + corrective envelope；DQS/LVP/Corrective 作为后续 ADR | OPEN |
| R-10 | 过早增加 DQS / 肌肉模拟扩大范围 | 中/中 | 首轮目标应验证静态形态和基础动作 | PoC-A/B/C 逐阶段证据驱动，暂不实施 DQS/物理肌肉 | OPEN |
| R-11 | Geometry Worker 复杂度破坏 RenderThread | 中/高 | 当前 RenderThread 同步生成几何 | 新 Worker 只通过 Runtime + bounded queue 接入；不大规模重构 RenderThread | OPEN |
| R-12 | EGL Context loss 后 GPU resource state 不一致 | 中/高 | 当前 release path 未释放 anatomy mesh/line buffers；CPU mesh恢复契约不完整 | Context lifecycle audit；CPU MeshData cache；所有 GL resource 仅 RenderThread 创建/删除 | OPEN |
| R-13 | 新 Vertex/UV/region metadata 破坏现有 64B GPU layout | 中/中 | 当前上传未包含 UV | CPU metadata 与 GPU attributes 分离；需要时通过 ADR 演进 | OPEN |
| R-14 | Android TV Box / 手机性能无法从单一 M12 设备外推 | 高/中 | 历史 51.7 FPS 只来自一台 Android 8.1/GLES3 设备 | 目标设备矩阵与 P95/P99 实测；限制分辨率与 buffer memory | OPEN |
| R-15 | 视觉证据不可复核 | 高/高 | 本次无法读取远程 PNG 原文件；历史报告和图片可能不是同批次/同机位 | 归档原始 PNG + manifest；固定相机/光照；图像复核；不使用文件名代替视觉 | OPEN |
| R-16 | 主线正式 ADR / Foundation contract 相互冲突 | 高/高 | feature Foundation 03 将 Surface Patch 写为主线；main ADR-009 已采纳 SDF 上半身 | 先列 ADR conflict map；新方向经负责人审阅后再写正式 ADR | OPEN |
| R-17 | 形体目标不明确（不同风格体型） | 中/高 | 卡通风格可包含大头、写实比例、夸张运动型等多目标 | 第一 PoC 只锁一个测试 archetype，后续开放 profile family | OPEN |
| R-18 | 视觉与工程 Gate 混合 | 高/高 | 过去报告容易以 topology/FPS PASS 代替视觉验收 | 每个阶段独立 Visual Gate + Engineering Gate，视觉可独立否决 | OPEN |

## 2. 待项目负责人决定的事项

| ID | 决策项 | 候选 | 当前建议 | 状态 |
|---|---|---|---|---|
| D-01 | 目标测试体型 | 中性/男性化/女性化/多个 profile | 先定一个中性、轻度运动型 baseline | OPEN |
| D-02 | 第一轮生成表面 | 当前 SDF；B-spline envelope；hybrid field | 采用 shared anatomy field/envelope，保留 B-spline guide controls | OPEN |
| D-03 | 最终动画 topology | Surface Nets；直接参数化 quad；模板投影 | 固定 quad template 投影到共享参考表面 | OPEN |
| D-04 | Hybrid 边界策略 | 各部件 mesh 焊接；共同场；严格 patch contract | 首选共同 envelope，禁止分部件裸拼 | OPEN |
| D-05 | 第一 PoC scope | 只做单个截面/单臂；完整 torso；包含肩根 | 完整胸腹腰盆 + 双侧肩连接锚点，不生成单臂局部管 | OPEN |
| D-06 | 肩/肘 corrective | LBS；DQS；Corrective Shapes；LVP | 第一阶段保留 LBS ABI；依据动作证据决定下一步 | OPEN |
| D-07 | 第一目标设备 | 现有 TV Box；最低端手机；代表性多机型 | 至少现有 TV Box + 一台 Android phone | OPEN |
| D-08 | 是否变更冻结的 Foundation 03 / ADR | 原合同；新 Hybrid 路线；分层兼容 | 若采纳新路线，先提交 ADR 更新，不直接改冻结文档 | OPEN |
| D-09 | AnatomyFeatures 方向 | 废弃/接入/重新定义 | 重新定义为 Shape Model 的 muscle feature set，不直接沿用当前半成品 API | OPEN |
| D-10 | 商业品质目标量化 | 只由视觉审稿；评分矩阵；参考角色标准 | 固定一个授权参考视觉目标和评分 rubric | OPEN |

## 3. 明确不做的决策

- 本次不决定新类/目录的最终 C++ 命名。
- 本次不决定用 NURBS 替换现有曲面系统。
- 本次不宣布 Surface Nets 永远弃用。
- 本次不决定完整身体、面部、头发、服装的生成方式。
- 本次不增加/重排 BoneId，不改变 UBO ABI。
- 本次不进入 P1-B，也不宣称新架构已经通过视觉 Gate。

## 4. 证据缺口

1. 项目 PNG 的独立像素审查尚未完成。
2. P1-A BoundaryReport 的原始 seam 对应与法线数据需要重新采集；不能从历史 hard-coded report 反推。
3. 当前 feature 分支 HumanAnatomyGenerator 在各种参数极端组合下的 self-intersection/topology 稳定性未做本次重跑。
4. 当前 Renderer 在真实 `EGL_CONTEXT_LOST` 情况下的资源恢复路径需要设备验证。
5. 新 shape model、topology template、field backend 的性能没有任何实测数据。
