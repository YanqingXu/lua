---
status: current
verified_against: docs/status/project-status.md; docs/compiler/bytecode-generation.md; src/runtime/runtime_services.hpp; src/compiler/codegen/codegen_types.hpp; src/vm/vm.hpp
last_checked: 2026-05-19
applies_to: Lua terminology mapped to current repository code
---

# Glossary

这份术语表把 Lua 概念、仓库类型和主要文件放在一起。它不是完整规范，而是读代码时的地图。

| 术语 | 在本仓库中的对应物 | 入口文件 | 说明 |
|---|---|---|---|
| Chunk | `Chunk` | `src/compiler/ast.hpp` | 一段 Lua 源码解析后的顶层 AST 节点。 |
| AST | `Expr` / `Stmt` 派生节点 | `src/compiler/ast.hpp` | Parser 产出的语法树，CodeGenerator 的输入。 |
| Lexer | `Lexer` | `src/compiler/parser/lexer.hpp` | 把源码切成 Token。 |
| Parser | `Parser` | `src/compiler/parser/parser.hpp` | 把 Token 流转换成 AST；现在支持接收 `RuntimeServices` 的构造重载。 |
| Token | `Token` / `TokenType` | `src/compiler/parser/token.hpp` | Parser 消费的词法单元。 |
| Proto | `Proto` | `src/core/function.hpp` | Lua 函数原型，保存字节码、常量表、子函数、调试信息。 |
| Function | `Function` | `src/core/function.hpp` | 可执行函数对象，可能包装 `Proto` 或 C 函数。 |
| Value | `Value` | `src/core/value.hpp` | Lua 值的统一表示，覆盖 nil、boolean、number、string、table、function 等。 |
| Table | `Table` | `src/core/table.hpp` | Lua 表对象，承载数组部分、哈希部分和元表。 |
| StringPool | `StringPool` | `src/core/string_pool.hpp` | 字符串驻留池；新入口优先通过 `RuntimeServices` 显式传递。 |
| GlobalState | `GlobalState` | `src/vm/state/global_state.hpp` | 共享运行时状态，包含字符串池、GC、registry、基础类型元表和元方法名称。 |
| RuntimeServices | `RuntimeServices` | `src/runtime/runtime_services.hpp` | 当前显式运行时服务边界，封装 `GlobalState`、`StringPool`、`GarbageCollector`。 |
| LuaState | `LuaState` | `src/vm/state/lua_state.hpp` | 单个 Lua 线程/协程的执行状态，包含栈、调用帧、全局表和 hook 状态。 |
| Stack | `Stack` | `src/vm/state/stack.hpp` | VM 执行时的值栈。 |
| CallInfo | `CallInfo` | `src/vm/state/call_info.hpp` | 单个调用帧的信息，包括函数位置、base、top、savedpc 和期望返回值。 |
| VM | `Lua::VM` 自由函数 | `src/vm/vm.hpp` | 执行 `Proto` 的字节码解释器；已有可接收 `RuntimeServices` 的 context-aware 执行重载。 |
| OpCode | `OpCode` | `src/compiler/opcode.hpp` | Lua 5.1 风格的字节码指令枚举。 |
| RK | `RK` 操作数编码 | `src/compiler/opcode.hpp`, `src/vm/vm.cpp` | 指令操作数可指向寄存器或常量表，VM 通过 `ISK` / `INDEXK` 区分。 |
| SymbolRef | `SymbolRef` | `src/compiler/codegen/codegen_types.hpp` | 名字绑定结果，描述 local、upvalue 或 global。 |
| ValueResult | `ValueResult` | `src/compiler/codegen/codegen_types.hpp` | 表达式右值通道，描述常量、寄存器、pending load 或 call 结果。 |
| CondResult | `CondResult` | `src/compiler/codegen/codegen_types.hpp` | 条件表达式通道，携带 true/false 跳转列表。 |
| LValueRef | `LValueRef` | `src/compiler/codegen/codegen_types.hpp` | 赋值左侧通道，描述 local、upvalue、global 或 table slot。 |
| CallResultInfo | `CallResultInfo` | `src/compiler/codegen/codegen_types.hpp` | 调用/vararg 多返回值通道，记录 base register 和指令位置。 |
| Upvalue | `Upvalue` | `src/core/upvalue.hpp` | 闭包捕获的外层局部变量；VM 负责 open/closed 生命周期。 |
| Environment | `Function::env` | `src/core/function.hpp` | 函数的全局环境表，用于 `GETGLOBAL` / `SETGLOBAL` 路径。 |
| Registry | `GlobalState::getRegistry()` | `src/vm/state/global_state.hpp` | C API 风格的全局注册表，目前由 `GlobalState` 持有。 |
| Metatable | `Table::getMetatable()` | `src/core/table.hpp`, `src/core/metatable.hpp` | 自定义对象行为的表。 |
| TMS | `TMS` | `src/core/metatable.hpp` | Tag Method System 元方法枚举，如 `TM_INDEX`、`TM_ADD`、`TM_CALL`。 |
| Standard Library Catalog | `LibModule` catalog | `src/lib/lib_catalog.hpp` | 标准库表驱动注册入口，集中描述库名和 open 函数。 |
| Test Registry | `TestRegistry` | `lua_test/include/test_framework/test_framework.hpp` | 自定义测试框架的注册和执行中心，支持 `--list`、`--filter`、`--report=junit`。 |

## 推荐交叉阅读

- 编译管线：`docs/compiler/bytecode-generation.md`
- 测试学习路径：`docs/walkthroughs/index.md`
- 当前事实：`docs/status/project-status.md`
- 后续任务：`docs/roadmap/current.md`
