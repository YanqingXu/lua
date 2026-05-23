---
status: current
verified_against: lua_bytecode.vcxproj; CMakeLists.txt; src/bytecode/bytecode_main.cpp; src/bytecode/bytecode_printer.cpp; src/bytecode/bytecode_printer.hpp
last_checked: 2026-05-23
applies_to: lua_bytecode executable
---

# lua_bytecode

`lua_bytecode.vcxproj` builds the bytecode inspection executable. In CMake, the target is `lua_bytecode`.

## Current Status

The executable can parse and compile Lua source files into `Proto` objects. `printProtoBytecode()` prints Proto metadata, decoded instructions, constant references, the constant table, and recursive child Proto sections when the optional `full` argument is used. `--diff` compiles two scripts and prints a side-by-side summary of changed bytecode lines. `--cfg` compiles one script and emits a Mermaid `flowchart TD` control-flow graph with basic blocks and labeled jump / fallthrough / loop / return edges.

## Source Files

- `src/bytecode/bytecode_main.cpp`
- `src/bytecode/bytecode_printer.cpp`
- `src/bytecode/bytecode_printer.hpp`

## Documentation

See `docs/guides/bytecode-tool.md`.
