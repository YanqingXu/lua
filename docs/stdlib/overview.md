---
status: current
verified_against: src/lib/lib_catalog.cpp; src/lib/lib_manager.cpp; src/lib/lib_registry.hpp; src/lib/baselib.cpp; src/gc/gc_strategy.hpp; src/lib/mathlib.cpp; src/lib/iolib.cpp; src/lib/stringlib.cpp; src/lib/tablelib.cpp; src/lib/oslib.cpp; src/lib/coroutinelib.cpp; src/lib/debuglib.cpp; src/lib/packagelib.cpp; tests/unit/stdlib/test_lib_catalog.cpp; tests/unit/stdlib/; tests/unit/gc/test_gc.cpp
last_checked: 2026-05-23
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
| base | `baselib.hpp/.cpp` | Global functions, `_G`, `_VERSION`, `pcall`, `xpcall`, loading helpers, environment helpers, GC facade including `collectgarbage("strategy")` |
| math | `mathlib.hpp/.cpp` | Math functions and constants |
| io | `iolib.hpp/.cpp` | File userdata, `io` table, file methods |
| string | `stringlib.hpp/.cpp` | String operations, pattern functions, `string.dump` |
| table | `tablelib.hpp/.cpp` | Insert/remove/sort/concat plus 5.2-style convenience helpers |
| os | `oslib.hpp/.cpp` | Date/time, environment, command, remove/rename/tmpname |
| coroutine | `coroutinelib.hpp/.cpp` | `create`, `resume`, `yield`, `status`, `running`, `wrap` |
| debug | `debuglib.hpp/.cpp` | Stack/upvalue/debug hook/traceback surface |
| package | `packagelib.hpp/.cpp` | `require`, `module`, `package.*`, Lua and C loader paths |

## Known Compatibility Gaps

The current project tests are green, but this does not mean full official Lua 5.1.5 compatibility. Known high-value gaps include:

- `error(level)` source location formatting and arbitrary error object behavior
- `xpcall` error handler semantics
- `newproxy`
- stdin behavior for no-argument `loadfile()` / `dofile()`
- `debug.getfenv` / `setfenv` stack-level and thread environment details
- debug library operations for primitive type metatables
- `io.lines` and `file:lines` format arguments
- `os.remove` / `os.rename` failure return details
- `table.concat` strict element type behavior
- real incremental `collectgarbage` scheduling and write barriers; `collectgarbage("strategy", "incremental")` currently selects an equivalent teaching placeholder

## Verification

```powershell
bin\lua_test.exe --filter "Standard Library Catalog"
bin\lua_test.exe --filter "Base Library"
bin\lua_test.exe --filter "Package Library"
```
