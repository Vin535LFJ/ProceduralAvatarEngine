# M2 — 01 需求

- FUN-RND-001：创建 GLES 3.0 Context、能力检测、绘制程序化三角形与 3D primitive。
- COMP-001：GLES 3.0 检测，不支持时明确报错并安全停止。
- COMP-002：Surface 创建/变更/销毁与 Context 重建。
- REL-001 / OBS-001：GL 调用仅在 Render Thread；命令队列有界；记录 GL 字符串、Shader 错误完整源码、RenderStats。
- 验收：真机 logcat 有 GL 字符串与绘制统计；截图有 Native GL 画面；反复 Surface 重建不崩溃。
