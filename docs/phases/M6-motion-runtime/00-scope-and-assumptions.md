# M6 — 00 范围与假设

- 范围：PoseFrame/IPoseProvider、SyntheticPoseProvider 六类动作、坐标转换、Landmark→骨骼 Retargeting、One Euro 滤波与四元数平滑、关节限制、丢点渐退、最新值缓冲。
- 非目标：ML Kit/CameraX（M7）、视频（M8）。
- 假设：Synthetic Provider 以 AvatarLocal 米制坐标输出 33 点；M5 的硬编码测试姿态将被 Motion Runtime 输出替换；单目深度不作真值。
