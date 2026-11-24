# G3 Decision Record — DEFER（已签署）

> **状态：`DEFER` —— 已于 2026-11-24 由项目负责人签署。** 本记录**不选定任何生产路线**。
> 依据 SPEC §7.2：DEFER = 补指定证据后**重开 G2**，不得默认为通过。
> G2 证据基线：`a32251b`；后续文档提交：`e8e697e`、`92765cb`、`7c451ae`、`1ce1a21`。
> 上游设计：`09-final-recommendation.md`（ADOPTED IN PRINCIPLE）。
> 推进方案：`03-g3-defer-and-next-phase-proposal.md`。

## 决策记录（DECISION RECORD）

| 字段 | 值 |
|---|---|
| Gate | **GATE-04 — G3 Owner Investment Decision** |
| 状态 | **`DEFER`** |
| `approved route` | **（空 —— 本次不选择生产路线）** |
| 路线处置 | R1 保留为参考；R2 保留为拓扑投影候选（**暂停加码**）；R3 **暂停投入** |
| 决策人 | **项目负责人** |
| 决策日期 | **2026-11-24** |
| 决策来源 | 负责人在 WorkBuddy 会话中明确选择「签 4 份 + 下发 AUTH-08」执行包 |
| 记录方式 | AI **仅作转录与归档**；未代签，未代填 `approved route`，未添加负责人未表达的判断 |

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

> ⚠️ 归因结论仍待审核人复核：**OQ-06**（E1/E2/E3 归因是否成立）、**OQ-08**（R1/R2 "自比为 0" 是否诚实）。
> 这两项不阻塞 G3 的 DEFER 状态，但**阻塞 G3 重新批准**。

**截图**：`evidence/g2/clay/index.md`，3 组 × 3 路线 × 7 视角，共 63 张。灰模相机/光/取景统一。

**kill 记录**：G1/G2 无路线被 KILL。DEF-02（骨架不一致 + 手臂 lobe 污染）已在 G1 修复并有回归测试 `routes_share_one_skeleton`。

## 3. 推荐项

**无 approved route。** 决定 `DEFER`。

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
| **≤5° 接缝合同失败** | **FAIL — 阻塞**（测量口径见 `10-decision-records/01-OQ-03-decision-seam-hard-condition.md`） |
| GPU parity `[UNVERIFIED]` | 按 §11.1 **不是 G2 kill** |

## 6. GPU Parity 后续风险与 G4 前完成计划

- 当前：`[UNVERIFIED]`。G2 只要求权重合法性 + CPU 侧变形，均已完成。
- G3 决策记录必须明确该风险（本文件即记录处）。
- G4 前计划：在批准路线确定后，立专项完成 **CPU/GPU 数值 Parity + Android GLES3 实机验证**，阈值由设备精度实验确定（未测前 TBD）；未完成则 G4 回退。
- 当前 APK 因 `SdfFusion.cpp` 改动哈希变化，虽两函数无生产调用方，仍需在 G4 前做一次设备复验。

## 7. `approved route`

**空。** 任何 AI 或审核人不得代填。

## 8. 重开 G2 的补证要求（四项，缺一不可）

| # | 补证项 | 当前状态 | 阻塞 |
|---|---|---|---|
| ① | **DEC-05 完成**（参考体系批准 + Canonical Acceptance Sheet 批准） | `NOT RUN` | OQ-01（及其子项 OQ-28/29/30/31/32） |
| ② | **PoC-SM Before/After 证据齐备**（同相机/同光/同带/同取景灰模对照） | `NOT RUN` — AUTH-08 已签发，待实施 | 已解除（OQ-02 已关闭） |
| ③ | **肩根接缝复测**（按固定口径 ≤5°） | `NOT RUN` — 需新实现 | 已解除（OQ-02 已关闭）；OQ-14 |
| ④ | **以新 Target Reference 重跑 G2**（E1/E2/E3 分列 + Layer A/B 全指标） | `NOT RUN` — 依赖 ①②③ | 已解除（OQ-02）；OQ-18（体积口径） |

四项**全部**达成后，由**负责人**重新决策 G3；DEFER 不得被默认为通过。

## 9. DEFER 期间的边界

| # | 边界 |
|---|---|
| D-1 | **不选择生产路线**；`approved route` 字段留空 |
| D-2 | **不宣布任何 Gate 为 PASS**；`G-P1-GEO` 保持 FAIL |
| D-3 | **不启动 G4**（SPEC 要求 G3 已签署**且**视觉 Gate 通过） |
| D-4 | **不扩展 R2/R3 代码量**；R3 保持暂停 |
| D-5 | G2 证据冻结在 `a32251b`，后续文档提交不改动 G2 证据 |
| D-6 | 审核人意见（OQ-06…OQ-13）不阻塞推进，但建议在 G3 重新批准前收回以避免返工 |

## 10. 签署

```
G3 决策：        DEFER
approved route： （保持空）
路线处置确认：   R1 参考 / R2 候选暂停加码 / R3 暂停投入 —— 同意
§8 四项补证：    已知悉
GPU parity 状态： 已知悉为 UNVERIFIED，且非 G2 kill
```

**签署结果（2026-11-24）：** `DEFER`，`approved route` 保持空 —— 由负责人在 WorkBuddy 会话中明确批准，AI 转录归档（见文首「决策记录」）。
