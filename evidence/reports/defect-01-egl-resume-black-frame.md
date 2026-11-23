# 缺陷诊断：EGL 恢复路径黑帧（DEF-01）

> 日期时间：2026-11-23 18:58–19:02 GMT+8
> Worktree：`E:/AI-Tools/ProceduralAvatarEngine-PoC`
> Git SHA：`99941a94ec58fb37ab0f1a36b90590d3fdce5198`
> 设备：`nunai_max`（Mali-T860，Android 8.1.0）
> 状态：**根因已确认；修复方案需负责人批准（属生产渲染路径，超出 AUTH-07 的 R2/R3 实验范围）**

---

## 1. 现象

按 HOME 退出应用后重新进入，GL 渲染区出现**持续约 4.2 秒的纯黑**，随后自动恢复为正常画面（无需任何用户操作）。

复现次数：2/2 稳定复现。

## 2. 证据

### 2.1 设备日志（`evidence/tvbox/logcat-resume-timeline.txt`）

```
19:01:18.901  PAE_EGL   : EGLSurface destroyed          <- HOME，surface 销毁
19:01:21.179  (adb 发起 am start)                        <- 重新进入
19:01:25.914  PAE_RENDER: anatomy preview ready: 40984 tris, 4130 ms
19:01:26.237  PAE_RENDER: P1A-REPORT ... gen_ms=2.384
19:01:26.249  PAE_RENDER: surface ready 1920x1008        <- 资源就绪
```

### 2.2 截图时间线（`evidence/screenshots/resume-timeline/`）

| 帧 | 采集时刻 | 文件大小 | 内容 |
|---|---|---|---|
| frame-01 | 19:01:23.230 | 23,142 B | 黑（仅按钮栏） |
| frame-02 | 19:01:24.161 | 23,142 B | 黑 |
| frame-03 | 19:01:25.202 | 23,142 B | 黑 |
| frame-04 | 19:01:26.205 | 23,142 B | 黑 |
| frame-05 | 19:01:27.329 | 197,660 B | **恢复正常渲染** |
| frame-06…12 | 19:01:28.46 → 19:01:35.56 | 197,660 B | 正常 |

黑帧区间：**19:01:22 ~ 19:01:26.2，约 4.2 秒**；恢复正常发生在 `surface ready`（19:01:26.249）之后约 1 秒。

## 3. 根因

**渲染线程在 surface 重建时同步重算全部几何，期间不提交任何一帧。**

代码链路：

1. `RenderThread.cpp:424-427` — `SurfaceDestroyed` → `releaseGpuResources()`（`resourcesReady_ = false`）+ `egl_.destroySurface()`
2. `RenderThread.cpp:400-418` — `SurfaceCreated` → `egl_.createSurface()` → **`ensureResources()`**
3. `RenderThread.cpp:499-500` — `ensureResources()` 在 `resourcesReady_ == false` 时**同步重建全部 GPU 资源**，其中包含：
   - `RenderThread.cpp:568-570` `generateHumanoid(...)` + `avatarMesh_.upload(...)`
   - `RenderThread.cpp:592-598` `anatomy::generateAnatomyUpperBody(...)` + `anatomyMesh_.upload(...)` → **设备端耗时 4130 ms**
   - `RenderThread.cpp:613-628` P1-A patch + wireframe
4. `RenderThread.cpp:377-387` — 渲染循环条件为 `eglReady_ && egl_.surfaceCreated() && resourcesReady_`；
   在上述 4.1 秒内 `resourcesReady_ == false`，因此 **既不调用 `drawFrame()` 也不调用 `eglSwapBuffers()`**
5. 而 Android 侧的 Activity 窗口此时已经可见，SurfaceFlinger 只能显示上一帧或空缓冲 → **黑屏 4.2 秒**

排除项：

| 假设 | 结论 | 依据 |
|---|---|---|
| EGL context 丢失导致 GL 对象失效 | **排除** | `EglContextManager.cpp:111-118` `destroySurface()` 只销毁 surface、保留 `context_`；恢复后日志中未再出现 `EGL initialized` / `GLES 3.0 EGL context created` |
| 崩溃 / ANR | **排除** | crash buffer 为空，无 FATAL EXCEPTION、无 ANR |
| 几何生成失败 | **排除** | `anatomy preview ready: 40984 tris` 正常产出 |
| `releaseGpuResources()` 漏释放 anatomy mesh/lines 导致失效 | **非直接原因** | `SkinnedGpuMesh::upload()` 首行即 `release()`（`SkinnedGpuMesh.cpp:19-21`），重上传会先释放旧对象；但该函数确实**没有**释放 `anatomyMesh_`/`anatomyLines_`（`RenderThread.cpp:636-643`），属于潜在资源生命周期隐患，需在架构演进中处理 |

**结论：黑帧 = 资源重建期间渲染线程阻塞（4.1 s）且窗口已可见，属「阻塞式重建」问题，而非崩溃或 GL 状态错误。**

## 4. 影响

- 用户可感知：从后台返回时黑屏约 4 秒（TV 大屏上更明显）。
- 若后续路线几何量更大（例如 R2/R3 全模板网格），该阻塞只会更长。
- 当前 `ensureResources()` 无条件重建 humanoid + anatomy + P1-A 三套几何，即使当前 previewMode 只需要其中一套，属无谓开销。

## 5. 修复方案（需批准）

| 方案 | 做法 | 风险 | 收益 |
|---|---|---|---|
| **A（推荐先做）** | 资源重建期间仍提交「清屏 + 背景」帧，避免黑窗 | 低，仅改渲染循环判定，不动几何逻辑 | 黑屏变背景，感知问题立即消失 |
| **B（推荐同步规划）** | 缓存 CPU 侧 MeshData，surface 恢复只重传 GPU buffer，不重算几何 | 中，需引入缓存与失效策略 | 恢复耗时从 4.1 s 降到毫秒级，符合「Geometry Worker / 不可变 MeshData / Upload Queue」目标架构 |
| **C（不在本次范围）** | 生成完全移出渲染线程 | 高，属架构重构 | 彻底解决阻塞 |

建议顺序：**A 先止血 → B 在 G1/G2 路线改造中一并落地（缓存不可变 MeshData + 上传队列）**。

## 6. 授权请求

- 方案 A 需要修改 `native/renderer/RenderThread.cpp` 的渲染循环判定，属**正式生产渲染路径**。
- AUTH-07 的授权范围是「R2/R3 最小实验」及其必要的测试与构建配置；本缺陷是既有渲染器的历史缺陷，不属于 R2/R3 实验自身问题。
- 按总指令「对超出授权范围的缺陷，提交负责人审批」，**本次未做任何代码修改**，等待负责人批准后再实施。

## 7. 本次诊断的边界

- 仅静态代码分析 + 设备只读复现与截图；未修改任何源码 / CMake / Gradle。
- 设备操作限于 HOME、启动、截屏、logcat；未清除数据、未改系统配置。
- 所有产物位于 Worktree `evidence/`；原工作区未触碰；未 Commit / Push。
