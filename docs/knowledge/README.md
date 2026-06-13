---
status: current
verified_against: docs/index.md; docs/status/project-status.md; docs2/README.md; docs/knowledge/source-document-map.md; docs/ai/rag-knowledge-base.md; docs/agents/senior-lua-architect.md
last_checked: 2026-06-13
applies_to: structured documentation library for human and AI collaborators
---

# Knowledge Library

This directory is the coordination layer between the original technical docs in `docs/` and the Chinese architecture-understanding docs in `docs2/`.

## Layers

| Layer | Location | Role | Trust rule |
|---|---|---|---|
| Fact layer | `docs/` | Current build, roadmap, implementation contracts, quality gates, and deep technical references | Prefer this layer when status, test counts, or implementation facts disagree |
| Understanding layer | `docs2/` | Chinese learning path, mental model, source-location guide, and module-by-module explanations | Use this layer to orient, then confirm current facts through `docs/` |
| Mapping layer | `docs/knowledge/` | Source-to-document index and AI navigation policy | Keep it synchronized when modules or document ownership changes |
| RAG layer | `docs/ai/` and `docs/ai/generated/` | Retrieval index recipe plus generated sparse-vector chunks | Regenerate after meaningful source or documentation changes |
| Agent layer | `docs/agents/` | Reusable collaboration workflow for AI coding agents | Use it before continuing roadmap or behavior-changing tasks |

## Required Entry Points

Read these first for nearly every maintenance task:

1. `docs/status/project-status.md`
2. `docs/roadmap/current.md`
3. `docs/guides/development.md`
4. `docs/knowledge/source-document-map.md`
5. `docs/ai/rag-knowledge-base.md`
6. `docs/agents/senior-lua-architect.md`

For a first human reading path, `docs/index.md` remains the friendly start page. For Chinese architectural orientation, `docs2/README.md` remains the table of contents.

## Synchronization Rules

- `docs/status/project-status.md` is the single source of truth for current build path, test counts, quality gates, and implementation status.
- `docs/roadmap/current.md` is the single source of truth for continuation context and the next-session checklist.
- `docs/vm/instruction-set.md` is the single source of truth for the current 38-opcode VM contract.
- `docs2/` documents may explain the same ideas in Chinese, but should link back to the fact layer for volatile facts.
- Behavior-changing code edits must run `tools/check_doc_drift.ps1` after updating related docs.
- New C++ source files should be registered through `tools/add_source.ps1` so Visual Studio and CMake stay aligned.

## Metadata Contract

Core documents use a YAML fact header:

```yaml
---
status: current
verified_against: path/a; path/b
last_checked: 2026-06-13
applies_to: short scope description
---
```

`tools/check_doc_drift.ps1` validates this contract for the curated core-doc list. The required fields are intentionally boring: they let humans and agents quickly answer "Is this current?", "What did we compare it to?", and "Which part of the repository does it describe?"
