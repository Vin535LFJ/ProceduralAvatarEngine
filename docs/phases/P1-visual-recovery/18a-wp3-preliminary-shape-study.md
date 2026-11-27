# VR-1B WP3 Host 研究报告 — 人体形态参数化（参数 → Shape Target 映射）

> 日期：2026-11-27
> 授权：负责人 2026-11-27 会话指示——"授权启动 WP3 人体形态参数化，先在 Host 上做独立研究……暂不改 Android"。
> 范围：**纯 Host**。本轮未修改任何 Android 代码、JNI、渲染管线。
> 工具：`tools/human-template-poc/host/src/wp3_shape_study_main.cpp`（`pae_wp3_shape_study`，host-only）。
> 证据：`evidence/vr1b-wp3-host/`（20 张图 + 3 份生成报告 + 运行/构建日志）。

## 0. 一句话结论

9 个已验证参数到 Shape Target 的映射在 Host 上跑通：**模板内 18 个 target 与固定 commit 资产逐值等价（18/18）**；按校准斜率从人体测量请求反解 α，生成 5 个命名体型，**全部 0 翻面**、边拉伸 p99 ≤ 1.215。两个关键限制被实测暴露：**±1 authored 范围钳制了可达幅度**，且**组合效应非加性**（逐参数线性校准在组合下偏差可达数毫米到厘米级）。

## 1. 模板作为形态来源的等价性

`pae-human-base-mh-a8bc2d54.paht`（v3，SHA-256 `d3bcbbe…417b`）内 18 个 shapeTargets 与 `ProofContext` 从固定 commit 资产加载的 target **逐顶点逐位移完全一致**：18 matched / 0 mismatched。Android WP3 后续可直接以模板为形态来源（无需运行时接触 MakeHuman 文件）。

## 2. 参数校准（α = ±1 实测，非对称斜率）

测量口径：PAE proof 平面（ht_measure.h），每个变体在变形后的表面上重新解析关节再测量。**不是** ANSUR 验收口径（见 §5）。

| 参数 | 命名测量 | base (m) | +1 斜率 (m/α) | −1 斜率 (m/α) | 翻面 +/− |
|---|---|---:|---:|---:|---|
| shoulderWidth | bideltoid width | 0.5595 | +0.0498 | −0.0270 | 0/0 |
| ribcageWidth | chest width | 0.2757 | +0.0550 | −0.0550 | 0/0 |
| chestDepth | chest depth | 0.2005 | +0.0557 | −0.0302 | 0/0 |
| waistWidth | waist width | 0.2971 | +0.0168 | −0.0168 | 0/0 |
| hipWidth | hip width | 0.3699 | +0.0590 | −0.0294 | 0/0 |
| stature | stature（macro） | 1.6659 | **+0.7225** | **−0.3664** | 0/0 |
| armShape | 代理：上臂平均位移 | — | +3.39 mm | — | 0/0 |
| legShape | 代理：大腿平均位移 | — | +3.50 mm | — | 0/0 |
| headShape | 代理：头部平均位移 | — | +9.07 mm | — | 0/0 |

要点：斜率普遍非对称（incr/decr 是不同 target）；stature 是 macro target，+1 高达 +0.72 m，**MakeHuman 未验证其单独使用的人体测量正确性**（本研究按请求小幅度使用并实测质量）。

## 3. 命名体型：请求 vs 达成

α 由请求增量除以对应方向斜率反解，钳制到 [−1, +1]（钳制已标记）。完整表见 `report/body-types.md`。

| 体型 |  stature 请求/达成 | bideltoid 请求/达成 | waist 请求/达成 | 翻面 | 边 p99 | 体积比 |
|---|---|---|---|---:|---:|---:|
| base | — / 1.6659 | — / 0.5595 | — / 0.2971 | 0 | 1.000 | 1.000 |
| tall-athletic | +0.080 / **+0.080** | +0.060 / +0.081 (!) | −0.020 / −0.008 (!) | 0 | 1.214 | 1.109 |
| petite | −0.100 / **−0.100** | −0.040 / −0.052 (!) | — | 0 | 1.140 | 0.885 |
| heavy-set | — | — | +0.080 / +0.023 (!) | 0 | 1.215 | 1.259 |
| v-taper | — | +0.080 / +0.075 (!) | −0.050 / −0.019 (!) | 0 | 1.211 | 0.963 |

(!) = 达成偏离请求 > 5 mm。来源有二：
1. **范围钳制**：shoulderWidth +60 mm 需 α≈1.2，超出 authored +1（最大 +49.8 mm）；waistWidth −20/−50 mm 需 α≈−1.2/−3.0，超出 −1（最大 −16.8 mm）。
2. **组合非加性**：stature 增大会连带抬高/加宽 bideltoid；hipWidth/ribcageWidth 会移动 waist 平面上的顶点。逐参数线性校准不能精确预测组合结果（tall-athletic 的 bideltoid 实际 +81 mm > 请求 60 mm）。

## 4. 视觉证据

20 张：5 体型 × 3 视角（front / front-45-left / side-left，共享相机帧可直接对比）+ 5 张正面线框。图像描述模型确认五者均为完整人体、体型区分明显、无黑洞/折叠（**仅供参考，负责人人眼验收为准**）。注意：host 软件渲染器用平面法线（Newell），预览有棱面感；Android 路径用平滑顶点法线，二者不可互相推断。

## 5. 口径与未验证项（如实记录）

| 项 | 状态 |
|---|---|
| 测量口径 | PAE proof 平面（研究口径）；base mesh stature 1.6659 m ≠ CharacterDefinition 的 ANSUR II 男性 p50 1.7549 m。**对齐验收口径（R7 条件 5）是 WP3-Android 前置，未做** |
| 变形后自相交 | NOT RUN（本轮用翻面/边拉伸/体积比；自相交检查留待接入 PAE 检查器） |
| MakeHuman authored slider 范围 / macro 组合（性别/年龄/肌肉/体重） | NOT RUN |
| Android 集成 | NOT RUN（本轮授权明确暂不改 Android） |
| 模板内关节-顶点分组 | 缺口：模板有关节 landmark 与 aux 位置，但无 joint→顶点映射；Android 上 rig 随形态重解析需要它（WP3-Android 前置） |

## 6. 对 WP3-Android 的建议（需另行授权）

1. **范围策略**：±1 不够覆盖常见人体测量请求。选项：(a) 受控 overshoot（>1 需逐组合质量验证，原型 +1.5 数据可参考）；(b) 引入更多已验证 CC0 target（如围度类）；(c) 接受钳制并在 UI 标注可达范围。建议先 (c)+(a) 小步验证。
2. **组合求解**：精确人体测量目标需要迭代求解或实测查找表，不能用逐参数线性外推。
3. **模板补全**：把 joint→顶点分组写入模板（或从 aux 几何推导），使 Android rig 能随形态重解析。
4. **口径对齐**：建立 proof 平面 ↔ ANSUR/CharacterDefinition 的换算，再谈验收数值。