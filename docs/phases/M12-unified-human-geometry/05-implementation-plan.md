# M12 — 05 实施计划

| 任务 | 内容 | DoD |
|---|---|---|
| T1 AnatomyField | HumanAnatomyParameters + 区域场函数 + smooth 组合 | 场单测（各区域符号正确/参数生效） |
| T2 Surface Nets 扩展 | 各向异性分辨率 + 场回调 | 网格有效/确定性 |
| T3 MeshTopology | 拓扑分析器 | SDF 网格 components=1/boundary=0 断言 |
| T4 HumanAnatomyGenerator | 场→网格→区域权重→Anatomy Report | 生成耗时/顶点数记录；权重合法 |
| T5 Preview Mode | RenderThread 双网格/相机预设/调试视图/JNI/UI | 真机 8 视角 × 3 模式截图 |
| T6 PoC 对比渲染 | 方案 A（M11 上半身）vs 方案 B（SDF）同相机截图 | 对比数据 + ADR-009 定案 |
| T7 验收与归档 | ctest/构建/真机/报告/合并 | 全部证据归档 → 停止等用户验收 |

顺序 T1→T2→T3→T4→T5→T6→T7；每任务 ctest + 提交。
