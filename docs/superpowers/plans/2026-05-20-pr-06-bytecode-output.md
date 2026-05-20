# PR-06 Bytecode Output Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Expand `lua_bytecode` output so each `Proto` shows source metadata, decoded instructions, constant references, and the full constant table.

**Architecture:** Keep `src/bytecode/bytecode_main.cpp` as a thin CLI wrapper and put formatting behavior in `src/bytecode/bytecode_printer.cpp`. Unit tests call `printProtoBytecode()` directly with handcrafted `Proto` instances so the output contract is fast, deterministic, and independent of parser/codegen details.

**Tech Stack:** C++23, existing `Proto` / opcode helpers, MSBuild and CMake project files, existing lightweight unit test framework.

---

### Task 1: Add Bytecode Printer Contract Tests

**Files:**
- Create: `tests/unit/bytecode/test_bytecode_printer.cpp`
- Modify: `tests/unit/framework/test_runner.cpp`
- Modify: `tests/unit/framework/test_registry.hpp`
- Modify: `CMakeLists.txt`
- Modify: `lua_test.vcxproj`
- Modify: `lua_test.vcxproj.filters`

- [x] **Step 1: Write failing printer tests**

Create tests that build a `Proto` with:
- source name `sample.lua`
- `numparams = 2`
- vararg enabled
- `maxStackSize = 7`
- one upvalue name `outer`
- constants: string `answer`, number `42`, bool `true`, nil
- instructions: `LOADK`, `GETGLOBAL`, `ADD` with RK constants, `JMP`

Assert the output contains:
- `source: sample.lua`
- `numparams: 2`
- `is_vararg: true`
- `maxStackSize: 7`
- `upvalues (1): outer`
- `constants (4)`
- `K[0] = string "answer"`
- `K[1] = number 42`
- `K[2] = boolean true`
- `K[3] = nil`
- decoded instruction fields such as `0000 | line 10 | LOADK | A=0 Bx=1 ; K[1] = number 42`
- `JMP` target text such as `; target=`

- [x] **Step 2: Run RED build**

Run: `& 'D:\VS2026\MSBuild\Current\Bin\MSBuild.exe' .\lua_test.vcxproj /p:Configuration=Debug /p:Platform=x64 /m`

Expected before implementation: build or test fails because `bytecode_printer` still prints a stub and the new test file is not yet registered in all build files.

### Task 2: Implement Printer Formatting

**Files:**
- Modify: `src/bytecode/bytecode_printer.cpp`
- Modify: `src/bytecode/bytecode_printer.hpp`

- [x] **Step 1: Format constants**

Add a local helper that converts `Value` to:
- `nil`
- `boolean true` / `boolean false`
- `number <value>`
- `string "<escaped value>"`
- fallback `<type name>` for non-constant runtime values

- [x] **Step 2: Print metadata block**

At the top of `printProtoBytecode()` output:

```text
Proto
  source: sample.lua
  linedefined: 0
  lastlinedefined: 0
  numparams: 2
  is_vararg: true
  maxStackSize: 7
  upvalues (1): outer
```

- [x] **Step 3: Print decoded instructions**

Use `getOpMode()`, `GETARG_A/B/C/Bx/sBx()`, and `getOpName()` to print one line per instruction:

```text
0000 | line 10 | LOADK | A=0 Bx=1 ; K[1] = number 42
```

For `OpArgK` RK operands, append referenced constants. For `LOADK`, `GETGLOBAL`, and `SETGLOBAL`, append `K[Bx]`. For `JMP`, `FORLOOP`, and `FORPREP`, append the absolute target PC.

- [x] **Step 4: Print constant table**

After instructions, print:

```text
constants (4)
  K[0] = string "answer"
  K[1] = number 42
  K[2] = boolean true
  K[3] = nil
```

### Task 3: Verify CLI Tool Builds

**Files:**
- Modify: `lua_app.vcxproj`
- Modify: `lua_bytecode.vcxproj`

- [x] **Step 1: Keep tool projects on C++23**

Because `parser.hpp` now exposes `std::expected`, set all Visual Studio tool configurations to `stdcpp23`. Add `/utf-8 /FS` to `lua_bytecode.vcxproj` compiler options to match the other projects.

- [x] **Step 2: Build the bytecode tool**

Run: `& 'D:\VS2026\MSBuild\Current\Bin\MSBuild.exe' .\lua_bytecode.vcxproj /p:Configuration=Debug /p:Platform=x64 /m`

Expected: build succeeds with no errors.

### Task 4: Final Verification

**Files:**
- No source changes expected.

- [x] **Step 1: Run focused bytecode tests**

Run: `.\bin\lua_test.exe --filter "Bytecode Printer"`

Expected: all selected tests pass.

- [x] **Step 2: Smoke-test `lua_bytecode`**

Create a small temporary Lua script and run:

```powershell
@'
local x = 42
return x
'@ | Set-Content -Encoding UTF8 build\pr06_bytecode_smoke.lua
.\bin\lua_bytecode.exe build\pr06_bytecode_smoke.lua
```

Expected: output contains `source: build\pr06_bytecode_smoke.lua`, `instructions`, and `constants`.

- [x] **Step 3: Run quality gate**

Run: `powershell -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1`

Expected: build succeeds and all unit tests pass. If clang tools are unavailable on PATH, the script may skip those checks.
