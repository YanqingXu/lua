---
status: current
verified_against: README.md; lua.slnx; lua.vcxproj; lua_app.vcxproj; lua_test.vcxproj; lua_bytecode.vcxproj; docs/refactor_expdesc_pr_checklist.md
last_checked: 2026-05-18
applies_to: current repository facts and contributor-facing workflow
---

# Project Status

This file is the repository's single source of truth for externally visible project facts. README and development docs should point here instead of duplicating build, test, and compiler-pipeline status in multiple places.

## Current Build Path

- Primary platform: Windows.
- Primary IDE/toolchain: Visual Studio / MSBuild.
- Primary solution entry: `lua.slnx`.
- Active project files:
  - `lua.vcxproj`: core static library.
  - `lua_app.vcxproj`: interpreter / REPL executable.
  - `lua_test.vcxproj`: unit test executable.
  - `lua_bytecode.vcxproj`: bytecode inspection executable.
- MSVC platform toolset recorded in project files: `v145`.
- Current C++ standard setting in project files: mixed `stdcpp20` and `stdcpp23`; the active x64 Debug configuration currently uses `stdcpp23`.

## Planned Build Path

- CMake and CTest are architectural goals; they are planned and not current.
- There is currently no repository-root `CMakeLists.txt`.
- Any CMake/CTest instructions in contributor-facing docs must be marked as planned/future work until a real CMake entry exists.

## Test Status

- Test framework: custom lightweight C++ test framework vendored under `lua_test/include/test_framework` and adapted by `tests/unit/framework`.
- Latest documented test count in README: 414 registered tests, 1634 assertion results, 0 failures.
- This number describes the project test runner result. It is not a Lua 5.1.5 compatibility percentage.

## Compiler Pipeline Status

- `ExprDesc` and `ExprKind` have been removed from production compiler sources.
- Current bytecode-generation documentation should explain this pipeline:

```text
AST
  -> SymbolRef
  -> ValueResult / CondResult / LValueRef / CallResultInfo
  -> Proto
```

- Historical `ExprDesc` notes belong under `docs/history/exprdesc.md`.
- Drift guard:

```powershell
rg "ExprDesc|ExprKind|expdesc" src/compiler
```

The command above must return no matches for production compiler sources.

## Documentation Status Rules

Every core documentation file should begin with:

```yaml
---
status: current|historical|planned
verified_against: <files or commands used as evidence>
last_checked: YYYY-MM-DD
applies_to: <scope>
---
```

Use `current` only when the document describes the implementation that exists in this repository today. Use `historical` for completed refactor notes and removed designs. Use `planned` for future architecture that is not yet implemented.
