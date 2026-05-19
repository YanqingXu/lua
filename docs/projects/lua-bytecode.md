---
status: current
verified_against: lua_bytecode.vcxproj; CMakeLists.txt; src/bytecode/bytecode_main.cpp; src/bytecode/bytecode_printer.cpp; src/bytecode/bytecode_printer.hpp
last_checked: 2026-05-19
applies_to: lua_bytecode executable
---

# lua_bytecode

`lua_bytecode.vcxproj` builds the bytecode inspection executable. In CMake, the target is `lua_bytecode`.

## Current Status

The executable can parse and compile a Lua source file into a `Proto`, but the printer is currently a stub. Running it reaches `printProtoBytecode()`, which prints `(bytecode_printer stub)`.

## Source Files

- `src/bytecode/bytecode_main.cpp`
- `src/bytecode/bytecode_printer.cpp`
- `src/bytecode/bytecode_printer.hpp`

## Documentation

See `docs/guides/bytecode-tool.md`.
