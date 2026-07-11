---
status: current
verified_against: src/compiler/; src/vm/; src/runtime/; src/core/; src/gc/; tests/lua/regressions/; tests/lua/integration/
last_checked: 2026-07-11
applies_to: Execution Pipeline Overview
---

# Execution Pipeline Overview

本页是源码进入解释器后各阶段的权威导航。阶段细节不再拆成七个短页面；每个边界只记录输入、输出、不变量、源码责任区和验证证据。

## 端到端数据流

```text
source text
  → Lexer / TokenStream
  → Parser / AST Chunk
  → CodeGenerator / Proto
  → Function + LuaState
  → VM dispatch / CallInfo + register window
  → Runtime objects and services
  → observable Lua results
```

| 阶段 | 输入 → 输出 | 关键不变量 | 实现入口 |
|---|---|---|---|
| Load | 文件、字符串或 REPL chunk → 拥有生命周期的源码缓冲 | parser 生命周期内 `StrView` 不悬空；chunk name 与内容分离 | `src/io/`、`src/app/`、`src/repl/` |
| Tokenize | 字节序列 → 带行列的 token 流 | lookahead 不重复消费；错误 token 保留原始 lexeme 与位置 | `src/compiler/lexer/`、`src/compiler/parser/token.hpp` |
| Parse | token 流 → `Chunk` AST | 优先级和结合性稳定；scope 结构化；失败由 `ParseError` 表达 | `src/compiler/parser/`、`src/compiler/ast.hpp` |
| Compile | AST → `Proto` | 每条指令字段合法；寄存器生命周期不交叠；跳转已回填；lineInfo 与 code 对齐 | `src/compiler/codegen/`、`src/compiler/opcode.hpp` |
| Materialize | `Proto*` → Lua `Function` 与根调用帧 | Proto 由 GC 跟踪；函数对象、全局环境和 LuaState 属于同一运行时服务集合 | `src/core/function.*`、`src/runtime/runtime_services.*` |
| Execute | Proto + frame → 状态变化 | PC 只有一个更新责任；`base/top` 在帧窗口内；CALL/RETURN 遵守开放结果协议 | `src/vm/vm.cpp`、`src/vm/vm_handlers/` |
| Return | VM 结果 → API/CLI 可观察值 | 固定结果补 nil/截断；multret 保留开放 top；异常转换不丢 Lua 错误对象 | `src/vm/vm_call.cpp`、`src/vm/state/lua_state.cpp` |

## 阶段边界

### 源码所有权与位置

输入适配层负责拥有源码，lexer 只借用稳定视图。token 保存起始行列，CodeGen 将行号降维写入 `Proto::lineInfo`，运行时再由 `savedpc` 反查。这个链条使编译错误和 traceback 可以指向同一 chunk，但运行时不能恢复已经丢弃的列信息。

### AST 到 Proto

Parser 表达语言结构，CodeGen 表达执行约束。名字在生成阶段绑定为 local/global/upvalue，表达式进一步降为 `ValueResult`、`CondResult` 或 `LValueRef`，最后通过 `BytecodeBuilder` 发射指令。不能把寄存器号或跳转 PC 泄漏回 AST，否则 frontend 会被 VM 布局反向耦合。

### Proto 到 Function

`Proto` 是编译产物，`Function` 是运行时可调用对象。创建 closure 时，VM 还需要根据 `CLOSURE` 后的伪指令连接父帧局部槽或父 upvalue。两者分离使同一 Proto 能产生多个捕获环境不同的闭包。

### VM 到 Runtime

VM 负责指令顺序、寄存器窗口与调用协议；Runtime 对象负责值身份、table、metamethod、closure 和 GC 可达性。handler 可以调用运行时服务，但不应自行复制 table 或 GC 策略。依赖方向保持 `VM → Runtime/Core`，回调通过窄接口返回 VM。

## 跨阶段不变量

- opcode 必须同时有 CodeGen 生产证据和 VM handler 消费证据；覆盖矩阵验证双向闭合。
- `Proto::maxStackSize` 必须覆盖所有可达寄存器；frame 的 `base + maxStackSize` 不越过栈容量。
- CALL、TAILCALL、RETURN 和 VARARG 对“固定数量/开放数量”的编码必须一致。
- open upvalue 指向活动栈槽，帧离开前必须 close；关闭后多个闭包仍共享同一对象身份。
- GC root 集必须覆盖 LuaState、活动 frame、全局对象、开放 upvalue 与待终结对象。
- `std::expected` 用于模块 API 的可恢复失败，异常用于深层非局部退出；转换边界必须保留 Lua `Value` 错误对象。

## 阅读与验证

- 最小示例：[Hello World](hello-world-walkthrough.md)
- 包含 closure、metamethod、GC 的复杂示例：[全链路追踪](full-trace-example.md)
- Compiler：[字节码生成](../../compiler/bytecode-generation.md)
- VM：[运行时总览](../../vm/runtime/overview.md)
- Runtime：[值系统](../../runtime/value/overview.md)
- GC：[实现](../../gc/implementation.md)

端到端回归位于 `tests/lua/integration/` 和 `tests/lua/regressions/`；阶段内部契约由 `tests/unit/compiler/`、`tests/unit/vm/`、`tests/unit/core/` 与 `tests/unit/gc/` 锁定。
