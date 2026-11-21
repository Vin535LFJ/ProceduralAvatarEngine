# M10 视觉提升迭代 — 验收记录

- 日期：2026-11-21；触发：用户反馈数字人观感差
- 回归：`ctest` 5 套件 24 用例全绿（新几何通过 MeshValidator/确定性/权重测试）；`./gradlew :app:assembleDebug` BUILD SUCCESSFUL

## 改动内容（全部程序化，零外部资产）

| 项 | 改动 | 证据 |
|---|---|---|
| 卡通描边 | inverted-hull 描边（蒙皮感知：顶点沿法线偏移 0.01m，正面剔除只留背面壳），角色 draw call +1 | `frame-m10-final.png` 全身轮廓线 |
| 卡通脸 | 眼白球 + 凸出式墨色虹膜瞳盘（新 materialId 4 固定深色分支）+ 白色高光点 + 眉（发色）+ 嘴线 + 鼻 | `frame-m10-final.png` 面部 |
| 发型 | 发帽加大后移 + 额前刘海带 | 同上 |
| 躯干 | 4 截面→7 截面平滑 S 曲线，胸腔加深、肩线收窄，消除方块感 | 前后对比 |
| 四肢 | 锥度 {1.20,1.10,0.78}→{1.12,0.96,0.70}；关节球 overshoot 12–35%→2–22% | 关节处疙瘩消失 |
| 手部 | 加大 + 拇指球 | 手部形状 |
| 比例 | headScale 1.0→1.12、armRadius→0.062、legRadius→0.082、handScale→1.15（卡通头身比） | 整体比例 |
| 细分 | radial/rings 公式 10+4d/8+4d → 12+6d/10+6d | Medium 3053→**7537** 三角形（预算 5–15k 内） |
| 接触阴影 | 地面圆盘（blend + depthMask off），画在角色前、被角色遮挡 | 脚下椭圆阴影 |
| 配色 | 默认肤/发/衣/眼暖色化（native + MainActivity 同步） | 整体色调 |

## 验证

- **性能无回归**：FPS 47.7–47.9、P50 20.96ms、P95 21.27ms（`evidence/stats-m10.txt`）——三角形翻倍无影响（瓶颈为渲染节拍）
- **动作变形**：抬臂/屈肘姿势下描边跟随蒙皮变形、肘肩无裂缝（`frame-m10-pose-armraise.png`、`frame-m10-pose-elbow.png`）
- **修复的几何问题**：虹膜球初始位于眼白球内部不可见 → 前移凸出眼白表面（0.62r→0.80r z 偏移），高光点同步调整
- 前后对比：`frame-m10-checkpoint1.png`（改后）vs `docs/phases/M9-performance-delivery/evidence/frame-m9-final.png`（改前）

## 遗留（视觉 backlog）

- 膝盖球体在正面视角仍有细微接缝线（低优先级）
- 眼睛风格可再调（当前偏"圆睁"卡通风，参数已集中在 Humanoid.cpp 面部常量处）
- toon 阶梯数/rim 强度可继续调优；服装款式变化（Spec §11.3）未实现
