# PR-22 Architecture Patterns Documentation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 新增 `docs/architecture/patterns.md`，把当前已经落地和仍处于规划状态的架构模式登记到一个稳定入口。

**Architecture:** 该文档只描述现状和边界，不引入新的运行时代码。模式登记以源码文件为锚点，明确哪些模式已经实现、哪些只是路线图中的后续方向，避免后续 PR 把模式名误用成重构目标本身。

**Tech Stack:** Markdown, existing architecture documentation metadata, current C++23 source layout.

---

### Task 1: Add The Pattern Registry

**Files:**
- Create: `docs/architecture/patterns.md`

- [x] **Step 1: Create the document metadata**

Use the same front matter shape as the existing architecture docs:

```markdown
---
status: current
verified_against: src/compiler/ast_visitor.hpp; src/compiler/codegen.hpp; src/compiler/codegen_expr.cpp; src/vm/vm_handlers.hpp; src/vm/vm_handlers.cpp; src/vm/vm_handlers/; src/vm/vm_dispatch_strategy.hpp; src/runtime/runtime_services.hpp; src/vm/global_state.hpp; src/compiler/bytecode_builder.hpp; docs/roadmap/optimization_and_refactoring.md
last_checked: 2026-05-21
applies_to: architecture pattern registry and implementation boundaries
---
```

- [x] **Step 2: Document implemented patterns**

Add a table covering the implemented patterns:

```markdown
| Pattern | Status | Primary files | Current role |
|---|---|---|---|
| Visitor | Implemented | `src/compiler/ast_visitor.hpp`, `src/compiler/codegen.hpp`, `src/compiler/codegen_expr.cpp` | CRTP visitors over AST variants; expression lowering uses `ExprVisitor<CodeGenerator, ValueResult>`. |
| Command | Implemented | `src/vm/vm_handlers.hpp`, `src/vm/vm_handlers.cpp`, `src/vm/vm_handlers/` | Opcode handlers are free functions registered into a `HandlerTable`; table dispatch calls `runHandler()`. |
| Strategy | Implemented | `src/vm/vm_dispatch_strategy.hpp`, `src/vm/vm_dispatch_strategy.cpp`, `src/runtime/runtime_services.hpp`, `src/vm/vm.cpp` | `SwitchDispatch` remains the default strategy; `TableDispatch` is opt-in through `RuntimeServices::dispatchStrategy`. |
| Singleton | Compatibility boundary | `src/vm/global_state.hpp`, `src/runtime/runtime_services.hpp`, `src/gc/garbage_collector.hpp` | `GlobalState` remains singleton-backed; new entry points should prefer explicit `RuntimeServices`. |
| Builder | Implemented | `src/compiler/bytecode_builder.hpp`, `src/compiler/codegen_state.hpp` | `BytecodeBuilder` is the narrow write boundary for `Proto` bytecode emission. |
```

- [x] **Step 3: Document planned patterns separately**

Record `GCStrategy` as planned, not implemented:

```markdown
| Pattern | Status | Planned files | Notes |
|---|---|---|---|
| GC Strategy | Planned | future `GCStrategy` / `MarkSweepGC` boundary | The current collector is still one concrete mark-sweep implementation owned by `GlobalState`. |
```

- [x] **Step 4: Add usage rules**

Add short guidance for future contributors:

```markdown
- Treat this file as a registry, not a mandate.
- Prefer the existing implementation style before introducing a new abstraction.
- Keep `SwitchDispatch` as the default VM strategy unless a task explicitly changes that policy.
- Do not add computed-goto or threaded-code dispatch paths; they conflict with the readability goal in the roadmap.
```

### Task 2: Link The New Registry From Architecture Overview

**Files:**
- Modify: `docs/architecture/overview.md`

- [x] **Step 1: Update metadata**

Append `docs/architecture/patterns.md` to `verified_against` and set `last_checked` to `2026-05-21`.

- [x] **Step 2: Add a Reading Map entry**

Add this line near the other architecture links:

```markdown
- Architecture patterns: `docs/architecture/patterns.md`
```

### Task 3: Verify Documentation Changes

**Files:**
- Read: `tools/run_quality_gate.ps1`
- Read: `tools/check_doc_drift.ps1`

- [x] **Step 1: Check new docs for trailing whitespace**

Run:

```powershell
rg -n "[ \t]+$" docs\architecture\patterns.md docs\superpowers\plans\2026-05-21-pr-22-patterns-doc.md
```

Expected: no matches.

- [x] **Step 2: Run the quality gate**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1
```

Expected: build and tests pass. `clang-format` / `clang-tidy` may be skipped when those tools are not installed on PATH.

- [x] **Step 3: Check the git diff for whitespace errors**

Run:

```powershell
git diff --check
```

Expected: no whitespace errors. Line-ending warnings may appear for existing CRLF/LF normalization differences.
