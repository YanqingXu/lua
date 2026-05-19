---
status: current
verified_against: src/runtime/runtime_services.hpp; src/main.cpp; src/repl.cpp; src/bytecode/bytecode_main.cpp; src/compiler/parser.hpp; src/compiler/codegen.hpp; src/vm/vm.hpp; tests/unit/vm/test_runtime_services.cpp
last_checked: 2026-05-19
applies_to: current RuntimeServices boundary
---

# Runtime Services

`RuntimeServices` is a small explicit dependency bundle:

```cpp
struct RuntimeServices {
    GlobalState& globalState;
    StringPool& strings;
    GarbageCollector& gc;
};
```

It is intentionally thin. The runtime is still backed by `GlobalState::getInstance()` and `GarbageCollector::getInstance()`, but newer compiler and VM entry points can receive the services they use instead of reaching for singletons at every call site.

## Why It Exists

The project is moving from implicit global access toward explicit runtime boundaries. `RuntimeServices` gives that migration a narrow first step:

- parser/codegen entry points can share the same string pool
- VM execution can be context-aware
- CLI tools can create one services bundle and pass it through
- tests can assert the boundary without forcing a large ownership rewrite

## Current Users

| User | Path |
|---|---|
| Interpreter app | `src/main.cpp` |
| REPL | `src/repl.cpp` |
| Bytecode tool | `src/bytecode/bytecode_main.cpp` |
| Parser overloads | `src/compiler/parser.hpp/.cpp` |
| CodeGenerator constructors | `src/compiler/codegen.hpp/.cpp` |
| VM execute overloads | `src/vm/vm.hpp`, `src/vm/vm.cpp`, `src/vm/vm_entry.cpp` |
| Tests | `tests/unit/vm/test_runtime_services.cpp` |

## Current Contract

- `RuntimeServices::fromSingletons()` is the compatibility constructor.
- `RuntimeServices(GlobalState&)` derives string pool and collector from that global state.
- `RuntimeServices(GlobalState&, StringPool&, GarbageCollector&)` exists for explicit wiring.
- It does not own services and must not outlive the referenced runtime objects.

## Future Direction

The next boundary would be a true runtime context that owns or selects `GlobalState`, `StringPool`, and `GarbageCollector`. Until then, prefer `RuntimeServices` in new compiler/VM-facing code so call sites make dependencies visible.
