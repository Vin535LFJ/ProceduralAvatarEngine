# VR-1A-R2 目标解剖形态与测量定义

日期：2026-11-25
基线：`299ae3bf3fcb6d76be563f815aa52380e018d50b`
状态：**`DRAFT — 待负责人确认目标比例与形态方向`**

本文件定义 R2 的几何目标，不是生产代码授权，也不是视觉 Gate。所有目标值在负责人确认前保持 `Assumed`，不得自动升级为 `Measured` 或 `Accepted`。

## 1. 坐标、单位与置信度

- 单位：米。
- 坐标：`+X` 为人物左侧，`+Y` 向上，`+Z` 为前方。
- `H`：stature，当前默认 `1.7549 m`。
- `yBottom`：R1 局部 Patch 的骨盆下缘，当前 `0.941572 m`。
- `yNeckBase`：当前 resolved neck base，`1.468507 m`。
- 躯干归一化高度：`s = (y - yBottom) / (yNeckBase - yBottom)`。
- 同时记录 `y/H`，便于跨身高缩放。
- `Measured`：有明确第一方人体测量来源。
- `Derived`：仅由 Measured 值算术推导。
- `Assumed`：本项目选择的形态目标，必须由负责人确认。

## 2. 当前默认比例是否适合最终成人视觉目标

结论：**当前 ProportionProfile 可作为尺度参考，但不能直接作为最终审美形态目标。**

实测代码输出：

| 项目 | 当前值 | 占 H | 来源/问题 |
|---|---:|---:|---|
| Stature | 1.754900 m | 100% | ANSUR II male 50th |
| Bideltoid shoulder breadth | 0.508921 m | 29.00% | Derived |
| Biacromial breadth | 0.415911 m | 23.70% | Derived，但未控制 mesh |
| Chest width | 0.290611 m | 16.56% | Assumed aspect 1.15 |
| Chest depth | 0.252706 m | 14.40% | Measured source |
| Waist width | 0.329489 m | 18.78% | Assumed aspect 1.25 |
| Waist depth | 0.263591 m | 15.02% | Assumed aspect 1.25 |
| Hip breadth | 0.343960 m | 19.60% | Derived |
| Upper-arm length | 0.324656 m | 18.50% | Assumed |

主要问题：

1. 语义腰宽 `0.3295 m` 大于胸宽 `0.2906 m`，腰深 `0.2636 m` 也大于胸深 `0.2527 m`。
2. R1 的 waist pinch 只压缩宽度，不压缩深度，无法纠正“腰比胸更深”的桶状轮廓。
3. 胸宽来自假定 aspect ratio，不是测量值。
4. stature 来自 ANSUR II male 50th，腰围来自 CDC NHANES male 20–29 median，来源人群并不完全一致。
5. 男性统计尺寸不等于最终人物审美目标；R2 不应自动把性别、年龄或统计分位当作视觉身份。
6. 当前肩关节和 acromion 的 Y 位置低于 chest center，且这些 landmark 没有控制肩部曲面。

因此 R2 保留当前身高作为统一尺度，但重新定义胸、腰、肩带和上臂的形态控制值。

### 2.1 视觉方向与比例角色

长期目标仍是自然半写实的亚洲成年人物，但当前 ANSUR II/CDC 数据不能证明其已经满足该审美方向。R2 的比例只用于建立第一版可测量几何，不作为最终人体统计标准。

负责人需在授权前选择：

- [ ] 方案 A（推荐）：R2 采用“不指定性别的成人基础比例”，保留亚洲成年审美为长期方向；本轮不把统计分位写成角色身份。
- [ ] 方案 B：R2 明确采用亚洲成年男性半写实比例，并另附经负责人批准的测量/审美依据。
- [ ] 其他：________________________________。

无论选择哪项，07 中的 R2 候选轮廓在签署前都保持 `Assumed`，不得写入生产默认值。

## 3. R2 候选全局比例

以下为候选自然成人比例，均为 `Assumed`，需负责人确认：

| 测量 | R1 当前/实际 | R2 候选 | 归一化 | 目的 |
|---|---:|---:|---:|---|
| Chest width | 0.2906 m 语义值；upper ring 约 0.368 m | 0.344 m | 19.60% H | 建立明确胸廓宽度 |
| Chest depth | 0.2527 m | 0.260 m | 14.82% H | 保持前后体积但不继续增宽 |
| Waist width | 0.3295 m 语义值；实际腰环约 0.283 m | 0.264 m | 15.04% H | 明显收腰，低于胸宽 |
| Waist depth | 0.2636 m 语义值；实际腰环约 0.259 m | 0.200 m | 11.40% H | 腰深低于胸深，形成侧向收束 |
| Upper pelvis width | 实际下环约 0.305 m | 0.332 m | 18.92% H | 与髋部衔接，避免下端过窄 |
| Iliac crest width | 无独立站点 | 0.344 m | 19.60% H | 骨盆侧缘可读 |
| Biacromial width | landmark 0.4159 m，未控制 mesh | 0.416 m | 23.70% H | 肩峰间距保持尺度 |
| Visible shoulder-yoke max width | 顶环约 0.472 m | 0.432 m 最大值 | 24.62% H | 肩带 yoke 自身外轮廓，不含三角肌最外侧体积 |
| Final external shoulder width | 无统一定义 | 0.460 m 候选值 | 26.21% H | 含三角肌软组织的最外侧轮廓；必须大于 yoke 宽度，待负责人确认 |
| Shoulder / chest half-width ratio | 肩带插值过宽 | 1.21 | — | acromion 明显位于胸廓外侧 |
| Waist / chest width ratio | 语义 1.134；实际腰环/上胸环约 0.770 | 0.767 | — | 保持收腰，同时重定义胸廓宽度 |
| Waist / chest depth ratio | 语义 1.043；实际腰环/上胸环约 0.992 | 0.769 | — | 腰深明显小于胸深 |
| Chest depth / width | 约 0.87 语义值 | 0.756 | — | 胸廓前深、侧宽平衡 |

### 3.1 肩宽定义与测量归属

三个肩宽概念不得混用：

| 定义 | R2 数值 | 测量对象 | 几何区域 |
|---|---:|---|---|
| Bideltoid statistical breadth | 0.508921 m | 统计测量参考 | 不作为 mesh 外轮廓目标 |
| Biacromial bony width | 0.416 m | 左右 Acromion landmark 中心距 | Shoulder Girdle / Acromion |
| Shoulder-yoke outer width | 0.432 m | 肩带 yoke 自身最外侧点 | Shoulder Girdle |
| Final external shoulder width | 0.460 m 候选 | 含三角肌软组织后的最外侧点 | Upper Arm Root / Deltoid |

规则：

- `0.432 m` 不得直接当作最终人体外肩宽。
- Acromion 控制骨性肩峰和 shoulder yoke 顶点。
- Deltoid lateral 控制最终外肩宽，必须位于 acromion 外下方。
- Upper-arm root center 位于 `x≈±0.225 m`，是 sweep 中心，不是外肩宽测量点。
- 所有宽度测量必须按 `BoundaryId/RegionId` 提取，不能只取整个 mesh 的 AABB。

R2 不把上述数字解释为某个具体人物，只把它们作为本轮 Clay 轮廓的候选几何目标。

## 4. 躯干解剖控制站点

每个站点必须同时定义高度、中心、横向控制、前后控制和局部形态基，不再只用三个宽深区间。

| 站点 | s | y/H | 中心 Z (m) | 半宽 (m) | 半深 (m) | 前/后/侧控制重点 | 主要形态基 | Confidence |
|---|---:|---:|---:|---:|---:|---|---|---|
| Upper Pelvis | 0.00 | 0.5365 | 0.000 | 0.166 | 0.116 | 前侧微凸、后侧臀上延续、侧缘 iliac flare | Pelvic side contour | Assumed |
| Iliac Crest | 0.16 | 0.5846 | -0.002 | 0.172 | 0.120 | 侧缘最高点、前后斜面向腰收束 | Iliac crest relief | Assumed |
| Lower Abdomen | 0.32 | 0.6326 | 0.004 | 0.145 | 0.106 | 腹前平缓、侧腹 oblique、后腰 lumbar | Abdominal/oblique | Assumed |
| Waist | 0.44 | 0.6687 | 0.002 | 0.132 | 0.100 | 前腹平、后腰凹、侧腰收束 | Waist pinch | Assumed |
| Umbilical Level | 0.50 | 0.6867 | 0.006 | 0.138 | 0.104 | 中线微前凸，不做脐孔装饰 | Linea alba contour | Assumed |
| Costal Margin | 0.62 | 0.7227 | 0.004 | 0.150 | 0.112 | 下肋外展、前下缘清楚、侧肋过渡 | Costal margin relief | Assumed |
| Lower Rib Cage | 0.72 | 0.7527 | 0.006 | 0.165 | 0.125 | 侧肋宽度、前胸开始展开 | Ribcage relief | Assumed |
| Chest / Sternum | 0.80 | 0.7768 | 0.012 | 0.172 | 0.130 | 胸骨中线、双侧 pectoral、前后不对称 | Pectoral/sternum | Assumed |
| Upper Rib Cage | 0.88 | 0.8008 | 0.008 | 0.168 | 0.124 | 上胸收口、背部 scapular/trapezius | Upper rib/back | Assumed |
| Shoulder Girdle / Acromion | 0.90 | 0.8068 | 0.000 | 0.208 | 0.098 | 非椭圆：肩峰顶点、锁骨前后弧、斜方肌后坡 | Clavicle/trapezius/deltoid | Assumed |
| Clavicle Level | 0.94 | 0.8188 | 0.004 | 0.190 | 0.090 | 内侧锁骨高、外侧肩峰低，front/back 独立 | Clavicle relief | Assumed |
| Neck Base Interface | 1.00 | 0.8368 | 0.000 | 0.060 | 0.052 | 仅接口环，不生成头颈动画 | Trapezius transition | Derived/Assumed |

站点高度是单调的；肩带站点不能简单把整环变成 `0.208 m` 半宽椭圆，而应由前后独立曲线和左右 acromion 控制点共同构成。

### 4.1 Station × AngularControl × Side 权威数据

R2 统一采用 **8 个角向控制**，不再使用“12 个方向”的不完整表述：

- `F`：Front center。
- `FL/FR`：Front-left / front-right anterolateral。
- `L/R`：Left / right lateral。
- `BL/BR`：Back-left / back-right posterolateral。
- `B`：Back center。

下表坐标均为相对项目原点的绝对 `(x,z)`，不是相对半宽/半深的未知比例。`L/R` 的 Z 使用站点中心 Z；`F/B` 和斜向控制直接给出绝对 Z，因而腰部的 `F=+0.108 m`、`B=-0.096 m` 是唯一权威值。

| Station | L (+X) | R (-X) | F | B | FL / FR (|x|, z) | BL / BR (|x|, z) |
|---|---|---|---|---|---|---|
| Upper Pelvis | (+0.166, 0.000) | (-0.166, 0.000) | (0, +0.128) | (0, -0.122) | (0.150, +0.098) | (0.150, -0.092) |
| Iliac Crest | (+0.174, -0.002) | (-0.174, -0.002) | (0, +0.126) | (0, -0.122) | (0.156, +0.096) | (0.156, -0.092) |
| Lower Abdomen | (+0.148, 0.004) | (-0.148, 0.004) | (0, +0.114) | (0, -0.104) | (0.132, +0.088) | (0.132, -0.078) |
| Waist | (+0.134, 0.002) | (-0.134, 0.002) | (0, +0.108) | (0, -0.096) | (0.118, +0.082) | (0.118, -0.070) |
| Umbilical Level | (+0.140, 0.006) | (-0.140, 0.006) | (0, +0.114) | (0, -0.100) | (0.124, +0.086) | (0.124, -0.074) |
| Costal Margin | (+0.154, 0.004) | (-0.154, 0.004) | (0, +0.122) | (0, -0.108) | (0.136, +0.092) | (0.136, -0.080) |
| Lower Rib Cage | (+0.168, 0.006) | (-0.168, 0.006) | (0, +0.137) | (0, -0.116) | (0.150, +0.104) | (0.150, -0.088) |
| Chest / Sternum | (+0.174, 0.012) | (-0.174, 0.012) | (0, +0.142) | (0, -0.118) | (0.156, +0.112) | (0.156, -0.098) |
| Upper Rib Cage | (+0.170, 0.008) | (-0.170, 0.008) | (0, +0.136) | (0, -0.114) | (0.152, +0.106) | (0.152, -0.094) |
| Shoulder Girdle / Acromion | (+0.216, 0.000) | (-0.216, 0.000) | (0, +0.098) | (0, -0.098) | (0.190, +0.072) | (0.190, -0.074) |
| Clavicle Level | (+0.196, 0.004) | (-0.196, 0.004) | (0, +0.096) | (0, -0.086) | (0.174, +0.070) | (0.174, -0.064) |
| Neck Base Interface | (+0.060, 0.000) | (-0.060, 0.000) | (0, +0.052) | (0, -0.052) | (0.045, +0.040) | (0.045, -0.040) |

角向插值使用 periodic cubic B-spline/Catmull-Rom；左右控制默认镜像。非对称时只允许显式左右参数分别缩放 `L/FL/BL` 与 `R/FR/BR`，`F/B` 中线保持独立，禁止左右交叉混用。

## 5. 正面、侧面、背面轮廓定义

### 5.1 正面

必须测量：

- 胸廓最大半宽：`max |x|`，目标约 `0.174 m`（胸站点名义半宽 `0.172 m` 加侧向控制）。
- 腰最小半宽：`min |x|`，目标约 `0.134 m`（腰站点名义半宽 `0.132 m` 加侧向收束控制）。
- 髂嵴半宽：约 `0.174 m`。
- 左/右 acromion：`x = ±0.208 m`，肩峰间距 `0.416 m`。
- 锁骨内侧点：`x ≈ ±0.025 m`，`y ≈ 1.438 m`，`z ≈ +0.060 m`。
- 锁骨外侧点：acromion 前上缘，`x ≈ ±0.196 m`，`y ≈ 1.414 m`，`z ≈ +0.030 m`。
- 肩胸转折：胸廓外侧到 acromion 之间不能出现垂直平板或球体台阶。

### 5.2 侧面

必须测量：

- 胸骨/胸廓前最大 `z`：目标约 `+0.142 m`（中心前移加局部 relief）。
- 胸廓后最大 `z`：目标约 `-0.118 m`。
- 腰前 `z`：目标约 `+0.108 m`。
- 腰后 `z`：目标约 `-0.096 m`，形成 lumbar contour。
- 肩峰顶点 `y`：目标约 `1.400–1.415 m`。
-  Neck base `y`：保持 `1.468507 m` 作为接口尺度。
- 上臂前侧 biceps 与后侧 triceps 必须沿路径分别变化，不能只用一个 radius。

### 5.3 背面

必须测量：

- 肩胛内侧缘：`x ≈ ±0.055 m`，高度约 `1.32–1.39 m`。
- 肩胛外侧缘：靠近腋窝后侧，约 `x ≈ ±0.150 m`。
- 斜方肌过渡：从 neck base 两侧下降到 scapular/acromion，不形成水平帽檐。
- 胸椎后侧中心 `z`：目标约 `-0.120 m`。
- 腰部后侧 `z`：目标约 `-0.096 m`。
- 左右肩胛和斜方肌在对称定义下保持镜像；asymmetry 只能由显式参数驱动。

## 6. 肩带、腋窝与上臂根目标

### 6.1 肩带控制点

| 控制点 | 左 (+X) | 右 (-X) | 用途 |
|---|---|---|---|
| Sternoclavicular | (0.025, 1.438, 0.060) | (-0.025, 1.438, 0.060) | 锁骨内侧 |
| Clavicle mid | (0.105, 1.430, 0.050) | (-0.105, 1.430, 0.050) | 锁骨前上弧 |
| Acromion | (0.208, 1.415, 0.000) | (-0.208, 1.415, 0.000) | 肩峰/三角肌顶 |
| Axilla apex | (0.160, 1.285, 0.005) | (-0.160, 1.285, 0.005) | 腋窝顶 |
| Axilla posterior fold | (0.175, 1.265, -0.035) | (-0.175, 1.265, -0.035) | 腋窝后壁 |
| Upper-arm root center | (0.225, 1.330, 0.000) | (-0.225, 1.330, 0.000) | 上臂 sweep 起点 |

这些点必须进入 `ResolvedCharacterState.landmarks` 并直接控制曲面；仅保存数据不算实现。

### 6.1.1 肩部四类点语义

| 点 | 左 (+) | 右 (-) | 语义 | 不得混用为 |
|---|---|---|---|---|
| Acromion Surface Landmark | (0.208, 1.415, 0.000) | (-0.208, 1.415, 0.000) | 体表肩峰 | Shoulder Joint Center |
| Shoulder Joint Center | (0.241821, 1.302865, 0.000) | (-0.241821, 1.302865, 0.000) | R1 骨骼关节中心 | 表面三角肌顶点 |
| Upper-arm Root | (0.225, 1.330, 0.000) | (-0.225, 1.330, 0.000) | 几何 sweep 连接中心 | 最终外肩宽测量点 |
| Deltoid Surface | 最外侧候选 x≈+0.230, y≈1.350 | 最外侧候选 x≈-0.230, y≈1.350 | 软组织外轮廓 | 骨骼关节中心 |

R1 Shoulder Joint 与 R2 Acromion 的距离约 `117.1 mm`。该差值不是错误，但必须由肩带 patch 和权重过渡解释，不能直接把 Acromion 当作骨关节。

### 6.2 腋窝

腋窝由四个边界共同定义：

1. 胸壁内侧边界。
2. 上臂内侧边界。
3. 腋窝顶部/前壁。
4. 腋窝后壁与肩胛过渡。

目标凹深约 `10–14 mm`，支撑范围约 `70–90 mm`，不能使用当前单中心球形负向 basis。Bind Pose 和肩部基础动作都要保留凹面。

### 6.3 上臂根截面

建议 `t=0` 为肩峰下方 root，`t=1` 为 elbow interface：

| t | 半宽 (m) | 半深 (m) | 前侧偏移 (m) | 后侧偏移 (m) | 形态 |
|---:|---:|---:|---:|---:|---|
| 0.00 | 0.058 | 0.052 | 0.000 | 0.000 | deltoid insertion |
| 0.10 | 0.066 | 0.058 | 0.006 | 0.002 | deltoid lateral/anterior |
| 0.25 | 0.060 | 0.054 | 0.010 | 0.000 | biceps anterior |
| 0.50 | 0.052 | 0.048 | 0.008 | 0.006 | biceps/triceps 分离 |
| 0.75 | 0.044 | 0.042 | 0.003 | 0.005 | brachialis/triceps |
| 1.00 | 0.038 | 0.036 | 0.000 | 0.000 | elbow transition |

三角肌前、中、后体积必须使用不同方向和支撑，不是统一放大 radius。

## 7. 形态基目标幅度

| 形态基 | 方向 | 候选幅度 | 支撑 | 连续性 |
|---|---|---:|---|---|
| Pectoral relief | 前/外侧 | 8–12 mm | 胸廓上部双侧 | C1 边界 |
| Ribcage relief | 侧/前 | 4–6 mm | costal margin 到 upper rib | C1 |
| Abdominal contour | 前 | 3–5 mm | umbilicus 到下肋 | C1 |
| Oblique relief | 前外侧 | 2–4 mm | 腰侧腹 | C1 |
| Thoracic back contour | 后 | 5–8 mm | 上胸/肩胛间 | C1 |
| Scapular relief | 后/外侧 | 3–5 mm | 左右肩胛 | C1 |
| Trapezius transition | 上/后 | 4–6 mm | neck base 到 acromion | C1 |
| Clavicle relief | 前/上 | 3–4 mm | 锁骨曲线 | C1 |
| Deltoid anterior | 前/外侧 | 8–12 mm | root 前侧 | C1 |
| Deltoid lateral | 外侧 | 12–16 mm | root 外侧 | C1 |
| Deltoid posterior | 后/外侧 | 10–14 mm | root 后侧 | C1 |
| Axilla concavity | 内侧/后 | 10–14 mm | 腋窝四边界 | C0/C1 内部连续 |

## 8. 测量与验收方法

R2 的每个目标都必须由 mesh 测量，不以参数名或截图审美替代：

- 按 `BoundaryId/RegionId/StationId` 提取局部顶点。
- 记录采样点数、max/mean/p95、左右差异和 min/max。
- 参数扰动测试必须同时给出目标区域最大位移和非目标区域最大位移。
- 相同 `CharacterDefinition` 下 R1/R2 使用同一相机、灯光、设备、分辨率和姿态。
- 至少八视角、Shoulder/Chest/Axilla 近景、Clay/Normal/Weight。
- 三角形数量只作为成本记录，不作为形态改善证据。

## 9. 非目标与授权影响

- 不导入外部人体网格、morph、blendshape、训练数据或模型资产。
- 不生成完整头、手、足、腿或 mode-3 Canonical。
- 不在 R2 自动加入新骨骼、JNI、UBO 或 vertex packing。
- 若需要 Neck Base/Trapezius 的最小接口几何，必须在 R2 授权中单独勾选；基础方案只允许显式 `neck-base-interface` crop loop，不生成头颈动画。
- 本文件中的目标比例在负责人确认前不得写入生产默认值。
