# PR-05 Parser Expected Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Change `Parser::parse()` from throwing `ParseError` at the public boundary to returning `std::expected<Chunk, ParseError>`.

**Architecture:** Keep the existing recursive-descent parser internals unchanged for this PR: helper functions may still throw `ParseError`. Convert those exceptions to `std::unexpected` inside `Parser::parse()`, then update production and test callers to check the returned expected explicitly.

**Tech Stack:** C++23 `std::expected`, MSVC / Visual Studio project files, existing Lua unit test framework.

---

### Task 1: Add Expected API Sentinel Tests

**Files:**
- Modify: `tests/unit/compiler/test_parser_error_recovery.cpp`

- [x] **Step 1: Write the failing tests**

Add a compile/runtime sentinel that proves `Parser::parse()` returns `std::expected<Chunk, ParseError>`, succeeds for valid input, and returns `unexpected(ParseError)` for invalid input.

- [x] **Step 2: Run the parser error suite**

Run: `.\bin\lua_test.exe --filter "Parser Error Reporting"`

Expected before implementation: compile failure or test failure because `Parser::parse()` still returns `Chunk`.

### Task 2: Change Parser Public Boundary

**Files:**
- Modify: `src/compiler/parser.hpp`
- Modify: `src/compiler/parser.cpp`

- [x] **Step 1: Update the signature**

Change the public method to:

```cpp
[[nodiscard]] std::expected<Chunk, ParseError> parse();
```

Include `<expected>` from `parser.hpp` so callers see the complete return type.

- [x] **Step 2: Convert internal parser exceptions at the boundary**

Wrap the old parse body in `try/catch (const ParseError&)` and return `std::unexpected(error)` on failure.

- [x] **Step 3: Preserve parser internals**

Do not rewrite recursive helper functions in this PR. `error()` and focused helper throws remain internal control flow until later parser cleanup PRs.

### Task 3: Update Callers

**Files:**
- Modify all `Parser::parse()` production and test call sites under `src/` and `tests/`.

- [x] **Step 1: For success-only test helpers, assert expected has value**

Use a local pattern:

```cpp
auto parsed = parser.parse();
ASSERT_TRUE(suite, parsed.has_value(), "parse succeeds");
if (!parsed) return;
Chunk chunk = std::move(*parsed);
```

- [x] **Step 2: For error tests, check expected.error()**

Replace `try/catch (ParseError&)` parser expectations with:

```cpp
auto parsed = parser.parse();
ASSERT_TRUE(suite, !parsed.has_value(), "parse fails");
const ParseError& e = parsed.error();
```

- [x] **Step 3: For production callers, propagate existing behavior**

Where the surrounding code already catches `ParseError`, preserve behavior by throwing `parsed.error()` after checking failure. This keeps CLI, REPL, stdlib, and bytecode behavior stable while making the parser boundary explicit.

### Task 4: Verify

**Files:**
- No source changes expected.

- [x] **Step 1: Build**

Run: `& 'D:\VS2026\MSBuild\Current\Bin\MSBuild.exe' .\lua_test.vcxproj /p:Configuration=Debug /p:Platform=x64 /m`

Expected: build succeeds with no errors.

- [x] **Step 2: Run focused parser tests**

Run: `.\bin\lua_test.exe --filter "Parser"`

Expected: selected parser tests pass.

- [x] **Step 3: Run full quality gate**

Run: `powershell -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1`

Expected: build succeeds and all unit tests pass. If clang tools are unavailable on PATH, the script may skip those checks.
