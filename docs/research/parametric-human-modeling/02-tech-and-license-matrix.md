# 成熟人体模型技术与许可证矩阵

文件：`docs/research/parametric-human-modeling/02-tech-and-license-matrix.md`
任务：P1-VR / Deliverable R2
日期：2026-11-25
状态：只读研究记录；不是法律意见。未导入任何外部模型、形态数据或训练数据。

## 1. 判定口径

必须区分：

1. 源代码许可证；
2. 预训练模型/基础网格许可证；
3. 训练数据许可证；
4. 形态目标/morph/blendshape 数据许可证；
5. 论文、算法和专利；
6. 商业使用；
7. 再分发。

“代码开源”不等于“模型可商用”，“模型可下载”不等于“训练数据可再分发”。无法核实的项目标记为 `LEGAL REVIEW REQUIRED`，不得进入 Android 或交付包。

## 2. 许可证矩阵

| 项目 / 核查版本 | 源代码 | 预构建模型/基础网格 | 训练数据 | Morph / target / blendshape | 论文/算法 | 商业使用 | 再分发 |
|---|---|---|---|---|---|---|---|
| MakeHuman `a8bc2d54ff0ac92e78ff71431b1023eda42bf482` | AGPL-3.0 | 官方 base/proxies/outputs 为 CC0；第三方资产另查 | 未发现独立 raw corpus 许可：**LRR** | bundled targets/modifiers 为 CC0；第三方 target 另查 | 未发现独立论文/算法专利授权：**LRR** | 代码可在 AGPL 下商用；CC0 资产可用 | 代码有 copyleft/source 义务；CC0 资产无限制 |
| MPFB2 `650c5c53bd6989a8ac264f31fa1d11d4a0c7bd8a` | GPL-3.0 | core/output CC0；packs 分别许可 | 未发现独立 corpus 许可：**LRR** | bundled targets/rigs/expressions/mesh JSON 为 CC0；外部 packs 逐包核查 | 未发现独立论文许可：**LRR** | GPL 下可商用；core 资产可用 | 代码有 source 义务；CC0 无限制；CC-BY pack 需署名 |
| SMPL-X CVPR 2019；代码 `1265df7ba545e8b00f72e7c557c766e15c71632f` | 官方 model/software/scripts 为非商业研究许可 | full model 非商业；SMPL-X Body 子集 CC BY 4.0 | scans/MoMoCap 被提及；raw data grant 未公开：**LRR** | full shape blendshapes 非商业/禁止分发；Body pose/dynamic blendshapes CC BY 4.0，但不含 shape blendshapes/tools | 有引用要求；SMPL 专利被官方标注：**LRR** | full model 需单独商业许可；Body 子集按 CC BY | full model 禁止再分发；Body 按 CC BY |
| FLAME 2017–2023；2023 Open | 官方 academic add-on/software 为非商业研究；第三方实现按各自许可 | 2017/2019/2020/2023 非商业/禁止分发；2023 Open 为 CC BY 4.0 | D3DFACS registrations 非商业/禁止分发；raw scans：**LRR** | academic blendshapes 非商业/禁止分发；2023 Open model 为 CC BY 4.0；texture 为 CC BY-NC-SA 4.0 | SIGGRAPH Asia 2017 引用；专利范围：**LRR** | 仅 2023 Open model 明确允许；texture 不允许商业 | academic 禁止分发；Open 需署名；texture NC-SA |
| MANO v1.2 | 下载 code/data 为非商业研究/教育/艺术 | 非商业/禁止分发 | 约 1,000 scans / 31 subjects 被描述；raw scans：**LRR** | pose blendshape corrections 属于非商业数据/软件 | SIGGRAPH Asia 2017 引用；专利：**LRR** | 无单独许可不可商用 | 禁止再分发 |
| GHUM/GHUML CVPR 2020；repo `20fadc5c985631044ccc047815cbd0300918e690` | 仓库代码 Apache-2.0 | request form；模型条款未在 README 公开：**LRR** | >60,000 scans / 60 people 被描述；许可：**LRR** | gated model 内容/条款：**LRR** | Apache 代码含 contributor patent grant；模型权利：**LRR** | 代码可商用；模型待法律核查 | 代码按 Apache；模型待核查 |
| MB-Lab 1.8.1 `063bff04e60f3e7c651fda628c30f5d83f3f3078` | GPL-3.0 | `.blend`/database 与生成模型为 AGPL-3.0 | 未发现独立训练 corpus/provenance 许可：**LRR** | JSON morph deltas 为 AGPL-3.0 | 未发现独立论文许可；GPL/AGPL patent clauses | copyleft 下可商用；闭源分发冲突：**LRR** | 需履行 source/AGPL 义务；普通 2D renders 可商用，但官方禁止以渲染进行 reverse engineering/texture extraction |
| OpenSim core `c594ccb9c789ac1451ce4961ecd563b6712b2d93`；models `d9b05d470b1a481c222372c85b75772faf8f7792` | Apache-2.0 | 逐模型许可，例如 Gait2392 CC BY 3.0；Rajagopal 许可缺失：**LRR** | 无统一 corpus；trial/model data 逐项核查 | 不适用统计 avatar morph；measurement/target 按 model/data 条款 | Apache 代码/patent grant；第三方算法/论文另查 | 代码可商用；模型逐项判断 | 代码保留 notice；模型逐项判断 |

## 3. PAE 使用决策

| 材料 | PAE 决策 |
|---|---|
| MakeHuman/MPFB2 `.target` 或官方人体网格 | **不导入**。只学习语义参数 → 局部目标组织方式 |
| SMPL-X full model / shape blendshapes | **不导入**。只学习 shape/pose 分离、joint regression、LBS 数学 |
| FLAME 模型/纹理/expression data | **不导入**。只学习固定面部拓扑、眼睑/眼球分离和 jaw 表达 |
| MANO 模型/pose basis | **不导入**。只学习手部固定拓扑和局部 corrective 思想 |
| GHUM 模型/训练数据 | **不导入**。只学习区域 mask、Top-4 权重和区域耦合 |
| MB-Lab 数据库/morph JSON | **不导入**。只学习配置与生成分离、参数分组 |
| OpenSim 模型/数据 | **不导入**。只学习 landmark、joint frame、measurement 语义 |
| 通用数学方法 | 可在 PAE 中独立实现，但需记录来源与推导，不复制受限资产 |

## 4. 许可证风险结论

- MakeHuman/MPFB2 的代码与核心 CC0 资产许可较清晰，但 PAE 仍不直接使用其网格和 target，以保持自主生成边界。
- SMPL-X、FLAME、MANO 的 full model/data 许可明确限制商业和再分发，不能作为 Android 运行资产。
- GHUM 代码为 Apache-2.0，但模型和训练数据条款不明确，必须法律核查。
- MB-Lab 的 GPL/AGPL 与闭源 Android 分发可能冲突，不引入代码或数据库。
- OpenSim 代码许可清晰，但具体模型/data 需逐项核查；PAE 只复用解剖语义。
- 任何未来外部参考素材，即使只用于研究，也必须单独记录版本、许可、用途和是否进入交付包。

## 5. 证据来源

### MakeHuman / MPFB2

- [MakeHuman LICENSE.md](https://github.com/makehumancommunity/makehuman/blob/a8bc2d54ff0ac92e78ff71431b1023eda42bf482/LICENSE.md)
- [MakeHuman LICENSE.CODE.md](https://github.com/makehumancommunity/makehuman/blob/a8bc2d54ff0ac92e78ff71431b1023eda42bf482/LICENSE.CODE.md)
- [MakeHuman LICENSE.ASSETS.md](https://github.com/makehumancommunity/makehuman/blob/a8bc2d54ff0ac92e78ff71431b1023eda42bf482/LICENSE.ASSETS.md)
- [MPFB2 LICENSE.md](https://github.com/makehumancommunity/mpfb2/blob/650c5c53bd6989a8ac264f31fa1d11d4a0c7bd8a/LICENSE.md)
- [MakeHuman Community split-license](https://static.makehumancommunity.org/about/license.html)

### SMPL-X / FLAME / MANO / GHUM

- [SMPL-X model license](https://smpl-x.is.tue.mpg.de/modellicense.html)
- [SMPL-X Body license](https://smpl-x.is.tue.mpg.de/bodylicense.html)
- [FLAME model license](https://flame.is.tuebingen.mpg.de/modellicense.html)
- [FLAME data license](https://flame.is.tuebingen.mpg.de/datalicense.html)
- [FLAME texture license](https://flame.is.tuebingen.mpg.de/texturelicense.html)
- [MANO license](https://mano.is.tue.mpg.de/license.html)
- [GHUM README](https://github.com/google-research/google-research/blob/20fadc5c985631044ccc047815cbd0300918e690/ghum/README.md)
- [google-research Apache-2.0](https://github.com/google-research/google-research/blob/20fadc5c985631044ccc047815cbd0300918e690/LICENSE)

### MB-Lab / OpenSim

- [MB-Lab license.txt](https://github.com/animate1978/MB-Lab/blob/063bff04e60f3e7c651fda628c30f5d83f3f3078/license.txt)
- [MB-Lab Asset Licensing](https://github.com/animate1978/MB-Lab/wiki/Asset-Licensing)
- [OpenSim LICENSE.txt](https://github.com/opensim-org/opensim-core/blob/c594ccb9c789ac1451ce4961ecd563b6712b2d93/LICENSE.txt)
- [OpenSim model catalog](https://opensimconfluence.atlassian.net/wiki/spaces/OpenSim/pages/53090607)
- [Rajagopal model README](https://github.com/opensim-org/opensim-models/blob/d9b05d470b1a481c222372c85b75772faf8f7792/Models/Rajagopal/README.txt)
