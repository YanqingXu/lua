---
status: current
verified_against: docs/compiler/codegen-responsibility-map.md; src/compiler/codegen/codegen_state.hpp; src/compiler/codegen/codegen_context.hpp; src/compiler/codegen/codegen_ops.hpp; src/compiler/codegen/bytecode_builder.hpp; src/compiler/register_allocator.hpp; src/compiler/codegen/jump_patcher.hpp; src/compiler/codegen/scope_manager.hpp; src/compiler/codegen/expression_emitter.cpp; src/compiler/codegen/statement_emitter.cpp; docs/compiler/register-allocation.md
last_checked: 2026-05-24
applies_to: current shared codegen state, low-level emission helpers, and Proto write boundary
---

# Infrastructure

codegen infrastructure 层负责保存共享生成状态、封装低层指令写入、维护寄存器游标，并为各 emitter 提供一致的 guard 和 patch helper。它不决定 AST 语义，而是让 facade、expression、statement、binding、jump 和 function compiler 在同一套状态上协作。

核心文件：

| 文件 | 职责 |
|---|---|
| `src/compiler/codegen/codegen_state.hpp` | 当前函数生成状态聚合体 |
| `src/compiler/codegen/codegen_context.hpp` | local、upvalue、block、breaklist 等上下文结构 |
| `src/compiler/codegen/codegen_ops.hpp` | 低层发射、指令参数 patch、寄存器 helper、guard |
| `src/compiler/codegen/bytecode_builder.hpp` | 当前 `Proto` 的窄写入边界 |
| `src/compiler/register_allocator.hpp` | free register、临时寄存器和 `maxStackSize` 维护 |

## CodegenState

`CodegenState` 是所有 implementation slices 共享的 mutable state：

```cpp
struct CodegenState {
    RuntimeServices services;
    StringPool* pool = nullptr;
    CodeGenerator* parent = nullptr;
    Proto* proto = nullptr;
    i32 pc = 0;
    i32 currentLine = 0;

    RegisterAllocator registers;
    LocalVarScope localScope;
    BlockManager blockManager;
    UpvalueContext upvalueContext;
    BytecodeBuilder bytecode;
};
```

字段含义：

| 字段 | 作用 |
|---|---|
| `services` | 当前编译使用的运行时服务集合，包括 GC 和 string pool |
| `pool` | 字符串常量和 debug 名称 intern 的目标 `StringPool` |
| `parent` | 子函数编译时指向外层 `CodeGenerator`，用于 upvalue 解析 |
| `proto` | 当前正在生成的 `Proto` |
| `pc` | 兼容性 PC 字段，jump patching 中会同步到 instruction count |
| `currentLine` | 当前发射指令使用的源码行号 |
| `registers` | 当前函数的寄存器分配器 |
| `localScope` | 当前函数 local 列表和 active local 数 |
| `blockManager` | 当前 block 栈、breaklist 和 pending jump |
| `upvalueContext` | 当前函数捕获的 upvalue 列表 |
| `bytecode` | 对当前 `Proto` 的指令、常量、debug metadata 写入边界 |

`resetForProto(Proto& nextProto, bool isVararg, StrView sourceName = {})` 负责切换到一个新 `Proto`：

1. 绑定 `bytecode` 到 `nextProto` 和 `pool`。
2. 绑定 `registers` 到当前 `Proto`。
3. 设置 `maxStackSize` 初值为 `2`。
4. 设置 vararg 标记和可选 source。
5. 重置 register cursor、local scope、block manager、upvalue context、PC 和当前行号。

顶层 `CodeGenerator::generateUnchecked()` 和子函数 `FunctionCompiler::compile()` 都通过这个方法初始化函数级生成状态。

## CodegenContext Structures

`codegen_context.hpp` 保存从 facade 中抽出的上下文结构。

`LocalVar` 记录 local debug 信息：

```cpp
struct LocalVar {
    Str name;
    i32 reg;
    i32 startpc;
    i32 endpc;
};
```

`LocalVarScope` 维护：

- `localVars_`：当前函数所有 local 的记录。
- `activeVarCount_`：当前词法范围中仍活跃的 local 数。
- `findLocal()`：从后向前查找 `endpc == -1` 的同名 local。
- `closeLocals(tolevel, currentPc)`：关闭离开作用域的 local，并写入 `endpc`。

`UpvalueCapture` 记录一个捕获：

```cpp
struct UpvalueCapture {
    Str name;
    bool inStack;
    i32 index;
};
```

`UpvalueContext` 持有 `upvalues_`，`add()` 会按名字去重。`inStack` 表示捕获的是父函数栈上的 local，`false` 表示捕获的是父函数已有 upvalue。

`BlockInfo` 和 `BlockManager` 管理结构化 block：

- `previous` 形成 block 栈。
- `breaklist` 保存该 block 的 break 跳转链。
- `activeVarCount` 记录进入 block 时的 active local 数。
- `isbreakable` 标记是否允许 `break`。
- `jpc_` 保存 pending jumps to next instruction。

当前生产路径主要通过 `ScopeManager` 调用这些结构，而不是让 statement/expression emitter 直接改写它们。

## BytecodeBuilder

`BytecodeBuilder` 是直接写 `Proto` 的窄边界。它只在 `bind(Proto&, StringPool&)` 后可用。

主要能力：

| 方法 | 作用 |
|---|---|
| `emitABC(line, op, a, b, c)` | 生成 ABC 指令并写入 line info |
| `emitABx(line, op, a, bx)` | 生成 ABx 指令并写入 line info |
| `emitAsBx(line, op, a, sbx)` | 生成 AsBx 指令并写入 line info |
| `instruction(pc)` | 读取指定 PC 的指令 |
| `replaceInstruction(pc, inst)` | 替换指定 PC 的指令 |
| `addNumberConstant()` / `addStringConstant()` / `addBoolConstant()` / `addNilConstant()` | 写入常量表 |
| `addSubProto(Proto*)` | 写入子函数原型 |
| `setSource(StrView)` | 设置 `Proto` source |
| `addLocalDebug(name, startpc, endpc, reg)` | 写入 local debug metadata |

`BytecodeBuilder` 不 flush pending jumps，不维护寄存器，也不决定 opcode 选择。它只负责把调用方给出的指令和 metadata 安全写入当前 `Proto`。

## CodegenOps

`CodegenOps` 是 emitter 共享的低层操作 facade。它持有 `CodegenState&` 和 `JumpPatcher&`。

指令发射方法：

```cpp
i32 codeABC(OpCode op, i32 a, i32 b, i32 c);
i32 codeABx(OpCode op, i32 a, i32 bx);
i32 codeAsBx(OpCode op, i32 a, i32 sbx);
```

这三个方法都会先 `jumps_.flushPendingJumps()`，再调用 `state_.bytecode.emit*()`。因此普通指令发射统一承担 pending jump 落点修复。

寄存器 helper：

| 方法 | 作用 |
|---|---|
| `allocReg()` | 分配一个临时寄存器 |
| `currentReg()` | 读取当前 free register |
| `freeReg(reg, activeLocalCount)` | 释放最近的临时寄存器 |
| `checkStack(n)` | 检查并更新 `maxStackSize` |
| `setFreeReg(reg)` / `setFreeRegAndCheck(reg)` | 设置 free register |
| `reserveRegs(count)` / `reserveRegsAndCheck(count)` | 预留连续寄存器 |
| `ensureRegAtLeast(reg)` | 保证 cursor 至少到指定位置 |
| `resetToLocals(activeLocalCount)` | 将 cursor 重置到 active locals 后 |

常量和指令 patch helper：

- `numberConstant()` / `stringConstant()` 转发到 `BytecodeBuilder`。
- `instruction()` / `replaceInstruction()` 读取和替换指令。
- `patchArgA()`、`patchArgB()`、`patchArgC()` 改写单个参数。
- `patchArgsAB()`、`patchArgsBC()` 改写参数组合。
- `patchToABC()` 用完整 ABC 指令替换原指令，例如 return path 中把 `CALL` 改为 `TAILCALL`。

## Guards And Frames

`LineGuard` 暂存 `state.currentLine`，构造时在传入行号大于 0 时设置当前行，析构时恢复。语句和表达式入口都用它确保后续发射指令带上正确源码行号。

`RegisterGuard` 保存进入某段 lowering 前的 free register。析构时如果仍 active，会调用 `state.registers.restore(savedFreeReg_)`。也可以通过 `restoreNow()` 提前恢复，或 `dismiss()` 放弃自动恢复。

`RegisterFrame` 用于固定格式寄存器区域，例如 for-loop 和 call frame：

- 构造时把 free register 设为 `base`。
- `at(offset)` 计算 `base + offset`。
- `setTop(offset)` 将 free register 设为 `base + offset` 并检查栈。
- `setTopUnchecked(offset)` 只设置 cursor，不检查。

这些 helper 让 statement/expression lowering 可以表达“这个结构需要一段连续寄存器”，同时把 cursor 操作集中到 `CodegenOps`。

## Register Allocation Model

寄存器布局遵循 `docs/compiler/register-allocation.md` 中的当前模型：

```text
R(0) ... R(activeVarCount-1)     active locals
R(activeVarCount) ... R(free-1)  temporaries, call frames, table fields
R(free) ...                      available
```

`ScopeManager::addLocalVar()` 使用 `state_.registers.current()` 作为 local slot，并 reserve 一个寄存器。`ScopeManager::adjustLocalVars()` 增加 `activeVarCount_` 后将 free register 重置到 active locals 后方。

表达式临时值通过 `ExpressionEmitter::valueToAnyReg()`、`valueToNextReg()` 和 `RegisterGuard` 管理。语句边界通常通过 `ops_.resetToLocals(scopes_.activeLocalCount())` 或 `ScopeManager::removeLocalVars()` 将临时寄存器释放回 local 区之后。

函数调用、表构造和 for-loop 需要连续寄存器，当前实现用 `RegisterFrame`、`reserveRegsAndCheck()`、`ensureRegAtLeast()` 等 helper 显式维护。

## Collaboration Pattern

基础设施层的协作路径可以概括为：

```text
CodeGenerator owns CodegenState
  -> helpers share references
  -> CodegenOps flushes jumps and emits through BytecodeBuilder
  -> BytecodeBuilder mutates Proto
  -> RegisterAllocator updates maxStackSize
  -> CodegenContext records locals, blocks, and upvalues
```

各上层模块的边界是：

- `NameBinder` 只解析名字和生成 `SymbolRef` 转换结果。
- `ExpressionEmitter` 只决定表达式如何变成值、条件、调用或左值。
- `StatementEmitter` 只决定语句结构如何连接表达式和跳转。
- `JumpPatcher` 只维护 jump 链和 offset patch。
- `ScopeManager` 只维护 local/block/upvalue lifecycle。
- `FunctionCompiler` 只维护函数级 `Proto` 生命周期和 metadata。

这些模块共享同一个 `CodegenState`，但通过窄 helper 访问底层资源，避免每个文件都直接修改 `Proto`、寄存器游标或 jump pending state。
