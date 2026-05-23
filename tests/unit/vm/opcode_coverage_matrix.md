# VM Opcode Coverage Matrix

> PR-54 / task 5.1.1. This matrix is a readable checklist, not a test runner.
> The quality gate verifies that every opcode in `src/compiler/opcode.hpp` has exactly one row here.

Legend:

- **Positive path**: an existing test that exercises the normal handler behavior.
- **Boundary path**: an existing edge/branch test, or an explicit TODO when the gap is still open.
- **Metamethod path**: an existing runtime metamethod test when the opcode can invoke one; otherwise `N/A`.
- **Current gaps**: follow-up work made visible by this checklist.

| Opcode | Group | Positive path | Boundary path | Metamethod path | Current gaps |
|---|---|---|---|---|---|
| MOVE | DataMove | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Data Move Handlers Execute Directly | TODO: add alias copy case where A equals B | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Boundary edge still missing |
| LOADK | DataMove | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Data Move Handlers Execute Directly | TODO: add highest valid constant index case | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Boundary edge still missing |
| LOADBOOL | DataMove | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Data Move Handlers Execute Directly | Covered: C set skips the next instruction | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Good for PR-54 |
| LOADNIL | DataMove | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Data Move Handlers Execute Directly | Covered: inclusive A..B register range clearing | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Good for PR-54 |
| GETUPVAL | Upvalue | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Global And Upvalue Handlers Execute Directly | TODO: add highest valid upvalue slot case | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Boundary edge still missing |
| GETGLOBAL | Global | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Global And Upvalue Handlers Execute Directly | TODO: add missing global returns nil case | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Boundary edge still missing |
| GETTABLE | Table | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Table Handlers Execute Directly | TODO: add absent key direct table lookup case | `tests/unit/metamethod/test_metamethod_arith.cpp` - Metamethod/Lua function metamethods and basic type metatable covers `__index` | Direct boundary edge still missing |
| SETGLOBAL | Global | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Global And Upvalue Handlers Execute Directly | TODO: add overwrite existing global case | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Boundary edge still missing |
| SETUPVAL | Upvalue | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Global And Upvalue Handlers Execute Directly | TODO: add closed upvalue overwrite case after multiple writes | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Boundary edge still missing |
| SETTABLE | Table | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Table Handlers Execute Directly | TODO: add nil value assignment case | `tests/unit/metamethod/test_metamethod_arith.cpp` - Metamethod/Lua function metamethods and basic type metatable covers `__newindex` | Direct boundary edge still missing |
| NEWTABLE | Table | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Table Handlers Execute Directly | TODO: add non-zero array/hash size operands case | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Boundary edge still missing |
| SELF | Table | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Table Handlers Execute Directly | Covered: receiver copy into R(A+1) | `tests/unit/metamethod/test_metamethod_arith.cpp` - Metamethod/Lua function metamethods and basic type metatable covers method lookup via string metatable `__index` | Good for PR-54 |
| ADD | Arithmetic | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Arithmetic Handlers Execute Directly | `tests/unit/metamethod/test_metamethod_arith.cpp` - Metamethod/Fallback covers left operand fallback | `tests/unit/metamethod/test_metamethod_arith.cpp` - Metamethod/Add and Lua function metamethods cover `__add` | Good for PR-54 |
| SUB | Arithmetic | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Arithmetic Handlers Execute Directly | TODO: add non-number error path through VM handler | `tests/unit/metamethod/test_metamethod_complete.cpp` - Complete Metamethods/Arithmetic metamethods covers `__sub` | Boundary edge still missing |
| MUL | Arithmetic | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Arithmetic Handlers Execute Directly | TODO: add RK constant/register mixed operand edge | `tests/unit/metamethod/test_metamethod_complete.cpp` - Complete Metamethods/Arithmetic metamethods covers `__mul` | Boundary edge still missing |
| DIV | Arithmetic | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Arithmetic Handlers Execute Directly | TODO: add division by zero behavior lock if intended | `tests/unit/metamethod/test_metamethod_complete.cpp` - Complete Metamethods/Arithmetic metamethods covers `__div` | Boundary policy still implicit |
| MOD | Arithmetic | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Arithmetic Handlers Execute Directly | TODO: add modulo by zero behavior lock if intended | `tests/unit/metamethod/test_metamethod_complete.cpp` - Complete Metamethods/__mod and __pow metamethods currently registers `__mod` only | Runtime `MOD` metamethod execution still weak |
| POW | Arithmetic | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Arithmetic Handlers Execute Directly | TODO: add fractional or negative exponent edge | `tests/unit/metamethod/test_metamethod_complete.cpp` - Complete Metamethods/__mod and __pow metamethods currently registers `__pow` only | Runtime `POW` metamethod execution still weak |
| UNM | Unary | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Unary Handlers Execute Directly | TODO: add non-number error path through VM handler | TODO: add runtime `__unm` execution test | Metamethod path missing |
| NOT | Unary | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Unary Handlers Execute Directly | TODO: add nil and false truthiness split | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Boundary edge still missing |
| LEN | Unary | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Unary Handlers Execute Directly | Covered: string length path | `tests/unit/metamethod/test_metamethod_complete.cpp` - Complete Metamethods/Other metamethods covers `__len` | Good for PR-54 |
| CONCAT | Unary | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Unary Handlers Execute Directly | Covered: multi-register concat range | `tests/unit/metamethod/test_metamethod_complete.cpp` - Complete Metamethods/Other metamethods currently registers `__concat` only | Runtime `CONCAT` metamethod execution still weak |
| JMP | Branch | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Branch And Comparison Handlers Execute Directly | Covered: signed sBx pc adjustment | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Good for PR-54 |
| EQ | Comparison | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Branch And Comparison Handlers Execute Directly | Covered: skip when comparison result differs from A | `tests/unit/metamethod/test_metamethod_complete.cpp` - Complete Metamethods/Comparison metamethods covers `__eq` | Good for PR-54 |
| LT | Comparison | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Branch And Comparison Handlers Execute Directly | Covered: skip when comparison result differs from A | `tests/unit/metamethod/test_metamethod_complete.cpp` - Complete Metamethods/Comparison metamethods covers `__lt` | Good for PR-54 |
| LE | Comparison | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Branch And Comparison Handlers Execute Directly | Covered: keep pc when comparison result matches A | `tests/unit/metamethod/test_metamethod_complete.cpp` - Complete Metamethods/Comparison metamethods covers `__le` | Good for PR-54 |
| TEST | Branch | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Branch And Comparison Handlers Execute Directly | Covered: match applies following JMP and miss advances once | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Good for PR-54 |
| TESTSET | Branch | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Branch And Comparison Handlers Execute Directly | Covered: match copies value and miss leaves destination unchanged | N/A - opcode metadata marks `mayInvokeMetamethod=false` | Good for PR-54 |
| CALL | Call | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Call And Return Handlers Execute Directly | Covered: C call, Lua call reentry, and yielded C call | `tests/unit/metamethod/test_metamethod_arith.cpp` - Metamethod/Lua function metamethods and basic type metatable covers `__call` | Good for PR-54 |
| TAILCALL | Call | `tests/unit/vm/test_vm_dispatch.cpp` - VM Dispatch/Call And Return Handlers Execute Directly | Covered: Lua tailcall reuses current CallInfo and increments tailcall count | TODO: add tailcall through `__call` metamethod | Metamethod path missing |
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
- TODO cells are allowed because this matrix is intentionally surfacing test gaps; removing a TODO requires adding or identifying the corresponding test first.
