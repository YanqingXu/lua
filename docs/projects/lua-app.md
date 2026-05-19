---
status: current
verified_against: lua_app.vcxproj; CMakeLists.txt; src/main.cpp; src/repl.cpp; src/repl.hpp; src/app/app_options.cpp; src/app/app_options.hpp
last_checked: 2026-05-19
applies_to: lua_app interpreter executable
---

# lua_app

`lua_app.vcxproj` builds the interpreter executable. In CMake, the target is `lua_app`.

## Source Files

- `src/app/app_options.cpp`
- `src/app/app_options.hpp`
- `src/main.cpp`
- `src/repl.cpp`
- `src/repl.hpp`

## Modes

The executable supports:

- version mode
- help mode
- script mode
- REPL mode
- default behavior mode
- optional JSONL VM trace output

See `docs/guides/repl-cli.md` for user-facing behavior.

## Runtime Setup

`main.cpp` creates a `LuaState`, opens standard libraries, optionally installs a `JsonTraceSink`, then either runs a script or enters the REPL.
