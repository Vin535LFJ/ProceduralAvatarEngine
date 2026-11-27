# 负责人视觉验收包 — PAE 基础人体模板（Host 证据）

日期：2026-11-27
任务：PAE-CONSOLIDATION-02 §10 起草；03 轮记录负责人决定
状态：**HUMAN_TEMPLATE_BASELINE = ACCEPTED_FOR_VR1B**（转录，非代签）

> 记录依据：负责人既往反馈"这次感觉比较成功了，像个人了"；2026-11-27 会话指示按
> PAE-CONSOLIDATION-03 §三/§六/§十三 正式记录。含义与边界见
> `docs/phases/P1-visual-recovery/15-vr1b-wp0-decision-package.md` §3：
> 认可基础拓扑作为下一阶段模板起点；**不是**最终自然半写实成人视觉 Gate PASS；
> Android GLES3 渲染验证仍为 NOT RUN。下方逐图签署栏保留，供负责人补签细节意见。

## 1. 验收问题（只回答这一个）

> **这个基础人体拓扑，是否足以作为 PAE 后续"自然半写实成人"的起点？**

不是问"是否已经达到最终半写实成人质量"。中性基础网格只是起点，最终形象由后续 Target + PAE 形态参数 + 负责人视觉 Gate 决定。

建议回答格式：`ACCEPTED / REJECTED / ACCEPTED WITH CONDITIONS`，附一句理由。

## 2. 这些图是什么、不是什么

| 验证类型 | 状态 |
|---|---|
| Host 软件光栅器渲染（本包全部 18 张） | 已生成，可复现（staging 重建 101/101 逐字节一致） |
| 图像描述模型检查 | 做过，结论不可靠，**不构成本验收** |
| 负责人亲自查看 | **本包的目的，未做** |
| Android GLES3 真机验证 | 未做（VR-1B WP2 的内容，不是本包） |

本包验收**不等于** Android 视觉 Gate。Android 验收在 VR-1B WP8 单独做。

## 3. 已知缺陷（看图时请带着这些事实）

- **握拳不成立**：图 15 的手指屈曲约 700 个面翻面（基础网格手指环线太少 + 纯 LBS），与权重来源无关。VR-1B 建议声明不支持握拳（D9）。
- **肩外展皮肤拉伸**：图 14/18 大幅外展时 LBS 产生约 3 倍边拉伸；PAE 自生成权重更差（一条边 10 倍）。corrective/DQS 留到后续阶段。
- **足弓、腋窝偏"软"**：图 09/12 形态平缓，是基础网格的中性形态，不是拼接缝。
- 所有图无材质、无头发服装，灰模/线框是有意为之（不用外观掩盖几何问题）。

## 4. 逐图清单（18 张）

来源：`evidence/host-proof/`（链 A 84 张 + 链 B 17 张中的精选）。每张的 SHA-256 与来源映射见 `SHA256SUMS.txt`。

| # | 文件 | 看什么 | 负责人结论 |
|---|---|---|---|
| 01 | base-clay-front | 正面全身比例、肩胸腰髋过渡 | ☐ |
| 02 | base-clay-back | 背面、脊柱沟、肩胛区域 | ☐ |
| 03 | base-clay-side-left | 左侧面轮廓（胸/腹/臀曲线） | ☐ |
| 04 | base-clay-side-right | 右侧面（与 03 对称性） | ☐ |
| 05 | base-clay-front-45-left | 45° 立体感 | ☐ |
| 06 | base-clay-back-45-right | 背 45° | ☐ |
| 07 | base-wire-front-45-left | 四边形环线走向：肩、腋、髋、指根 | ☐ |
| 08 | closeup-shoulder-L-1 | 肩部环线连续、无拼接缝 | ☐ |
| 09 | closeup-axilla-L-1 | 腋窝形态（偏软是已知事实） | ☐ |
| 10 | closeup-face-0 | 眼眶/口部环线、五官结构合理性 | ☐ |
| 11 | closeup-hand-L-0 | 五指独立、指根拓扑 | ☐ |
| 12 | closeup-foot-L-1 | 足弓、脚趾 | ☐ |
| 13 | closeup-hip-groin-L-1 | 髋/腹股沟过渡 | ☐ |
| 14 | pose-shoulder-abduction-L-mh-front-45-left | 肩外展 70°：方向正确、皮肤拉伸程度可否接受为起点 | ☐ |
| 15 | pose-finger-curl-L-mh-front-45-left | 握拳破面（已知缺陷，确认"声明不支持"是否可接受） | ☐ |
| 16 | pose-elbow-flexion-L-mh-front-45-left | 屈肘 90° 方向与形态 | ☐ |
| 17 | base-front-clay（链 B） | 独立实现交叉印证：与 01 同一基础网格 | ☐ |
| 18 | both-abduct-front-clay（链 B） | 双臂外展，左右独立 | ☐ |

完整 101 张在 `../evidence/host-proof/images/`（84）与 `../evidence/host-proof/images-proof-b/`（17），含全部 target 变体、三套权重对比、7 类动作 × 3 权重来源。本包是精选，不是全部。

## 5. 签署

```text
验收人：________________
日期：________________
结论：ACCEPTED / REJECTED / ACCEPTED WITH CONDITIONS
理由与附加条件：________________
```

签署后请把结论记入 `docs/phases/P1-visual-recovery/15-vr1b-wp0-decision-package.md` 的对应条目；本包保持原样作为验收对象存档。