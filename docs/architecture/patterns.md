---
status: current
verified_against: src/compiler/ast_visitor.hpp; src/compiler/codegen/codegen.hpp; src/compiler/codegen/expression_emitter.hpp; src/compiler/codegen/statement_emitter.hpp; src/compiler/codegen/function_compiler.hpp; src/vm/vm_handlers.hpp; src/vm/vm_handlers.cpp; src/vm/vm_handlers/; src/vm/vm_dispatch_strategy.hpp; src/vm/vm_dispatch_strategy.cpp; src/runtime/runtime_services.hpp; src/vm/state/global_state.hpp; src/gc/garbage_collector.hpp; src/gc/gc_strategy.hpp; src/gc/gc_strategy.cpp; src/compiler/codegen/bytecode_builder.hpp; src/compiler/codegen/codegen_ops.hpp; src/compiler/codegen/codegen_state.hpp; src/lib/lib_catalog.hpp; src/lib/lib_catalog.cpp; src/lib/lib_manager.hpp
last_checked: 2026-05-23
applies_to: architecture pattern registry and implementation boundaries
---

# 架构模式

本文档记录了当前代码库中有意采用的设计模式。它是模式注册表，而非强制规范：新增代码应优先选择能保持解释器可读、可维护且适合教学的最简局部形态。

## 当前注册表

| 模式 | 状态 | 主要文件 | 当前角色 |
|---|---|---|---|
| Visitor（访问者） | 已实现 | `src/compiler/ast_visitor.hpp`, `src/compiler/codegen/expression_emitter.hpp`, `src/compiler/codegen/statement_emitter.hpp`, `src/repl/repl_meta.cpp` | CRTP 访问者封装 AST `std::variant` 分发。`ExprVisitor` 和 `StmtVisitor` 分别覆盖仅表达式和仅语句的消费者；`AstVisitor` 将两者组合，供 REPL AST 打印器等全树工具使用。 |
| Command（命令） | 已实现 | `src/vm/vm_handlers.hpp`, `src/vm/vm_handlers.cpp`, `src/vm/vm_handlers/` | VM 操作码行为由注册到 `HandlerTable` 中的自由函数处理器表示。表分发调用 `runHandler()` 而非对每条操作码直接 switch。 |
| Strategy（策略） | 已实现 | `src/vm/vm_dispatch_strategy.hpp`, `src/vm/vm_dispatch_strategy.cpp`, `src/gc/gc_strategy.hpp`, `src/gc/gc_strategy.cpp`, `src/runtime/runtime_services.hpp`, `src/vm/vm.cpp` | `DispatchStrategy` 选择 VM 执行算法；`GCStrategy` 选择回收器算法边界。`SwitchDispatch` 和 `MarkSweepGC` 为默认策略。 |
| Singleton（单例） | 兼容边界 | `src/vm/state/global_state.hpp`, `src/runtime/runtime_services.hpp`, `src/gc/garbage_collector.hpp` | `GlobalState` 仍保留单例支持以提供进程级运行时服务。新增的编译器、VM 和 GC 路径应优先使用显式服务传递。`GarbageCollector::getInstance()` 仅保留为已弃用的兼容垫片。 |
| Builder（构建器） | 已实现 | `src/compiler/codegen/bytecode_builder.hpp`, `src/compiler/codegen/codegen_ops.hpp`, `src/compiler/codegen/codegen_state.hpp` | `BytecodeBuilder` 是对活跃 `Proto` 进行变更的窄写入边界；`CodegenOps` 围绕该构建器集中管理重复发射、回填和保护机制。 |
| Catalog（目录） | 已实现 | `src/lib/lib_catalog.hpp`, `src/lib/lib_catalog.cpp`, `src/lib/lib_manager.hpp` | 标准库加载顺序和单库查找通过显式 `constexpr` 表以数据驱动方式实现；`openCatalogLibrary()` 是具名单库入口点。 |

## 模式边界

### Visitor（访问者）

AST 访问者层刻意保持精简。`ExprVisitor`、`StmtVisitor` 和组合的 `AstVisitor` 仅集中管理 variant 分发；其覆盖检查共享 `detail::canVisitNode()` 和 `detail::visitsVariantNodes()`。它们不拥有遍历策略、作用域状态、字节码发射或诊断逻辑。这些职责仍由 `CodeGenerator` 和 REPL AST 打印器等使用方持有。

新增 AST 消费者时，优先使用具体访问者类型，而非在 `CodeGenerator` 中添加更多分支。修改代码生成本身时，保持当前 `CodeGenerator` 的公开 API 稳定。

### Command（命令）

VM 命令模式通过函数指针表而非虚类层次实现。这使得操作码处理器易于审查，并避免了每个操作码的堆分配或继承管道。

注册文件 `src/vm/vm_handlers.cpp` 负责元数据初始化和处理器族注册。`src/vm/vm_handlers/` 下的文件按分组管理操作码行为。每个处理器分片应仅注册自己所属的操作码族。

### Strategy（策略）

`SwitchDispatch` 仍是默认的 VM 分发路径，因为它最易于调试且与解释器的历史控制流一致。`TableDispatch` 为可选路径，使用与命令层相同的处理器表。

不要添加计算 goto 或线程化代码分发路径。它们会牺牲可读性、可移植性和教学透明度。

`MarkSweepGC` 仍是默认的 GC 策略。`IncrementalGC` 以教学占位形式存在，委托给相同的标记-清除阶段，以便在未来的写屏障和调度引入之前，测试可以证明可达性等价。

### Singleton（单例）

`GlobalState` 仍然作为共享运行时服务的兼容锚点。迁移方向是通过 `RuntimeServices` 进行显式依赖传递，尤其是在编译器、VM、REPL、字节码工具和测试入口点。

当已有 `RuntimeServices&`、`LuaState*` 或 `GlobalState&` 可用时，新代码不应再调用 `GlobalState::getInstance()`。

### Builder（构建器）

`BytecodeBuilder` 限制了对 `Proto` 的直接写入，但它不是完整的编译器门面。下降决策仍属于 `CodeGenerator`；构建器应专注于发射机制和边界检查。

### Catalog（目录）

标准库目录是默认库加载顺序和库标识符的可读权威来源。使用 `StandardLibrary::openAll()` 加载全部库，使用 `StandardLibrary::openCatalogLibrary(L, "<id>")` 加载单个库。旧有的 `openBase()` / `openMath()` / ... 包装函数仅保留为已弃用的兼容垫片。

没有采用 `LibRegistrar` 静态自注册设计。静态注册器可以减少对 `lib_catalog.cpp` 的一次编辑，但会将加载顺序隐藏在动态初始化之后，并需要 MSVC 的链接器保活规则。对于教学导向的代码库，显式目录更易于阅读、测试和审计。

## 更新本文档

当模式被引入、移除或迁移到不同的源边界时，请按源码事实更新此注册表。
