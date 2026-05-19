---
status: current
verified_against: docs/OPTIMIZATION_ROADMAP.md; docs/deep-research-report.md; tests/unit/framework/test_runner.cpp; bin/lua_test.exe --list
last_checked: 2026-05-19
applies_to: test-based learning path for compiler, VM, runtime, and standard library behavior
---

# Walkthrough Test Index

这份索引把现有测试从“验证清单”整理成“阅读路线”。如果你想理解这个解释器如何把 Lua 源码变成字节码、如何执行、以及标准库如何装配，可以先从这些测试切片读起，再回到实现文件。

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
| 2 | LuaState 与 VM 基础 | `VM Core` / `LuaState Init` | `tests/unit/vm/test_vm_core.cpp`, `tests/unit/vm/test_lua_state_init.cpp` | 栈、全局表、固定字符串、基础 VM 执行状态 |
| 3 | 词法和解析边界 | `Lexer` / `Parser` | `tests/unit/compiler/test_lexer_number.cpp`, `tests/unit/compiler/test_parser_error_recovery.cpp` | 源码如何进入 AST，以及错误恢复的边界 |
| 4 | 符号绑定 | `Symbol Binding` | `tests/unit/compiler/test_symbol_binding.cpp` | local/global/upvalue 如何被解析为 `SymbolRef` 并映射到读写通道 |
| 5 | 值表达式通道 | `Value Pipeline` | `tests/unit/compiler/test_value_pipeline.cpp` | literal/name/index/call 如何生成 `ValueResult` 并落到运行时行为 |
| 6 | 左值写入通道 | `LValue Pipeline` | `tests/unit/compiler/test_lvalue_pipeline.cpp` | local/global/table/upvalue 赋值如何降低为可写目标 |
| 7 | 条件与短路 | `Codegen Conditions` | `tests/unit/compiler/test_codegen_conditions.cpp` | `and`、`or`、`not` 和条件上下文如何使用跳转列表避免错误物化 |
| 8 | 多返回值 | `Codegen MultiRet` / `Call Pipeline` | `tests/unit/compiler/test_codegen_multret.cpp`, `tests/unit/compiler/test_call_pipeline.cpp` | 函数调用、vararg、括号、表构造和返回语句中的多返回值规则 |
| 9 | 函数编译与调用 | `Function Codegen` / `Function Call` | `tests/unit/compiler/test_function_codegen.cpp`, `tests/unit/vm/test_function_call.cpp` | 函数定义、闭包、递归和调用帧如何串起来 |
| 10 | 元方法 | `Metamethod` | `tests/unit/metamethod/test_metamethod_arith.cpp`, `tests/unit/metamethod/test_metamethod_complete.cpp` | 算术、索引、调用等慢路径如何委托给元方法 |
| 11 | GC 与 upvalue 生命周期 | `GC` | `tests/unit/gc/test_gc.cpp` | 根集、弱表、终结器和 upvalue 关闭语义 |
| 12 | 协程 | `Coroutine Library` | `tests/unit/stdlib/test_coroutinelib.cpp` | `resume`/`yield`、状态切换、wrap 和多值传递 |
| 13 | 标准库装配 | `Standard Library Catalog` / `Package Library` | `tests/unit/stdlib/test_lib_catalog.cpp`, `tests/unit/stdlib/test_packagelib.cpp` | 标准库默认加载顺序、`package.loaded` 缓存和 `require` 路径 |

## Walkthrough Workflow

1. 先用 `bin\lua_test.exe --list` 找到套件和测试名称。
2. 用 `--filter` 运行一个小范围，确认输出里哪些断言属于这个主题。
3. 打开对应测试文件，从注册函数附近开始读，因为注册顺序通常就是教学顺序。
4. 读完测试后再跳到 `src/` 中的实现文件，对照测试输入、期望字节码和运行时断言。
5. 如果你新增了能解释关键机制的测试，把它加到本索引，而不是只把它留在测试输出里。
