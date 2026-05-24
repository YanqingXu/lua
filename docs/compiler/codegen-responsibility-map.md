---
status: current
verified_against: src/common/diagnostics.hpp; src/compiler/codegen/codegen.hpp; src/compiler/codegen/codegen.cpp; src/compiler/codegen/codegen_binding.cpp; src/compiler/codegen/codegen_expr.cpp; src/compiler/codegen/expression_emitter.hpp; src/compiler/codegen/expression_emitter.cpp; src/compiler/codegen/statement_emitter.hpp; src/compiler/codegen/statement_emitter.cpp; src/compiler/codegen/codegen_jump.cpp; src/compiler/codegen/codegen_stmt.cpp; src/compiler/codegen/codegen_types.hpp; src/compiler/codegen/codegen_state.hpp; src/compiler/codegen/jump_patcher.hpp; src/compiler/codegen/jump_patcher.cpp; src/compiler/codegen/scope_manager.hpp; src/compiler/codegen/scope_manager.cpp; src/compiler/codegen/bytecode_builder.hpp; tests/unit/compiler/test_codegen_result_types.cpp; tests/unit/compiler/test_codegen_characterization.cpp; tests/unit/compiler/test_jump_patcher.cpp; tests/unit/compiler/test_scope_manager.cpp; tests/unit/compiler/test_expression_emitter.cpp; tests/unit/compiler/test_statement_emitter.cpp; tests/unit/compiler/test_symbol_binding.cpp; tools/check_value_result_legacy_fields.ps1; tools/run_value_result_private_trial.ps1
last_checked: 2026-05-24
applies_to: CodeGenerator responsibilities after facade cleanup and ValueResult default-private legacy fields
---

# CodeGenerator 职责地图

本文记录 2026-05-24 当前 `CodeGenerator` 的真实职责边界。PR-41 补齐职责地图和 characterization 测试；PR-42 已抽出 `JumpPatcher`，PR-43 已抽出 `ScopeManager`，PR-44 已抽出 `ExpressionEmitter`，PR-45 已抽出 `StatementEmitter`。随后 facade cleanup 移除了 `CodeGenerator` 上的 expression / statement / jump 兼容包装方法，`CodeGenerator` 私有声明收敛为编译总控、函数编译和少量指令写入口。`ValueResult` 的 legacy mirror 现在默认 private，普通代码必须通过 `visit()` 或 `legacyFields()` 表达读取意图。

## 当前结构

`CodeGenerator` 仍是 AST 到 `Proto` 的编译总控类。物理文件按 binding / emitter / helper / state 拆分；facade 不再暴露 expression / statement / jump 的私有转发层。

| 职责域 | 入口 / 关键方法 | 当前落点 | 可拆分目标 |
|---|---|---|---|
| public facade | `generate()`、`tryGenerate()`、`generateUnchecked()` | `codegen.cpp` | 保留在 `CodeGenerator`，作为外部稳定 API |
| 指令写入 | `codeABC()`、`codeABx()`、`codeAsBx()`；emitters 内部直接经 `BytecodeBuilder` 发射 | `codegen.cpp` + emitters + `BytecodeBuilder` | 后续可抽 `InstructionEmitter` 消除重复 flush 模式 |
| 寄存器与常量 | `RegisterAllocator`、`BytecodeBuilder::add*Constant()` | emitters + `RegisterAllocator` + `BytecodeBuilder` | 已从 `CodeGenerator` 私有包装中移出 |
| 符号绑定 | `resolve()`、`symbolToValue()`、`symbolToLValue()` | `codegen_binding.cpp` | 后续可独立为 `NameBinder` |
| 作用域 / 局部 / upvalue | `ScopeManager::addLocalVar()`、`resolveUpvalue()`、`enterBlock()`、`leaveBlock()`、`closeScopeUpvalues()` | `scope_manager.hpp/.cpp` | ✓ PR-43 已抽出 |
| 跳转回填 | `JumpPatcher::emitJump()`、`patchList()`、`patchToHere()`、`flushPendingJumps()`、`getJump()`、`fixJump()` | `jump_patcher.hpp/.cpp` | ✓ PR-42 已抽出 |
| 条件 lowering | `ExpressionEmitter::emitCondResult()`、`emitComparisonJump()`、`materializeCondResult()` | `expression_emitter.hpp/.cpp` | ✓ PR-44 已抽出 |
| 右值表达式 | `ExpressionEmitter::emitValue()`、`visitNode()`、`materializeValue()`、`valueToRK()` | `expression_emitter.hpp/.cpp` | ✓ PR-44 已抽出 |
| 调用 / vararg / 多返回值 | `ExpressionEmitter::emitCallExpr()`、`emitVarargExpr()`、`setWantedResults()` | `expression_emitter.hpp/.cpp` | ✓ PR-44 已抽出 |
| 左值与存储 | `ExpressionEmitter::emitLValue()`、`emitStore()` | `expression_emitter.hpp/.cpp` | ✓ PR-44 已抽出 |
| 语句 lowering | `StatementEmitter::statement()`、各 `emitStmt()`、`block()` | `statement_emitter.hpp/.cpp` | ✓ PR-45 已抽出 |
| 函数编译 | `compileFunction()`、`emitClosureUpvalues()`、`attachDebugMetadata()` | `codegen_stmt.cpp` + `codegen.cpp` | 拆分后仍由 facade 编排 |

`CodegenState` 是所有分片共享的状态容器，包含当前 `Proto`、`RuntimeServices`、`BytecodeBuilder`、`RegisterAllocator`、局部/upvalue/block 上下文、PC 和源码行号。后续拆分时应优先把行为从 `CodeGenerator` 移走，暂不急于移动 `CodegenState` 字段，避免同时改变数据所有权和控制流。

## 优先级

1. **PR-42：抽取 `JumpPatcher`** ✓ 已完成

   `src/compiler/codegen/jump_patcher.hpp/.cpp` 已承载旧式 jump-list、`NO_JUMP` 哨兵、pending `jpc_`、`PatchList` 回填和 `JMP` offset 写入。statement / expression lowering 现在直接依赖 `JumpPatcher`。

2. **PR-43：抽取 `ScopeManager`** ✓ 已完成

   `src/compiler/codegen/scope_manager.hpp/.cpp` 已承载局部变量、block、breaklist、upvalue close 和 repeat-until 作用域生命周期。statement lowering 和 upvalue 解析现在直接依赖 `ScopeManager`。

3. **PR-44：抽取 `ExpressionEmitter`** ✓ 已完成

   `src/compiler/codegen/expression_emitter.hpp/.cpp` 已承载 `ValueResult`、`CondResult`、`LValueRef`、`CallResultInfo` 表达式通道。`CodeGenerator` 不再保留表达式私有转发包装。

4. **PR-45：抽取 `StatementEmitter`** ✓ 已完成

   `src/compiler/codegen/statement_emitter.hpp/.cpp` 已承载 `statement()`、各 `emitStmt()` 和 `block()`。`CodeGenerator` 不再保留 statement / block 私有转发包装，函数体编译、closure upvalue 装配和 debug metadata 暂留 facade 编排。

## Characterization 护栏

PR-41 新增 `tests/unit/compiler/test_codegen_characterization.cpp`，测试套件为 `Codegen Characterization`。它不要求某条指令的绝对 PC 固定，只锁住后续拆分必须保持的语义和字节码形状：

| 测试 | 锁定行为 | 保护后续 PR |
|---|---|---|
| `Statement Lowering Runtime Keeps Loop And Scope Semantics` | numeric for + break、while + break、do block、repeat body local 在 until 条件中可见 | PR-43 / PR-45 |
| `Structured Statements Leave No Pending Jumps` | if/elseif/else、while、repeat、numeric for 的 `JMP` 全部回填；同时保留前跳/后跳和 `FORPREP` / `FORLOOP` | PR-42 / PR-45 |
| `Generic For Bytecode Shape Is Stable` | generic for 保留 `TFORLOOP` 和回到 loop body 的后跳，且没有 pending `JMP` | PR-42 / PR-43 |
| `Jump Patcher` | 直接锁住 pending `jpc_` flush、旧式链表头尾方向、`PatchList` 显式回填、`TESTSET + NO_REG -> TEST` 和过长跳转错误 | PR-42 / PR-43 |
| `Scope Manager` | 直接锁住 local 生命周期、`RETURN` 后冗余 `CLOSE` 抑制、breaklist 延迟进入 pending `jpc_`、upvalue 去重与查找 | PR-43 / PR-45 |
| `Expression Emitter` | 直接锁住 `ExpressionEmitter` facade 构造、`emitValue` / `emitCondResult` / `emitLValue` 返回契约、immediate literal lowering，以及 payload 与旧字段漂移时 `materializeValue()` 仍按 payload 发射 | PR-44 / PR-45 / PR-72 / PR-74 / PR-75 / PR-76 / PR-77 / PR-78 |
| `Statement Emitter` | 直接锁住 `StatementEmitter` facade 构造、`statement()` / `block()` void 契约和空语句 lowering | PR-45 |

相邻护栏仍包括：

- `Codegen Conditions`：锁住条件表达式、短路逻辑、`TEST`/`JMP` 组合和嵌套 `not` 行为。
- `Codegen State`：锁住 `CodegenState` / `BytecodeBuilder` / `RegisterAllocator` 的基本状态契约。
- `Symbol Binding`：锁住 local / upvalue / global 的解析顺序和捕获路径；`symbolToValue()` 断言读取 payload visitor，不再把旧公开字段当作普通事实源。
- `Call Pipeline`、`ValueResult Pipeline`、`Codegen MultiRet`：锁住表达式和多返回值通道。

## 拆分约束

- PR-42 已完成；后续不要把 statement lowering 或 expression lowering 重新塞回 `JumpPatcher`。
- PR-43 已完成；后续不要把 expression 或 statement lowering 塞进 `ScopeManager`。
- PR-44 已完成；后续不要把 statement lowering 塞进 `ExpressionEmitter`。
- PR-45 已完成；后续不要把 statement lowering 重新塞回 `CodeGenerator`。
- PR-48 已完成 `ValueResult` 的兼容式 `std::variant` payload prototype；PR-72 已新增 `ValueResultVisitor` / `ValueResult::visit()` 并迁移 `ExpressionEmitter` 核心读路径；PR-74 已把普通断言读取面迁到 payload visitor；PR-75 已新增 `legacyFields()` 快照桥；PR-76 已把旧字段标记为 `[[deprecated]]`；PR-77 已新增 `tools/check_value_result_legacy_fields.ps1`；PR-78 已新增 private trial。当前默认构建已启用 `LUA_VALUE_RESULT_PRIVATE_LEGACY_FIELDS`，旧字段直接读写只应出现在 legacy mirror 内部同步或 `detail::ValueResultLegacyMirrorProbe` 这类明确命名的兼容边界中，并用 `LUA_SUPPRESS_DEPRECATED_DECLARATIONS_*` 局部包裹；质量门会阻止新增普通直接访问。
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
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_value_result_private_trial.ps1
```
