---
status: current
verified_against: docs/status/project-status.md; docs/guides/development.md; docs/architecture/patterns.md; docs/walkthroughs/index.md; docs/walkthroughs/gc-cycle.md; docs/glossary.md; examples/README.md
last_checked: 2026-05-23
applies_to: first-read learning path for contributors and readers
---

# Start Here

这份文档回答一个问题：第一次打开这个仓库，应该先读哪里。

本项目是一个教学取向很强的 Lua 解释器实现。最有效的阅读方式不是从最大文件开始啃，而是先确认仓库事实，再用测试和示例建立一条从源码到 VM 的路径。

## 10 分钟了解现状

1. 读 `docs/status/project-status.md`，确认当前构建入口、测试数量和已实现的运行时边界。
2. 读 README 的项目概览，不要把旧的规划项当作当前事实。
3. 读 `docs/guides/development.md`，按 Windows / MSBuild 路径构建和验证。

常用命令：

```powershell
bin\lua_test.exe --list
bin\lua_test.exe --filter "Runtime Services"
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1
```

## 30 分钟跑起来

1. 先运行完整测试，确认本机状态：

```powershell
bin\lua_test.exe
```

2. 再运行一个小范围测试，观察测试输出如何描述功能：

```powershell
bin\lua_test.exe --filter "Symbol Binding"
```

3. 打开 `docs/walkthroughs/index.md`，按主题选择一个测试切片继续读。

## 2 小时理解主线

建议按这条顺序读：

1. `docs/glossary.md`：把 Lua 术语和仓库类名对齐。
2. `docs/architecture/overview.md`：确认源码分层和四个构建目标。
3. `docs/architecture/patterns.md`：确认哪些设计模式已经落地，哪些仍只是路线图目标。
4. `docs/compiler/bytecode-generation.md`：理解当前编译管线。
5. `docs/vm/instruction-set.md`：理解 VM 指令集。
6. `docs/walkthroughs/index.md`：从测试反推实现。
7. `examples/README.md`：运行小脚本，观察解释器行为。

对应的实现主线是：

```text
source text
  -> Lexer / Parser
  -> AST
  -> SymbolRef
  -> ValueResult / CondResult / LValueRef / CallResultInfo
  -> Proto
  -> VM
```

## 深入源码路线

如果你想理解编译器：

- 从 `tests/unit/compiler/test_symbol_binding.cpp` 开始。
- 接着看 `test_value_pipeline.cpp`、`test_lvalue_pipeline.cpp`、`test_codegen_conditions.cpp`。
- 最后看 `test_call_pipeline.cpp` 和 `test_codegen_multret.cpp`。

如果你想理解运行时：

- 从 `tests/unit/vm/test_vm_core.cpp` 和 `tests/unit/vm/test_runtime_services.cpp` 开始。
- 接着看 `src/vm/state/lua_state.hpp`、`src/vm/vm.hpp`、`src/runtime/runtime_services.hpp`。
- 元方法慢路径从 `tests/unit/metamethod/` 和 `src/core/metatable.cpp` 开始。
- GC 路径先看 `docs/walkthroughs/gc-cycle.md`，再看 `docs/architecture/gc.md`。
- Trace 路径看 `docs/vm/trace-system.md`。

如果你想理解标准库：

- 从 `tests/unit/stdlib/test_lib_catalog.cpp` 开始。
- 再看 `src/lib/lib_catalog.hpp` 和 `src/lib/lib_manager.cpp`。
- `package` 行为从 `tests/unit/stdlib/test_packagelib.cpp` 开始。
- 总览看 `docs/stdlib/overview.md`。

## 按子项目阅读

| 子项目 | 文档 |
|---|---|
| 核心静态库 `lua.lib` / `lua_core` | `docs/projects/lua-lib.md` |
| 解释器 / REPL `lua_app` | `docs/projects/lua-app.md`, `docs/guides/repl-cli.md` |
| 测试入口 `lua_test` | `docs/projects/lua-test.md`, `docs/guides/test-runner.md` |
| 字节码工具 `lua_bytecode` | `docs/projects/lua-bytecode.md`, `docs/guides/bytecode-tool.md` |

## 示例脚本

`examples/` 目录提供可以直接运行的小脚本：

```powershell
bin\lua_app.exe examples\hello.lua
bin\lua_app.exe examples\control_flow.lua
bin\lua_app.exe examples\tables_and_methods.lua
bin\lua_app.exe examples\metamethods.lua
```

示例只覆盖当前解释器稳定支持的路径。想确认更细的语义时，以 `bin\lua_test.exe --filter <topic>` 的测试输出为准。
