---
status: current
verified_against: docs/compiler/codegen-responsibility-map.md; src/compiler/codegen/codegen_binding.cpp; src/compiler/codegen/name_binder.hpp; src/compiler/codegen/name_binder.cpp; src/compiler/codegen/codegen_types.hpp; src/compiler/codegen/scope_manager.hpp; src/compiler/codegen/scope_manager.cpp; src/compiler/codegen/function_compiler.hpp; src/compiler/codegen/function_compiler.cpp; tests/unit/compiler/test_symbol_binding.cpp
last_checked: 2026-05-24
applies_to: current NameBinder symbol resolution and SymbolRef conversion pipeline
---

# Binding

名字绑定的当前边界是 `NameBinder`。它负责把 AST 中的名字解析为 `SymbolRef`，再把 `SymbolRef` 转换为表达式通道需要的 `ValueResult` 或赋值通道需要的 `LValueRef`。

当前文件分工：

| 文件 | 职责 |
|---|---|
| `src/compiler/codegen/name_binder.hpp` | 声明 `NameBinder`，持有 `CodegenState&` 和 `ScopeManager&` 引用 |
| `src/compiler/codegen/name_binder.cpp` | 实现 `resolve()`、`symbolToValue()`、`symbolToLValue()` |
| `src/compiler/codegen/codegen_binding.cpp` | `CodeGenerator` public wrapper，全部委托给 `binder_` |
| `src/compiler/codegen/codegen_types.hpp` | 定义 `SymbolRef`、`ValueResult`、`LValueRef` |
| `src/compiler/codegen/scope_manager.cpp` | 提供 local 查找和跨函数 upvalue 解析 |

## SymbolRef

`SymbolRef` 是名字解析结果：

```cpp
struct SymbolRef {
    enum class Kind { None, Local, Upvalue, Global };
    Kind kind = Kind::None;
    i32 index = -1;
    Str name;
};
```

`index` 的含义取决于 `kind`：

| Kind | index 含义 |
|---|---|
| `Local` | 当前函数栈帧中的局部寄存器槽位 |
| `Upvalue` | 当前 `Proto` 的 upvalue 索引 |
| `Global` | 当前 `Proto` 常量表中的字符串常量索引 |

`name` 保留原始名字，主要用于 global 场景和调试阅读。

## Resolve Order

`NameBinder::resolve(const Str& name)` 使用固定查找顺序：

```text
Local -> Upvalue -> Global
```

具体实现如下：

1. 调用 `scopes_.findLocalVar(name)`。如果返回非负寄存器号，生成 `SymbolRef::Kind::Local`。
2. 调用 `scopes_.resolveUpvalue(name)`。如果返回非负索引，生成 `SymbolRef::Kind::Upvalue`。
3. fallback 为 global。此时通过 `state_.bytecode.addStringConstant(name)` 将名字加入当前 `Proto` 常量表，生成 `SymbolRef::Kind::Global`。

这条顺序是 Lua 名字解析的核心约束：局部变量遮蔽外层变量，外层变量可被捕获为 upvalue，找不到时才按 global 处理。

## Local Resolution

local 查找由 `ScopeManager::findLocalVar()` 转到 `LocalVarScope::findLocal()`。`LocalVarScope` 从 `localVars_` 尾部向前扫描，只接受 `endpc == -1` 的仍活跃局部变量。

局部变量的寄存器在 `ScopeManager::addLocalVar()` 中确定：

```cpp
i32 reg = state_.registers.current();
state_.localScope.localVars_.emplace_back(name, reg, state_.bytecode.instructionCount());
state_.registers.reserve(1);
```

因此，`SymbolRef::Kind::Local` 的 `index` 可以直接作为读取或写入该 local 的寄存器号。

## Upvalue Resolution

upvalue 查找在 `ScopeManager::resolveUpvalue()` 中递归完成：

1. 如果 `state_.parent == nullptr`，说明当前函数没有外层 `CodeGenerator`，返回 `-1`。
2. 先在父编译器的 local 中查找名字。命中时调用 `addUpvalue(name, true, local)`，表示捕获父函数栈上的 local。
3. 如果父 local 未命中，递归调用 `state_.parent->scopes_.resolveUpvalue(name)`。命中时调用 `addUpvalue(name, false, parentUp)`，表示捕获父函数已有的 upvalue。
4. 全部失败时返回 `-1`。

`UpvalueContext::add()` 会先调用 `find(name)` 去重，因此同一个名字在同一子函数内只占一个 upvalue slot。

`FunctionCompiler::compile()` 创建子编译器时设置：

```cpp
CodeGenerator child(owner_.state_.services);
child.state_.parent = &owner_;
```

这条 parent 链就是 `resolveUpvalue()` 能跨函数向外查找的原因。子函数编译完成后，`FunctionCompiler::compile()` 会把 `child.scopes_.upvalues()` 写入子 `Proto` 的 upvalue 名称列表，并通过 `outUpvalues` 返回给外层 closure 发射路径。

## SymbolRef To ValueResult

`NameBinder::symbolToValue()` 定义读取路径：

| SymbolRef | ValueResult |
|---|---|
| `Local` | `ValueResult::makeRegister(sym.index, false, AccessKind::Local)` |
| `Upvalue` | `ValueResult::makePendingLoad(AccessKind::Upvalue, -1, -1, sym.index)` |
| `Global` | `ValueResult::makePendingLoad(AccessKind::Global, -1, sym.index, -1)` |
| `None` | 空 `ValueResult` |

local 读取已经在寄存器中，因此返回非 owning `RegisterRef`。upvalue 和 global 返回 `PendingLoad`，由 `ExpressionEmitter::materializeValue()` 在需要具体寄存器时分别发射 `GETUPVAL` 或 `GETGLOBAL`。

## SymbolRef To LValueRef

`NameBinder::symbolToLValue()` 定义写入路径：

| SymbolRef | LValueRef |
|---|---|
| `Local` | `LValueRef::Kind::Local`，`slot = sym.index` |
| `Upvalue` | `LValueRef::Kind::Upvalue`，`slot = sym.index` |
| `Global` | `LValueRef::Kind::Global`，`slot = sym.index` |
| `None` | `LValueRef::Kind::None` |

真正的写入指令由 `ExpressionEmitter::emitStore()` 生成：

- local 通过 `materializeValue(val, target.slot)` 写入寄存器。
- upvalue 发射 `SETUPVAL`。
- global 发射 `SETGLOBAL`。
- table/indexed lvalue 发射 `SETTABLE`，这类 lvalue 不来自 `NameBinder`，而由 `ExpressionEmitter::emitLValue()` 对 `IndexExpr` / `MemberExpr` 生成。

## CodeGenerator Compatibility Wrappers

`codegen_binding.cpp` 只保留兼容 facade：

```cpp
SymbolRef CodeGenerator::resolve(const Str& name) {
    return binder_.resolve(name);
}
```

`symbolToValue()` 和 `symbolToLValue()` 同理。外部代码仍可通过 `CodeGenerator` 调用这些方法，但职责已经落到 `NameBinder`。

## Closure Upvalue Emission

名字绑定只记录捕获信息；closure 创建时还需要在外层函数中写入捕获指令。该步骤在 `FunctionCompiler::emitClosureUpvalues()` 中完成：

| `UpvalueCapture` | 外层发射 |
|---|---|
| `inStack == true` | `MOVE 0, uv.index, 0` |
| `inStack == false` | `GETUPVAL 0, uv.index, 0` |

这些指令紧跟在 `CLOSURE` 后面，用 Lua 5.1 风格的 closure upvalue 描述方式把子 `Proto` 的 upvalue 与外层运行时位置连接起来。

## Collaboration Through State

`NameBinder` 自身没有所有权。它通过：

- `CodegenState& state_` 访问当前 `BytecodeBuilder`，用于 global 名字的字符串常量写入。
- `ScopeManager& scopes_` 查找 local、解析 upvalue。
- `CodegenState::parent` 间接参与跨函数 upvalue 解析。
- `UpvalueContext` 记录当前函数已捕获的 upvalue 列表。

因此 binding 层只决定“名字指向哪里”，不直接发射读取或写入指令。读取、写入和 closure 物化分别由 expression、statement 和 function compiler 边界完成。
