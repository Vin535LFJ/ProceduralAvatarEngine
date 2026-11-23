# P1-A — 04 Test & Acceptance

## 工程验证

| 项目 | 结果 | 证据 |
|---|---|---|
| 宿主测试 | PASS | `evidence/ctest-final.txt`：9/9 套件通过（新增 `pae_character_patch_tests`） |
| 新几何合法性 | PASS | `p1a_patch_mesh_is_valid_and_connected`：MeshValidator 通过；参数变化真实改变胸宽；权重和=1、索引/法线合法 |
| 拓扑 | PASS | 真机 `evidence/logcat-p1a.txt`：`verts=870 tris=1664 components=1 boundary=80 nonmanifold=0 degenerate=0 badnorm=0` |
| Boundary 契约 | PASS | 4 条 boundary 均为共享索引，C0 位置偏差=0、G1 法线偏差=0；80 条边界边为局部 PoC 的显式开放缝 |
| 生成耗时 | PASS | 真机 `gen_ms=2.419`；host 报告约 0.5–0.6ms |
| Android GLES3 真机 | PASS | `gradle-assemble-final.txt` BUILD SUCCESSFUL；设备安装成功；GLES 3.2 / Mali-T860 |
| 性能 | PASS | `logcat-p1a.txt`：draw_calls=2，triangles=1664，约 46–55 FPS，P50≈20.5–20.8ms / P95≈23–24ms / P99≈30ms |
| 固定相机证据 | PASS | `clay_preset0..7.png`（gray Lambert），`wire_front/shoulder.png`，`normals_front/shoulder.png`，`action_elbow_front/shoulder.png` |
| 与 M11/M12 同机位对照 | PASS（证据已提交，结论待用户） | P1-A `clay_presetN.png` 与 `docs/phases/M12-unified-human-geometry/evidence/dv3_presetN.png` 使用相同 preset 0..7 |
| 关节动作 | PARTIAL | 屈肘动作可见连续变形；动作前后仍有可见局部折痕/接缝，见下 |
| 用户视觉 Gate | **REJECTED** | 项目负责人判定“目前效果特别差了”；缺陷清单见 `06-visual-defect-report.md`；未进入 P1-B |

## 视觉自评（不代替用户 Gate）

已消除：初版肩胸桥接绕到身体另一侧造成的大三角膜、明显黑洞、肘部硬权重断层。

仍存在、需用户裁决的缺陷：
1. 肩部上方仍有一条细暗线，对应局部开放缝/顶部 rim；尚未达到“完全无痕”。
2. 上臂虽有肱二头肌/肱三头肌与肘部截面调制，但局部仍带一定圆管感。
3. 躯干上下端为开放边界（PoC 局部范围），不是完整人体靴形。
4. 屈肘时肘部有轻微折痕，虽无断裂，但尚未达到商业成品的顺滑过渡。

未使用增加三角形数量、Toon 材质或 Shader 复杂度作为几何质量证据：P1-A 使用连续 gray Lambert，1664 三角形，2 个 draw call。

## 迭代记录

1. 初版肩桥接到错误半侧 → 大三角膜；修正 torso arc 索引方向。
2. 初版桥接面朝向错误 → 翻转 winding。
3. 单侧桥仍有暗线 → 增加对侧桥闭合顶部 rim 与臂根环。
4. 肘部硬权重断层 → 改为 smoothstep 过渡。
5. 圆管感 → 加入沿程宽深变化、胸部前突、前/后肌腹与肘部凹槽调制。
