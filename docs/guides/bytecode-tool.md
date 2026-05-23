---
status: current
verified_against: src/bytecode/bytecode_main.cpp; src/bytecode/bytecode_printer.cpp; src/bytecode/bytecode_printer.hpp; lua_bytecode.vcxproj; CMakeLists.txt
last_checked: 2026-05-23
applies_to: lua_bytecode command-line tool status
---

# Bytecode Tool Guide

`lua_bytecode.exe` is intended to compile a Lua script and print the generated `Proto` bytecode.

Current status is partial but no longer a stub:

- `src/bytecode/bytecode_main.cpp` reads a script, parses it, generates a `Proto`, and calls `printProtoBytecode`.
- `src/bytecode/bytecode_printer.cpp` prints the Proto header, decoded instructions, constant references, the constant table, and recursive child Proto sections in `full` mode.

So the target is useful for bytecode inspection across closures, but not yet a diff tool or CFG visualizer.

## Usage

```powershell
bin\lua_bytecode.exe examples\hello.lua
bin\lua_bytecode.exe examples\hello.lua full
```

`full` enables recursive child Proto output. Compact mode prints only the top-level Proto; both modes annotate `CLOSURE` with the referenced `proto[index]` summary.

## Current Data Flow

```text
script path
  -> readWholeFile()
  -> Parser
  -> CodeGenerator
  -> Proto*
  -> printProtoBytecode(...)
```

## Current Output

The current printer shows:

- source name
- parameter count and vararg flag
- `maxStackSize`
- constants
- line info
- each instruction with decoded `A/B/C/Bx/sBx`
- constant comments for `LOADK`, RK operands, and jump targets
- `CLOSURE` comments with child proto references
- recursive child protos in full mode

## Needed Work

A complete printer should still add:

- local debug names
- diff mode for two scripts
- optional CFG output

The natural implementation dependency is `src/compiler/opcode.hpp` for instruction decoding and `src/core/function.hpp` for `Proto` accessors.
