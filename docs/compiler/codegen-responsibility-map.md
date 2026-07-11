---
status: current
verified_against: src/compiler/codegen/codegen.hpp; src/compiler/codegen/codegen.cpp; src/compiler/codegen/codegen_binding.cpp; src/compiler/codegen/name_binder.hpp; src/compiler/codegen/name_binder.cpp; src/compiler/codegen/codegen_ops.hpp; src/compiler/codegen/function_compiler.hpp; src/compiler/codegen/function_compiler.cpp; src/compiler/codegen/expression_emitter.hpp; src/compiler/codegen/expression_emitter.cpp; src/compiler/codegen/statement_emitter.hpp; src/compiler/codegen/statement_emitter.cpp; src/compiler/codegen/codegen_stmt.cpp; src/compiler/codegen/codegen_types.hpp; src/compiler/codegen/codegen_state.hpp; src/compiler/codegen/jump_patcher.hpp; src/compiler/codegen/jump_patcher.cpp; src/compiler/codegen/scope_manager.hpp; src/compiler/codegen/scope_manager.cpp; src/compiler/codegen/bytecode_builder.hpp; tests/unit/compiler/test_codegen_result_types.cpp; tests/unit/compiler/test_codegen_characterization.cpp; tests/unit/compiler/test_jump_patcher.cpp; tests/unit/compiler/test_scope_manager.cpp; tests/unit/compiler/test_expression_emitter.cpp; tests/unit/compiler/test_statement_emitter.cpp; tests/unit/compiler/test_symbol_binding.cpp; tools/check_value_result_variant_only.ps1
last_checked: 2026-05-24
applies_to: CodeGenerator responsibilities after NameBinder extraction and ValueResult variant-only cleanup
---

# CodeGenerator 职责地图

本文记录 2026-05-24 当前 `CodeGenerator` 的真实职责边界。PR-41 补齐职责地图和 characterization 测试；PR-42 已抽出 `JumpPatcher`，PR-43 已抽出 `ScopeManager`，PR-44 已抽出 `ExpressionEmitter`，PR-45 已抽出 `StatementEmitter`。随后 facade cleanup 移除了 `CodeGenerator` 上的 expression / statement / jump 兼容包装方法，`NameBinder` 承接名字解析和 SymbolRef 转换，`CodegenOps` 收口重复的指令发射、回填和行号 / 寄存器 guard，`FunctionCompiler` 承接函数级 `Proto` 编译、closure upvalue 和 debug metadata；`CodeGenerator` 私有声明收敛为编译总控和少量兼容转发入口。`ValueResult` 已删除 legacy mirror，普通代码必须通过 `visit()` 读取 variant payload。

## 当前结构

`CodeGenerator` 仍是 AST 到 `Proto` 的编译总控类。物理文件按 binding / emitter / helper / state 拆分；facade 不再暴露 expression / statement / jump 的私有转发层。

| 职责域 | 入口 / 关键方法 | 当前落点 | 可拆分目标 |
|---|---|---|---|
| public facade | `generate()`、`tryGenerate()`、`generateUnchecked()` | `codegen.cpp` | 保留在 `CodeGenerator`，作为外部稳定 API |
| 指令写入 | `CodegenOps::codeABC()`、`codeABx()`、`codeAsBx()`；`patchArg*()`；`LineGuard` / `RegisterGuard` | `codegen_ops.hpp` + `BytecodeBuilder` | ✓ 已抽出低层发射/回填与 guard 边界 |
| 寄存器与常量 | `RegisterAllocator`、`BytecodeBuilder::add*Constant()` | emitters + `RegisterAllocator` + `BytecodeBuilder` | 已从 `CodeGenerator` 私有包装中移出 |
| 符号绑定 | `NameBinder::resolve()`、`symbolToValue()`、`symbolToLValue()`；`CodeGenerator` public wrapper | `name_binder.hpp/.cpp` + `codegen_binding.cpp` | ✓ 已抽出，public API 保持稳定 |
| 作用域 / 局部 / upvalue | `ScopeManager::addLocalVar()`、`resolveUpvalue()`、`enterBlock()`、`leaveBlock()`、`closeScopeUpvalues()` | `scope_manager.hpp/.cpp` | ✓ PR-43 已抽出 |
| 跳转回填 | `JumpPatcher::emitJump()`、`patchList()`、`patchToHere()`、`flushPendingJumps()`、`getJump()`、`fixJump()` | `jump_patcher.hpp/.cpp` | ✓ PR-42 已抽出 |
| 条件 lowering | `ExpressionEmitter::emitCondResult()`、`emitComparisonJump()`、`materializeCondResult()` | `expression_emitter.hpp/.cpp` | ✓ PR-44 已抽出 |
| 右值表达式 | `ExpressionEmitter::emitValue()`、`visitNode()`、`materializeValue()`、`valueToRK()` | `expression_emitter.hpp/.cpp` | ✓ PR-44 已抽出 |
| 调用 / vararg / 多返回值 | `ExpressionEmitter::emitCallExpr()`、`emitVarargExpr()`、`setWantedResults()` | `expression_emitter.hpp/.cpp` | ✓ PR-44 已抽出 |
| 左值与存储 | `ExpressionEmitter::emitLValue()`、`emitStore()` | `expression_emitter.hpp/.cpp` | ✓ PR-44 已抽出 |
| 语句 lowering | `StatementEmitter::statement()`、各 `emitStmt()`、`block()` | `statement_emitter.hpp/.cpp` | ✓ PR-45 已抽出 |
| 函数编译 | `FunctionCompiler::compile()`、`emitClosureUpvalues()`、`attachDebugMetadata()` | `function_compiler.hpp/.cpp` + `codegen_stmt.cpp` forwarding | ✓ 已抽出函数级编译边界 |

`CodegenState` 是所有分片共享的状态容器，包含当前 `Proto`、`RuntimeServices`、`BytecodeBuilder`、`RegisterAllocator`、局部/upvalue/block 上下文、PC 和源码行号。`CodegenOps` 在不改变所有权的前提下集中低层写入操作，避免 expression / statement / facade 分片重复维护 pending-jump flush、指令参数回填和寄存器游标更新细节。`FunctionCompiler` 在 facade 内侧集中子函数编译生命周期，避免函数级编译继续滞留在 `CodeGenerator` 主体中。

## 优先级

1. **PR-42：抽取 `JumpPatcher`** ✓ 已完成

   `src/compiler/codegen/jump_patcher.hpp/.cpp` 已承载旧式 jump-list、`NO_JUMP` 哨兵、pending `jpc_`、`PatchList` 回填和 `JMP` offset 写入。statement / expression lowering 现在直接依赖 `JumpPatcher`。

2. **PR-43：抽取 `ScopeManager`** ✓ 已完成

   `src/compiler/codegen/scope_manager.hpp/.cpp` 已承载局部变量、block、breaklist、upvalue close 和 repeat-until 作用域生命周期。statement lowering 和 upvalue 解析现在直接依赖 `ScopeManager`。

3. **PR-44：抽取 `ExpressionEmitter`** ✓ 已完成

   `src/compiler/codegen/expression_emitter.hpp/.cpp` 已承载 `ValueResult`、`CondResult`、`LValueRef`、`CallResultInfo` 表达式通道。`CodeGenerator` 不再保留表达式私有转发包装。

4. **PR-45：抽取 `StatementEmitter`** ✓ 已完成

   `src/compiler/codegen/statement_emitter.hpp/.cpp` 已承载 `statement()`、各 `emitStmt()` 和 `block()`。`CodeGenerator` 不再保留 statement / block 私有转发包装。

5. **PR-81 至 PR-83：抽取 `NameBinder` / `CodegenOps` / `FunctionCompiler`** ✓ 已完成

   `NameBinder` 承载名字解析和 SymbolRef 转换，`CodegenOps` 承载低层发射 / 回填 / guard / register frame helper，`FunctionCompiler` 承载函数体编译、closure upvalue 装配和 debug metadata。

## Characterization 护栏

PR-41 新增 `tests/unit/compiler/test_codegen_characterization.cpp`，测试套件为 `Codegen Characterization`。它不要求某条指令的绝对 PC 固定，只锁住后续拆分必须保持的语义和字节码形状：

| 测试 | 锁定行为 | 保护后续 PR |
|---|---|---|
| `Statement Lowering Runtime Keeps Loop And Scope Semantics` | numeric for + break、while + break、do block、repeat body local 在 until 条件中可见 | PR-43 / PR-45 |
| `Structured Statements Leave No Pending Jumps` | if/elseif/else、while、repeat、numeric for 的 `JMP` 全部回填；同时保留前跳/后跳和 `FORPREP` / `FORLOOP` | PR-42 / PR-45 |
| `Generic For Bytecode Shape Is Stable` | generic for 保留 `TFORLOOP` 和回到 loop body 的后跳，且没有 pending `JMP` | PR-42 / PR-43 |
| Lua 5.1 parity characterization | 锁住 `T.listcode` 关心的当前形状：显式 nil local 已合并为 `LOADNIL` 区间指令；顶层未捕获、后续不可读且不处于控制块中的局部变量 nil 赋值会作为 dead store 消除，future read、captured local 和 loop 路径保持可观察；数值字面量算术已常量折叠，连续局部变量 return 已复用原寄存器，单局部 `a = a` 已消除，local/table assignment 已复用稳定寄存器，concat chain 已合并并保护源寄存器，常量 `not not` 已规约为单条 `LOADBOOL`；动态 boolean/jump normalization 仍记录为后续差异 | L51-0405 / L51-0406 |
| `Jump Patcher` | 直接锁住 pending `jpc_` flush、旧式链表头尾方向、`PatchList` 显式回填、`TESTSET + NO_REG -> TEST` 和过长跳转错误 | PR-42 / PR-43 |
| `Scope Manager` | 直接锁住 local 生命周期、`RETURN` 后冗余 `CLOSE` 抑制、breaklist 延迟进入 pending `jpc_`、upvalue 去重与查找 | PR-43 / PR-45 |
| `Expression Emitter` | 直接锁住 `ExpressionEmitter` facade 构造、`emitValue` / `emitCondResult` / `emitLValue` 返回契约、immediate literal lowering，以及 `materializeValue()` 按 payload 发射 | PR-44 / PR-45 / PR-72 / PR-74 / PR-75 / PR-76 / PR-77 / PR-78 |
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
- PR-48 已完成 `ValueResult` 的兼容式 `std::variant` payload prototype；PR-72 已新增 `ValueResultVisitor` / `ValueResult::visit()` 并迁移 `ExpressionEmitter` 核心读路径；PR-74 已把普通断言读取面迁到 payload visitor；PR-75 到 PR-78 已完成兼容窗口、deprecation fence 和默认 private trial。当前已进入最终形态：`ValueResult` 只保存 variant payload，`LegacyFields`、`legacyFields()`、旧字段 mirror、probe 和 private trial 宏均已删除；`tools/check_value_result_variant_only.ps1` 会阻止这些兼容符号回流。
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
