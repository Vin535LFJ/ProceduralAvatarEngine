# PAE-P1 — 连续灰模身体、UV/GPU 与最终验收证据报告

日期：2026-11-25
状态：实现、Host 测试与真机技术验证完成；视觉验收仍未通过。依据负责人明确授权，本结果将提交并推送当前分支；提交与推送只是结果归档，不构成 Gate PASS，也不表示生产人物实现已获验收。

## 1. 结论摘要

P1 已在现有 C++17 / Android NDK / OpenGL ES 3.0 运行时内完成连续 Canonical 身体、Render Mesh、UV、GPU skinning、质量切换和 Surface 重建路径。最新安装 APK 的真机结果表明：

- High detail Canonical 为 57,652 vertices / 115,300 triangles / 345,900 render vertices。
- 拓扑、权重、法线、退化面、重复点和自交检查均无异常。
- 三组 GPU/CPU 位置与法线一致性案例全部通过。
- `2 → 0 → 2` 质量刷新、四模式切换和 Surface 重建均通过技术验证。
- UV Checker 在 Surface 重建前后像素 SHA-256 完全一致。
- 全量 Host CTest 为 19/19，Android `assembleDebug` 成功。
- 肩部比例和手腿分离已较此前改善，动作截图未出现明显跨肢体拉扯。

但当前结果仍是光滑、对称、无材质的灰模人台：五官不够自然，手指呈手套状，脚趾为模糊小突起，整体尚未达到自然半写实成人亚洲人物的视觉要求。因此视觉验收保持 **BLOCKED**。Git 提交与推送是负责人授权的结果归档，不表示 P1 或任何 Gate PASS。

## 2. 冻结约束与授权边界

本次工作保持：

- 22 骨骼，BoneId 0–21。
- GPU skinning `UBO[22]`。
- 旧 interleaved 64-byte vertex packing 与属性位置 0–4。
- 冻结 JNI ABI。
- C++17、Android NDK、OpenGL ES 3.0。
- 无新增运行时技术栈。
- Canonical 为一个 welded continuous shell，没有用断开肢体网格掩盖蒙皮问题。
- 所有视觉证据来自实际安装 APK 的程序化运行，没有使用现成人物模型、图片或预渲染视频替代生成结果。

未修改治理/授权文件，未 push、merge、rebase，未清理既有未提交工作。AUTH-08、DEC-05、CAS 和 Gate 状态不因本报告改变。

## 3. 实现范围

### 3.1 Canonical 与连续身体

主要实现位于：

- `native/procedural/humanoid/canonical/`
- `native/procedural/fusion/SdfFusion.cpp/.h`
- `native/procedural/humanoid/Humanoid.cpp`
- `native/procedural/humanoid/anatomy/`
- `native/procedural/humanoid/HeadAnatomy.cpp/.h`（受保护 P0 来源，当前仍未跟踪）

Canonical 由全局 smooth-union SDF 经 Surface Nets 提取，保持拓扑、焊接、权重和未来 corrective shape 的权威地位。手指和脚趾使用 digit capsules，并仅在对应局部 AABB 内参与场求值。Detail 0 不生成 digits，detail 1/2 生成 digits。

本轮修复包括：

- Surface Nets quad 过滤阈值改为 pair-local 尺度，不再依赖各向异性体积的最大轴或空白 margin。
- 分别评估两条 quad 对角线，选择有效且最小三角形面积更大的对角线。
- Canonical 在米空间单独清理退化三角形。
- 边界 hole 填充后同步修正 submesh `indexCount`。
- 权重按解剖区域限制，避免手臂权重拉到腿部、腿权重拉到手臂或对侧肢体。
- 肩关节保持在颈根以下，手臂自然下垂时与大腿外侧留出间隙。

### 3.2 Render Mesh、UV 与 GPU

- Canonical 不因 UV 拆点而改变拓扑。
- Render Mesh 对每个源三角形使用独立 UV chart，三个独立 render vertices。
- `renderToCanonical[renderVertexId] = canonicalVertexId` 保留接缝追溯。
- 独立 `vec2` UV VBO 使用属性位置 5，并有独立 UV VAO。
- UV Checker 复用 debug view 7，未改变 JNI 签名。
- GPU 验证使用 GLES 3.0 transform feedback，并以 `glMapBufferRange` readback。
- GPU 资源替换采用事务性提交；上传或验证失败时保留旧 base/UV 资源。

当前每三角形独立 chart 证明 UV 数据、拆点、映射和 GPU 路径可运行，但不是最终生产级 artist-friendly UV unwrap。Atlas 密度和利用率仍可优化。

### 3.3 渲染命令可靠性

`RenderThread::postExternalPose()` 原先在队列满时删除 `queue_.front()`，高速 pose 输入可能丢掉 `SurfaceCreated`、质量或预览模式等控制命令。现改为只淘汰过期 `SetExternalPose`：

- 控制命令不会被 pose 队列压力删除。
- 若队列中只有控制命令，则丢弃新 pose，而不丢控制命令。
- 最新 APK 的 detail-2 Canonical 冷启动已无早期控制命令丢失问题。

## 4. Host 测试与构建

| 检查 | 结果 | 证据 |
|---|---|---|
| Canonical / Skinning 专项 | 2/2 passed | `evidence/p1-final/host-focused-tests.log` |
| 全量 CTest | 19/19 passed | `evidence/p1-final/ctest-full-queue-fix.log` |
| Android `assembleDebug` | BUILD SUCCESSFUL | `evidence/p1-final/gradle-assemble-review-final.log` |
| `git diff --check` | 无 whitespace error | 本次工作树复核 |

新增回归覆盖：

- 各向异性体积下的 Surface Nets quad 过滤。
- submesh `indexCount == mesh.indices.size()`。
- 手/腿、对侧肢体和头部蒙皮隔离。
- 腿部动作不产生明显手部位移，手臂动作不产生明显大腿位移。

说明：当前工作树的全量测试通过，不等于 P1-only 提交范围已经独立构建通过。`ContinuousSkinGenerator.cpp` 仍包含受保护 P0 来源的 `HeadAnatomy.h`，P1-only 范围具有未解决的提交自包含性问题。

## 5. 最终 APK 与可复现数据

最终 APK：

- 路径：`app/build/outputs/apk/debug/app-debug.apk`
- 大小：60,500,525 bytes
- 本地与设备 SHA-256 均为：
  `6618b24f8bdb40b924bcd78e742a6bcf702f90b4623092d6c0c89feaa0aafdde`
- 设备路径：`/data/app/com.pae.host-E9xpBJgyRBMVE3FfKxbdDA==/base.apk`
- 证据：`evidence/p1-final/apk-sha256-review-final.txt`

最终 Host Canonical / Render 数据：

```text
95c0eebdb5ca9c785145e47b077d7772638400a697276e5e84c8d389009c61ce  build-host/p1b-canonical-final.bin
b053a7b8b6310ec92f9f8529927e9e190cd833d6304221abf5f330d25da9e264  build-host/p1b-render-final.bin
```

构建环境记录为 Windows 10.0.26300 x64、Java 17.0.7、CMake 3.20.0-rc1、NDK 25.1.8937393、compileSdk 33、arm64-v8a、C++17、GLES 3.0。

## 6. 真机技术验证

设备为 `MAX0019111000104` / `nunai_max`，Android 8.1 / API 27，Mali-T860，1920×1080，density 240。设备相机不可用，日志明确显示 synthetic provider 保持工作；因此姿态证据是确定性程序姿态，不是相机人体追踪结果。

### 6.1 High detail 冷启动

同进程 PID 6994 的最终冷启动：

```text
P1 continuous skin:
verts=57652 tris=115300 components=1 boundary=0 nonmanifold=0
degen=0 dup=0 badnorm=0 weights_bad=0 self_intersections=0
candidates=3732395 holes=0
extraction_ms=65798.6 weight_ms=400.1 intersection_ms=6549.2 total_ms=76832.7
```

```text
canonical body ready:
verts=57652 tris=115300 render_verts=345900 uv_charts=115300 gen_ms=83825
applied previewMode=3
```

证据：`evidence/p1-final/device-cold-start-queue-fix.log`。

### 6.2 GPU/CPU 一致性

测试前固定阈值：

- 最大位置误差 `< 5e-5 m`。
- 平均位置误差 `< 2e-5 m`。
- 最大法线角差 `< 0.5°`。

High detail 结果：

| Case | Max position error (m) | Mean position error (m) | Max normal angle (deg) | Result |
|---|---:|---:|---:|---|
| bind | 5.97627604e-07 | 7.57310019e-08 | 0.0279764533 | ok |
| elbow85_knee80 | 5.97210601e-07 | 7.92637707e-08 | 0.0395646803 | ok |
| elbow85_knee80_scale_2_0.5_3 | 5.67223594e-07 | 8.96253596e-08 | 0.0395646803 | ok |

### 6.3 质量刷新 `2 → 0 → 2`

Low detail：

```text
verts=36496 tris=72988 components=1 boundary=0 nonmanifold=0
degen=0 dup=0 badnorm=0 weights_bad=0 self_intersections=0 holes=0
canonical body ready: verts=36496 tris=72988 render_verts=218964 uv_charts=72988
quality=Low canonical triangles=72988 vertices=36496
```

Low detail 三组 GPU/CPU 案例通过，最大位置误差不超过 `6.02898865e-07 m`，最大法线角差不超过 `0.0395646803°`。

切回 High detail 后重新生成 57,652 vertices / 115,300 triangles，三组 GPU/CPU 案例再次全部通过。证据：`evidence/p1-final/device-quality-refresh-queue-fix.log`。

### 6.4 四模式切换与旧路径刷新

最终按日志确认后重拍的 `0 → 1 → 2 → 3` 全部 `applied`。模式 0 日志记录旧 Avatar 路径重新生成 30,961 vertices / 59,408 triangles，说明 Canonical 质量变化后旧路径能够刷新，而不是沿用旧 Medium cache。模式 3 命中 115,300-triangle Canonical CPU cache。

证据：`evidence/p1-final/device-mode-switching-recapture.log`。

注意：早期 `visual-mode0.png` 至 `visual-mode3.png` 四张哈希完全相同，已判定为命令生效前截取的无效证据。有效图像为 `visual-mode0-confirmed.png` 至 `visual-mode3-confirmed.png`，四张哈希互不相同。

### 6.5 Surface 重建

最终同进程 PID 6994：

```text
surfaceDestroyed
surfaceCreated
surfaceChanged 1920x1008
geometry cache hit: canonical (115300 tris)
```

三组 GPU/CPU 案例再次通过。UV Checker 重建前后 SHA-256 均为：

```text
152667a925f6ed3f8ecc4fc0056e2b741518a81c88f32d8b990f9dbee0896a57
```

证据：`evidence/p1-final/device-final-surface-and-runtime.log`、`uv-final-before.png`、`uv-final-after.png`。

## 7. 姿态与局部视觉证据

已采集：

- action 2：肘部弯曲，正面与 45° 视角。
- action 4：膝部弯曲，正面与 45° 视角。
- 手部近景。
- 脚部近景。
- 八组固定相机 preset。

文件包括：

- `evidence/p1-final/visual-pose2-preset0.png`
- `evidence/p1-final/visual-pose2-preset4.png`
- `evidence/p1-final/visual-pose4-preset0.png`
- `evidence/p1-final/visual-pose4-preset4.png`
- `evidence/p1-final/visual-hand-closeup.png`
- `evidence/p1-final/visual-foot-closeup.png`
- `evidence/p1-final/visual-final-front-confirmed.png`
- `evidence/p1-final/visual-queue-fix-preset0.png` 至 `visual-queue-fix-preset7.png`

观察结果：

- 手与腿之间已有清晰空隙，自然下垂手臂不再贴住大腿。
- action 2/4 截图未出现明显跨肢体拉扯；Host 隔离测试提供更强的量化约束。
- 手指确实存在，但近景仍呈连续手套状，指间沟槽和独立指形不够自然。
- 脚趾存在，但仅为模糊小突起，不能作为自然脚趾验收。
- 面部有眼、鼻、口 relief，但灰模全貌仍显平面、人工且细节不足。

## 8. 性能、内存与环境

最终 High detail 稳定窗口：

- FPS：45.5–46.9。
- frame_ms：21.32–21.97。
- p50：21.29–21.46 ms。
- p95：23.22–24.32 ms。
- p99：24.78–31.36 ms，其中一个窗口为 31.36 ms。
- draw calls：5。
- triangles：115,300。
- buffer bytes：31,363,136。

Surface 重建后的首个统计窗口包含恢复尖峰，fps 12.7 / frame_ms 79.04 / p99 44.05 ms，不计入稳定期。证据：`evidence/p1-final/device-final-stable-stats.log`、`device-final-surface-and-runtime.log`。

Surface 重建后进程内存：

- TOTAL PSS：244,785 KB。
- Native Heap PSS：204,472 KB。
- TOTAL SWAP PSS：278 KB。

环境：

- 电池 50%，温度 26°C，AC/USB powered。
- thermal_zone0/1：63,125（63.125°C）。
- thermal_zone2：2,600（2.6°C）。

证据：`evidence/p1-final/device-final-runtime-review.txt`。数据仅代表该设备当前构建。

## 9. 验收状态

| 项目 | 状态 |
|---|---|
| Host 全量测试 | PASS |
| Android 构建 | PASS |
| Canonical 拓扑/权重/法线/自交 | PASS |
| GPU/CPU 一致性 | PASS |
| 质量切换与 Surface 重建 | PASS |
| UV 重建一致性 | PASS |
| 肩部比例与手腿分离 | 改善，但未完成整体视觉验收 |
| 自然五官 | BLOCKED |
| 自然手指/脚趾 | BLOCKED |
| 自然半写实成人亚洲人物 | BLOCKED |
| P1-only 提交范围自包含 | 本次不采用；按负责人授权纳入 P0+P1 全部人物代码 |
| P1 / Gate 验收 | 未作出，不构成 PASS |

## 10. 已知限制与后续工作

1. 当前每三角形独立 UV chart 不是最终生产 UV 展开方案。
2. 姿态截面、局部体积、穿透和左右偏差的完整量化仍未执行。
3. 生成阶段、临时峰值内存和 GPU 上传耗时未完整拆分。
4. Preview 资源切换未完成完全 failure-atomic 重构。
5. 视觉需要继续迭代：五官、手指、脚趾、自然不对称、皮肤/头发/服装均不能由当前 Host 通过替代。
6. P1-only 独立范围仍受 `HeadAnatomy` 依赖影响。本次按负责人明确决定纳入 P0+P1 全部人物代码，避免生成无法独立编译的 P1-only 提交；`.workbuddy-ai/`、v6/v7 evidence、治理文件和无关既有修改仍排除。
7. 当前视觉验收保持 BLOCKED；提交与推送不改变该状态。
8. 外部 synthetic input 的精确来源仍未完全确定；最终 Surface 重建由本会话明确触发，最终稳定截图时 PAE 保持前台。
