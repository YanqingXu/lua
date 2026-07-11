---
status: current
verified_against: src/compiler/codegen/codegen.hpp; src/compiler/codegen/codegen.cpp; src/compiler/codegen/codegen_binding.cpp; src/compiler/codegen/name_binder.hpp; src/compiler/codegen/codegen_ops.hpp; src/compiler/codegen/function_compiler.hpp; src/compiler/codegen/expression_emitter.hpp; src/compiler/codegen/statement_emitter.hpp; src/compiler/codegen/codegen_types.hpp; src/compiler/codegen/codegen_state.hpp; src/compiler/codegen/jump_patcher.hpp; src/compiler/codegen/scope_manager.hpp; src/compiler/codegen/bytecode_builder.hpp; tests/unit/compiler/test_codegen_characterization.cpp
last_checked: 2026-07-11
applies_to: CodeGenerator responsibility boundaries and characterization guardrails
---

# CodeGenerator 职责地图

`CodeGenerator` 是 AST 到 `Proto` 的编译总控和稳定公开入口。具体 lowering、状态管理与字节码写入由窄职责组件承担，避免 facade 同时拥有语义决策、寄存器状态和跳转链。

## 组件边界

| 职责域 | 入口或关键方法 | 代码落点 | 边界 |
|---|---|---|---|
| Public facade | `generate()`、`tryGenerate()`、`generateUnchecked()` | `codegen.cpp` | 管线编排和公开 API |
| 共享状态 | `CodegenState` | `codegen_state.hpp` | 当前 `Proto`、服务、寄存器、作用域、PC 与行号 |
| 指令写入 | `CodegenOps::codeABC()`、`codeABx()`、`codeAsBx()`、`patchArg*()` | `codegen_ops.hpp`, `BytecodeBuilder` | 低层发射、参数回填和 guard |
| 寄存器与常量 | `RegisterAllocator`、`BytecodeBuilder::add*Constant()` | allocator、builder 和 emitters | 资源分配与常量去重 |
| 名字绑定 | `NameBinder::resolve()`、`symbolToValue()`、`symbolToLValue()` | `name_binder.*`, `codegen_binding.cpp` | local、upvalue、global 到显式引用类型 |
| 作用域 | `ScopeManager::enterBlock()`、`leaveBlock()`、`resolveUpvalue()` | `scope_manager.*` | local 生命周期、block、breaklist 和捕获 |
| 跳转回填 | `emitJump()`、`patchList()`、`patchToHere()`、`flushPendingJumps()` | `jump_patcher.*` | 跳转链、偏移和 pending jump |
| 表达式 lowering | `emitValue()`、`emitCondResult()`、`emitLValue()` | `expression_emitter.*` | 右值、条件、左值、调用、vararg 和 table 表达式 |
| 语句 lowering | `statement()`、`emitStmt()`、`block()` | `statement_emitter.*` | 语句、循环、return 和块控制流 |
| 函数编译 | `compile()`、`emitClosureUpvalues()`、`attachDebugMetadata()` | `function_compiler.*` | 子 `Proto` 生命周期、closure upvalue 和调试元数据 |

## 中间结果类型

- `SymbolRef`：名字绑定结果，区分 local、upvalue 和 global。
- `ValueResult`：表达式右值，只保存受约束的 variant payload。
- `CondResult`：尚未物化为布尔寄存器的 true/false 跳转集合。
- `LValueRef`：可写位置，区分 local、upvalue、global 和 table slot。
- `CallResultInfo`：调用或 vararg 的 base、指令位置和结果数量契约。

普通代码通过 `ValueResult::visit()` 读取 payload；旧字段 mirror、`LegacyFields` 和兼容探针不得重新成为数据通道。

## Characterization 护栏

`tests/unit/compiler/test_codegen_characterization.cpp` 锁定语义和字节码形状，而不锁死无意义的绝对 PC：

| 测试区域 | 保护内容 |
|---|---|
| Structured statements | if、while、repeat、numeric/generic for 的跳转全部回填 |
| Loop and scope semantics | break、do block、repeat-until body local 和 upvalue close |
| Generic for shape | `TFORLOOP`、回边和 pending jump 不变量 |
| Jump Patcher | jump-list 方向、`PatchList`、`TESTSET` 规约和过长跳转错误 |
| Scope Manager | local 生命周期、冗余 `CLOSE` 抑制、breaklist 和 upvalue 去重 |
| Expression Emitter | value/condition/lvalue 返回契约和 payload 物化 |
| Statement Emitter | statement/block facade 和空语句 lowering |
| Lua 5.1 parity | `T.listcode` 关注的可观察字节码形状 |

## 不变量

- `JumpPatcher` 不拥有 statement 或 expression lowering。
- `ScopeManager` 不拥有表达式或语句发射。
- `ExpressionEmitter` 不拥有 statement lowering。
- `CodeGenerator` 不重新吸收已经分离的 lowering 细节。
- 新增生产源文件时必须同步 CMake、`.vcxproj` 与 `.vcxproj.filters` 源清单。
