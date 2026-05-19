---
status: current
verified_against: README.md; lua.slnx; lua.vcxproj; lua_app.vcxproj; lua_test.vcxproj; lua_bytecode.vcxproj; CMakeLists.txt; tools/run_cmake_smoke.ps1; tests/unit/framework/test_runner.cpp; tests/unit/compiler/test_codegen_state.cpp; docs/START_HERE.md; docs/glossary.md; docs/walkthroughs/index.md; examples/README.md; src/runtime/runtime_services.hpp; src/compiler/codegen.cpp; src/compiler/codegen_binding.cpp; src/compiler/codegen_expr.cpp; src/compiler/codegen_jump.cpp; src/compiler/codegen_stmt.cpp; src/compiler/codegen_state.hpp
last_checked: 2026-05-19
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

## Secondary Build Path

- CMake and CTest now exist as a secondary build/test path, not a replacement for the primary Visual Studio/MSBuild workflow.
- Repository-root `CMakeLists.txt` builds `lua_core`, `lua_app`, `lua_bytecode`, and `lua_test`.
- Local secondary smoke entry: `tools/run_cmake_smoke.ps1`.
- CTest currently registers one unit-test executable test plus the example Lua scripts under `examples/`.

## Planned Build Path Work

- Broaden CMake validation beyond the current Windows smoke path before treating it as a cross-platform contract.
- Add install/export or packaging rules only after the source target boundaries stabilize.

## Test Status

- Test framework: custom lightweight C++ test framework vendored under `lua_test/include/test_framework` and adapted by `tests/unit/framework`.
- Latest verified test count: 425 registered tests, 1738 assertion results, 0 failures.
- `bin\lua_test.exe` supports `--list`, `--filter <suite-or-name>`, and `--report=junit`.
- These numbers describe the project test runner result. They are not a Lua 5.1.5 compatibility percentage.

## Quality Gate Status

- Formatting configuration: `.clang-format`, based on LLVM with repository-specific width/include choices.
- Static analysis configuration: `.clang-tidy`, currently limited to conservative `bugprone-*`, `performance-*`, `portability-*`, and selected `readability-*` checks.
- Local quality entry: `tools/run_quality_gate.ps1`.
- Quality-gate self-test: `tools/test_quality_gate.ps1`.
- Documentation drift guard: `tools/check_doc_drift.ps1`.
- CI entry: `.github/workflows/ci.yml` on GitHub Actions, using Windows/MSBuild as the current primary workflow.
- The quality gate is intentionally incremental: local formatting defaults to changed source files, missing local `clang-format` or `clang-tidy` tools are reported as skips by the local script, while MSBuild and unit tests remain the canonical Windows validation path.

## Compiler Pipeline Status

- `ExprDesc` and `ExprKind` have been removed from production compiler sources.
- `CodeGenerator` keeps its public API in `src/compiler/codegen.hpp`, while its implementation is physically split across `codegen.cpp`, `codegen_binding.cpp`, `codegen_expr.cpp`, `codegen_jump.cpp`, and `codegen_stmt.cpp`.
- `src/compiler/codegen_state.hpp` centralizes the mutable generation state shared by those implementation slices, including the current `Proto`, program counter, line number, register allocator, local scope, block manager, and upvalue context.
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

## Runtime Boundary Status

- `src/runtime/runtime_services.hpp` defines `RuntimeServices` as the current explicit compatibility layer over `GlobalState`, `StringPool`, and `GarbageCollector`.
- `CodeGenerator`, `Parser`, `LuaState`, and `VM` expose context-aware construction/execution overloads while retaining singleton-backed compatibility overloads.
- `src/main.cpp`, `src/repl.cpp`, and `src/bytecode/bytecode_main.cpp` use `RuntimeServices` for the first compiler/VM entry-point slice.

## Learning Path Status

- `docs/START_HERE.md` is the first-read entry point for new contributors.
- `docs/glossary.md` maps Lua terminology to current repository types and files.
- `docs/walkthroughs/index.md` organizes important test suites into a guided compiler, VM, runtime, and standard-library reading path.
- `examples/` contains small Lua scripts that can be run with `bin\lua_app.exe` after building the app target.

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
