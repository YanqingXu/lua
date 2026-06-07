# Lua Interpreter Documentation

> **从 Lua 源码输入 → 编译/解释 → VM 执行 → 运行时对象 → 标准库/工程化 → 测试验证**

本文档帮助理解、维护和继续开发本项目。它不是项目 README，而是**文档导航页**。

## 推荐阅读顺序

1. [00-project-overview/00-architecture.md](00-project-overview/00-architecture.md) — 项目整体架构
2. [01-execution-pipeline/08-full-trace-example.md](01-execution-pipeline/08-full-trace-example.md) — Lua 源码执行链路
3. [02-source-code-map/00-directory-map.md](02-source-code-map/00-directory-map.md) — 源码目录地图
4. [03-lexer-parser/00-overview.md](03-lexer-parser/00-overview.md) — Lexer / Parser
5. [04-bytecode-compiler/00-overview.md](04-bytecode-compiler/00-overview.md) — Bytecode / Compiler
6. [05-vm-runtime/00-overview.md](05-vm-runtime/00-overview.md) — VM Runtime
7. [06-value-object-system/00-overview.md](06-value-object-system/00-overview.md) — Value / Object System
8. [07-table-metatable/00-overview.md](07-table-metatable/00-overview.md) — Table / Metatable
9. [08-function-call-closure/00-overview.md](08-function-call-closure/00-overview.md) — Function / Closure / Upvalue
10. [09-control-flow/00-overview.md](09-control-flow/00-overview.md) — 控制流语义
11. [10-stdlib/00-overview.md](10-stdlib/00-overview.md) — 标准库
12. [11-error-debug-trace/00-overview.md](11-error-debug-trace/00-overview.md) — 错误处理与调试
13. [12-gc-memory/00-overview.md](12-gc-memory/00-overview.md) — GC 与内存管理
14. [13-compatibility/00-lua51-compatibility-goal.md](13-compatibility/00-lua51-compatibility-goal.md) — Lua 5.1 兼容性
15. [14-testing/00-testing-strategy.md](14-testing/00-testing-strategy.md) — 测试体系
16. [15-engineering-guide/00-build-and-run.md](15-engineering-guide/00-build-and-run.md) — 工程开发指南
17. [16-learning-notes/00-my-mental-model.md](16-learning-notes/00-my-mental-model.md) — 个人学习笔记

## 文档目标

- **看懂项目主流程** — 从源码到执行结果的全链路追踪
- **看懂每个核心模块职责** — 每个模块的输入、输出、数据结构、调用链
- **能定位 Bug 属于哪个阶段** — Lexer / Parser / Compiler / VM / Runtime
- **能独立新增 Lua 语法/VM 指令/标准库函数** — 工程开发指南
- **能验证项目与 Lua 5.1 的兼容性** — 兼容性矩阵和 Golden Test

## 文档分类

### A. 理解型文档 — 帮助你看懂项目

| 目录 | 说明 |
|------|------|
| [00-project-overview/](00-project-overview/) | 架构总览、设计目标、当前状态 |
| [01-execution-pipeline/](01-execution-pipeline/) | 完整执行链路追踪 |
| [02-source-code-map/](02-source-code-map/) | 源码目录地图与定位指南 |
| [06-value-object-system/](06-value-object-system/) | Value/Table/Function 对象系统 |
| [08-function-call-closure/](08-function-call-closure/) | 闭包与 Upvalue 模型 |
| [05-vm-runtime/](05-vm-runtime/) | VM 主循环与栈布局 |

### B. 维护型文档 — 帮助你改项目

| 目录 | 说明 |
|------|------|
| [11-error-debug-trace/](11-error-debug-trace/) | Bug 排查指南 |
| [15-engineering-guide/](15-engineering-guide/) | 如何新增 Opcode / 标准库函数 / 语法特性 |
| [02-source-code-map/](02-source-code-map/) | 改动位置指南 |

### C. 证明型文档 — 帮助你证明项目成熟度

| 目录 | 说明 |
|------|------|
| [13-compatibility/](13-compatibility/) | Lua 5.1 兼容性矩阵 |
| [14-testing/](14-testing/) | Golden Test、回归测试策略 |
| [12-gc-memory/](12-gc-memory/) | 已知限制 |

## 快速入口

- **新人第一份文档** → [01-execution-pipeline/08-full-trace-example.md](01-execution-pipeline/08-full-trace-example.md)
- **想改代码不知道在哪** → [02-source-code-map/05-change-location-guide.md](02-source-code-map/05-change-location-guide.md)
- **遇到 Bug 不知道怎么排查** → [11-error-debug-trace/08-troubleshooting-guide.md](11-error-debug-trace/08-troubleshooting-guide.md)
- **想了解当前能力边界** → [13-compatibility/01-language-feature-matrix.md](13-compatibility/01-language-feature-matrix.md)
