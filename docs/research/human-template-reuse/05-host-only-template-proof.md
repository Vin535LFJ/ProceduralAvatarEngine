# R6：Host-only 原型实施报告

任务：P1-VR-ARCH-1 / Deliverable R6  
日期：2026-11-26（合并版）  
状态：**`IMPLEMENTED (HOST-ONLY) / TESTS 65 PASS 0 FAIL / VISUAL REVIEWED VIA IMAGE MODEL ONLY / ANDROID NOT RUN`**

本报告不宣布任何 Gate PASS，不授权 VR-1B。按 `AGENT.md` 的轮次验收契约，本轮产出为：① 代码（`tools/human-template-poc/`，29 个文件）+ 65 项测试，② 本轮新生成的 101 张图（84 + 17）+ 几何测量数值，③ 测试日志原文 + 失败项 + 未验证项。证据目录：`evidence/host-proof/`（入口 `README.md`，完整性 `SHA256SUMS.txt`）。

## 1. 仓库与分支状态

| 项 | 结果 |
|---|---|
| 代码提交 | **`0baa6987bf141db7f87b02fd21810634fa1a38ca`**：`tools/human-template-poc/`（29 个文件）+ `.gitattributes`（1 处追加），30 个文件、6,515 行新增，没有修改任何既有文件。文档与证据在其后的提交里 |
| 最终成果所在分支 | **`poc-sm/shape-model-v2`（唯一分支）**。理由：链 B 链接的引擎 host 库依赖 `native/procedural/humanoid/canonical/CanonicalSkinGenerator.h`，该文件只存在于这个分支；它也是任务书指定的工作分支和 VR-1B 的基线 |
| 基线 | `f1b4404f5d58b11397eee6f2d37372a7126b3bb5`（R1 / R2 代码在其历史中：`299ae3bf…`、`66a7e590…`） |
| 主仓 `feature/procedural-character-foundation` | HEAD `2ac101fd40e533d2f08776cb6f4e9356cd1b25d1`，已跟踪文件**零改动**，未做分支合并（两分支相差 607 个文件且 `AGENT.md`、ADR-013 两边都改过，合并是另一个决定） |
| 远端 | 2026-11-26 17:30 `git ls-remote` 成功，两个分支的远端头与本地一致 |
| 生产路径 | 未修改 Android 生成器、Canonical、LegacyRigV1（`native/skeleton/Skeleton.h:10-34`）、BoneId、`UBO[22]`（`RenderThread.cpp:31`）、64 B 顶点打包（`SkinnedGpuMesh.cpp:55-66`）、LBS（`Skinning.cpp:27-78`）、JNI、RenderThread |
| 禁止事项 | 未执行 `reset --hard` / `clean` / checkout / merge / rebase；提交与推送由负责人在本轮明确要求 |

## 2. 原型在哪、是什么

- 位置：`tools/human-template-poc/host/`（**在仓库内，但不被 Android 构建引用**；构建产物进 `build-host/human-template-poc/`，已被 `.gitignore` 忽略）。
- **不包含**任何 MakeHuman / MPFB2 代码；不依赖 Blender、MakeHuman、在线服务；Python 只用于链 B 的证据渲染、PNG 重压缩和一个只读统计脚本。
- 固定 commit 的 CC0 资产留在仓库外（`assets/`，不进 git）。

| | 链 A | 链 B |
|---|---|---|
| 入口 | `src/`：库 `pae_ht`（10 个模块）+ `pae_ht_tests` + `pae_ht_pipeline` | `template_proof.cpp`（单文件） |
| 复用的 PAE 代码 | 只读编译 `native/procedural/shape/{ImageWriter, CameraProtocol, ClayRenderer}.cpp` | 链接 `build-host/libpae_core.a`（Math、MeshData、MeshTopology、`canonical::detectSelfIntersections`） |
| 构建 | `powershell -File tools/human-template-poc/host/build.ps1` | `powershell -File tools/human-template-poc/host/run_template_proof.ps1 -Assets <固定资产根目录>`（先要有引擎的 host 构建） |
| 渲染 | 流水线内置软件光栅器，相机沿用 `CameraProtocol::fixedViews()` 7 视角 + 右侧视图 | `python tools/human-template-poc/host/render_proof.py <out>`（NumPy，正交相机） |
| 模板 | `.paht` 格式 2，写 → 读 → 写逐字节一致 | `base.paehuman`（`PAEHUM01`），只写不读 |

工具链：Cygwin g++ 11.5.0、CMake、`-O2 -Wall -Wextra`。源码与哈希：`evidence/host-proof/tooling-manifest.md`。

## 3. 验证链（任务书要求的六步）

| 步骤 | 链 A | 链 B | 证据 |
|---|---|---|---|
| 1 读取成熟人体拓扑 | 体表 13,380 稳定顶点 ID；**保留四边形**；UV；125 个关节组 + 13 个 helper 组；拓扑审计 18 项 + 单元质量 | 三角化读取；固定剖分的单元质量；自相交 = 0 | `report/topology-audit.md`，T02/T03；`logs/template-proof.log` |
| 2 通过 Shape Target 改变胸、腰、肩、髋 | 9 个参数在同一基础网格上 ±1.0；八参数组合与 +1.5 过冲；区域位移表 | 8 个 incr target 单独 + 组合，自相交均为 0 | `report/shape-targets.md`，T05–T08；图 `target-*.png` |
| 3 由 PAE 生成 / 加载 Rig V2 | 从 `.mhskel` 构建 102 骨 deform 与 20 骨 core；关节在当前表面重解析，骨架跟随体型 | 官方 163 骨装入可变容器 | `report/rig.md`，T09/T14 |
| 4 自动权重与 Top-4 | MakeHuman 权重 + **无权重回退规则**；PAE 距离法、热扩散法；重映射到 20 骨 | MakeHuman 权重迁移 + Top-4（保留面部骨） | `report/weights.md`，T10 |
| 5 肩、肘、髋、膝基础动作 | 7 个姿势 × 5 种权重/骨架组合，翻面 / 边拉伸 | 8 个动作，方向断言 | `report/poses.md`，T11；`DIRECTION_CHECK` |
| 6 生成 PAE 内部模板 | `.paht` 写 → 读 → 写逐字节一致；中性三角化 OBJ 导出 | `PAEHUM01` 导出 | T12/T13（链 A） |

## 4. 测试日志原文

`evidence/host-proof/logs/pae_ht_tests-final.log`（末行）：

```text
== summary: 65 passed, 0 failed, 65 total | load 319 ms ==
```

`evidence/host-proof/logs/template-proof.log`（末行）：

```text
DIRECTION_CHECK actions=8 failed=0
```

构建（`logs/build-final.log`、`logs/template_proof-build.log`）：两条链在 `-Wall -Wextra` 下零警告零错误。流水线（`logs/pipeline-final.log`）：总耗时 9.2 s（host 单线程：加载 343 ms；九参数应用+测量+质量审计 2.09 s；距离权重 41 ms；热扩散权重 564 ms；40 组蒙皮 9 ms；模板往返 40 ms；其余为出图与哈希）。

### 4.1 内存与耗时

| | 耗时 | 峰值工作集 | 口径 |
|---|---:|---:|---|
| 链 A `pae_ht_pipeline`（完整运行，含 84 张图） | 12.0 s（进程内计时） | **69,021,696 字节（65.82 MiB）** | PowerShell 秒表 + `Get-Process` 每 30 ms 采样；`logs/pipeline-memory.log` |
| 链 B `template_proof` | 8.15 s（CPU 7.53 s） | **38,207,488 字节（36.44 MiB）** | 同上，60 ms 采样；`logs/template-proof-timing.log`、`template-proof-memory.json` |

这是 **host 工具**的内存，包含解析、三套权重、40 次蒙皮审计和模板往返，**不是**模板在 Android 上的运行时占用。本轮不宣布任何性能 guardrail。

### 4.2 测试项一览（65 项，编号唯一）

| 组 | 数量 | 覆盖 |
|---|---:|---|
| T01 | 2 | SHA-256 已知答案 |
| T02 | 3 | OBJ 计数、无负索引、稳定顶点序 |
| T03 | 8 | 闭合、无非流形、绕向一致、欧拉示性数、外向、价态 |
| T04 | 1 | 关节辅助组存在 |
| T05 | 6 | 索引 0 基、三轴顺序、Y 不取反、行数 |
| T06 | 4 | α=0、+α−α、线性叠加、稳定 ID |
| T07 | 5 | 局部性、方向性、非整体缩放、无翻面、命名测量非零 |
| T08 | 2 | 左右独立 |
| T09 | 5 | 163 骨、父子/无环、面部骨剔除、20 骨 core、重父 |
| T10 | 15 | 权重解析、0 基、文件内许可证声明、**无权重回退规则（d–m）**、PAE 两套权重 |
| T11 | 5 | bind、骨骼存在、屈肘刚性、左右独立、单骨刚性 |
| T12 | 5 | 模板往返、内容、**回退标记随模板往返（e）**、顶点序、落盘哈希 |
| T13 | 3 | 三角化导出、PNG、与 PAE ClayRenderer 轮廓 IoU 0.9617 |
| T14 | 1 | 骨架跟随形态 |

### 4.3 失败记录（全部保留，未粉饰）

最终 65/65 是多轮迭代得到的。过程中出现的失败及其性质：

| 轮次 | 失败 | 性质 | 处理 |
|---|---|---|---|
| 1 | `extractGroup` 按首次出现顺序编号，稳定 ID 不连续 | 工具 bug | 改为按 `v` 行升序重编号 |
| 3 | 索引基准探测得到 0 基，而我此前向负责人报告“1 基” | **我的判断错误**（numpy 快查时把标签弄反） | 以数据为准：0 基；已更正并写入 R3 |
| 4–5 | 区域标注错误（小腿 172、腹部 210、neck02 一根骨吃掉 1,553 个上胸顶点） | 工具 bug（半径估计 + 最近骨策略不适合短关节骨） | 改为：骨半径 = 最近顶点距离中位数；躯干按高度分带 |
| 4 | 肢段长度把“中段骨尾”当成腕/踝 | 工具 bug | 腕 = `wrist` head，踝 = `foot` head |
| 4 | `+α 再 −α` 判据要求逐位相等 | 判据过严 | 改为 1e-12 m |
| 4 | 探针用骨骼 global 矩阵变换静止关节点，前臂长度“翻倍” | 工具 bug | 改用 skin 矩阵 |
| 4 | 模板解析要求父骨排在子骨之前 | 工具 bug | 只校验范围与自引用 |
| 4 | 与 PAE ClayRenderer 的 IoU 只有 0.766（32×64 小图） | 判据/分辨率问题 | 192×384 下 0.9617 |
| 5 | T14 身高变了但骨架没变 | **真实发现**：target 必须应用到含 helper 的完整 OBJ | 改为对完整 OBJ 应用；写入 R3/R5 |
| 5–6 | T11c 判据“腕部前移” | 我的解剖预期错误：前臂本就前伸，屈肘 90° 是把腕抬到肘上 | 改为“腕高于肘且前臂长度守恒” |
| 5–6 | T07 局部性判据 | 真实发现：target 的局部性是软的 | 先定判据（允许集合 + 2 % 或 0.5 mm），legShape 的骨盆渗入如实登记 |
| 6（合并前） | 175 个体表顶点无权重，蒙皮时留在原位 | **真实缺陷**：我的工具对无权重顶点没有回退 | 见 §6.2 |
| 7（复核） | 链 B 的屈肘方向反了 | **真实缺陷**：原日志只检查“数值有限” | 见 §6.1 |
| 7（复核） | 我给链 B 写的第一版断言：屈肘要求“腕向前移”、对侧表面要求 < 0.1 mm | 判据写错：屈肘是弧线运动；对侧渗漏来自源数据的杂散权重 | 改为位置判据与两层判据，均先用数据定性 |
| 7（复核） | 我的两个探针脚本：单位多乘了 0.1；按猜测的顶点映射配对权重，出现“头顶顶点权重在脚趾” | 探针 bug | 改用导出文件自带的 `original_index` 列重算；探针结论不进证据 |
| 7（收尾） | 生成的 `shape-targets.md` 把没有命名测量的三行填成了 `bideltoid width: 1.6659` | 报告生成 bug | 改为输出 `-`；已重跑流水线 |

## 5. 几何测量数值（口径见 R3 §3.3）

基础体表：身高 1.6659 m；双肩宽 0.5595（A 姿势含上臂外缘，口径偏大）；胸 0.2757 × 0.2005；腰 0.2971 × 0.1832；髋 0.3699；上臂 0.2197（0.132 H）、前臂 0.2021（0.121 H）、大腿 0.4162（0.250 H）、小腿 0.3392（0.204 H）；表面积 1.614 m²，体积 54.9 L。参数影响逐项数值见 R3 §3。

## 6. 本轮复核与修正（合并时完成）

### 6.1 链 B 屈肘方向

- 复核发现：`elbow_bend` 用绕世界 X 轴 **+85°**，实测左手向后移 0.336 m、向下 0.070 m——过伸，不是屈肘。
- 修正：改为 **−85°**；8 个动作全部加方向断言（R4 §8）。反向对照：把 +85° 编一个临时版本，断言报 `FAIL`，退出码非 0。
- 证据：`logs/template-proof.log` 的 `ACTION … direction=PASS` 与 `DIRECTION_CHECK actions=8 failed=0`。

### 6.2 175 个无权重体表顶点

- 数据定性：175 个顶点**全部**是“源权重只落在被剔除的面部骨上”（口轮匝肌、提上唇肌、笑肌），没有任何一个是源文件漏掉的；沿源骨架层级向上找，100 % 落到 `head`。
- 修正：实现 `kept-ancestor` / `nearest-bone` 两条回退规则（R4 §4.2），每顶点规则写进模板格式 2。
- 结果：体表 175 + 辅助 648 全部按 `kept-ancestor` 绑到 `head`；`nearest-bone` 命中 0；无权重顶点 0。测试 T10d–T10m、T12e。

### 6.3 骨架与权重文件的许可证标签

- `default.mhskel`、`default_weights.mhw` 的标签从链 B 的 `LICENSE_VERIFIED` 与链 A 的 `CC0 (declared in file)` **统一为 `PENDING_CONFIRMATION`（待确认，未验证）**。
- 同步修改：流水线生成的 `provenance/asset-manifest.md`、`report/rig.md`、`report/weights.md`、`.paht` 与 `base.paehuman` 内的来源字符串、R1。

### 6.4 对链 B 其余说法的复核结论

| 说法 | 判定 | 依据 |
|---|---|---|
| 未修改生产路径 | 属实 | 两个工作树已跟踪文件零改动 |
| 13,380 顶点、13,378 Quad | 属实 | 两条链独立得到同一组数 |
| 163 骨骼 | 属实但需限定 | 是“加载了官方骨架”，不是 PAE 自己定义的 Rig V2；无 bone roll |
| Top-4 权重 | 属实 | 压缩损失 L1 口径 1.66 %；链 A 的质量口径 0.575 % |
| 8 个动作 | 修正后属实 | 原日志只检查“数值有限”，屈肘方向错误被漏掉；现已加方向断言 |
| `self_intersections=0` | 属实 | 确实调用了 PAE 的 `detectSelfIntersections`；但未覆盖姿势之后 |
| 17 张最终渲染 | 属实 | 文件在；两条链都**没有经过人眼** |
| 内存 38.3 MB | 属实 | 本轮重测 38,207,488 字节 |
| `ZERO_RESET max_error=0` | 不构成验证 | 位移乘以 0 再比较，恒为 0 |
| “最终 17 张图已实际查看” | 未核实 | 没有说明查看方式；只能借助图像描述模型 |

## 7. 对照：旧 P1 Canonical / R1 mode-2 / R2 mode-2 / 本路线

旧路线数字**引用既有证据文件，本轮未重跑**（来源：`docs/phases/P1-visual-recovery/evidence/vr1a-r2/…`、`11-vr1a-r2-implementation-report.md`、`06-vr1a-r1-implementation-report.md`、`docs/phases/P1-continuous-gray-body/05-…`）。

| 指标 | P1 Canonical（mode-3） | R1 mode-2 | R2 mode-2 | Base-Topology Host Proof（链 A / 链 B） |
|---|---|---|---|---|
| 身高 | 名义 1.75 m | 仅躯干段（y 0.94–1.40） | 未报告 | 1.6659（实测） |
| 肩宽 | 输入 0.465 | 环 @1.3814 宽 0.4720 | 未报告 | 0.5595（口径不同，不可直接比） |
| 胸廓 宽×厚 | 输入 0.365 | 0.3683 × 0.2611 | 未报告 | 0.2757 × 0.2005 |
| 腰 宽×厚 | 输入 0.285 | 0.2893 × 0.2653 | 未报告 | 0.2971 × 0.1832 |
| 骨盆宽 | 输入 0.345 | — | — | 0.3699 |
| 肢段比例 | 未报告 | 未报告 | 未报告 | 上臂 0.132、前臂 0.121、大腿 0.250、小腿 0.204 |
| 顶点 / 三角形 | 46,674 / 93,344 | 524 / 1,028 | 1,795 / 3,520 | 13,380 / 26,756（四边形 13,378） |
| 身体覆盖 | 全身 | 躯干 + 肩 + 上臂根 | 躯干 + 肩 + 上臂根 | 全身 |
| 拓扑质量 | 分量 1、边界 0、自相交 0 | 边界 0、sliver 6、接缝角 max 114.5° | 边界 32、sliver 56、最小角 1.42°、纵横比 max 36.4 | 全四边形、边界 0、χ=2、四边形内角 min 13.86°、纵横比 max 10.41 |
| 形态变化 | 参数驱动生成 | 站点控制 | 站点 + 形态基 | 9 个 target，±1.0 翻面 0 |
| Rig | 22 | 22 | 22 | 20 core / 102 deform / 163（三条实现） |
| 权重 | 距离法 Top-4 | 区域 Top-4 | 区域 Top-4 | MakeHuman / PAE 距离 / PAE 热扩散 |
| 基础动作 | 真机已验证 | 单侧动作 NOT VERIFIED | 左右独立 NOT VERIFIED | 链 A 7 姿势（翻面数见 R4）；链 B 8 动作（方向断言） |
| 生成时间 | 62,467 ms（设备） | 7.651 ms（设备） | 49.73 ms（设备） | host 全流程 9.2 s / 8.15 s（**不是同一口径，不可比较**） |
| 内存 | PSS 244,785 KB | 未报告 | 未报告 | host 工具峰值 65.82 MiB / 36.44 MiB（**不是设备口径**） |
| 手 / 足 / 脸 | 占位或粗略 | 无 | 无 | 有独立拓扑（手 3,136、足 2,152、头 2,723 顶点） |
| 视觉状态 | 历史基线 | VISUAL BLOCKED | VISUAL BLOCKED | Host 路线可行，最终视觉未验收 |

**核心比较标准**：

- 更自然：是。R1/R2 只覆盖躯干—肩—上臂根（边界/裁剪/接缝问题来自拼接），本路线是完整闭合人体，没有拼接缝。**但“更自然”的视觉判断只经过图像描述模型，未经人眼。**
- 形态变化合理：是。九个参数都改变了命名区域，方向正确，局部性有实测边界。
- 适合动作：肩外展 70° 翻面 0、髋屈 60° 翻面 1；握拳不成立。
- 易于扩展：面部、手指、脚趾已有独立拓扑与骨骼；足弓、掌侧细节、材质分区需要 PAE 补。
- 不以顶点数少或生成时间短论优劣：本路线 26,756 三角形，多于 R2 的 3,520；R1/R2 仍是历史基线。

## 8. 视觉证据（本轮新生成，101 张）

| 类别 | 文件 | 数量 |
|---|---|---:|
| 全身灰模 | `images/base-clay-{front, front-45-left, side-left, side-left-30up, back, back-45-right, front-30up, side-right}.png` | 8 |
| 线框 / 法线 / 区域 | `images/base-wire-front.png`、`base-wire-front-45-left.png`、`base-normal-front.png`、`base-regions-front.png` | 4 |
| 权重热图 | `images/weights-{mh, pae-heat, pae-distance}-front.png`、`weights-mh-back.png` | 4 |
| 形态参数 | `images/target-NN-<参数>-{incr,decr}.png`、`target-all-combined-front{,-45-left}.png` | 20 |
| 动作 | `images/pose-<动作>-{mh, paeheat, mhcore20}-front-45-left.png`、屈肘侧视 ×3 | 24 |
| 解剖近景（线框叠加） | `images/closeup-{shoulder-L, axilla-L, hip-groin-L, face, hand-L, foot-L, elbow-L, knee-L}-N.png` | 24 |
| 链 B | `images-proof-b/`：6 视向灰模、线框、法线、权重热图、肩/腋近景、双臂外展正侧视、4 个 target 对照 | 17 |

索引：`report/images.md`。图像已做像素级无损重压缩（`logs/recompress*.log` 记录压缩前后字节数与像素哈希）。链 A 连续两次运行产出的 84 张图逐字节相同。

交叉检查：链 A 光栅器与 PAE `ClayRenderer::renderClayView` 在正面视图的轮廓 IoU = 0.9617。**这些图是 host 软件渲染，不是 Android GLES3 输出。**

### 8.1 我是如何“看”这些图的

我没有眼睛。图像通过 `media_read` 交给一个图像描述模型，逐张返回客观描述，我据此与数值交叉核对：

- 全身 8 视角：完整人体（头、躯干、双臂带手、双腿带足），A 姿势，无孔洞、无翻转、无裁剪。
- 屈肘 / 肩外展：角色左臂呈对应姿势，右臂不变。
- 握拳：描述为“手部压扁/揉皱，屈曲不清晰”——与 714 个翻面一致。
- 近景：肩部环线从胸廓越过三角肌进入上臂；面部有眼眶环线、口部同心环线、鼻孔；五指各自独立带环线；足弓浅、趾间分隔浅；腋窝凹陷偏浅。
- 描述模型有可见的不可靠之处（例如把 A 姿势的肩部近景说成“手臂抬起”），两次对足趾的描述互相矛盾（一次说融合、一次说分开）。因此 **R2 中的“图上观察”只能当作线索，负责人必须亲自打开 `closeup-*.png` 复核**。

## 9. 发现汇总（按重要性）

1. 完整链路成立：读取成熟拓扑 → target 改变胸腰肩髋 → PAE Rig V2 → 权重 + Top-4 → 六类基础动作 → 内部模板，全部由 PAE 自己的 C++ 在 host 上完成，两条独立实现互相印证。
2. Target 索引 0 基、轴顺序 XYZ；MPFB2 文档的 “X Z Y + Y 取反” 是 Blender 导入约定。
3. Target 必须应用到含 helper 的完整 OBJ，否则骨架不跟随形态。
4. MakeHuman 权重剔除面部骨后留下 175 个无权重体表顶点——需要、且已经有了确定的回退规则。
5. 握拳在基础网格 + LBS 下不成立；肩外展下 LBS 有 3 倍边拉伸。
6. 肩宽/胸廓宽 target 会平移整条手臂；`upperleg-muscle` 渗入臀区。局部性是软的。incr 与 decr 不对称。
7. macro height target 不能单独使用（直接得到 2.39 m）。
8. 动作方向必须靠断言或看图确认：链 B 的“屈肘”曾把肘向后掰直，而位移大小与正确版本完全相同。
9. 单位 0.1 m/单位是推断；测量平面是 PAE 自定义口径。
10. 治理冲突：项目章程“零外部 3D 美术资产”、ADR-004“不得改回外部模型”、11-25 研究记录“不导入”。

## 10. 未验证项清单

- Android：GLES3 渲染、GPU/CPU skinning parity、UBO、JNI、RenderThread——**NOT RUN**。
- 人眼视觉验收——**NOT DONE**（只有图像描述模型）。
- 姿势之后的自相交（肩/腋、腹股沟、膝）——NOT RUN；target 之后的自相交只覆盖链 B 的 8 个 incr。
- Dual Quaternion Skinning、姿势修正形态、扭转传递——NOT RUN。
- Bounded Biharmonic / Harmonic 权重——NOT RUN。
- 围度类 target、宏参数组合、其余 1,254 个 target——NOT RUN。
- 模板在设备上的内存与加载时间——NOT MEASURED（host 工具内存不能代替）。
- Bone roll 作为蒙皮轴的使用、锁骨/肩胛联动、手指/脚趾动作——NOT RUN。
- 网格单位的官方说明、`default.mhskel` / `default_weights.mhw` 的再分发依据——UNVERIFIED。
- 表面解剖 Landmark、LOD / 稳定顶点对应、材质区域——未做。
