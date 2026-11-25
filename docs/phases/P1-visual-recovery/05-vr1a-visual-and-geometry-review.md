# VR-1A-R1 视觉与几何复核报告

日期：2026-11-25
范围：`P1-VR-1A-R1 — Bilateral Torso-Shoulder Vertical Slice`
分支：`poc-sm/shape-model-v2`
授权：`AUTH-P1-VR-01 = GRANTED`
当前状态：**`IMPLEMENTED / TECHNICAL CHECK PARTIAL / VISUAL BLOCKED`**

本报告不宣布 Gate PASS，也不宣布人体视觉验收通过。它记录本轮实际实现、真机结果、仍存在的视觉问题和下一轮决策依据。

## 1. 状态与范围结论

已完成：

- `CharacterDefinition → ResolvedCharacterState → GeometryBuildRequest` 单一语义链。
- 双侧躯干、肩带、腋窝和上臂根部的连续 indexed mesh。
- 非圆闭合截面、variable-profile sweep、RMF frame、局部形态基、共享边界和区域权重。
- Android mode-2 局部相机、Clay/Wireframe/Normal/Weight Heatmap/Material ID。
- mode-2 cache、quality refresh、skeleton/joints/MotionRuntime 恢复。
- Host 专项测试、完整 CTest、Android APK、八视角和动作证据。

未完成或不能宣称：

- 自然半写实成人视觉验收。
- 同一 APK 内 Legacy P1-A 与 VR-1A 的受控对比；按负责人选择保持 `NOT VERIFIED`。
- 与旧 P1-A 完全同工作量的性能 guardrail 判定；旧 baseline 不含本轮双侧几何、权重和拓扑验证，不能伪称等价。
- head/hand/foot/full Canonical/VR-1B；本轮未进入这些范围。

## 2. 实际实现

主要新增模块：

- `native/procedural/character/Anthropometry.*`
- `native/procedural/character/AnatomicalLandmarks.*`
- `native/procedural/character/AnatomicalRegions.*`
- `native/procedural/character/ResolvedCharacterState.*`
- `native/procedural/character/GeometryBuildRequest.*`
- `native/procedural/character/TopologyTemplate.*`
- `native/procedural/character/SurfacePatch.*`
- `native/procedural/character/VariableProfileSweep.*`
- `native/procedural/character/LocalShapeBasis.*`
- `native/procedural/character/PatchAssembler.*`
- `native/procedural/character/TorsoShoulderPatchGenerator.*`
- `native/procedural/character/LodCorrespondence.*`
- `native/procedural/character/RegionWeightGenerator.*`

生成器最终采用：

- 11 个躯干环、每环 32 段。
- 躯干侧面真实臂根开口，而不是把内部肩胸交界伪装成 crop boundary。
- 每个开口 12 个共享顶点。
- 开口到外侧肩部过渡环，再到 6 层上臂 sweep。
- 首环相位与 RMF frame 对齐，避免环间扭转。
- 全局 winding 修复、signed-volume outward orientation、法线重算和 Top-4 权重生成。

最终 mesh：**524 vertices / 1,028 triangles**。

## 3. 裂纹、Normal 异常和折叠的根因与修复

### 3.1 为什么 `components=1 / boundary=0 / non-manifold=0` 仍会看起来断裂

这些指标只说明 indexed topology 的引用关系闭合，不说明空间几何连续。以下问题仍可同时满足上述三个条件：

1. 相邻三角形绕共享边方向不一致，开启 backface culling 后出现黑三角或孔洞。
2. 两个独立环的顶点对应顺序扭转，quad strip 自身折叠。
3. 完整上臂管从躯干内部开口开始，管壁与胸廓或肩盖相交。
4. 表面虽然索引连通，但法线在 seam 两侧突变，Normal 视图出现异常色带。
5. Material ID 或权重区域在低分辨率网格上形成锯齿状视觉分界。
6. 空间位置重叠的自交三角形不会被 indexed boundary 检测捕获。

因此本轮同时使用 indexed topology、spatial weld、directed-edge winding、seam normal、duplicate/sliver/self-intersection 和真机 culling 诊断。

### 3.2 已修复

- 修正 quad 三角连接，消除错误 diagonal。
- 修正左右臂根与 `+X/-X` 侧别对应，避免两臂路径横穿躯干。
- 移除放置在躯干顶面、导致上臂管穿入躯干的臂根开口。
- 改为躯干侧面开口 + 向外肩部过渡 + 上臂 sweep。
- 对齐 root loop 与 RMF 首环相位，消除环间扭转。
- 使用 flat adjacency 的 winding BFS 和 signed-volume 外向修正。
- 最终 bind-pose self-intersection 从 595 降至 **0**。

## 4. Host 几何验证

最终专项诊断：

```text
VR1A-TOPO verts=524 tris=1028 comps=1 boundary=0 nonmanifold=0 degen=0 badnorm=0 weights=1
VR1A-WINDING same_direction_edges=0 opposite_direction_edges=1542
VR1A-DIAG indexed comps=1 boundary=0 nonmanifold=0 degen=0
edge1=0 edge2=1542 edge3+=0
welded_verts=524 welded_comps=1 welded_boundary=0 welded_nonmanifold=0
duplicate_faces=0 slivers=6 self_intersections=0
seam_max_deg=114.520 seam_p95_deg=83.131
left_verts=112 right_verts=113 registered_boundaries=4 oq03=NOT_APPLICABLE
```

解释：

- indexed 与 `1e-4 m` spatial weld 均保持一个封闭连通分量。
- 无 boundary、non-manifold、degenerate、invalid normal、duplicate face 或 bind-pose self-intersection。
- 6 个 sliver 仍作为研究诊断保留，不以增加密度隐藏。
- seam 角度远高于 5°，说明拓扑连续但法线/曲率过渡仍不自然；这正是视觉仍受阻的技术原因之一。
- OQ-03 按负责人决定暂缓硬阈值，本轮不把 5° 当 PASS/FAIL。

完整 CTest：

```text
20/20 Test passed
Total Test time = 145.48 s
```

证据：`evidence/vr1a-r1/final-geometry/ctest-full-final.log`。

## 5. 参数、权重和动作验证

Host 测试确认：

- 胸廓宽度和胸廓深度可独立测量。
- waist pinch 只改变腰部轮廓。
- 左右肩宽、上臂半径和 asymmetry 输入可独立生效。
- 腋窝 concavity basis 有局部支撑和实际位移。
- RMF orthonormality/handedness/tangent 诊断有效。
- Stable ID 可重复生成，LOD identity/mismatch 和 cache key 可区分。
- 所有顶点 BoneId 在 0–21，Top-4、非负、权重和 `1±1e-3`。
- 左/右肩和上臂权重没有跨侧串扰。
- Weight Heatmap 使用实际 `joints/weights/color`，不是伪造颜色。

真机动作证据覆盖 Bind、左臂抬起、右臂抬起和双臂外展。双臂外展时肩胸牵引仍显得偏硬，腋窝体积和肩带参与不足，因此动作视觉不能判为自然人体通过。

## 6. Android 视觉证据

最终 APK：

```text
SHA-256 1294761ad6be8d0ac33b2798c026f9b0dd43030dc50d7ea2d4cb0d83e400c90c
```

设备：`nunai_max`，Android 8.1，1920×1008 Surface。

最终证据目录：`evidence/vr1a-r1/final-geometry/`。

已采集：

- Front、Back、Left、Right、45° Left、45° Right。
- Shoulder/Chest Close-up、Axilla Close-up。
- Clay、Wireframe、Normal、Weight Heatmap、Material ID。
- Bind、左臂抬起、右臂抬起、双臂外展。
- culling on/off 诊断对比。

观察结果：

- 原肩胸裂缝和黑色三角在最终 culling-on Clay 视图中消失。
- Normal 视图中不再出现由错误 winding 造成的局部黑洞。
- Weight Heatmap 显示连续但仍有硬边感的肩/胸/臂权重过渡。
- mode-2 局部 patch 约占视口高度 55–65%，脱离的 full-body ground shadow 未再出现。
- 视觉仍读作“无头躯干 + 平坦肩盖 + 圆管”，不是自然半写实成人。主要原因包括：
  - 肩盖顶部仍过于平坦。
  - clavicle/acromion/deltoid 的体积层级不足。
  - axilla concavity 偏小且局部支撑过窄。
  - 胸部正背面、腰部和骨盆的体积比例仍缺少真实人体细节。
  - 低多边形 shoulder transition 在近景仍可见折面。

因此状态保持 **`VISUAL BLOCKED`**。

## 7. Backface Culling 与 Material ID 诊断

- 最终代码保持 `glEnable(GL_CULL_FACE)` 和 `glCullFace(GL_BACK)`。
- 临时 culling-off APK 仅用于诊断，未作为最终修复。
- Front、Shoulder/Chest、Axilla 的 culling on/off 图像未出现额外孔洞或黑三角，说明最终连续性不是依赖关闭 culling。
- Material ID 作为可选附加诊断保留；其边界锯齿主要来自低分辨率区域分类，不能替代 Clay/Normal/Weight 判断。

证据：`evidence/vr1a-r1/final-geometry/culling-off-diagnostic/`。

## 8. Cache、质量刷新和模式回归

已验证：

- mode-2 Low/High quality 会按 cache key 失效并重建。
- mode-2 恢复 matching `Skeleton/BodyJoints` 并重新初始化 `MotionRuntime`。
- 0→1→2→3 在 Canonical cache 已建立时完成切换。
- Canonical cold start 和 Surface recreation 后仍可渲染。
- mode-3 Canonical 几何、权重、UV 和 GPU 路径未修改。
- mode-3 最终证据仍为完整 Canonical 灰模，不把局部 VR-1A 当作全身替代。

限制：

- 从“冷启动直接进入 mode-2、此前从未生成 mode-3”的状态首次切入 mode-3，需要重新建立 Canonical GPU/cache，不能把这一路径冒充为已验证的同 cache 热切换。
- 本轮按授权没有重写 mode-3 几何或新增 JNI/UI 入口。

## 9. 性能、上传和内存

最终真机冷启动分项：

| 项目 | 实测 |
|---|---:|
| State resolution | 0.176 ms |
| Geometry generation | 1.465 ms |
| Winding/topology assembly | 4.983 ms |
| Weight generation | 0.808 ms |
| Generator total | 7.484 ms |
| GPU mesh upload | 2.512 ms |
| Wireframe upload | 6.618 ms |
| CPU mesh accounted bytes | 50,064 B |
| GPU buffer accounted bytes | 51,472 B |
| Process peak RSS | 287,828 KB |
| Current TOTAL PSS | 209,167 KB |

稳态 mode-2：

```text
frames=360 fps=49.1 draw_calls=2 triangles=1028
p50_ms=20.11 p95_ms=22.02 p99_ms=22.48
```

参考 baseline：

- 旧 P1-A mode-2：870 vertices / 1,664 triangles，历史生成约 2.369 ms，且有 80 个 boundary。
- P1 full Canonical：45.5–46.9 FPS、p95 23.22–24.32 ms、TOTAL PSS 244,785 KB。

guardrail 结论：

- 稳态 FPS/frame time 和当前 PSS 与历史参考相比没有明显超限。
- 但旧 P1-A baseline 不含本轮双侧几何、Top-4 权重生成和完整 winding/topology 验证，工作量不等价；本轮 Generator total 7.484 ms **不能**据此宣称 20% guardrail PASS。
- 同工作量性能 guardrail 保持 `NOT VERIFIED`，下一轮应先建立可复现的旧/新同配置 baseline，再作正式判定。

## 10. 授权、Git 和未验证项

授权边界：

- 仅 VR-1A-R1 白名单范围。
- 未修改 22 骨骼、`UBO[22]`、64-byte vertex packing、JNI ABI。
- 未修改 head/hand/foot/full Canonical 生成器。
- 未修改 `AGENT.md`、ADR、DEC、CAS、Gate 文件。
- 未清理或纳入 `.workbuddy-ai/`、`evidence/character-v6/**`、`evidence/character-v7/**`。

未验证项：

1. Legacy P1-A vs VR-1A 同一 APK 对比：`NOT VERIFIED`。
2. 同工作量 20% 性能 guardrail：`NOT VERIFIED`。
3. 自然人体视觉、seam 法线连续性和肩带体积：`BLOCKED`。
4. 更高密度 shoulder topology 的 LOD0↔LOD1↔LOD2 完整对应：本轮只验证 identity/mismatch/cache key，未进入 VR-1B。
5. 动作中的体积保持、腋窝塌陷和局部 corrective：未达到视觉接受标准。

## 11. 专家复核要点与下一轮建议

本轮可确认的结论：

- 数据层和双侧生成链已经真实运行，不是文档或 Host 玩具。
- indexed/spatial-weld topology、winding、权重和 bind-pose self-intersection 已通过。
- 肩胸裂缝和 culling 黑三角已经修复。
- Android mode-2 局部预览和调试视图可用。

本轮不能确认的结论：

- 视觉已经接近自然成人。
- seam 法线已经连续。
- 性能 guardrail 已通过。
- 可以进入 head/hand/foot/full Canonical。

下一轮建议只聚焦一个目标：**让肩带在 Clay 和 Normal 视图中先读成人体肩带，而不是平坦肩盖和圆管拼接。** 优先顺序：

1. 用 clavicle/acromion 局部 relief 重塑肩盖上表面。
2. 增大并平滑 axilla concavity 支撑，减少动作时腋窝拉扯。
3. 提高 shoulder transition 的环数和截面控制，但不用于掩盖 seam。
4. 使 ShoulderGirdle/UpperArmRoot 权重随 clavicle 和 deltoid 区域连续参与。
5. 建立旧/新同配置、同工作量的性能 baseline 后再判定 guardrail。
6. 由负责人独立查看八视角和近景，决定是否进入 VR-1B；AI 不代签视觉 Gate。
