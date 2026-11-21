# M11 — 05 视觉验收（Visual Quality Matrix 评分）与 06 实施报告

- 日期：2026-11-21；分支 `m11-anatomy-visual-rebuild`
- 回归基线：ctest **7 套件 / 43 用例全绿**（新增 fusion 3 + 样条扫掠 4 + 权重 2 + 面发 3 + 服装 2）；`./gradlew :app:assembleDebug` BUILD SUCCESSFUL
- 设备：nunai_max 盒子（Android 8.1 / API 27 / arm64-v8a）

## Visual Quality Matrix（证据 = evidence/ 截图）

| # | 维度 | 评分 (1-5) | 说明与证据 |
|---|---|---|---|
| 1 | Body Silhouette | **4** | 轮廓自然（腰收/臀形/四肢纺锤），`frame-m11c-front.png` vs 改前 `frame-m9-final.png` |
| 2 | Torso | **4** | 7 站位样条放掠 + 胸深/腰收参数生效；胸-腰-臀层次可见 |
| 3 | Shoulder Integration | **4** | 关节球全部移除，嵌入扫掠过渡连续（抬臂/双臂展开无凸起），`frame-m10-checkpoint1.png` |
| 4 | Limb Shape | **4** | 大腿/膝/小腿/踝锥度自然（小腿后弧可见），`frame-m11c-front.png` |
| 5 | Face | **3→4** | 眼白/虹膜/瞳孔/高光/眉/鼻梁鼻头鼻翼/唇/发际完整；嘴部比例与下巴形状仍可调优（backlog） |
| 6 | Hair | **3** | 帽+刘海+侧发+后发束多层结构 ✓；发束形态较简单（v1） |
| 7 | Clothing | **3** | 衬衫/长裤独立外壳有厚度+松量，领口/袖口/衣摆 ✓；褶皱未实现 |
| 8 | Skin/Material | **3** | 材质分支 + 墨色；roughness/SSS 增强在 M11-G 剩余项 |
| 9 | Deformation | **4** | 抬臂/屈肘无裂缝（`frame-m11d-*.png`）；视频跟随回归 conf 1.00（`frame-m11h-vedio-demo.png`） |
| 10 | Bind Pose | **4** | A-Pose 结构正确、左右对称、恒等=绑定（测试锁定） |
| 11 | Params→Geometry | **4** | eyeSpacing/shirtHem/hair.style 参数对比测试 + 截图验证 |

**P0 维度全部 ≥4 → 视觉阶段门 PASS WITH FOLLOW-UPS**（Face/Hair/Clothing/Material 的深化列入 M11 backlog）。

## 性能（M11 构建实测，Medium 档）

| 指标 | M10 基线 | M11 实测 | 预算 |
|---|---|---|---|
| 三角形 | 7537 | **14065**（含服装+发束+面部） | 5k-15k ✓ |
| FPS | 47.7-48.5 | **50.6-53.2** | ≥30 ✓ |
| P50/P95/P99 | 20.8/23.3/23.5 | 18.7-18.9 / 21.5-21.9 / 22.8-23.1 ms | P95≤33.3 ✓ |
| Draw calls | 4 | 4 | — |
| 生成耗时 | ~15ms | **25-40ms**（一次性初始化） | 不阻塞实时 ✓ |
| Native Heap | 87.3MB | 202.7MB（含视频管线+ML Kit 会话） | 长测观察 |
| APK | ~56MB | ~56MB | — |

10 分钟稳定性长测：`evidence/soak-10min-m11.log`（后台执行中，完成后归档）。

## 06 实施报告

### 完成（M11-A 至 M11-H）
- **A**：审计（15 项确认，02 文档）+ 视觉需求矩阵 + 架构设计 + ADR-008
- **B**：CatmullRom 样条、SplineSweep（椭圆截面/roll/锥度法线/端盖/UV 接缝）、SplineLoft（旋转/winding 归一化）、SdfFusion（Surface Nets）；三法 PoC → ADR-008 定案混合方案
- **C**：Humanoid 重写——BodyShapeParams 全参数（胸深/体积/倾斜/腰收/髋flare/三角肌/前臂纺锤/小腿弧/踝收…）+ 部位子生成器 + 关节球全移除（嵌入扫掠）+ 髋缝消除
- **D**：WeightGenerator 区域权重（跨部位连续 + 路径骨主导约束 + 邻居影响半径）+ Head/Neck 骨骼桩修正
- **E**：FaceGenerator（剖面调制头形 + 全五官）+ HairGenerator（帽/刘海/侧/后发束，3 预设）
- **F/G**：ClothingGenerator（衬衫/袖/领/长裤，厚度+松量+衣摆参数）+ 调色板 5 色（pants 材质）
- **H**：置信度真正生效（hold-predict 收缩）、缺失关键点 0.5s 保持、lastTargets 保持、ProviderWorld 拒绝、PixelImage z 归一化、debug autostart 钩子

### 未完成（如实记录，backlog）
- 侧面/背面/特写相机预设按钮（当前证据为单一正面固定相机 + 动作集）
- 材质增强（roughness/SSS/AO）、服装褶皱、M11-G 的 High 档噪声细节
- Head/Neck 的鼻方向弱驱动、Hips 骨盆 roll（M11-H 范围收缩项）
- 摄像头路径仍需带摄像头设备（M7 遗留，与本阶段无关）
