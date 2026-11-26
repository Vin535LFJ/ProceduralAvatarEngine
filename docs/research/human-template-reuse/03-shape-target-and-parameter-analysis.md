# R3：人体参数化与 Target 解析报告

任务：P1-VR-ARCH-1 / Deliverable R3  
日期：2026-11-26（合并版）  
状态：工具实测。链 A 的数值来自 `evidence/host-proof/report/shape-targets.md` 与 `logs/pae_ht_tests-final.log`；链 B 的数值来自 `logs/template-proof.log`。

## 1. Target 格式（以数据为准）

| 问题 | 结论 | 证据 |
|---|---|---|
| 文件形态 | 纯文本。`#` 开头为注释（含 CC0 声明）；数据行 4 个空白分隔字段：`<顶点索引> <dx> <dy> <dz>` | 解析器统计；`measure-shoulder-dist-incr.target` 5,568 行 |
| 顶点索引 | **0 基**，指向 `base.obj` 的 `v` 行序号，**可指向 helper/joint 顶点** | `head-trans-up.target` 首行为 `0 0 .3 0`；按 0 基解释时位移集中在头部（跨度 0.458 m），按 1 基解释散到全身（1.872 m） |
| 轴顺序 | **X Y Z，Y 不取反**（MakeHuman 网格坐标系：+Y 上、+Z 前、+X 角色左侧） | `torso-trans-forward` 位移只在第 3 列；`head-trans-up` 只在第 2 列且为正；`torso-trans-in` 只在第 1 列 |
| 与 MPFB2 文档的差异 | MPFB2 `target.md` 写的是 “X Z Y、Y 取反”——那是它导入 **Blender**（Z 上）时的约定，不是 MakeHuman 网格帧的格式。照文档直接实现会得到错误的位移方向 | 实测三轴 + 符号测试 T05b–T05e |
| 精度 | 三位小数；在 0.1 m/单位下即 **0.1 mm** | 文件内容 |
| 分类行 | 非数字首 token 的行被当作 category 头，记录但不解释 | 解析器 `categoryHeaders` |
| 依赖固定顶点索引 | 是。Target 不含拓扑信息，只含索引；与 `base.obj` 的 sha256 绑定 | 格式本身 |
| 权重文件索引 | 同为 0 基（57,107 条全部落在 19,158 个 `v` 之内；出现索引 0） | T10b |

> 更正记录：链 A 最初向负责人报告“索引是 1 基”，那是 numpy 快查时把标签弄反；工具按数据判定后改为 0 基。两条链的最终结论一致（0 基）。

## 2. 组合与数值性质

公式：`V = V0 + Σ αᵢ·ΔVᵢ`。

**链 A** 对**完整 OBJ**（体表 + helper + joint）应用 target：

| 验证 | 结果 |
|---|---|
| α = 0 | 逐位恢复基础网格（T06a） |
| +α 再 −α | 恢复，误差 < 1e-12 m（双精度舍入；T06b） |
| 线性叠加 | 0.5 + 0.5 与 1.0 一致，误差 < 1e-12 m（T06c） |
| 稳定顶点 ID | 九个参数同时 +1.0 后顶点数、面数、边数不变（T06d） |
| 八个局部参数同时 +1.0 | 翻面 0，边长变化 p99 1.409，体积比 1.363 |
| 八个局部参数同时 +1.5（超出作者设定范围） | 翻面 0，边长变化 p99 1.618，体积比 1.570 |
| 法线 | Target 后法线必须重算（位移不是刚体运动）；工具在每次审计时重算 |

**链 B** 只把 target 映射到体表顶点（`originalToBody`），每个 α = 1 单独应用并做完整拓扑与自相交审计：

| Target（incr） | 体表 delta 数 | 最大增量 | 结果 |
|---|---:|---:|---|
| `torso-scale-horiz-incr` | 5,526 | 34.2 mm | 自交 0，拓扑不变 |
| `torso-scale-depth-incr` | 1,590 | 33.6 mm | 同上 |
| `measure-waist-circ-incr` | 862 | 28.1 mm | 同上 |
| `measure-shoulder-dist-incr` | 4,524 | 24.9 mm | 同上 |
| `hip-scale-horiz-incr` | 1,110 | 37.3 mm | 同上（**仅链 B 验证**） |
| `l-upperarm-shoulder-muscle-incr` | 163 | 16.3 mm | 同上（**仅链 B 验证**） |
| `r-upperarm-shoulder-muscle-incr` | 163 | 16.3 mm | 同上；与左侧镜像，位移质心 (±0.2019, 0.4925, 0.0161)（**仅链 B 验证**） |
| `measure-upperarm-circ-incr` | 540 | 17.3 mm | 同上（**仅链 B 验证**） |
| 8 个组合 | — | — | 自交 0 |

链 A 的“行数”包含 helper/joint 行，链 B 只数体表行，两者相减正好是 helper 行数（例：胸廓宽 7,196 − 1,670 = 5,526）。

**关于链 B 的 `ZERO_RESET max_error=0`**：代码里是把位移乘以 0 再与基础位置比较，恒为 0，**不构成验证**。真实路径上的归零/可逆性证据是链 A 的 T06a–T06c。

## 3. 参数影响（哪个参数动了哪些区域、位移多少）

链 A 实测。均值位移 = 该区域内每个顶点的位移长度平均（不受区域顶点密度影响）。+1.0 表示 incr target 取 1.0，−1.0 表示 decr target 取 1.0。测量单位米。

| 参数 | Target（+ / −） | 行数 | +1.0 时各区域均值位移（mm，前五） | helper 几何占位移量 | 命名测量：基础 / +1 / −1 | 变化（+ / −） | 身高变化 | 翻面 |
|---|---|---:|---|---:|---|---|---|---|
| shoulderWidth | measure-shoulder-dist | 5,568 | 手 24.9、前臂 24.9、上臂 24.8、肩带 11.9、胸廓 1.7 | 17.6 % | 双肩宽 0.5595 / 0.6093 / 0.5325 | +0.0498 / −0.0270 | 0 | 0 / 0 |
| ribcageWidth | torso-scale-horiz | 7,196 | 前臂 34.2、手 34.2、上臂 34.0、肩带 21.8、胸廓 13.9 | 20.1 % | 胸宽 0.2757 / 0.3307 / 0.2207 | +0.0550 / −0.0550 | 0 | 0 / 0 |
| chestDepth | torso-scale-depth | 2,651 | 胸廓 20.8、腹部 11.6、肩带 6.5、上臂 1.5、骨盆 0.1 | 42.7 % | 胸厚 0.2005 / 0.2561 / 0.1703 | +0.0557 / −0.0302 | 0 | 0 / 0 |
| waistWidth | measure-waist-circ | 1,518 | 腹部 17.7、骨盆 3.2、胸廓 1.3 | 43.9 % | 腰宽 0.2971 / 0.3139 / 0.2803 | +0.0168 / −0.0168 | 0 | 0 / 0 |
| hipWidth | measure-hips-circ | 1,418 | 骨盆 18.6、大腿 1.3、腹部 0.4 | 51.7 % | 髋宽 0.3699 / 0.4289 / 0.3405 | +0.0590 / −0.0294 | 0 | 0 / 0 |
| armShape | l/r-upperarm-muscle | 638 | 上臂 3.4、肩带 0.2 | 36.5 % | —（未定义命名测量） | — | 0 | 0 / 0 |
| legShape | l/r-upperleg-muscle | 948 | 大腿 3.5、骨盆 1.5 | 47.4 % | — | — | 0 | 0 / 0 |
| headShape | head-scale-horiz | 5,240 | 头 9.1、颈 3.7、胸廓 0.1 | 15.2 % | — | — | 0 | 0 / 0 |
| stature（单独使用） | macro height max / min | 19,150 | 头 687.9、颈 669.8、肩带 628.9… | 28.0 % | 身高 1.6659 / 2.3884 / 1.2995 | +0.7225 / −0.3664 | 同左 | 0 / 0 |

### 3.1 怎么读这张表

- **肩宽和胸廓宽 target 会把整条手臂向外平移约 25–34 mm**（手、前臂、上臂均值几乎相同）。这是正确的解剖联动（手臂长在躯干上），但意味着这两个参数不是“只改躯干”。
- **局部性是软的**：`upperleg-muscle` 有 1.5 mm 渗入骨盆/臀区（相当于其大腿效应的 43 %）；`head-scale-horiz` 带动颈部 3.7 mm。PAE 不能假设 target 只影响它命名的区域，参数→区域映射必须以实测为准。
- **helper 几何占位移量的 15–52 %**：target 也在移动关节立方体和 helper。骨架随形态变化正是靠这一点（见 §4）。
- **stature 的 macro height target 不能单独使用**：在中性基础网格上直接取 1.0 得到 2.39 m。它是为 MakeHuman 的宏参数组合（性别 × 年龄 × 肌肉 × 体重 × 身高）设计的，PAE 要么实现等价的组合公式，要么自己写身高形态基。本轮**未验证**其正确用法。
- **+1 与 −1 不对称**：肩宽 +49.8 / −27.0 mm，髋宽 +59.0 / −29.4 mm。incr 与 decr 是两个独立文件，不是同一条直线的两端；参数映射必须按 α∈[−1,1] 分段标定。

### 3.2 局部性判据（测试 T07a，先定口径再测）

对每个局部参数预先给出“允许影响的区域集合”，要求集合外每个区域的均值位移 ≤ max(最强允许区域的 2 %，0.5 mm)。结果 8/8 通过（legShape 的骨盆渗入按实测被列入允许集合并在此登记，不是隐藏）。

### 3.3 测量口径（必须先于数值声明）

- 身高 = 体表包围盒 Y 跨度：1.6659 m。
- 胸平面 = 身高 72 %、腰平面 = 60 %（自脚底），带宽 ±1.5 % 身高，只统计对应躯干区域顶点。
- 髋平面 = 髋关节高度，统计骨盆 + 大腿区域。
- “双肩宽”平面 = 肩关节高度 − 2.5 % 身高，统计肩带 + 上臂区域。**在 A 姿势下它包含上臂外缘，数值（0.56 m）偏大，不能与 ANSUR II 的 bideltoid/biacromial 直接比较。**
- 区域标签是启发式的（最近骨 + 半径归一化，躯干按高度分带），不是解剖标注。
- 这些平面是 PAE 为本轮自定义的，**不是** MakeHuman 的测量平面；因此 `measure-*-circ` 类 target 的名义围度变化 **UNVERIFIED**（本轮只测了宽度，没测围度）。

## 4. Target → Rig 的联动

关节位置 = rig 文件中关节立方体 8 个顶点在**当前**表面上的均值。因为 target 也移动这些 helper 顶点，所以对变形后的完整 OBJ 重新解析关节，骨架会跟随体型：

- 身高 target +1.0：身高 1.6659 → 2.3884 m；头部关节 y 0.6793 → 1.3533 m；肘关节随之上移（测试 T14a）。
- 反例（本轮真实踩到的坑）：最初只对体表 13,380 个顶点应用 target，关节立方体保持原位，结果“身高变了但骨架没变”（见 R6 §4 失败记录）。
- 链 B 把 target 只映射到体表顶点，所以它的骨架不会随形态变化；它也没有测试这一点。**模板必须携带辅助顶点的位置**（R5 §3.2）。

## 5. Target → PAE `CharacterDefinition` 映射

| CharacterDefinition 字段（schema 3） | 默认值 | 可对应的 Target | 状态 |
|---|---|---|---|
| `statureM` | 1.7549 m | macro height（需组合公式）或 PAE 自写 | **未解决**：单独使用不成立 |
| `shoulderBreadthRatio` | 0.290 | `measure-shoulder-dist-incr/decr` | 可用；+0.0498 / −0.0270 m 不对称，需分段标定 |
| `biacromialRatio` | 0.237 | 无直接 target | 需 PAE 自写 |
| `hipBreadthRatio` | 0.196 | `measure-hips-circ-incr/decr` | 可用；+0.0590 / −0.0294 m |
| chest/hip 的另一组源 | — | `hip-scale-horiz-incr`（骨盆横向） | 链 B 验证了 incr 方向（最大 37.3 mm），未与 `measure-hips-circ` 比较 |
| `chestDepthRatio` | 0.144 | `torso-scale-depth-incr/decr` | 可用；+0.0557 / −0.0302 m |
| `abdominalDepthRatio` | 0.143 | `torso-scale-depth`（同时影响腹、骨盆） | 可用但非独立 |
| `waistCircumferenceRatio` | 0.5325 | `measure-waist-circ-incr/decr` | 宽度可用（±0.0168 m）；围度 UNVERIFIED |
| `upperArmLengthRatio` / `forearmLengthRatio` | 0.185 / 0.168 | `measure-upperarm-length`、`measure-lowerarm-length` | **本轮未测** |
| `upperLegLengthRatio` / `lowerLegLengthRatio` | 0.259 / 0.254 | `measure-upperleg-height`、`measure-lowerleg-height` | **本轮未测** |
| `handLengthRatio` / `footLengthRatio` | 0.108 / 0.152 | `l/r-hand-scale`、`l/r-foot-scale` | **本轮未测** |
| `headHeightRatio` | 0.125 | `head-scale-vert` | **本轮未测**（本轮测的是 `head-scale-horiz`） |
| `TorsoShapeParameters.*`（relief、contour） | — | `torso-muscle-*`、`stomach-tone`、`pelvis-tone`、`torso-vshape` 等 | **本轮未测** |
| `ShoulderShapeParameters.*` | — | `l/r-upperarm-shoulder-muscle` | 链 B 验证了 incr 方向（163 个 delta，最大 16.3 mm，左右镜像）；decr 未测 |
| 上臂围度 | — | `measure-upperarm-circ-incr` | 链 B 验证了 incr 方向（540 个 delta，最大 17.3 mm）；对应字段需核对 schema |
| `stylization` | 0.25 | 无对应 target（MakeHuman 无卡通化 target） | **必须由 PAE 自写 Shape Basis** |
| `AsymmetryParameters.*` | 0 | `asym/` 目录下的单侧 target | 机制可行（单侧 target 已验证），**具体 target 未测** |

现有十项 legacy `ShapeParameters` 在 R2 基线上已是 `DELTA max_all=0.000000`（惰性）。映射到 target 不会自动让它们生效，仍需经过 typed 字段接线。

## 6. 哪些形态靠 Target、哪些必须 PAE 自己写

| 形态 | 判断 |
|---|---|
| 身高、肩宽、胸廓宽/厚、腰宽、髋宽、肢体长度/围度、肌肉/脂肪量、头部整体比例 | **可复用 CC0 target**（两条链合计验证 26 个 target 文件；其余 1,254 个未验证） |
| 左右不对称 | 单侧 target 机制成立，需逐个 target 核查 |
| 卡通化 / 风格化 | 无 target，PAE 自写 |
| 姿势修正形态（corrective） | 不是 target 的职责，PAE 在 Rig V2 层做 |
| 面部表情 | `expression/` 目录存在，本轮未使用未验证 |
| 骨骼随体型变化 | 通过 helper 关节立方体自动实现，不需要额外 target |

## 7. 稳定顶点 ID 的两种做法

链 A：稳定 ID = 源 OBJ `v` 行序号（体表是连续前缀 0..13,379，helper/joint 紧随其后）。链 B：`stableVertexId = FNV 风格哈希("PAEH", 原始顶点序号)`，共 13,380 个。两种都**只在固定的 `base.obj` 哈希与拓扑版本内有意义**；换基础网格版本必须重新编译并重新映射 target。链 A 的做法保留原始顺序、可直接寻址，模板格式（R5）采用它。

## 8. 未验证项

- 其余 1,254 个 target、所有 `*-circ` 的围度、宏参数组合公式、参数的官方取值范围。
- 链 B 独有的 4 个 target 的 decr 方向与组合行为。
- Target 之后的自相交：链 B 覆盖了 8 个 incr 与其组合（均为 0）；链 A 的 decr 方向以及臂形、腿形、头形、身高 target 未覆盖。
- ±1.0 之外只有 +1.5 一组过冲测试。
