# M11 — 03 技术设计

## 1. 几何核心（native/procedural）

### 1.1 CatmullRom.h（新增，header-only）
`sampleCatmullRom(points, samplesPerSegment, out)`：标准 Catmull-Rom（P1/P2 控制点端点重复），输出重采样中心线 + 弧长参数表；`sampleCatmullRomScalar(values, t)` 用于半径/截面参数的同步样条插值。确定性：纯函数无随机。

### 1.2 ParametricSurfaces 扩展
```cpp
struct SweepRing { Vec3 point; float radius; Vec2 sectionScale; float sectionRoll; }; // 轴/短轴缩放 + 截面旋转
MeshData generateSplineSweep(const std::vector<SweepRing>& rings,
                             int radialSegments, int splineSamplesPerSegment,
                             bool capStart, bool capEnd, bool closedRing);
```
- 中心线：Catmull-Rom 重采样（每段 splineSamplesPerSegment 环，自适应：按弦长密度）；
- 截面：椭圆（sectionScale.x/z 乘半径）+ roll 旋转角，仍用 PTF（修 `binormal` 回退）；
- 法线：径向分量 + 锥度倾斜（环间 dr/dl 的切向分量合成后归一化）；
- 端盖：扇形圆盘（法线=±切向）；closedRing=true 时首尾环融合（用于环形闭合如衣领）；
- UV：接缝复制列（S+1 顶点/环），u∈[0,1] 正确；
- winding 归一化：按首环切向与截面绕向断言朝外。
Loft 增强：`LoftSection` 增加 `roll`；截面样条插值（SectionProfile 复用 SweepRing 路径）；winding 按行进方向归一化；taper 法线同扫掠。旧 API `generateSweepSurface/generateLoftSurface` 保留为兼容包装（内部转调）。

### 1.3 SdfFusion（新增 fusion/SdfFusion.{h,cpp}）
- SDF 原语：`sdSphere/sdEllipsoid/sdCapsule(p, a, b, r)`；`sdSmoothUnion(a,b,k)`（指数或多项式 smooth-min）；
- `fuseLocal(volume 24³, sdf 回调, iso=0, 出 MeshData)`：局部 marching cubes（含标准 256 边表，内嵌生成或编译期表）；
- 顶点属性：法线=SDF 梯度（中心差分）；joints/weights 由调用方按位置回查（区域权重）。
- 用途限定：肩/髋/颈融合补丁（每关节局部体素域），不用于全身。

### 1.4 PoC（M11-B）：肩部+上臂、髋部+大腿三法对比
- A：现状（扫掠+关节球）；B：扫掠起点嵌入躯干（中心线上探 12% + 锥度融合）；C：SDF 融合补丁（躯干椭球 ∪ 肢体胶囊，smooth k 调参）；
- 度量：接缝法线偏差（相邻区域 30° 锥内采样点法线夹角均值）、三角形数、生成耗时（steady_clock）、蒙皮后视觉（设备截图）；
- 输出 ADR-008 定案。

## 2. 人体生成器（M11-C，Humanoid.cpp 重写）

`BodyParameters` 嵌套：
```cpp
struct TorsoShape { float chestWidth, chestDepth, chestTopCurve, chestBottomCurve, chestTilt, chestVolume; float waistWidth, waistPinch; };
struct PelvisShape { float hipWidth, hipDepth, waistHipBlend, pelvicTilt; };
struct ArmShape { float deltoidVolume, upperArmWidth, elbowShape, forearmCurve, wristPinch, upperArmLength, lowerArmLength; };
struct LegShape { float thighVolume, kneeShape, calfCurve, anklePinch, upperLegLength, lowerLegLength; };
struct NeckShape { float neckLength, neckRadius, neckTaper; };
```
每部位子生成器（独立函数 + 参数结构）：
- **躯干**：7+ 样条放掠（胸弧/腰收/臀形由上述参数插值控制点），胸腔前后不对称（背部平/胸凸），顶部肩线收窄；
- **肩**：三角肌体积 = 扫掠首环 sectionScale 放大 + （若选型 C）SDF 补丁；
- **臂**：SplineSweep 5 控制点（肩/肱骨中/肘/前臂中/腕），半径曲线 {deltoid, upper, elbow 抬升, forearm 纺锤, wrist 收缩}；
- **腿**：同理 5 点（髋/股中/膝/小腿纺锤/踝），小腿后弧（calfCurve 使 Z 向偏移）；
- **手/脚**：保留 M10 手掌+拇指（参数化），脚=圆角盒+趾收窄（脚背弧）；
- **颈**：SplineSweep（胸→颈→头，taper）。
兼容：`generateHumanoid(params, heatmap)` 签名不变；detailLevel 映射 splineSamples/radial。

## 3. 蒙皮（M11-D，WeightGenerator）

每个部位声明 `WeightRegion { primaryBones[], spanStart/End, falloff }`；生成算法：
1. 部位内顶点对 region 骨骼做胶囊距离（点到骨段距离，取代中心线单距离）；
2. 候选 2–4 骨 = 本 region 主骨 + 相邻 region 交界骨；
3. smoothstep 边界混合（跨 region 时对相邻骨权重做 C¹ 过渡）；
4. 归一化 + 合法索引校验；
5. 热力图模式保留。
测试：区域边界连续性（跨界顶点权重差 < 0.25 断言）、非法索引、运动回归。

## 4. 面部（M11-E，FaceGenerator）

- **头形**：球面参数化 (θ,φ) + 剖面调制：`r(θ,φ) = R * (1 + forehead(θ) + temple(φ) + cheek(φ,θ) + jawTaper(θ) + chin(θ,φ) + backHead(θ))`——各剖面为参数驱动的 smoothstep/高斯形状函数；正面/侧面均合理；输出解析法线（数值梯度）。
- **五官**：眼眶（头面位移凹陷）+ 上/下眼睑（部分球壳，可保留闭眼参数）+ 眼球（球）+ 虹膜/瞳孔（前凸盘）+ 高光；眉（发色扫掠弧）；鼻（梁=截面扫掠 + 鼻头椭球 + 鼻翼双球）；唇（上下两段截面弧，ink/唇色）；下巴（头形参数）；耳（C 形压扁椭球对）。
- 参数结构对应提示 §8.2 JSON 全字段，全部参与几何。

## 5. 头发（M11-E，HairGenerator）

- 层：头皮帽（头形偏移壳）+ 刘海（前额发束组）+ 侧发 + 后发 + 顶区发束；
- 发束 = SplineSweep（3-5 控制点，椭圆渐细截面 roll 对齐重力方向）；
- 预设 3 种：Short（帽+短刘海+鬓角）/ Medium（+侧发过耳）/ Long（+后发束披肩）；
- 参数：strandCount/width/bend/direction，全部参与几何。

## 6. 服装（M11-F，ClothingGenerator）

- 外壳 = 人体部位放掠/扫掠的复用（同中心线，radius+offset+offsetProfile），**独立网格**，权重复制底层顶点区域权重（同中心线采样）；
- T 恤：躯干壳（肩线到衣摆）+ 袖（上臂壳到袖口）+ 领口环（closedRing）；长裤：髋-腿壳到裤脚；
- 关节区松量（offset+ε）；褶皱 = 低频正弦径向调制（几何位移，非贴图）；
- 参数：厚度/长度/松量/领型；材质走 shirt/pants 新调色板槽位。

## 7. 材质与 Shader（M11-G）

- 片元着色器：materialId 驱动 `{baseColor, roughness, specular, fresnelScale}` 参数组（uniform 数组或常量分支），toon→smooth 混合系数按材质；rim 保留；High 档：廉价 hash 噪声细节（布料/皮肤微纹理）+ 屏幕空间无关 AO 近似（关节暗色顶点色烘焙）；
- 皮肤次表面近似：背光时 base 向红色偏移（低成本）；
- 调色板 4→6（skin/hair/shirt/pants/eye；ink 固定）；`setAppearance` 12→18 浮点（JNI 长度兼容检查）；
- 三档：Low（纯 toon）/ Medium（+specular+fresnel）/ High（+噪声+AO+SSS）。

## 8. 重定向修复（M11-H）

- **RetargetingState**（MotionRuntime 持有）：`lastTargets[22]`/`hasTargets[22]`/`lastConfidence`；有目标→更新并按 conf 缩放 slerp 步长；无目标→保持 lastTargets（不再恒等）；hasPerson=false 恢复渐退不变。
- 置信度：`w=0.4+0.6*conf*visibility` 用于滤波位置向 lastPositions 收缩（hold-predict）+ addPair 时按端点 visibility 加权方向可信度（<0.3 的方向不更新目标）。
- Hips：髋线（L/R hip 连线）的 roll + 脊柱方向的摆动分解；Neck/Head：鼻方向弱驱动（限制 0.3 rad）。
- 坐标协议：ProviderWorld 显式拒绝（返回失败）或文档化恒等；PixelImage z 除以 max(width,height)；镜像/旋转实证：TestImg 推 0°/90° 旋转与人像镜像图到设备，对照 ML Kit 输出坐标修正 Adapter；协议写入 module-contracts.md。
- 声明：2D 重定向 + 单目 Z 线索；Provider 接入 3D World Landmarks 留作评估项。

## 9. 视觉证据与性能（M11-I）

- `nativeSetCameraPreset(preset)` JNI + RenderThread 相机预设表（10 预设，固定 lookAt/projection/light）；MainActivity "Shot" 按钮循环；adb screencap 固定分辨率采集；
- 生成耗时打点（generateHumanoid 内 steady_clock，PAE_LOG 输出 ms）；
- Visual Quality Matrix 评分报告 + 改前/改后对比。

## 10. 兼容与回归红线

- 测试契约全保留（见 02 文档"测试契约"节）；旧 surface API 兼容包装；`generateHumanoid` 签名不变；JNI 变更全部向后兼容（长度检查）；`MotionRuntime` reset/外部帧语义不变（M8 测试锁定）。
