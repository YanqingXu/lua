---
status: current
verified_against: src/compiler/codegen/codegen.hpp; src/compiler/codegen/codegen.cpp; src/compiler/codegen/codegen_binding.cpp; src/compiler/codegen/name_binder.hpp; src/compiler/codegen/name_binder.cpp; src/compiler/codegen/codegen_ops.hpp; src/compiler/codegen/function_compiler.hpp; src/compiler/codegen/function_compiler.cpp; src/compiler/codegen/expression_emitter.hpp; src/compiler/codegen/expression_emitter.cpp; src/compiler/codegen/statement_emitter.hpp; src/compiler/codegen/statement_emitter.cpp; src/compiler/codegen/codegen_stmt.cpp; src/compiler/codegen/codegen_types.hpp; src/compiler/codegen/codegen_state.hpp; src/compiler/codegen/jump_patcher.hpp; src/compiler/codegen/jump_patcher.cpp; src/compiler/codegen/scope_manager.hpp; src/compiler/codegen/scope_manager.cpp; src/compiler/codegen/bytecode_builder.hpp; tests/unit/compiler/test_codegen_result_types.cpp; tests/unit/compiler/test_codegen_characterization.cpp; tests/unit/compiler/test_jump_patcher.cpp; tests/unit/compiler/test_scope_manager.cpp; tests/unit/compiler/test_expression_emitter.cpp; tests/unit/compiler/test_statement_emitter.cpp; tests/unit/compiler/test_symbol_binding.cpp; tools/check_value_result_variant_only.ps1
last_checked: 2026-05-24
applies_to: CodeGenerator responsibility boundaries and characterization guardrails
---

# CodeGenerator 职责地图

本文记录 `CodeGenerator` 的职责边界。`JumpPatcher`、`ScopeManager`、`ExpressionEmitter`、`StatementEmitter`、`NameBinder`、`CodegenOps` 和 `FunctionCompiler` 分别承载跳转、作用域、表达式、语句、名字解析、低层发射和函数级编译职责；`CodeGenerator` 负责编译总控与稳定公开入口。`ValueResult` 只保存 variant payload，普通代码必须通过 `visit()` 读取。

## 结构

`CodeGenerator` 仍是 AST 到 `Proto` 的编译总控类。物理文件按 binding / emitter / helper / state 拆分；facade 不再暴露 expression / statement / jump 的私有转发层。

| 职责域 | 入口 / 关键方法 | 代码落点 | 边界说明 |
|---|---|---|---|
| public facade | `generate()`、`tryGenerate()`、`generateUnchecked()` | `codegen.cpp` | 保留在 `CodeGenerator`，作为外部稳定 API |
| 指令写入 | `CodegenOps::codeABC()`、`codeABx()`、`codeAsBx()`；`patchArg*()`；`LineGuard` / `RegisterGuard` | `codegen_ops.hpp` + `BytecodeBuilder` | 集中低层发射、回填与 guard |
| 寄存器与常量 | `RegisterAllocator`、`BytecodeBuilder::add*Constant()` | emitters + `RegisterAllocator` + `BytecodeBuilder` | 不经由 `CodeGenerator` 私有包装 |
| 符号绑定 | `NameBinder::resolve()`、`symbolToValue()`、`symbolToLValue()`；`CodeGenerator` public wrapper | `name_binder.hpp/.cpp` + `codegen_binding.cpp` | 内部绑定集中于 `NameBinder`，公开 API 保持稳定 |
| 作用域 / 局部 / upvalue | `ScopeManager::addLocalVar()`、`resolveUpvalue()`、`enterBlock()`、`leaveBlock()`、`closeScopeUpvalues()` | `scope_manager.hpp/.cpp` | 管理作用域与捕获生命周期 |
| 跳转回填 | `JumpPatcher::emitJump()`、`patchList()`、`patchToHere()`、`flushPendingJumps()`、`getJump()`、`fixJump()` | `jump_patcher.hpp/.cpp` | 管理跳转链与偏移回填 |
| 条件 lowering | `ExpressionEmitter::emitCondResult()`、`emitComparisonJump()`、`materializeCondResult()` | `expression_emitter.hpp/.cpp` | 表达式分片负责 |
| 右值表达式 | `ExpressionEmitter::emitValue()`、`visitNode()`、`materializeValue()`、`valueToRK()` | `expression_emitter.hpp/.cpp` | 表达式分片负责 |
| 调用 / vararg / 多返回值 | `ExpressionEmitter::emitCallExpr()`、`emitVarargExpr()`、`setWantedResults()` | `expression_emitter.hpp/.cpp` | 表达式分片负责 |
| 左值与存储 | `ExpressionEmitter::emitLValue()`、`emitStore()` | `expression_emitter.hpp/.cpp` | 表达式分片负责 |
| 语句 lowering | `StatementEmitter::statement()`、各 `emitStmt()`、`block()` | `statement_emitter.hpp/.cpp` | 语句分片负责 |
| 函数编译 | `FunctionCompiler::compile()`、`emitClosureUpvalues()`、`attachDebugMetadata()` | `function_compiler.hpp/.cpp` + `codegen_stmt.cpp` forwarding | 形成函数级编译边界 |

`CodegenState` 是所有分片共享的状态容器，包含当前 `Proto`、`RuntimeServices`、`BytecodeBuilder`、`RegisterAllocator`、局部/upvalue/block 上下文、PC 和源码行号。`CodegenOps` 在不改变所有权的前提下集中低层写入操作，避免 expression / statement / facade 分片重复维护 pending-jump flush、指令参数回填和寄存器游标更新细节。`FunctionCompiler` 在 facade 内侧集中子函数编译生命周期，避免函数级编译继续滞留在 `CodeGenerator` 主体中。

## Characterization 护栏

`tests/unit/compiler/test_codegen_characterization.cpp` 中的 `Codegen Characterization` 测试套件不要求某条指令的绝对 PC 固定，只锁住重构必须保持的语义和字节码形状：

| 测试 | 锁定行为 | 保护边界 |
|---|---|---|
| `Statement Lowering Runtime Keeps Loop And Scope Semantics` | numeric for + break、while + break、do block、repeat body local 在 until 条件中可见 | `ScopeManager` / `StatementEmitter` |
| `Structured Statements Leave No Pending Jumps` | if/elseif/else、while、repeat、numeric for 的 `JMP` 全部回填；同时保留前跳/后跳和 `FORPREP` / `FORLOOP` | `JumpPatcher` / `StatementEmitter` |
| `Generic For Bytecode Shape Is Stable` | generic for 保留 `TFORLOOP` 和回到 loop body 的后跳，且没有 pending `JMP` | `JumpPatcher` / `ScopeManager` |
| Lua 5.1 parity characterization | 锁住 `T.listcode` 关心的字节码形状与可观察语义 | lowering 与优化边界 |
| `Jump Patcher` | 直接锁住 pending `jpc_` flush、旧式链表头尾方向、`PatchList` 显式回填、`TESTSET + NO_REG -> TEST` 和过长跳转错误 | `JumpPatcher` |
| `Scope Manager` | 直接锁住 local 生命周期、`RETURN` 后冗余 `CLOSE` 抑制、breaklist 延迟进入 pending `jpc_`、upvalue 去重与查找 | `ScopeManager` |
| `Expression Emitter` | 直接锁住 `ExpressionEmitter` facade 构造、`emitValue` / `emitCondResult` / `emitLValue` 返回契约、immediate literal lowering，以及 `materializeValue()` 按 payload 发射 | `ExpressionEmitter` / `ValueResult` |
| `Statement Emitter` | 直接锁住 `StatementEmitter` facade 构造、`statement()` / `block()` void 契约和空语句 lowering | `StatementEmitter` |

相邻护栏仍包括：

- `Codegen Conditions`：锁住条件表达式、短路逻辑、`TEST`/`JMP` 组合和嵌套 `not` 行为。
- `Codegen State`：锁住 `CodegenState` / `BytecodeBuilder` / `RegisterAllocator` 的基本状态契约。
- `Symbol Binding`：锁住 local / upvalue / global 的解析顺序和捕获路径；`symbolToValue()` 断言读取 payload visitor，不再把旧公开字段当作普通事实源。
- `Call Pipeline`、`ValueResult Pipeline`、`Codegen MultiRet`：锁住表达式和多返回值通道。

## 拆分约束

- 不要把 statement lowering 或 expression lowering 塞入 `JumpPatcher`。
- 不要把 expression 或 statement lowering 塞入 `ScopeManager`。
- 不要把 statement lowering 塞入 `ExpressionEmitter` 或 `CodeGenerator`。
- `ValueResult` 只保存 variant payload；`LegacyFields`、`legacyFields()`、旧字段 mirror 和兼容探针不得回流，`tools/check_value_result_variant_only.ps1` 负责执行该约束。
- 每次新增 `.cpp` / `.hpp` 优先使用 `tools\add_source.ps1` 同步 `CMakeLists.txt`、`.vcxproj` 和 `.vcxproj.filters`；新增生产源文件用 `-Target Core`，新增测试用 `-Target Test`。

## 推荐验证命令

```powershell
bin\lua_test.exe --filter "Codegen Characterization"
bin\lua_test.exe --filter "Codegen Conditions"
bin\lua_test.exe --filter "Symbol Binding"
bin\lua_test.exe --filter "Expression Emitter"
bin\lua_test.exe --filter "Statement Emitter"
bin\lua_test.exe --filter "Codegen Result Types"
bin\lua_test.exe --filter "ValueResult Pipeline"
bin\lua_test.exe --filter "Call Pipeline"
bin\lua_test.exe
powershell -NoProfile -ExecutionPolicy Bypass -File tools\check_value_result_variant_only.ps1
```
