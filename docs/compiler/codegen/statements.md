---
status: current
verified_against: docs/compiler/codegen-responsibility-map.md; src/compiler/codegen/statement_emitter.hpp; src/compiler/codegen/statement_emitter.cpp; src/compiler/codegen/codegen_stmt.cpp; src/compiler/codegen/function_compiler.hpp; src/compiler/codegen/function_compiler.cpp; src/compiler/codegen/scope_manager.hpp; src/compiler/codegen/scope_manager.cpp; src/compiler/codegen/expression_emitter.hpp; src/compiler/codegen/expression_emitter.cpp; src/compiler/codegen/codegen_ops.hpp; tests/unit/compiler/test_statement_emitter.cpp; tests/unit/compiler/test_codegen_characterization.cpp
last_checked: 2026-05-24
applies_to: current StatementEmitter lowering and FunctionCompiler lifecycle boundary
---

# Statements

当前语句 lowering 的物理落点是 `src/compiler/codegen/statement_emitter.hpp/.cpp`。`src/compiler/codegen/codegen_stmt.cpp` 现在只保留函数级 helper 的兼容转发，实际函数编译生命周期由 `FunctionCompiler` 承接。

| 文件 | 职责 |
|---|---|
| `statement_emitter.hpp/.cpp` | 语句 visitor、block lowering、控制流语句、赋值、local、return、loop、function statement |
| `codegen_stmt.cpp` | `CodeGenerator::compileFunction()`、`emitClosureUpvalues()`、`attachDebugMetadata()` 转发到 `FunctionCompiler` |
| `function_compiler.hpp/.cpp` | 子函数 `Proto` 创建、参数 local、upvalue metadata、closure upvalue 指令和 debug metadata |

## Statement Dispatcher

`StatementEmitter::statement(const Stmt& s)` 是语句入口。它使用 `LineGuard line(state_, s.getLine())` 设置当前行号，再通过 `StmtVisitor` 分派到 `visitNode()`，最终调用对应 `emitStmt()`。

`StatementEmitter` 自身不拥有状态。构造时从 `CodeGenerator& owner` 缓存：

- `CodegenState& state_`
- `CodegenOps& ops_`
- `JumpPatcher& jumps_`
- `ScopeManager& scopes_`
- `NameBinder& binder_`
- `ExpressionEmitter& expressions_`

文件顶部大量小 wrapper 只是把语句 lowering 需要的操作转发到对应边界，例如 `emitValue()` 转给 `ExpressionEmitter`，`enterBlock()` 转给 `ScopeManager`，`jump()` 转给 `JumpPatcher`。

## Block Management

`block(const Vec<StmtPtr>& stmts)` 保存进入 block 前的 active local 数量：

```cpp
i32 oldActiveVarCount = scopes_.activeLocalCount();
...
removeLocalVars(oldActiveVarCount);
```

它逐条调用 `statement(*stmt)`，最后移除该 block 新增的 local。这里的 block 是语句块词法 local 生命周期边界，不一定创建 `BlockInfo`。循环和 breakable block 由 `enterBlock(true)` / `leaveBlock()` 单独管理。

`removeLocalVars()` 通过 `ScopeManager` 完成：

- 对离开范围的 upvalue 发射必要的 `CLOSE`。
- 设置 local debug `endpc`。
- 将寄存器游标重置到当前 active local 数。

## Assignment

`emitStmt(const AssignStmt& s)` 处理多目标赋值：

- 前 `nexps - 1` 个表达式按单值处理：`emitValue()`、`forceSingleValue()`、`emitLValue()`、`emitStore()`。
- 如果最后一个表达式是 `CallExpr`，调用 `emitCallExpr()`，再用 `setWantedResults(callResult, wanted)` 让 `CALL` 返回足够数量，随后把连续结果寄存器逐个写入剩余目标。
- 如果最后一个表达式是 `VarargExpr`，调用 `emitVarargExpr()` 并 patch B 为 `wanted + 1`。
- 普通最后表达式按单值写入。
- 目标多于表达式时，剩余目标写入 `nil`。

这里的写入目标全部通过 LValue Channel 完成，因此 local/upvalue/global/table index 赋值共用 `ExpressionEmitter::emitStore()`。

## Local Declarations

`emitStmt(const LocalStmt& s)` 的关键点是“先预留 local slot，再激活 local”：

1. `base = state_.localScope.activeVarCount_`。
2. 创建 `RegisterGuard`，并把 free register 设到 `base`。
3. 对每个名字调用 `addLocalVar()`，这会记录 local debug 起始 PC 并 reserve 寄存器，但还不会增加 active local count。
4. 生成 initializer 到 `base + i`。
5. 最后一个 initializer 若是 call/vararg，按需要结果数 patch `CALL` / `VARARG`。
6. initializer 不足时发射 `LOADNIL`。
7. 恢复寄存器 guard，再调用 `adjustLocalVars(nvars)` 激活这些 local。

这样当前实现会先确定 debug 记录和寄存器 slot，再在 initializer lowering 完成后统一推进 active local count。

## Return

`emitStmt(const ReturnStmt& s)` 使用 active local 数作为 return values 的临时 base。

- 无返回值时发射 `RETURN 0, 1, 0`。
- 多个返回值时，前 `nret - 1` 个固定为单值并物化。
- 最后一个值若是 call，则调用 `emitCallExpr(..., base + nret - 1)` 并 `setOpenMultiRet()`。
- 当只有一个返回值且 call base 正好是目标 base 时，会把 `CALL` patch 成 `TAILCALL`，然后发射开放返回 `RETURN base, 0, 0`。
- 最后一个值若是 vararg，patch `VARARG` 为开放多返回，再发射 `RETURN base, 0, 0`。
- 普通返回值发射 `RETURN base, nret + 1, 0`。

`ScopeManager::closeScopeUpvalues()` 会避免在最后一条指令已经是 `RETURN` 时再追加冗余 `CLOSE`。

## If And Loops

`emitStmt(const IfStmt& s)` 使用 `CondResult::falseList` 和 `PatchList escapelist` 管理分支：

- 第一分支生成条件 false list，然后 lower body。
- 每个 `elseif` 前先追加一条逃逸 `JMP`，patch 上一个 false list 到当前条件。
- `else` 存在时同样先追加逃逸 jump，再 patch false list 到 else body。
- 最后把 escapelist patch 到 if 结束位置。

`emitStmt(const WhileStmt& s)`：

1. 保存条件开始 label。
2. 生成条件 false list。
3. `enterBlock(true)` 创建 breakable block。
4. lower body。
5. 发射回到条件开始的 jump。
6. `leaveBlock()` patch breaklist。
7. patch 条件 false list 到循环结束。

`emitStmt(const RepeatStmt& s)` 的特殊点是 repeat body 的 local 在 until 条件中可见：

1. 保存 body 起点 label。
2. `enterBlock(true)`。
3. 记录 `bodyActiveVarCount`。
4. 逐条 lower body。
5. lower until condition。
6. 在 patch false 回跳前 `removeLocalVars(bodyActiveVarCount)`。
7. `patchList(cond.falseList, repeat_init)`。
8. `leaveBlock()`。

`emitStmt(const BreakStmt&)` 查找最近的 breakable block，先 `closeScopeUpvalues(bl->activeVarCount)`，再把 `jump()` 追加到该 block 的 breaklist。

## Function Statements

`emitStmt(const FunctionStmt& s)` 同时处理 local function、global function 和 table field function。

公共流程：

1. 计算 `linedefined = s.line` 和 `lastlinedefined = getLastLineOfBlock(s.body)`。
2. 调用 `compileFunction(s.params, s.isVararg, s.body, linedefined, lastlinedefined, &childUpvalues)`。
3. 通过 `state_.bytecode.addSubProto(funcProto)` 将子 `Proto` 加入当前 `Proto`。
4. 发射 `CLOSURE`。
5. 调用 `emitClosureUpvalues(childUpvalues)` 写入 closure 捕获指令。

local function 会先 `addLocalVar(s.name)`，把 closure 放入该 local register，再 `adjustLocalVars(1)`。global function 会把名字加入字符串常量并发射 `SETGLOBAL`。带 `tablePath` 的 function 会逐段解析 table，再对最终 table 发射 `SETTABLE`。

## Numeric For

`emitStmt(const ForNumStmt& s)` 使用 Lua 5.1 固定寄存器布局：

```text
base + 0  (for index)
base + 1  (for limit)
base + 2  (for step)
base + 3  loop variable
```

流程是先把 init、limit、step 放入连续寄存器。缺省 step 时发射 `LOADK 1.0`。进入 breakable block 后添加 4 个 local，发射 `FORPREP`，lower body，再发射 `FORLOOP` 回到 body，最后 `fixjump(prep, loop)`。

`RegisterFrame` 用于把 free register 对齐到 loop base，并在 local 激活后设置 loop frame 顶部。

## Generic For

`emitStmt(const ForInStmt& s)` 使用 Lua 5.1 generic for 布局：

```text
base + 0  generator
base + 1  state
base + 2  control
base + 3  first loop variable
```

iterator 表达式至少需要一个。最后一个 iterator 如果是 call 或 vararg，可以返回多个值来填充 generator/state/control。进入 breakable block 后先添加隐藏 local，再添加用户 loop vars。

循环结构：

1. 先发射 `jmpToTfor = jump()` 跳到 `TFORLOOP`。
2. 记录 body label 并 lower body。
3. patch `jmpToTfor` 到 `TFORLOOP`。
4. 发射 `TFORLOOP base, 0, nvars`。
5. 发射回到 body 的 `JMP`。
6. `leaveBlock()` patch break。

## Function Compiler Lifecycle

`FunctionCompiler::compile()` 是子函数 `Proto` 的生命周期边界：

1. 创建 `CodeGenerator child(owner_.state_.services)`，并设置 `child.state_.parent = &owner_`。
2. 创建新 `Proto` 并注册到 GC。
3. 设置参数数、vararg 标记、`linedefined`、`lastlinedefined` 和 source。
4. 调用 `child.state_.resetForProto(*newProto, isVararg)`。
5. 将参数添加为 local，并 `adjustLocalVars(params.size())`。
6. 调用 `child.statements_.block(body)` lower 函数体。
7. 补发 `RETURN 0, 1, 0`。
8. 写入 upvalue 数量和 upvalue 名称。
9. 调用 `child.functions_.attachDebugMetadata()`。
10. 根据寄存器游标修正 `maxStackSize`。
11. 如传入 `outUpvalues`，返回子函数捕获列表。

`codegen_stmt.cpp` 中的 `CodeGenerator::compileFunction()` 只是调用 `functions_.compile(...)`。这保留了 facade 内部调用点，同时让函数生命周期逻辑集中在 `FunctionCompiler`。

## Debug Metadata

行号由 `LineGuard` 和 `BytecodeBuilder` 协作生成。语句和表达式入口设置 `state_.currentLine`，`CodegenOps::codeABC()` 等最终调用 `BytecodeBuilder::emitABC(line, ...)`，由 `Proto::addLineInfo(line)` 保存。

local debug metadata 由 `FunctionCompiler::attachDebugMetadata()` 生成。它遍历 `owner_.scopes_.localVars()`：

- 如果 `local.endpc >= 0`，使用记录的 end PC。
- 如果 local 仍未关闭，使用当前 instruction count 作为 end PC。
- 调用 `state_.bytecode.addLocalDebug(local.name, startpc, endpc, reg)` 写入 `Proto`。

函数定义行和结束行由 `FunctionCompiler::compile()` 写入 `Proto::setLineDefined()` 和 `setLastLineDefined()`。

## Collaboration Through State

`StatementEmitter` 是语句语义决策层。它通过：

- `ExpressionEmitter` 获取值、条件、调用和左值结果。
- `ScopeManager` 管理 local/block/upvalue close。
- `JumpPatcher` 管理 jump、patch list 和 breaklist。
- `CodegenOps` 发射低层指令并维护寄存器。
- `FunctionCompiler` 编译嵌套函数。

语句层不直接操作 `Proto` 的指令数组，也不重新实现表达式 materialization。跨模块协作都围绕同一个 `CodegenState` 展开。
