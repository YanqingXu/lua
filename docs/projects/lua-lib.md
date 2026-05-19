---
status: current
verified_against: lua.vcxproj; CMakeLists.txt; src/common/; src/core/; src/compiler/; src/vm/; src/gc/; src/lib/; src/runtime/runtime_services.hpp
last_checked: 2026-05-19
applies_to: lua.vcxproj / lua_core static library
---

# lua.lib / lua_core

`lua.vcxproj` builds the core static library used by the interpreter app, bytecode tool, and test runner. In CMake, the equivalent target is `lua_core`.

## Responsibilities

- Lua value and object model
- string pool
- table, function, upvalue, userdata, thread objects
- garbage collector
- lexer, parser, AST, bytecode generation
- VM execution helpers
- standard library implementations
- runtime service boundary
- debug trace serialization

## Not Included

The core library does not own:

- CLI parsing
- REPL loop
- bytecode printer executable entry
- test runner main

Those live in the other projects.

## Main Documentation

- `docs/architecture/overview.md`
- `docs/architecture/gc.md`
- `docs/architecture/runtime-services.md`
- `docs/compiler/bytecode-generation.md`
- `docs/vm/instruction-set.md`
- `docs/stdlib/overview.md`
