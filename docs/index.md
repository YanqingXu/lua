---
status: current
verified_against: docs/architecture/overview.md; docs/compiler/bytecode-generation.md; docs/vm/instruction-set.md; docs/runtime/services.md; docs/gc/implementation.md; docs/stdlib/overview.md; docs/compatibility/lua51/compare-with-official-lua.md; docs/testing/testing-strategy.md; docs/knowledge/source-document-map.md
last_checked: 2026-07-11
applies_to: technical documentation entry point
---

# Lua 解释器技术实现百科

`docs/` 是项目唯一的文档根目录，只收录解释器架构、算法、运行时语义、兼容性边界、测试方法和源码定位资料。项目状态、路线图、协作流程、AI 指令以及单纯的工具操作说明不属于本知识库。

## 技术模块

| 模块 | 内容 | 推荐入口 |
|---|---|---|
| Architecture | 整体分层、执行流水线、关键概念和设计模式 | [architecture/overview.md](architecture/overview.md)、[architecture/execution-pipeline/overview.md](architecture/execution-pipeline/overview.md) |
| Compiler | Lexer、Parser、AST、作用域、CodeGen、寄存器、控制流和字节码格式 | [compiler/frontend/overview.md](compiler/frontend/overview.md)、[compiler/bytecode-generation.md](compiler/bytecode-generation.md) |
| VM | 指令集、分发循环、寄存器模型、调用帧、native call 和 trace | [vm/instruction-set.md](vm/instruction-set.md)、[vm/runtime/overview.md](vm/runtime/overview.md) |
| Runtime | `Value`、Table、Metatable、Function、Closure、Upvalue 和运行时服务 | [runtime/value/overview.md](runtime/value/overview.md)、[runtime/table/overview.md](runtime/table/overview.md)、[runtime/functions/overview.md](runtime/functions/overview.md) |
| GC | 对象生命周期、分配、引用图、标记清除、弱表、字符串池和回收周期 | [gc/overview.md](gc/overview.md)、[gc/implementation.md](gc/implementation.md) |
| Standard Library | 库注册架构和各标准库的实现边界 | [stdlib/overview.md](stdlib/overview.md)、[stdlib/library-reference/overview.md](stdlib/library-reference/overview.md) |
| Compatibility | 与 Lua 5.1 的语言、运行时、库和实现策略差异 | [compatibility/lua51/overview.md](compatibility/lua51/overview.md) |
| Testing | 单元、Golden、回归和兼容性测试方法 | [testing/testing-strategy.md](testing/testing-strategy.md) |
| Debugging | 编译错误、运行时错误、源码位置、调用栈、字节码和 VM trace | [debugging/overview.md](debugging/overview.md)、[debugging/diagnostic-workflow.md](debugging/diagnostic-workflow.md) |
| Reference | 术语以及源码、文档和测试的映射 | [glossary.md](glossary.md)、[knowledge/source-document-map.md](knowledge/source-document-map.md) |

## 建议阅读顺序

理解一次 Lua 源码执行：

1. [执行流水线总览](architecture/execution-pipeline/overview.md)
2. [Lexer 与 Parser](compiler/frontend/overview.md)
3. [字节码生成](compiler/bytecode-generation.md)
4. [VM 运行时](vm/runtime/overview.md)
5. [值与对象系统](runtime/value/overview.md)

深入某个子系统时，先读该模块的 `overview.md`，再按数据结构、控制流和测试证据进入专题文档。跨模块定位使用 [源码与文档映射](knowledge/source-document-map.md)。

## 收录边界

- 文档必须解释源码结构、数据模型、算法、语义或验证方法。
- 易变化的测试数量、完成百分比和阶段计划不在 `docs/` 中维护。
- 命令行参数清单、构建步骤和脚本使用方式由可执行程序的 `--help`、根 README 或脚本自身负责。
- 同一技术主题优先在所属模块维护，避免按语言、作者或生成来源建立第二套文档树。
