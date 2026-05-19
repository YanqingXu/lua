---
status: current
verified_against: src/bytecode/bytecode_main.cpp; src/bytecode/bytecode_printer.cpp; src/bytecode/bytecode_printer.hpp; lua_bytecode.vcxproj; CMakeLists.txt
last_checked: 2026-05-19
applies_to: lua_bytecode command-line tool status
---

# Bytecode Tool Guide

`lua_bytecode.exe` is intended to compile a Lua script and print the generated `Proto` bytecode.

Current status is partial:

- `src/bytecode/bytecode_main.cpp` reads a script, parses it, generates a `Proto`, and calls `printProtoBytecode`.
- `src/bytecode/bytecode_printer.cpp` currently prints `(bytecode_printer stub)`.

So the target is useful as a build and integration slice, but not yet as a real bytecode inspection tool.

## Usage

```powershell
bin\lua_bytecode.exe examples\hello.lua
bin\lua_bytecode.exe examples\hello.lua full
```

`full` is parsed and passed to `printProtoBytecode`, but the current stub ignores it.

## Current Data Flow

```text
script path
  -> readWholeFile()
  -> Parser
  -> CodeGenerator
  -> Proto*
  -> printProtoBytecode(...)
```

## Needed Work

A complete printer should show:

- source name
- parameter count and vararg flag
- `maxStackSize`
- constants
- child protos
- line info
- local debug names
- each instruction with decoded `A/B/C/Bx/sBx`
- optional recursive full output for child protos

The natural implementation dependency is `src/compiler/opcode.hpp` for instruction decoding and `src/core/function.hpp` for `Proto` accessors.
