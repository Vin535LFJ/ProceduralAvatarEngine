# P1-A — 06 视觉缺陷报告（项目方判定）

- 日期：2026-11-23
- 判定来源：项目负责人原话“目前效果特别差了”。
- Gate 结论：**REJECTED / NOT ACCEPTED**。P1-A 视觉方向未通过，不得进入 P1-B。
- 处理决定：**保留当前最新代码、全部迭代截图与缺陷记录**，不回滚、不删除证据、不把缺陷包装成通过。

## 缺陷清单

| ID | 严重度 | 问题 | 证据 |
|---|---|---|---|
| P1A-DEF-001 | P0 | 整体形态离“精致、自然、协调的商业游戏角色”目标很远，当前更像局部几何研究件而不是可接受的卡通人体局部 | `evidence/clay_preset0.png`, `clay_preset2.png`, `clay_preset4.png` |
| P1A-DEF-002 | P0 | 躯干呈外扩筒状，胸腔/腹部/腰/骨盆的体积和比例没有形成可信人体轮廓；胸腹过渡生硬 | `evidence/clay_preset0.png`, `clay_preset7.png`, `normals_front.png` |
| P1A-DEF-003 | P0 | 肩胸连接仍有细暗线和硬边，顶部 rim 明显；没有达到“无可见断层”要求 | `evidence/clay_preset6.png`, `normals_shoulder.png`, `wire_shoulder.png` |
| P1A-DEF-004 | P0 | 上臂/前臂仍读作独立圆管，肘部动作有硬折痕；关节过渡不自然 | `evidence/clay_preset0.png`, `action_elbow_front.png`, `action_elbow_shoulder.png` |
| P1A-DEF-005 | P1 | 上下端为开放边界，局部 PoC 可解释，但视觉上加重“截断筒体”观感 | 全部 `clay_preset*.png` |
| P1A-DEF-006 | P1 | 肩部附近线框/法线出现不均匀与折叠感，局部法线连续性不足 | `evidence/wire_shoulder.png`, `normals_shoulder.png` |
| P1A-DEF-007 | P0 | 工程指标（拓扑/性能/测试）合格，但无法抵消视觉质量不达标；不能以三角形数、Shader 或测试通过证明几何质量 | `evidence/ctest-final.txt`, `evidence/logcat-p1a.txt` |

## 可能原因（待后续 PoC 验证，不作为已确认结论）

1. 躯干仅用 5 个截面与单环 loft，无法表达胸大肌、腹直肌、骨盆与肋弓的体块差异。
2. 肩胸 bridge 是拓扑正确的条带，但在视觉上仍是“贴上去的过渡片”，并非从躯干体块自然长出。
3. 上臂/前臂虽使用非圆 profile 与肌腹调制，但仍是闭合环扫掠，缺少前后肌群分离与真实肩袖体积。
4. 当前权重与法线使关节动作保持连续，但关节折痕说明变形质量仍不足。
5. 缺少头颈、手、髋腿会显著影响整体比例判断，但 P1-A 范围不允许用这些内容掩盖局部几何缺陷。

## 保留内容

- 代码：`native/procedural/character/*`、`native/renderer/RenderThread.*` 的 previewMode=2、JNI/Kotlin 调试入口。
- 提交：`759c9d6 p1a: surface-patch geometry PoC with device evidence`。
- 证据：全部 `evidence/` 图片与日志，包括迭代过程 `iter2_*`、`iter3_*`，用于说明缺陷是如何逐步暴露与部分修复的。
- 结论：P1-A **REJECTED**，仅作为失败 PoC 存档。

## 若后续继续

只能在 P1-A 内重做几何方案，优先顺序：
1. 重做躯干截面与解剖 landmark 映射，先解决整体轮廓与腹胸体积；
2. 重做肩胸 patch，让肩部成为躯干体块的一部分而不是贴片；
3. 上臂/前臂做前后肌群分离与肘部过渡；
4. 同样固定相机、灰模、拓扑与真机证据重新提交 Gate。

不得在 P1-A 视觉 Gate 通过前扩展到 P1-B。
