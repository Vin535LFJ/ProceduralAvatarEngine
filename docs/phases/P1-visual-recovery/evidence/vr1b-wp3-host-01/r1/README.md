# WP3-H-R1 复测证据（审查修正轮）

> 本目录是 **VR-1B-WP3-HOST-R1**（人体参数化实测结果复核与修正）的复测证据。
> 上级目录 `../data/` 与 `../images/` 是 **WP3-HOST-01 原始实验记录，原样保留，未被覆盖**。
> 本目录 `r1/` 存放修正后重跑的结果。凡例：`r1/data/X` 取代 `../data/X` 的口径时，下表显式标注。

## 基线

- 分支 `poc-sm/shape-model-v2`，审查基线 HEAD `56c5f9f8a9fb3d5017ab3845eeee05ef1a79c23c`。
- 工具代码提交 `d7e83ee`（WP3-HOST-01）；本轮 R1 修改仅发生在 `tools/human-template-poc/host/`（4 个源文件），未 commit/push（授权要求）。
- Base mesh SHA-256 `8e761e6624b8f54536409135d1636da63b32486a90d4897f84e121d144f6fb4c`；分发模板 `.paht` v3 SHA-256 `d3bcbbece3db11c0df79cb7202fdf1730b325108509d60ad9faa209a8a5e417b`（与 WP1/WP2 已验证资产一致，未改）。

## 本轮修正与对应证据

| 问题 | 修正 | R1 证据 | 取代的原始记录 |
|---|---|---|---|
| P0-1 身高缩放锚点错误（`D3{sole,sole,sole}`） | pivot=(rootX=0 矢状对称面, soleY, rootZ=root 骨 head) | `data/anchor-study.md/.csv`（9 项检查 × 4 身高全 PASS） | `../data/height-study.md/.csv` 的 B 节均匀缩放行（旧结果含 ~5.68 cm 水平漂移） |
| P0-2 shorter-pae “翻面 2” 矛盾 | 查明：CSV 的 `2` 是 **degenerate** 列，flipped_faces 实为 **0**；2 个退化三角形源于检查器绝对阈值+缩小 | `data/flip-investigation.md`（face 320/6947、法线点积 1.0、面积前后对比、根因） | 澄清 `../data/variants-quality.csv` 的列读法；flips 口径三处统一为 0 |
| P1-1 联动矩阵“平均位移”语义 | 拆分 affected/total/coverage/meanAffected/meanAll/p95/max | `data/linkage-matrix.md/.csv`（修正语义） | `../data/linkage-matrix.md/.csv`（旧 mean 仅 affected-only，不得当作 whole-region） |
| P1-2 截面解剖验证 | largestArea 候选 + 躯干质心空间确认 + 环统计 + 可视化 | `data/section-validation.md/.csv`（63 截面 0 NOT_VERIFIED）、`images/section-base-*.png` | 新增；`../data/measurements-protocol.md` 的躯干环口径由此验证 |

## 复测全量结果（`logs/`）

- `wp3-tests.log`：**33 passed / 0 failed**（含 6 项联动语义新测试 + 截面验证测试）。
- `ht-tests.log`：**65 passed / 0 failed**（原有 Host 全量）。
- `engine-ctest.log`：**22/22**（WP1 模板格式/加载、WP2 往返/哈希、Legacy 回归）。
- `experiment.log`：10 变体 + 身高专项 + 150 图 + 63 截面，全部 flips=0 / selfInt=0；15.8 s。

## 图像

`images/` 153 张 = 150 张变体多视角（10 变体 × 5 视角 × 3 模式，共享相机帧）+ 3 张 base 胸/腰/髋截面可视化。
**缩放变体（taller-pae / shorter-pae / combo-tall-heavy）的图已因锚点修正重生成**（消除水平漂移）；非缩放变体几何不变。
全部经 `recompress_png.py` 无损重压缩（像素级往返断言通过），94.3 MB → 2.62 MB。

## 未改 / 未验证

- Android 生产路径（app/、native/renderer/、native/runtime/、native/procedural/human_template/、JNI、PAHT v3、LegacyRigV1、UBO[22]、64B packing、mode-3/4）：**零改动**（git diff 证明）。
- `native/procedural/MeshTopology.cpp:45` 的退化绝对阈值：只读复用，**未修改**（白名单外）；尺度相关特性已记录，建议 WP3-Production 改用尺度相对阈值。
- 峰值内存：Cygwin 下采样不可用，NOT MEASURED（如实记录）。