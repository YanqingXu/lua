---
status: current
verified_against: src/compiler/; src/vm/; src/core/; src/runtime/; src/gc/; src/lib/; src/debug/; tests/unit/; tests/lua/; docs/knowledge/source-map/directory-map.md; src/
last_checked: 2026-07-11
applies_to: source-to-document-to-test responsibility map
---

# 源码、文档与测试责任映射

本页回答“修改某项语义时从哪里读、在哪里改、用什么证据验证”。详细物理目录见 [目录地图](source-map/directory-map.md)。

## 模块矩阵

| 模块 | 生产源码 | 权威文档 | 主要测试证据 |
|---|---|---|---|
| Lexer | `src/compiler/lexer/`、`parser/token.hpp` | [lexer.md](../compiler/lexer.md) | `tests/unit/compiler/test_lexer_*` |
| Parser/AST | `src/compiler/parser/`、`ast.hpp` | [parser.md](../compiler/parser.md) | parser boundary/recovery/recursion tests |
| CodeGen | `src/compiler/codegen/`、register/jump helpers | [bytecode-generation.md](../compiler/bytecode-generation.md)、[codegen/](../compiler/codegen/) | emitter、binding、multret、jump tests |
| Opcode ABI | `src/compiler/opcode.*`、`src/vm/vm_handlers/` | [instruction-set.md](../vm/instruction-set.md) | `tests/unit/vm/opcode_coverage_matrix.md`、VM tests |
| VM call/dispatch | `src/vm/`、`src/vm/state/` | [vm/runtime/overview.md](../vm/runtime/overview.md) | `tests/unit/vm/`、call pipeline tests |
| Value/object | `src/core/value.*`、string/userdata | [runtime/value/overview.md](../runtime/value/overview.md) | `tests/unit/core/` |
| Table/metamethod | `src/core/table.*`、`metatable.*`、`vm_ops.cpp` | [runtime/table/overview.md](../runtime/table/overview.md) | table 与 metamethod tests |
| Function/closure | `src/core/function.*`、`upvalue.*`、`vm_frame.cpp` | [runtime/functions/overview.md](../runtime/functions/overview.md) | function、closure、tailcall tests |
| Runtime services | `src/runtime/`、`GlobalState` | [runtime/services.md](../runtime/services.md) | runtime-services tests |
| GC | `src/gc/`、各对象 `mark()` | [gc/implementation.md](../gc/implementation.md) | GC、weak table、finalizer tests |
| Standard library | `src/lib/`、`src/api/` | [stdlib/library-reference/overview.md](../stdlib/library-reference/overview.md) | stdlib 与 official scripts |
| Diagnostics | `src/common/lua_error.hpp`、`src/debug/`、debuglib | [debugging/overview.md](../debugging/overview.md) | parser recovery、trace、debuglib tests |
| Compatibility | 跨全部解释器模块 | [compatibility/lua51/overview.md](../compatibility/lua51/overview.md) | `tests/lua/official/`、regressions |

## 按改动意图定位

| 改动 | 首个责任区 | 必须联查 | 最小验证闭环 |
|---|---|---|---|
| 新/改语法 | lexer/parser/AST | CodeGen binding/lowering | parser unit + Lua regression |
| 新/改 opcode | opcode enum/encoding | CodeGen producer + handler consumer + docs | coverage matrix + handler behavior |
| 表达式/语句 lowering | emitter 与 result types | register allocator、jump patcher | unit invariant + bytecode + Lua behavior |
| CALL/RETURN/VARARG | CodeGen call result + `vm_call` | frame/top、native、tailcall | fixed/open matrix + regressions |
| Closure/upvalue | binding + `vm_frame` + Upvalue | scope close、异常展开、GC | shared identity + close paths |
| Table/metamethod | raw Table + VM semantic layer | call pipeline、GC barrier | raw test + metamethod script |
| GC edge/root | 对象 `mark()` 或 GlobalState roots | barrier、weak/finalizer、StringPool | unit graph + multi-cycle behavior |
| 标准库函数 | 对应 `src/lib/*` | stack delta、error object、GC root | C++ unit + Lua/official script |
| 错误/trace | error boundary 或 trace event | source/line、frame、Value serialization | structured assertions + regression |

## 跨模块审查清单

- 数据的 owner、root、strong/weak edge 与 observer 是否明确？
- Compiler 产生的布局/模式是否由 VM 完全消费？
- 可失败 API 是否使用分类 `std::expected`，深层异常是否在边界转换？
- 栈或容器扩容后是否仍持有失效引用？
- 修改对象图是否需要 write barrier？
- 测试是否同时覆盖内部不变量与 Lua 可观察行为？
- 文档是否更新唯一权威页，而不是新增第二份短说明？
