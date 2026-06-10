---
status: current
verified_against: tests/unit/framework/test_runner.cpp; tests/unit/framework/test_framework.hpp; lua_test.vcxproj; CMakeLists.txt; tools/add_source.ps1; docs/status/project-status.md
last_checked: 2026-06-02
applies_to: lua_test executable usage and extension
---

# 测试运行器指南

`lua_test.exe` 是仓库的自定义单元测试运行器。它注册 `tests/unit/**` 下的所有 C++ 单元测试，可以列出、筛选、运行和导出 JUnit XML。

## 命令

```powershell
bin\lua_test.exe
bin\lua_test.exe --list
bin\lua_test.exe --filter "Symbol Binding"
bin\lua_test.exe --filter=Runtime
bin\lua_test.exe --report=junit
bin\lua_test.exe --report=junit:bin\lua_test_junit.xml
bin\lua_test.exe --filter "Lua 5.1 Official Suite"
```

## 选项

| 选项 | 行为 |
|---|---|
| `--help`, `-h` | 打印用法 |
| `--list` | 打印已注册的测试而不运行它们 |
| `--filter <text>` | 运行套件、名称或 `Suite::Name` 包含文本的测试（大小写不敏感） |
| `--filter=<text>` | 同上 |
| `--report=junit` | 写入 `lua_test_junit.xml` |
| `--report=junit:<path>` | 将 JUnit XML 写入指定路径 |
| `--max-memory-mb <mb>` | 为本次运行覆盖进程内存上限 |
| `--max-memory-mb=<mb>` | 同上 |
| `--no-memory-limit` | 禁用运行器上限；仅在另一个内存上限保护的脚手架内使用 |

## 内存安全

`lua_test.exe` 在注册或运行测试之前安装进程内存上限。默认上限为 512 MB。如果无法安装上限，运行器在测试执行前退出而非无保护运行。

这是官方 Lua 5.1 压力路径的默认安全边界。在这些路径上工作时优先使用聚焦的筛选器和显式上限：

```powershell
bin\lua_test.exe --max-memory-mb 128 --filter "post-vararg"
bin\lua_test.exe --max-memory-mb 128 --filter "closure.lua weak GC loop cap"
```

环境变量覆盖：

- `LUA_TEST_MAX_MEMORY_MB=<mb>` 更改默认上限。
- `LUA_TEST_DISABLE_MEMORY_LIMIT=1` 为外部隔离运行器禁用上限。

## 添加测试

1. 在 `tests/unit/<area>/` 下添加或编辑文件。
2. 定义一个 `registerXTests()` 函数。
3. 通过 `TestRegistry::getInstance().registerTest(...)` 注册各个测试。
4. 在 `tests/unit/framework/test_runner.cpp` 中添加声明和调用。
5. 使用 `tools\add_source.ps1 -SourcePath tests\unit\<area>\test_name.cpp -Target Test` 将新文件添加到 `lua_test.vcxproj`、filters 和 `CMakeLists.txt`。

开发时使用聚焦的筛选器：

```powershell
bin\lua_test.exe --filter "Your Suite"
```

然后将变更视为已验证前运行完整的运行器：

```powershell
bin\lua_test.exe
```

完整运行器仍使用默认内存上限。

## 当前测试领域

仓库当前将 C++ 测试分组在以下目录：

- `app`
- `compiler`
- `core`
- `gc`
- `io`
- `metamethod`
- `stdlib`
- `vm`

## CTest

CMake 将相同的 `lua_test` 可执行文件注册为 CTest 测试。CTest 是辅助路径，非主要的 Windows/MSBuild 工作流。
