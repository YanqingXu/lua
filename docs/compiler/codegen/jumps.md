---
status: current
verified_against: docs/compiler/codegen-responsibility-map.md; src/compiler/codegen/jump_patcher.hpp; src/compiler/codegen/jump_patcher.cpp; src/compiler/codegen/codegen_types.hpp; src/compiler/codegen/codegen_ops.hpp; src/compiler/codegen/expression_emitter.hpp; src/compiler/codegen/expression_emitter.cpp; src/compiler/codegen/statement_emitter.hpp; src/compiler/codegen/statement_emitter.cpp; tests/unit/compiler/test_jump_patcher.cpp; tests/unit/compiler/test_codegen_conditions.cpp; tests/unit/compiler/test_codegen_characterization.cpp
last_checked: 2026-05-24
applies_to: current jump-list, comparison jump, and condition materialization pipeline
---

# Jumps

当前跳转回填的物理落点是 `src/compiler/codegen/jump_patcher.hpp/.cpp`。如果旧设计中提到 `codegen_jump.cpp`，它对应的职责现在由 `JumpPatcher` 和 `ExpressionEmitter` 的条件 lowering 方法共同承接。

跳转相关职责分为三层：

| 层级 | 文件 / 方法 | 职责 |
|---|---|---|
| Jump chain storage and patching | `JumpPatcher` | `JMP` 链表编码、pending jump、offset 回填 |
| Condition lowering | `ExpressionEmitter::emitCondResult()`、`emitCondResultTrue()`、`emitComparisonJump()` | 条件表达式生成 true/false patch lists |
| Statement control flow | `StatementEmitter::emitStmt(IfStmt/WhileStmt/RepeatStmt/BreakStmt/For*)` | 把条件 patch lists 接到语句结构目标 |

## Jump Chain Encoding

`NO_JUMP` 在 `codegen_types.hpp` 中定义为 `-1`，表示空跳转链。

未回填的 `JMP` 指令以单链表形式编码：当某条 `JMP` 还没有最终目的地时，它的 `sBx` 不是最终 offset，而是指向下一个未回填 jump PC 的相对 offset。`JumpPatcher::getJump(pc)` 读取该链表节点：

```cpp
i32 offset = GETARG_sBx(inst);
return (pc + 1) + offset;
```

`JumpPatcher::fixJump(pc, dest)` 将链表节点改写为最终跳转目标：

```cpp
i32 offset = dest - (pc + 1);
SETARG_sBx(jump, offset);
```

如果 offset 超过 `MAXARG_sBx` 范围，会抛出 `"control structure too long"`。

## Pending Jumps

`CodegenState::blockManager.jpc_` 保存“应该落到下一条普通指令”的 pending jump list。

`JumpPatcher::emitJump()` 的流程：

1. 取出 `state_.blockManager.jpc_`。
2. 清空 `jpc_`。
3. 直接通过 `state_.bytecode.emitAsBx(..., OpCode::JMP, 0, NO_JUMP)` 发射一个未回填 `JMP`。
4. 把原 pending list 串到新 jump 后面。
5. 返回新 jump PC。

`CodegenOps::codeABC()`、`codeABx()`、`codeAsBx()` 在发射任何普通指令前都会调用 `jumps_.flushPendingJumps()`。这保证 pending jump 在下一条真实指令出现时自动落到正确 PC。

## Patching APIs

`JumpPatcher` 提供两类 patch API：

| 方法 | 行为 |
|---|---|
| `patchList(i32 list, i32 target)` | 沿旧式链表读取每个 next，再逐个 `fixJump()` 到 target |
| `patchList(const PatchList& list, i32 target)` | 对 `PatchList::pcs` 中每个 PC 直接 `fixJump()` |
| `patchToHere(i32 list)` | 将旧式链表追加到 `jpc_`，等待下一条普通指令 flush |
| `patchToHere(const PatchList& list)` | 立即 patch 到当前 instruction count |
| `concatJumpList(i32& left, i32 right)` | 将两个旧式链表串接 |
| `collectPatchList(i32 list)` | 把旧式链表转换为 `PatchList` |

`PatchList` 是较新的显式列表结构，主要服务 `CondResult::trueList` 和 `falseList`。旧式 `i32` 链表仍用于 block breaklist 和部分 legacy-compatible jump 操作。

## Conditional Jump Primitive

`JumpPatcher::emitConditionalJump(OpCode op, i32 a, i32 b, i32 c)` 是底层条件跳转 helper：

1. 如果传入 `TESTSET` 且 `a == NO_REG`，会转换为 `TEST`，并把 `a = b`、`b = 0`。
2. flush pending jumps。
3. 发射条件测试指令。
4. 调用 `emitJump()` 发射随后的 pending `JMP`。
5. 返回 jump PC。

当前生产表达式比较主要通过 `ExpressionEmitter::emitComparisonJump()` 手动发射比较指令再调用 `jump()`，而 `emitConditionalJump()` 仍作为跳转边界的可测试 primitive 存在。

## Comparison Instructions

`ExpressionEmitter::emitComparisonJump()` 将 AST 比较 lowering 为 Lua 5.1 风格比较指令加跳转：

```text
EQ/LT/LE A, B, C
JMP <pending>
```

其中 A 参数表示比较结果应当与哪个 boolean 匹配才跳过下一条指令。当前实现把 `jumpOnTrue` 映射为 `cond`：

| AST op | opcode | cond 处理 | 操作数 |
|---|---|---|---|
| `==` | `EQ` | `jumpOnTrue ? 1 : 0` | 原顺序 |
| `~=` | `EQ` | `jumpOnTrue ? 0 : 1` | 原顺序 |
| `<` | `LT` | `jumpOnTrue ? 1 : 0` | 原顺序 |
| `<=` | `LE` | `jumpOnTrue ? 1 : 0` | 原顺序 |
| `>` | `LT` | `jumpOnTrue ? 1 : 0` | 左右交换 |
| `>=` | `LE` | `jumpOnTrue ? 1 : 0` | 左右交换 |

左右操作数先经过 `emitValue()`，再由 `valueToRK()` 尽量编码为 RK operand。比较指令之后的 `jump()` 返回未回填 `JMP`，放入 `PatchList`。

## Condition Channels

`CondResult` 保存两条 patch list：

```cpp
struct CondResult {
    PatchList trueList;
    PatchList falseList;
    bool knownConstant = false;
    bool constantValue = false;
};
```

当前核心使用的是 `trueList` 和 `falseList`。`knownConstant` 和 `constantValue` 字段保留在类型中，但主要条件 lowering 路径通过 immediate truthiness 和 patch lists 表达结果。

`emitCondResult(e)` 生成 false-channel：

- 条件为假时跳转到 `falseList`。
- 条件为真时自然 fallthrough。

`emitCondResultTrue(e)` 生成 true-channel：

- 条件为真时跳转到 `trueList`。
- 条件为假时自然 fallthrough。

这两个入口使 `and`、`or`、`not` 可以按目标方向生成更少的中间 boolean。

## Short-Circuit Lowering

条件通道中的短路规则：

- `a and b`：先生成左 false list，再生成右 false list，最终合并两者。只要左假或右假，整个表达式假。
- `a or b`：先生成左 true list，再生成右 false list，随后把左 true list patch 到右侧之后。左真时跳过右侧，左假时 fallthrough 到右侧继续判断。
- `not x`：false-channel 中使用 `emitCondResultTrue(x)`，true-channel 中使用 `emitCondResult(x)`，本质是交换列表方向。

普通 runtime 值会生成 `TEST` 加 `JMP`。例如 false-channel 中：

```text
TEST reg, 0, 0
JMP <falseList>
```

其中 truthy 会跳过 `JMP` 并 fallthrough，falsy 会执行 `JMP` 进入 false list。

## Materialization

条件结果需要作为普通值使用时，由 `ExpressionEmitter::materializeCondResult()` 发射 `LOADBOOL` 序列。

默认 `fallthroughOnTrue == false` 时：

```text
LOADBOOL reg, 0, 1
<patch trueList here>
LOADBOOL reg, 1, 0
```

含义是先假设结果为 false，并跳过下一条；true list 被 patch 到第二条 `LOADBOOL`，从而把结果改为 true。

`fallthroughOnTrue == true` 时反过来，先生成 true 值，再把 false list patch 到 false 赋值处。

`ValueResult::PendingJump` 的 materialization 也在 `ExpressionEmitter::materializeValue()` 中处理：先发射 false 值和 skip，再把 pending jump patch 到 true label，最后发射 true 值。

## Statement Integration

语句层把 patch lists 接到结构化控制流：

- `IfStmt` 用每个分支条件的 `falseList` 跳到下一个分支或 else，用 `escapelist` 跳出整个 if。
- `WhileStmt` 记录条件 label，body 结束后跳回条件 label，条件 `falseList` patch 到循环结束。
- `RepeatStmt` 先 lower body，再 lower condition，条件 `falseList` patch 回 body 起点。
- `BreakStmt` 把 `jump()` 追加到当前 breakable block 的 `breaklist`，`ScopeManager::leaveBlock()` patch 到 block 之后。
- `ForNumStmt` 使用 `FORPREP` / `FORLOOP` 的 sBx offset，并用 `fixjump(prep, loop)` 回填准备跳转。
- `ForInStmt` 先跳到 `TFORLOOP`，body 后 patch 到 `TFORLOOP`，再发射回 body 的 `JMP`。

## Collaboration Through State

`JumpPatcher` 直接共享 `CodegenState`：

- 通过 `state_.bytecode` 读取、替换和发射 `JMP` 指令。
- 通过 `state_.blockManager.jpc_` 保存 pending jump list。
- 通过 `state_.pc` 在 `patchToHere()` / `syncPc()` 中同步当前 PC。

它不决定语句或表达式语义。表达式层决定何时需要 true/false list，语句层决定这些 list 的结构化目标，`CodegenOps` 决定普通指令发射前自动 flush pending jumps。
