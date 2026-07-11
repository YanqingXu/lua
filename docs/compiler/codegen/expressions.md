---
status: current
verified_against: docs/compiler/codegen-responsibility-map.md; src/compiler/codegen/expression_emitter.hpp; src/compiler/codegen/expression_emitter.cpp; src/compiler/codegen/codegen_types.hpp; src/compiler/codegen/name_binder.hpp; src/compiler/codegen/name_binder.cpp; src/compiler/codegen/jump_patcher.hpp; src/compiler/codegen/jump_patcher.cpp; src/compiler/codegen/codegen_ops.hpp; src/compiler/codegen/function_compiler.hpp; src/compiler/codegen/function_compiler.cpp; tests/unit/compiler/test_expression_emitter.cpp; tests/unit/compiler/test_value_pipeline.cpp; tests/unit/compiler/test_call_pipeline.cpp; tests/unit/compiler/test_lvalue_pipeline.cpp; tests/unit/compiler/test_codegen_conditions.cpp; src/compiler/; tests/unit/compiler/; tests/lua/bytecode/; tests/lua/control_flow/
last_checked: 2026-07-11
applies_to: current ExpressionEmitter value, condition, call, vararg, and lvalue lowering boundary
---

# Expressions

当前表达式 lowering 的物理落点是 `src/compiler/codegen/expression_emitter.hpp/.cpp`。如果在旧计划或历史说明中看到 `codegen_expr.cpp`，它对应的职责现在已经由 `ExpressionEmitter` 承接。

`ExpressionEmitter` 的职责是把 AST 表达式降为四条协作通道：

| 通道 | 类型 | 作用 |
|---|---|---|
| Value Channel | `ValueResult` | 表示表达式的右值，允许延迟物化、常量 RK 编码和多返回值 |
| Condition Channel | `CondResult` | 表示条件表达式的 true/false jump lists |
| Call Channel | `CallResultInfo` | 表示 `CALL` / `VARARG` 的基址、PC 和是否开放多返回 |
| LValue Channel | `LValueRef` | 表示赋值目标，供 `emitStore()` 生成写入指令 |

`ExpressionEmitter` 不拥有编译状态。构造时从 `CodeGenerator& owner` 缓存 `state_`、`ops_`、`jumps_`、`scopes_`、`binder_` 等引用。

## Value Channel

`emitValue(const Expr& e)` 是右值入口。它先用 `LineGuard` 将 `CodegenState::currentLine` 暂时设为表达式行号，再通过 `ExprVisitor` 分派到对应 `visitNode()`。

核心返回形态定义在 `ValueResult`：

| Variant | 用途 |
|---|---|
| `Immediate` | `nil`、boolean、number 等可延迟发射的立即值 |
| `ConstantRef` | 当前 `Proto` 常量表索引，如 string literal |
| `RegisterRef` | 已在寄存器中的值，可标记是否拥有临时寄存器 |
| `PendingLoad` | global / upvalue / table index 等尚未发射读取指令的位置 |
| `Relocatable` | 已发射但 A 参数待定的指令 |
| `MultiRet` | `CALL` / `VARARG` 的多返回值结果 |
| `PendingJump` | 待物化为 boolean 的跳转结果 |

典型 `visitNode()` 行为：

- `NilExpr`、`BoolExpr`、`NumberExpr` 返回 `Immediate`。
- `StringExpr` 调用 `stringConstant()` 后返回 `ConstantRef`。
- `NameExpr` 调用 `resolve()`，再通过 `symbolToValue()` 得到 local/upvalue/global 读取描述。
- `CallExpr` 调用 `emitCallExpr()`，再返回 `ValueResult::makeMultiRet(AccessKind::Call, ...)`。
- `VarargExpr` 调用 `emitVarargExpr()`，再返回 `ValueResult::makeMultiRet(AccessKind::Vararg, ...)`。
- `FunctionExpr` 调用 `compileFunction()` 创建子 `Proto`，在当前 `Proto` 中 `addSubProto()`，发射 `CLOSURE`，再调用 `emitClosureUpvalues()`。
- `ParenExpr` 会对内部结果调用 `forceSingleValue()`，保证括号表达式按单值语义处理。

## Materialization

`materializeValue(const ValueResult& val, i32 reg)` 将 `ValueResult` 强制写入指定寄存器：

| ValueResult | 发射或 patch 行为 |
|---|---|
| `Immediate::Nil` | `LOADNIL reg, reg, 0` |
| `Immediate::Boolean` | `LOADBOOL reg, bool, 0` |
| `Immediate::Number` | 先加入 number 常量，再 `LOADK` |
| `ConstantRef` | `LOADK reg, constIndex` |
| `RegisterRef` | 源寄存器不同于目标时发射 `MOVE` |
| `PendingLoad::Global` | `GETGLOBAL reg, constIndex` |
| `PendingLoad::Upvalue` | `GETUPVAL reg, aux` |
| `PendingLoad::Indexed` | `GETTABLE reg, tableReg, rkKey` |
| `Relocatable` | patch 已发射指令的 A 参数为 `reg` |
| `MultiRet::Call` | 将 `CALL` 的 C 改为 `2`，必要时把 call base 移到 `reg` |
| `MultiRet::Vararg` | patch `VARARG` 的 A 为 `reg`，B 为 `2` |
| `PendingJump` | 发射 `LOADBOOL` 序列，把跳转结果物化为 boolean |

辅助方法围绕这条 materialization 规则工作：

| 方法 | 作用 |
|---|---|
| `valueToRK()` | number / constant 能进入 RK 且索引不超过 `MAXINDEXRK` 时返回 `RKASK(k)`，否则落到寄存器 |
| `valueToAnyReg()` | 若值已经在寄存器中则复用，否则分配寄存器并 `materializeValue()` |
| `valueToNextReg()` | 将值放入当前 free register，并推进寄存器游标 |
| `forceSingleValue()` | 将 `CALL` / `VARARG` 多返回值固定为一个值 |

## Condition Channel

`emitCondResult(const Expr& e)` 生成“条件为假时跳转”的 `CondResult`。`emitCondResultTrue(const Expr& e)` 生成“条件为真时跳转”的对称结果。两者都通过 `PatchList` 保存尚未决定目标的 jump PC。

当前短路规则：

- `a and b` 的 false list 合并左、右两侧 false list。
- `a or b` 在 false-channel 中先生成左侧 true list，生成右侧后把左侧 true list patch 到当前位置，从而让左侧为真时跳过右侧。
- `not x` 交换 true/false 通道。
- 比较表达式委托给 `emitComparisonJump()`。
- 普通表达式先走 Value Channel，再按常量真值或运行时 `TEST` 生成跳转。

`constantTruthiness()` 对 `Immediate` 和 `ConstantRef` 做简单静态真值判断。`nil` 和 `false` 为 falsy，number 和常量引用为 truthy，其他结果按 runtime 处理。

## Comparison Jumps

`emitComparisonJump(const BinaryExpr& e, bool jumpOnTrue)` 将比较表达式 lowering 为：

```text
EQ/LT/LE cond, lhs, rhs
JMP <pending>
```

映射规则：

| AST op | 指令 | 特殊处理 |
|---|---|---|
| `Eq` | `EQ` | `cond` 由 `jumpOnTrue` 决定 |
| `Ne` | `EQ` | 反转 `cond` |
| `Lt` | `LT` | 直接比较 |
| `Le` | `LE` | 直接比较 |
| `Gt` | `LT` | 交换左右操作数 |
| `Ge` | `LE` | 交换左右操作数 |

左右操作数通过 `emitValue()` 生成，再用 `valueToRK()` 尽量进入 RK 编码。比较指令之后调用 `jump()` 生成未回填的 `JMP`，并把该 PC 放入 `PatchList` 返回。

`materializeCondResult(const CondResult& cond, i32 reg, bool fallthroughOnTrue)` 把条件通道变成 boolean 值。默认路径会发射：

```text
LOADBOOL reg, 0, 1
<patch trueList here>
LOADBOOL reg, 1, 0
```

`fallthroughOnTrue == true` 时逻辑反向，用 false list 作为需要 patch 的列表。

## Composite Expressions

`emitValueBinary()` 处理复合二元表达式：

- 比较表达式走 Condition Channel，再 `materializeCondResult()` 为 boolean 寄存器。
- `and` / `or` 在 Value Channel 中实现短路，先把左值物化到结果寄存器，发射 `TEST` 和待回填 `JMP`，必要时再物化右值覆盖同一结果寄存器。
- `Concat` 将值物化到连续 scratch 寄存器后发射 `CONCAT`；三段及以上链式 concat 会合并为单条 `CONCAT`，避免当前 VM 在归并时改写 active source registers。
- 算术 `Add/Sub/Mul/Div/Mod/Pow` 使用 `valueToRK()`，发射对应算术 opcode，并返回 A 参数待定的 `Relocatable`。

`emitValueUnary()` 处理一元表达式：

- `not` 走条件通道再物化为 boolean；`not not nil/false/true/number/string` 这类字面量双重否定会直接折叠为 boolean immediate。
- `-x` 对 immediate number 做常量折叠，否则发射 `UNM`。
- `#x` 发射 `LEN`。

`emitValueIndex()` 和 `emitValueMember()` 返回 `PendingLoad::Indexed`，把实际 `GETTABLE` 延迟到 materialization 阶段。

## Table Constructors

`emitValueTable(const TableExpr& table)` 的流程：

1. 先发射 `NEWTABLE`，A/B/C 暂时为 `0`。
2. 分配 table register，并 patch `NEWTABLE` 的 A 参数。
3. hash 字段在 `RegisterGuard` 中求 key/value，发射 `SETTABLE`，避免临时寄存器泄漏。
4. array 字段累积在 table register 后面的连续寄存器中，达到 `LFIELDS_PER_FLUSH` 时发射 `SETLIST`。
5. 最后一个 array 字段如果是 call 或 vararg，保留开放多返回，发射 `SETLIST tableReg, 0, c`。
6. 最后 patch `NEWTABLE` 的 B/C 为 array/hash 估计数量。

这里依赖 `RegisterGuard`、`CodegenOps::patchArgA()`、`patchArgsBC()` 和 `setOpenMultiRet()` 协作维护指令参数和寄存器游标。

## Calls And Vararg

`emitCallExpr(const CallExpr& e, i32 targetBase = -1)` 负责函数调用布局：

- 方法调用 `obj:method(args)` 要求 callee AST 是 `MemberExpr`，先求 `obj`，再发射 `SELF base, objReg, RK(method)`，隐式 `self` 占用一个参数位。
- 普通调用先 `emitValue()` callee，再用 `valueToAnyReg()` 得到函数寄存器。
- 如果调用需要落到指定 `targetBase`，会移动 callee/self 到该基址；否则必要时把调用帧移动到当前 free register。
- 实参从 `base + 1` 或 `base + 2` 开始。最后一个实参若是 call 或 vararg，可以保持开放多返回并将 `CALL` 的 B 设为 `0`。
- 当前默认 `CALL` 的 C 为 `2`，即期待一个返回值。外层通过 `setWantedResults()` 或 `setOpenMultiRet()` 改写。

`emitVarargExpr()` 只允许在 `state_.proto->isVararg()` 为真时使用。它先发射 `VARARG 0, 1, 0`，再返回 `CallResultInfo::Kind::Vararg`，后续按上下文 patch A/B。

## LValue Channel

`emitLValue(const Expr& e)` 只接受可赋值表达式：

| 表达式 | LValueRef |
|---|---|
| `NameExpr` | 通过 `resolve()` 和 `symbolToLValue()` 生成 local/upvalue/global |
| `IndexExpr` | 求 table register 和 key RK，生成 `Kind::Indexed` |
| `MemberExpr` | 求 table register，member 名字转字符串常量并进入 RK，生成 `Kind::Indexed` |

其他表达式会抛出 `"Expression is not a valid lvalue"`。

`emitStore(const LValueRef& target, const ValueResult& val)` 根据目标生成写入指令：

- local：直接把值物化到目标寄存器。
- upvalue：值固定为单值后发射 `SETUPVAL`。
- global：值固定为单值后发射 `SETGLOBAL`。
- indexed：值转换为 RK 后发射 `SETTABLE tableReg, key, value`。

## Collaboration Through State

`ExpressionEmitter` 通过 `CodegenState` 读取当前 `Proto`、行号和寄存器状态，通过 `CodegenOps` 发射和 patch 指令，通过 `JumpPatcher` 管理条件跳转，通过 `NameBinder` 解析名字，通过 `FunctionCompiler` facade helper 编译嵌套函数。

它的边界是“表达式语义决策”。低层写入不越过 `CodegenOps` / `BytecodeBuilder`，作用域生命周期不直接改写 `LocalVarScope`，名字解析不重新实现 local/upvalue/global 查找。
