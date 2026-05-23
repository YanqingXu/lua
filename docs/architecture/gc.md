---
status: current
verified_against: src/gc/garbage_collector.hpp; src/gc/garbage_collector.cpp; src/gc/gc_strategy.hpp; src/gc/gc_strategy.cpp; src/gc/gc_sweep.cpp; src/core/string_pool.cpp; src/core/gc_object.hpp; src/core/table.cpp; src/core/function.cpp; src/core/upvalue.cpp; src/core/thread.cpp; src/vm/state/global_state.cpp; tests/unit/gc/test_gc.cpp
last_checked: 2026-05-23
applies_to: current garbage collector implementation
---

# Garbage Collection

The current collector is a GlobalState-backed collector exposed through `RuntimeServices::gc` and `GlobalState::getGC()`. Collection now runs through a `GCStrategy` boundary. The default `MarkSweepGC` strategy owns the real stop-the-world mark-sweep algorithm; `IncrementalGC` is a teaching placeholder that delegates to the same phases until write barriers and scheduling exist. The legacy `GarbageCollector::getInstance()` still exists as a deprecated compatibility shim.

## Managed Objects

Every collectable object inherits from `GCObject` and implements:

- `mark(GarbageCollector&)`
- `getSize()`

Current managed types are `GCString`, `Table`, `Proto`, `Function`, `Upvalue`, `Userdata`, and `Thread`.

Objects are linked through the collector's `allObjects_` list. Most object constructors do not automatically register themselves; creation sites call `registerObject()` when the object should join GC ownership. `GCString` objects are registered by `StringPool`.

## Collection Flow

`GarbageCollector::collect(LuaState* currentState)` creates a `GCContext` and delegates to the active `GCStrategy`. The current mark-sweep implementation runs:

1. Clear transient mark state and mark roots.
2. Mark explicit roots and `GlobalState` roots.
3. Propagate gray objects by calling each object's `mark()`.
4. Prepare unreachable userdata with `__gc` finalizers.
5. Clear weak table entries before sweeping.
6. Sweep unreachable objects with an explicit `StringPool&` so dead `GCString` entries are removed from the same interning table.
7. Run queued finalizers when a current `LuaState` is available.

`collectgarbage("collect")` enters this path through the base library.

## Strategy Boundary

`src/gc/gc_strategy.hpp` defines:

- `GCContext`: the collector, explicit `StringPool&`, and optional current `LuaState`
- `GCStrategy`: the abstract collection strategy interface
- `MarkSweepGC`: the default implementation
- `IncrementalGC`: a placeholder strategy with equivalent reachability behavior

The active strategy can be queried through `GarbageCollector::getStrategyName()`.
`collectgarbage("strategy")` returns the current strategy name, and `collectgarbage("strategy", "mark-sweep" | "incremental")` switches the active boundary.
The incremental strategy is intentionally conservative today: it reuses mark-sweep collection to keep correctness stable while making the future incremental algorithm visible in code and tests.

## Roots

The root set includes:

- explicit roots registered with `addRoot`
- registry table
- memory error string
- fixed metamethod names and reserved strings
- primitive type metatables
- current state stack and call frames
- main thread stack and call frames
- running coroutine thread
- debug hook function
- pending finalizer userdata

`GlobalState::markRoots()` coordinates the shared runtime roots.

## Weak Tables

`Table` supports weak keys and weak values through metatable field `__mode`:

- `"k"`: weak keys
- `"v"`: weak values
- `"kv"`: weak keys and values

During marking, `GarbageCollector::markTable()` records weak tables and avoids marking weak sides. Before sweep, `clearWeakTableEntries()` removes entries whose weak key or value is about to die.

## Userdata Finalizers

`Userdata` can have a `__gc` finalizer in its metatable. Unreachable userdata with finalizers are revived for one collection cycle, queued, and finalized through `runFinalizers()`.

Current behavior:

- the finalizer receives the userdata as argument
- a finalizer should not be called twice for the same userdata
- finalizer errors are contained so a single failing finalizer does not abort the whole collection cycle

## Known Limits

- `collectgarbage("stop")`, `"restart"`, `"step"`, `"setpause"`, and `"setstepmul"` are compatibility surfaces, not a real incremental-control implementation.
- The main collector is still GlobalState-backed; the deprecated legacy collector singleton remains for compatibility.
- `IncrementalGC` does not yet perform incremental scheduling; it preserves mark-sweep behavior behind a strategy boundary.
- Write barriers are not a current feature because real incremental collection is not implemented yet.

## Verification

```powershell
bin\lua_test.exe --filter "GC"
bin\lua_test.exe --filter "collectgarbage"
```
