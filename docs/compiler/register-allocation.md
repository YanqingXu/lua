---
status: current
verified_against: docs/status/project-status.md; src/compiler/register_allocator.hpp; src/compiler/codegen/codegen.hpp; src/compiler/codegen/codegen.cpp; src/compiler/codegen/codegen_ops.hpp; src/compiler/codegen/expression_emitter.cpp; src/compiler/codegen/statement_emitter.cpp; src/compiler/codegen/codegen_stmt.cpp; src/compiler/codegen/codegen_state.hpp
last_checked: 2026-05-22
applies_to: current CodeGenerator register allocation model
---

# Register Allocation

Lua 5.1 bytecode is register-based. Each `Proto` records a `maxStackSize`, and each instruction reads or writes numbered virtual registers within the active call frame. The C++ compiler therefore has to decide where locals, temporaries, call arguments, and return values live before VM execution begins.

The current implementation uses `RegisterAllocator` as the register cursor owner:

```cpp
class RegisterAllocator {
public:
    void bind(Proto* proto) noexcept;
    i32 current() const noexcept;
    i32 alloc();
    void freeReg(i32 reg, i32 activeLocals);
    void freeRegs(i32 n);
    void checkStack(i32 n);
    void setFreeReg(i32 reg) noexcept;
    void resetToLocals(i32 activeLocals) noexcept;
    void restore(i32 saved) noexcept;
    void reserve(i32 count) noexcept;
    void ensureAtLeast(i32 reg) noexcept;
    void reset(i32 start = 0) noexcept;
};
```

`freereg_` is private. Code generation code reaches it through the semantic methods above.

## Register Regions

At any point in a function body, registers are organized like this:

```text
R(0) ... R(activeVarCount-1)     active locals
R(activeVarCount) ... R(free-1)  temporary values, call frames, table fields
R(free) ...               available registers
```

`LocalVarScope::activeVarCount_` is the number of active local variables. `RegisterAllocator::current()` points at the next available temporary slot.

## Main Rules

- Local variables occupy fixed registers for the lifetime of their lexical scope.
- Temporaries are allocated from `current()` and freed only when they are the most recent temporary.
- Statement boundaries usually reset temporaries back to active locals.
- Function calls require contiguous registers: function at `base`, arguments after it, results beginning at `base`.
- Multi-return values are represented by `CallResultInfo` until the surrounding context decides how many results are wanted.
- Table array fields are accumulated after the table register until `SETLIST` flushes them.
- `FORPREP`, `FORLOOP`, and `TFORLOOP` use fixed register layouts defined by Lua 5.1.

## Value Lowering Helpers

The old `ExprDesc` / `exp2*` model is no longer part of production compiler sources. The current helpers operate on `ValueResult`:

| Current helper | Purpose |
|---|---|
| `emitValue(const Expr&)` | Lower an expression into a `ValueResult` |
| `materializeValue(const ValueResult&, i32 reg)` | Force a value into a specific register |
| `valueToRK(const ValueResult&)` | Use RK encoding when possible, otherwise materialize |
| `valueToAnyReg(const ValueResult&)` | Return a register containing the value |
| `valueToNextReg(const ValueResult&)` | Materialize at the current free register and advance |
| `forceSingleValue(const ValueResult&)` | Convert call/vararg multi-return to one value |

## Common Flows

### Local Declaration

For `local a, b = f()`:

1. Save the current free register.
2. Reserve local slots starting at `activeVarCount_`.
3. Generate initializer values into the local base.
4. If the final initializer is a call or vararg, set its wanted result count.
5. Fill missing locals with `LOADNIL`.
6. Activate locals with `adjustLocalVars`.

The important invariant is that local slots and initializer result slots line up before locals become active.

### Function Call Expression

For `print(type(x))`:

1. Lower the callee to a register.
2. Place each argument contiguously after the callee.
3. If the final argument is a call or vararg, decide whether it should be open-ended.
4. Emit `CALL`.
5. Return `CallResultInfo` so the parent context can decide whether to keep one result or many.

### Return Statement

For `return f()`:

- If the returned expression is a single call in tail position, code generation may emit `TAILCALL`.
- If the last returned expression is call/vararg and multiple values are wanted, emit `RETURN` with `B = 0`.
- Otherwise materialize each result into contiguous registers and emit a fixed-count `RETURN`.

### Numeric For

The numeric for loop uses:

```text
R(base)     internal index
R(base + 1) limit
R(base + 2) step
R(base + 3) visible loop variable
```

The compiler emits `FORPREP` before the body and `FORLOOP` after the body. The register allocator keeps the loop control range reserved while the loop body is generated.

### Generic For

The generic for loop uses:

```text
R(base)     generator function
R(base + 1) state
R(base + 2) control variable
R(base + 3) first visible loop variable
```

`TFORLOOP` writes iterator results beginning at `base + 3`.

## Debugging Register Bugs

Most register bugs fall into one of these categories:

- A temporary was not freed and later locals shifted upward.
- A saved free register was restored too early or too late.
- A call argument region was not kept contiguous.
- A multi-return call was accidentally forced to one value.
- A loop layout reused one of Lua's reserved control registers.

Useful tests:

```powershell
bin\lua_test.exe --filter "Value Pipeline"
bin\lua_test.exe --filter "Call Pipeline"
bin\lua_test.exe --filter "Codegen MultiRet"
bin\lua_test.exe --filter "Function Codegen"
```
