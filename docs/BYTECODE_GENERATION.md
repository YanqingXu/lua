---
status: current
verified_against: docs/PROJECT_STATUS.md; src/compiler/codegen.hpp; src/compiler/codegen.cpp; src/compiler/codegen_types.hpp; src/compiler/codegen_context.hpp; src/compiler/register_allocator.hpp
last_checked: 2026-05-18
applies_to: current AST-to-Proto bytecode generator
---

# Lua 字节码生成设计说明

本文描述当前产品代码中的字节码生成主线。旧版表达式描述器状态机已经完成迁移，相关历史说明已移动到 `docs/history/exprdesc.md`。

## 1. 当前主线

当前编译链可以概括为：

```text
source
  -> Lexer / Parser
  -> AST
  -> SymbolRef
  -> ValueResult / CondResult / LValueRef / CallResultInfo
  -> Proto
  -> VM execution
```

核心文件：

| 文件 | 当前职责 |
|---|---|
| `src/compiler/parser.hpp/.cpp` | 将 token 流解析为 AST |
| `src/compiler/codegen.hpp/.cpp` | 编译总控，遍历 AST 并生成 `Proto` |
| `src/compiler/codegen_types.hpp` | 编译管线结果类型：`SymbolRef`、`ValueResult`、`CondResult`、`LValueRef`、`CallResultInfo` |
| `src/compiler/codegen_context.hpp` | 局部变量、upvalue、block 与跳转上下文 |
| `src/compiler/register_allocator.hpp` | 临时寄存器分配、回收与 `maxStackSize` 维护 |
| `src/compiler/opcode.hpp/.cpp` | Lua 5.1 风格 VM 指令编码 |
| `src/core/function.hpp/.cpp` | `Proto`、`Function`、closure 与 upvalue 容器 |

## 2. 生成结果：Proto

`Proto` 是字节码生成的最终产物。它承载：

- 指令序列。
- 常量表。
- 子函数原型。
- upvalue 名称与捕获信息。
- 局部变量调试信息。
- 源码行号信息。
- `maxStackSize`、参数个数、vararg 标记等执行元数据。

换句话说，`CodeGenerator::generate()` 的目标不是“直接执行 AST”，而是把 AST 规整成 VM 可执行的函数原型。

## 3. 当前结果类型

### SymbolRef

`SymbolRef` 是名字绑定结果，负责把 `NameExpr` 解析成三类位置：

- `Local`：当前函数栈帧中的局部寄存器。
- `Upvalue`：外层函数捕获的变量。
- `Global`：全局表中的名字，当前实现通过字符串常量索引记录。

典型路径：

```text
NameExpr("x")
  -> resolve("x")
  -> SymbolRef::Local / Upvalue / Global
```

这样读路径和写路径可以共享同一套绑定逻辑，再分别转成 `ValueResult` 或 `LValueRef`。

### ValueResult

`ValueResult` 描述“右值”。它的目标是延迟决定值是否必须落到寄存器中：

- 字面量可以先保留为 immediate 或常量表索引。
- 已在寄存器中的值可直接复用。
- 全局读取、表索引、函数调用等可以记录为待物化结果。
- 多返回值通过独立标记表达，不再和普通单值混在一起。

常见转换：

```text
emitValue(expr)
  -> ValueResult
  -> valueToRK() / valueToAnyReg() / materializeValue()
```

### CondResult

`CondResult` 描述条件表达式，核心是两条跳转链：

- `trueList`：条件为真时需要回填的跳转。
- `falseList`：条件为假时需要回填的跳转。

它用于 `if`、`while`、`repeat-until`、逻辑短路和比较表达式布尔物化。这样条件生成可以保留控制流形态，而不是过早把所有条件都变成布尔常量。

### LValueRef

`LValueRef` 描述“可写位置”：

- 局部变量槽位。
- upvalue 槽位。
- 全局变量名。
- 表索引位置。

赋值流程应先解析左值位置，再把右值物化或展开到目标位置：

```text
emitLValue(target)
emitValue(value)
emitStore(target, value)
```

### CallResultInfo

`CallResultInfo` 描述函数调用和 vararg 的结果位置：

- 调用基址 `baseReg`。
- 发出调用指令的位置 `instructionPc`。
- 是否处于开放多返回值模式。

它用于保持 Lua 的多返回值规则：普通表达式收敛为单值，列表最后一个调用/vararg 可以展开，括号会强制单值收敛。

## 4. 寄存器模型

当前 VM 是寄存器机。生成器必须维护几个约定：

- 活跃局部变量占据低位寄存器。
- 临时表达式从 `RegisterAllocator::current()` 开始申请。
- 语句结束后通常把空闲寄存器恢复到活跃局部变量之后。
- 调用表达式要求函数、self、参数和返回值区域在连续寄存器中。
- `Proto::maxStackSize` 必须覆盖当前函数执行期间可能用到的最高寄存器。

建议从 `RegisterAllocator` 的方法读起：`alloc()`、`freeReg()`、`resetToLocals()`、`restore()`、`ensureAtLeast()`。

## 5. 名字、值、条件、左值的分层

当前实现最重要的可读性边界是“同一个 AST 表达式在不同语境下会走不同通道”：

| 语境 | 入口 | 结果类型 |
|---|---|---|
| 读取表达式值 | `emitValue()` | `ValueResult` |
| 条件判断 | `emitCondResult()` / `emitCondResultTrue()` | `CondResult` |
| 赋值目标 | `emitLValue()` | `LValueRef` |
| 函数调用/vararg | `emitCallExpr()` / `emitVarargExpr()` | `CallResultInfo` |
| 名字解析 | `resolve()` | `SymbolRef` |

这也是后续拆分 `CodeGenerator` 的自然边界：binder、value/condition/lvalue lowering、call lowering、bytecode builder 可以逐步从当前总控类中提取。

## 6. 语句生成概览

- `LocalStmt`：先分配局部槽位，再生成初始化表达式；最后补 nil 或展开多返回值。
- `AssignStmt`：先收集所有左值位置，再按 Lua 多赋值规则生成右值并存储。
- `ReturnStmt`：根据最后一个表达式是否可展开决定返回值数量；单值 `return f()` 可生成 `TAILCALL`。
- `IfStmt` / `WhileStmt` / `RepeatStmt`：通过 `CondResult` 和 patch list 回填跳转。
- `ForNumStmt` / `ForInStmt`：按 Lua 5.1 寄存器布局发出循环控制指令。
- `FunctionStmt`：编译子 `Proto`，发出 `CLOSURE`，再发出 upvalue 捕获伪指令。

## 7. 后续拆分方向

当前 `CodeGenerator` 仍是编译总控类。建议后续按以下顺序拆分，避免一次性重写：

1. `NameBinder`：拥有 `resolve()`、`symbolToValue()`、`symbolToLValue()`。
2. `BytecodeBuilder`：拥有 `codeABC()`、`codeABx()`、`codeAsBx()`、常量表、patch list 和行号记录。
3. `ExpressionLowerer`：拥有 `emitValue()`、`emitCondResult()`、`emitLValue()`、调用与 vararg lowering。
4. `StatementLowerer`：拥有语句、block、循环、return、break 生成。
5. `FunctionCompiler`：拥有子函数编译、closure 和 upvalue 捕获装配。

拆分时每一步都应保持 `Proto` 字节码输出不变，并优先复用现有 `test_symbol_binding`、`test_value_pipeline`、`test_codegen_conditions`、`test_lvalue_pipeline`、`test_call_pipeline` 和 `test_codegen_multret`。

## 8. 阅读顺序

1. `src/compiler/codegen_types.hpp`：先看五个结果类型。
2. `src/compiler/register_allocator.hpp`：理解寄存器分配。
3. `src/compiler/codegen_context.hpp`：理解局部变量、upvalue 与 block。
4. `src/compiler/codegen.hpp`：看当前总控 API。
5. `src/compiler/codegen.cpp`：按 `resolve -> emitValue/emitCond/emitLValue -> emitStmt -> compileFunction` 的顺序读。
6. `tests/unit/compiler/test_*pipeline.cpp`：把测试当作可执行示例。
