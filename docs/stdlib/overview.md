---
status: current
verified_against: src/lib/lib_catalog.cpp; src/lib/lib_manager.cpp; src/lib/lib_registry.hpp; src/lib/baselib.cpp; src/gc/gc_strategy.hpp; src/lib/mathlib.cpp; src/lib/iolib.cpp; src/lib/stringlib.cpp; src/lib/tablelib.cpp; src/lib/oslib.cpp; src/lib/coroutinelib.cpp; src/lib/debuglib.cpp; src/lib/packagelib.cpp; tests/unit/stdlib/test_lib_catalog.cpp; tests/unit/stdlib/test_baselib.cpp; tests/unit/stdlib/; tests/unit/gc/test_gc.cpp
last_checked: 2026-05-31
applies_to: current standard library implementation overview
---

# Standard Library Overview

Standard libraries are registered through `StandardLibrary` and the catalog in `src/lib/lib_catalog.cpp`.

Current catalog order:

1. `base`
2. `math`
3. `io`
4. `string`
5. `table`
6. `os`
7. `coroutine`
8. `debug`
9. `package`

`StandardLibrary::openAll()` iterates this catalog. Single-library loading should use `StandardLibrary::openCatalogLibrary(L, "<id>")`; the older `openMath()` / `openPackage()` convenience wrappers are deprecated compatibility shims over the same path.

PR-73 evaluated but intentionally rejected a `LibRegistrar` self-registration layer. The current explicit `constexpr` catalog stays as the only standard-library assembly source of truth because it makes load order visible, keeps tests direct, and avoids static-initialization / MSVC linker keep-alive surprises.

## Registration Model

Each library generally follows this shape:

```cpp
class XxxLibModule : public LibModule {
public:
    void registerFunctions(LuaState* L) override;
    void initialize(LuaState* L) override;
};
```

`FunctionRegistrar` provides fluent helpers for global functions and table functions.

## Library Files

| Library | Files | Notes |
|---|---|---|
| base | `baselib.hpp/.cpp` | Global functions, `_G`, `_VERSION`, `pcall`, `xpcall`, loading helpers including stdin `loadfile/dofile`, environment helpers, GC facade including `collectgarbage("strategy")` and stateful `setpause` / `setstepmul` controls |
| math | `mathlib.hpp/.cpp` | Math functions and constants |
| io | `iolib.hpp/.cpp` | File userdata, `io` table, file methods, `io.lines/file:lines` read formats |
| string | `stringlib.hpp/.cpp` | String operations, pattern functions, `string.dump` |
| table | `tablelib.hpp/.cpp` | Insert/remove/sort/concat plus 5.2-style convenience helpers |
| os | `oslib.hpp/.cpp` | Date/time, environment, command, remove/rename/tmpname |
| coroutine | `coroutinelib.hpp/.cpp` | `create`, `resume`, `yield`, `status`, `running`, `wrap` |
| debug | `debuglib.hpp/.cpp` | Stack/upvalue/debug hook/traceback surface |
| package | `packagelib.hpp/.cpp` | `require`, `module`, `package.*`, Lua and C loader paths |

## Known Compatibility Gaps

The current project tests are green, but this does not mean full official Lua 5.1.5 compatibility. Known high-value gaps include:

- official `testC` / `ltests.c` helper coverage for `api.lua` and `code.lua`
- official Lua 5.1 binary chunk compatibility; current dump/load is project-local
- byte-for-byte error/traceback text compatibility in uncommon paths
- debug library extreme stack-level and traceback formatting details
- exact Lua 5.1 GC work accounting and `IncrementalGC` strategy semantics; `collectgarbage("step")` has phased work, but `collectgarbage("strategy", "incremental")` still selects an equivalent teaching placeholder for full `collect()`
- more entry-point migration to owning `EngineContext`; the owning context exists, but singleton compatibility entry points remain

## Verification

```powershell
bin\lua_test.exe --filter "Standard Library Catalog"
bin\lua_test.exe --filter "Base Library"
bin\lua_test.exe --filter "Package Library"
```
