# M1 — 05 实施报告与阶段门

## 阶段结论
- 状态：**PASS**
- 完成需求 ID：FUN-POSE-001（接口 designed）、REL-001（designed）、COMP-001（designed）、TEST-001（implemented + verified）。
- 未完成/阻塞：无；视觉与真机验证未开始。

## 需求与设计
冻结 22 骨 ID、右手系/米/弧度、列主序矩阵约定；定义 PoseFrame/AvatarParameters/SceneParameters/MeshData、JNI API 与有界命令队列；形成 ADR-001…006，其中 ADR-003 完整披露 ML Kit Beta/许可/体积/Z 语义。

## 代码变更
新增：`settings.gradle.kts`、`build.gradle.kts`、`gradle.properties`、`gradle/wrapper/*`、`gradlew`、`app/build.gradle.kts`、`app/src/main/AndroidManifest.xml`、`app/src/main/java/com/pae/host/MainActivity.kt`、`native/CMakeLists.txt`、`native/core/{math,config,diagnostics}/*`、`tests/{CMakeLists.txt,harness/mini_test.hpp,unit/core_tests.cpp}`。修改：无既有代码。

## 验证证据
- `./gradlew :app:assembleDebug --no-daemon` → BUILD SUCCESSFUL（49s）。
- `ctest --test-dir build/host-cygwin --output-on-failure` → 1/1 test passed，5 cases。
- Cygwin g++ 11.5 + Unix Makefiles 生成器；Gradle 7.6.1 本地发行版 workaround 已记录。

## 风险与遗留
- Gradle wrapper 官方下载 10s 超时；当前环境通过本地缓存发行版解决，其他环境可能需要更长网络超时或预缓存。
- 尚无 JNI 实现；M2 添加 Surface 生命周期与 GLES3 探针。

## 下一阶段准备
进入条件满足；M2 首个任务：实现 EGL/GLES3 探针、Surface 生命周期与 Render Thread，在 `nunai_max` 上取真实 GL 字符串与截图。
