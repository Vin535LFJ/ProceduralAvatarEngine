# PAE Human Template 格式设计（R5）

任务：P1-VR-ARCH-1 / Deliverable R5  
日期：2026-11-26（合并版）  
状态：**草案 v0，Host-only 原型已实现并往返验证**。不是正式运行时格式；未获授权进入 Android。

## 1. 定位

`PAEHumanTemplate` 是**离线编译**的、带版本号的人体基础模板。运行时只读取它，不读取 OBJ、不读取 MakeHuman/MPFB2/Blender 的任何格式。模板里的四边形拓扑、稳定顶点 ID、区域、Landmark、Shape Target、Rig V2、权重和来源信息，都是 PAE 自己管理的数据。

它与外部模型的关系只有一条：**编译期**从许可证明确的来源转换一次，并把来源的 commit、文件哈希和许可证状态写进模板。

## 2. 逻辑结构

```cpp
struct PAEHumanTemplate {
    TemplateIdentity     identity;        // 名称、拓扑版本（= 源网格 sha256 前 16 位）、格式版本

    BaseTopology         topology;        // 体表：四边形面 + 每面角 UV + 稳定顶点 ID（米）
    std::vector<D3>      auxPositions;    // 辅助顶点：helper + joint 立方体，只有位置（米）

    AnatomicalRegionMap  regions;         // 每个稳定顶点 ID 一个区域标签
    AnatomicalLandmarkSet landmarks;      // 关节中心 + 表面标志，各自带来源与置信度
    ShapeTargetLibrary   shapeTargets;    // 每个目标：顶点索引 + 位移（米）
    HumanRigV2           rig;             // 骨名、父骨、head/tail、roll 参考平面法线、解剖角色
    SkinWeightData       weights;         // 每顶点 ≤N 个 (骨索引, 权重) + 回退规则标记，覆盖体表 + 辅助顶点
    UVData               uv;              // 并入 topology（每面角的 vt 索引 + vt 坐标）
    MaterialRegionData   materialRegions; // 本轮为空：源网格没有材质分区
    TemplateProvenance   provenance;      // 来源仓库、commit、逐文件 sha256、许可证状态、转换器版本、时间、单位换算
};
```

## 3. 各部分的定义

### 3.1 BaseTopology

| 内容 | 定义 | 本轮实测 |
|---|---|---|
| 顶点 | 稳定 ID = 源 OBJ `v` 行序号（0 基）；体表是连续前缀 0..13,379 | 13,380 |
| 边 | 不单独存储，由面推出 | 26,756 |
| 面 | 多边形，存每面角数 + 顶点索引 | 13,378，全部四边形 |
| Winding | 保持源文件绕向，不翻转 | 外向、内部边全部一致 |
| UV | 每面角一个 `vt` 索引；`vt` 坐标单独存 | 21,334 |
| 三角形 | **不在模板里**。Render Mesh 阶段按 PAE 自己的规则三角化 | 最小角 3.19°（短对角线剖分）/ 3.06°（固定剖分），说明剖分规则要单独设计 |
| Topology Version | `makehuman-base.obj@sha256:<前16位>` | `8e761e6624b8f545` |

**不三角化的理由**：四边形是区域选择、局部加密、Subdivision、Loop 细分和环线分析的基础；过早三角化会丢掉这些能力，也会把“选哪条对角线”这个质量决定冻结在数据里。

### 3.2 辅助顶点（`auxPositions`）

源 OBJ 里体表之外还有 5,778 个顶点：125 个关节立方体（750 面）和 13 个 helper 组（4,358 面：眼球、牙齿、舌、睫毛、头发、生殖器、tights、skirt）。本轮验证了两件必须把它们带进模板的事实：

1. Shape Target 也会移动它们（肩宽 target 17.6 % 的位移量、髋宽 target 51.7 % 在 helper 几何上）。
2. Rig 的关节中心 = 关节立方体顶点均值；不带它们，骨架不随体型变化。

模板 v0 只存它们的位置，不存面。哪些 helper 需要面（眼球、牙齿、舌作为可见几何或附着点）留给后续版本。

### 3.3 AnatomicalRegionMap

覆盖 Head、Neck、Thorax、Abdomen、Pelvis、ShoulderGirdle、UpperArm、Forearm、Hand、Thigh、LowerLeg、Foot 共 12 个区域，必要时再分手指、脚趾、面部子区。

本轮的区域标签是**启发式**的（最近骨 + 半径归一化，躯干按髋关节到颈根的高度分带：< 28 % 骨盆、28–50 % 腹部、> 50 % 胸廓），结果：head 2,723、neck 1,502、thorax 741、abdomen 333、pelvis 463、shoulder-girdle 374、upper-arm 603、forearm 398、hand 3,136、thigh 644、lower-leg 311、foot 2,152。neck 偏大（把部分下颌/颏部算了进去），标签边界不准确。**正式版需要 PAE 自己标注，或在模板里存每顶点的区域多边形/场函数参数**。源网格没有任何区域标签可用。

### 3.4 Landmark

| 类别 | 来源 | 本轮数量 | 说明 |
|---|---|---|---|
| 关节中心（`JointCenter`） | rig 文件的关节定义 → 关节立方体顶点均值 | 204（102 骨 × head/tail） | 骨架用，随形态重算 |
| 表面标志（`Surface`） | 解剖标志点（肩峰、胸骨上切迹、脐、髂前上棘…） | **0** | 源网格不提供，必须 PAE 自己标 |
| 派生极值（`Derived`） | 包围盒极值：头顶、足底、左右最外、前后最外 | 6 | 置信度 `auto-derived`，只用于测量口径 |

**表面 Landmark 不等于骨关节中心**：肩关节中心在体表内 4 cm 以上，髋关节同理。两者分别存储，各自带 `source` 与 `confidence`。`joint-*` 辅助体也不能当作表面标志。

### 3.5 ShapeTargetLibrary

| 字段 | 定义 |
|---|---|
| `id` / `name` | PAE 参数名（如 `shoulderWidth`） |
| `region` | 允许影响的区域（实测得出，见 R3） |
| `relPath` / `sha256` | 源文件与哈希 |
| `rangeMin` / `rangeMax` | 本轮固定 0..1；官方滑杆范围未取得 |
| `side` | 中线 / 左 / 右 / 左右成对 |
| `vertex` + `delta` | 位移（米），索引为稳定顶点 ID，**可指向辅助顶点** |
| 组合规则 | 加法：`V = V0 + Σ αᵢ ΔVᵢ`；incr/decr 各存一份，α<0 取 decr |

本轮写入模板 18 个目标（9 参数 × incr/decr）。未做量化（全部 float64 位移），因此模板 4.1 MB；正式版应把位移量化到 0.1 mm（源文件精度）并压到 int16/int32。

### 3.6 HumanRigV2

每根骨：`name`、`parent`（索引）、`head`、`tail`（米）、`planeNormal`（roll 参考）、`role`（PAE 解剖角色枚举）、`deform`、`headJoint`/`tailJoint`（来源关节名）。骨骼独立于 22 骨骼定义；102 骨 deform 与 20 骨 core 是同一结构的两个实例（R4 §2）。roll 目前只存参考平面法线，蒙皮旋转仍用世界轴——**把 roll 用作局部旋转轴尚未实现**。

### 3.7 SkinWeightData

每顶点：影响数（u8）+ `N × (骨索引 u16, 权重 f32)`；`maxInfluences` 是数据，不是常量（本轮 4）。覆盖全部 19,158 个稳定顶点 ID（体表 + 辅助）。记录 `source`（文件路径或生成器）、`license`、`generator`。

**格式 2 新增**：每顶点一个回退规则标记（`Source` = 直接来自源权重；`KeptAncestor` = 源权重全部落在被剔除的骨上，已归到最近的保留祖先；`NearestBone` = 源文件里没有条目，绑到最近的骨）。读取时拒绝未知标记值，并拒绝“标记非 Source 但影响数为 0”的顶点。这样运行时和离线工具都能知道哪些顶点的权重不是作者数据（R4 §4.2）。

### 3.8 MaterialRegionData

源 OBJ 无 `usemtl`，本轮为空。正式版需定义皮肤 / 指甲 / 眼球 / 口腔 / 头发的面集合或顶点权重。

### 3.9 TemplateProvenance

已实现并写入模板：来源仓库 URL、commit、许可证摘要、转换器版本、UTC 时间、单位换算（0.1 m/单位）及说明、逐文件清单（路径 / sha256 / 字节 / 角色 / 许可证状态 / 依据）。本轮 28 个文件。

许可证状态只有三种取值：`CC0-1.0`（§C 列举 + 文件头）、`PENDING_CONFIRMATION (CC0 declared in file only)`（骨架与权重两个文件）、`LICENSE_UNVERIFIED`（不得进入模板）。

## 4. 二进制容器（`PAHT` v2）

小端；字符串 = u32 长度 + UTF-8；浮点 f64（位置、位移、UV）与 f32（权重）。

| 段 | 内容 |
|---|---|
| 头 | `"PAHT"`、u32 版本 = 2、名称、拓扑版本、单位、上轴、前轴 |
| 拓扑 | u32 顶点数 + 顶点×3 f64；u32 UV 数 + UV×2 f64；u32 面数 + 每面 (u8 角数, 每角 u32 顶点 + i32 UV) |
| 辅助顶点 | u32 数 + 顶点×3 f64 |
| 区域 | u32 数 + 每顶点 u8 |
| Landmark | u32 数 + 每项 (u8 类别, 名, 位置, 来源, 置信度) |
| Shape Target | u32 数 + 每项 (id, 名, 区域, 路径, sha256, 范围, side, u32 行数, 每行 u32 顶点 + 位移) |
| Rig | 名称、来源、许可证状态、i32 格式版本、u32 骨数、每骨 (名, i32 父, head, tail, 平面法线, u8 角色, u8 deform, 两个关节名) |
| 权重 | u8 最大影响数、来源、许可证状态、生成器、u32 顶点数、每顶点 (u8 影响数, 每项 u16 骨 + f32 权重, **u8 回退规则**) |
| 来源 | 仓库、commit、许可证摘要、转换器版本、时间、单位换算、说明、u32 文件数、每文件 6 个字符串/整数 |

读取时校验：magic、版本、面角数 ≥ 3、顶点/UV/骨索引范围、父骨合法且非自引用、权重覆盖全部顶点、权重引用的骨存在、回退规则合法、无尾部字节。

**尚缺**：段级与整文件的校验和、schema 版本协商、可跳过的可选段。这三项是链 B 在 `PAEHUM02` 提案里列出的，本格式应吸收（§6）。

## 5. 本轮验证

| 项 | 结果 |
|---|---|
| 往返 | 序列化 → 解析 → 再序列化，**逐字节一致**（测试 T12a、流水线 `roundtrip=byte-identical`） |
| 内容 | 13,380 体表顶点 + 5,778 辅助顶点、13,378 四边形、210 landmark、18 target、102 骨、19,158 顶点权重（T12b） |
| 回退标记 | 格式 2 的每顶点回退规则随模板往返不丢失（T12e） |
| 顶点序 | 模板位置与源体表逐位相等（T12c） |
| 落盘哈希 | `pae-human-template-mh-a8bc2d54.paht`，**4,120,574 字节**（T12d 校验单次运行内的一致性） |
| 中性导出 | 三角化 OBJ 868,185 字节，重读后 26,756 个三角形、顶点序不变（T13a） |

模板文件的 SHA-256 **每次运行都不同**（内含 UTC 时间戳），因此证据目录不收模板文件，只收日志；要取得可复现的哈希，需要把时间戳移出内容或改存构建号。

## 6. 与链 B 的 `PAEHUM01` / `PAEHUM02` 提案的关系

链 B 的单文件 proof 写出 `PAEHUM01`：三角网格 + 163 骨（名、父、head/tail）+ 每顶点 4 个 `(bone, weight)` + 8 个 target 的体表 delta + 固定 commit 与 base/rig/weights 的 SHA-256 + 许可证状态字符串。它只写不读，不含四边形、UV、区域、Landmark、roll、权重来源、LOD 和校验和。

它同时提出了正式格式 `PAEHUM02` 的段清单，其中三样是本格式（`PAHT`）目前没有、应当吸收的：

| 提案段 / 原则 | 现状 | 建议 |
|---|---|---|
| `ValidationReport`、`FooterChecksum`、section checksum | `PAHT` 只有结构校验，无校验和 | 加段级与整文件校验和 |
| 显式 little-endian 定长编码、未知可选段可跳过 | `PAHT` 是显式小端，但没有可选段机制 | 加可选段与 schema 协商 |
| `LodLineage`（parent face、barycentric、refinement path） | 未做 | 见 §7 |
| `LocalShapeBasisLibrary`（PAE 自写形态基） | 未做 | VR-1B WP3 |
| 失败关闭：索引越界、权重非法、provenance 不完整时拒绝加载 | 已实现大部分 | 补许可证状态为 `LICENSE_UNVERIFIED` 时拒绝 |

两个格式不并存：`PAHT` 是唯一继续演进的模板格式，`PAEHUM01` 只是链 B 的导出物。

## 7. 尚未设计 / 未决

- LOD 与稳定顶点对应：模板 v0 只有一级。可行方向是把四边形环线作为细分层级（Catmull-Clark / Loop），子级顶点记录父面重心坐标——R2 的 `LodCorrespondence` 思路可迁移，本轮未做。
- 手部、面部的局部加密区域及其与基础顶点的对应。
- 材质区域、眼球/牙齿/舌的面数据。
- 位移量化与模板压缩（当前 4.1 MB）。
- 区域标注与表面 Landmark 的正式来源。
- 单位：0.1 m/单位是推断，需在来源说明中确认。
