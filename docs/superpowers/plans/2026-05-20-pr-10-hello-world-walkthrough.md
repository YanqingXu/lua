# PR-10 Hello World Walkthrough Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an end-to-end Chinese walkthrough for `print("hello")` from source text to VM execution and standard output.

**Architecture:** Create one focused document under `docs/walkthroughs/hello-world.md` and link it from the walkthrough index. The article uses real `lua_bytecode` output and source `path:line` anchors so readers can jump between observable output and implementation.

**Tech Stack:** Markdown docs, existing `lua_bytecode` tool, existing compiler/VM/library source files.

---

### Task 1: Gather Reproducible Output

**Files:**
- Read: `src/bytecode/bytecode_main.cpp`
- Read: `src/bytecode/bytecode_printer.cpp`
- Read: compiler, VM, and base library source files

- [ ] **Step 1: Generate `print("hello")` bytecode**

Run a temporary script through:

```powershell
$tmp = New-TemporaryFile
Set-Content -LiteralPath $tmp -Value 'print("hello")' -NoNewline
.\bin\lua_bytecode.exe $tmp full
Remove-Item -LiteralPath $tmp
```

Expected output contains `GETGLOBAL`, `MOVE`, `LOADK`, `CALL`, and `RETURN`.

- [ ] **Step 2: Run the same script**

Run:

```powershell
$tmp = New-TemporaryFile
Set-Content -LiteralPath $tmp -Value 'print("hello")' -NoNewline
.\bin\lua_app.exe $tmp
Remove-Item -LiteralPath $tmp
```

Expected output:

```text
hello
```

### Task 2: Write the Walkthrough

**Files:**
- Create: `docs/walkthroughs/hello-world.md`
- Modify: `docs/walkthroughs/index.md`

- [ ] **Step 1: Create the article**

Cover these concrete stages:

- Source and command reproduction
- Lexer tokens
- Parser/AST shape
- Codegen decisions and emitted bytecode
- VM dispatch through `SwitchDispatch`
- `CALL` into `luaB_print`
- `stdout` write and zero return values

- [ ] **Step 2: Link the article from the index**

Add `docs/walkthroughs/hello-world.md` to the suggested reading path and topic shortcuts.

### Task 3: Documentation Verification

**Files:**
- Verify: `docs/walkthroughs/hello-world.md`
- Verify: `docs/walkthroughs/index.md`

- [ ] **Step 1: Check source anchors**

Run `rg -n` for each cited function name and confirm the line anchors still point to the correct function or switch case.

- [ ] **Step 2: Check generated output**

Re-run `lua_bytecode` and `lua_app` with `print("hello")` and compare the snippets in the doc.
