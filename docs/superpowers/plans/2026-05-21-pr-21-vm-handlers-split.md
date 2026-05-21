# PR-21 VM Handlers Split Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Split the monolithic `src/vm/vm_handlers.cpp` into opcode-family source files under `src/vm/vm_handlers/`.

**Architecture:** Keep the public `vm_handlers.hpp` API stable. Add an internal `vm_handler_utils.hpp` for shared handler helpers and registration declarations, keep `src/vm/vm_handlers.cpp` as the registry/runtime entry point, and move concrete handler bodies into focused family files.

**Tech Stack:** C++23, existing VM command-handler table, existing CMake and Visual Studio project files.

---

### Task 1: Establish Baseline

**Files:**
- Read: `src/vm/vm_handlers.cpp`
- Read: `src/vm/vm_handlers.hpp`
- Read: `CMakeLists.txt`
- Read: `lua.vcxproj`
- Read: `lua.vcxproj.filters`

- [x] **Step 1: Run focused characterization test**

Run:

```powershell
.\build\cmake\Debug\lua_test.exe --filter "VM Dispatch"
```

Expected: existing VM dispatch behavior is green before any file movement.

### Task 2: Split Handler Implementations

**Files:**
- Create: `src/vm/vm_handlers/vm_handler_utils.hpp`
- Create: `src/vm/vm_handlers/vm_handlers_data.cpp`
- Create: `src/vm/vm_handlers/vm_handlers_global_upvalue.cpp`
- Create: `src/vm/vm_handlers/vm_handlers_table.cpp`
- Create: `src/vm/vm_handlers/vm_handlers_arith.cpp`
- Create: `src/vm/vm_handlers/vm_handlers_unary.cpp`
- Create: `src/vm/vm_handlers/vm_handlers_branch.cpp`
- Create: `src/vm/vm_handlers/vm_handlers_loop.cpp`
- Create: `src/vm/vm_handlers/vm_handlers_closure.cpp`
- Create: `src/vm/vm_handlers/vm_handlers_call.cpp`
- Modify: `src/vm/vm_handlers.cpp`

- [x] **Step 1: Add internal helper header**

Move shared helper functions into inline internal helpers:
- `opcodeIndex`
- `requireState`
- `requireFunction`
- `requireProto`
- `refreshBase`
- `getRK`

Declare one `register*Handlers(HandlerTable&)` function per opcode family.

- [x] **Step 2: Move concrete handlers by family**

Move handler bodies into the matching family file:
- data: `MOVE`, `LOADK`, `LOADBOOL`, `LOADNIL`
- global/upvalue: `GETGLOBAL`, `SETGLOBAL`, `GETUPVAL`, `SETUPVAL`
- table: `GETTABLE`, `SETTABLE`, `NEWTABLE`, `SELF`, `SETLIST`
- arithmetic: `ADD`, `SUB`, `MUL`, `DIV`, `MOD`, `POW`
- unary: `UNM`, `NOT`, `LEN`, `CONCAT`
- branch: `JMP`, `EQ`, `LT`, `LE`, `TEST`, `TESTSET`
- loop: `CLOSE`, `FORLOOP`, `FORPREP`, `TFORLOOP`
- closure: `CLOSURE`, `VARARG`
- call: `CALL`, `TAILCALL`, `RETURN`

- [x] **Step 3: Keep registry and runtime entry points in `vm_handlers.cpp`**

Keep only:
- `makeHandlerTable()`
- `handlerTable()`
- `handlerFor()`
- `hasHandler()`
- `runHandler()`

`makeHandlerTable()` should initialize metadata for every opcode, then call each family registration function.

### Task 3: Update Build Files

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `lua.vcxproj`
- Modify: `lua.vcxproj.filters`

- [x] **Step 1: Add new source files to CMake**

Add all `src/vm/vm_handlers/*.cpp` files to `LUA_CORE_SOURCES`.

- [x] **Step 2: Add new source files to Visual Studio**

Add all new `.cpp` files to `lua.vcxproj` and `lua.vcxproj.filters`.

- [x] **Step 3: Add internal helper header to Visual Studio filters**

Add `src\vm\vm_handlers\vm_handler_utils.hpp` to `lua.vcxproj` and `lua.vcxproj.filters`.

### Task 4: Verify PR-21

**Files:**
- Verify: source layout, build output, tests

- [x] **Step 1: Build focused target**

Run:

```powershell
& 'D:\VS2026\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build\cmake --target lua_test --config Debug
```

- [x] **Step 2: Run focused VM checks**

Run:

```powershell
.\build\cmake\Debug\lua_test.exe --filter "VM Dispatch"
.\build\cmake\Debug\lua_test.exe --filter "Call Pipeline"
.\build\cmake\Debug\lua_test.exe --filter "coroutine"
```

- [x] **Step 3: Run full checks**

Run:

```powershell
.\build\cmake\Debug\lua_test.exe
& 'D:\VS2026\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir build\cmake -C Debug --output-on-failure
& 'D:\VS2026\MSBuild\Current\Bin\MSBuild.exe' lua_app.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m
& 'D:\VS2026\MSBuild\Current\Bin\MSBuild.exe' lua_test.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m
.\bin\lua_test.exe
.\tools\run_quality_gate.ps1
git diff --check
```
