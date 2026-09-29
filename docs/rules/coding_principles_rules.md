---
status: current
verified_against: docs/architecture/overview.md; docs/architecture/patterns.md; docs/runtime/services.md; docs/runtime/execution-policy.md; docs/runtime/memory-contract.md; docs/gc/overview.md; docs/testing/testing-strategy.md; src/runtime/lua_allocator.hpp; src/common/types.hpp
last_checked: 2026-09-29
applies_to: design and review principles for repository-owned code and scripts, with C++ ownership constraints scoped to C++
---

# 编码原则

范围、采用节奏和等级见 [规则入口](README.md)。以下原则用于解释修改理由；布局、命名与重构细节分别由对应规则管理。

### R-PRINCIPLES-01 表达真实意图和副作用

- **等级：MUST。**
- **范围：** 新增或修改的接口、状态、表达式和脚本流程。
- **要求：** 名称区分检查、转换、执行与提交；单位、值域、错误状态和有意义的副作用可从接口或合同读出。查询名称不能隐藏未说明的写操作。变量靠近使用点，保持单一角色。
- **例外：** Lua 元方法或兼容 API 可能在看似读取时执行用户代码；保留既有行为并明确该边界，不能为满足命名偏好禁止回调。
- **验证：** 人工沿调用路径检查状态变化、求值次数、短路顺序和失败时机；变换表达式时遵守 [重构规则](refactoring_rules.md)。

### R-PRINCIPLES-02 职责与依赖保持可见

- **等级：SHOULD。**
- **范围：** 模块、函数和新抽象。
- **要求：** 按变化原因与知识归属组织代码；调用方只接收完成职责所需的数据和能力。已有 `RuntimeServices`、State 或 context 时沿显式依赖传递，不新增隐式单例访问。新抽象说明当前复用、变化轴、测试或风险隔离价值。
- **例外：** 已有兼容入口和独立初始化边界可以保留，但不得扩散为普通业务依赖；固定流程可以集中表达。
- **验证：** 人工检查调用方向与实际使用点，对照 [运行时服务](../runtime/services.md)、[架构模式](../architecture/patterns.md)；行数或模式数量不作为质量证明。

### R-PRINCIPLES-03 先明确所有权，再选择类型

- **等级：MUST。**
- **范围：** C++ 资源、GC 对象、引用、指针、视图及回调捕获。
- **要求：** 保持 owning、GC root、GC edge 与 observer 的区别，说明借用有效期和失效条件。资源清理采用适合其合同的 RAII；检查复制、移动、异常和提前返回。allocator 路由与 deleter 是类型语义的一部分。
- **例外：** GC 非拥有裸指针、C ABI 句柄及内部明确的创建/销毁边界可以保留；`LuaVector`、`LuaOwnedPtr` 等自定义 allocator/deleter 类型不能改为默认 `Vec`/`UPtr`。不能给 GC edge 套共享所有权。
- **验证：** 对照 [GC 模型](../gc/overview.md) 与 [内存合同](../runtime/memory-contract.md)，检查 root/barrier、释放方和失败回滚；涉及资源行为时运行相应 GC、allocator 或生命周期测试。

### R-PRINCIPLES-04 失败保证属于接口

- **等级：MUST。**
- **范围：** 可能失败的分配、加载、执行、资源治理和外部接入。
- **要求：** 明确失败如何报告、失败后哪些状态仍有效、是否可重试；保留 protected C API 的错误转换和栈恢复。外部输入用运行时校验，不用仅在调试版启用的断言替代。错误上下文足以定位问题，输出有界。
- **例外：** 明确的内部不变量可使用断言；已有容错或降级属于合同的，保留原终态并写清原因。
- **验证：** 检查正常与失败出口，按改动选择边界值、OOM 或重试验证；不把吞错、默认成功或重置状态当作结构简化。

### R-PRINCIPLES-05 保留重入、协程与执行窗口

- **等级：MUST。**
- **范围：** VM、C callback、coroutine、执行治理及其适配代码。
- **要求：** 保持 Lua→C→Lua 重入、yield/resume、错误传播与调用帧恢复合同。instruction/native-work 等窗口预算按所属 context 共享；重入和 yield 不隐式重置预算。线程访问与取消能力遵守现有执行治理合同。
- **例外：** 显式开启下一执行窗口和合同定义的 per-drain finalizer 预算具有各自语义，不能按统一重置规则处理。
- **验证：** 以 [执行治理](../runtime/execution-policy.md)、[VM Runtime](../vm/runtime/overview.md) 为依据，选择重入、yield、预算或失败路径测试；不得引入 Hunter 的“禁止同步重入”限制。

### R-PRINCIPLES-06 同一知识保持一个权威来源

- **等级：SHOULD。**
- **范围：** 重复逻辑、类型定义、生成数据和合同文档。
- **要求：** 需要同步改变的知识收敛到明确来源，派生信息通过生成、共享实现或一致性检查维护。通用类型来源为 [types.hpp](../../src/common/types.hpp)；语义文档引用既有权威入口。
- **例外：** 仅语法相似但变化原因不同的逻辑可以独立；测试 oracle 需要独立实现时保留其独立性和依据。
- **验证：** 追踪一次规则变更的使用方和生成链，人工检查遗漏与隐藏副本；不为去重削弱测试独立性。

### R-PRINCIPLES-07 用受影响行为证明修改

- **等级：MUST。**
- **范围：** 实现与重构的完成声明。
- **要求：** 按 [测试策略](../testing/testing-strategy.md) 选择能暴露本次风险的检查；区分已实现、已运行验证和未验证。行为修复保留最小回归证据，热点优化需要相应测量。
- **例外：** 纯文档或无语义排版只需相应引用、格式与差异检查，不为之新增镜像实现的测试。
- **验证：** 审阅真实命令、范围和结果；编译、格式或有限 smoke 通过不单独证明行为等价或全仓合规。
