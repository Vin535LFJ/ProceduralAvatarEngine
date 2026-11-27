# VR-1B WP2.1 实施报告 — APK 内置人体模板加载（AAssetManager 加法 JNI）

> 日期：2026-11-27
> 授权：负责人 2026-11-27 会话指示——"授权新增一个 AAssetManager 的加法 JNI 接口，让 APK 直接从 assets 加载 .paht 模板，不再依赖 adb push，保持现有 JNI 签名和冻结边界不变"。
> 前序：16 号报告（WP1/WP2，DEVICE_STATIC_RENDER_ACCEPTED）。
> 证据：`evidence/vr1b-wp2.1/`（device/ 7 张真机截图，logs/ 7 份）。

## 0. 一句话结论

**mode-4 现在不依赖任何外部文件**：APK 安装后直接从 assets 读取 `.paht`（4,054,264 字节，SHA-256 与编译资产一致），真机渲染完整人体；adb push 文件降级为**开发覆盖**（存在时优先，用于拒绝测试）。现有 JNI 签名与冻结边界零改动。

## 1. 设计（加法，单入口）

| 层 | 改动 |
|---|---|
| JNI（`app/src/main/cpp/jni_bridge.cpp`） | **新增一个**入口 `nativeLoadTemplateAsset(handle, AssetManager, path): Boolean`。在调用线程用 `AAssetManager_open(AASSET_MODE_BUFFER)` 读字节，经 `EngineRuntime::setTemplateBytes` 投递；**不保留 AAssetManager 句柄**，无跨线程生命周期问题。沿用 `nativeCameraCommand` 的加法先例注释 |
| Kotlin | `NativeBridge.loadTemplateAsset(assets, path)` 包装；`MainActivity.onCreate` 在 `create()` 后调用并记日志 |
| RenderThread | 新命令 `SetTemplateBytes`（RenderCommand 增加 `text` 成员，内部结构）；`ensureTemplateGeometry` 源选择：**开发文件存在 → 用文件（source=file）；否则用资产字节（source=asset）；都没有 → 拒绝**。日志与 stats 标明 source |
| 冻结边界 | 现有 17 个 JNI 签名、LegacyRigV1 / BoneId 0–21 / UBO[22] / 64B 打包 / mode-3 canonical 全部未动 |

资产路径：`assets/human_template/pae-human-base-mh-a8bc2d54.paht`（WP1 已打包，本轮未改资产内容）。

## 2. 真机验收（nunai_max，Mali-T860，GLES 3.2）

APK：64,821,849 B，SHA-256 `28222c72f8ac255122f6e8e80c51ac14a291073c8913fdfffe8320716c1f5778`。

| 测试 | 结果（日志原文在 evidence/logs/） |
|---|---|
| **独立加载**（先 `rm` 设备文件，确认 `No such file`） | `PAE_JNI: template asset loaded: human_template/pae-human-base-mh-a8bc2d54.paht bytes=4054264` → `human template ready: source=asset verts=13380 tris=26756 … sha256=d3bcbbe…417b`；front / 45° / wireframe 三张截图覆盖率 6.32 / 7.02 / 7.46% |
| **拒绝**（推送篡改文件作开发覆盖） | `human template rejected: payload hash mismatch source=file`；进程存活（pid 可查）；回退渲染正常 |
| **恢复 + 生命周期**（删除文件，HOME→resume） | `source=asset` 重新加载；surfaceDestroyed→Created 后 `geometry cache hit: human template (26756 tris)` |
| **Legacy 回归** | mode-0 正常（覆盖率 7.44%）；mode-3 canonical 正常（`canonical body ready: verts=46674 tris=93344 … gen_ms=62670`，覆盖率 7.23%） |
| Host 回归 | ctest **22/22**（pae_core 本轮未改，渲染线程为 Android 专属） |
| 内存 | dumpsys：native heap PSS ≈ 128 MB（mode-3 全缓存状态） |

## 3. 状态更新

```text
ASSET_PACKAGE_LOADING = VERIFIED（nunai_max 真机，APK 独立加载）
VR1B_WP1_WP2          = DEVICE_STATIC_RENDER_ACCEPTED（含 WP2.1）
```

16 号报告 §5 的负责人验收记录保持原文（历史快照）；其中 `ASSET_PACKAGE_LOADING = NOT_RUN` 由本轮取代。

## 4. 已知限制 / NOT RUN

| 项 | 状态 |
|---|---|
| 其他 Android 设备（手机） | NOT RUN |
| 资产字节常驻内存（约 4 MB） | 设计取舍：启动读一次，避免渲染线程持 AAssetManager；后续如需可改映射/流式 |
| 开发覆盖语义 | 设备上存在 `/data/local/tmp/pae-human-base.paht` 时优先于资产——仅开发/测试用途，正式分发无此文件 |
| Rig V2 动画 / 权重质量 / 最终视觉 Gate | 不变（NOT RUN / PENDING） |

## 5. 下一步

WP3（人体形态参数化）——按 15 号决策包另行批准。负责人建议顺序：WP2.1 ✓ → WP3 → WP4 Rig V2 → WP5 权重与动作质量 → WP6 生产 Render Mesh/材质。