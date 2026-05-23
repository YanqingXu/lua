---
status: current
verified_against: README.md; lua.slnx; lua.vcxproj; lua_app.vcxproj; lua_test.vcxproj; lua_bytecode.vcxproj; CMakeLists.txt; tools/run_cmake_smoke.ps1; tools/check_doc_drift.ps1; tools/test_quality_gate.ps1; tools/run_quality_gate.ps1; .github/workflows/ci.yml; tests/unit/framework/test_runner.cpp; tests/unit/compiler/test_parser_boundaries.cpp; tests/unit/compiler/test_codegen_characterization.cpp; tests/unit/compiler/test_jump_patcher.cpp; tests/unit/compiler/test_scope_manager.cpp; tests/unit/compiler/test_expression_emitter.cpp; tests/unit/compiler/test_statement_emitter.cpp; tests/unit/compiler/test_codegen_state.cpp; tests/unit/compiler/test_bytecode_builder.cpp; tests/unit/vm/test_runtime_services.cpp; tests/unit/vm/test_vm_dispatch.cpp; tests/unit/vm/test_vm_internal_boundaries.cpp; tests/unit/vm/test_vm_trace_debug.cpp; docs/index.md; docs/glossary.md; docs/walkthroughs/index.md; docs/walkthroughs/gc-cycle.md; docs/compiler/codegen-responsibility-map.md; examples/README.md; src/runtime/runtime_services.hpp; src/compiler/parser/parser.hpp; src/compiler/parser/parser.cpp; src/compiler/parser/parser_stmt.cpp; src/compiler/parser/parser_expr.cpp; src/compiler/parser/parser_primary.cpp; src/compiler/parser/parser_func.cpp; src/compiler/parser/parser_table.cpp; src/compiler/codegen/codegen.cpp; src/compiler/codegen/codegen_binding.cpp; src/compiler/codegen/codegen_expr.cpp; src/compiler/codegen/expression_emitter.hpp; src/compiler/codegen/expression_emitter.cpp; src/compiler/codegen/codegen_jump.cpp; src/compiler/codegen/codegen_stmt.cpp; src/compiler/codegen/statement_emitter.hpp; src/compiler/codegen/statement_emitter.cpp; src/compiler/codegen/codegen_state.hpp; src/compiler/codegen/jump_patcher.hpp; src/compiler/codegen/jump_patcher.cpp; src/compiler/codegen/scope_manager.hpp; src/compiler/codegen/scope_manager.cpp; src/compiler/codegen/bytecode_builder.hpp; src/vm/state/lua_state.hpp; src/vm/state/global_state.hpp; src/vm/state/stack.hpp; src/vm/state/call_info.hpp; src/vm/vm.cpp; src/vm/vm_entry.cpp; src/vm/vm_dispatch.hpp; src/vm/vm_switch_dispatch.hpp; src/vm/vm_dispatch_strategy.hpp; src/vm/vm_internal.hpp; src/vm/vm_ops.cpp; src/vm/vm_call.cpp; src/vm/vm_table.cpp; src/vm/vm_frame.cpp; src/vm/vm_loop.cpp; src/vm/vm_trace.cpp
last_checked: 2026-05-23
applies_to: 当前仓库事实与面向贡献者的工作流
---

# 项目状态

本文档是仓库对外可见事实的单一事实源。README 和开发文档应引用这里，而不是在多处重复维护构建、测试和编译器管线状态。

## 当前构建路径

- 主要平台：Windows。
- 主要 IDE / 工具链：Visual Studio / MSBuild。
- 主要解决方案入口：`lua.slnx`。
- 当前有效项目文件：
  - `lua.vcxproj`：核心静态库。
  - `lua_app.vcxproj`：解释器 / REPL 可执行程序。
  - `lua_test.vcxproj`：单元测试可执行程序。
  - `lua_bytecode.vcxproj`：字节码查看工具可执行程序。
- 项目文件中记录的 MSVC platform toolset：`v145`。
- 项目文件中的 C++ 标准设置混合使用 `stdcpp20` 和 `stdcpp23`；当前 x64 Debug 活跃配置使用 `stdcpp23`。

## 辅助构建路径

- CMake 和 CTest 已作为 secondary 构建 / 测试路径存在，但不替代主要的 Visual Studio/MSBuild 工作流。
- 仓库根目录的 `CMakeLists.txt` 会构建 `lua_core`、`lua_app`、`lua_bytecode` 和 `lua_test`。
- 本地 secondary 烟测入口：`tools/run_cmake_smoke.ps1`。
- CTest 当前注册了一个单元测试可执行程序测试，以及 `examples/` 下的示例 Lua 脚本。

## 项目目标状态

- `lua.vcxproj` / `lua_core`：当前核心库目标。
- `lua_app.vcxproj` / `lua_app`：当前脚本执行和 REPL 可执行目标；REPL 支持 `.help`、`.bytecode <expr|chunk>`、`.ast <expr|chunk>`、`.gc [stats|collect|strategy|help]`、Tab 补全、REPL 终端彩色错误、行号 prompt、历史记录和 `=expr` 快速求值；实现已拆分为 `src/repl.cpp` 会话入口和 `src/repl/repl_*` 子模块。
- `lua_test.vcxproj` / `lua_test`：当前单元测试可执行目标。
- `lua_bytecode.vcxproj` / `lua_bytecode`：编译到 `Proto` 的工具目标；`src/bytecode/bytecode_printer.cpp` 目前已经能输出 Proto 头信息、decoded instructions、常量注释、constant table，在 `full` 模式递归打印 child protos，并支持 `--diff` side-by-side 字节码差异；CFG 模式仍未实现。

## 计划中的构建路径工作

- 在把 CMake 视为跨平台契约前，需要把验证范围从当前 Windows 烟测路径继续扩展。
- install / export / packaging 规则应等源码目标边界稳定后再加入。

## 测试状态

- 测试框架：自定义轻量级 C++ 测试框架，vendored 在 `lua_test/include/test_framework`，由 `tests/unit/framework` 适配。
- 最近验证的测试计数：531 个 registered tests，2645 个 assertion results，0 failures。
- `bin\lua_test.exe` 支持 `--list`、`--filter <suite-or-name>` 和 `--report=junit`。
- 这些数字描述的是项目测试运行器结果，不是 Lua 5.1.5 兼容率百分比。

## 质量门状态

- 格式化配置：`.clang-format`，基于 LLVM，并带有仓库自己的宽度和 include 排序选择。
- 静态分析配置：`.clang-tidy`，当前限制在保守的 `bugprone-*`、`performance-*`、`portability-*` 和部分 `readability-*` 检查。
- 本地质量门入口：`tools/run_quality_gate.ps1`，包括 clang-format、clang-tidy smoke、opcode coverage matrix、MSBuild、documentation drift 和 unit tests。
- 质量门自检入口：`tools/test_quality_gate.ps1`。
- 文档漂移守卫：`tools/check_doc_drift.ps1`，会从 `bin\lua_test.exe` 汇总输出动态解析当前测试计数，并检查 README / status 文档没有落后。
- CI 平台：GitHub Actions；入口文件为 `.github/workflows/ci.yml`，当前以 Windows/MSBuild 作为主要工作流，并先构建 `lua_test` 再运行文档漂移检查。
- 质量门有意采用增量策略：本地格式化默认只检查变更过的源文件；本机缺少 `clang-format` 或 `clang-tidy` 时，本地脚本会明确报告跳过；MSBuild 和单元测试仍是 Windows 路径下的标准验证方式。
- `tools/check_opcode_coverage_matrix.ps1` 从 `src/compiler/opcode.hpp` 解析 38 条 opcode，并校验 `tests/unit/vm/opcode_coverage_matrix.md` 的覆盖矩阵同步。

## 编译器管线状态

- `ExprDesc` 和 `ExprKind` 已从产品编译器源码中移除。
- `CodeGenerator` 的 public API 保留在 `src/compiler/codegen/codegen.hpp`，实现已物理拆分到 `src/compiler/codegen/codegen.cpp`、`src/compiler/codegen/codegen_binding.cpp`、`src/compiler/codegen/codegen_expr.cpp`、`src/compiler/codegen/expression_emitter.cpp`、`src/compiler/codegen/statement_emitter.cpp`、`src/compiler/codegen/codegen_jump.cpp` 和 `src/compiler/codegen/codegen_stmt.cpp`。
- `docs/compiler/codegen-responsibility-map.md` 记录了 PR-48 后的 CodeGenerator 职责地图；`JumpPatcher` 已抽出 jump-list / pending-jump / PC offset 回填边界，`ScopeManager` 已抽出 local / block / upvalue 作用域生命周期边界，`ExpressionEmitter` 已抽出 ValueResult / CondResult / CallResultInfo / LValueRef 表达式通道边界，`StatementEmitter` 已抽出 statement / block lowering 边界，`ValueResult` 已有兼容式 `std::variant` payload 原型；`Codegen Characterization`、`Jump Patcher`、`Scope Manager`、`Expression Emitter`、`Statement Emitter` 与 `Codegen Result Types` 测试套件共同锁住 statement lowering、jump patching、repeat-until scope、generic-for、表达式 lowering 和 ValueResult payload 同步行为。
- `src/compiler/codegen/codegen_state.hpp` 集中管理这些实现分片共享的可变生成状态，包括当前 `Proto`、程序计数器、行号、寄存器分配器、局部作用域、块管理器和 upvalue 上下文。
- `src/compiler/codegen/bytecode_builder.hpp` 集中管理对当前 `Proto` 的字节码发射写入，包括指令创建、行号信息、常量、子 Proto、指令替换和局部调试元数据。
- 当前字节码生成文档应解释这条管线：

```text
AST
  -> SymbolRef
  -> ValueResult / CondResult / LValueRef / CallResultInfo
  -> Proto
```

- 历史 `ExprDesc` 说明归档在 `docs/archive/history/exprdesc.md`。
- 漂移守卫：

```powershell
rg "ExprDesc|ExprKind|expdesc" src/compiler
```

上面的命令对产品编译器源码必须没有匹配结果。

## 运行时边界状态

- `src/runtime/runtime_services.hpp` 定义了 `RuntimeServices`，它是当前对 `GlobalState`、`StringPool`、`GarbageCollector` 以及可选 `VM::DispatchStrategy* dispatchStrategy` 的显式兼容层。
- `CodeGenerator`、`Parser`、`LuaState` 和 `VM` 都暴露了 context-aware 的构造 / 执行重载，同时保留基于单例的兼容重载。
- `GarbageCollector::sweep(StringPool&)` 和 `clearAll(StringPool&)` 已显式接收字符串池，用于在删除 `GCString` 时同步摘除驻留表；旧 `GarbageCollector::getInstance()` 已标记为 `[[deprecated]]` 兼容 shim。
- `src/compiler/parser/parser.cpp` 现在保留 Parser 构造、token / error 处理、同步恢复和顶层 parse 入口；`src/compiler/parser/parser_stmt.cpp`、`src/compiler/parser/parser_expr.cpp`、`src/compiler/parser/parser_primary.cpp`、`src/compiler/parser/parser_func.cpp` 和 `src/compiler/parser/parser_table.cpp` 承载具体语法产生式分片，并由 `Parser Boundary Sentinels` 覆盖。
- `src/main.cpp`、`src/repl.cpp` / `src/repl/repl_*` 和 `src/bytecode/bytecode_main.cpp` 已在第一批编译器 / VM 入口分片中使用 `RuntimeServices`。
- `src/vm/vm_entry.cpp` 现在承载 `VM::call()` 和 `VM::execute()` 入口点；`src/vm/vm.cpp` 保留主字节码 dispatch 循环。
- VM 状态和栈帧存储类型位于 `src/vm/state/`：`lua_state.*`、`global_state.*`、`stack.*` 和 `call_info.hpp`。
- `src/vm/vm_dispatch.hpp` 对 opcode 家族和可触发元方法的 opcode 做分类，是第一层 VM dispatch 拆分边界。
- `src/vm/vm_switch_dispatch.hpp` 为 `SwitchDispatch` 路径提供 38 个 opcode-specific inline entry point，使 switch 后端的单步调试路径与 table 后端可区分。
- `src/vm/vm_ops.cpp` 包含 table / metamethod、算术、比较、一元、长度和 concat helper；`src/vm/vm_call.cpp` 包含 precall、postcall 和 tailcall 栈帧复用 helper。
- `src/vm/vm_table.cpp`、`src/vm/vm_frame.cpp` 和 `src/vm/vm_loop.cpp` 分别包含 SETLIST、closure / vararg 和 TFORLOOP helper。
- `src/vm/vm_trace.cpp` 包含 trace sink 状态、debug hook 分发、trace event 构造和 `changedRegisters` 差异计算；`src/vm/vm.cpp` 保留主 dispatch 循环以及普通 trace / `--trace-diff` 的事件触发点。
- `src/vm/vm_internal.hpp` 是 VM 内部实现分片边界，包含 `captureRuntimeErrors` / `mapExceptionToUnexpected` 这类可复用 expected 边界 helper；由 `tests/unit/vm/test_runtime_services.cpp`、`tests/unit/vm/test_vm_internal_boundaries.cpp` 以及 `tests/unit/vm/test_vm_trace_debug.cpp` 覆盖。

## 学习路径状态（Learning Path）

- `docs/index.md` 是新贡献者第一次阅读仓库的入口。
- `docs/glossary.md` 将 Lua 术语映射到当前仓库类型和文件。
- `docs/walkthroughs/index.md` 把关键测试套件整理为编译器、VM、运行时和标准库的引导式阅读路线；`hello-world.md`、`closure-and-upvalue.md` 和 `gc-cycle.md` 覆盖三条端到端机制 walkthrough。
- `docs/projects/` 记录四个构建目标。
- `docs/guides/` 记录贡献者流程和可执行程序使用方式。
- `docs/architecture/`、`docs/compiler/`、`docs/vm/` 和 `docs/stdlib/` 保存面向实现的当前文档。
- `examples/` 包含小型 Lua 脚本；构建 app 目标后，可用 `bin\lua_app.exe` 运行。

## 文档状态规则

每个核心文档文件都应以下列页眉开头：

```yaml
---
status: current|historical|planned
verified_against: <files or commands used as evidence>
last_checked: YYYY-MM-DD
applies_to: <scope>
---
```

只有文档描述的是本仓库当前已经存在的实现时，才使用 `current`。已完成重构记录和已移除设计使用 `historical`。尚未实现的未来架构使用 `planned`。
