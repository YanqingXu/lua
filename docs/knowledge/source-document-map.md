---
status: current
verified_against: docs2/02-source-code-map/00-directory-map.md; docs2/02-source-code-map/01-core-files.md; docs/architecture/overview.md; docs/compiler/bytecode-generation.md; docs/compiler/codegen-responsibility-map.md; docs/vm/instruction-set.md; docs/architecture/runtime-services.md; docs/architecture/gc.md; src/compiler/; src/core/; src/vm/; src/gc/; src/runtime/runtime_services.hpp
last_checked: 2026-06-13
applies_to: source-to-document mapping for core interpreter modules
---

# Source And Document Map

This index connects the source tree to the documents an agent should retrieve before changing behavior. It complements `docs2/02-source-code-map/00-directory-map.md`: that file is the Chinese directory tour, while this file is the maintenance index used by docs drift checks and RAG weighting.

## Core Module Matrix

| Module | Source ownership | Deep technical docs | Chinese understanding docs | High-signal tests |
|---|---|---|---|---|
| Compiler | `src/compiler/lexer/`, `src/compiler/parser/`, `src/compiler/codegen/`, `src/compiler/ast.*`, `src/compiler/opcode.*` | `docs/compiler/bytecode-generation.md`, `docs/compiler/codegen-responsibility-map.md`, `docs/compiler/lexer.md`, `docs/compiler/parser.md`, `docs/compiler/register-allocation.md` | `docs2/03-lexer-parser/00-overview.md`, `docs2/04-bytecode-compiler/00-overview.md` | `tests/unit/compiler/`, especially `Codegen Characterization`, `Expression Emitter`, `Statement Emitter`, `Symbol Binding` |
| VM | `src/vm/`, `src/vm/vm_handlers/`, `src/vm/state/` | `docs/vm/instruction-set.md`, `docs/vm/trace-system.md`, `docs/architecture/overview.md` | `docs2/05-vm-runtime/00-overview.md` | `tests/unit/vm/`, `tests/unit/metamethod/`, Lua script regressions under `tests/lua/` |
| Runtime | `src/core/`, `src/runtime/runtime_services.hpp`, `src/lib/`, `src/repl/`, `src/api/` | `docs/architecture/runtime-services.md`, `docs/architecture/overview.md`, `docs/stdlib/overview.md`, `docs/glossary.md` | `docs2/06-value-object-system/00-overview.md`, `docs2/07-table-metatable/00-overview.md`, `docs2/08-function-call-closure/00-overview.md`, `docs2/10-stdlib/00-overview.md` | `tests/unit/core/`, `tests/unit/stdlib/`, `tests/unit/app/`, `tests/lua/runtime/` |
| GC | `src/gc/`, `src/core/gc_object.*`, GC hooks in `src/core/` and `src/vm/state/` | `docs/architecture/gc.md`, `docs/walkthroughs/gc-cycle.md`, `docs/architecture/runtime-services.md` | `docs2/12-gc-memory/00-overview.md` | `tests/unit/gc/test_gc.cpp`, GC-related stdlib tests, official `gc.lua` staged smoke |

Each row above has at least one deep technical document and one Chinese orientation document. Add a new row or extend the matching row when a new subsystem becomes large enough to need separate ownership.

## File-Level Locator

| Change intent | Start in source | Read first | Verify with |
|---|---|---|---|
| Add or change syntax | `src/compiler/parser/`, `src/compiler/ast.hpp` | `docs/compiler/parser.md`, `docs2/03-lexer-parser/00-overview.md`, `docs2/02-source-code-map/05-change-location-guide.md` | Parser/compiler unit tests and relevant Lua script tests |
| Change bytecode emission | `src/compiler/codegen/` | `docs/compiler/bytecode-generation.md`, `docs/compiler/codegen-responsibility-map.md`, `docs/vm/instruction-set.md` | `Codegen Characterization`, `Expression Emitter`, `Statement Emitter` |
| Change opcode semantics | `src/vm/vm_handlers/`, `src/vm/vm_*.cpp`, `src/compiler/opcode.*` | `docs/vm/instruction-set.md`, `tests/unit/vm/opcode_coverage_matrix.md`, `docs2/05-vm-runtime/00-overview.md` | `tools/check_opcode_coverage_matrix.ps1`, VM unit tests |
| Change values, tables, closures, or upvalues | `src/core/`, `src/vm/vm_call.cpp`, `src/vm/vm_frame.cpp` | `docs/architecture/overview.md`, `docs/architecture/runtime-services.md`, `docs2/06-value-object-system/00-overview.md`, `docs2/08-function-call-closure/00-overview.md` | Core, metamethod, function-call, and regression tests |
| Change standard library behavior | `src/lib/`, `src/api/lapi.cpp` | `docs/stdlib/overview.md`, `docs/guides/development.md`, `docs2/10-stdlib/00-overview.md` | `tests/unit/stdlib/`, `tests/lua/stdlib/`, official suite slice |
| Change GC behavior | `src/gc/`, `src/core/gc_object.*`, `src/core/string_pool.*` | `docs/architecture/gc.md`, `docs/walkthroughs/gc-cycle.md`, `docs2/12-gc-memory/00-overview.md` | `tests/unit/gc/test_gc.cpp`, GC stdlib tests, official `gc.lua` |
| Change build or source lists | `CMakeLists.txt`, `*.vcxproj`, `*.vcxproj.filters` | `docs/guides/development.md`, `docs/status/project-status.md` | `tools/add_source.ps1`, `tools/test_quality_gate.ps1` |
| Change docs or status | `docs/`, `docs2/`, `README.md` | `docs/knowledge/README.md`, `docs/roadmap/current.md`, `tools/check_doc_drift.ps1` | `tools/check_doc_drift.ps1` |

## Retrieval Priority

Agents should boost these documents above ordinary chunks because they encode volatile coordination facts:

| Priority | Path | Why |
|---|---|---|
| Critical | `docs/roadmap/current.md` | Continuation checklist, current roadmap state, required validation commands |
| Critical | `docs/vm/instruction-set.md` | Current 38-opcode VM contract and instruction semantics |
| Critical | `tools/check_doc_drift.ps1` | Executable definition of documentation drift rules |
| High | `docs/status/project-status.md` | Current build/test/quality state and source-of-truth facts |
| High | `docs/guides/development.md` | Coding, build, and validation workflow |
| High | `docs/compiler/codegen-responsibility-map.md` | Current compiler split and characterization guardrails |
| High | `docs/architecture/runtime-services.md` | Explicit runtime-service boundary and singleton exceptions |
| High | `docs/architecture/gc.md` | Current GC semantics and invariants |

The local RAG builder in `tools/build_rag_index.ps1` encodes this table as retrieval boosts. This source map is intentionally not boosted: it is a router, while semantic answers should come from the module documents it points to.
