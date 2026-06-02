---
status: current
verified_against: tests/unit/framework/test_runner.cpp; tests/unit/framework/test_framework.hpp; lua_test.vcxproj; CMakeLists.txt; tools/add_source.ps1; docs/status/project-status.md
last_checked: 2026-06-02
applies_to: lua_test executable usage and extension
---

# Test Runner Guide

`lua_test.exe` is the repository's custom unit test runner. It registers all C++ unit tests from `tests/unit/**` and can list, filter, run, and export JUnit XML.

## Commands

```powershell
bin\lua_test.exe
bin\lua_test.exe --list
bin\lua_test.exe --filter "Symbol Binding"
bin\lua_test.exe --filter=Runtime
bin\lua_test.exe --report=junit
bin\lua_test.exe --report=junit:bin\lua_test_junit.xml
bin\lua_test.exe --filter "Lua 5.1 Official Suite"
```

## Options

| Option | Behavior |
|---|---|
| `--help`, `-h` | Print usage |
| `--list` | Print registered tests without running them |
| `--filter <text>` | Run tests whose suite, name, or `Suite::Name` contains text, case-insensitive |
| `--filter=<text>` | Same as above |
| `--report=junit` | Write `lua_test_junit.xml` |
| `--report=junit:<path>` | Write JUnit XML to a specific path |
| `--max-memory-mb <mb>` | Override the process memory cap for this run |
| `--max-memory-mb=<mb>` | Same as above |
| `--no-memory-limit` | Disable the runner cap; use only inside another memory-capped harness |

## Memory Safety

`lua_test.exe` installs a process memory cap before registering or running tests. The default cap is 512 MB.
If the cap cannot be installed, the runner exits before executing tests instead of running unprotected.

This is the default safety boundary for official Lua 5.1 pressure paths. Prefer focused filters and explicit caps while
working on those paths:

```powershell
bin\lua_test.exe --max-memory-mb 128 --filter "post-vararg"
bin\lua_test.exe --max-memory-mb 128 --filter "closure.lua weak GC loop cap"
```

Environment overrides:

- `LUA_TEST_MAX_MEMORY_MB=<mb>` changes the default cap.
- `LUA_TEST_DISABLE_MEMORY_LIMIT=1` disables the cap for externally isolated runners.

## Adding A Test

1. Add or edit a file under `tests/unit/<area>/`.
2. Define a `registerXTests()` function.
3. Register individual tests through `TestRegistry::getInstance().registerTest(...)`.
4. Add the declaration and call in `tests/unit/framework/test_runner.cpp`.
5. Add the new file to `lua_test.vcxproj`, filters, and `CMakeLists.txt` with `tools\add_source.ps1 -SourcePath tests\unit\<area>\test_name.cpp -Target Test`.

Use focused filters while developing:

```powershell
bin\lua_test.exe --filter "Your Suite"
```

Then run the full runner before treating the change as verified:

```powershell
bin\lua_test.exe
```

The full runner still uses the default memory cap.

## Current Areas

The repository currently groups C++ tests under:

- `app`
- `compiler`
- `core`
- `gc`
- `io`
- `metamethod`
- `stdlib`
- `vm`

## CTest

CMake registers the same `lua_test` executable as a CTest test. CTest is a secondary path, not the primary Windows/MSBuild workflow.
