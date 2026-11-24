# 来源原件清单（`source/` 未入库）

> 状态：**来源登记**。本文件记录第 2 轮实际读取并用于取数的一手原件。
> **两份 PDF 均未入库**，按 URL + SHA256 + 本地留存路径登记，任何人可复现全部数值。

---

## 1. 为什么不入库

`[VERIFIED]`

- 两份都是**公开可下载**的第三方出版物；
- NC State 那份在页脚标注 "© 11/2024 The Ergonomics Center"，**未声明开放许可**；
- **入库再分发存在授权不确定**，因此不把二进制文件放进 Git 历史；
- 替代做法是**地址 + 校验值 + 复现说明**——这与依赖锁定文件（lockfile）的做法一致，
  既保证数值可复核，又不产生再分发问题。

---

## 2. SRC-ANTH-01 — ANSUR II 人体测量汇总表

| 字段 | 值 |
|---|---|
| 名称 | *Anthropometric Data for U.S. Adults*（36 项，英寸） |
| 编制方 | **The Ergonomics Center**, North Carolina State University（© 11/2024） |
| 引用 | Gordon, Claire C. et al. (2014). *2012 Anthropometric Survey of U.S. Army Personnel: Methods and Summary Statistics.* |
| URL | `https://ergocenter.ncsu.edu/wp-content/uploads/sites/358/2026/02/01-Anthropometry-Summary-Tables.pdf` |
| SHA256 | `a1ea470e6d67d5870a1b35da9fdf4a7d60f1982ee0b187827b1b3a7a3f820ee6` |
| 字节 | 542,674 |
| 本地留存 | `E:/AI-Tools/DEC-05-Review-Drafts/blocker-closure/source/ANSUR-II-Anthropometry-Summary-Tables.pdf` |
| 样本 | 美军现役 + 预备役，**男 4,082 / 女 1,986**，2012 年测量，93 项；2012 内部发布、2017 公开 |
| 复现方式 | `curl -sL -o f.pdf "<URL>" && sha256sum f.pdf` → 应等于上表 SHA256 |
| 用途 | Canonical Sheet 的**形态尺寸**来源（breadth / depth / height / length） |

### 2.1 必须随数值一同引用的样本限定

`[VERIFIED — Penn State OPEN Design Lab, https://www.openlab.psu.edu/ansur2/ 原文]`

> "Despite the presence of reservists in the sample, it is still not an approximation of the US Civilian population.
> Consequently, while there is useful information here, designs and standards based on these data will not accommodate
> most user populations in the intended manner."

`[VERIFIED]` **ANSUR II 是军人样本，不代表美国民用人口。** 因此本项目：**形态尺寸取 ANSUR II，围度取 CDC NHANES**，两者不可互相推导。

### 2.2 该表**不包含**周长（girth）

`[VERIFIED]` 36 项全部为 height / breadth / depth / length，**没有任何 circumference 项**。
这是 Canonical Sheet 中「胸廓宽度」「腰宽」标 `TBD` 的直接原因。

---

## 3. SRC-ANTH-02 — CDC/NCHS 人体测量参考数据

| 字段 | 值 |
|---|---|
| 名称 | *Anthropometric Reference Data for Children and Adults: United States, August 2021–August 2023*，Statistical Report No. 50（Series 3） |
| 编制方 | **National Center for Health Statistics**（美国国家卫生统计中心） |
| URL | `https://www.cdc.gov/nchs/data/series/sr_03/sr03-050.pdf` |
| SHA256 | `ccb8d52b6e3f09bc50d5734493e512c348c85ad2ea98ea3cd559019da8f13f16` |
| 字节 | 1,812,869 |
| 本地留存 | `E:/AI-Tools/DEC-05-Review-Drafts/blocker-closure/source/CDC-NHANES-SR50-Anthropometric-Reference-Data.pdf` |
| 样本 | NHANES，美国民用人口；男性 20+ 腰围 n=2,605、臀围 n=2,569 |
| 复现方式 | `curl -sL -o f.pdf "<URL>" && sha256sum f.pdf` → 应等于上表 SHA256 |
| 用途 | Canonical Sheet 的**围度**来源（腰围 / 臀围），含精确测量位置定义 |

### 3.1 腰围测量位置（原文，直接决定"腰围"能否比较）

`[VERIFIED — SR50 原文]`

> "Standing height or stature and waist circumference were measured to the nearest 0.1 centimeter on participants.
> Standing height was measured with a wall-mounted stadiometer.
> **Waist circumference was measured with a tape measure at the uppermost lateral border of the hip crest (ilium).**"

`[INFERENCE]` 该定义是本项目「腰围」口径的**唯一基准**。不同测量位置可产生 9 cm 以上差异
（髂嵴上缘 vs 脐点 vs 最小腰围），因此 Canonical Sheet 中每个围度都必须绑定测量位置。

### 3.2 男性关键数值（报告原值）

| 项 | n | Mean | 50th %ile | 25th | 75th | 5th | 95th |
|---|---:|---:|---:|---:|---:|---:|---:|
| 腰围 20+ (mm) | 2605 | 1030 | **1009** | 914 | 1123 | 793 | 1332 |
| 腰围 20–29 (mm) | 282 | 951 | **934** | 826 | 1061 | 713 | 1244 |
| 臀围 20+ (mm) | 2569 | 1056 | **1036** | 977 | 1108 | 895 | 1291 |
| 臀围 20–29 (mm) | 278 | 1045 | **1030** | 962 | 1101 | 870 | 1278 |

`[VERIFIED]` 测量位置：腰围 = 髂嵴（ilium）上缘最外侧缘；臀围 = 臀部最大处。

### 3.3 SR50 **未**报告的成人测量

`[VERIFIED]` 成人 20+ 无胸围值（chest circumference 仅见于儿童表）；无 chest breadth / bi-iliac breadth /
seat breadth / sagittal abdominal diameter 的**数值**（这些在该报告中列为"已采集"的测量项，
但数值表只覆盖体重、身高、腰围、臀围、上臂围、上臂长、大腿长）。
→ Canonical Sheet 的 TBD-1 / TBD-2 / TBD-4 即由此而来。

---

## 4. 其他被引用来源（均**未**入库，仅记录地址）

| 编号 | 来源 | URL | 授权 | 权威层级 |
|---|---|---|---|---|
| SRC-ANAT-01 | OpenStax *Anatomy and Physiology 2e* | `https://openstax.org/books/anatomy-and-physiology-2e/pages/8-3-the-pelvic-girdle-and-pelvis` 等 | **CC BY 4.0** | T1 |
| SRC-ANTH-03 | Penn State OPEN Design Lab — ANSUR II | `https://www.openlab.psu.edu/ansur2/` | 公开页面 | — |
| R-B02 | BodyParts3D | `https://dbarchive.biosciencedbc.jp/data/bodyparts3d/LATEST/README_e.html` | **CC BY 4.0**（许可章节原文已取得） | T1 |
| R-B10 | Thunder Cloud Studio — Topology for Low Poly Game Characters Pt.2 | `https://thundercloudstudio.artstation.com/blog/G980G/topology-for-low-poly-game-characters-essential-tips-and-tricks-part-2` | 页面无许可声明 → 仅阅读、仅引要点 | T2 |
| R-B11 | Blender Studio — Generic Retopology（Julien Kaspar） | `https://studio.blender.org/training/realistic-human-research/retopology/` | 未见页面许可声明 | T2 |
| R-B12 | Renderosity — "Arm pit tolology"（pjz99, 2008, 21 posts） | `https://www.renderosity.com/forums/threads/2748028` | 论坛默认条款；作者内容未单独授权 | T3 |

`[VERIFIED]` R-B10 / R-B11 / R-B12 **未入库、未下载素材**，仅引用正文要点。
`[VERIFIED]` R-B02 的许可允许自由获取与再分发（须署名），本轮**仅**读取 README 正文用于确认许可条款与数据描述，
**未下载**其 OBJ 网格（`isa_BP3D_4.0_obj_99.zip` 136 MB / `partof_BP3D_4.0_obj_99.zip` 62 MB），
因为项目硬约束禁止引入外部模型资产。

---

## 5. 一句话

**本轮所有数值都可以由第三方按上面的 URL 下载、按上面的 SHA256 校验后独立复现。**
