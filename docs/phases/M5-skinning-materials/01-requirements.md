# M5 — 01 需求

- FUN-RND-002：GPU LBS 真实驱动 M4 生成的网格，位置与法线蒙皮，≤4 影响。
- FUN-AV-002：皮肤/头发/服装颜色运行时切换，仅更新 Uniform，不重传顶点/索引。
- FUN-SCN-001：SolidColor/Gradient/ProceduralStudio 背景，切换不重建角色。
- TEST-001：UBO 布局与 CPU/GPU 蒙皮对照测试。
- 验收：合成动作下主要关节变形可接受；Shader 编译无错误；颜色与背景切换 revision 不变。
