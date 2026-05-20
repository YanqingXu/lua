# PR-08: GarbageCollector De-Singletonization Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move `GarbageCollector` ownership into `GlobalState` while keeping `GarbageCollector::getInstance()` as a temporary compatibility shim.

**Architecture:** `GlobalState` owns a concrete `GarbageCollector` member and wires the singleton `StringPool` to that collector during initialization. `GCObject` tracks its owning collector so object destruction and re-registration operate on the correct instance instead of the legacy shim. Existing runtime entry points continue to reach GC through `GlobalState::getGC()` and `RuntimeServices`.

**Tech Stack:** C++23, existing lightweight unit test framework, CMake and Visual Studio project files.

---

### Task 1: Ownership Contract Tests

**Files:**
- Modify: `tests/unit/gc/test_gc.cpp`
- Modify: `tests/unit/vm/test_runtime_services.cpp`
- Modify: `tests/unit/vm/test_vm_core.cpp`

- [ ] **Step 1: Add failing GC instance isolation test**

Add a test that constructs a local `GarbageCollector`, registers an object, deletes that object, and verifies only the owning collector changes.

- [ ] **Step 2: Update runtime/global-state expectations**

Change tests so `GlobalState::getGC()` is expected to be distinct from the legacy `GarbageCollector::getInstance()` shim, while `RuntimeServices::fromSingletons().gc` still equals `GlobalState::getInstance().getGC()`.

- [ ] **Step 3: Run targeted tests to verify red**

Run: `D:\VS2026\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build\cmake --target lua_test --config Debug`

Expected: compile failure because `GarbageCollector` constructor is still private, or runtime failure on the old singleton equality expectation.

### Task 2: Explicit GC Ownership

**Files:**
- Modify: `src/gc/garbage_collector.hpp`
- Modify: `src/gc/garbage_collector.cpp`
- Modify: `src/gc/gc_sweep.cpp`
- Modify: `src/core/gc_object.hpp`
- Modify: `src/core/gc_object.cpp`

- [ ] **Step 1: Make `GarbageCollector` constructible**

Move the default constructor to the public API and keep copy/assignment deleted. Keep `getInstance()` as a documented compatibility shim.

- [ ] **Step 2: Track object ownership**

Add an owner pointer to `GCObject`. `GarbageCollector::registerObject()` sets it; `unregisterObject()`, sweep, and `clearAll()` clear it when unlinking or deleting objects.

- [ ] **Step 3: Route `GCObject` destruction through its owner**

Change `GCObject::~GCObject()` to call `owner->unregisterObject(this)` only when the object has an owner.

### Task 3: GlobalState-Owned Collector

**Files:**
- Modify: `src/vm/global_state.hpp`
- Modify: `src/vm/global_state.cpp`
- Modify: `src/core/string_pool.hpp`
- Modify: `src/core/string_pool.cpp`
- Modify: `src/gc/gc_mark.cpp`
- Modify: `src/gc/gc_finalize.cpp`
- Modify: `src/gc/gc_weak.cpp`
- Modify: `src/runtime/runtime_services.hpp`

- [ ] **Step 1: Store GC by value in `GlobalState`**

Replace `GarbageCollector& gc_` with `GarbageCollector gc_`; initialize it directly; keep `getGC()` returning a reference.

- [ ] **Step 2: Wire `StringPool` to the owned collector**

Add `StringPool::setGarbageCollector()` and make `intern()` register new strings with the configured collector, falling back to the compatibility shim only when no collector is configured.

- [ ] **Step 3: Let GC know its owning global state**

Store an optional `GlobalState*` inside `GarbageCollector` for metatable/finalizer lookups and owner-root marking. Standalone collectors keep this pointer null.

### Task 4: Test Migration and Verification

**Files:**
- Modify: `tests/unit/gc/test_gc.cpp`
- Modify: `tests/unit/vm/test_lua_state_init.cpp`

- [ ] **Step 1: Move stateful GC tests to `GlobalState::getGC()`**

Tests that create `LuaState` should use the state's collector. Standalone data-structure tests should use local collectors.

- [ ] **Step 2: Build and run targeted tests**

Run: `build\cmake\Debug\lua_test.exe --filter "GC"`

Expected: all selected GC tests pass.

- [ ] **Step 3: Run full gates**

Run CMake build, full `lua_test`, `ctest`, root `lua_test.vcxproj`, root `lua_app.vcxproj`, and `git diff --check`.
