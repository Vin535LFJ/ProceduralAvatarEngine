# M0 — 02 设计

- 目标架构边界沿用技术规范 §2：Android Host / Input Adapter / Pose Provider / Motion Runtime / Procedural Core / Avatar Runtime / Renderer。
- 逻辑目录：`app/`（Kotlin + JNI 胶水）、`native/`（平台无关 C++17 核心）、`tests/`（宿主机单测）、`docs/`（治理与证据）。
- 构建：缓存优先的 Gradle 7.6.1 + AGP 7.4.2 + JDK17 + NDK 25.1.8937393 + CMake 3.22.1；compileSdk/targetSdk 33、minSdk 23（ML Kit 底线）、arm64-v8a。
- 证据策略：所有命令输出存入 `evidence/`，设备声明值与 Native 实测值分开记录。
