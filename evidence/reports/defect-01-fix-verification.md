# DEF-01 修复验证报告（方案 B：几何缓存 + 恢复时只重传 GPU buffer）

> 日期时间：2026-11-23 19:06–19:12 GMT+8
> Worktree：`E:/AI-Tools/ProceduralAvatarEngine-PoC`
> 基线 SHA：`99941a94ec58fb37ab0f1a36b90590d3fdce5198`（detached HEAD）
> 授权：负责人于 19:06 明确批准「方案 B」与「G1」
> 结果：**恢复耗时 4347 ms → 487 ms（8.9×），黑帧完全消除**

---

## 1. 变更清单（仅 Worktree 内）

| 文件 | 变更 |
|---|---|
| `native/renderer/RenderThread.h` | 新增 CPU 侧几何缓存成员：`cachedAvatarMesh_`、`cachedAnatomyMesh_`、`cachedP1aMesh_` 及对应 `hasCached*_` 标志 |
| `native/renderer/RenderThread.cpp`（`ensureResources`） | 头像 / 解剖 / P1-A 三套几何改为「缓存未命中才生成，命中则直接重传 GPU buffer」；新增 `geometry generated:*` / `geometry cache hit:*` 日志 |
| `native/renderer/RenderThread.cpp`（`releaseGpuResources`） | 补上此前遗漏的 `anatomyMesh_.release()` / `anatomyLines_.release()`（GPU 对象随资源集释放，CPU 网格留在缓存） |
| `native/renderer/RenderThread.cpp`（Quality 路径） | 质量档重建后同步更新 `cachedAvatarMesh_`，保持缓存与实际几何一致 |
| `native/renderer/RenderThread.cpp`（P1-A 报告） | 缓存命中时输出 `P1A-REPORT (cached)` 标记，避免把历史 `gen_ms` 误读为本次实测 |

未触碰：22 Bone ABI、64B Vertex ABI、GPU skinning UBO 布局、JNI/Kotlin 对外签名、正式生产入口、任何 CMake/Gradle 文件。

## 2. 修复前后对比（同一设备、同一 APK 流程）

| 指标 | 修复前 | 修复后 | 变化 |
|---|---|---|---|
| 冷启动生成 | anatomy 4100 ms，surface ready 距启动 4576 ms | anatomy 4113 ms，surface ready 距启动 5345 ms | 不变（缓存为空时必须生成） |
| **后台恢复总耗时** | **4347 ms**（21.8 s 重建开始 → 26.249 s surface ready） | **487 ms**（24.862 s cache hit → 25.349 s surface ready） | **8.9× 缩短** |
| **恢复后黑帧** | 8 帧采样中 4 帧全黑（23,142 B） | 8 帧采样中 0 帧黑（56–68 KB 全部为已渲染画面） | **黑帧消除** |
| 崩溃 / ANR | 无 | 无 | — |

修复后恢复日志（`evidence/tvbox/logcat-def01b.txt`）：

```
19:10:22.559  PAE_EGL   : EGLSurface destroyed
19:10:24.862  PAE_RENDER: geometry cache hit: avatar
19:10:24.908  PAE_RENDER: geometry cache hit: anatomy (40984 tris)
19:10:25.317  PAE_RENDER: geometry cache hit: p1a
19:10:25.337  PAE_RENDER: P1A-REPORT (cached) verts=870 tris=1664 components=1 boundary=80 ...
19:10:25.349  PAE_RENDER: surface ready 1920x1008
```

## 3. 验证证据

| 证据 | 路径 |
|---|---|
| 修复后构建日志 | `evidence/android-build/gradle-assembleDebug-def01b.log` |
| APK SHA256 | `evidence/android-build/apk-sha256-def01b.txt` → `088476fcd1fd5b84edfe7109116f310a4753a8a627a837fe39691320f055a9c4` |
| 安装日志 | `evidence/tvbox/adb-install-def01.log` |
| 恢复日志 | `evidence/tvbox/logcat-def01b.txt` |
| 恢复截图序列（8 帧，无黑帧） | `evidence/screenshots/resume-timeline-def01b/` |
| 修复前对照序列（4 帧黑） | `evidence/screenshots/resume-timeline/` |
| Host Tests（改动后） | `evidence/host-tests/ctest-after-def01.log` → 9/9 PASS |

## 4. 已知限制与后续

- **冷启动仍需约 4.6 s**：首次创建时缓存为空，必须生成解剖几何。这是「生成耗时」问题，需由 G1/G2 的 Geometry Worker 架构解决，不属本缺陷范围。
- 缓存当前仅在两条路径被更新：首次生成与 Quality 切换。后续若新增参数化几何入口，必须同步失效/更新缓存，否则会出现「缓存内容与当前参数不一致」的隐性错误。
- `ensureResources()` 仍会重建 shader program（GL 对象重建），当前上下文存活时属冗余但开销小；后续可随 Geometry Worker 一起清理。
- 本次仅修复「恢复黑屏」，未改变任何几何、材质、相机或动画行为。

## 5. 边界

- 修改仅限负责人批准的方案 B 与生产渲染路径中的必要内部实现；未替换生产入口，未改 ABI。
- 未 Commit / Push；原工作区未触碰；全部产物位于 Worktree `evidence/`。
