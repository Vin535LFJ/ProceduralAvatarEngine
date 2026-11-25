# P1-VR VR-0 Design Review Addendum

文件：`docs/phases/P1-visual-recovery/03-design-review-addendum.md`
任务：P1-VR / 专家意见整合
日期：2026-11-25
状态：有条件认可路线；VR-1A 前置修订；不构成生产授权或 Gate PASS。

## 1. 复核基线

- 分支：`poc-sm/shape-model-v2`。
- 材料提交：`8151e01d812b21ba2ad6c5cfde7a657df68ea3c6`。
- 默认分支 `main`：`c539afbec299ed4a066429ea09f9d3d36e67f74d`，仍为旧 M12-A，不与 P1-VR 材料混同。
- 本轮开始前工作树：`8151e01` 已提交材料 + 既存未跟踪 `.workbuddy-ai/`、v6/v7 evidence；本附录为新增未跟踪文档。
- 生产代码修改：无。
- VR-1：未开始。

## 2. 专家总体结论

专家有条件认可：

- 参数化人体语义；
- 可变截面曲面；
- 结构化局部拓扑；
- 统一 Canonical；
- mode-2 双侧躯干—肩部最小原型。

不推翻四阶段路线，不增加大规模研究阶段；通过一次小规模 VR-0 技术修订解决契约问题。

## 3. 八项意见映射

| # | 专家意见 | 处理 |
|---|---|---|
| 1 | `CharacterDefinition` 唯一权威，但输入与派生结果必须分层 | 采纳；新增不可变 `ResolvedCharacterState` 与 `GeometryBuildRequest` |
| 2 | Sweep + 局部形态基方向正确，但 frame、截面和叠加质量不足 | 采纳；补充 RMF、闭合周期截面、Jacobian/曲率/自交约束 |
| 3 | 面/手/足必须结构化，且 Canonical Skin Shell 边界规则不足 | 采纳；新增 Shell 组成与开放边界登记规则 |
| 4 | Stable ID 不足以处理 LOD 新顶点 | 采纳；新增 refinement lineage、parent face/edge/vertex、barycentric、LOD map |
| 5 | Top-4 合法不等于变形正确 | 采纳；权重合法性、基础动作、局部形变质量三层验证 |
| 6 | 同意 mode-2 双侧躯干—肩部原型 | 采纳；增加先对称后独立差异测试，保持 VR-1A/1B 分离 |
| 7 | C0/OQ-03/Weight/Cache/Visual 中 OQ-03 存在阻塞 | 采纳；5° 暂不作为 VR-1A 硬验收，保留冲突待负责人确认 |
| 8 | 停止条件需补充 | 采纳；增加多权威、LOD、frame/Jacobian、重叠 SDF、未批准目标/预算停止条件 |

## 4. 输入、派生状态与生成请求分层

### 4.1 CharacterDefinition

唯一可编辑人体语义输入：

- ProportionProfile；
- 用户局部形态参数；
- 显式解剖标志点；
- 左右差异；
- 参数配置。

### 4.2 ResolvedCharacterState

不可变派生结果：

```cpp
struct ResolvedCharacterState {
    Anthropometry anthropometry;
    AnatomicalLandmarks landmarks;
    AnatomicalRegions regions;
    BodyJoints joints;
    std::array<JointFrame, BoneCount> jointFrames;
    Skeleton skeleton;
    ValueConfidence confidence;
};
```

规则：

- `BodyJoints` 由 resolved joint centers 派生，不作为第二权威输入；
- `Skeleton` 由 resolved joints 构建，BoneId 0–21 不变；
- resolver 优先级：显式 landmark → BodyJoints 兼容适配 → proportion/anthropometry → Undetermined；
- 派生对象不可回写 CharacterDefinition。

### 4.3 GeometryBuildRequest

生成策略和版本信息：

```cpp
struct GeometryBuildRequest {
    uint32_t generatorVersion;
    TopologyPolicy topology;
    uint32_t detailLevel;
    uint32_t templateVersion;
    RegionMask regionMask;
};
```

`TopologyPolicy` 属于生成配置，不属于人体解剖语义；CharacterDefinition 可持有版本化引用，但不得把 mesh 拓扑当作第二人体定义。

## 5. Sweep、frame 与局部形态约束

### 5.1 Frame

- 初始 frame 由解剖标志点和关节局部坐标系确定；
- 使用 Parallel Transport / Rotation-Minimizing Frame；
- 定义 tangent 反向、接近零曲率、重复点和急弯的处理；
- 左右侧采用一致的 side convention；
- 每段检查 `dot(t_prev,t_next)`、frame orthonormality 和 Jacobian 符号；
- frame 翻转或 Jacobian 退化立即停止。

### 5.2 截面

- 普通区域：可变截面 Sweep；
- 肩带、腋窝、关节等复杂区域：独立 Surface Patch；
- 截面使用闭合周期样条或二维控制曲线，不只单值椭圆半径；
- 12–16 个语义控制点仅为 VR-1A 起点，不是最终拓扑分辨率；
- 必须检查截面自交、星形性假设和局部曲率。

### 5.3 局部形态基

每个 basis 必须声明：

- support region；
- amplitude range；
- 边界值/一阶导数连续性；
- 法线方向定义；
- 允许组合集合。

组合测试必须覆盖：

- 曲面翻转；
- 局部折叠；
- 过度凸起；
- 曲率突变；
- 自相交；
- Jacobian 符号变化。

## 6. Canonical Skin Shell 边界规则

VR-2/VR-3 设计前必须明确：

- Skin Shell 包含哪些连续皮肤区域；
- eyeball、iris、cornea、teeth、tongue 等辅助几何不计入 Skin Shell 封闭性；
- eyelid 必须与面部皮肤保持真实拓扑连接；
- nostril、oral fissure、ear canal、nasal cavity、oral cavity 哪些是封闭凹陷，哪些是显式开放边界，哪些是独立内部几何；
- 所有开放边界必须登记 `BoundaryId`、用途、相邻组件和验收规则；
- 禁止局部 SDF 负向操作产生未管理裂口；
- 手指/脚趾必须是独立分支拓扑，不是在掌面/足面贴圆管；
- 独立 digit 几何不代表独立 digit 骨骼动画。

## 7. Stable ID 与 LOD Correspondence

### 7.1 持久语义 ID

```text
TemplateUUID + TopologyVersion + RegionId + Side + LocalVertexId
```

用于跨人体形态参数变化保持身份。

### 7.2 细分来源

新增顶点必须记录：

```text
ParentFaceStableId / ParentEdgeStableId / ParentVertexStableId
+ BarycentricCoordinates
+ RefinementPath
+ LODLevel
```

### 7.3 映射规则

- 参数变化只改变位置，不改变拓扑身份；
- detail 增加产生新顶点时保留 parent lineage；
- 建立 LOD0 ↔ LOD1 ↔ LOD2 correspondence map；
- 不兼容 topology version 必须生成显式迁移映射；
- 禁止把 build-local index 当作持久 ID；
- corrective、weights、shape basis、renderToCanonical 必须使用同一身份体系。

## 8. 权重、动作与局部形变验证

### 第一层：权重合法性

- BoneId 0–21；
- 非负、Top-4、和 `1±1e-3`；
- region bone mask；
- 左右肢体隔离；
- 左右肩使用不同 BoneId mask。

### 第二层：基础动作

VR-1A：

- Bind；
- 左/右手臂分别抬起；
- 肩外展；
- 肩前屈；
- 左右独立动作。

角度、旋转轴和坐标系必须记录。Shoulder Bone 只代表短锁骨/肩带连接，不等同完整肩胛运动学。

### 第三层：局部形变质量

- 肩部体积变化；
- 腋窝凹陷保留；
- 关节塌陷；
- 异常凸包；
- 穿插；
- 左右对称/独立变形规律。

## 9. VR-1A 推荐边界

保留：

- mode-2 双侧躯干—肩部原型；
- 旧 P1-A baseline；
- 四阶段主路线。

实施顺序：

1. 数据层：CharacterDefinition → ResolvedCharacterState → BodyJoints/Skeleton。
2. 几何层：Thorax、Abdomen、Upper Pelvis、Shoulder Girdle、Axilla、Bilateral Upper-arm Root。
3. 拓扑/权重：Shared BoundaryEvaluator、C0、G1 测量定义、无 T-junction、Top-4、左右隔离、Stable ID。
4. Android：Clay、Wireframe、Normal、Weight Heatmap、左右肩分别抬起、同机位新旧对比、Cache/Skeleton/Surface 生命周期。

新增测试：

- 先左右完全对称；
- 再左肩略宽/右肩略窄；
- 证明两侧共享规则但独立参数、区域、顶点和骨骼；
- 禁止镜像顶点复用和权重串扰。

暂不纳入：面部、手部、足部、完整 Canonical、最终 UV 重构、Corrective、新骨骼、PBR、头发、服装。

## 10. OQ-03 治理冲突

### 10.1 冲突事实

- 仓库 `docs/phases/anatomy-driven-geometry-poc/10-decision-records/01-OQ-03-decision-seam-hard-condition.md` 声称 OQ-03 已 DECIDED，并固定相邻 quad 外法线 max `<=5°`。
- 专家根据 ADR-013/AGENT.md 认为 OQ-03 第 5 条仍保留“测量口径未定，数值不得判定”的约束。
- 两种解释不一致，AI 不得自行裁定。

### 10.2 VR-1A 临时处理

- 5° 不作为 VR-1A 硬验收阈值；
- 可记录两类研究诊断：
  1. 现有 OQ-03 相邻 quad 外法线 max/mean/p95；
  2. 专家建议的 seam 两侧对应采样点法线夹角。
- 两类结果都不得标记 PASS；
- 正式阈值、测量对象和统计方式由负责人确认后写入验收；
- 未确认前使用 `NOT APPROVED / NOT APPLICABLE`，不得隐瞒或改写决策记录。

## 11. Weight Heatmap 与缓存

### Weight Heatmap

不能只验证“有颜色”。必须显示/记录：

- 当前 BoneId；
- 每顶点实际权重；
- 最高权重骨骼及权重；
- 左右区域是否出现不应有的骨骼影响；
- 模式切换后颜色与 Skinning 权重一致。

### 缓存

- 缓存不可变生成几何；
- 缓存 matching definition/hash、Skeleton、BodyJoints；
- 不把动态 MotionRuntime 状态当几何缓存；
- 模式切换事务式恢复；
- Pose、SkinPalette、MotionRuntime 明确重置或重新同步。

## 12. 视觉目标与性能预算

### 视觉目标

现有 `docs/foundation/07-visual-target-alignment.md` 仍含历史自然卡通目标。P1-VR 的自然半写实成人目标需要负责人正式确认 supersede 关系；AI 不擅自改写旧视觉文件。VR-1A 局部通过不代表完整人体最终视觉通过。

### 性能

分别监控：

1. 初始人体形态生成；
2. Patch 组装；
3. 权重生成；
4. GPU 上传；
5. 稳态渲染帧时间；
6. 峰值内存。

负责人已批准“比 P1 基线下降不超过 20%”的临时工程 guardrail，并要求分项记录初始形态生成、Patch 组装、权重生成、GPU 上传、稳态帧时间和峰值内存。正式产品性能目标仍需后续批准，但不阻塞采用临时 guardrail 的 VR-1A。

## 13. 新增停止条件

- CharacterDefinition 与 BodyJoints 出现双向或多重权威；
- LOD Stable ID 无法建立可靠映射；
- frame 翻转或局部 Jacobian 退化；
- 结构化 Patch 必须依靠大量重叠 SDF 才能融合；
- 基准人体参数未批准，或性能超过已批准的临时 guardrail；
- OQ-03 阈值/口径未确认却被当作硬验收；
- Weight Heatmap 与实际权重不一致；
- cache mesh 与 Skeleton/BodyJoints 不匹配。

## 14. GNM Head 补充研究

### 14.1 官方来源

- 仓库：`https://github.com/google/GNM`
- 核查 HEAD：`25aafd3bf310c2a50eb0347602a023af2eb09ae4`
- 技术报告：arXiv `2607.23687v3`，2026-09-11 修订。
- 官方 README：`gnm/shape/README.md`。

### 14.2 已核实内容

- GNM Head 包含 head、face、neck、eyeballs、teeth、tongue。
- v3.x identity 253 维：head 170、eyeball 3、teeth 80。
- expression 维度存在来源差异：官方 README 为 383（left eye 100、right eye 100、lower face 150、tongue 32、iris 1）；arXiv v3 §3.9 Table 1 为 382（100+100+150+31+1，最后一项称 pupil）。该差异未裁定，文档同时保留两种口径，不把 383 称为论文与仓库共同核实值。
- head pose 与 eyeball orientation 使用 joint axis-angle + global translation。
- model data 包含 template shape、identity basis、expression basis、skinning weights、UV layout。
- 提供 quad 与 triangulated topology，UDIM 1001–1007 按 skin/eye/teeth/tongue 分区。
- 明确训练数据使用二元性别分类和四个宽泛人口群组，代表性有限。

### 14.3 PAE 用法

- 作为 VR-2 面部拓扑、内部结构和参数分区的参考；
- 不导入 GNM 网格、basis、weights、textures、semantic sampler 或训练数据；
- 不推迟 VR-1A；VR-2 正式设计前完成针对性源码/论文复核。

### 14.4 许可证

- 仓库整体 Apache-2.0；
- LICENSE 中 tongue animation mesh 改编自 Salvador Medina，另有 MIT 条款；
- 官方 README 宣称 ecosystem 为 Apache-2.0，但第三方资产、模型文件、训练数据仍须逐项核查；
- PAE 当前不引入任何 GNM 资产。

## 15. 负责人决定与剩余前置条件

负责人已明确决定：

1. **OQ-03**：暂缓硬阈值；两类角度只作研究诊断，待正式决策记录确认。
2. **视觉目标**：确认自然半写实成人目标正式替代历史自然卡通目标；AI 不擅自改写旧视觉文件，只在 P1-VR 文档中记录替代关系。
3. **性能**：采用临时 20% 同设备基线 guardrail，并分项记录生成、组装、权重、GPU、帧时间和内存；该 guardrail 不替代产品预算。
4. **P1-VR 补充授权**：仍待负责人正式签发；AI 不代签。

当前唯一剩余的生产写入前置条件是正式补充授权和白名单。VR-1A 在授权签发前不进入生产实施；本附录不代表 Gate PASS。
