# Android Build 报告（AUTH-04）

> 日期时间：2026-11-23 18:42–18:47 GMT+8
> Worktree：`E:/AI-Tools/ProceduralAvatarEngine-PoC`
> Git SHA：`99941a94ec58fb37ab0f1a36b90590d3fdce5198`
> 结果：**BUILD SUCCESSFUL in 2m 59s，35/35 tasks executed**
> 注意：构建过程中出现一个授权边界副作用，见第 6 节。

---

## 1. 环境

| 项 | 值 |
|---|---|
| Gradle Wrapper | Gradle 7.6.1 |
| JVM | JetBrains JBR 17.0.12（通过本次进程级 `JAVA_HOME` 指定） |
| OS | Windows 11 10.0 amd64 |
| Android SDK 路径 | `C:/Users/Giant/AppData/Local/Android/Sdk`（通过本次进程级 `ANDROID_HOME` / `ANDROID_SDK_ROOT` 指定） |
| compileSdk / targetSdk / minSdk | 33 / 33 / 23 |
| NDK | `25.1.8937393`（与 `app/build.gradle.kts` 一致） |
| CMake | `3.22.1`（与 `app/build.gradle.kts` 一致） |
| ABI | `arm64-v8a` |

环境说明：原 shell 的 `JAVA_HOME` 指向带引号的无效路径，且未设置 `ANDROID_HOME` / `ANDROID_SDK_ROOT`。本次仅在单条构建命令中进程级设置上述变量，**未修改系统环境变量、未创建 `local.properties`、未编辑全局配置文件**。

## 2. 执行命令

```bash
JAVA_HOME="C:/Users/Giant/.jdks/jbr-17.0.12" \
ANDROID_HOME="C:/Users/Giant/AppData/Local/Android/Sdk" \
ANDROID_SDK_ROOT="C:/Users/Giant/AppData/Local/Android/Sdk" \
"E:/AI-Tools/ProceduralAvatarEngine-PoC/gradlew" \
  -p "E:/AI-Tools/ProceduralAvatarEngine-PoC" \
  assembleDebug --console=plain
```

日志：`evidence/android-build/gradle-assembleDebug.log`

## 3. 构建结果与产物

| 项 | 值 |
|---|---|
| 构建结果 | `BUILD SUCCESSFUL in 2m 59s` |
| Tasks | `35 actionable tasks: 35 executed` |
| APK | `app/build/outputs/apk/debug/app-debug.apk` |
| APK 大小 | `59,113,745` bytes |
| APK SHA256 | `128dad084b7d48d34df908d693c6dcfbee978668a0a22add859d6dc1857e2e39` |
| 元数据 | `app/build/outputs/apk/debug/output-metadata.json` |

APK 路径确认来自当前 Worktree，满足 AUTH-05 的“APK 来源可确认”前提。

## 4. 警告与错误

- 无构建错误。
- C++ 警告：`AnatomyField.cpp:487:13: warning: enumeration value 'RegionCount' not handled in switch [-Wswitch]`。
- Kotlin 警告：多处 `VideoPoseSource.kt` 的 RenderScript / Allocation 相关 deprecated API 警告，以及 `PoseProviderAdapter.kt` 的 `@OptIn` 标记警告。
- 未通过修改源码或配置绕过任何问题。

## 5. 边界复核

| 项 | 结果 |
|---|---|
| 构建产物位置 | 全部在 Worktree 内：`app/build/`、`app/.cxx/`、`build-host/` 等 |
| 源码 / CMake / Gradle 改动 | 无（`git diff --name-only -- native app/src tests CMakeLists.txt` 为空） |
| 原工作区 | 未写入；仍为 3 删 + README 修改 + 11 untracked |
| Commit / Push | 未执行 |
| 设备操作 | 未执行 |
| `local.properties` | 未创建 |

## 6. 授权边界副作用（需负责人知悉）

构建前只读检查发现全局 SDK 中**不存在** `ndk/` 与 `cmake/` 目录；构建完成后，Android Gradle Plugin 自动下载并安装了：

- `C:/Users/Giant/AppData/Local/Android/Sdk/ndk/25.1.8937393`
- `C:/Users/Giant/AppData/Local/Android/Sdk/cmake/3.22.1`

这是 Gradle 自动依赖解析导致的全局 SDK 内容变化，**不是通过编辑配置或手动安装完成**，但客观上超出了“不得修改项目之外的全局 Android SDK”的严格字面边界。

处置：未删除、未清理这些组件，因为删除会进一步修改全局 SDK，且可能影响其他 Android 项目。已停止进入 AUTH-05 设备阶段，等待负责人决定：

1. 接受此次自动安装作为构建环境补齐；
2. 要求将 PoC 构建环境迁移到独立 SDK 路径后重建；
3. 其他处理方式。

## 7. 阶段结论（18:49 更新）

- Android Debug 构建成功，APK 已生成并计算 SHA256。
- 负责人于 18:47 指定 SDK 路径 `E:\AndroidSdk`，并接受继续执行 AUTH-05。
- 已按该 SDK 执行 `clean assembleDebug`，**后续所有设备验证均使用该 SDK 构建的 APK**。
- CPU/GPU skinning parity 仍为 `UNVERIFIED`。

## 8. 指定 SDK 重建（`E:\AndroidSdk`）

| 项 | 值 |
|---|---|
| 命令 | `gradlew clean assembleDebug --console=plain`（`ANDROID_HOME=E:\AndroidSdk`） |
| 结果 | `BUILD SUCCESSFUL in 37s`，`39 actionable tasks: 38 executed, 1 up-to-date` |
| 日志 | `evidence/android-build/gradle-assembleDebug-sdkE.log` |
| APK SHA256 | `2c47990bad7fd9753344ff4c18a77412872e473373a50e3eb8a393925c9a5a44` |
| APK 大小 | 59,113,740 bytes |
| 使用的 SDK 组件 | platform android-33、build-tools 35.0.1、NDK 25.1.8937393、CMake 3.22.1（均已存在于 `E:\AndroidSdk`） |

**以本节 APK 为准**：设备上安装并验证的即为 SHA256 `2c47990b…` 的这一份。

**关于第 6 节的全局 SDK 副作用**：`C:\Users\Giant\AppData\Local\Android\Sdk` 下由本次首次构建自动下载的 `ndk/25.1.8937393` 与 `cmake/3.22.1` 仍然保留，未删除、未清理。后续 PoC 构建一律使用 `E:\AndroidSdk`，不再依赖该全局 SDK。是否清理这两个目录留待负责人决定。
