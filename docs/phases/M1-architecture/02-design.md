# M1 — 02 设计

见：
- `docs/architecture/target-architecture.md`（模块边界、数据流、坐标约定）
- `docs/architecture/module-contracts.md`（PoseFrame/AvatarParameters/SceneParameters/MeshData、渲染命令、JNI API）
- `docs/architecture/threading-and-lifecycle.md`（五线程模型、Surface/Context Lost、错误处理）
- `docs/architecture-decisions/ADR-001..006.md`

设计取舍：构建选缓存优先（Gradle 7.6.1/AGP 7.4.2/NDK 25.1.8937393/CMake 3.22.1）；宿主单测用自研 mini harness 避免网络依赖；JNI 只投递 POD/命令。
