---
status: current
verified_against: docs/architecture/overview.md; docs/architecture/execution-pipeline/overview.md; docs/compiler/lexer.md; docs/compiler/parser.md; docs/compiler/bytecode-generation.md; docs/vm/runtime/overview.md; docs/runtime/value/overview.md; docs/runtime/table/overview.md; docs/runtime/functions/overview.md; docs/runtime/services.md; docs/runtime/execution-policy.md; docs/runtime/sandbox-policy.md; docs/runtime/memory-contract.md; docs/gc/implementation.md; docs/knowledge/source-document-map.md; src/compiler/; src/vm/; src/core/; src/gc/; tests/lua/official/
last_checked: 2026-07-15
applies_to: technical documentation entry point
---

# Lua 解释器技术实现百科

`docs/` 只收录架构、算法、运行时语义、兼容性边界、验证方法和源码映射。每个主题保留一个权威入口；项目状态、路线图、AI 指令、命令清单和按用例拆分的短页不进入百科。

## 技术模块

| 数据流阶段 | 内容 | 权威入口 |
|---|---|---|
| Architecture | 分层、端到端数据流、跨模块不变量和完整 walkthrough | [架构总览](architecture/overview.md)、[执行流水线](architecture/execution-pipeline/overview.md) |
| Compiler | Lexer、Parser、AST、binding、CodeGen、控制流、寄存器与字节码格式 | [Lexer](compiler/lexer.md)、[Parser](compiler/parser.md)、[字节码生成](compiler/bytecode-generation.md) |
| VM | opcode 语义、dispatch、寄存器窗口、调用/返回、native call 和 trace | [指令集](vm/instruction-set.md)、[VM Runtime](vm/runtime/overview.md)、[Trace](vm/trace-system.md) |
| Runtime | Value、Table、Metatable、Function、Closure、Upvalue、RuntimeServices、公开生产配置、执行治理、脚本能力与内存合同 | [Value](runtime/value/overview.md)、[Table](runtime/table/overview.md)、[Function](runtime/functions/overview.md)、[Services](runtime/services.md)、[生产运行时 C API](runtime/public-runtime-api.md)、[ExecutionPolicy](runtime/execution-policy.md)、[SandboxPolicy](runtime/sandbox-policy.md)、[内存合同](runtime/memory-contract.md) |
| Operations | 不可信脚本 worker 的 allocator/进程隔离、结构化结果、容量与回滚 | [生产部署合同](operations/production-deployment.md) |
| Quality | PR 快速门禁与 scheduled runtime/native-module soak、长 fuzz 分层 | [长稳与 Fuzz 合同](quality/endurance.md) |
| Release | RC/正式版本的同 SHA 证据、仓库治理、SBOM、校验和与回滚 | [发布门禁](release/release-checklist.md) |
| GC | root/object graph、mark/sweep、barrier、weak table、finalizer 和 StringPool | [GC 总览](gc/overview.md)、[GC 实现](gc/implementation.md)、[周期 walkthrough](gc/cycle-walkthrough.md) |
| Standard Library | native function 注册、公共栈协议和 Lua 5.1 库语义 | [注册架构](stdlib/overview.md)、[库实现边界](stdlib/library-reference/overview.md) |
| Compatibility | Lua 5.1 支持面、实现自由度、高风险差异和验证门槛 | [兼容性边界](compatibility/lua51/overview.md) |
| Diagnostics | 错误分类、阶段二分、trace、source/line、traceback 与交互式调试边界 | [诊断指南](debugging/overview.md)、[调试器架构](../../Debugger/docs/architecture.md)、[调试信息合同](../../Debugger/docs/debug-info-contract.md) |
| Testing | unit、Lua behavior、official、regression 与 golden 的证据分工 | [测试策略](testing/testing-strategy.md) |
| Source Map | 源码、文档与测试责任区 | [源码映射](knowledge/source-document-map.md)、[目录地图](knowledge/source-map/directory-map.md) |

## 建议阅读顺序

先读 [执行流水线](architecture/execution-pipeline/overview.md)，再沿真实数据流推进：

1. **Compiler**： [Lexer](compiler/lexer.md) → [Parser](compiler/parser.md) → [字节码生成](compiler/bytecode-generation.md) → [控制流 lowering](compiler/control-flow/overview.md)
2. **VM**： [指令编码](compiler/bytecode/instruction-format.md) → [指令集](vm/instruction-set.md) → [VM Runtime](vm/runtime/overview.md)
3. **Runtime**： [Value](runtime/value/overview.md) → [Table/Metatable](runtime/table/overview.md) → [Function/Upvalue](runtime/functions/overview.md)
4. **GC**： [对象图与阶段](gc/overview.md) → [算法实现](gc/implementation.md) → [完整周期](gc/cycle-walkthrough.md)

需要观察动态行为时再读 [Trace 系统](vm/trace-system.md) 和 [诊断指南](debugging/overview.md)。需要定位改动责任区时使用 [源码映射](knowledge/source-document-map.md)。

## 质量边界

- 事实必须由 `verified_against` 中仍存在的源码或测试支撑。
- 文档描述不变量和责任边界，不复制完整 API、命令帮助或测试清单。
- 同一事实只在一个主题页展开；其他页面使用链接，不维护第二份正文。
- 易变化的数量和进度只由自动化产生，不手工散落在技术页。
- C++ 示例区分 owning、root、GC edge 和 observer，不用“现代化”名义改变 Lua 对象语义。
