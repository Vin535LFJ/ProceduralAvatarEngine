# 成熟参数化人体模型研究报告

文件：`docs/research/parametric-human-modeling/01-reference-project-analysis.md`
任务：P1-VR / Deliverable R1
日期：2026-11-25
状态：只读源码/论文研究；未运行外部项目，未导入模型或数据。

## 1. 结论摘要

这些项目不是同一类“程序化人体生成器”：

- MakeHuman / MPFB2：固定 hm08 基础网格 + 语义参数 + 预授权顶点 delta。
- SMPL-X / FLAME / MANO / GHUM：固定拓扑 + 学习到的 shape/pose basis + joint predictor/regressor + LBS。
- MB-Lab：Blender 数据库 + 固定模板 + JSON morph + 配置驱动生成流程。
- OpenSim：骨骼/关节/标志点/测量语义，不是人体皮肤生成模型。
- GNM Head：固定统计头部拓扑 + identity/expression/pose 参数 + 眼球/牙齿/舌头内部结构。

PAE 应吸收的是：参数分层、固定自有拓扑、局部形态基、shape/pose 分离、区域 mask、关节中心与解剖标志点语义。不能直接搬用模型、网格、morph 或受限训练数据。

## 2. 核查版本

| 项目 | 核查版本/Commit |
|---|---|
| MakeHuman | `a8bc2d54ff0ac92e78ff71431b1023eda42bf482` |
| MPFB2 | `650c5c53bd6989a8ac264f31fa1d11d4a0c7bd8a` |
| SMPL-X code | `1265df7ba545e8b00f72e7c557c766e15c71632f` |
| FLAME | 论文 SIGGRAPH Asia 2017；`TimoBolkart/FLAME` 仓库不可用，改查 `TF_FLAME d4dab55213a93851f09689016be32d7fa10a6ce1` 与官方许可 |
| MANO | v1.2 / SMPL-X `MANO` 实现 |
| GHUM/GHUML | release v1.0；repo HEAD `20fadc5c985631044ccc047815cbd0300918e690`，`ghum/` 最后实质更新 `7d1794c26097387d7ae664295d19532833fa1c92` |
| MB-Lab | 1.8.1 `063bff04e60f3e7c651fda628c30f5d83f3f3078` |
| OpenSim core | `c594ccb9c789ac1451ce4961ecd563b6712b2d93` |
| OpenSim models | `d9b05d470b1a481c222372c85b75772faf8f7792` |
| GNM Head | repo HEAD `25aafd3bf310c2a50eb0347602a023af2eb09ae4`；arXiv `2607.23687v3` |

## 3. MakeHuman / MPFB2

### 3.1 实际数据流

MakeHuman：

1. macro 参数写入 human macro variables；
2. modifier/target 按分类和 macrovar 计算权重；
3. 每次应用从 seed 坐标重置；
4. 所有 target delta 按权重累加。

关键源码：

- `makehuman/apps/humanmodifier.py:389-438,547-610,644-652`
- `makehuman/lib/targets.py:47-67,188-227`
- `makehuman/apps/human.py:474-888,1147-1165`

MPFB2：

1. `HumanObjectProperties` 从 JSON 读取 phenotype 参数；
2. `macro.json` 定义分段线性 macro 权重；
3. `TargetService` 加载 `.target`/`.target.gz`；
4. 多条 target 的权重按 gender/age/muscle/weight 等维度相乘；
5. delta 写入 Blender Basis shape key。

关键源码：

- `src/mpfb/entities/objectproperties/__init__.py:24-28`
- `src/mpfb/services/targetservice.py:90-96,422-445,480-544,866-918,989-1203,1253-1302`
- `src/mpfb/data/targets/macrodetails/macro.json`
- `src/mpfb/data/targets/target.json`

### 3.2 `.target` 与 `macro.json` / `target.json`

- `.target` 是文本记录：`vertex_index x y z` 偏移；MakeHuman 可编译为 `targets.npz`，索引 `uint16`，向量量化 `int16 * 1e-3`。
- MPFB2 读取时使用 `index x z -y` 坐标转换。
- `macro.json` 包含 macro 的分段线性控制信息；实际组合逻辑仍有硬编码。
- `target.json` 是 UI/区域/左右分类元数据，不是几何本身；含 23 个 region section、234 个 category，以及 `has_left_and_right` / `opposites`。

### 3.3 固定网格依赖

- 两仓库 `base.obj` 字节相同，SHA-256 `8e761e6624b8f54536409135d1636da63b32486a90d4897f84e121d144f6fb4c`。
- hm08：19,158 vertices、18,486 quad faces。
- target 使用绝对 vertex index，拓扑必须不变。
- MPFB2 使用固定 vertex-group 范围、固定 `hm08.mirror` 表和 helper geometry。
- 这不是“完全程序化生成人体解剖”，而是“固定基础网格 + 参数化 delta”。

### 3.4 PAE 可借鉴 / 不可借鉴

可借鉴：

- 语义 macro 参数与局部 target 权重分离；
- 分段线性 basis weight；
- 区域/左右独立控制；
- `base + Σ w_i d_i` 的数学组织；
- topology version 和稳定 vertex ID。

不可直接借鉴：

- hm08 网格、target delta、mirror table、vertex-group 范围；
- 将 target index 映射到 PAE 自主 Canonical；
- 把外部 delta 当作“程序化生成”证据。

## 4. SMPL-X

### 4.1 数学表达

论文 §3.1：

\[
T_p=V_0+B_S(\beta)+B_E(\psi)+B_P(\theta)
\]

\[
J(\beta)=J_{regressor}(V_0+B_S(\beta))
\]

\[
M(\beta,\theta,\psi)=W(T_p,J(\beta),\theta,W)
\]

实现顺序：

```text
Vshaped = V0 + Bshape(β) + Bexpression(ψ)
Jrest   = Jregressor · Vshaped
Vposed  = Vshaped + Bpose(θ)
Vfinal  = LBS(Vposed, Jrest, θ, W)
```

Pose corrective 使用 rotation matrix difference：

\[
B_P(\theta)=\sum_n (R_n(\theta)-R_n(\theta_{rest}))P_n
\]

关键源码：`smplx/lbs.py` 的 `blend_shapes`、`vertices2joints`、`batch_rodrigues`、`batch_rigid_transform`、`lbs`；`smplx/body_models.py` 的 `SMPLX`。

### 4.2 依赖

- 固定 10,475-vertex 拓扑；
- 54 joints，包括 body 21、每手 15、jaw/eyes；
- 300 shape directions、最多 100 expression directions；
- 学习到的 joint regressor、skin weights、pose blendshapes；
- 训练数据包括 3,800 A-pose identities、1,786 diverse poses，以及 MANO/FLAME 参数。

### 4.3 PAE 映射

可独立实现：

- shape 与 pose 分开；
- joint center 随 shape 变化时重新从 landmarks 解析；
- 局部区域 shape basis；
- 用现有 22 骨骼做 `LBS22(V0 + ΔVshape + ΔVpose)`。

不可做：

- 导入 SMPL-X 网格、shape blendshapes 或 54-joint hierarchy；
- 把 SMPL-X 顶点 delta 映射到不同 PAE 拓扑。

## 5. FLAME

### 5.1 数学与结构

\[
T_p=V_0+B_S(\beta)+B_P(\theta)+B_E(\psi)
\]

\[
J(\beta)=J_{regressor}(V_0+B_S(\beta))
\]

- 固定 5,023-vertex 拓扑；
- neck、jaw、left/right eyeball 四个 articulated joints，加 global rotation；
- shape/expression 为线性 PCA/blendshape basis；
- pose corrective 为 rotation-matrix-difference basis。

### 5.2 眼睑与眼睛

原始 FLAME 没有独立 eyelid joints。论文明确加入 eyeballs，避免 eyelid 解释 eyeball noise。眼睑运动来自局部 expression/pose-corrective 位移和 eye-region skinning，而不是新增 eyelid bone。2023 官方站报告 eye region 修订，但细节需登录，未核实。

### 5.3 PAE 映射

- 使用 PAE 自有固定面部模板；
- 眉弓、眼眶、颧骨、鼻梁、鼻翼、唇、下颌分区；
- eyeball/iris 辅助几何与 Canonical eyelid skin 分离；
- jaw 使用现有 Neck/Head 权重和局部 corrective 入口；
- 不导入 FLAME 网格、expression basis 或纹理。

## 6. MANO

### 6.1 数学与结构

- 固定 SMPL hand topology；
- 15 articulated hand joints + global orientation；
- shape 为 neutral-pose registrations 的 PCA；
- joints 由 sparse linear regression 得到；
- pose corrective 使用 geodesic-distance regularization，保持局部；
- hand pose embedding 使用 axis-angle PCA；6 个 component 捕获约 81% 训练方差。

### 6.2 PAE 映射

可借鉴：

- 固定手部模板；
- palm/digit 局部拓扑；
- shape/pose 分离；
- geodesic/局部支撑 corrective；
- 手指表面形变与骨骼层级分离的思想。

PAE 限制：

- 不导入 MANO 网格或 15-joint hand hierarchy；
- 不新增手指骨骼；
- VR-3 只构建静态/整体 Hand 权重的手指形态，测试腕/手旋转；
- 独立手指关节运动需要后续拓扑/形变授权。

## 7. GHUM / GHUML

### 7.1 数学与结构

\[
X=LBS(X_{shape}(\beta_b)+\Delta X_{pose}(\theta)+\Delta X_{face}(\beta_f), C(X_{shape}), \theta, W)
\]

- body shape 与 face expression 使用非线性 VAE；
- joint centers 由 shape-dependent local regression 得到；
- pose deformation 使用 rotation difference + shared ReLU + local regressors；
- region feature mask 限制 head/body/left-hand/right-hand 越界影响；
- skin weights 非负、归一化、每顶点最多 4 个；
- 固定 artist-designed quad topology，10,168 / 3,194 vertices，63 joints。

### 7.2 PAE 映射

可借鉴：

- 区域 mask 和 locality loss 的思想；
- joint center 随 shape 更新；
- 非线性局部耦合，而不是一个全局标量缩放全身；
- Top-4 非负权重约束。

不可借鉴：

- 63-joint hierarchy；
- 学习模型、VAE 数据或固定 GHUM 网格。

## 8. MB-Lab

### 8.1 工作流

- `characters_config.json` 将 template、morph、measurement、anthropometry、joints、vertex groups 分开；
- `HumanCategory` / `HumanModifier` 管理参数；
- face 分为 Eye、Eyelid、Nose、Mouth、Ear、Head、Forehead、Cheek、Jaw；
- body 分为 Chest、Torso、Pelvis、Arms、Hands、Legs、Feet；
- `MorphingEngine.calculate_morph()` 逐 vertex index 应用 delta；
- finalize 后转换为 shape keys，MB-Lab 编辑信息被移除。

### 8.2 固定依赖

- human female 18,210 vertices / 17,288 faces；
- human male 17,996 / 17,074；
- morph JSON 为 `[vertex_index, dx, dy, dz]`；
- parser 检查顶点数相同，但顶点数相同不代表 topology/order/UV 相同。

### 8.3 PAE 映射

- 配置与生成分离；
- 参数按 face/body region 组织；
- 生成结果与编辑状态分离；
- 不引入 `.blend`、数据库或 AGPL morph 数据。

## 9. OpenSim

### 9.1 可复用概念

- `Marker`：命名 `Station`，附着于 `PhysicalFrame`；
- `StationDefinedFrame`：三个非共线 station + origin 定义 landmark-driven frame；
- `Joint`：parent/child frame + joint center + generalized coordinates；
- `Body`：frame + mass/COM/inertia，可见 mesh 可选；
- `ModelScaler`：marker-pair 距离产生 segment scale factor；
- `Coordinate`：rotational/translational/coupled type、range、locked/clamped。

### 9.2 PAE 映射

- `AnatomicalLandmarks`：命名 surface station、measured/virtual、frame attachment；
- `JointDeformationRegions`：joint center、axes、coordinate range、coupled transform；
- anthropometry：marker-pair/segment 测量作为拟合约束；
- 不导入 OpenSim skeleton、model 或 multibody tree；
- OpenSim 不提供 skin weights 或表面 falloff，PAE 必须自行定义。

## 10. GNM Head（专家补充参考）

核查版本：`google/GNM` HEAD `25aafd3bf310c2a50eb0347602a023af2eb09ae4`；技术报告 arXiv `2607.23687v3`（2026-09-11）。

已核实：

- 模型覆盖 head、face、neck、eyeballs、teeth、tongue。
- v3.x identity 253 维：head 170、eyeball 3、teeth 80。
- expression 维度存在来源差异：官方 README 为 383（100+100+150+32+1，tongue/iris）；arXiv v3 §3.9 Table 1 为 382（100+100+150+31+1，最后一项称 pupil）。未裁定前同时记录，不把 383 称为两来源共同核实值。
- head pose/eyeball orientation 使用 joint axis-angle + global translation。
- model data 包含 template shape、identity basis、expression basis、skinning weights、UV layout。
- 同时提供 quad 与 triangulated topology；UDIM 1001–1007 按 skin/eye/teeth/tongue 分区。
- 官方明确训练数据使用二元性别分类和四个宽泛人口群组，代表性有限。

PAE 用法：VR-2 的面部拓扑、内部结构和参数分区参考；不导入网格、basis、weights、textures、semantic sampler 或训练数据，不推迟 VR-1A。

许可证：仓库整体 Apache-2.0；LICENSE 另有 tongue animation mesh 的 MIT 第三方条款；模型/训练数据/第三方资产仍须逐项核查。

## 11. 五类技术归纳

| 类别 | 成熟项目做法 | PAE 自主实现 |
|---|---|---|
| A 人体测量 | MakeHuman macro、OpenSim marker-pair scaling、SMPL shape β | `ProportionProfile` + `Anthropometry` |
| B 解剖标志点 | OpenSim stations/frames、SMPL joint regressor | `AnatomicalLandmarks` + `BodyJoints` 解析 |
| C 局部形态 | MakeHuman target、SMPL/FLAME/MANO shape basis | 区域参数 + 解析 local shape basis |
| D 几何拓扑 | 固定 hm08/SMPL/FLAME/MANO/GHUM/MB-Lab 模板 | PAE 自有 `TopologyTemplate` + `SurfacePatch` |
| E 姿势形变 | pose blendshape、joint hierarchy、LBS、region mask | 现有 22-bone LBS + `JointDeformationRegions`，后续局部 corrective |

## 12. 对 PAE 架构的最终修正

1. 保留负责人提出的 `P(s,φ)` 可变截面作为躯干/四肢主体。
2. 为每个区域建立 PAE 自有固定拓扑模板，获得稳定 ID 和局部形态基。
3. 面部、手部、足部使用独立结构化模板，不套用全身截面公式。
4. SDF 只做局部融合/凹面，不做全身最终形态。
5. shape 与 pose 分开；VR-1 不急于实现学习模型或完整 corrective。
6. 用区域 mask 和参数影响测试替代“最近骨骼距离”单一权重逻辑。
7. 不导入任何外部模型、morph、训练数据或受限资产。

## 13. 来源

### MakeHuman / MPFB2

- [humanmodifier.py](https://github.com/makehumancommunity/makehuman/blob/a8bc2d54ff0ac92e78ff71431b1023eda42bf482/makehuman/apps/humanmodifier.py#L389-L652)
- [targets.py](https://github.com/makehumancommunity/makehuman/blob/a8bc2d54ff0ac92e78ff71431b1023eda42bf482/makehuman/lib/targets.py#L47-L227)
- [human.py](https://github.com/makehumancommunity/makehuman/blob/a8bc2d54ff0ac92e78ff71431b1023eda42bf482/makehuman/apps/human.py#L474-L888)
- [algos3d.py](https://github.com/makehumancommunity/makehuman/blob/a8bc2d54ff0ac92e78ff71431b1023eda42bf482/makehuman/core/algos3d.py#L126-L239)
- [MPFB2 TargetService](https://github.com/makehumancommunity/mpfb2/blob/650c5c53bd6989a8ac264f31fa1d11d4a0c7bd8a/src/mpfb/services/targetservice.py#L422-L918)
- [macro.json](https://github.com/makehumancommunity/mpfb2/blob/650c5c53bd6989a8ac264f31fa1d11d4a0c7bd8a/src/mpfb/data/targets/macrodetails/macro.json)
- [target.json](https://github.com/makehumancommunity/mpfb2/blob/650c5c53bd6989a8ac264f31fa1d11d4a0c7bd8a/src/mpfb/data/targets/target.json)
- [MakeHuman base.obj](https://github.com/makehumancommunity/makehuman/blob/a8bc2d54ff0ac92e78ff71431b1023eda42bf482/makehuman/data/3dobjs/base.obj)

### SMPL-X / FLAME / MANO / GHUM

- [SMPL-X paper §3.1](https://arxiv.org/html/1904.05866#S3.SS1)
- [SMPL-X lbs.py](https://github.com/vchoutas/smplx/blob/1265df7ba545e8b00f72e7c557c766e15c71632f/smplx/lbs.py)
- [SMPL-X body_models.py](https://github.com/vchoutas/smplx/blob/1265df7ba545e8b00f72e7c557c766e15c71632f/smplx/body_models.py)
- [FLAME paper](https://download.is.tue-mpg.de/flame/flame_paper.pdf)
- [FLAME official](https://flame.is.tuebingen.mpg.de/)
- [TF_FLAME batch_smpl.py](https://github.com/TimoBolkart/TF_FLAME/blob/d4dab55213a93851f09689016be32d7fa10a6ce1/tf_smpl/batch_smpl.py)
- [MANO paper §3](https://ar5iv.labs.arxiv.org/html/2201.02610#S3)
- [MANO official](https://mano.is.tue.mpg.de/)
- [GHUM paper](https://openaccess.thecvf.com/content_CVPR_2020/papers/Xu_GHUM__GHUML_Generative_3D_Human_Shape_and_Articulated_Pose_CVPR_2020_paper.pdf)
- [GHUM README](https://github.com/google-research/google-research/blob/20fadc5c985631044ccc047815cbd0300918e690/ghum/README.md)

### GNM Head

- [GNM repository](https://github.com/google/GNM)
- [GNM Head shape README](https://github.com/google/GNM/blob/main/gnm/shape/README.md)
- [GNM Head technical report](https://arxiv.org/abs/2607.23687)
- [GNM LICENSE](https://github.com/google/GNM/blob/main/LICENSE)

### MB-Lab / OpenSim

- [MB-Lab characters_config.json](https://github.com/animate1978/MB-Lab/blob/063bff04e60f3e7c651fda628c30f5d83f3f3078/data/characters_config.json)
- [MB-Lab humanoid.py](https://github.com/animate1978/MB-Lab/blob/063bff04e60f3e7c651fda628c30f5d83f3f3078/humanoid.py)
- [MB-Lab morphengine.py](https://github.com/animate1978/MB-Lab/blob/063bff04e60f3e7c651fda628c30f5d83f3f3078/morphengine.py)
- [OpenSim Marker.h](https://github.com/opensim-org/opensim-core/blob/c594ccb9c789ac1451ce4961ecd563b6712b2d93/OpenSim/Simulation/Model/Marker.h)
- [OpenSim StationDefinedFrame.h](https://github.com/opensim-org/opensim-core/blob/c594ccb9c789ac1451ce4961ecd563b6712b2d93/OpenSim/Simulation/Model/StationDefinedFrame.h)
- [OpenSim ModelScaler.h](https://github.com/opensim-org/opensim-core/blob/c594ccb9c789ac1451ce4961ecd563b6712b2d93/OpenSim/Tools/ModelScaler.h)
