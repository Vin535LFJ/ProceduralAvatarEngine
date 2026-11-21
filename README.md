# Procedural Avatar Engine

纯程序化 3D 卡通数字人引擎 MVP：姿态输入（合成动作 / ML Kit 姿态识别 / 本地视频）驱动由代码与参数生成的卡通人体，使用 Android NDK + C++17 + 自研 OpenGL ES 3.0 渲染器实时渲染。

**零外部美术资产**：几何、骨骼、蒙皮权重、材质、背景全部由程序生成；不加载 GLB/GLTF/FBX/OBJ、外部贴图或 HDRI。姿态识别 SDK（ML Kit）作为已披露的软件依赖（见 `docs/dependencies.md`）。

## 构建

要求：JDK 17、Android SDK（platform 33 / build-tools / NDK 25.1.8937393 / CMake 3.22.1）。

```bash
# 指定 SDK 位置（或使用 ANDROID_HOME）
echo "sdk.dir=E\\:\\\\AndroidSdk" > local.properties   # 按实际 SDK 路径修改（该文件被 gitignore）

./gradlew :app:assembleDebug            # 产出 app/build/outputs/apk/debug/app-debug.apk
```

## 安装与运行

```bash
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb shell am start -n com.pae.host/.MainActivity
```

## 界面操作（TV 遥控 / 触屏通用）

| 按钮 | 功能 |
|---|---|
| Skin / Hair / Cloth | 循环切换皮肤 / 头发 / 服装颜色（仅 Uniform 更新，不重建几何） |
| BG | 循环纯色 / 渐变 / 程序化摄影棚背景 |
| Pose | 循环合成动作：自动 → 抬臂 → 屈肘 → 双臂 → 下蹲 → 抬腿（同时切回合成驱动源） |
| Camera | 授予相机权限后启动实时姿态跟随（需要设备有可用摄像头） |
| TestImg | 选择一张本机图片做单帧姿态推理（管线验证；无系统文件选择器的电视盒子自动回退到应用内浏览器） |
| Video | 选择本机视频：解码 → 姿态推理 → PTS 时间轴驱动（Pause / ±10s / 倍速 / Replay / Stop） |
| Quality | 循环 Low / Medium / High 几何细分档位 |
| Stats | 输出渲染统计（FPS、帧时间 P50/P95/P99、draw calls、三角形数） |

## 测试

```bash
# 宿主单元测试（无需 Android 设备）：CMake + 自研 mini test 框架
cmake -S native -B build/host -G "Unix Makefiles" -DPAE_HOST=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build/host -j 8
ctest --test-dir build/host --output-on-failure    # 5 套件 / 24 用例
```

真机验证记录与证据：`docs/phases/M0..M9/*/04-test-and-acceptance.md` 与 `evidence/`。

## 文档

- 技术规范：`docs/Procedural_Avatar_Engine_Technical_Spec_v1.0.md`
- 开发计划与阶段门：`docs/Procedural_Avatar_Engine_Development_Guidance_Plan_v1.0.md`
- 项目状态：`docs/project-status.md`；需求追踪：`docs/requirements/requirements-traceability.md`
- 架构决策（ADR）：`docs/architecture-decisions/`
- 性能：`docs/performance/`；发布清单：`docs/release/mvp-readiness-checklist.md`

## 已知限制

- 单目姿态识别的深度歧义（转身 / 前后动作近似跟随，Spec §7.6）；不宣称精确 3D 动捕。
- 摄像头实时路径需带可用摄像头的设备（基准电视盒子无摄像头，该项未验证）。
- ML Kit 对非人物画面可能输出误报关键点（人物存在性判定列入 backlog）。
