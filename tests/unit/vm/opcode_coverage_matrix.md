# VM Opcode Coverage Matrix

> PR-54 / task 5.1.1. This matrix is a readable checklist, not a test runner.
> The quality gate verifies that every opcode in `src/compiler/opcode.hpp` has exactly one row here.

Legend:

- **Positive path**: an existing test that exercises the normal handler behavior.
- **Boundary path**: an existing edge/branch test. New gaps should be added here only with a follow-up task.
- **Metamethod path**: an existing runtime metamethod test when the opcode can invoke one; otherwise `N/A`.
- **Current gaps**: follow-up work made visible by this checklist.

| Opcode | Group | Positive path | Boundary path | Metamethod path | Current gaps |
|---|---|---|---|---|---|
| MOVE | DataMove | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Data Move Handlers Execute Directly | Covered: alias copy case where A equals B | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Good for Phase 4 |
| LOADK | DataMove | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Data Move Handlers Execute Directly | Covered: highest valid Bx constant index | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Good for Phase 4 |
| LOADBOOL | DataMove | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Data Move Handlers Execute Directly | Covered: C set skips the next instruction | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Good for PR-54 |
| LOADNIL | DataMove | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Data Move Handlers Execute Directly | Covered: inclusive A..B register range clearing | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Good for PR-54 |
| GETUPVAL | Upvalue | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Global And Upvalue Handlers Execute Directly | Covered: highest configured upvalue slot case | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Good for Phase 4 |
| GETGLOBAL | Global | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Global And Upvalue Handlers Execute Directly | Covered: missing global returns nil | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Good for Phase 4 |
| GETTABLE | Table | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Table Handlers Execute Directly | Covered: absent key direct table lookup returns nil | `tests/unit/metamethod/test_metamethod_arith.cpp` - Metamethod/Lua function metamethods and basic type metatable covers `__index` | Good for Phase 4 |
| SETGLOBAL | Global | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Global And Upvalue Handlers Execute Directly | Covered: overwrites an existing global in the function environment | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Good for Phase 4 |
| SETUPVAL | Upvalue | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Global And Upvalue Handlers Execute Directly | Covered: closed upvalue overwrite after multiple writes | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Good for Phase 4 |
| SETTABLE | Table | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Table Handlers Execute Directly | Covered: nil value assignment deletes a key | `tests/unit/metamethod/test_metamethod_arith.cpp` - Metamethod/Lua function metamethods and basic type metatable covers `__newindex` | Good for Phase 4 |
| NEWTABLE | Table | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Table Handlers Execute Directly | Covered: non-zero array/hash size operands | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Good for Phase 4 |
| SELF | Table | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Table Handlers Execute Directly | Covered: receiver copy into R(A+1) | `tests/unit/metamethod/test_metamethod_arith.cpp` - Metamethod/Lua function metamethods and basic type metatable covers method lookup via string metatable `__index` | Good for PR-54 |
| ADD | Arithmetic | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Arithmetic Handlers Execute Directly | `tests/unit/metamethod/test_metamethod_arith.cpp` - Metamethod/Fallback covers left operand fallback | `tests/unit/metamethod/test_metamethod_arith.cpp` - Metamethod/Add and Lua function metamethods cover `__add` | Good for PR-54 |
| SUB | Arithmetic | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Arithmetic Handlers Execute Directly | Covered: non-number error path through VM handler | `tests/unit/metamethod/test_metamethod_complete.cpp` - Complete Metamethods/Arithmetic metamethods covers `__sub` | Good for Phase 4 |
| MUL | Arithmetic | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Arithmetic Handlers Execute Directly | Covered: RK constant/register mixed operand edge | `tests/unit/metamethod/test_metamethod_complete.cpp` - Complete Metamethods/Arithmetic metamethods covers `__mul` | Good for Phase 4 |
| DIV | Arithmetic | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Arithmetic Handlers Execute Directly | Covered: Lua 5.1 double division-by-zero behavior lock | `tests/unit/metamethod/test_metamethod_complete.cpp` - Complete Metamethods/Arithmetic metamethods covers `__div` | Good for Phase 4 |
| MOD | Arithmetic | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Arithmetic Handlers Execute Directly | Covered: modulo-by-zero NaN behavior lock | `tests/unit/metamethod/test_metamethod_arith.cpp` - Metamethod/Runtime metamethod opcode execution covers runtime `__mod` | Good for Phase 4 |
| POW | Arithmetic | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Arithmetic Handlers Execute Directly | Covered: fractional and negative exponent edges | `tests/unit/metamethod/test_metamethod_arith.cpp` - Metamethod/Runtime metamethod opcode execution covers runtime `__pow` | Good for Phase 4 |
| UNM | Unary | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Unary Handlers Execute Directly | Covered: non-number error path through VM handler | `tests/unit/metamethod/test_metamethod_arith.cpp` - Metamethod/Runtime metamethod opcode execution covers runtime `__unm` | Good for Phase 4 |
| NOT | Unary | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Unary Handlers Execute Directly | Covered: nil, false, true, and zero truthiness split | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Good for Phase 4 |
| LEN | Unary | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Unary Handlers Execute Directly | Covered: string length path | `tests/unit/metamethod/test_metamethod_complete.cpp` - Complete Metamethods/Other metamethods covers `__len` | Good for PR-54 |
| CONCAT | Unary | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Unary Handlers Execute Directly | Covered: multi-register concat range | `tests/unit/metamethod/test_metamethod_arith.cpp` - Metamethod/Runtime metamethod opcode execution covers runtime `__concat` | Good for Phase 4 |
| JMP | Branch | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Branch And Comparison Handlers Execute Directly | Covered: signed sBx pc adjustment | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Good for PR-54 |
| EQ | Comparison | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Branch And Comparison Handlers Execute Directly | Covered: skip when comparison result differs from A | `tests/unit/metamethod/test_metamethod_complete.cpp` - Complete Metamethods/Comparison metamethods covers `__eq` | Good for PR-54 |
| LT | Comparison | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Branch And Comparison Handlers Execute Directly | Covered: skip when comparison result differs from A | `tests/unit/metamethod/test_metamethod_complete.cpp` - Complete Metamethods/Comparison metamethods covers `__lt` | Good for PR-54 |
| LE | Comparison | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Branch And Comparison Handlers Execute Directly | Covered: keep pc when comparison result matches A | `tests/unit/metamethod/test_metamethod_complete.cpp` - Complete Metamethods/Comparison metamethods covers `__le` | Good for PR-54 |
| TEST | Branch | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Branch And Comparison Handlers Execute Directly | Covered: match applies following JMP and miss advances once | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Good for PR-54 |
| TESTSET | Branch | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Branch And Comparison Handlers Execute Directly | Covered: match copies value and miss leaves destination unchanged | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Good for PR-54 |
| CALL | Call | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Call And Return Handlers Execute Directly | Covered: C call, Lua call reentry, and yielded C call | `tests/unit/metamethod/test_metamethod_arith.cpp` - Metamethod/Lua function metamethods and basic type metatable covers `__call` | Good for PR-54 |
| TAILCALL | Call | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Call And Return Handlers Execute Directly | Covered: Lua tailcall reuses current CallInfo and increments tailcall count | `tests/unit/metamethod/test_metamethod_arith.cpp` - Metamethod/Runtime metamethod opcode execution covers tailcall through `__call` | Good for Phase 4 |
| RETURN | Call | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Call And Return Handlers Execute Directly | Covered: outermost frame completion moves values to `ci.func` and shrinks top | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Good for PR-54 |
| FORLOOP | Loop | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Loop And Close Handlers Execute Directly | Covered: continue jumps and termination does not jump | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Good for PR-54 |
| FORPREP | Loop | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Loop And Close Handlers Execute Directly | Covered: initializes internal index and applies sBx | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Good for PR-54 |
| TFORLOOP | Loop | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Loop And Close Handlers Execute Directly | Covered: iterator result continues and nil result advances once | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Good for PR-54 |
| SETLIST | Table | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Table Handlers Execute Directly | Covered: consecutive array slot population | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Good for PR-54 |
| CLOSE | Branch | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Loop And Close Handlers Execute Directly | Covered: lower upvalue stays open and upper upvalue closes | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Good for PR-54 |
| CLOSURE | Closure | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Closure And Vararg Handlers Execute Directly | Covered: consumes MOVE and GETUPVAL pseudo instructions | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Good for PR-54 |
| VARARG | Vararg | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Closure And Vararg Handlers Execute Directly | Covered: fixed and open requested varargs | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Good for PR-54 |

## PR-54 Verification Standard

- `tools/check_opcode_coverage_matrix.ps1` must pass and report all 38 opcodes.
- `tools/run_quality_gate.ps1` must include the opcode coverage matrix step before build/test work.
- Adding, removing, or renaming an opcode in `src/compiler/opcode.hpp` must fail the matrix check until this file is updated.
- Open gaps should be tracked by adding a clear Current gaps entry and a roadmap task; removing a gap requires adding or identifying the corresponding test first.
