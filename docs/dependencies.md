# Dependencies — M0 初始清单

## 构建/平台依赖
| 名称 | 版本 | 用途 | 许可证/来源 | 体积/模型 | 状态 |
|---|---|---|---|---|---|
| JDK | 17.0.7 | Gradle/Kotlin 编译 | Oracle/OpenJDK | — | 已安装 |
| Gradle | 7.6.1 | 构建工具 | Gradle Apache-2.0 | 缓存发行版 | 已缓存 |
| Android Gradle Plugin | 7.4.2 | Android 构建 | Apache-2.0 | 缓存 | 已缓存 |
| Kotlin Android Plugin | 1.8.x | Kotlin 宿主 | Apache-2.0 | 需下载 | 待验证 |
| Android SDK | compileSdk/targetSdk 33, minSdk 23 | 平台 API | Android SDK 许可 | — | 已安装 |
| NDK | 25.1.8937393 | C++17 原生构建 | NDK 许可 | — | 已安装 |
| CMake | 3.22.1 (SDK) | native 构建 | BSD-3 | — | 已安装 |
| MinGW g++ | 11.5.0 | 宿主机单测 | GPL（工具链） | — | 已安装 |

## 运行时依赖
| 名称 | 版本 | 用途 | 许可证 | 模型/体积 | 离线/网络 | 状态 |
|---|---|---|---|---|---|---|
| AndroidX CameraX (`camera-core`, `camera-camera2`, `camera-lifecycle`) | 1.2.3 | 摄像头帧输入 | Apache-2.0 | 无模型 | 离线可用 | verified（M7） |
| ML Kit Pose Detection (`com.google.mlkit:pose-detection` + `pose-detection-accurate`) | **18.0.0-beta5** | 33 点姿态识别 | **ML Kit Terms of Service（专有，非 Apache）**；Beta，无 SLA/弃用策略 | Bundled 模型：`assets/mlkit_pose/` 16 项约 9.2 MB（含 accurate/lite tflite）；APK 总体积 ~56 MB | 端侧推理；条款允许为修复/模型更新/兼容性联系 Google 服务 | verified（M7/M8 真机推理）；ADR-003 披露 |
| AndroidX core/appcompat/lifecycle | 1.10.1 / 1.6.1 / 2.6.2 | UI/生命周期 | Apache-2.0 | 无模型 | 离线可用 | verified |
| EGL / OpenGL ES 3.0 | 系统 | 渲染 | 平台 API | — | 离线 | 运行时检测（M2 verified） |
| Android RenderScript（框架 API，非打包依赖） | 平台（API 17+，31 起弃用） | M8 视频 YUV→RGB 转换 | 平台 API | 无模型/无体积 | 离线 | verified（盒子实测可用）；API 31+ 设备自动走 JPEG 兜底路径 |
| MediaCodec / MediaExtractor | 系统 | M8 视频解码 | 平台 API | — | 离线 | verified（盒子 H.264 MP4 实测） |

## 运行时权限
| 权限 | 用途 | 状态 |
|---|---|---|
| CAMERA | 实时姿态输入（M7） | 运行时请求；盒子无摄像头时优雅降级 |
| READ_EXTERNAL_STORAGE | 无 DocumentsUI 设备上的应用内文件浏览器（ADR-007，M7/M8） | 运行时请求；SAF 可用的设备不需要 |

## 明确禁止引入
Filament、Unity、Unreal、Godot、Three.js 等完整渲染引擎；任何 GLB/GLTF/FBX/OBJ/DAE 模型；外部角色/服装/头发/皮肤/场景贴图、HDRI、环境贴图、预制动画；示例视频/图片打包成资产冒充用户输入。

## 治理规则
新增依赖前记录：用途、必要性、许可证、体积、ABI、是否含模型、联网需求、替代方案；更新本文件与对应 ADR。姿态 SDK 的模型属于软件/推理依赖，不是美术资产，但必须单独披露；若产品进一步要求“APK 无任何模型二进制”，需另立 ADR 评估替代部署方式。
