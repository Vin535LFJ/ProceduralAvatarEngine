# 04 — Shoulder and Joint Study

## 1. 肩部必须是 Shoulder Complex，不是肩部球体

肩部的三维外轮廓由胸廓上缘、锁骨、肩峰、肩胛、斜方肌、胸大肌外侧、三角肌各束、肱骨头和腋窝共同形成。任何一项独立生成后仅靠重叠遮挡，都会把解剖体积错误转化为拼接边界。

## 2. 肩部解剖语义拆分

### 2.1 骨性与骨架锚点

- `SternumNotch`：锁骨内侧起点参考。
- `ClavicleMedial/Lateral`：锁骨走向控制点。
- `Acromion`：肩峰最外上方轮廓参考。
- `ScapularSpine`：肩胛冈方向曲线参考。
- `GlenohumeralCenter`：肱骨头/肩关节中心，必须与 UpperArm 骨头端一致或有明确偏移约束。
- `AxillaAnterior/Posterior`：腋窝前后边界控制点。
- `ChestWallLateral`：胸壁侧面参考。

这些是语义锚点，需从当前 22 Bone Skeleton + 体型比例推导。不能把 `leftShoulder` 一个点当成肩峰、锁骨末端和肱骨头的全部语义。

### 2.2 软组织体积

- 斜方肌上束从颈背到肩峰区域，控制颈肩坡度。
- 三角肌前/中/后束包裹肩关节，但需保持三束融合的外轮廓，不应显成三颗小球。
- 胸大肌外侧与肱骨近端、前肩过渡。
- 背阔肌/大圆肌与后腋襞构成腋窝后壁方向。
- 肩袖以关节周围包络和变形语义表达，不要求第一 PoC 逐条肌腱精确建模。

## 3. Axilla：不能只 Smooth Union

腋窝是肩/上臂和胸壁之间的负形空间。正体积场 union 能消除两个物体之间的缝隙，但 union 半径过大时会把凹陷填满，形成肩下圆球或腋窝鼓包。

建议的形态解算：

1. 以胸壁表面、肱骨头/上臂内侧、前后腋襞的 landmark 定义腋窝目标截线。
2. 肩部正体积由胸壁/三角肌/肌群 field 形成。
3. 使用 anatomically constrained concave envelope / negative field 控制腋窝凹面深度与宽度；凹面不应使用孤立巨大球 carve，而应受腋窝边界曲线限制。
4. 手臂下垂、外展和前举时，腋窝 envelope 随关节姿态变化；中性姿态和动作姿态使用同一变形语义，不能只在 Bind Pose 调出好看形状。
5. 最终 skin envelope 只提取一次，避免腋窝 patch 和 chest surface 再次 seam。

## 4. 肩部推荐几何

建议 ShoulderComplex 使用共享 Shape Model 产生以下场贡献：

- chest wall / upper thorax envelope；
- clavicle ridge guide surface / narrow field；
- trapezius posterior-superior mass；
- pectoralis anterior fan volume；
- anterior/lateral/posterior deltoid masses；
- scapular posterior mass；
- axilla concavity constraint；
- proximal upper-arm outer envelope。

这些可以由多个数学形态函数表示，但只有一个最终 surface field / common cage 负责皮肤外表面。区域字段保留 `RegionId`，用于最终拓扑与权重，而不是将每个 feature 各自 MeshData 输出。

## 5. 肘部：几何与运动共同设计

### 5.1 关节局部坐标

肘部至少定义：

- hinge axis（基于 shoulder-to-wrist 方向和实际关节平面）；
- forearm twist axis；
- flexion side / extension side；
- medial / lateral side；
- olecranon posterior landmark；
- medial/lateral epicondyle proxies；
- elbow crease / flexion pocket curve。

局部截面 frame 应跟随骨骼 frame 与姿势更新，而不是只对路径切线执行 PTF。

### 5.2 两侧变形模式

- 屈肘：内侧（屈曲侧）收缩和软组织压缩；外侧拉伸但维持最小曲率和表面连续；肘尖后侧形成受控突出。
- 伸肘：肘窝不应被拉成尖洞；后侧组织回到合理厚度，前侧过渡平顺。
- 前臂旋转：肌群周向方向随 twist 变化，不能让所有截面只围绕固定 world axis 扭转。
- 极端角度：应有 angle clamp / twist limits，超范围动作不能导致网格翻面。

### 5.3 变形方案

第一期继续以当前 LBS 为兼容基线，但应在 Shape Model 中定义 joint envelopes 和 region weights；通过姿态对照找出 LBS 无法维持的轮廓，再决定 corrective deformation。

后续候选：

- DQS/混合 LBS-DQS：可缓解 LBS 在 twist / collapse 的部分问题，但不是体积和局部肌肉隆起的完整替代。DQS 也可能有 joint bulging / normal distortion，需要校正。[Kavan et al. 2007](https://dcgi.fel.cvut.cz/publications/2007/kavan-i3d-sdq)；[Kim & Han 2014](https://onlinelibrary.wiley.com/doi/full/10.1002/cav.1604)
- Corrective Shapes：针对 shoulder raise、elbow flex 45°/90°、forearm pronation/supination 存储少量局部修正形态。
- Local Volume Preservation：在 shoulder/elbow regions 施加局部体积约束，作为 LBS 后处理或优化目标。[Rohmer et al. 2009](https://onlinelibrary.wiley.com/doi/10.1111/j.1467-8659.2008.01340.x)
- Implicit skinning：通过局部隐式场/约束使关节附近的体积与肌肉隆起更自然，研究文献展示了以局部场和形态约束改善 joint deformation 的路线。[Implicit Skinning / Dynamic Implicit Muscles](https://www.sciencedirect.com/science/article/abs/pii/S0097849318300995)

这些属于后续候选，不在本次被选定为当前实现。

## 6. 必须执行的肩肘动作验证

| Action ID | 动作 | 形态关注点 | 主要关节/区域 |
|---|---|---|---|
| BIND | A-Pose | 肩线、肩胸体积、腋窝初始形态 | ShoulderComplex |
| ARM_RAISE_45 | 双臂外展 45° | 三角肌和腋窝开始变形 | Shoulder / Axilla |
| ARM_ABDUCT_90 | 双臂外展 90° | 腋窝凹面、肩峰和胸侧连接 | Axilla / Thorax |
| ARM_FORWARD_90 | 双臂前举 90° | 胸大肌、前肩与肩胛运动 | Pectoral / Deltoid |
| SHOULDER_EXT_ROT | 肩外旋 | 肌群与上臂轴向稳定 | Cuff / UpperArm |
| ELBOW_FLEX_45 | 屈肘 45° | 内侧压缩、外侧拉伸 | ElbowComplex |
| ELBOW_FLEX_90 | 屈肘 90° | 肘窝、鹰嘴、前臂体积 | Elbow / Forearm |
| ELBOW_EXTEND | 伸肘复位 | 恢复中性轮廓，无折痕 | Elbow |
| FOREARM_PRONATE | 前臂旋前 | 截面方向、肌群相对转动 | Forearm |
| FOREARM_SUPINATE | 前臂旋后 | 扭转连续性和腕端稳定 | Forearm |

每种动作至少采集 front、side、45° 和关节 close-up，neutral pose 与 action pose 对照；不能只通过一张动作截图。

## 7. 肩部和肘部拓扑要求

- 肩关节周围必须有可变形环线，但不能让所有 edge loops 都以一个圆环包围肱骨；环线需顺应胸廓/肩带/上臂流向。
- 腋窝三角区要有足够环线密度，支持压缩和外展；极少数高价点应远离主要拉伸折线。
- 肘部至少有一组稳定环线跨过 joint region，且在 flexion crease 附近有足够密度；避免单圈折痕。
- 区域边界与骨骼影响带需要对齐，但不要把拓扑 seam 与 skinning region boundary 误认为同一条边。
- UV seam 应尽量避开高变形区域；必要时 separate UV seam 可存在，但不能造成几何断开。

## 8. 形态 Gate

任何肩部 PoC 若仍呈“肩球贴在胸壁上”“上臂管插进肩部”“腋下黑洞/鼓包”，应判 REJECTED，不得扩展更多人体区域。任何肘部 PoC 若只有静态姿势好看而屈肘折叠，亦 REJECTED。

