---
status: current
verified_against: src/runtime/runtime_services.hpp; src/gc/garbage_collector.hpp; src/gc/gc_strategy.hpp; src/gc/gc_sweep.cpp; src/vm/vm_dispatch_strategy.hpp; src/vm/vm.cpp; src/main.cpp; src/repl.cpp; src/bytecode/bytecode_main.cpp; src/compiler/parser/parser.hpp; src/compiler/codegen/codegen.hpp; src/vm/vm.hpp; tests/unit/vm/test_runtime_services.cpp; tests/unit/vm/test_vm_dispatch.cpp; tests/unit/gc/test_gc.cpp
last_checked: 2026-05-23
applies_to: current RuntimeServices boundary
---

# Runtime Services

`RuntimeServices` is a small explicit dependency bundle:

```cpp
struct RuntimeServices {
    GlobalState& globalState;
    StringPool& strings;
    GarbageCollector& gc;
    VM::DispatchStrategy* dispatchStrategy;
};
```

It is intentionally thin. The main runtime path is still anchored by `GlobalState::getInstance()`, while the legacy `GarbageCollector::getInstance()` remains only as a deprecated compatibility shim. Newer compiler, VM, and GC paths can receive the services they use instead of reaching for singletons at every call site. VM execution can receive an optional dispatch strategy for testing or teaching the switch/table dispatch difference; GC execution now reaches the active `GCStrategy` through the collector.

## Why It Exists

The project is moving from implicit global access toward explicit runtime boundaries. `RuntimeServices` gives that migration a narrow first step:

- parser/codegen entry points can share the same string pool
- VM execution can be context-aware
- VM dispatch can use the default `SwitchDispatch` or an injected `TableDispatch`
- CLI tools can create one services bundle and pass it through
- GC sweep receives the relevant `StringPool&` explicitly when removing interned strings
- GC strategy selection stays behind `RuntimeServices.gc`
- tests can assert the boundary without forcing a large ownership rewrite

## Current Users

| User | Path |
|---|---|
| Interpreter app | `src/main.cpp` |
| REPL | `src/repl.cpp` |
| Bytecode tool | `src/bytecode/bytecode_main.cpp` |
| Parser overloads | `src/compiler/parser/parser.hpp`, `src/compiler/parser/parser.cpp` |
| CodeGenerator constructors | `src/compiler/codegen/codegen.hpp`, `src/compiler/codegen/codegen.cpp` |
| VM execute overloads | `src/vm/vm.hpp`, `src/vm/vm.cpp`, `src/vm/vm_entry.cpp` |
| Tests | `tests/unit/vm/test_runtime_services.cpp` |

## Current Contract

- `RuntimeServices::fromSingletons()` is the compatibility constructor.
- `RuntimeServices(GlobalState&, VM::DispatchStrategy* = nullptr)` derives string pool and collector from that global state.
- `RuntimeServices(GlobalState&, StringPool&, GarbageCollector&, VM::DispatchStrategy* = nullptr)` exists for explicit wiring.
- `dispatchStrategy == nullptr` means VM execution uses `VM::defaultDispatchStrategy()`.
- It does not own services and must not outlive the referenced runtime objects.

## Future Direction

The next boundary would be a true runtime context that owns or selects `GlobalState`, `StringPool`, `GarbageCollector`, and VM dispatch policy. Until then, prefer `RuntimeServices` in new compiler/VM-facing code so call sites make dependencies visible.
