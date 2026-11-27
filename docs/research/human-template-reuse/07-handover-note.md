# 交接说明 — PAE 人体模板路线（P1-VR-ARCH-1）

日期：2026-11-26
读者：项目负责人 / 外部专家
状态：建议 **CONDITIONAL GO**，判定权在负责人。本文只做交接，不含新结论。

---

## 1. 两个分支的现状

两个目录是**同一个仓库**的两个 worktree。

| | 主仓 | poc-sm 工作树 |
|---|---|---|
| 路径 | `C:\Users\Giant\Desktop\GITCODE\Procedural Avatar Engine` | `E:\AI-Tools\ProceduralAvatarEngine-poc-sm` |
| 分支 | `feature/procedural-character-foundation` | `poc-sm/shape-model-v2` |
| HEAD | `2ac101f` | `a0783a2` |
| 与远端 | 一致 | 一致（`ls-remote` 已核实） |
| 已跟踪文件改动 | 0 | 0 |
| 本轮成果 | 无 | **全部在这里** |

### 提交差异

- 共同祖先：`a32251b`
- **仅在 poc-sm**（16 笔）：PoC-SM 竖切 → VR-1A-R2 → 本轮 `0baa698` 工具 → `a0783a2` 文档与证据
- **仅在 feature**（11 笔）：G3 DEFER 决策记录、ADR-011/012 签署、AUTH-08 执行记录、ADR-013 采纳
- 两棵树整体 diff：**727 文件，+113,762 行 / −551 行**

> 更正一处早前说法：治理文件 `AGENT.md`、`docs/ADR_INDEX.md`、ADR-013、`docs/project-charter.md`
> 在两分支上**逐字节相同**（feature 的 `2ac101f` 已由 `8d1377e` 同步到 poc-sm）。
> 真正双边都改过的只有 **35 个文件**：`native/` 20、`app/` 4、`tests/` 2、`docs/` 8、`.gitattributes` 1。
> 合并冲突面因此比早前估计的小，但 `native/` 下的代码两边都动过，仍需逐文件裁决。

### 两分支尚未合并

合并是 727 文件、11 万行的改动，且会触及 `native/` 代码，**本轮未做**。成果单点落在
`poc-sm/shape-model-v2`：另一条验证链依赖的 `CanonicalSkinGenerator.h` 只存在于该分支，
主仓分支无法原地重建。

---

## 2. 专家评审清单

入口：`docs/research/human-template-reuse/00-README.md`（分支 `poc-sm/shape-model-v2`）

### 必读三份

| 顺序 | 文件 | 看什么 |
|---|---|---|
| 1 | `docs/research/human-template-reuse/06-consolidated-review-and-status.md` | 现在到哪一步、两条链的复核对照、还差什么 |
| 2 | `docs/architecture/human-template/02-human-geometry-architecture-decision.md`（R7） | 路线建议、15 问回答、**ADR-014 草案** |
| 3 | `docs/phases/P1-visual-recovery/14-vr1b-base-topology-human-generator-plan.md`（R8） | 下一阶段计划与授权草案 |

### 支撑文档（`docs/research/human-template-reuse/`）

`01` 许可证与来源审计 · `02` 拓扑审计 · `03` Target 格式与参数映射 ·
`04` Rig V2 与三套权重对比 · `05` 实施报告（见 `docs/architecture/human-template/01-pae-human-template-format.md` 为模板格式定义）

### 证据（`docs/research/human-template-reuse/evidence/host-proof/`，127 个文件 / 3.40 MB）

- `images/` 84 张（链 A：host 软件渲染，含 12 组近景）· `images-proof-b/` 17 张（链 B）
- `logs/` 10 份日志原文 · `report/` 8 份自动生成报告 · `provenance/` 逐文件 SHA-256 + Git blob + 许可证状态
- `SHA256SUMS.txt` 完整性清单 · `tooling-manifest.md` 工具清单 · `reference-docs/` 3 份上游格式文档

**关键日志三行**

```text
pae_ht_tests    == summary: 65 passed, 0 failed, 65 total ==
template_proof  DIRECTION_CHECK actions=8 failed=0
pipeline        [done] 9200 ms  84 PNG files | PAE clay cross-check IoU 0.9617
```

### 工具（`tools/human-template-poc/host/`，29 个文件）

host-only，不被 Android 构建引用。链 A：`src/`（`pae_ht_tests`、`pae_ht_pipeline`）；链 B：`template_proof.cpp` + `render_proof.py`。

---

## 3. 仍待负责人拍板的三件事

1. **是否批准 ADR-014** —— 把「许可证明确、离线编译成 PAE 自有模板的拓扑与形态数据」列为例外。
   项目章程现写着「零外部 3D 美术资产」，ADR-004 现写着「不得改回外部模型」，11-25 的研究记录写的是 MakeHuman 网格「不导入」。
   **不修订，这条路线就只能停在电脑上的离线工具，进不了运行时。**（ADR-014 草案见 R7 附录；VR-1B 的 D1–D11 授权范围随此决定一并给。）
2. **`default.mhskel`、`default_weights.mhw` 能否使用** —— 两者只在文件内部声明 CC0，未被官方 `LICENSE.md` 逐项列举，需负责人或法务确认。
3. **人眼视觉验收** —— 至今没有人眼看过任何一张图。请打开 `evidence/host-proof/images/`（尤其 `closeup-*.png` 与 `pose-finger-curl-*`），判断「是否比 R1/R2 的拼接躯干更像完整的人」。

---

## 4. 已验证 / 未验证

**已验证（host）**：读取成熟拓扑 → Shape Target 改形态 → Rig V2 → Top-4 权重 + 回退 → 肩/肘/髋/膝基础动作 → 写出 PAE 自有模板并逐字节往返一致。两条互相独立的实现互相印证。

**NOT RUN**：Android 全链路（GLES3 / UBO / JNI / RenderThread / CPU-GPU parity）、姿势后的自相交、DQS 与 corrective、Biharmonic 权重、围度类 target 与宏参数组合、LOD、表面 Landmark。

**已知缺陷**：握拳时约 700 个面翻面（基础网格手指环线太少 + 纯 LBS），与权重来源无关；肩外展时 LBS 产生约 3 倍边拉伸。

**证据边界**：所有图都是 host 软件光栅器输出，不是 Android 渲染结果；「更自然」类判断此前只有图像描述模型给出，不可当人眼结论。
