# VR-1A-R1 实现与专家决策摘要

日期：2026-11-25
状态：**`IMPLEMENTED / TECHNICAL CHECK PARTIAL / VISUAL BLOCKED`**

## 一句话结论

双侧躯干—肩部连续网格已经在 Android mode-2 真实运行，肩胸裂纹、winding 黑三角和 bind-pose 自交已修复；但结果仍是平坦肩盖与圆管感明显的局部灰模，不能进入自然人体视觉验收或 VR-1B。

## 已完成

- 单一语义链：`CharacterDefinition → ResolvedCharacterState → GeometryBuildRequest`。
- 双侧躯干、肩带、腋窝、上臂根部连续生成。
- 共享 indexed 边界、RMF sweep、局部形态基、Top-4 区域权重。
- Android mode-2 局部相机、Clay/Wireframe/Normal/Weight/Material ID。
- cache/quality/Skeleton/BodyJoints/MotionRuntime 恢复。
- 20/20 Host CTest。
- 最终 APK 构建、安装和八视角/近景/动作真机证据。

## 关键技术结果

```text
524 vertices / 1,028 triangles
components=1 boundary=0 non-manifold=0
degenerate=0 invalid_normals=0
same-direction edges=0
duplicate_faces=0 self_intersections=0
slivers=6
seam max=114.520° p95=83.131° (OQ-03 NOT APPLICABLE)
```

最终 APK SHA-256：

```text
1294761ad6be8d0ac33b2798c026f9b0dd43030dc50d7ea2d4cb0d83e400c90c
```

## 真机性能与内存

- Generator total：7.484 ms。
- State / geometry / assembly / weight：0.176 / 1.465 / 4.983 / 0.808 ms。
- GPU mesh / wireframe upload：2.512 / 6.618 ms。
- Steady mode-2：约 49.1 FPS，P50 20.11 ms，P95 22.02 ms，P99 22.48 ms。
- CPU mesh：50,064 B；GPU buffer accounted：51,472 B。
- Peak RSS：287,828 KB；TOTAL PSS：209,167 KB。

旧 P1-A baseline 不含本轮双侧几何、权重和完整拓扑验证，工作量不等价，因此 20% guardrail 保持 `NOT VERIFIED`，不伪称 PASS。

## 视觉判断

已通过：

- 肩胸无可见裂缝。
- backface culling 开启时无黑三角。
- Normal 视图无 winding 黑洞。
- 局部 patch 占视口约 55–65%，无脱离的全身地面阴影。

仍受阻：

- 肩盖上表面过平。
- clavicle/acromion/deltoid 体积层级不足。
- 腋窝凹面偏小，动作时肩胸牵引偏硬。
- seam 法线仍突变，近景折面明显。
- 整体仍读作“无头躯干 + 肩盖 + 圆管”。

## 未验证项

1. 同一 APK Legacy P1-A vs VR-1A：`NOT VERIFIED`。
2. 同工作量 20% 性能 guardrail：`NOT VERIFIED`。
3. 自然成人视觉与动作质量：`BLOCKED`。
4. head/hand/foot/full Canonical/VR-1B：未进入。

## 请专家重点决策

1. 下一轮是否只做“肩带体积与 seam 法线”修复，不扩大身体范围？
2. clavicle/acromion relief 与 axilla support 的优先级和参数方向。
3. 是否接受把 seam max/p95 作为研究诊断继续观察，而不设硬阈值？
4. 如何建立旧/新同工作量性能 baseline，再决定 guardrail。
5. 负责人独立查看最终八视角和近景后，是否允许进入 VR-1B。

详细证据与诊断见 `05-vr1a-visual-and-geometry-review.md`；原始日志和截图见 `evidence/vr1a-r1/final-geometry/`。
