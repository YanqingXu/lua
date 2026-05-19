---
status: current
verified_against: docs/status/project-status.md; README.md; src/common/; src/core/; src/compiler/; src/vm/; src/gc/; src/lib/; src/runtime/runtime_services.hpp
last_checked: 2026-05-19
applies_to: current high-level architecture and source layout
---

# Architecture Overview

This project is a modern C++ implementation of a Lua 5.1.5-style interpreter. The current codebase is organized around a straightforward pipeline:

```text
Lua source
  -> Lexer / Parser
  -> AST
  -> CodeGenerator
  -> Proto bytecode
  -> VM execution
```

The implementation is intentionally learning-friendly: most Lua concepts have direct C++ counterparts, and the compiler / VM boundary is visible through `Proto`, `OpCode`, and the register-based instruction model.

## Source Layers

| Layer | Main files | Current responsibility |
|---|---|---|
| Application | `src/main.cpp`, `src/repl.cpp`, `src/app/app_options.*` | CLI, script mode, REPL mode, trace option parsing |
| Bytecode tool | `src/bytecode/bytecode_main.cpp`, `src/bytecode/bytecode_printer.*` | Compile a script to `Proto`; printer is currently a stub |
| Runtime services | `src/runtime/runtime_services.hpp` | Thin explicit bundle over `GlobalState`, `StringPool`, and `GarbageCollector` |
| Compiler frontend | `src/compiler/lexer.*`, `src/compiler/parser*.cpp`, `src/compiler/ast.*` | Tokenize source and build AST |
| Code generation | `src/compiler/codegen*.cpp`, `codegen_types.hpp`, `codegen_context.hpp`, `bytecode_builder.hpp` | Lower AST to `Proto` bytecode |
| Core objects | `src/core/value.*`, `table.*`, `function.*`, `upvalue.*`, `userdata.*`, `thread.*` | C++ representation of Lua values and GC objects |
| VM | `src/vm/vm*.cpp`, `lua_state.*`, `stack.*`, `call_info.hpp` | Execute bytecode, manage calls, stack, hooks, trace, coroutine yield |
| GC | `src/gc/garbage_collector.*`, `src/core/gc_object.*` | Mark-sweep collection, weak tables, userdata finalizers |
| Standard library | `src/lib/*.cpp`, `lib_catalog.*`, `lib_manager.*`, `lib_registry.*` | Register Lua standard libraries into a `LuaState` |
| Debug trace | `src/debug/*.hpp`, `src/debug/*.cpp`, `src/vm/vm_trace.cpp` | JSONL VM execution trace and value serialization |

## Core Runtime Model

`Value` is the single Lua value container. It uses `std::variant` over nil, boolean, light userdata, number, `GCString*`, `Table*`, `Function*`, `Userdata*`, and `Thread*`.

`GCObject` is the base class for collectable objects. Current collectable types include:

- `GCString`
- `Table`
- `Proto`
- `Function`
- `Upvalue`
- `Userdata`
- `Thread`

Each subclass implements `mark(GarbageCollector&)` and `getSize()`. The collector owns the global object list and performs mark-sweep collection.

## Global And Thread State

`GlobalState` is still singleton-backed, but new entry points should prefer passing `RuntimeServices` where available. It owns shared runtime services:

- string pool
- garbage collector
- registry table
- fixed strings and metamethod names
- primitive type metatables
- main and currently running thread pointers

`LuaState` represents an execution state. It owns the value stack, call stack, global table, debug hook state, open upvalues, and yield bookkeeping. `Thread` wraps an independent `LuaState` for coroutine execution.

## Compiler Shape

The compiler is currently AST-based rather than Lua 5.1's original single-pass parser/codegen pipeline.

```text
Parser -> Chunk AST -> CodeGenerator -> Proto
```

`CodeGenerator` is still the orchestration class, but its implementation has been split:

- `codegen_binding.cpp`: name resolution to `SymbolRef`
- `codegen_expr.cpp`: value, condition, lvalue, call, vararg, table expression lowering
- `codegen_jump.cpp`: jump lists and patching
- `codegen_stmt.cpp`: statements, loops, functions, returns, blocks
- `codegen.cpp`: constructor, top-level generation, bytecode emission wrappers

The current expression pipeline uses `SymbolRef`, `ValueResult`, `CondResult`, `LValueRef`, and `CallResultInfo`. Historical `ExprDesc` material is archived under `docs/archive/history/exprdesc.md`.

## VM Shape

The VM executes Lua 5.1-style register bytecode. `src/compiler/opcode.hpp` defines 38 opcodes and instruction encodings. `src/vm/vm.cpp` contains the main dispatch loop, while helpers live in focused files:

- `vm_entry.cpp`: `VM::execute()` and `VM::call()`
- `vm_call.cpp`: precall, postcall, tailcall frame reuse
- `vm_ops.cpp`: arithmetic, comparison, table/metamethod helpers
- `vm_table.cpp`: table initialization helpers
- `vm_frame.cpp`: closure and vararg helpers
- `vm_loop.cpp`: generic for-loop helpers
- `vm_trace.cpp`: trace and debug hook dispatch

## Build Targets

The Visual Studio solution contains four active projects:

- `lua.vcxproj`: core static library
- `lua_app.vcxproj`: interpreter / REPL executable
- `lua_test.vcxproj`: unit test executable
- `lua_bytecode.vcxproj`: bytecode inspection executable

CMake/CTest exists as a secondary validation path and builds `lua_core`, `lua_app`, `lua_test`, and `lua_bytecode`.

## Reading Map

- Current facts: `docs/status/project-status.md`
- First-read path: `docs/index.md`
- Compiler details: `docs/compiler/bytecode-generation.md`
- VM opcodes: `docs/vm/instruction-set.md`
- GC details: `docs/architecture/gc.md`
- Standard library overview: `docs/stdlib/overview.md`
