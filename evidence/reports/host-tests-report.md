# Host Tests 报告（AUTH-03）

> 日期时间：2026-11-23 18:38–18:45 GMT+8
> Worktree：`E:/AI-Tools/ProceduralAvatarEngine-PoC`
> Git SHA：`99941a94ec58fb37ab0f1a36b90590d3fdce5198`
> 构建目录：`E:/AI-Tools/ProceduralAvatarEngine-PoC/build-host`
> 结果：**9/9 PASS，0 FAIL，0 SKIP**

---

## 1. 环境

| 项 | 值 |
|---|---|
| CMake | `cmake version 3.20.0-rc1` |
| CTest | `ctest version 3.20.0-rc1` |
| C++ 编译器 | `g++ (GCC) 11.5.0`（`C:/cygwin64/bin/c++.exe`） |
| Make | GNU Make 4.4.1，Built for x86_64-pc-cygwin |
| Generator | `Unix Makefiles` |
| Ninja | 未安装（仅探测，不影响执行） |

## 2. 执行命令

```bash
cmake -S "E:/AI-Tools/ProceduralAvatarEngine-PoC/native" \
      -B "E:/AI-Tools/ProceduralAvatarEngine-PoC/build-host" \
      -G "Unix Makefiles" -DPAE_HOST=ON

cmake --build "E:/AI-Tools/ProceduralAvatarEngine-PoC/build-host" -- -j4

ctest --test-dir "E:/AI-Tools/ProceduralAvatarEngine-PoC/build-host" --output-on-failure
```

## 3. 日志位置

| 日志 | 路径 |
|---|---|
| CMake 配置 | `evidence/host-tests/cmake-configure.log` |
| 编译 | `evidence/host-tests/cmake-build.log` |
| CTest | `evidence/host-tests/ctest.log` |

## 4. 测试结果

| # | 测试名 | 结果 | 耗时 |
|---|---|---|---|
| 1 | `pae_core_tests` | Passed | 0.72 s |
| 2 | `pae_geometry_tests` | Passed | 0.64 s |
| 3 | `pae_fusion_tests` | Passed | 0.75 s |
| 4 | `pae_face_hair_tests` | Passed | 0.70 s |
| 5 | `pae_anatomy_tests` | Passed | 1.93 s |
| 6 | `pae_humanoid_tests` | Passed | 0.78 s |
| 7 | `pae_skinning_tests` | Passed | 0.73 s |
| 8 | `pae_motion_tests` | Passed | 0.72 s |
| 9 | `pae_character_patch_tests` | Passed | 1.00 s |

汇总：`100% tests passed, 0 tests failed out of 9`；`Total Test time (real) = 7.99 sec`。

## 5. 重要限制

- 本报告只证明上述 9 个 CTest 用例在本次环境中执行并通过。
- `pae_character_patch_tests` 的边界 C0/G1 指标来自 `BodyPatchGenerator.cpp:334-337` 的硬编码值，不是实测。
- CTest PASS 不证明：水密性、无自交、外法线正确、动画变形质量、视觉形态质量、Android GLES 渲染、CPU/GPU skinning parity。
- 未构建 Android APK，未执行设备操作，未采集帧率或性能数据。

## 6. 授权与边界

- 使用的授权：AUTH-03（Host Tests）、AUTH-06（证据文件）。
- 所有构建产物与日志位于 Worktree 内；原工作区未写入。
- 未修改源码、CMake、Gradle；未 Commit / Push。
