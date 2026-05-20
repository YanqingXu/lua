# PR-09: DispatchStrategy Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Introduce a VM dispatch strategy abstraction while keeping the existing switch-based interpreter as the default behavior.

**Architecture:** Add a small `VMContext` value and `DispatchStrategy` interface under `src/vm/`. `RuntimeServices` carries an optional strategy pointer; `VM::executeProto()` chooses that injected strategy or the process-wide default `SwitchDispatch`. The current `while + switch(op)` loop moves behind `SwitchDispatch::run()` without changing opcode semantics.

**Tech Stack:** C++23, existing VM helper slices, existing lightweight unit test framework, CMake and Visual Studio projects.

---

### Task 1: Strategy Contract Tests

**Files:**
- Modify: `tests/unit/vm/test_vm_dispatch.cpp`

- [ ] **Step 1: Add a default strategy contract test**

Assert that `VM::defaultDispatchStrategy().name()` returns `"switch"`.

- [ ] **Step 2: Add an injected strategy test**

Define a recording `DispatchStrategy` in the test, set `RuntimeServices::dispatchStrategy`, run `VM::execute()`, and assert the recording strategy receives the expected `RuntimeServices`, `LuaState`, `Proto`, and call depth.

- [ ] **Step 3: Run build to verify red**

Run: `D:\VS2026\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build\cmake --target lua_test --config Debug`

Expected: compile failure because `vm_dispatch_strategy.hpp` and `RuntimeServices::dispatchStrategy` do not exist.

### Task 2: Strategy Types and Build Wiring

**Files:**
- Create: `src/vm/vm_dispatch_strategy.hpp`
- Create: `src/vm/vm_dispatch_strategy.cpp`
- Modify: `src/runtime/runtime_services.hpp`
- Modify: `CMakeLists.txt`
- Modify: `lua.vcxproj`
- Modify: `lua.vcxproj.filters`

- [ ] **Step 1: Add `VMContext` and `DispatchStrategy` declarations**

`VMContext` stores `RuntimeServices&`, `LuaState*`, `Proto*`, and `i32 nexeccalls`.

- [ ] **Step 2: Add default `SwitchDispatch` instance**

Expose `DispatchStrategy& defaultDispatchStrategy() noexcept` from `vm_dispatch_strategy.cpp`.

- [ ] **Step 3: Add optional strategy pointer to `RuntimeServices`**

Existing constructors default to `nullptr` so all current call sites keep default behavior.

- [ ] **Step 4: Wire the new `.cpp` and header into CMake and Visual Studio**

Add `src/vm/vm_dispatch_strategy.cpp` to the core source lists and `src/vm/vm_dispatch_strategy.hpp` to the Visual Studio header list/filter.

### Task 3: Move Switch Loop Behind Strategy

**Files:**
- Modify: `src/vm/vm.cpp`

- [ ] **Step 1: Include strategy header**

Add `#include "vm/vm_dispatch_strategy.hpp"`.

- [ ] **Step 2: Change `executeProto(RuntimeServices&, ...)` to select a strategy**

Build `VMContext context{services, L, proto, nexeccalls}` and call the injected strategy or `defaultDispatchStrategy()`.

- [ ] **Step 3: Move current switch body into `SwitchDispatch::run()`**

Preserve the current local variables, hooks, trace emission, reentry label, and return/yield behavior.

### Task 4: Verification

**Files:**
- No additional files.

- [ ] **Step 1: Run targeted dispatch tests**

Run: `build\cmake\Debug\lua_test.exe --filter "VM Dispatch"`

- [ ] **Step 2: Run full tests and project builds**

Run CMake full build, full `lua_test`, `ctest`, root `lua_test.vcxproj`, root `lua_app.vcxproj`, and `git diff --check`.
