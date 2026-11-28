# Android Workflow

适用：Android 动态人体运行时（M4）。**当前 NOT_AUTHORIZED，须 M3 Gate + 正式授权。**

1. **前置检查（强制）**：M1 Gate / M2 Gate / M3 Gate 是否通过？M4 是否正式授权？未授权→**停止生产修改**，输出前置条件 + 授权提案，不擅自启动。
2. **Preflight**：核对 Git/白名单。冻结 LegacyRigV1 / UBO[22] / 64B packing / 旧 JNI ABI / mode-3 / mode-4 静态默认 / PAHT v3 语义。
3. **设备能力实测**：Dynamic Palette / GPU Vertex Layout / UBO Capability / GLES3.0 Device Capability。**不得默认 Full92 Rig 适配所有 GLES3 设备。**
4. **独立动态路径**：新数据独立版本化格式；PAHT v3 兼容（新格式版本升级 + 读旧）。CPU 路径作参考 + 必要回退。
5. **EL-09 逐点**：CPU/GPU Parity / Surface Lifecycle / APK Asset Loading / Memory-FPS。
6. **EL-10 真机证据**：真实设备测试（非 Host），静动态一致，多设备能力检查，性能/内存/生命周期。Host 过≠Android 过。
7. **Gate + 交付**：Compatibility/Performance/Safety Gate + implementation-report + STATUS。

未获授权时只做 READ_ONLY/DESIGN_ONLY 准备，不写 Android 生产代码。