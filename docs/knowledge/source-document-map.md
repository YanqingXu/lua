---
status: current
verified_against: docs/knowledge/source-map/directory-map.md; docs/knowledge/source-map/core-files.md; docs/knowledge/source-map/entry-points.md; docs/knowledge/source-map/dependency-map.md; docs/architecture/overview.md; docs/compiler/bytecode-generation.md; docs/vm/instruction-set.md; docs/runtime/services.md; docs/gc/implementation.md; src/compiler/; src/core/; src/vm/; src/gc/; src/lib/; src/runtime/
last_checked: 2026-07-11
applies_to: source-to-document mapping for interpreter modules
---

# 源码与技术文档映射

本索引把生产源码、技术文档和高信号测试连接起来。`docs/` 按解释器模块组织；详细目录结构见 [source-map/directory-map.md](source-map/directory-map.md)。

## 模块矩阵

| 模块 | 源码所有权 | 技术文档 | 主要测试 |
|---|---|---|---|
| Architecture | `src/main.cpp`, `src/runtime/`, 跨模块入口 | `docs/architecture/overview.md`, `docs/architecture/execution-pipeline/`, `docs/architecture/patterns.md` | 端到端 Lua 脚本与各模块测试 |
| Compiler frontend | `src/compiler/lexer/`, `src/compiler/parser/`, `src/compiler/ast.*` | `docs/compiler/lexer.md`, `docs/compiler/parser.md`, `docs/compiler/frontend/` | `tests/unit/compiler/test_lexer_*`, `test_parser_*` |
| Bytecode compiler | `src/compiler/codegen/`, `src/compiler/opcode.*`, `src/compiler/register_allocator.*` | `docs/compiler/bytecode-generation.md`, `docs/compiler/codegen/`, `docs/compiler/bytecode/`, `docs/compiler/control-flow/` | Codegen Characterization、Expression/Statement Emitter、Symbol Binding |
| VM | `src/vm/`, `src/vm/state/`, `src/vm/vm_handlers/` | `docs/vm/instruction-set.md`, `docs/vm/runtime/`, `docs/vm/trace-system.md` | `tests/unit/vm/`, `tests/unit/metamethod/` |
| Runtime values | `src/core/value.*`, `src/core/string*`, `src/core/userdata.*` | `docs/runtime/value/`, `docs/runtime/services.md` | `tests/unit/core/` |
| Tables and metatables | `src/core/table.*`, `src/core/metatable.*` | `docs/runtime/table/` | `tests/unit/core/test_table.cpp`, `tests/unit/metamethod/` |
| Functions and closures | `src/core/function.*`, `src/core/upvalue.*`, `src/core/thread.*`, `src/vm/vm_call.cpp`, `src/vm/vm_frame.cpp` | `docs/runtime/functions/` | function、closure、upvalue、coroutine 测试 |
| GC | `src/gc/`, `src/core/gc_object.*`, `src/core/string_pool.*` | `docs/gc/` | `tests/unit/gc/` 及弱表、finalizer 测试 |
| Standard library | `src/lib/`, `src/api/` | `docs/stdlib/` | `tests/unit/stdlib/`, `tests/lua/stdlib/` |
| Compatibility | 跨 compiler、VM、runtime、GC 和 stdlib | `docs/compatibility/lua51/` | `tests/lua/official/`, compatibility probes |
| Diagnostics | `src/debug/`, VM trace、错误传播路径 | `docs/debugging/` | trace、error、stack dump 和回归测试 |

## 按改动定位

| 技术意图 | 源码起点 | 必读文档 | 验证证据 |
|---|---|---|---|
| 修改词法或语法 | `src/compiler/lexer/`, `src/compiler/parser/`, `src/compiler/ast.hpp` | `docs/compiler/frontend/`, `docs/compiler/extending-syntax.md` | lexer/parser 单测和 Lua 语法回归 |
| 修改表达式或语句 lowering | `src/compiler/codegen/` | `docs/compiler/codegen/`, `docs/compiler/bytecode/` | emitter、characterization 和 multret 测试 |
| 修改 opcode | `src/compiler/opcode.*`, `src/vm/vm_handlers/` | `docs/vm/instruction-set.md`, `docs/vm/extending-opcodes.md` | opcode coverage matrix 和 VM 单测 |
| 修改调用、闭包或 upvalue | `src/core/function.*`, `src/core/upvalue.*`, `src/vm/vm_call.cpp` | `docs/runtime/functions/`, `docs/vm/runtime/call-frame.md` | closure、call、tail-call 和 coroutine 测试 |
| 修改值、table 或元方法 | `src/core/` | `docs/runtime/value/`, `docs/runtime/table/` | core 与 metamethod 测试 |
| 修改 GC | `src/gc/`, GC hooks in `src/core/` | `docs/gc/implementation.md`, `docs/gc/reference-graph.md` | GC、weak table 和 finalizer 测试 |
| 修改标准库 | `src/lib/`, `src/api/` | `docs/stdlib/overview.md`, `docs/stdlib/library-reference/` | stdlib 单测和 Lua 脚本 |
| 修改错误或 trace | `src/debug/`, compiler diagnostics, VM error paths | `docs/debugging/` | error recovery、trace 和 stack tests |

## 详细源码地图

- [目录地图](source-map/directory-map.md)
- [核心文件](source-map/core-files.md)
- [入口点](source-map/entry-points.md)
- [模块依赖](source-map/dependency-map.md)
