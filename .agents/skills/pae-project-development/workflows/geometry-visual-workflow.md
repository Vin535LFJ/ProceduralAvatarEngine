# Geometry / Visual Workflow

适用：几何/解剖/形态/视觉质量任务（M1/M6/M7 相关）。

1. **Preflight + 授权**：核对 Git/白名单。几何 Host 研究通常 HOST_IMPLEMENTATION（Host 工具）；生产模板改动需授权。
2. **区域定位**：按人体区域（Torso/Shoulder/Axilla/Elbow/Hand/Fingers/Hip/Knee/Foot/Toes/Face 等）。查 geometry-and-skinning-principles。
3. **EL-02 基线**：固定网格/相机/Mask 的 Before 基线。Bind + Pose 双态。
4. **方法选择**：优先 PAE 自有 Shape Basis/控制点/结构化拓扑/局部 Corrective。不退回全身 SDF/Primitive 拼接主线。手指关注 MCP/PIP/DIP/分离/碰撞/限位。
5. **EL-09 逐点**：单变量修改。结构化手指生成关注 Palm/Thenar/Hypothenar/Finger Identity/Separation。
6. **EL-10 证据**：Clay/Wireframe/Normal/Closeup/CSV，固定 Mask，翻面率，区分穿透/折叠/体积损失。Bind+Pose 双态。负结果保存。
7. **视觉 Gate**：技术 PASS≠视觉 PASS。图像描述模型仅供参考，最终人眼 Gate 由负责人。
8. **交付 + 归因**：implementation-report + 更新 STATUS。

禁止：重复统一细分作为握拳主方案；用移动 Mask/旧图掩盖问题；把结构连续性当自然度。