# PR-11 VM Handler Table Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Introduce the first command-handler table for VM opcode execution while preserving `SwitchDispatch` as the default strategy.

**Architecture:** Add `vm_handlers.hpp/.cpp` with an `OpExecutionContext`, `OpHandler` function pointer type, a `NUM_OPCODES`-sized table, and handlers for the low-risk data movement opcodes. `SwitchDispatch` delegates `MOVE`, `LOADK`, `LOADBOOL`, and `LOADNIL` to the table; all other opcodes stay in the existing switch.

**Tech Stack:** C++23, existing VM helper slices, existing lightweight unit test framework, CMake and Visual Studio projects.

---

### Task 1: Handler Table Contract Tests

**Files:**
- Modify: `tests/unit/vm/test_vm_dispatch.cpp`

- [ ] **Step 1: Add a red test for table shape**

Include `vm/vm_handlers.hpp` and assert that `handlerTable().size() == NUM_OPCODES` and each entry's opcode matches its array index.

- [ ] **Step 2: Add a red test for initial registrations**

Assert `hasHandler()` is true for `MOVE`, `LOADK`, `LOADBOOL`, and `LOADNIL`, and false for a still-switch-only opcode such as `CALL`.

- [ ] **Step 3: Add a red test for direct handler execution**

Build a small `OpExecutionContext` over local registers and a local `Proto`; run handlers for the four data movement opcodes and assert register/pc effects.

- [ ] **Step 4: Run the targeted build**

Run:

```powershell
& 'D:\VS2026\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build\cmake --target lua_test --config Debug
```

Expected: compile failure because `vm/vm_handlers.hpp` is not present.

### Task 2: Handler Table Implementation

**Files:**
- Create: `src/vm/vm_handlers.hpp`
- Create: `src/vm/vm_handlers.cpp`
- Modify: `src/vm/vm.cpp`
- Modify: `CMakeLists.txt`
- Modify: `lua.vcxproj`
- Modify: `lua.vcxproj.filters`

- [ ] **Step 1: Define handler context and API**

Expose `OpExecutionContext`, `HandlerStatus`, `OpHandler`, `HandlerEntry`, `handlerTable()`, `handlerFor()`, `hasHandler()`, and `runHandler()`.

- [ ] **Step 2: Implement four data movement handlers**

Move the exact behavior for `MOVE`, `LOADK`, `LOADBOOL`, and `LOADNIL` into handler functions.

- [ ] **Step 3: Delegate matching switch cases**

In `SwitchDispatch::run()`, replace the four inline data movement bodies with one `runHandler()` call.

- [ ] **Step 4: Wire build systems**

Add `src/vm/vm_handlers.cpp` and `src/vm/vm_handlers.hpp` to CMake and Visual Studio project/filter files.

### Task 3: Verification

**Files:**
- Verify all modified code and docs.

- [ ] **Step 1: Run targeted VM dispatch tests**

Run:

```powershell
.\build\cmake\Debug\lua_test.exe --filter "VM Dispatch"
```

- [ ] **Step 2: Run full quality gate subset**

Run CMake full build, full `lua_test`, CTest, root VS builds, root `bin\lua_test.exe`, and `git diff --check`.
