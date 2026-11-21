# ADR-007 无 DocumentsUI 设备的文件选择降级策略

- 状态：已采纳（2026-11-21）
- 关联：M7 实测发现、M8 视频输入、技术规范 §3.4（本地视频）、ADR-006

## 背景

技术规范要求本地视频"使用 Android 系统文件选择器获取用户选择的视频 URI"。M7 真机实测发现目标基准设备（nunai_max 电视盒子，Android 8.1 leanback-only）**不存在系统 DocumentsUI**：`ActivityResultContracts.OpenDocument` 抛出 `ActivityNotFoundException: No Activity found to handle Intent { act=android.intent.action.OPEN_DOCUMENT }`。

## 问题

若无降级路径，M8 视频输入在目标盒子上完全不可用；同时不能违反"禁止把示例视频打包成应用资产"的约束。

## 候选方案

1. 仅支持 SAF —— 目标盒子不可用，放弃。
2. 硬编码读取固定目录 —— 违背"用户选择"语义与隐私边界，放弃。
3. **SAF 主路径 + 应用内存储浏览器回退**（采纳）—— 手机/有 DocumentsUI 设备走 SAF；盒子走 `StoragePickerDialog`（列目录导航，按扩展名过滤），需 `READ_EXTERNAL_STORAGE` 运行时权限。

## 决定

采用方案 3。文件始终来自用户经 UI 的显式选择，应用自身不打包、不下载任何媒体资产，零外部美术资产约束不受影响。

## 影响

- 新增 `StoragePickerDialog`（M7 已落地）与存储权限流。
- M8 视频选择复用同一回退链。
- 风险：READ_EXTERNAL_STORAGE 在 API 33+ 手机上失效——届时 SAF 主路径可用，回退路径仅面向 ≤API 32 的盒子设备；如需覆盖 33+ 无 DocumentsUI 场景需引入 MediaStore 查询（backlog）。

## 回滚

回退路径为独立组件，移除后 SAF 路径不受影响。
