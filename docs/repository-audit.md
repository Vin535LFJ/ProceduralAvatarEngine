# Repository Audit — M0 事实记录

- 审计日期：2026-11-21
- 目录：`C:\Users\Giant\Desktop\GITCODE\Procedural Avatar Engine`
- 证据：`docs/phases/M0-project-discovery/evidence/toolchain.txt`、`device.txt`

## 仓库现状
| 项目 | 事实 |
|---|---|
| 初始内容 | 仅 3 份 Markdown、`README_交付说明.md`、`Procedural_Avatar_Engine_Design_Package_v1.1.zip`（仅文档副本） |
| 源码/构建脚本/测试/CI | 无 |
| `docs/` / ADR / RTM / 阶段记录 | 初始无，M0 已创建 |
| `AGENTS.md` / `CLAUDE.md` | 无 |
| Git | 初始不是仓库；M0 已执行 `git init`，未提交、未推送 |
| 外部 3D 美术资源 | 无（全目录扫描模型/贴图/视频扩展名无命中） |

## 构建环境
| 项目 | 事实 |
|---|---|
| OS / Shell | Windows 10（26300）/ Git Bash |
| JDK | 17.0.7 LTS |
| CMake | 3.20.0-rc1（PATH）；SDK 自带 3.22.1 |
| C++ 编译器 | MinGW g++ 11.5.0；无 clang |
| Gradle | PATH 无；缓存发行版 7.6、7.6.1、8.0.2、8.9、8.10(.2)、8.13、9.2.0 |
| AGP | 本地缓存 7.4.2、3.5.4 |
| Android SDK | `E:\AndroidSdk`；platforms 14–35、build-tools 28.0.3–36.1.0 |
| NDK | 21.4.7075529 / 22.1.7171670 / 25.1.8937393 / 27.0.12077973 |
| SDK 环境变量 | `ANDROID_HOME`/`ANDROID_SDK_ROOT` 未设置，构建命令内联设置 |
| 网络 | Gradle services 与 Google Maven 可达；CameraX/ML Kit 构件未缓存 |

## 设备
| 项目 | 事实 |
|---|---|
| 设备 ID | `MAX0019111000104`（adb online） |
| 型号 / 品牌 | nunai_max / nunai |
| Android | 8.1.0（API 27） |
| ABI | arm64-v8a（兼容 armeabi-v7a、armeabi） |
| 类型 | `android.hardware.type.television` + `android.software.leanback_only`（电视盒子） |
| GLES 声明 | `reqGlEsVersion=0x30002`、`ro.opengles.version=196610`（GLES 3.2）；**仍须 Native EGL 实测** |
| 摄像头 | 声明 `android.hardware.camera` / `camera.any` / `camera.front` / `autofocus`；实际可用性需运行时验证 |
| 模拟器 | 无已配置 AVD（system-images 存在但 `.android/avd` 为空） |

## 构建基线决策
Gradle 7.6.1 + AGP 7.4.2（均缓存）+ JDK 17 + compileSdk/targetSdk 33 + minSdk 23 + NDK 25.1.8937393 + CMake 3.22.1 + arm64-v8a。Kotlin 1.8.x、CameraX、ML Kit 需联网下载。宿主单测用自研 mini test harness，避免网络依赖。

## 未验证事项
- Native EGL GLES 3.0 Context 实际创建与渲染（M2 验证）。
- 摄像头真实取帧与 ML Kit 实时推理（M7 验证）。
- 真机 FPS/温度/内存数据（M9 验证）。

## 基线复核（2026-11-21，M0/M1 重验）
- 宿主单测 fresh build（cygwin g++ + Unix Makefiles + `PAE_HOST=ON`）→ `ctest` 5/5 passed；证据 `docs/phases/M0-project-discovery/evidence/ctest-reverify-20261121.txt`。
- `./gradlew :app:assembleDebug` BUILD SUCCESSFUL（JDK 17 位于 `D:\Program Files\Java\jdk-17`，需内联 `JAVA_HOME`；SDK 经 gitignored `local.properties` 指向 `E:\AndroidSdk`）；证据 `docs/phases/M0-project-discovery/evidence/gradle-reverify-20261121.txt`。
- 设备 `MAX0019111000104` 仍 adb 在线；摄像头运行时不可用（M7 logcat：`camera selector unable to resolve a camera`），与 Manifest 声明不一致——真实硬件以运行时结果为准。
