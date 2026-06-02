---
status: current
verified_against: src/runtime/runtime_services.hpp; src/core/string_pool.hpp; src/vm/state/global_state.hpp; src/vm/state/global_state.cpp; src/vm/state/lua_state.hpp; src/vm/state/lua_state.cpp; src/gc/garbage_collector.hpp; src/gc/gc_strategy.hpp; src/gc/gc_sweep.cpp; src/vm/vm_dispatch_strategy.hpp; src/vm/vm.cpp; src/main.cpp; src/repl.cpp; src/bytecode/bytecode_main.cpp; src/compiler/parser/parser.hpp; src/compiler/codegen/codegen.hpp; src/vm/vm.hpp; tests/unit/vm/test_runtime_services.cpp; tests/unit/vm/test_vm_dispatch.cpp; tests/unit/gc/test_gc.cpp
last_checked: 2026-06-01
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

It is intentionally thin. The main executable paths now create an owning `EngineContext`, while `GlobalState::getInstance()` and the legacy `GarbageCollector::getInstance()` remain as compatibility shims for older overloads and tests. Newer compiler, VM, standard-library, and GC paths receive the services they use instead of reaching for singletons at every call site. VM execution can receive an optional dispatch strategy for testing or teaching the switch/table dispatch difference; GC execution reaches the active `GCStrategy` through the collector.

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
| Standard library call sites | `src/lib/baselib.cpp`, `src/lib/debuglib.cpp`, `src/lib/packagelib.cpp`, `src/lib/stringlib.cpp`, `src/lib/tablelib.cpp` |
| Tests | `tests/unit/vm/test_runtime_services.cpp` |

## Current Contract

- `RuntimeServices::fromSingletons()` is the compatibility constructor.
- `RuntimeServices(GlobalState&, VM::DispatchStrategy* = nullptr)` derives string pool and collector from that global state.
- `RuntimeServices(GlobalState&, StringPool&, GarbageCollector&, VM::DispatchStrategy* = nullptr)` exists for explicit wiring.
- `dispatchStrategy == nullptr` means VM execution uses `VM::defaultDispatchStrategy()`.
- It does not own services and must not outlive the referenced runtime objects.

## Owning EngineContext

`EngineContext` is the owning counterpart to `RuntimeServices`:

```cpp
class EngineContext {
public:
    RuntimeServices services(VM::DispatchStrategy* dispatch = nullptr) noexcept;
    GlobalState& globalState() noexcept;
    StringPool& strings() noexcept;
    GarbageCollector& gc() noexcept;
};
```

Each context owns:

- an independent `StringPool`
- an independent `GlobalState`
- the `GarbageCollector` owned by that global state
- registry, primitive metatables, reserved strings, metamethod names, and main-thread bookkeeping created inside that global state

`LuaState::newState(EngineContext&)` creates a main state inside that context. Tests assert that two contexts intern the same text into distinct `GCString` objects and that those strings belong to different collectors.

## Future Direction

`EngineContext` is now used by the `lua_app` and `lua_bytecode` executable entry points. Several VM/compiler compatibility overloads and older tests still intentionally use `RuntimeServices::fromSingletons()`. Prefer `EngineContext` for new isolation tests and new embedding surfaces; keep `fromSingletons()` only where preserving legacy behavior is part of the caller's contract.

## Documented Singleton Exceptions

The current `src/` singleton references are intentional compatibility or null-context fallbacks:

| Path | Exception |
|---|---|
| `src/runtime/runtime_services.hpp` | Defines `RuntimeServices::fromSingletons()` as the legacy constructor. |
| `src/vm/state/global_state.*` | Owns the deprecated singleton accessor and default constructor shim. |
| `src/gc/garbage_collector.cpp` | Owns the deprecated legacy GC accessor and standalone string-pool fallback for collector-only tests. |
| `src/vm/state/lua_state.cpp` | No-argument `LuaState::create()` / `newState()` remain legacy singleton shims; `newState(EngineContext&)` is the isolated path. |
| `src/vm/vm*.cpp` | Service-less VM overloads preserve older call sites; service-taking overloads are the modern path. |
| `src/compiler/codegen/codegen.cpp` | The `StringPool*` constructor keeps old tests and tools source-compatible; service-taking constructors are preferred. |
| `src/core/metatable.cpp` | Overloads without `LuaState*` fall back to singleton global metatables for legacy helpers. |
| `src/gc/gc_finalize.cpp`, `src/gc/gc_weak.cpp` | Collector instances without an owning `GlobalState` use the singleton only for legacy/test collectors. |

New production or integration harnesses should avoid this list. As of 2026-06-01, the Lua 5.1 official-suite C++ harness also creates a fresh `EngineContext` per gate, so staged official tests do not rely on `GlobalState::getInstance()` cleanup between cases.
