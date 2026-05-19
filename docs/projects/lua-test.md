---
status: current
verified_against: lua_test.vcxproj; CMakeLists.txt; tests/unit/framework/test_runner.cpp; tests/unit/
last_checked: 2026-05-19
applies_to: lua_test unit test executable
---

# lua_test

`lua_test.vcxproj` builds the custom C++ unit test executable. In CMake, the target is `lua_test`.

## Responsibilities

- register all unit tests
- run full or filtered test sets
- report pass/fail counts
- optionally write JUnit XML

The latest recorded project status is tracked in `docs/status/project-status.md`.

## Test Areas

- app options
- compiler
- core runtime objects
- GC
- IO helpers
- metamethods
- standard library
- VM

See `docs/guides/test-runner.md`.
