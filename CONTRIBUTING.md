# 参与贡献

感谢你考虑为本项目贡献代码、测试或文档。项目当前处于 `0.1.x` Runtime Preview 阶段；优先接受可复现的兼容性问题、安全边界修复、测试完善、跨平台构建修复和能提高可读性的聚焦改动。

## 提交 Issue 前

- 安全漏洞请按 [SECURITY.md](SECURITY.md) 私密报告，不要创建包含利用细节的公开 Issue。
- 先搜索现有 Issue，确认问题没有被记录。
- 缺陷报告应包含最小复现、实际与预期结果、提交 SHA、操作系统、编译器、CMake 生成器、构建类型和相关 sanitizer/crash 输出。
- Runtime、sandbox、allocator 或 ABI 问题请说明使用的宿主配置、资源限制、是否加载原生模块，以及是否能在独立 worker 中复现。

## 本地构建与验证

项目要求 C++23、CMake 3.20+ 和 Python 3。一个通用的开发构建流程是：

```powershell
cmake -S . -B build/contrib -DCMAKE_BUILD_TYPE=Debug
cmake --build build/contrib --config Debug
ctest --test-dir build/contrib -C Debug --output-on-failure
```

Windows 也可以使用 `lua.slnx` 和 MSBuild。提交前至少运行与你的改动直接相关的测试；准备合并时，在 Windows 上运行严格 changed-scope 质量门：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/run_quality_gate.ps1 `
  -Strict -FormatScope Changed -FormatBase origin/main
```

如果环境无法运行某项检查，请在 Pull Request 中明确列出未运行项和原因，不要把缺少工具描述为通过。

## 改动要求

- 保持改动聚焦；行为修复同时提供失败前可复现、修复后通过的测试。
- 修改 Lua 语义、公开 C API、ABI、sandbox、allocator 或平台支持时，同步更新相应权威文档和合同测试。
- 不直接编辑 `tests/lua/official/` 中由 SHA-256 清单锁定的上游文件。确需升级上游材料时，必须同时更新来源、哈希、偏差说明和验证脚本。
- 新增或复制第三方材料时，在 `THIRD_PARTY_NOTICES.md` 中记录来源 URL、精确版本或提交、许可证、版权声明及本项目修改。
- 新增 C++ 源文件时使用 `tools/add_source.ps1`，保持 CMake 与 Visual Studio 工程清单同步。
- 不提交构建产物、个人 IDE 配置、REPL 历史、完整诊断 trace、真实业务数据或凭据。测试 fixture 应最小化并去标识化。
- 不以放宽 coverage、benchmark、sanitizer、fuzz、XFAIL 或 release evidence 门禁来掩盖回归。

## Pull Request

Pull Request 应说明问题、方案、风险和验证结果，并关联相关 Issue。请保持所有 review conversation 已解决。外部贡献由非作者维护者审查；维护者自有改动在独立审查者到位后应于最后一次实质修改后取得非作者批准。单维护者阶段可以继续普通开发，但不得把自我审查描述为独立批准，也不得在缺少独立审查者时生成 RC 或正式发布治理证明。维护者可能要求拆分无关改动，或为兼容性声明补充官方 Lua 5.1.5 对照证据。

贡献者必须有权提交相关内容。提交贡献即表示同意按仓库根目录的 [MIT License](LICENSE) 许可该贡献；第三方内容仍须遵守并保留其原许可证与版权声明。
