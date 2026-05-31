---
status: current
verified_against: src/gc/garbage_collector.hpp; src/gc/garbage_collector.cpp; src/gc/gc_strategy.hpp; src/gc/gc_strategy.cpp; src/gc/gc_mark.cpp; src/gc/gc_sweep.cpp; src/gc/gc_finalize.cpp; src/core/string_pool.cpp; src/core/gc_object.hpp; src/core/table.cpp; src/core/function.cpp; src/core/upvalue.cpp; src/core/userdata.cpp; src/core/thread.cpp; src/vm/state/global_state.cpp; tests/unit/gc/test_gc.cpp
last_checked: 2026-05-31
applies_to: current garbage collector implementation
---

# Garbage Collection

The current collector is a GlobalState-backed collector exposed through `RuntimeServices::gc`, `EngineContext::gc()`, and `GlobalState::getGC()`. Collection runs through a `GCStrategy` boundary. The default `MarkSweepGC` strategy owns the real stop-the-world mark-sweep algorithm; `collectgarbage("step")` uses an internal phased scheduler, while `IncrementalGC` is still a teaching placeholder for full `collect()` calls. The legacy `GarbageCollector::getInstance()` still exists as a deprecated compatibility shim.

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
- `IncrementalGC`: a placeholder strategy with equivalent reachability behavior for full `collect()` calls

The active strategy can be queried through `GarbageCollector::getStrategyName()`.
`collectgarbage("strategy")` returns the current strategy name, and `collectgarbage("strategy", "mark-sweep" | "incremental")` switches the active boundary.
The incremental strategy is intentionally conservative today: it reuses mark-sweep collection for full `collect()` calls while `collectgarbage("step")` exercises the collector's phased pause/propagate/atomic/sweep/finalize path.

`collectgarbage("setpause", n)` and `collectgarbage("setstepmul", n)` now store real collector parameters and return the previous values. `pause` influences the automatic-GC threshold after an automatic collection; `stepmul` scales the `step` work budget. These controls are compatibility surfaces for the current collector, though the work accounting is still a project-local approximation rather than Lua 5.1's byte-for-byte debt model.

## Incremental Step Flow

`collectgarbage("step")` drives these states:

1. `pause`: wait until allocation debt reaches the next threshold.
2. `propagate`: scan a bounded number of gray objects per step.
3. `atomic`: rescan roots, process weak tables, prepare finalizers, and close marking.
4. `sweep`: sweep the object list in bounded slices and remove dead interned strings from the owning `StringPool`.
5. `finalize`: run queued userdata finalizers outside arbitrary VM allocation sites.

Key invariants:

- no black object may point to a white object that can be swept in the same cycle
- weak keys and weak values are cleared after marking and before object deletion
- finalizable userdata are revived for one cycle, marked again, and finalized at most once
- resurrected userdata and objects reachable from finalizers survive the current cycle
- roots include registry, primitive metatables, metamethod names, memory error string, current/main thread stacks, running coroutine, debug hook, open upvalues, and pending finalizers

Current implementation status:

- `collect()` remains a complete mark-sweep cycle through the active `GCStrategy`
- `step()` starts marking, propagates gray objects by budget, performs atomic weak/finalizer preparation, sweeps by cursor, and runs finalizers at cycle end
- conservative write barriers preserve the tricolor invariant for current mutation points
- `setpause` / `setstepmul` are stateful and affect automatic thresholds / step budget
- debug-hook-triggered manual collection preserves interrupted Lua register windows without keeping ordinary dead temporaries alive

## Write Barriers

`GarbageCollector::writeBarrier(owner, child)` and its `Value` overload implement a conservative forward barrier. When a black owner receives a white child owned by the same collector, the child is marked and its graph is immediately propagated. This is more eager than Lua 5.1's incremental collector, but it protects the same correctness invariant while phase cursors are still pending.

Covered mutation sites:

- `Table::set`, `Table::setArray`, and `Table::setMetatable`
- `Userdata::setMetatable`
- `Function::setEnv`, `Function::setUpvalue`, and `Function::addUpvalue`
- `Upvalue::setValue` and `Upvalue::close`
- `GlobalState::setMetatable` and `GlobalState::setRunningThread` through a root barrier

`tests/unit/gc/test_gc.cpp` has regression probes that make owners black, attach white child graphs through these paths, then sweep to ensure the new references are preserved.

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

- `IncrementalGC` does not change full `collect()` behavior; it preserves mark-sweep semantics behind a strategy boundary.
- `collectgarbage("step")` has bounded phases, but its work units are a project-local approximation rather than Lua 5.1's exact debt accounting.
- The deprecated legacy collector singleton remains for compatibility.
- Fixed objects are preserved by `clearAll()`; this matches current test/shutdown behavior but is not a general-purpose heap teardown API.

## Verification

```powershell
bin\lua_test.exe --filter "GC"
bin\lua_test.exe --filter "collectgarbage"
```
