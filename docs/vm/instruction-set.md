---
status: current
verified_against: src/compiler/opcode.hpp; src/compiler/opcode.cpp; src/vm/vm.cpp; src/vm/vm_ops.cpp; src/vm/vm_call.cpp; src/vm/vm_table.cpp; src/vm/vm_frame.cpp; src/vm/vm_loop.cpp
last_checked: 2026-05-19
applies_to: current Lua 5.1-style VM opcode set
---

# VM Instruction Set

The VM uses Lua 5.1-style register bytecode. Instructions are encoded in `src/compiler/opcode.hpp` and executed by the VM dispatch loop plus helper files under `src/vm/`.

`NUM_OPCODES` is currently 38.

## Encoding

| Encoding | Meaning |
|---|---|
| iABC | opcode + A + B + C |
| iABx | opcode + A + Bx |
| iAsBx | opcode + A + signed Bx |

`RK` operands can refer to either a register or a constant table slot. `BITRK` marks constants; `ISK()` and `INDEXK()` decode the value.

## Opcode Groups

| Group | Opcodes |
|---|---|
| Data movement | `MOVE`, `LOADK`, `LOADBOOL`, `LOADNIL` |
| Variable access | `GETUPVAL`, `GETGLOBAL`, `GETTABLE` |
| Variable writes | `SETGLOBAL`, `SETUPVAL`, `SETTABLE` |
| Table setup | `NEWTABLE`, `SELF`, `SETLIST` |
| Arithmetic/unary | `ADD`, `SUB`, `MUL`, `DIV`, `MOD`, `POW`, `UNM`, `NOT`, `LEN` |
| String | `CONCAT` |
| Branching | `JMP`, `EQ`, `LT`, `LE`, `TEST`, `TESTSET` |
| Calls | `CALL`, `TAILCALL`, `RETURN` |
| Loops | `FORLOOP`, `FORPREP`, `TFORLOOP` |
| Closures/upvalues | `CLOSE`, `CLOSURE` |
| Vararg | `VARARG` |

## Semantics Summary

| Opcode | Short behavior |
|---|---|
| `MOVE` | `R(A) := R(B)` |
| `LOADK` | `R(A) := K(Bx)` |
| `LOADBOOL` | `R(A) := bool(B)`; skip next instruction if `C != 0` |
| `LOADNIL` | Set `R(A)` through `R(B)` to nil |
| `GETUPVAL` | Load upvalue `B` into `R(A)` |
| `GETGLOBAL` | Load global `K(Bx)` into `R(A)` |
| `GETTABLE` | Load `R(B)[RK(C)]` into `R(A)` |
| `SETGLOBAL` | Store `R(A)` into global `K(Bx)` |
| `SETUPVAL` | Store `R(A)` into upvalue `B` |
| `SETTABLE` | Store `RK(C)` into `R(A)[RK(B)]` |
| `NEWTABLE` | Create a table in `R(A)` |
| `SELF` | Prepare method call receiver and method function |
| `ADD`..`POW` | Arithmetic over `RK(B)` and `RK(C)` with metamethod fallbacks where applicable |
| `UNM`, `LEN` | Unary minus and length with metamethod support where applicable |
| `NOT` | Lua truthiness negation |
| `CONCAT` | Concatenate registers `R(B)` through `R(C)` |
| `JMP` | Add signed offset `sBx` to PC |
| `EQ`, `LT`, `LE` | Conditional comparison and skip |
| `TEST`, `TESTSET` | Truthiness tests used by conditions and short-circuit expressions |
| `CALL` | Call function in `R(A)` with encoded arg/result counts |
| `TAILCALL` | Tail-call function in `R(A)` and reuse the current frame where possible |
| `RETURN` | Return fixed or open-ended values from a frame |
| `FORPREP`, `FORLOOP` | Numeric for-loop setup and iteration |
| `TFORLOOP` | Generic for-loop iterator call |
| `SETLIST` | Bulk write array fields into a table |
| `CLOSE` | Close open upvalues at or above `R(A)` |
| `CLOSURE` | Create a closure from child proto `Bx` and capture upvalues |
| `VARARG` | Load vararg values into registers |

## Where To Read

- Encoding helpers: `src/compiler/opcode.hpp`
- Main dispatch: `src/vm/vm.cpp`
- Arithmetic, comparison, metamethod helpers: `src/vm/vm_ops.cpp`
- Calls and tail calls: `src/vm/vm_call.cpp`
- Closures and vararg: `src/vm/vm_frame.cpp`
- Generic for: `src/vm/vm_loop.cpp`
- SETLIST: `src/vm/vm_table.cpp`

Useful tests:

```powershell
bin\lua_test.exe --filter "VM Dispatch"
bin\lua_test.exe --filter "VM Internal"
bin\lua_test.exe --filter "Function Call"
```
