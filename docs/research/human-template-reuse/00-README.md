# Human Template Reuse（P1-VR-ARCH-1）——阅读入口

日期：2026-11-26  
范围：评估“以许可证明确的成熟人体拓扑（MakeHuman CC0）为模板来源，由 PAE 自己做形态、Rig、权重和渲染”这条路线。  
结论：**建议 CONDITIONAL GO**（建议稿，判定权在负责人）。全部工作在 host 上完成，**Android 未实现、未验证**；没有任何生产路径改动。

这是两条并行实现（链 A、链 B）合并后的**唯一一套文档**，位于 `poc-sm/shape-model-v2`。

## 先读哪份

| 想知道什么 | 读 |
|---|---|
| 现在到哪一步、还差什么、要我定什么 | `06-consolidated-review-and-status.md` |
| 路线建议、15 个问题的回答、ADR-014 草案 | `../../architecture/human-template/02-human-geometry-architecture-decision.md`（R7） |
| 下一阶段怎么做、授权书草案 | `../../phases/P1-visual-recovery/14-vr1b-base-topology-human-generator-plan.md`（R8） |

## 交付物索引

| 编号 | 文件 | 内容 |
|---|---|---|
| R1 | `01-asset-license-and-provenance.md` | 许可证与来源审计：网格与 target 为 CC0-1.0；**骨架与权重两个文件待确认** |
| R2 | `02-base-human-topology-audit.md` | 拓扑质量：13,380 顶点、13,378 四边形、闭合、带 UV；区域检视与已知弱点 |
| R3 | `03-shape-target-and-parameter-analysis.md` | Target 格式（0 基、XYZ）、参数影响了哪些区域、与 `CharacterDefinition` 的映射 |
| R4 | `04-rig-and-skin-weight-analysis.md` | Rig V2（102 / 20 骨）、三套权重对比、无权重回退规则、基础动作的翻面与拉伸 |
| R5 | `../../architecture/human-template/01-pae-human-template-format.md` | PAE 自己的模板格式 `.paht`（保留四边形、稳定顶点 ID、来源可审计） |
| R6 | `05-host-only-template-proof.md` | 实施报告：六步验证链、测试日志原文、失败记录、与旧路线的对照、101 张图 |
| — | `06-consolidated-review-and-status.md` | 复核记录、覆盖对照、完成度、下一步 |
| R7 | `../../architecture/human-template/02-human-geometry-architecture-decision.md` | 架构决策建议 + ADR-014 草案 |
| R8 | `../../phases/P1-visual-recovery/14-vr1b-base-topology-human-generator-plan.md` | VR-1B 计划与授权草案 |

## 代码与证据

| 位置 | 内容 |
|---|---|
| `tools/human-template-poc/host/` | host-only 工具（不被 Android 构建引用）。链 A：`src/`（`pae_ht_tests`、`pae_ht_pipeline`）；链 B：`template_proof.cpp`、`render_proof.py` |
| `evidence/host-proof/README.md` | 证据入口：两条链的区别、文件说明、复现命令 |
| `evidence/host-proof/logs/` | 测试与运行日志原文（`65 passed, 0 failed`；`DIRECTION_CHECK actions=8 failed=0`） |
| `evidence/host-proof/images/`、`images-proof-b/` | 本轮新生成的 84 + 17 张图（host 软件渲染） |
| `evidence/host-proof/provenance/` | 逐文件的 SHA-256、Git blob、许可证状态 |
| `evidence/host-proof/SHA256SUMS.txt` | 证据目录完整性清单 |

## 状态标签（按 `AGENT.md` 语义）

- Host 链路：**已执行**，两条独立实现互相印证。
- 视觉：**只有图像描述模型看过，未经人眼**。
- Android（GLES3、UBO、JNI、RenderThread、CPU/GPU parity）：**NOT RUN**。
- 姿势后的自相交、DQS / corrective、Biharmonic 权重、宏参数组合、LOD、表面 Landmark：**NOT RUN**。
- 骨架与权重源文件的许可证：**PENDING_CONFIRMATION**。
- 所有 Gate：**未判定**（判定权在负责人）。

## 需要负责人决定

1. 是否批准 ADR-014（把“许可证明确、离线编译成 PAE 自有模板的拓扑与形态数据”列为例外）。
2. `default.mhskel`、`default_weights.mhw` 能否使用。
3. VR-1B 计划里的 D1–D11 与授权范围。
4. 亲自查看证据图后，对“是否比 R1/R2 的拼接躯干更像完整的人”作出判断。
