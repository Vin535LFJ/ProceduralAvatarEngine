# 路线审核提示词（GATE-03 结果 → G3 投资决策输入）

> 把下面「提示词正文」整段复制给审核人。**不要**把本报告结论当成已批准事实：
> 审核人的任务是独立证伪，不是确认。

---

## 提示词正文

你是**独立的几何路线审核人**。请对下面这次 PoC-A 路线公平比较做独立复核，并给出是否可以把某条路线提交给负责人做 G3 签署的建议。你不受本次执行者的结论约束；发现报告错误、测量口径作弊或证据不足时，必须直接指出。

### 0. 环境与复现

- 仓库：`https://github.com/Vin535LFJ/ProceduralAvatarEngine.git`
- 分支：`feature/procedural-character-foundation`
- 提交：`a32251b`（`poc-a: G0/G1/G2 route-comparison evidence and sign unification`）
- 基线：`99941a9`
- 在隔离 worktree 中构建与复现：

```bash
cmake --build build-host --parallel 4
ctest --test-dir build-host --output-on-failure                 # 应 11/11
./build-host/pae_tests/poc_fair_comparison_probe.exe evidence/g2
```

探针会重新生成 `evidence/g2/fair-comparison.md` 和 `evidence/g2/clay/`。请对比重生成结果与仓库中的文件是否一致。

### 1. 背景

- 比较对象：**R1** = SDF + Surface Nets（参考，无固定拓扑）；**R2** = 固定 quad 模板投影到解剖 SDF；**R3** = 固定 quad 模板架在三次 B-spline 包络控制面上。
- PoC-A 范围：T1 躯干规范拓扑 + T2 肩根边界/连接拓扑。T3 肩部形变环、T4 肘关节环、T5 四肢拓扑**明确延后**，不要用它们否决本次结果。
- 参数组：Baseline / Chest Width-Depth / Waist-Pelvis Ratio；三组用同一 Shape Model、同一骨架、同一采样、同一相机。
- 门禁：G0 → G1 → **G2（本次）** → G3（负责人签字选路线）→ G4（全身扩展）。**AI 不得自批 G3**。

### 2. 按顺序阅读证据

1. `evidence/reports/g2-fair-comparison-report.md` — 验收报告与门禁状态
2. `evidence/g2/fair-comparison.md` — 全部指标表（E1/E2/E3、Layer A/B、7 视角、§13 基线）
3. `evidence/g2/clay/index.md` — 63 张灰模图（3 组 × 3 路线 × 7 视角），请**逐张抽查**而非只看索引
4. `evidence/g2/smooth-union-sign-derivation.md` — 符号统一的数学推导
5. `evidence/reports/g1-route-feasibility-report.md`，**特别是 §12 勘误**（T2 后拓扑计数已变）
6. `tests/unit/poc_routes_tests.cpp`、`tests/unit/poc_fair_comparison_probe.cpp`、`tests/unit/smooth_union_sign_tests.cpp`

### 3. 执行者给出的关键主张（请逐条独立验证，不要直接采信）

- Baseline 组：R2 的 E3 mean **0.00149 m**，R3 为 **0.03211 m**，R2 领先 **21.5×**。
- R3 的 **E2 反而更低**（0.00040 vs R2 的 0.00098），所以 R3 的误差在**包络曲面**，不在重拓扑投影。
- R1/R2 的 E1、landmark、截面偏差为 0 是**自比**（共用同一连续 SDF），**不是质量分**。
- DEF-02 曾使两路线坐在不同骨架上（腕 x 差 0.22 m），修正后结论方向反转；请确认回归测试 `routes_share_one_skeleton` 真的封住了这个问题。
- 三路线拓扑均 components=1、non-manifold=0、无未登记裂缝；R2/R3 的 32 条边界是**已登记**肩根 crop。
- 肩根接缝最大二面角 R1/R2 47.66°、R3 16.13°，均**超过合同值 ≤5°**（这是真实缺陷，不是 TBD）。
- 权重全部合法、CPU 变形正确；**GPU 数值 parity 仍 `[UNVERIFIED]`**，按 SPEC §11.1 在 G2 不是 kill。

### 4. 你必须回答的问题

1. **归因是否成立**：E1/E2/E3 是否真的按 SPEC §8.2 分离？"R3 的误差在曲面不在投影"这个判断，有没有被采样口径、参考面定义或网格离散误差污染？
2. **公平性**：逐项核对 SPEC §8.1 的 8 项对称性（Target Reference / Sample Distribution / Sample Count / Distance Metric / Normal Metric / Landmark Set / Coordinate System / Parameter Set）。哪一项实际没有做到？
3. **自比陷阱**：报告把 R1/R2 的若干指标标为 0 并解释为自比。这个解释是否诚实？有没有把本该有区分度的指标设计成了恒等式？
4. **拓扑与 T2**：`TopologyAudit` 区分已登记 crop 与未登记裂缝的逻辑是否可靠？肩根是不是"连续解剖连接"而不是独立肩球或圆管？crop 高度是否落在胸廓标志点？
5. **接缝合同失败**：≤5° 是合同已有阈值。它是否构成阻塞项？如果 R2 与 R3 都失败，这条失败是否应该改变路线推荐？
6. **视觉 Gate**：独立看 63 张灰模图（尤其 front / side / front-45）。你看到的形态缺陷是否与数值表一致？有没有数值没抓到的一票否决缺陷？
7. **§13 阈值**：列出哪些指标仍是 `TBD`、哪些用了合同值。报告有没有把 TBD 指标写成 PASS 或暗示通过？
8. **kill 清单**：按 SPEC §6.3 的 G2 触发项逐条给出 CLEAR / FAIL / NOT VERIFIED。注意：GPU parity 未测**不是** G2 kill。
9. **代码审阅**：`SdfFusion.cpp` 的符号修正是否推导正确？`SkinWeights.cpp` 的 PoC 权重赋值器是否真的等价于生产实现（测试声称 0/5108 不一致）？灰模渲染器是否与数值相机协议共用同一 `viewFrame()`？
10. **推荐**：你是否同意把 **R2** 作为提交负责人签署的候选路线？如果不同意，给出替代方案和必要条件。

### 5. 输出格式（严格遵守）

```
结论: APPROVE_ROUTE | REJECT_ALL | DEFER
推荐路线: R1 / R2 / R3 / 无
置信度: 高 | 中 | 低
------------------------------------------
1. 归因复核: <通过 / 不通过 + 理由>
2. 公平性缺口: <逐项列出>
3. 自比陷阱复核: <结论>
4. 拓扑/T2 复核: <结论>
5. ≤5°接缝失败的处理意见: <阻塞 / 非阻塞 + 理由>
6. 视觉 Gate: <你的独立判断，列出具体图片>
7. §13 阈值: <TBD 清单 + 有无违规判 PASS>
8. kill 清单: <逐项 CLEAR/FAIL/NOT VERIFIED>
9. 代码问题: <发现的错误或遗漏>
------------------------------------------
独立复现: <你实际跑的命令 + 结果，未跑写 NOT RUN>
必须修复项（阻塞签署）:
可接受风险:
给负责人的一句话建议:
```

### 6. 硬约束

- 不得把 `TBD` 阈值判为 PASS；未执行项写 `NOT RUN`，不预写结论。
- 不得用顶点数 / 三角形数冒充质量排名（只能作为成本）。
- 不得因为 GPU parity 未测而判 G2 kill；但要指出它在 G4 必须补。
- 不得把 R1/R2 的 0 值当作质量优势。
- 你自己的结论也只是**建议**；最终 G3 必须由负责人签字，任何 AI 或审核人都不能代签。
