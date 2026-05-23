---
status: current
verified_against: src/compiler/ast_visitor.hpp; src/compiler/codegen/codegen.hpp; src/compiler/codegen/codegen_expr.cpp; src/vm/vm_handlers.hpp; src/vm/vm_handlers.cpp; src/vm/vm_handlers/; src/vm/vm_dispatch_strategy.hpp; src/vm/vm_dispatch_strategy.cpp; src/runtime/runtime_services.hpp; src/vm/state/global_state.hpp; src/gc/garbage_collector.hpp; src/gc/gc_strategy.hpp; src/gc/gc_strategy.cpp; src/compiler/codegen/bytecode_builder.hpp; src/compiler/codegen/codegen_state.hpp; docs/roadmap/optimization_and_refactoring.md
last_checked: 2026-05-23
applies_to: architecture pattern registry and implementation boundaries
---

# Architecture Patterns

This file records the design patterns that are intentionally present in the current codebase. It is a registry, not a mandate: new code should prefer the simplest local shape that keeps the interpreter readable, maintainable, and useful for teaching.

## Current Registry

| Pattern | Status | Primary files | Current role |
|---|---|---|---|
| Visitor | Implemented | `src/compiler/ast_visitor.hpp`, `src/compiler/codegen/codegen.hpp`, `src/compiler/codegen/codegen_expr.cpp` | CRTP visitors wrap AST `std::variant` dispatch. Expression lowering currently uses `ExprVisitor<CodeGenerator, ValueResult>`; `StmtVisitor` is available for future statement-side migration. |
| Command | Implemented | `src/vm/vm_handlers.hpp`, `src/vm/vm_handlers.cpp`, `src/vm/vm_handlers/` | VM opcode behavior is represented by free-function handlers registered into `HandlerTable`. Table dispatch calls `runHandler()` instead of switching directly on every opcode. |
| Strategy | Implemented | `src/vm/vm_dispatch_strategy.hpp`, `src/vm/vm_dispatch_strategy.cpp`, `src/gc/gc_strategy.hpp`, `src/gc/gc_strategy.cpp`, `src/runtime/runtime_services.hpp`, `src/vm/vm.cpp` | `DispatchStrategy` selects the VM execution algorithm; `GCStrategy` selects the collector algorithm boundary. `SwitchDispatch` and `MarkSweepGC` are the defaults. |
| Singleton | Compatibility boundary | `src/vm/state/global_state.hpp`, `src/runtime/runtime_services.hpp`, `src/gc/garbage_collector.hpp` | `GlobalState` remains singleton-backed for process-wide runtime services. New compiler, VM, and GC paths should prefer explicit service wiring. `GarbageCollector::getInstance()` remains only as a deprecated compatibility shim. |
| Builder | Implemented | `src/compiler/codegen/bytecode_builder.hpp`, `src/compiler/codegen/codegen_state.hpp` | `BytecodeBuilder` is the narrow write boundary for mutating the active `Proto`: instructions, line info, constants, sub-protos, and debug locals. |

## Boundaries

### Visitor

The AST visitor layer intentionally stays small. `ExprVisitor` and `StmtVisitor` only centralize variant dispatch; they do not own traversal policy, scope state, bytecode emission, or diagnostics. Those responsibilities remain in users such as `CodeGenerator`.

When adding a new AST consumer, prefer a concrete visitor type over adding more branching to `CodeGenerator`. When changing code generation itself, keep the current `CodeGenerator` public API stable.

### Command

The VM command pattern is implemented as a function pointer table, not a virtual class hierarchy. This keeps opcode handlers easy to inspect and avoids per-opcode heap ownership or inheritance plumbing.

The registry file `src/vm/vm_handlers.cpp` owns metadata initialization and family registration. The files under `src/vm/vm_handlers/` own opcode behavior by group. A handler shard should register only its own opcode family.

### Strategy

`SwitchDispatch` remains the default VM dispatch path because it is easiest to debug and matches the interpreter's historical control flow. `TableDispatch` is opt-in and uses the same handler table as the command layer.

Do not add computed-goto or threaded-code dispatch paths. They trade away readability and portability, which conflicts with the roadmap's project goals.

`MarkSweepGC` remains the default GC strategy. `IncrementalGC` exists as a teaching placeholder that delegates to the same mark-sweep phases, so tests can prove reachability equivalence before future write barriers and scheduling are introduced.

### Singleton

`GlobalState` is still the compatibility anchor for shared runtime services. The migration direction is explicit dependency passing through `RuntimeServices`, especially at compiler, VM, REPL, bytecode-tool, and test entry points.

New code should not reach for `GlobalState::getInstance()` when a `RuntimeServices&`, `LuaState*`, or `GlobalState&` is already available.

### Builder

`BytecodeBuilder` narrows direct writes to `Proto`, but it is not a full compiler facade. Lowering decisions still belong in `CodeGenerator`; the builder should remain focused on emission mechanics and bounds checks.

## Updating This File

Update this registry when a pattern is introduced, removed, or moved to a different source boundary. Keep planned items separate from implemented items so readers can distinguish current architecture from roadmap intent.
