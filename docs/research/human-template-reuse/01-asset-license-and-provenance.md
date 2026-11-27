# R1：资产许可证与来源审计

任务：P1-VR-ARCH-1 / Deliverable R1  
日期：2026-11-26（合并版）  
状态：只读核查 + 工具生成的逐文件清单。**不是法律意见。** 未把任何第三方资产复制进仓库、APK 或生产源码目录。

> 本文件合并了两条并行证据链（下称链 A、链 B）各自的 R1。两份原文对这 13 个共同文件的 Git blob 与 SHA-256 逐行一致（脚本复核 13/13）。所有哈希由脚本生成，见 `evidence/host-proof/provenance/`。

## 1. 结论

1. MakeHuman 官方仓库在同一份 `LICENSE.md` 里把**软件代码（AGPL）**和**随软件发布的资产（CC0 1.0 Universal）**分开授权。基础网格 `base.obj` 和 `.target` 文件各自带有 CC0 文件头，属于明确列举的 CC0 资产。
2. 骨架定义 `default.mhskel` 和权重 `default_weights.mhw` **没有被 `LICENSE.md` §C 逐项列举**。它们在文件内部声明 `"license": "CC0"`，也符合 §C 的一般定义（“any data contributing to the graphical output”），但这一条依据弱于网格和 target。两条链原先的标签不一致（链 A 写“需确认”，链 B 写 `LICENSE_VERIFIED`），复核后**统一降为 `PENDING_CONFIRMATION`（待确认，未验证）**，工具生成的清单与模板内的来源字符串同步修改。负责人/法务确认之前，这两个文件只能用于 host 上的评估，不得进入正式运行时分发。
3. 两条链都没有复制、移植或链接任何 MakeHuman（AGPL）或 MPFB2（GPL-3.0）代码。PAE 原型只读取 CC0 数据文件；MPFB2 仅作为格式文档参考。
4. 社区资产库、第三方下载、未带 CC0 文件的任何资源一律未使用。

## 2. 固定版本与获取方式

| 项 | 值 |
|---|---|
| 仓库 | `https://github.com/makehumancommunity/makehuman` |
| Commit | `a8bc2d54ff0ac92e78ff71431b1023eda42bf482`（GitHub API：提交时间 2024-06-26T08:28:37Z） |
| 固定 URL 形式 | `https://github.com/makehumancommunity/makehuman/blob/a8bc2d54ff0ac92e78ff71431b1023eda42bf482/<仓库路径>` |
| 归档 | `codeload.github.com/.../zip/<commit>`，45,251,153 字节，SHA-256 `1604d6b35925c8b1c0a7a5e8be8d1f01a53ca8dd80e07dea95c1ba84b192e9b5`（2026-11-27 更正：原记录多抄一位 "1"，以实算 64 位为准） |
| 本地位置（仓库外） | `E:\AI-Tools\PAE-human-template-poc\assets\makehuman-a8bc2d54\` |
| 参考文档 | `makehumancommunity/mpfb2` @ `650c5c53bd6989a8ac264f31fa1d11d4a0c7bd8a`（2026-11-22），仅 `docs/fileformats/{rig,weights,target}.md` 三份文档 |

网络情况（如实记录）：首次核查时本机到 `github.com:443` 的 `git fetch` / `git ls-remote` 失败（连接被重置），`api.github.com` 与 `codeload.github.com` 可达，因此资产通过固定 commit 的 zip 归档取得而非 git clone。2026-11-26 17:30 复测 `git ls-remote origin` 成功：`origin/poc-sm/shape-model-v2` = `f1b4404…`、`origin/feature/procedural-character-foundation` = `2ac101f…`，均与本地一致，期间没有他人推送。

## 3. 许可证原文要点（`LICENSE.md` @ 同一 commit）

- §B 源代码：`.py`、脚本、GLSL shader、UI 图像 → **AGPL**。
- §C 随附资产：定义为 “any data contributing to the graphical output”，并列举 **base mesh and proxies、targets and modifiers、textures、clothes (MHCLO)、poses and expressions** → **CC0 1.0 Universal**。全文在 `LICENSE.ASSETS.md`。
- §D 输出：MakeHuman 项目认为任何输出（导出、渲染、脚本生成数据）不包含程序逻辑，资产是 CC0，所以对输出不作主张。
- §C/§D 末尾明确：**第三方资产（如社区资产库下载）不在此列，使用者自负其责。**

CC0 允许商业使用、修改和再分发；但不提供商标、专利、肖像权或其他第三方权利的保证。

## 4. 逐类核查

| 资产 | 路径（`makehuman/data/`） | 文件内证据 | 判定 | 依据强度 |
|---|---|---|---|---|
| 基础人体网格 | `3dobjs/base.obj` | 文件头：“This asset was explicitly released as CC0 in september 2020” | **CC0-1.0** | 强：§C 列举 + 文件头 |
| 基础代理 | `3dobjs/base.mhclo`、`a7_converter.proxy` | 同样的 CC0 文件头 | CC0-1.0（本轮未使用） | 强 |
| 形态目标 | `targets/**/*.target`（1,280 个，全量扫描） | 1,280 / 1,280 带 CC0 文件头 | **CC0-1.0** | 强：§C 列举 + 文件头 |
| 骨架定义 | `rigs/default.mhskel` | 无文件头；JSON `"license": "CC0"`、`"version": 110` | **`PENDING_CONFIRMATION`** | 中：**未被 §C 逐项列举** |
| 皮肤权重 | `rigs/default_weights.mhw` | 无文件头；JSON `"license": "CC0"`、`"version": 110` | **`PENDING_CONFIRMATION`** | 中：同上 |
| 软件代码 | 全部 `.py` / shader | `LICENSE.CODE.md` = AGPL-3.0 | **不使用** | — |
| 纹理 / 皮肤 / 眼睛 / 头发 / UV | `textures/ skins/ eyes/ hair/ uvs/` | 本轮未逐文件核查 | `LICENSE_UNVERIFIED`，本轮未使用 | — |
| MPFB2 代码与资产包 | — | 仓库 GPL-3.0 | 不使用；只读了三份格式文档 | — |

> 扫描口径：1,280 个 `.target` 加 `base.obj`、`base.mhclo`、`a7_converter.proxy`、`default.mhskel`、`default_weights.mhw` 共 1,285 个文件，读取每个文件前 14 行匹配 “CC0”：1,284 个命中，唯一未命中的是 `default.mhskel`。

## 5. 本轮实际使用的文件

链 A 使用 22 个 target（9 个参数的 incr/decr）+ 网格 + 骨架 + 权重 + 3 份许可证文本；链 B 使用 8 个 target（仅 incr，其中 4 个与链 A 相同）+ 同样的网格、骨架、权重。合计 **32 个不同文件**，其中 26 个不同的 target（占该 commit 全部 1,280 个的 2 %）。

关键文件（完整 32 行清单含 Git blob、SHA-256、文件头扫描结果、使用链：`evidence/host-proof/provenance/source-file-identities.md`；流水线自动生成的 28 行清单：`provenance/asset-manifest.md`）：

| 文件 | 字节 | Git blob | SHA-256 | 许可证 / 状态 |
|---|---:|---|---|---|
| `LICENSE.md` | 3598 | `5d1a49d31ebdaa46b06c52eae2c005c678a63ffa` | `edd99571ca62698f78c943fd4fffb159a413c315fa0d4819d3cec5cadaf8b4f1` | CC0 1.0（资产）/ AGPL（代码，未使用） |
| `LICENSE.ASSETS.md` | 6962 | `f3ede1fc3318f8d794e2cb51924186c62f02f71a` | `f6089cba01cb570a24712b41ab8a586ccd3cc5ef33dc266ca50b95c288956d2c` | CC0-1.0 |
| `LICENSE.CODE.md` | 34354 | `4ef32f0833975b98cb4080343c8fd60ab6d0f88c` | `32a0a7381bbcb97b043b34595a1f440557d5cfe904f4e91770b0abb465e8db25` | AGPL-3.0（未使用） |
| `makehuman/data/3dobjs/base.obj` | 1749303 | `d26635e9323ecca30778fd7b9c00062b03cce09` | `8e761e6624b8f54536409135d1636da63b32486a90d4897f84e121d144f6fb4c` | CC0-1.0 |
| `makehuman/data/rigs/default.mhskel` | 117790 | `b02cbecae00143856410d7561adf006d83bf9b3e` | `99f179bce0aa850b45d4191a1d0d234c5851f881c057439470ded3bddf729a24` | **待确认**（文件内声明 CC0） |
| `makehuman/data/rigs/default_weights.mhw` | 897521 | `66185806afacf87749be8e5cf6dbf66496b2a6b0` | `0f3641d651ae3d00ad6b4ccee43142edb109d3bd909d27d9e4139ef1beed8625` | **待确认**（文件内声明 CC0） |

MPFB2 文档副本在 `evidence/host-proof/reference-docs/`。

### 5.1 使用与分发状态（两条链一致）

| 项目 | 本轮状态 |
|---|---|
| 原始 OBJ / Target / Rig / Weights 进入 PAE Git | 否 |
| 原始资产进入 Android APK | 否 |
| 原始资产进入生产源码目录 | 否 |
| MakeHuman / MPFB2 / Blender / Python 作为 Android 运行时依赖 | 否 |
| Host 工具读取和转换这些数据 | 是，PAE C++17（链 A、链 B） |
| 生成的内部模板进 Git | 否（`.paht` / `base.paehuman` 留在构建目录，含时间戳） |
| 生产分发授权 | 未授予，需负责人另行确认 |

## 6. 与既有治理文本的关系（必须由负责人裁定）

| 既有文本 | 原文 | 与本路线的关系 |
|---|---|---|
| `docs/project-charter.md` §硬性约束 | “零外部 3D 美术资产：禁止 GLB/GLTF/FBX/OBJ/DAE…全部几何/骨骼/权重/材质/场景由代码和参数生成” | 由 CC0 基础网格**离线编译**出的 PAE 模板，在实质上仍是“外部来源的几何数据”。进入运行时之前需要负责人通过 ADR 修订此条（草案见 R7） |
| `docs/architecture-decisions/ADR-004` | “不得改回外部模型” | 同上 |
| `docs/foundation/00-product-scope.md` | 正式资产来源包含“拓扑模板”，同时禁止“外部人体…模型…作为运行时依赖” | “拓扑模板”是否允许由第三方 CC0 网格派生，文本有歧义，需 ADR 明确 |
| `docs/research/parametric-human-modeling/02-tech-and-license-matrix.md` §3（2026-11-25） | MakeHuman 网格/target：“不导入” | 本轮任务书把方向调整为“允许研究并使用许可证明确的第三方基础拓扑”。该研究记录原文没有被改写；取代关系写入 R7 的 ADR 草案 |

## 7. 未验证项

- `textures / skins / eyes / hair / uvs` 等其余随附资产的逐文件许可证（本轮未使用，不影响本轮结论）。
- `default.mhskel` / `default_weights.mhw` 的文件内 CC0 声明是否足以约束其再分发（需负责人/法务意见）。这是本文件唯一改变结论性质的待决项。
- 网格单位的官方说明：按 0.1 m/单位换算得到 1.666 m 身高，落在成人范围；**官方单位声明未取得**。
