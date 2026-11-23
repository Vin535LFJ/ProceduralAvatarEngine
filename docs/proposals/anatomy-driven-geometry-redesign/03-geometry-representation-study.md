# 03 — Geometry Representation Study

## 1. 评估方法与边界

所有候选技术均先按同一 `RepresentationNeutralShape` 评估；先决定如何描述人体，再决定该区域用哪种曲面/场实现。评分为设计阶段的相对工程判断，不是设备实测或 PoC 结果。

评分：1=明显不适合，2=局限较大，3=可在受控范围使用，4=适合，5=高度适合。Android 评分只代表架构上的预估适配性，必须真机验证。

## 2. 参数化曲面家族

### 2.1 Bezier Surface

- 通过张量积 Bernstein 基函数和控制点构成单片曲面；控制点局部性较弱，编辑单一控制点会影响整个片面。
- 优点是公式简单、实现可控、适合定义 landmark 约束区域和边界曲线。
- 缺点是多片拼接需要显式匹配边界及导数；Bezier degree 高时控制复杂，肩/腋窝高拓扑复杂区域通常需要拆片，容易回到接缝工程。
- 适合用于小型受控片面、轮廓曲线和模板拟合，不建议把全身拆成大批 Bezier patch。

### 2.2 B-Spline Surface

- 多节点区间与局部基函数支持局部控制，比单片 Bezier 更适合连续控制网格和参数曲面。
- 可以用 cubic B-spline 表示躯干主曲面、截面曲线和肩带过渡控制面；节点向量/参数域需稳定且可控。
- 多个独立 B-spline patch 仍需要边界位置、切线和参数化方向约束；C0/G1 不是自动获得。
- 相较 NURBS，基础实现更轻，适合作为 C++17 自研核心的首选参数化曲面基础。

### 2.3 NURBS Surface

- 在 B-spline 基础上增加权重，能够精确表示圆锥曲线等几何，但需维护有理参数、权重、节点向量和 trim 边界。
- 对人体软组织而言，精确圆弧表示通常不是核心收益；形状控制和边界管理比“精确圆”更重要。
- Android runtime 可以用 CPU 评价 NURBS，但完整 trim/tessellation 与 seam 约束实现复杂；当期没有证据证明收益足以抵销工程成本。
- 候选定位：不作为第一期运行时核心；未来如要导入精确设计曲面/参数资产再评估。

### 2.4 Coons Patch

- 通过四条边界曲线构造插值曲面；适合连接两组已存在边界。
- 仅能保证边界位置插值，内部切线/曲率质量依赖边界曲线和张力项；它不会自动知道肩部解剖体积。
- 可作为局部补面或参数化区域过渡工具，但不应再把“Coons bridge 已生成”当作人体肩部已成功融合。

### 2.5 Subdivision Surface

- Catmull-Clark 从控制网格反复细分，形成平滑极限曲面，可适配一般四边形拓扑。
- 优点是拓扑由控制网格决定，细分曲面平滑、可用低分辨率控制网格形成高质量表面；适合最终动画网格和细节层级。
- 缺点是 extraordinary vertices、边界规则、crease/semi-sharp crease 和面片流向需要良好拓扑；细分不会自动修复糟糕的肩腋 Edge Flow。
- 对 GLES3，不建议每帧在 GPU 上实现复杂 Catmull-Clark。可在 CPU 生成/预细分一次，或保留控制网格并在离线研究工具中检查极限表面。
- 参考 Pixar 对 subdivision surfaces 的动画用途说明：可表示任意拓扑且极限表面平滑，但拓扑与动画控制仍需设计。[Subdivision Surfaces in Character Animation](https://graphics.pixar.com/library/Geri/paper.pdf)

## 3. Implicit / SDF 家族

### 3.1 多区域隐式场

将不同解剖区域建模为标量场贡献，使用 union / smooth union / subtraction / displacement 组成最终 skin envelope。优势是各个局部体积可自然重叠并由同一个等值面产生，不需要在最终表面拼接 patch。

限制：

- 隐式场只保证表示上的融合，不会自动创造正确肌肉形态；场中只有椭球/胶囊时，最终只是更圆润的一组椭球/胶囊。
- Smooth Union 半径影响体块分离、腋窝凹面、胸腹转折；参数值本身必须与人体尺度和区域语义关联，不能全局一个 smoothness multiplier。
- 普通 SDF 在非均匀缩放、复杂距离变形后可能不再是真正距离场；需要区分 signed field 与 exact distance，不能对 field value 盲目当作毫米距离。
- 多个正体积 union 特别容易把肩胛和腋窝凹面填平；应显式约束外包络和局部负体积/凹面区域。

### 3.2 各向异性距离场与肌肉体积场

建议使用方向明确的局部坐标框架，让肌肉截面分别定义 longitudinal、thickness、width 三个方向，并基于 origin/insertion/belly curve 产生体积。肌肉 feature 可以是 implicit muscle/fan，但应参与同一 skin envelope，而不是独立渲染成多个肌肉块。

`sdMuscleFan()` 的研究方向是有价值的：肌腹位置、起止端半径、section scale、roll、groove 可以描述肌肉纤维方向。但 feature branch 中该代码尚未进入最终 `makeUpperBodyField()` 组合，属于未验证的形状构件。

### 3.3 Surface Nets

当前 M12 使用 Surface Nets：每个跨越 cell 生成一个顶点，边跨越处生成 quad 并三角化。特点是实现相对简单，输出通常比经典 marching tetrahedra 更规整，适合生成连续场表面。

局限：

- 当前实现的 cell vertex 是多条 edge crossing 的平均，不是 QEF 最小化；尖锐/狭窄/曲率变化大的区域可能被平均抹平。
- 网格密度受采样分辨率约束；局部加密需 adaptive/octree 或局部细分支持。
- 生成拓扑不是角色动画 Edge Flow；quad 来源是体素 lattice edge，不能直接等同于围绕肩/肘关节的解剖环线。
- 如果特征尺寸小于体素步长，可能丢失或产生不稳定拓扑。

### 3.4 Marching Cubes

优点：算法成熟、实现资料多、可直接控制体素密度；适合快速生成预览网格。缺点：传统 cell tri-table 产生大量三角形、尖锐特征受分辨率影响，拓扑 case/ambiguity 需要正确处理；输出同样不是动画拓扑。对项目而言，已有 Surface Nets 资产更值得先校验与扩展，而非立即替换。

### 3.5 Dual Contouring

Dual Contouring 使用 Hermite edge intersection + normals 和每 cell 的 QEF 近似求解顶点位置，可比 crossing-average 更好保留棱线和细节；适合肩峰、髂嵴、肘骨轮廓等需要保持局部特征的隐式提取研究。

局限：QEF 的数值稳定、单 cell 多特征、octree 自适应、拓扑保证和 crack handling 都更复杂。论文提出 Hermite 数据、稳定 QEF 和拓扑保持测试，但不能推导为所有场/配置自动无缺陷。[Ju et al., Dual Contouring of Hermite Data (2002)](https://doi.org/10.1145/566654.566586)

建议保留为第二轮比较候选，不作第一轮核心实现。

## 4. 骨架驱动变截面曲面 / Sweep

### 4.1 能力边界

Sweep 用中心曲线、局部 frame 和截面 profile 构建管状区域。只要中心曲线和 frame 稳定，适合表达具有主轴方向、截面沿轴向变化的体段。

适合：

- 上臂：肱骨长轴，配合前/后肌肉体积场；
- 前臂：肘到腕主轴，前屈肌/伸肌体积可以在周向不对称；
- 大腿/小腿：骨骼中心线与主肌腹变化；
- 颈部部分区域。

不适合单独承担：胸腔、腹部、骨盆、肩胛/腋窝与肘关节本体。它们没有一个简单中心线可以代表全部三维轮廓。

### 4.2 Parallel Transport Frame 风险

PTF 可减少 Frenet frame 在曲率接近零时的不稳定翻转，但在以下情况必须明确处理：

- 相邻点重合/切线退化；
- 前后切线接近 180° 反向；
- 初始参考向量与切线近平行；
- 关节转动导致截面局部方向需要跟随骨骼 twist，而不只是随 path tangent；
- 闭合曲线的 frame 首尾扭转不能无缝对齐。

生成前应清理重复采样点，做 quaternion shortest-arc 反向特例，设置稳定 fallback axis，并对 frame 的正交性、左右手性和逐段 roll delta 做验证。现有 `computeParallelTransportFrames()` 有投影和重正交化，但对反向切线没有显式分支；P1-A 自有 `computeFrames()` 的 `cross().normalized()` 也应覆盖退化情况。

### 4.3 截面不是万能解剖模型

非圆 Superellipse profile 比圆管更丰富，但仍然是沿中心线扫出的同一截面族。若要表达肱二头肌相对肱三头肌的独立前后体积，应让 shape model 描述 muscle compartments，不能继续为每个截面增加一堆周向参数。

## 5. 混合几何方案比较

这里的 Hybrid 不是“躯干用 SDF、手臂用 sweep，各自生成 mesh 后再焊接”。这种组合会复刻 M11/M12/P1-A 的 seam 风险。

推荐的 Hybrid 含义：

1. 统一 Anatomy Shape Model 产生所有区域的 landmark、volume、curve 和语义边界；
2. 不同区域可使用不同局部数学表达（B-spline envelope、oriented muscle implicit volume、variable profile sweep、joint deformation field）；
3. 所有形态贡献最终进入**一个共享 skin-envelope 评价器/边界约束器**，在区域交界处只产生一个最终皮肤表面；
4. 连续高密度 surface 用于形态目标与距离/法线参考；动画网格由固定拓扑模板投影/拟合，而不是把 extraction mesh 直接作为角色最终拓扑。

如果局部 patch 作为表面网格输出，必须定义显式 seam contract、共享边界控制点、边界导数约束和同一拓扑模板。否则不使用局部表面拼接。

## 6. 量化比较矩阵（设计评分，不是实测）

评分范围 1–5。表格是针对本项目所需的人体与动画任务，不是技术一般优劣榜。

| 方案 | 形态表达 | 正/侧/45°轮廓 | 连续性 | 参数语义 | 拓扑控制 | 局部编辑 | 蒙皮兼容 | 体型变化 | 极端稳定性 | Android适配 | C++复杂度（易=5） | 后续扩展 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 单纯 Bezier patch | 3 | 3 | 2 | 3 | 4 | 3 | 4 | 3 | 3 | 4 | 3 | 3 |
| B-spline 控制曲面 | 4 | 4 | 3 | 4 | 4 | 4 | 4 | 4 | 3 | 4 | 3 | 4 |
| NURBS | 4 | 4 | 3 | 3 | 4 | 4 | 4 | 4 | 3 | 3 | 2 | 4 |
| Coons patch | 2 | 3 | 2 | 2 | 3 | 3 | 3 | 3 | 3 | 5 | 4 | 2 |
| Catmull-Clark SubD | 4 | 4 | 4 | 3 | 5 | 4 | 5 | 4 | 4 | 3 | 3 | 5 |
| Primitive SDF union | 3 | 3 | 5 | 2 | 1 | 4 | 3 | 5 | 3 | 4 | 4 | 4 |
| 解剖 feature implicit field | 5 | 5 | 5 | 4 | 1 | 4 | 4 | 4 | 3 | 4 | 2 | 5 |
| Surface Nets extraction | 3 | 3 | 5 | 1 | 1 | 2 | 2 | 4 | 3 | 4 | 4 | 3 |
| Marching Cubes extraction | 3 | 3 | 4 | 1 | 1 | 2 | 2 | 4 | 3 | 4 | 4 | 3 |
| Dual Contouring | 4 | 4 | 4 | 2 | 2 | 3 | 2 | 4 | 2 | 3 | 2 | 4 |
| Variable-profile Sweep | 3 | 4 | 3 | 4 | 4 | 4 | 4 | 4 | 3 | 5 | 4 | 4 |
| **共享 Shape Model + Hybrid 表面 + 固定拓扑投影** | **5** | **5** | **5** | **5** | **4** | **4** | **5** | **5** | **4** | **4** | **2** | **5** |

### 6.1 评分解释与未验证项

- Bezier/B-spline/NURBS 的连续性分数不表示曲面天然无缝。多个 patch 的 C0/G1 需通过边界约束实现；B-spline 提升的是局部控制与连续曲面设计效率。
- SubD 拓扑控制高，但人体表面质量取决于控制 cage edge flow；SubD 不会把不正确的拓扑自动变成动画友好拓扑。Pixar/OpenSubdiv 建议规则四边形、少 extraordinary vertices、manifold topology 和清楚的 edge loops。[OpenSubdiv Modeling Tips](https://github.com/PixarAnimationStudios/OpenSubdiv/blob/dev/documentation/mod_notes.rst)
- Implicit field 连续性高，是因为最后评估为同一个 level set；其形态评分取决于 field feature 设计，不取决于 Smooth Union API 数量。
- Surface Nets/Marching Cubes 只有提取作用，不负责语义控制、Skinning 和拓扑流向，因此拓扑/蒙皮评分低。
- Dual Contouring 对特征保持有潜在收益，但对软组织曲面不是自动更优；需和当前 Surface Nets 用相同 shape field 比较。
- Hybrid 得分高意味着系统目标能分工，不意味着实现简单。其主要风险在统一 field / surface envelope 和 retopo 投影质量。
- 生成时间、内存占用、FPS 没有从评分推断。只有 M12 历史单设备数据，不能外推新路线性能。

## 7. 各身体区域推荐的表示

| 区域 | 首选表示 | 辅助表示 | 不建议作为最终路线 |
|---|---|---|---|
| 胸腔 | 由 landmark/轮廓曲线控制的 B-spline envelope + 解剖体积场 | Surface Nets/dual contouring 作为形态参考 | 五截面简单椭圆 loft；单椭球 |
| 腹部/腰部 | 参数化前后轮廓 + 共享 implicit volume field | 样条曲线控制肋弓/腰侧/背部 | 仅 X 宽度缩放；纵向圆柱收腰 |
| 骨盆 | 多 landmark 闭合包络 + 臀/髂骨体积特征 | 隐式体积场 | 单椭球和两腿根部重叠 |
| 肩/腋下 | 肩带共享隐式 envelope，必要时曲面 cage 辅助 | B-spline guide surfaces、局部 negative/concave field | 独立 shoulder patch + arm sweep bridge |
| 上臂 | oriented muscle implicit volumes + skeleton-guided variable sweep envelope | 固定拓扑 quad tube cage | 单圆管 sweep |
| 前臂 | 分屈肌/伸肌方向的 implicit masses + taper sweep envelope | 分区截面 cage | 宽深变化但肌群无语义的管状 Sweep |
| 肘 | joint-specific deformation envelope + topology rings | implicit joint volume / corrective shape | 单球、截面半径单点缩放 |

## 8. Android / C++17 / GLES3 可行性

- 几何生成在 CPU 端以 C++17 实现，不需要 GLES3 compute shader；GLES 3.0 主要承担 mesh draw、vertex skinning 和 buffer 生命周期。
- B-spline evaluation、隐式场 sampling、Surface Nets 与重拓扑投影可用纯 C++ 实现；算法必须按区域限制采样，避免全身高分辨率网格盲目膨胀。
- M12 24k triangles / 约 51.7 FPS 的单设备报告说明此量级在那台 Android 8.1 / GLES3 设备上可渲染，不等于新几何生成成本、所有手机/TV Box 都已验证。
- 生成可采用一次性异步 CPU job；关键是不会阻塞 RenderThread/UI，而不是假设字段求值能达到某个未经测量的毫秒数。
- CPU 端保留 `MeshData` 与 generation cache；GPU 上传只在 RenderThread 且 current EGL Context 执行。
- 不依赖 Unity/Unreal/Godot/Blender runtime；外部论文和离线研究工具仅用于算法参考，不作为运行时模型资产依赖。

## 9. 研究参考

- Ju et al., *Dual Contouring of Hermite Data*, SIGGRAPH 2002, [DOI 10.1145/566654.566586](https://doi.org/10.1145/566654.566586)。
- Huang et al., *QuadriFlow: A Scalable and Robust Method for Quadrangulation*, Computer Graphics Forum 2018, [DOI 10.1111/cgf.13498](https://doi.org/10.1111/cgf.13498)。QuadriFlow 说明场对齐自动四边形化可扩展、可减少奇异点，但自动 quad remesh 仍不等于角色关节拓扑。[论文页](https://stanford.edu/~jingweih/papers/quadriflow/)
- *Conversion of trimmed NURBS surfaces to Catmull-Clark subdivision surfaces*, CAGD 2014, [ScienceDirect](https://www.sciencedirect.com/science/article/pii/S0167839614000624)。示例说明边界约束、cross-field 分解和控制网格构造对于 G1 连续和 gap-free 转换是实质工作，不是自动效果。
- Pixar, *Subdivision Surfaces in Character Animation*, [PDF](https://graphics.pixar.com/library/Geri/paper.pdf)。

## 10. 阶段性选择建议

第一轮不要换掉 Surface Nets，也不要把 NURBS 当救命方案。建议：保留 M12 的 SDF/Surface Nets 作为连续形态参考路径；把 M13 风格的肌肉 feature fields 真正接入 Shape Model；增加 B-spline/landmark envelope 管理体块；再通过固定解剖拓扑模板生成动画 mesh。Dual Contouring、NURBS runtime 和全动态 SubD 暂缓，等 PoC 暴露确切问题再投资。
