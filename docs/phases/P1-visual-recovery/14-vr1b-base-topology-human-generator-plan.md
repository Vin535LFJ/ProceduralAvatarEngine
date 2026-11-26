# VR-1B：Base-Topology 人体生成器计划与授权草案

任务：P1-VR-ARCH-1 / Deliverable R8  
日期：2026-11-26（合并版）  
状态：**`DRAFT / NOT AUTHORIZED / NOT STARTED`**

本文只提交方案和授权草案。**不启动 VR-1B 的任何生产实现**，不修改 Android 生成器、Canonical 路径、LegacyRigV1、UBO、顶点打包或 JNI。前置条件是负责人对 R7（含 ADR-014 草案）的裁定。

## 1. 目标与一句话验收

在一个**完整人体**上证明：`PAEHumanTemplate → 参数 → Rig V2 → 权重 → 姿势 → Render Mesh → Android GLES3` 全链路由 PAE 自己的代码在设备上跑通，并产出与 R1/R2 同口径的 7 视角证据。

一句话验收：Android 设备上渲染出一个由参数改变过体型、由 Rig V2 驱动、能完成肩/肘/髋/膝/腕/踝六类动作的完整人体，CPU/GPU skinning 结果一致。

长期视觉目标保持：**完整、自然、半写实的成人亚洲人体**。基础网格的中性形态只是起点，不代表最终身份或视觉验收。

## 2. 范围

### 2.1 包含

| 工作包 | 内容 | 依赖 |
|---|---|---|
| WP0 治理 | ADR-014 签署；骨架/权重许可证裁定；本计划的白名单与授权书 | 负责人 |
| WP1 模板编译器产品化 | 把 `tools/human-template-poc/host/src/` 的读取、审计、target 解析、模板序列化整理为 `native/procedural/human_template/`（host 工具目标，不进 APK）；补校验和与可选段；模板生成物作为版本化资产 | WP0 |
| WP2 运行时模板加载 | 只读 `.paht`；校验 magic/版本/哈希；建立稳定顶点 ID → 顶点缓冲的映射 | WP1 |
| WP3 形态层 | `CharacterDefinition` 参数 → target 权重的映射表（先做已验证的 9 个）；`V = V0 + Σ α ΔV`；法线重算；测量口径对齐 | WP2 |
| WP4 Rig V2（core 20 骨） | 可变骨骼数骨架定义；动态调色板；与 LegacyRigV1 并存 | WP0、WP2 |
| WP5 权重 | 已许可权重或 PAE 生成 + Top-4 + 无权重顶点回退规则（host 已实现，见 R4 §4.2） | WP4 |
| WP6 蒙皮 | CPU LBS 与 GPU LBS（现有 shader 结构）适配可变骨骼数；parity 测试 | WP4、WP5 |
| WP7 Render Mesh | 四边形三角化（剖分规则）；法线；UV；现有材质系统接入；RenderThread 接入点 | WP2 |
| WP8 验证 | 7 视角 + 关节近景；六类动作（每个带方向断言）；性能；失败/未验证清单 | 全部 |

现状说明：P1-VR-ARCH-1 已经把 host 原型放进仓库（`tools/human-template-poc/`，65 项测试通过）。WP1 是**产品化整理**，不是从零开始；原型不进 Android 构建。

### 2.2 不包含

- 面部表情、下颌、眼球、舌头、睫毛、头发、牙齿、服装。
- 逐指/逐趾动作（MVP 非目标）；deform 102 骨只作为可选导出，不进本轮设备验证。
- DQS、corrective shapes（只在 WP8 记录需求，不实现）。
- LOD / 细分 / 局部加密。
- 材质烘焙。
- 替换 mode-3 Canonical（VR-1B 验收通过后另议）。
- 任何 MakeHuman / MPFB2 运行时或代码。

## 3. 关键设计决定（需在 WP0 一并裁定）

| # | 决定 | 建议 | 备选 / 现状 |
|---|---|---|---|
| D1 | 模板资产进仓形式 | 版本化二进制 `.paht` + 来源清单；不进 OBJ | 只进编译器，模板在构建期生成。现状：模板含时间戳，哈希不可复现，进仓前要先解决 |
| D2 | 骨架/权重来源 | 许可证确认则用源 rig + 源权重；否则 PAE 从关节立方体自建 20 骨 + 距离法权重 | 全部 PAE 自建。现状：两个源文件为 `PENDING_CONFIRMATION`；PAE 距离法在肩外展下有一条边拉伸 10 倍，需先调参 |
| D3 | Rig V2 骨骼数 | 20（core） | 22（向 LegacyRigV1 槽位数靠拢，但语义仍不同） |
| D4 | 调色板承载 | UBO 分块（≤ 64 骨/块，本渲染器只需 1 块）或骨骼纹理 | 沿用 `uBones[22]` 常量改为 32 |
| D5 | 顶点打包 | 不变（64 B，joints u16 ×4，weights f32 ×4） | 增加影响数到 8 |
| D6 | 三角化 | 最长对角线或“按质量择优”剖分；不在模板里固定 | 固定 0-2 对角线（不推荐，最小角 3.06°） |
| D7 | helper 几何 | 关节立方体位置随模板进运行时（用于骨架随形态）；眼球/牙齿/舌/睫毛本轮不渲染 | 全部丢弃（则骨架不能随形态） |
| D8 | 无权重顶点 | **已在 host 实现**：`kept-ancestor`（被剔除的骨 → 源层级最近的保留祖先），备选 `nearest-bone`（本轮命中 0） | 待负责人确认采用 |
| D9 | 手指动作范围 | 声明不支持握拳；腕/踝只做旋转 | 手掌局部加密（超出本轮）。依据：握拳 714 翻面 |
| D10 | 测量口径 | 以 `CharacterDefinition` 的 ANSUR II p50 比例为准，建立 PAE 表面 Landmark | 沿用本轮自定义平面（不可用于验收） |
| D11 | 动作验收 | 每个动作带方向断言（关节位移方向 + 镜像关节不动 + 末端向躯干靠近），沿用链 B 的做法 | 仅检查“数值有限”（不接受：链 B 的屈肘曾方向相反而位移大小不变） |

## 4. 任务拆分与顺序

1. **WP0**（治理，负责人）：ADR-014；R1 的两个许可证待决项；批准白名单；D1–D11。
2. **WP1 → WP2**：模板编译器与加载器，带往返测试与哈希校验（原型的 T12/T13 可直接迁移）。
3. **WP7（先行一小步）**：只加载模板并三角化渲染静态人体——尽早拿到第一张设备截图。
4. **WP3**：9 个参数的映射 + 测量；参数影响表（复用 R3 的口径）。
5. **WP4 → WP5 → WP6**：Rig V2 core、权重、蒙皮、parity。
6. **WP8**：证据、性能、失败与未验证清单。

每一步都按 `AGENT.md` 交付代码 + 新图 + 日志原文，不允许只交文档的轮次。

## 5. 生产文件白名单（规划名称，不表示现在创建或修改）

### 5.1 Host / Template 核心（WP1）

```text
native/procedural/human_template/PAEHumanTemplateFormat.h/.cpp
native/procedural/human_template/HumanTemplateCompiler.h/.cpp
native/procedural/human_template/HumanTemplateLoader.h/.cpp
native/procedural/human_template/AnatomicalRegionMap.h/.cpp
native/procedural/human_template/HumanTemplateLandmarks.h/.cpp
native/procedural/human_template/HumanRigV2.h/.cpp
native/procedural/human_template/HumanWeightCompiler.h/.cpp
```

### 5.2 Android 隔离适配（WP2、WP4、WP7）

```text
native/procedural/human_template/BaseHumanCanonicalGenerator.h/.cpp
native/procedural/human_template/BaseHumanRenderMeshBuilder.h/.cpp
native/renderer/gl/DynamicBonePalette.h/.cpp
```

`RenderThread.cpp` 只有在单独授权后才能增加构建期选择；默认生产行为不得改变。

### 5.3 测试

```text
tests/unit/human_template_format_tests.cpp
tests/unit/human_template_target_tests.cpp
tests/unit/human_rig_v2_tests.cpp
tests/unit/base_human_canonical_tests.cpp
tests/unit/human_pose_direction_tests.cpp
```

### 5.4 明确不修改

```text
LegacyRigV1 / BoneId 0–21
UBO[22]（LegacyRigV1 分支）
64 B Vertex Packing
JNI ABI
现有 RenderThread 生产路径
R1/R2 生成器与证据
```

## 6. 测试矩阵

### 6.1 数据与格式

- 固定输入 byte-identical（需先解决时间戳）；
- Stable ID 唯一；
- Quad/UV/Region/Landmark 完整；
- Target/Base/Topology hash 一致；
- provenance 缺失或许可证状态不是 `CC0-1.0` 时拒绝；
- 损坏 section/index 拒绝；
- 权重归一化、BoneId 范围、回退规则合法；
- LOD parent/barycentric 可逆验证（本轮不做 LOD，留位）。

### 6.2 形态

- 每个 Target 单独、归零（走真实路径，不是乘 0）、min/max；
- 左右独立；
- 随机组合；
- 高层参数因果与区域外影响统计；
- 法线、winding、退化、sliver、自相交（target 之后与姿势之后）；
- 极值失败定位到 Target/顶点。

### 6.3 Rig / 动作

- Bind Pose 恢复；
- 左右肢体独立（关节严格不动；表面渗漏如实报告）；
- 肩/肘/髋/膝/腕/踝，每个带方向断言；
- clavicle/scapula 联动（如纳入）；
- Top-4 vs full-weight 位置误差；
- LBS vs DQS/pose corrective 决策；
- 极限姿态体积与穿插检查。

### 6.4 Android

- CPU/GPU parity；
- cache key 覆盖所有形态/Rig/LOD 字段；
- Surface recreation；
- 进程生命周期；
- Legacy 回归；
- 同条件 R1/R2/Base APK；
- 性能与内存。

## 7. 验收证据（WP8）与门槛

交付：7 视角灰模（`CameraProtocol::fixedViews()`）+ 右侧视图 + 肩/腋/髋/手/足近景；基础体型与至少 4 组参数变体；六类动作各一张；CPU/GPU 顶点位置最大差；参数影响表；模板校验日志；生成/加载/上传/帧时间/峰值内存；与 mode-3、R1、R2 的对照表（旧数据引用既有证据，标明未重跑）；失败项清单与 `NOT RUN / UNVERIFIED / PARTIALLY VERIFIED` 清单。

| 门槛 | 内容 |
|---|---|
| 技术 | Body 单一闭合流形，无内部 assembly seam；无非流形、重复面、退化、未处理自交；Target 改变正确区域；Rig/Weight/动作可复现且方向正确；模板完整可审计；Android CPU/GPU 与 Legacy 回归通过 |
| 视觉 | 肩胸、腋窝、髋腹股沟连续自然；面部、手指、脚趾结构合理；基础动作无明显撕裂、塌陷或异常牵扯；最终成人亚洲形象由负责人确认；不使用服装/头发/Normal Map 隐藏几何问题 |
| 性能 | 使用同 workload baseline；记录生成、上传、帧时间和峰值内存；不把 Host 全流程时间与 Android 局部生成时间混比；现有临时 20% guardrail 只有在同 workload 下才可判定 |

任何 Gate 由负责人决定。

## 8. 停止条件

出现以下情况立即停止并报告：

- 许可证或来源无法确认；
- Target 与 Base 版本不匹配；
- 形态极值造成严重折叠/自交；
- Rig/Weight 无法稳定恢复 Bind Pose；
- Android 需要破坏冻结 ABI 才能继续；
- 新路线没有相对 R2 显示结构或视觉优势；
- 需要未经授权扩大生产范围。

## 9. 风险

| 风险 | 应对 |
|---|---|
| 章程冲突未解除 | WP0 不通过则不启动 WP1 之后的任何运行时工作 |
| 许可证依据不足 | D2 走 PAE 自建，并先把距离法调到可用 |
| 设备上 LBS 变形质量不达标（肩外展 3 倍边拉伸） | 限制测试姿势幅度并如实记录；corrective 留到下一阶段 |
| 模板体积（4.1 MB）与哈希不可复现 | 位移量化到 0.1 mm、int16/int32；时间戳移出内容；评估 APK 体积影响 |
| 调色板改动触及 RenderThread 生产路径 | 只新增路径，不改 LegacyRigV1 分支；白名单列明 |
| 动作方向错误但数值“通过” | D11：方向断言 + 至少一张对应视角的图 |

## 10. 回退

任一条件不满足：保持 mode-3 / R1 / R2 现状；`tools/human-template-poc/` 保留为离线工具链；模板数据不进运行时。

---

## 附：授权书草案（未签署，待负责人填写）

### P1-VR-1B 授权书（草案）

状态：**`NOT AUTHORIZED — DRAFT`**

基线：

- 工作分支：`poc-sm/shape-model-v2`。
- 已审查基线：`f1b4404f5d58b11397eee6f2d37372a7126b3bb5`；其上新增的 P1-VR-ARCH-1 提交（host 工具 + 文档 + 证据）不含任何生产路径改动。
- 研究/原型依据：`docs/research/human-template-reuse/00…06`、`docs/architecture/human-template/01…02`。
- 实施白名单：待起草（建议命名 `15-vr1b-implementation-whitelist-draft.md`）。

需负责人逐项决定：

- [ ] 是否批准 ADR-014。
- [ ] `default.mhskel` 与 `default_weights.mhw` 是否可用于运行时模板。
- [ ] D1–D11 的取舍。
- [ ] Rig V2 采用 20 骨 core。
- [ ] 允许修改的范围：新增文件 + RenderThread 的新增接入点 + CMake；明确禁止修改 BoneId 0–21、UBO[22] 的 LegacyRigV1 分支、64 B 打包、JNI 现有签名、mode-3 Canonical、Head/Face/Hand/Foot 生产实现。
- [ ] 是否允许把 `.paht` 模板纳入仓库与 APK。
- [ ] 性能 Guardrail 口径（同设备、同工作量）。
- [ ] 视觉验收人及验收时间。

```text
授权编号：待负责人指定
状态：DRAFT / NOT EFFECTIVE
批准人：待负责人签署
日期：待填写
允许阶段：待填写
额外限制：待填写
```

本授权只覆盖 VR-1B，不自动授权替换 mode-3 Canonical、面部/手部细节、DQS/corrective 或 LOD。
