# P0 — 04 Test & Acceptance

| 验收项 | 结果 | 证据 |
|---|---|---|
| 基线 CTest | PASS | `evidence/ctest-baseline.txt`：8/8 套件通过，7.60s |
| 基线 Android 构建 | PASS | `evidence/gradle-assemble-baseline.txt`：BUILD SUCCESSFUL in 12s |
| 基线编译稳定化 | PASS | `evidence/cmake-build.txt` 初版失败（`trapRbBelly` 缺字段）→ 修复后 8/8 通过 |
| Shoulder 回归先红 | PASS（预期失败） | `evidence/shoulder-regression-before-fix.txt`：humanoid_tests 与 skinning_tests 均失败 |
| Shoulder 修复后全绿 | PASS | `evidence/ctest-after-shoulder-fix.txt`：8/8 通过，2.20s；新增 shoulder 语义、肩区权重、BoneId 21 等断言 |
| Retargeting 无回归 | PASS | motion_tests 中的抬臂/抬腿与低置信度测试在修复后通过（同上 8/8） |
| GPU Skinning 真机定性对照 | PASS（NOT visually accepted） | 历史 `M11-anatomy-visual-rebuild/evidence/frame-m11d-armraise.png` vs `evidence/frame-shoulder-fix-2.png`：抬臂动作下角色完整，无肩/肘塌陷；非像素级同机位对照，仅作定性回归 |
| 真机运行状态 | PASS | `evidence/logcat-after-shoulder-fix-2.txt`：GLES3.2/Mali-T860；surface ready；48 FPS，P50 20.86ms / P95 21.4ms / P99 ~22ms；无 shader 错误 |
| 状态文档唯一性 | PASS | 仓库仅 `docs/project-status.md`；重复大写副本已删除；分 Historical/Current/Roadmap |
| Git 安全 | PASS | 基线提交 399967f 已推送；未合并 main、未 force push；后续提交显式路径 |

## 未验证 / BLOCKED

- P0 Gate 用户确认：NOT VERIFIED。
- bindLocal 是否足以表达目标 A-Pose：PRELIMINARY，需完整链路审计；当前没有因“仅平移”而修改 Bind 系统。
- UV 是否需进入 GPU：PRELIMINARY，当前 shader 不使用 UV，不为结构完整改布局。
- 摄像头实时：BLOCKED（盒子无摄像头）；视频/摄像头分项验收见 `docs/project-status.md`。
- CPU↔GPU 逐点蒙皮数值对照：NOT VERIFIED，P1-C 补。
