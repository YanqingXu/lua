---
status: current
verified_against: docs/roadmap/current.md; docs/index.md; docs/glossary.md; docs/walkthroughs/hello-world.md; docs/walkthroughs/closure-and-upvalue.md; examples/README.md; tests/unit/framework/test_runner.cpp; bin/lua_test.exe --list
last_checked: 2026-05-22
applies_to: test-based learning path for compiler, VM, runtime, and standard library behavior
---

# Walkthrough Test Index

这份索引把现有测试从“验证清单”整理成“阅读路线”。如果你想理解这个解释器如何把 Lua 源码变成字节码、如何执行、以及标准库如何装配，可以先从这些测试切片读起，再回到实现文件。

第一次阅读请先看 `docs/index.md` 和 `docs/glossary.md`。如果你更想先运行代码，再看 `examples/README.md`。

## Runner Commands

```powershell
# 列出所有已注册测试用例
bin\lua_test.exe --list

# 只运行某个测试套件或名称片段
bin\lua_test.exe --filter "Symbol Binding"

# 生成 CI 可消费的 JUnit XML 报告
bin\lua_test.exe --report=junit
```

`--filter` 会匹配测试套件名、测试名或 `Suite::Name` 完整名；大小写不敏感。

## Suggested Reading Path

| 顺序 | 主题 | 推荐过滤器 | 重点文件 | 读完后应理解 |
|---|---|---|---|---|
| 1 | 值、表和函数对象 | `Value` / `Table` / `Function` | `tests/unit/core/test_value.cpp`, `tests/unit/core/test_table.cpp`, `tests/unit/core/test_function.cpp` | Lua 值类型、表存储、函数/Proto 的基础形态 |
| 1.5 | Hello World 端到端 | `VM Dispatch` / `Value Pipeline` | `docs/walkthroughs/hello-world.md`, `src/compiler/codegen/codegen_expr.cpp`, `src/vm/vm.cpp`, `src/lib/baselib.cpp` | `print("hello")` 如何从源码走到字节码、VM 调度和 C 函数输出 |
| 2 | LuaState 与 VM 基础 | `VM Core` / `LuaState Init` | `tests/unit/vm/test_vm_core.cpp`, `tests/unit/vm/test_lua_state_init.cpp` | 栈、全局表、固定字符串、基础 VM 执行状态 |
| 2.5 | 显式运行时边界 | `Runtime Services` | `tests/unit/vm/test_runtime_services.cpp`, `src/runtime/runtime_services.hpp` | `RuntimeServices` 如何把单例兼容层变成可传递的上下文 |
| 3 | 词法和解析边界 | `Lexer` / `Parser` | `tests/unit/compiler/test_lexer_number.cpp`, `tests/unit/compiler/test_parser_error_recovery.cpp` | 源码如何进入 AST，以及错误恢复的边界 |
| 4 | 符号绑定 | `Symbol Binding` | `tests/unit/compiler/test_symbol_binding.cpp` | local/global/upvalue 如何被解析为 `SymbolRef` 并映射到读写通道 |
| 5 | 值表达式通道 | `Value Pipeline` | `tests/unit/compiler/test_value_pipeline.cpp` | literal/name/index/call 如何生成 `ValueResult` 并落到运行时行为 |
| 6 | 左值写入通道 | `LValue Pipeline` | `tests/unit/compiler/test_lvalue_pipeline.cpp` | local/global/table/upvalue 赋值如何降低为可写目标 |
| 7 | 条件与短路 | `Codegen Conditions` | `tests/unit/compiler/test_codegen_conditions.cpp` | `and`、`or`、`not` 和条件上下文如何使用跳转列表避免错误物化 |
| 8 | 多返回值 | `Codegen MultiRet` / `Call Pipeline` | `tests/unit/compiler/test_codegen_multret.cpp`, `tests/unit/compiler/test_call_pipeline.cpp` | 函数调用、vararg、括号、表构造和返回语句中的多返回值规则 |
| 9 | 函数编译与调用 | `Function Codegen` / `Function Call` | `docs/walkthroughs/closure-and-upvalue.md`, `tests/unit/compiler/test_function_codegen.cpp`, `tests/unit/vm/test_function_call.cpp` | 函数定义、闭包、递归、upvalue 生命周期和调用帧如何串起来 |
| 10 | 元方法 | `Metamethod` | `tests/unit/metamethod/test_metamethod_arith.cpp`, `tests/unit/metamethod/test_metamethod_complete.cpp` | 算术、索引、调用等慢路径如何委托给元方法 |
| 11 | GC 与 upvalue 生命周期 | `GC` | `tests/unit/gc/test_gc.cpp` | 根集、弱表、终结器和 upvalue 关闭语义 |
| 12 | 协程 | `Coroutine Library` | `tests/unit/stdlib/test_coroutinelib.cpp` | `resume`/`yield`、状态切换、wrap 和多值传递 |
| 13 | 标准库装配 | `Standard Library Catalog` / `Package Library` | `tests/unit/stdlib/test_lib_catalog.cpp`, `tests/unit/stdlib/test_packagelib.cpp` | 标准库默认加载顺序、`package.loaded` 缓存和 `require` 路径 |

## Topic Shortcuts

| 想理解 | 先运行 | 再读 |
|---|---|---|
| 名字如何绑定到 local/global/upvalue | `bin\lua_test.exe --filter "Symbol Binding"` | `src/compiler/codegen/codegen_binding.cpp`, `src/compiler/codegen/codegen.hpp` |
| 闭包如何捕获并关闭 upvalue | `bin\lua_test.exe --filter "Function Codegen"` | `docs/walkthroughs/closure-and-upvalue.md`, `src/compiler/codegen/codegen_stmt.cpp`, `src/vm/vm_frame.cpp`, `src/vm/state/lua_state.cpp` |
| 条件和短路如何避免错误物化 | `bin\lua_test.exe --filter "Codegen Conditions"` | `src/compiler/codegen/codegen_expr.cpp`, `src/compiler/codegen/codegen_jump.cpp`, `src/compiler/codegen/codegen_types.hpp` |
| 多返回值为什么依赖调用位置 | `bin\lua_test.exe --filter "Call Pipeline"` | `src/compiler/codegen/codegen_types.hpp`, `src/compiler/codegen/expression_emitter.cpp`, `src/compiler/codegen/statement_emitter.cpp`, `src/vm/vm.cpp` |
| RuntimeServices 当前解决了什么 | `bin\lua_test.exe --filter "Runtime Services"` | `src/runtime/runtime_services.hpp`, `src/main.cpp`, `src/repl.cpp` |
| 标准库如何统一装配 | `bin\lua_test.exe --filter "Standard Library Catalog"` | `src/lib/lib_catalog.hpp`, `src/lib/lib_manager.cpp`, `docs/stdlib/overview.md` |
| `print("hello")` 如何走完整链路 | `bin\lua_test.exe --filter "VM Dispatch"` | `docs/walkthroughs/hello-world.md` |
| `local function` 返回闭包后变量为什么还活着 | `bin\lua_test.exe --filter "Symbol Binding"` | `docs/walkthroughs/closure-and-upvalue.md` |

## Example Pairings

| 示例 | 配套测试 | 说明 |
|---|---|---|
| `examples/hello.lua` | `Value Pipeline` | 字符串常量和连接表达式如何进入运行时。 |
| `examples/control_flow.lua` | `Codegen Conditions` | `if`、`while` 和比较跳转如何配合。 |
| `examples/tables_and_methods.lua` | `Method Call` / `LValue Pipeline` | 冒号调用和表字段写入如何降低。 |
| `examples/metamethods.lua` | `Metamethod` | 算术慢路径如何通过元方法返回新值。 |

## Walkthrough Workflow

1. 先用 `bin\lua_test.exe --list` 找到套件和测试名称。
2. 用 `--filter` 运行一个小范围，确认输出里哪些断言属于这个主题。
3. 打开对应测试文件，从注册函数附近开始读，因为注册顺序通常就是教学顺序。
4. 读完测试后再跳到 `src/` 中的实现文件，对照测试输入、期望字节码和运行时断言。
5. 如果你新增了能解释关键机制的测试，把它加到本索引，而不是只把它留在测试输出里。
