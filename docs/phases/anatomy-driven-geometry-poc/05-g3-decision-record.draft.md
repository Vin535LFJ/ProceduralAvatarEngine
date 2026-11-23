# G3 Decision Record — DRAFT（按 SPEC §7.2 模板）

> 状态：**`DEFER` — 待负责人签署**。本记录不选定任何生产路线。
> 依据：负责人 2026-11-23「同意，采纳你全部建议」；SPEC §7.2 规定 DEFER = 补指定证据后**重开 G2**，不得默认为通过。
> G2 证据基线：`a32251b`；后续文档提交：`e8e697e`、`92765cb`。
> 上游设计：`09-final-recommendation.md`（ADOPTED IN PRINCIPLE）。
> 推进方案：`03-g3-defer-and-next-phase-proposal.md`。

## 1. 参与决策的路线

G1 判定三条均 `FEASIBLE`，无 `KILLED` 路线。R4 Dual Contouring 无理由不扩大范围。

| 路线 | 定位 | 本决策处置 |
|---|---|---|
| R1 SDF + Surface Nets | 参考基准（无固定拓扑） | **保留为参考**；不作生产候选（不满足 §8.1 固定拓扑前提） |
| R2 固定 quad 模板投影到 SDF | 后端候选 | **保留候选，暂停加码** |
| R3 固定 quad 模板架在 B-spline 包络 | 备选 | **暂停投入**（包络表达力不足，实证见下） |

## 2. 各路线 Layer A / Layer B 证据

全部数据来自 `evidence/g2/fair-comparison.md`，三参数组同输入/同采样/同相机。

| 证据 | R1 | R2 | R3 |
|---|---|---|---|
| E1 mean / Hausdorff（m） | 0.00440 / 0.01724 | 0.00440 / 0.01724（与 R1 共面，自比） | 0.03272 / 0.13310 |
| E2 mean / max（m） | 0.00098 / 0.02310 | 0.00098 / 0.02310 | **0.00040 / 0.00100**（投影最优） |
| E3 mean（m） | **0.00149** | **0.00149** | 0.03211（差 21.5×） |
| Landmark mean / max（m） | 0（自比） | 0（自比） | 0.0544 / 0.1268 |
| 法线 shipped mean / p95 | 9.23° / 32.57° | 9.23° / 32.57° | 27.14° / 67.15° |
| 截面 \|dev\| mean / p95（m） | 0（自比） | 0（自比） | 0.0268 / 0.1022 |
| 顶点数跨参数组 | 5108→5234→5314（ID 不稳定） | 833 恒定 | 833 恒定 |
| 面邻接恒定 | 否 | **是** | **是** |
| Sliver 数 | 30 | **0** | 20 |
| 正交 quad 比 | 0.637 | 0.935 | 1.000 |
| 肩根 crop 最大二面角 | 47.66°（无登记 crop） | **47.66° — FAIL ≤5°** | **16.13° — FAIL ≤5°** |
| 权重合法性 / CPU 变形 | 合法 / 通过 | 合法 / 通过 | 合法 / 通过 |
| GPU 数值 parity | `[UNVERIFIED]` | `[UNVERIFIED]` | `[UNVERIFIED]` |

**归因结论**：R3 的误差在其包络曲面（上胸缺口 13.5 cm、有效参数 7/20、加密不收敛），不在固定拓扑投影（E2 反而最低）。R2 的 E3 优势是相对于当前 SDF target 的**后端保真度**，不能解读为"人体形态正确"。

**截图**：`evidence/g2/clay/index.md`，3 组 × 3 路线 × 7 视角，共 63 张。灰模相机/光/取景统一。

**kill 记录**：G1/G2 无路线被 KILL。DEF-02（骨架不一致 + 手臂 lobe 污染）已在 G1 修复并有回归测试 `routes_share_one_skeleton`。

## 3. 推荐项

**无 approved route。** 建议 `DEFER`。

理由：
1. §13 阈值除 ≤5° 合同值外全为 `TBD`，未获负责人批准，不得判 PASS。
2. 负责人独立视觉 Gate 未行使；AI 不得自标 VISUALLY ACCEPTED。
3. ≤5° 接缝合同失败未解决，且 R2/R3 均失败。
4. 共同上游 Shape Model 形态不足；此时选路线等于锁定一个不够好的目标（DEC-09 采纳的依据）。

## 4. 后果

- **ABI**：无变更。BoneId 0–21、UBO[22]、64B packing 保持冻结。
- **主线**：R2 作为候选后端保留；R1 保留参考；R3 暂停投入但不删除证据。
- **遗留**：legacy 三路径冻结，未触碰；G2 生产源码唯一改动是 `SdfFusion.cpp` 符号修正（已推导 + 双实现单测，APK SHA `088476fc…` → `c1cace9d…`，设备复验 NOT RUN）。
- **方法资产**：E1/E2/E3 分离、Layer A/B、`TopologyAudit` crop/裂缝区分、7 视角相机协议均可继承到下一轮。
- **作废项**：换 Target Reference 后，所有 E1/E3、landmark、截面、7 视角偏差数字作废，须重跑 G2。

## 5. Kill criteria（重开 G2 时必须全清）

| 触发 | 当前状态 |
|---|---|
| 视觉 P0 缺陷 / owner 否决 | **NOT RUN** — 63 张图已交付，未获裁决 |
| 拓扑不稳（components≠1 / 未登记 boundary / non-manifold） | CLEAR |
| 指标硬编码 / 赋值 / TBD 判 PASS | CLEAR |
| 参数声明未达顶点 | PARTIALLY VERIFIED（G0/G1），本轮未升级 |
| 设备 UNVERIFIED 报 PASS | CLEAR |
| 用 tri/vertex 数冒充质量 | CLEAR |
| **≤5° 接缝合同失败** | **FAIL — 阻塞** |
| GPU parity `[UNVERIFIED]` | 按 §11.1 **不是 G2 kill** |

## 6. GPU Parity 后续风险与 G4 前完成计划

- 当前：`[UNVERIFIED]`。G2 只要求权重合法性 + CPU 侧变形，均已完成。
- G3 决策记录必须明确该风险（本文件即记录处）。
- G4 前计划：在批准路线确定后，立专项完成 **CPU/GPU 数值 Parity + Android GLES3 实机验证**，阈值由设备精度实验确定（未测前 TBD）；未完成则 G4 回退。
- 当前 APK 因 `SdfFusion.cpp` 改动哈希变化，虽两函数无生产调用方，仍需在 G4 前做一次设备复验。

## 7. `approved route`

**空。** 任何 AI 或审核人不得代填。

## 8. 重开 G2 的补证要求

1. DEC-05 授权参考图 + rubric（`04-visual-rubric.md` 判定栏填满并签字）。
2. PoC-SM Before/After 灰模对照（需 AUTH-08 授权后实施）。
3. 肩根接缝 ≤5° 复测通过。
4. 以新 Target Reference 重跑的 G2 全套指标（E1/E3 重新计算）。

## 9. 签署

```
owner 签字：____________________  日期：____________
状态：DEFER / OPEN / SIGNED（负责人填写）
approved route：________________（负责人填写，空 = 未批准）
```
