---
status: current
verified_against: tools/build_rag_index.ps1; tools/search_rag_index.ps1; docs/knowledge/source-document-map.md; docs/vm/instruction-set.md; tools/check_doc_drift.ps1; README.md
last_checked: 2026-07-11
applies_to: local sparse-vector RAG knowledge base for this repository
---

# RAG Knowledge Base

The repository has a local, dependency-free sparse-vector RAG index. It vectorizes:

- C++ source under `src/`
- Markdown documents under `docs/`
- Markdown documents under `docs2/`
- selected workflow scripts under `tools/`, including `check_doc_drift.ps1`

The index is designed for agent context selection, not for replacing the source tree. Always read the actual file before editing behavior.

## Build

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\build_rag_index.ps1
```

Generated files:

| File | Purpose |
|---|---|
| `docs/ai/generated/rag-index.jsonl` | One JSON object per chunk, including text, metadata, boost, tags, and sparse vector |
| `docs/ai/generated/rag-manifest.json` | Build metadata, source counts, chunk counts, and high-priority retrieval files |

## Search

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\search_rag_index.ps1 -Query "README current facts 文档漂移" -TopK 5
powershell -NoProfile -ExecutionPolicy Bypass -File tools\search_rag_index.ps1 -Query "38 opcode instruction semantics" -TopK 5
```

The search script uses the same tokenizer as the builder, scores query vectors against each chunk vector, and applies the chunk boost again at ranking time. It is intentionally simple so agents can run it on a clean Windows checkout without an embedding service.

## Boost Policy

The builder assigns higher retrieval weights to volatile coordination facts:

| Boost | Path |
|---|---|
| 4.0 | `README.md` |
| 4.0 | `docs/vm/instruction-set.md` |
| 5.0 | `tools/check_doc_drift.ps1` |
| 2.5 | `docs/guides/development.md` |
| 2.0 | `docs/compiler/codegen-responsibility-map.md` |
| 2.0 | `docs/architecture/runtime-services.md` |
| 2.0 | `docs/architecture/gc.md` |

This makes continuation state, the 38-instruction VM contract, and documentation drift mechanics surface before older or more narrative docs.

## Chunk Schema

Each `rag-index.jsonl` row has this shape:

```json
{
  "id": "docs/vm/instruction-set.md#chunk-0001",
  "path": "docs/vm/instruction-set.md",
  "kind": "doc",
  "language": "markdown",
  "boost": 4.0,
  "start_line": 1,
  "end_line": 80,
  "title": "VM 指令集",
  "tags": ["vm", "documentation"],
  "vector": {"opcode": 0.42, "instruction": 0.31},
  "text": "..."
}
```

The vector is a normalized sparse term-weight map. English/code identifiers are tokenized as words; Chinese text also receives CJK bigrams so queries such as `文档漂移` and `续接检查` can match Chinese documentation. The builder indexes each chunk together with its path, title, and selected retrieval hints for high-priority coordination files.

## Agent Use

Before a code change, retrieve with two queries:

1. A task query, such as `table metamethod __newindex VM handler`.
2. A coordination query, such as `README 当前事实 文档漂移 质量门`.

Then read the top source files and docs directly. After behavior changes, update docs and run `tools/check_doc_drift.ps1`.
