# Asset & License Policy

权威：`docs/architecture-decisions/ADR-014-offline-compiled-human-base-topology.md`、`docs/research/human-template-reuse/01-asset-license-and-provenance.md`。

## 核心规则

- 人体数据由 PAE 自有代码处理；几何/形态由 PAE 程序化生成或驱动；骨骼/关节/权重 PAE 自定义管理。
- 运行时不依赖在线 AI、Blender、MakeHuman、MPFB2 等 DCC，不导入外部完整人体模型作运行时人体。
- AI 只在开发阶段协助研究/设计/实现/测试。

## CC0 资产范围（ADR-014）

MakeHuman 官方 Base Mesh + 逐文件验证的 CC0 Shape Targets 仅允许离线编译为 PAE 模板。固定 commit `a8bc2d54`，逐文件 SHA-256 与许可证记录。

**不得引入** MakeHuman 软件代码、MPFB2 代码或未审核第三方数据。

## PENDING_CONFIRMATION（不得分发/运行时依赖）

`default.mhskel`、`default_weights.mhw` 仍 PENDING_CONFIRMATION：只能作合规范围内的 Host 研究参照，不得进 APK、不得作 PAE 默认 Rig/权重。生产 Rig 与权重必须 PAE 自有。

## 模板处理检查清单（PAHT Workflow）

Asset Provenance / License Manifest / Base Mesh SHA-256 / Target SHA-256 / OBJ Parsing / Stable Vertex ID / Helper-Joint Groups / PAHT Compiler / PAHT Version / Payload Hash / Read-Write Roundtrip / Corrupted File Rejection / APK Asset Loading / Runtime Compatibility。

PAHT v3 字段语义冻结；新格式须版本化迁移策略（读旧格式），不静默改 v3 语义。

## Saltline

仅作材质/渲染/程序化生成技术参考，不作完整自然人体拓扑与变形质量证明。借鉴范围需先研究 + 许可证审核。