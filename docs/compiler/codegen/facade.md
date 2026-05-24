---
status: current
verified_against: docs/compiler/codegen-responsibility-map.md; src/compiler/codegen/codegen.hpp; src/compiler/codegen/codegen.cpp; src/compiler/codegen/codegen_binding.cpp; src/compiler/codegen/codegen_stmt.cpp; src/compiler/codegen/codegen_state.hpp; src/compiler/codegen/codegen_ops.hpp; src/compiler/codegen/function_compiler.hpp; src/compiler/codegen/function_compiler.cpp; src/compiler/codegen/statement_emitter.hpp; src/compiler/codegen/statement_emitter.cpp
last_checked: 2026-05-24
applies_to: current CodeGenerator public facade and pipeline orchestration boundary
---

# Codegen Facade

`CodeGenerator` 是当前 AST 到 `Proto` 编译链的公共入口。按照 `docs/compiler/codegen-responsibility-map.md` 的职责边界，它不再直接承载表达式、语句、跳转、作用域或名字绑定的主要 lowering 逻辑，而是保留稳定 public API、共享状态所有权和管线编排职责。

当前 facade 的主文件是：

| 文件 | 职责 |
|---|---|
| `src/compiler/codegen/codegen.hpp` | 声明 `CodeGenerator` public API、私有编排入口和 facade 持有的 helper 成员 |
| `src/compiler/codegen/codegen.cpp` | 构造 helper 图、实现 `generate()` / `tryGenerate()` / `generateUnchecked()` / 指令发射转发 |
| `src/compiler/codegen/codegen_binding.cpp` | 保留 `resolve()`、`symbolToValue()`、`symbolToLValue()` 的 public wrapper |
| `src/compiler/codegen/codegen_stmt.cpp` | 保留函数级 helper 的兼容转发，实际落点是 `FunctionCompiler` |

## Public API

`CodeGenerator` 的外部稳定入口集中在 `codegen.hpp`：

```cpp
explicit CodeGenerator(StringPool* pool);
explicit CodeGenerator(RuntimeServices& services);
Proto* generate(const Chunk& chunk, StrView sourceName = {});
std::expected<Proto*, CodegenError> tryGenerate(const Chunk& chunk, StrView sourceName = {});
SymbolRef resolve(const Str& name);
ValueResult symbolToValue(const SymbolRef& sym);
LValueRef symbolToLValue(const SymbolRef& sym);
```

`CodeGenerator(StringPool*)` 使用 `RuntimeServices::fromSingletons()` 构造共享运行时服务，并要求传入的 `StringPool*` 非空。`CodeGenerator(RuntimeServices&)` 用显式服务集合构造，适合测试或子编译器复用当前运行时服务。

`generate()` 是旧的异常式入口。它调用 `tryGenerate()`，成功时返回 `Proto*`，失败时抛出 `CodegenError`。这让已有调用点可以继续按异常模型工作。

`tryGenerate()` 是当前推荐的显式错误返回入口，返回 `std::expected<Proto*, CodegenError>`。它捕获 `CodegenError`、`LuaError` 和一般 `std::exception`，调用 `discardCurrentProto()` 清理当前失败的 `Proto` 后返回 `std::unexpected`。遇到 `std::bad_alloc` 时同样先清理当前 `Proto`，再重新抛出。

`resolve()`、`symbolToValue()`、`symbolToLValue()` 保持在 public API 上，但实现只委托给 `NameBinder`。这组 wrapper 是 binding 边界对外的兼容层，不是 facade 自己重新实现名字查找。

## Pipeline

`generateUnchecked()` 是实际编译流程的内部入口：

1. 创建新的 `Proto`，并通过 `state_.services.gc.registerObject(state_.proto)` 交给 GC 管理。
2. 调用 `state_.resetForProto(*state_.proto, true, sourceName)` 绑定 `BytecodeBuilder`、`RegisterAllocator`，重置局部变量、block、upvalue、PC 和当前行号。
3. 调用 `statements_.block(chunk.statements)` 让 `StatementEmitter` lower 顶层语句块。
4. 补发兜底 `RETURN 0 1 0`，覆盖仍能从条件分支落出的路径。
5. 调用 `attachDebugMetadata()`，由 `FunctionCompiler` 将 local debug metadata 写入 `Proto`。
6. 返回当前 `state_.proto`。

顶层 `resetForProto(..., true, sourceName)` 会把 chunk 标记为 vararg。这是当前实现事实：顶层 chunk 可以使用 `...`，而子函数由 `FunctionCompiler::compile()` 根据函数 AST 的 `isVararg` 设置。

## Owned Helper Graph

`CodeGenerator` 持有所有 codegen helper，并通过构造顺序把同一个 `CodegenState` 传给它们：

```cpp
CodegenState state_;
JumpPatcher jumps_;
CodegenOps ops_;
ScopeManager scopes_;
NameBinder binder_;
ExpressionEmitter expressions_;
StatementEmitter statements_;
FunctionCompiler functions_;
```

这些 helper 不拥有状态；它们引用 facade 中的共享对象。当前构造关系是：

| 成员 | 构造依赖 | 作用 |
|---|---|---|
| `state_` | `RuntimeServices` / `StringPool` | 当前 `Proto`、寄存器、局部/upvalue/block 上下文、字节码写入器 |
| `jumps_` | `state_` | jump-list、pending jump 和 offset 回填 |
| `ops_` | `state_`, `jumps_` | 低层指令发射、参数 patch、寄存器 helper |
| `scopes_` | `state_`, `jumps_` | local、block、break、upvalue lifecycle |
| `binder_` | `state_`, `scopes_` | `Local -> Upvalue -> Global` 名字绑定 |
| `expressions_` | `*this` | 表达式、条件、调用、左值 lowering |
| `statements_` | `*this` | 语句和 block lowering |
| `functions_` | `*this` | 子函数 `Proto` 生命周期、closure upvalue、debug metadata |

`ExpressionEmitter` 和 `StatementEmitter` 接收 `CodeGenerator&`，再缓存 `state_`、`ops_`、`jumps_`、`scopes_`、`binder_` 等引用。这让它们可以协作，但所有权仍集中在 facade。

## Instruction Facade

`CodeGenerator` 仍保留私有 `codeABC()`、`codeABx()`、`codeAsBx()`。这些方法不直接写 `Proto`，而是转发给 `CodegenOps`：

```cpp
i32 CodeGenerator::codeABC(OpCode op, i32 a, i32 b, i32 c) {
    return ops_.codeABC(op, a, b, c);
}
```

这样 `FunctionCompiler` 等 friend helper 可以继续通过 facade 发射少量函数级指令，同时低层行为统一由 `CodegenOps` 处理。`CodegenOps::codeABC()` / `codeABx()` / `codeAsBx()` 会先调用 `JumpPatcher::flushPendingJumps()`，再由 `BytecodeBuilder` 写入带行号的指令。

## Error Cleanup

`discardCurrentProto()` 是 `tryGenerate()` 的失败清理路径。它会：

- 暂存失败的 `Proto*`。
- 将 `state_.proto` 置空。
- 重置 `state_.bytecode` 为未绑定的 `BytecodeBuilder`。
- 调用 `state_.registers.bind(nullptr)` 解除寄存器分配器对失败 `Proto` 的绑定。
- 从 GC 反注册失败 `Proto` 并 `delete`。

因此，入口失败不会把半生成的 `Proto` 留在当前 facade 状态中。

## Collaboration Through State

Facade 本身只做编排，实际数据通过 `CodegenState` 和 `CodegenContext` 中的子结构共享：

- `CodegenState::proto` 是当前写入目标。
- `CodegenState::bytecode` 是 `Proto` 的窄写入边界。
- `CodegenState::registers` 维护当前 free register 和 `maxStackSize`。
- `CodegenState::localScope`、`blockManager`、`upvalueContext` 来自 `codegen_context.hpp`，分别承载 local、block/jump pending 和 upvalue 捕获上下文。
- `CodegenState::parent` 连接子函数编译器和外层 `CodeGenerator`，供 upvalue 解析递归使用。

这也是当前拆分的关键约束：`CodeGenerator` 是 API 和所有权边界，不是所有 lowering 规则的堆放处。
