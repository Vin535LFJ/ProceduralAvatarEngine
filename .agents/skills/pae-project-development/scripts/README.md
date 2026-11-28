# Scripts

本目录预留 PAE 工作流辅助脚本。当前仓库已有的可复用工具（不在本技能内重复维护）：

- `tools/human-template-poc/host/tools/recompress_png.py`：证据 PNG 无损重压缩（像素往返校验）。
- `tools/human-template-poc/host/build.ps1`：Host 工具构建（含 CMake configure）。
- `tools/png_probe.py`：截图覆盖率探针（真机证据）。

新增脚本须：解决实际问题而非敷衍；文档化依赖；显式错误处理；不用 Windows 风格路径（用 `scripts/helper.py`）。

不为凑目录结构创建空脚本。