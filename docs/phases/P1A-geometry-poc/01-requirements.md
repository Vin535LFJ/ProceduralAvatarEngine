# P1-A — 01 Requirements

| ID | 要求 | 证据 |
|---|---|---|
| P1A-GEO-1 | Rig-aware Anatomical Landmarks 驱动几何 | `AnatomicalLandmarks` 由 BodyJoints 派生几何与骨骼共用控制点 |
| P1A-GEO-2 | Topology Template + Patch Boundary 契约 | boundary id / 环顶点数 / winding / C0 / G1 测试报告 |
| P1A-GEO-3 | Variable-profile Sweeps，非圆截面、宽深独立、PTF 稳定 | 参数改变形状测试、PTF 连续性测试、法线视图 |
| P1A-SURF-1 | 胸腔/腹/腰/肩胸/上臂/肘/前臂连续自然 | 正面/侧面/45°/肩肘特写灰模截图 |
| P1A-MOT-1 | 中性姿态与关节动作 | 至少 idle + elbow bend 动作截图 |
| P1A-CMP-1 | 与 M11/M12 同机位对照 | 相同 preset 0..7 的新旧截图 |
| P1A-DEV-1 | 真机 GLES3 运行 | logcat、构建输出、FPS/P50/P95/P99、gen_ms、tris/verts |
| P1A-GATE | 用户视觉验收 | 状态 NOT VERIFIED，提交后等待确认 |
