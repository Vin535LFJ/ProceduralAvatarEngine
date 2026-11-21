# M11 — 程序化人体解剖与视觉质量重构：范围与假设

- 日期：2026-11-21；分支：`m11-anatomy-visual-rebuild`
- 目标：把引擎从"拼装式人体原型"升级为"参数化生成精致卡通人体的程序化资产体系"（Parametric + Spline Sweep + Local SDF Fusion 混合，PoC 定案）
- 范围：几何核心重构、表面融合与拓扑、蒙皮权重体系、面部/头发/服装生成器、程序化材质 Shader、重定向修复、视觉验收体系、性能复测
- 非目标：真实 3D 动捕宣称（保持 2D 重定向 + 单目 Z 线索）、表情动画、写实人体、外部美术资产、更换渲染路线
- 不变量（必须保留）：EGL/Renderer/JNI/Kotlin 宿主/Camera/Video/MotionProvider/Skeleton 协议（22 骨骼、bindLocal 平移、恒等=绑定）/性能统计/测试契约（权重和=1、joints<22、>1000 顶点、≥10 submesh、确定性、父<子拓扑序）
- 假设：基准设备不变（nunai_max 盒子）；单会话推进到资源允许的最远阶段，阶段门以证据为准
