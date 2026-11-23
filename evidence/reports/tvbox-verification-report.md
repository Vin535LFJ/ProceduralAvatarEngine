# TV Box 验证报告（AUTH-05）

> 日期时间：2026-11-23 18:49–18:55 GMT+8
> Worktree：`E:/AI-Tools/ProceduralAvatarEngine-PoC`
> Git SHA：`99941a94ec58fb37ab0f1a36b90590d3fdce5198`
> 设备：`nunai_max`（rockchip rk30board，Android 8.1.0 / SDK 27，arm64-v8a）
> SDK 路径：按负责人指定使用 `E:\AndroidSdk`
> 结果：**安装成功、启动成功、无崩溃、GLES 3.0 渲染正常、发现 1 项渲染恢复缺陷**

---

## 1. 设备身份

| 项 | 值 |
|---|---|
| 序列号 | `MAX0019111000104` |
| model / device | `nunai_max` / `nunai_max` |
| 制造商 / 硬件 | rockchip / `rk30board` |
| Android | 8.1.0（SDK 27） |
| ABI | `arm64-v8a`（abilist: arm64-v8a, armeabi-v7a, armeabi） |
| GPU | ARM **Mali-T860** |
| GLES | **OpenGL ES 3.2** v1.r18p0-01rel0（满足 `glEsVersion 0x00030000` 要求） |
| EGL | EGL 1.4 Midgard-r18p0-01rel0 |
| 分辨率 | 1920×1080，density 240 |
| ADB | 1.0.41 / 36.0.0-13206524（`E:\AndroidSdk\platform-tools\adb.exe`） |
| 设备识别方式 | `adb devices -l` → `product:nunai_max model:nunai_max`，唯一在线设备 |

## 2. APK 来源确认

| 项 | 值 |
|---|---|
| APK 路径 | `E:/AI-Tools/ProceduralAvatarEngine-PoC/app/build/outputs/apk/debug/app-debug.apk` |
| APK SHA256 | `2c47990bad7fd9753344ff4c18a77412872e473373a50e3eb8a393925c9a5a44` |
| 大小 | 59,113,740 bytes |
| 构建 SDK | `E:\AndroidSdk`（本次按负责人指定路径 clean rebuild） |
| 安装命令 | `adb install -r <apk>` |
| 安装结果 | `Performing Streamed Install` / `Success`（`evidence/tvbox/adb-install.log`） |

设备上原有 `com.pae.host`（lastUpdateTime 2026-11-23 11:54）。`-r` 覆盖安装，PackageManager 日志显示 `Retaining data and using new`，**应用数据被保留**，未清除数据。

## 3. 启动

| 项 | 值 |
|---|---|
| 命令 | `adb shell am start -W -n com.pae.host/.MainActivity` |
| 结果 | `Status: ok`，`ThisTime/TotalTime 1015 ms`，`WaitTime 1037 ms`，`Complete` |

## 4. 渲染与运行时日志（`evidence/tvbox/logcat-pae.txt`）

```
PAE_JNI   : runtime created
PAE_EGL   : EGL initialized version 1.4
PAE_EGL   : GLES 3.0 EGL context created
PAE_UI    : debug extras: anatomy=false preset=0 view=0
PAE_RENDER: anatomy preview ready: 40984 tris, 4120 ms
PAE_RENDER: P1A-REPORT verts=870 tris=1664 components=1 boundary=80 nonmanifold=0 degenerate=0 badnorm=0 gen_ms=2.381 boundaries=4
PAE_RENDER: surface ready 1920x1008
```

- GLES 3.0 EGL context 创建成功。
- 解剖预览几何：40984 triangles，**设备端生成耗时 4120 ms**（重启后 4125 ms）。
- P1-A 路径：870 verts / 1664 tris / 1 component / 80 boundary edges / 0 non-manifold / 0 degenerate / 0 bad normals，`gen_ms=2.381`。

## 5. 性能实测（本次设备实测，非历史数据）

`PAE_UI: frames=2983 fps=54.6 frame_ms=18.30 draw_calls=4 triangles=14064 buffer_bytes=688960 p50_ms=20.93 p95_ms=21.21 p99_ms=21.57`

| 指标 | 值 |
|---|---|
| 累计帧数 | 2983 |
| FPS | 54.6 |
| 平均帧耗时 | 18.30 ms |
| p50 / p95 / p99 | 20.93 / 21.21 / 21.57 ms |
| Draw calls | 4 |
| 三角形数 | 14064 |
| GPU buffer | 688,960 bytes |

## 6. 稳定性

- `logcat -b crash` 输出为空。
- 全量 logcat（`evidence/tvbox/logcat-session.txt`，655 KB）中无 `FATAL EXCEPTION`，无 `libEGL` 错误，无 ANR。
- 安装、启动、几何生成、8 段相机切换、3 段 debug view 切换、HOME 退出再进入，全程无崩溃。

## 7. 截图证据（`evidence/screenshots/`）

| 文件 | 内容 |
|---|---|
| `tvbox-launch-01.png` | 启动默认视图：带皮肤/服装的 humanoid，按钮栏完整 |
| `tvbox-anatomy-on-01.png` | 切换 ANATOMY 预览（bind pose，neutral material） |
| `tvbox-anatomy-wire-02.png` | debugView=1 wireframe |
| `tvbox-anatomy-normals-03.png` | debugView=2 normals |
| `tvbox-anatomy-clay-04.png` | debugView=3 gray clay，完整躯干/肩/臂 |
| `tvbox-anatomy-clay-preset6.png` | cameraPreset=6 SHOULDER CLOSEUP |
| `tvbox-anatomy-clay-preset7.png` | cameraPreset=7 TORSO CLOSEUP |
| `tvbox-relaunch-05.png` | **HOME 后重新进入的第一帧：渲染区全黑** |
| `tvbox-relaunch-06-late.png` | 同一时刻稍后（约 1 分钟后）：角色已恢复正常渲染 |

### 7.1 发现的渲染缺陷

**EGL 恢复路径存在一帧黑屏。** 按 HOME 后日志出现 `PAE_EGL: EGLSurface destroyed`；重新进入后日志出现 `anatomy preview ready` 与 `surface ready 1920x1008`（几何已重新生成），但紧接其后抓取的第一张截图为**纯黑渲染区**（按钮栏仍正常），约一分钟后再次抓取则完全恢复正常。

判定：恢复路径存在**瞬时黑帧**，非持续故障。需在 G1/G2 前查明是首次上传时序问题，还是与 `releaseGpuResources()` 未释放 anatomy mesh/lines 有关（`RenderThread.cpp:636-643`）。标记为 **缺陷待查**，不是本次可判定为 PASS 的项。

### 7.2 灰模形态观察（仅记录，不作视觉验收）

- 躯干与肩部过渡整体连续，无穿模、无破面、无大面积塌陷。
- **颈根/胸骨上窝处存在可见的尖刺状凸起与褶皱**（多个 preset 中一致出现）。
- **胸骨下端附近有一个小的暗色凹陷/破口状区域**。
- 手臂为独立圆柱段式形态，肘部有明显分段感；髋/腹部体积偏球形，与 DEC-01 的轻度运动型基础体型目标差距明显。
- 以上属当前 R1/解剖 SDF 路线的既有表现，**不构成对 PoC-A 的判定**，仅作为 G1/G2 对照基线。

## 8. 未验证 / 未覆盖

| 项 | 状态 |
|---|---|
| CPU/GPU skinning parity 数值对比 | `UNVERIFIED`（需专门 parity 测试） |
| Skinning 权重合法性（设备端） | 未验证，仅有 Host 侧 `pae_skinning_tests` 通过 |
| cameraPreset 0–5 截图 | 未采集（本次仅采 6、7 与默认视图） |
| 帧率长时间稳定性 / 温控降频 | 未测（单次 STATS 采样） |
| EGL 多次重建压力 | 未测（仅一次 HOME 往返） |
| 其他设备（手机 / 桌面） | 未测，符合 DEC-03「OPEN D」 |
| 相机与视频输入路径 | 未测（本次只做渲染与几何验证） |

## 9. 边界与授权使用

- 使用授权：AUTH-05（设备只读信息、安装、启动、日志、截图）、AUTH-06（证据文件）。
- 设备侧实际操作：`adb devices` / `getprop` / `dumpsys SurfaceFlinger` / `wm size` / `uiautomator dump`（写入 `/data/local/tmp/pae_dump.xml`）/ `install -r` / `am start` / `input tap` / `input keyevent 3` / `screencap` / `logcat -d`。
- **未执行**：清除应用数据、恢复出厂设置、修改系统配置、卸载或改动其他应用数据。
- 所有产物仅写入 Worktree `evidence/`；原工作区未触碰；源码 / CMake / Gradle 未修改；未 Commit / Push。
