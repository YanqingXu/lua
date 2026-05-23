# Lua 解释器优化与重构路线图

> 适用范围：`g:\github\lua`（现代 C++ Lua 5.1.5 解释器）
> 设计目标：**可读性 > 可维护性 > 教育价值 > 性能**
> 约束：保持 543 个注册测试 / 2730 个断言结果 / 0 失败，不破坏 `LuaState` / `VM` public API。
> 最近审计：2026-05-21（深度审计报告，覆盖 Readability / Extensibility / Educational Value 三维度）
> 最近同步：2026-05-23（PR-69：Visitor 内部 `canVisit*` 检查去重）

---

## 审计总览（2026-05-21）

| 维度 | 评分 | 关键优势 | 关键改进点 |
|---|---|---|---|
| **可读性** | **A-** | CRTP+concept 编译期检查、`std::expected` 边界清晰、VM 主循环已精简为策略入口 + handler table | `ValueResult` 已有 variant prototype 但旧兼容字段仍待迁移、`CodeGenerator` 方法数 60+、部分命名继承 Lua C 缩写 |
| **易扩展性** | **B+** | Visitor 模式添加新工具零摩擦、`DispatchStrategy` 可插拔、`HandlerTable` 按组注册，`GCStrategy` 已把 collector 算法边界显式化 | GC/metatable 兼容 fallback 仍需收口、`lib_manager.hpp` 冗余声明、`CodeGenerator` facade 仍可继续瘦身 |
| **教学价值** | **A-** | hello-world / closure-and-upvalue / gc-cycle walkthrough 已覆盖端到端执行、闭包生命周期和完整 GC 周期，glossary 降低认知负担，trace 系统层次分明且已有差异模式与 JSONL golden，REPL 已能打印 AST、bytecode 与 GC 状态，支持 Tab 探索全局名和库字段，并在终端中高亮错误、展示行号 prompt 和覆盖增量解析行为，`lua_bytecode --cfg` 可直接生成 Mermaid CFG | 后续可继续补样例和文档深挖 |

各维度详细评估见本文档对应阶段的任务标注；已完成项以 ✓ 标记。

---

## 阶段划分总览

| 阶段 | 周期定位 | 关注点 | 是否触发 API 变化 | 完成度 |
|---|---|---|---|---|
| **阶段 1 — 短期代码清理** | 1–2 个迭代 | 删冗余、统一命名、收紧错误处理、文档与代码同步 | 否 | ~95% |
| **阶段 2 — 中期模式重构** | 3–5 个迭代 | 引入访问者 / 策略 / 命令模式，VM dispatch 与 GC 抽象化 | 内部 API 调整、public 不变 | ~93% |
| **阶段 3 — 现代 C++ 特性** | 穿插于 1 & 2 | `std::expected` / `concepts` / `std::format` / `[[nodiscard]]` | 部分 public 签名变更 | ~90% |
| **阶段 4 — 教育价值增强** | 持续推进 | 自解释代码、字节码可视化、执行链路教学文档、REPL 体验、Trace 差异模式 | 否（增量增强） | ~88% |
| **阶段 5 — 工程实践** | 持续 | 测试覆盖、质量门、构建一致性、文档漂移检测 | 否 | ~82% |

---

## 阶段 1：短期代码清理（1–2 个迭代）

### 1.1 架构边界微调

**现状**

- `src/compiler/` 已按 `parser_*.cpp` / `codegen_binding|expr|jump|stmt.cpp` 分片，`codegen_state.hpp` / `codegen_context.hpp` 已隔离状态。✓
- `src/vm/vm.cpp` 已从 ~2000 行 switch 精简为 ~327 行，handler 实现分入 `vm_handlers/` 子目录（9 个分组文件）。✓
- `src/gc/garbage_collector.cpp` 已拆分为 `gc_mark.cpp` / `gc_sweep.cpp` / `gc_finalize.cpp` / `gc_weak.cpp`。✓

**2026-05-22 重新排序**：`1.1.1` 已作为 PR-39 完成，因为它直接决定 `SwitchDispatch` 与 `TableDispatch` 的教学差异；`1.3.1` 已作为 PR-40 完成，把 VM expected 边界的异常映射集中到可复用 helper；`2.5.1-a` 已作为 PR-41 完成；`2.5.1-b` 已作为 PR-42 完成；`2.5.1-c` 已作为 PR-43 完成；`2.5.1-d` 已作为 PR-44 完成；`2.5.1-e` 已作为 PR-45 完成。命名类清理继续跟随 CodeGenerator 拆分推进，避免在同一批代码上重复改名。

**审计发现**：

2026-05-21 审计发现 `vm.cpp` 的 Switch dispatch 路径将所有 38 个 opcode 归入一个 fall-through case 块，统一调用 `runCurrentHandler()` lambda。这与 Table dispatch 路径的实际行为等价——两种 `DispatchBackend` 的区别仅在是否经过 `switch(op)` 语句，调试时无法通过单步进入区分具体 opcode 逻辑。

2026-05-22 PR-39 已修正该偏差：`src/vm/vm_switch_dispatch.hpp` 提供每 opcode 一个 inline helper，`src/vm/vm.cpp` 的 switch case 逐个调用 `execOp*()`，Table 后端继续通过 handler table 展示命令模式。

**任务**

| 编号 | 任务 | 文件 | 验证 | 状态 |
|---|---|---|---|---|
| 1.1.1 | 把 Switch dispatch 路径的 38 个 `case` 改为每 opcode 调用独立 inline 函数（如 `execOpAdd(ctx, inst)`），使调试器单步可直接定位到具体 opcode | `src/vm/vm.cpp` / `src/vm/vm_switch_dispatch.hpp` | `test_vm_dispatch.cpp` 通过；调试构建下 `SwitchDispatch` 单步体验与 `TableDispatch` 可区分 | ✓ **已完成** |
| 1.1.2 | 拆分 `garbage_collector.cpp`，按 `gc_mark.cpp` / `gc_sweep.cpp` / `gc_finalize.cpp` / `gc_weak.cpp` 分片 | `src/gc/` | GC 相关测试全绿 | ✓ **已完成** |
| 1.1.3 | 同步 `docs/architecture/overview.md` 与 `docs/architecture/runtime-services.md`；`RuntimeServices` 结构体示例必须包含 `VM::DispatchStrategy* dispatchStrategy` 字段 | `docs/` | `check_doc_drift.ps1` 通过 | ✓ **已完成** |

### 1.2 命名与可读性收紧

- GC 内部成员命名 `allObjects_` / `roots_` / `grayList_` 已是 trailing-underscore 风格。✓
- 删除 `ExprDesc` / `ExprKind` 历史残留：`check_doc_drift.ps1` 已覆盖扫描。✓
- ✓ **已完成 — PR-49**：`CodegenState` 内部成员已从 `regs` / `locals` / `blocks` / `upvalues` 收口为 `registers` / `localScope` / `blockManager` / `upvalueContext`；`LocalVarScope::nactvar_` 与 `BlockInfo::nactvar` 已改为 `activeVarCount_` / `activeVarCount`。
- ✓ **已完成 — PR-67**：`Parser::tokenString` 已从 `Parser` 类声明中移出，集中为 `src/compiler/parser/parser_utils.hpp` 的 `ParserUtils::tokenString()`；`parser_*.cpp` 分片显式依赖该无状态 helper，`Parser Boundary Sentinels` 继续锁住借用型 `StrView` 返回边界。

CodeGenerator 状态命名已在 emitter 边界稳定后完成，`tokenString` 也已完成低风险收尾，阶段 1 后续只剩更低优先级的跨阶段清理项。

### 1.3 错误处理收紧

**现状**：统一异常层级 `LuaError` ← `ParseError` / `CodegenError` / `RuntimeError` / `MemoryError` 已集中在 [lua_error.hpp](src/common/lua_error.hpp)。✓
`std::expected<T, E>` 已在 `CodeGenerator::tryGenerate()` 和 `VM::tryExecuteProto()` 落地。✓

**审计发现**：`tryExecuteProto(LuaState*, ...)` 已委托给 context-aware 重载，重复 catch 链不再出现在两个函数体内；但 `tryExecuteProto(RuntimeServices&, ...)` 曾内联异常到 `RuntimeError` 的映射规则。PR-40 已将该规则提取为 `VM::detail::captureRuntimeErrors<T>()` / `mapExceptionToUnexpected()`，后续新增 `std::expected` 边界时可复用同一规则：

```cpp
template <typename T, typename Fn>
static std::expected<T, RuntimeError> captureRuntimeErrors(Fn&& fn) {
    try {
        return fn();
    } catch (const std::bad_alloc&) {
        throw;
    } catch (const RuntimeError& error) {
        return std::unexpected(RuntimeError(error.what()));
    } catch (const LuaError& error) {
        return std::unexpected(RuntimeError(error.what()));
    } catch (const std::exception& error) {
        return std::unexpected(RuntimeError(error.what()));
    }
}
```

**任务**：

| 编号 | 任务 | 文件 | 验证 | 状态 |
|---|---|---|---|---|
| 1.3.1 | 提取 `captureRuntimeErrors` / `mapExceptionToUnexpected`，集中 `tryExecuteProto` 的异常映射规则 | `src/vm/vm.cpp` / `src/vm/vm_internal.hpp` | `Runtime Services` + 全量测试通过 | ✓ **已完成** |

---

## 阶段 2：中期模式重构（3–5 个迭代）

### 2.1 访问者模式（Visitor）应用到 AST ✓ 已完成

[ast_visitor.hpp](src/compiler/ast_visitor.hpp) 已实现 CRTP `ExprVisitor<Derived, R>` + `StmtVisitor<Derived, R>`，并在 PR-68 新增 `AstVisitor<Derived, R>` 组合模板与 `VisitsAstNodes` concept；PR-69 已将节点可访问性检查收口到 `detail::canVisitNode()` / `detail::visitsVariantNodes()`，让公开 concepts 与 visitor 入口共享同一套 variant 覆盖逻辑。`CodeGenerator` 通过 `ExprVisitor<CodeGenerator, ValueResult>` 继承获得分发能力，REPL AST printer 使用 `AstVisitor<AstPrinter>` 覆盖表达式与语句树。

**审计评价（A 级）**：`detail::visitsVariantNodes()` 使用 fold expression 遍历 variant 的 `std::index_sequence`，确保遗漏任何 AST 节点类型都会产生编译期 `static_assert` 错误——这对教学项目极有价值。

**审计建议**：

| 编号 | 任务 | 说明 | 状态 |
|---|---|---|---|
| 2.1.1 | 提供 `AstVisitor<Derived, R>` 组合模板 | 继承 `ExprVisitor + StmtVisitor`，减少新 visitor 的样板代码；`AstPrinter` 已迁移为真实使用点 | ✓ **已完成 — PR-68** |
| 2.1.2 | 复用 `detail::visitsVariantNodes`，减少 `ExprVisitor` / `StmtVisitor` 内部 `canVisit*` 重复 | `ExprVisitor` / `StmtVisitor` 已删除各自的 `canVisitNode` / `canVisitAll` 私有重复实现；私有 emitter 通过精确 friend 保持封装 | ✓ **已完成 — PR-69** |

### 2.2 命令模式 + 策略模式 应用到 VM Dispatch ✓ 已完成

- `HandlerTable`（`std::array<HandlerEntry, 38>`）已实现，9 个 `register*Handlers()` 函数按 opcode 组注册。✓
- `DispatchStrategy` 抽象基类 + `SwitchDispatch` / `TableDispatch` 实现。✓
- 默认策略仍为 `SwitchDispatch`，保留调试兼容性。✓

**审计发现**：Switch 和 Table 两种 `DispatchBackend` 曾经在实现中功能等价——两者都调用 `runCurrentHandler()` lambda。区别仅在于 Switch 路径经过 `switch(op)` 语句作为 fallback，实际行为一致。这降低了两种策略的教学对比价值。

**PR-39 结果**：1.1.1 完成后，Switch 路径每个 case 调用独立 inline 函数（不再通过 `runCurrentHandler()`），使两种策略产生实质性差异：
- `SwitchDispatch`：每 opcode 独立内联函数，调试友好
- `TableDispatch`：函数指针表，展示命令模式

### 2.3 策略模式应用到 GC

**现状**

`GlobalState` 已拥有当前主路径使用的 `GarbageCollector`，`RuntimeServices::fromSingletons().gc` 指向 `GlobalState::getInstance().getGC()`；PR-46 后 `GarbageCollector::getInstance()` 仍保留为 `[[deprecated]]` 旧兼容 shim。`GarbageCollector::sweep(StringPool&)` 和 `clearAll(StringPool&)` 已显式接收字符串池，sweep / clearAll 删除 `GCString` 时不再在清扫逻辑中直接调用 `StringPool::getInstance().remove()`。

**单例残留清单（2026-05-21 审计）**：

| 位置 | 调用 | 风险等级 |
|---|---|---|
| [gc_sweep.cpp](src/gc/gc_sweep.cpp) | `sweep(StringPool&)` 显式接收字符串池 | ✓ 已完成 — PR-46 |
| [garbage_collector.cpp](src/gc/garbage_collector.cpp) | `clearAll(StringPool&)` 显式接收字符串池；无上下文旧入口才走兼容 fallback | ✓ 已完成 — PR-46 |
| [gc_finalize.cpp:23](src/gc/gc_finalize.cpp#L23) | `GlobalState::getInstance()` (fallback) | 低 — 已有 `globalState_` 成员优先 |
| [gc_weak.cpp:23](src/gc/gc_weak.cpp#L23) | `GlobalState::getInstance()` (fallback) | 低 — 同上 |
| [metatable.cpp](src/core/metatable.cpp) (4 处) | `GlobalState::getInstance()` | 低 — 静态辅助函数无上下文时的 fallback |
| [lua_state.cpp:88](src/vm/state/lua_state.cpp#L88) | `GlobalState::getInstance()` (默认构造) | 低 — 仅默认构造函数路径 |

**重构目标**

1. ✓ **已完成**：主运行时 GC 所有权迁入 `GlobalState`，`RuntimeServices` 通过显式 `GlobalState&` 取得 GC。
2. ✓ **已完成**：去单例化收尾，`GarbageCollector::sweep(StringPool&)` 接收显式字符串池；`clearAll(StringPool&)` 同步收口；旧 `GarbageCollector::getInstance()` 标记为 `[[deprecated]]`。
3. ✓ **已完成 — PR-62**：抽象 `GCStrategy`（保留教学预留）：

   ```cpp
   class GCStrategy {
   public:
       virtual ~GCStrategy() = default;
       virtual usize collect(GCContext& ctx) const = 0;
       virtual const char* name() const = 0;
   };
   class MarkSweepGC : public GCStrategy { ... };   // 当前实现
   class IncrementalGC : public GCStrategy { ... }; // 教学占位，当前委托 mark-sweep 保持语义等价
   ```

4. ✓ **已完成 — PR-62**：标准库新增 `collectgarbage("strategy")` 查询当前策略，并支持 `collectgarbage("strategy", "mark-sweep" | "incremental")` 切换教学策略边界。

### 2.4 编译期/编辑期模式登记 ✓ 已完成

- `docs/architecture/patterns.md` 已记录 Visitor、Command + Strategy、Singleton、Builder 等模式。✓

### 2.5 审计新增：CodeGenerator 职责拆分

**审计发现**：[codegen.hpp](src/compiler/codegen/codegen.hpp) 暴露约 60 个 private/protected 方法。虽然实现已按文件分片（`codegen_binding/expr/jump/stmt.cpp`），但类声明本身仍是"上帝类"——方法涵盖指令生成、寄存器分配、常量表管理、局部变量管理、跳转回填、表达式降低、语句降低、块管理、函数编译等 9 类职责。

**重新排序**：`CodeGenerator` 拆分是 2026-05-21 审计后最重要的中期重构，但不应直接一次性拆完整个类。已先做职责地图和 characterization 测试，再按低耦合到高耦合的顺序抽取：`JumpPatcher`、`ScopeManager`、`ExpressionEmitter`、`StatementEmitter`。PR-48 已在稳定边界上完成 `ValueResult -> std::variant` 兼容式 prototype；后续可继续迁移调用点到 `std::visit`，或先做低风险命名清理。

**建议**：将 `CodeGenerator` 拆分为独立策略对象，共享 `CodegenState`：

```text
CodeGenerator (orchestration facade)
  ├── ExpressionEmitter  (emitValue / emitCond / emitLValue 通道)
  ├── StatementEmitter   (13 种语句降低)
  ├── JumpPatcher        (回填链表管理)
  └── ScopeManager       (块/局部变量/upvalue 作用域)
```

这同时解决"一个类声明 60+ 方法"的可读性问题，且各 emitter 可独立单测。

| 编号 | 子任务 | 说明 | 状态 |
|---|---|---|---|
| 2.5.1-a | 职责地图 + 现有行为锁定 | 已新增 `docs/compiler/codegen-responsibility-map.md` 与 `Codegen Characterization` 测试，锁住 statement / jump / repeat scope / generic-for 行为 | ✓ **已完成** |
| 2.5.1-b | 抽取 `JumpPatcher` | 已新增 `jump_patcher.hpp/.cpp`，切出 jump-list、pending `jpc_`、`PatchList` 和 `fixJump/getJump`，`CodeGenerator` 保留薄包装 | ✓ **已完成** |
| 2.5.1-c | 抽取 `ScopeManager` | 已新增 `scope_manager.hpp/.cpp`，切出 locals / blocks / upvalues 作用域生命周期、breaklist 接入、scope close 和 upvalue 查找；`CodeGenerator` 保留薄包装 | ✓ **已完成** |
| 2.5.1-d | 抽取 `ExpressionEmitter` | 已新增 `expression_emitter.hpp/.cpp`，切出 `ValueResult` / `CondResult` / `CallResultInfo` / `LValueRef` 表达式通道；`CodeGenerator` 保留薄包装 | ✓ **已完成** |
| 2.5.1-e | 抽取 `StatementEmitter` | 已新增 `statement_emitter.hpp/.cpp`，承载 `statement()`、各 `emitStmt()`、`block()` 和控制流 / local / return / function / loop lowering；`CodeGenerator` 保留薄包装 | ✓ **已完成** |

### 2.6 审计新增：标准库声明式注册

**现状**：`lib_catalog.cpp` 的 `constexpr std::array<LibCatalogEntry, 9>` 已实现表驱动注册，但 `lib_manager.hpp` 仍保留 9 个独立的 `openBase()` / `openMath()` / ... 方法声明——这些只是对 `openCatalogLibrary(L, "base")` 的薄封装，属于冗余的便利包装。

**建议方案**（声明式自注册）：

```cpp
// lib_catalog.hpp
struct LibRegistrar {
    LibRegistrar(const char* id, const char* name, LibOpenFunction open);
};

// lib_catalog.cpp
static Vec<LibCatalogEntry>& mutableCatalog() {
    static Vec<LibCatalogEntry> catalog;
    return catalog;
}

// 在每个 lib/*.cpp 文件末尾：
namespace { LibRegistrar kReg("base", "Base Library", openBaseLib); }
```

添加新库只需创建 `.cpp` 文件，无需修改任何注册中心文件。注意：MSVC `/OPT:REF` 可能丢弃未引用的静态对象，需添加 `#pragma comment(linker, "/include:...")`。

| 编号 | 任务 | 说明 | 状态 |
|---|---|---|---|
| 2.5.1 | `CodeGenerator` 拆分为 ExpressionEmitter / StatementEmitter / JumpPatcher / ScopeManager | 共享 CodegenState；2.5.1-a/b/c/d/e 已完成，后续剩余是 facade 瘦身与类型表达升级 | ✓ **已完成（核心拆分）** |
| 2.6.1 | `lib_manager.hpp` 的 9 个 `openXxx()` 标记 `[[deprecated]]`，引导调用方使用 `openCatalogLibrary()` | 消除冗余声明 | P3 待执行 |
| 2.6.2 | 引入 `LibRegistrar` 声明式自注册，消除 #include 耦合 | 可选：注意 MSVC linker 行为 | P4 候选 |

---

## 阶段 3：现代 C++ 特性升级（穿插于阶段 1 & 2）

> 项目 toolchain 是 MSVC + VS 2026，已具备 C++23 能力。**升级原则：只在能显著简化代码时引入，不为新语法而新语法。**

### 3.1 `std::expected<T, LuaError>` 替换部分异常 ✓ 基本完成

| 当前签名 | 状态 |
|---|---|
| `std::expected<Chunk, ParseError> Parser::parse()` | ✓ 已完成 |
| `std::expected<Proto*, CodegenError> CodeGenerator::tryGenerate()` | ✓ 已完成 |
| `std::expected<ExecResult, RuntimeError> VM::tryExecuteProto()` | ✓ 已完成 |

**保留异常的位置**：REPL 顶层、`std::bad_alloc` 等真正异常路径。

**PR-40 结果**：见 1.3.1 —— `tryExecuteProto` 的 catch 链已收口到 `VM::detail::captureRuntimeErrors<T>()`，后续 VM expected 边界应复用该 helper。

### 3.2 `concepts` 约束模板 ✓ 已完成

[ast_visitor.hpp](src/compiler/ast_visitor.hpp) 的 concept 实现是项目中 C++23 特性应用的典范：

- `VisitsNode<Visitor, Node>` — 单节点约束
- `VisitsNodeAs<Visitor, Node, R>` — 区分 void / non-void 返回值
- `VisitsExprNodes<Visitor, R>` / `VisitsStmtNodes<Visitor, R>` — 编译期全量节点覆盖检查

### 3.3 `std::format` 替换 `std::ostringstream` ✓ 已完成

`src/debug/*.cpp`、`src/repl.cpp`、`src/bytecode/*.cpp`、标准库格式化等位置已迁移。

### 3.4 其它适配点 ✓ 基本完成

- `[[nodiscard]]` 已覆盖 `parse()` / `generate()` / GC `collect()` 返回值。✓
- `std::span<const Instruction>` 已在 Proto / VM 路径使用。✓
- `constexpr` opcode 元数据表已在 [opcode.hpp](src/compiler/opcode.hpp) / [opcode.cpp](src/compiler/opcode.cpp) 落地。✓
- `std::string_view` 已在 Parser / VM 路径大量使用。✓

### 3.5 审计新增：ValueResult 重构为 std::variant

**审计发现**：[codegen_types.hpp](src/compiler/codegen/codegen_types.hpp) 的 `ValueResult` 结构体有 12 个公开字段（`kind`, `immediate`, `access`, `reg`, `constIndex`, `aux`, `instructionPc`, `boolValue`, `numberValue`, `ownsRegister`, `isMultiResult`, `isSingleValue`）。部分字段仅在特定 `Kind` 下有效，类似 tagged union，但缺少编译期约束保证字段使用的一致性。

**当前进展**：PR-48 已加入兼容式 `std::variant` payload prototype，定义 `None`、`Immediate`、`ConstantRef`、`RegisterRef`、`PendingLoad`、`Relocatable`、`MultiRet` 和 `PendingJump` alternatives，并通过工厂函数同步填充 payload 与旧字段。`ExpressionEmitter` / `StatementEmitter` 主要构造点已改用工厂函数；旧字段读面暂保留，后续可按调用点迁移到 `std::visit`。

**建议**：将 `ValueResult` 重构为 `std::variant` 子类型：

```cpp
struct ImmediateValue { f64 numberValue; bool boolValue; /* ... */ };
struct ConstantRef { i32 constIndex; };
struct RegisterRef { i32 reg; bool ownsRegister; };
struct MultiRet { i32 instructionPc; };
struct PendingJump { i32 instructionPc; };

using ValueResult = std::variant<
    ImmediateValue, ConstantRef, RegisterRef, MultiRet, PendingJump
>;
```

利用 variant 的 `std::visit` + pattern matching 消除隐式契约，同时为教学提供"编译期安全 tagged union"的范例。

| 编号 | 任务 | 说明 |
|---|---|---|
| 3.5.1 | `ValueResult` 兼容式 `std::variant` payload prototype | ✓ **已完成** — PR-48；旧字段读面仍保留，后续逐步迁移到 `std::visit` |

---

## 阶段 4：教育价值增强（长期）

### 4.1 自解释代码：注释规范

**原则**：注释必须解释 **"为什么"**，不解释 **"是什么"**。新增教育性注释不要插入函数体中间；函数体需要保持连续、可扫读，避免被大段讲解切割。当前 `garbage_collector.hpp` 的中文 doxygen 块是好的样板。

**注释分层**：

1. **文件级 / 函数组注释**：承载状态机、时序约束、核心算法和 ASCII 图示，放在 `namespace` 内或一组 helper 之前。
2. **函数前契约注释**：承载调用顺序、不变量、异常/恢复语义，放在函数定义前，不打断函数体。
3. **类型旁表格注释**：承载 enum / variant / 常量映射表，放在类型定义上方。
4. **深度教学文档**：超过 8-12 行、需要例子或逐步推演的内容放到 `docs/walkthroughs/`、`docs/compiler/` 或 `docs/architecture/`；代码中只保留一句文档入口。

**函数体内注释限制**：阶段 4 新增注释不进入函数体；已有函数体短注释如果只是分段标题，可保留或在后续顺手外移。禁止为了教学在语句之间插入多行解释。

**重点补强位置**：

| 主题 | 推荐落点 | 说明 | 状态 |
|---|---|---|---|
| `src/vm/vm.cpp` 主循环时序 | `runDispatchBackend()` 前的 dispatch timing note | 解释 `pc++` / `savedpc` / hook 时序，不在 loop 内穿插长注释 | ✓ 已完成 |
| `src/gc/garbage_collector.cpp` `collect()` finalizer 顺序 | `collect(LuaState*)` 前的 phase contract | 解释 `prepareFinalizers` → `propagateMarks` → sweep → `runFinalizers` 的顺序原因 | ✓ 已完成 |
| `src/compiler/codegen/codegen_jump.cpp` 回填算法 | 文件级 backpatching model | 用 ASCII 图示解释 jump list 链表如何形成，helper 内部不再放教学段落 | ✓ 已完成 |
| `src/core/value.hpp` `ValueVariant` 类型旁表格 | `ValueVariant` 上方类型旁表格 | 现有注释已说明 variant 索引；后续可补 Lua 5.1 `LUA_T*` 常量对照关系 | P4 候选 |

### 4.2 `lua_bytecode` 工具可视化升级

**现状**：`bytecode_main.cpp` 支持单脚本打印、单脚本 `--cfg` Mermaid CFG 和双脚本 `--diff`；`src/bytecode/bytecode_printer.cpp` 已能打印 source、参数信息、upvalue 摘要、逐条指令、常量引用和常量表。PR-55 后 `full` 参数会递归打印 child protos；PR-56 后 `--diff` 会并排展示两段渲染后字节码的差异，并忽略 `source:` 路径元数据噪声；PR-63 后 `--cfg [full]` 会输出 basic block、TEST / TFORLOOP companion jump、FORLOOP 回边和 return exit 的 Mermaid `flowchart TD`。

**升级方案（分子任务）**：

1. ✓ **已完成**：基础信息块，包含 source name、`numparams`、`is_vararg`、`maxStackSize`、upvalue 列表。
2. ✓ **已完成**：指令解码：
   - 每条指令打印 `pc | line | OP_XXX | A=.. B=.. C=..`
   - 对 `LOADK` / `GETGLOBAL` 等同时显示其引用的常量内容
   - 对 `JMP` 计算并显示绝对目标 PC
3. ✓ **已完成**：常量表分类型打印，如 `K[0] = number 42`。
4. ✓ **已完成 — PR-55**：子原型递归，`full` 模式下深入打印 child protos，缩进可视化层级，并在 `CLOSURE` 指令注释中显示 `proto[index]` 摘要。
5. ✓ **已完成 — PR-56**：diff 模式，`bin\lua_bytecode.exe a.lua b.lua --diff`，并排展示两段字节码差异；`full` 参数可把 child protos 纳入对比。
6. ✓ **已完成 — PR-63**：Mermaid 输出，`bin\lua_bytecode.exe script.lua --cfg [full]` 输出函数的基本块控制流图（Mermaid 格式），可直接贴入文档；`full` 会递归打印 child Proto CFG 子图。

### 4.3 核心执行链路逻辑文档

在 `docs/walkthroughs/`（已存在目录）补三篇"端到端追踪"文章：

1. **`hello-world.md`**：✓ **已完成**。从 `print("hello")` 走完 Lexer → Parser → AST → CodeGen → Bytecode → VM dispatch → C 函数调用 → I/O。典范级教学文档，含具体文件:行号引用、字节码输出、寄存器视角分析。
2. **`closure-and-upvalue.md`**：✓ **已完成 — PR-50**。闭包从语法结构到 `OP_CLOSURE` / `OP_GETUPVAL` / `OP_SETUPVAL`，并追踪 Open/Closed Upvalue 状态转换。
3. **`gc-cycle.md`**：✓ **已完成 — PR-52**。构造一个含 weak table + `__gc` 的最小例子，逐步追踪 mark → finalizer prepare → weak cleanup → sweep → finalizer run。

每篇文章应同时引用源码 `:line` 锚点和 `lua_bytecode` 工具输出，做到"边看代码边看产物"。

### 4.4 REPL 体验改进（`lua_app`）

**现状**：`src/repl.cpp` 保留公共入口与 `ReplSession` 会话循环；补全、历史记录、元命令、信号 / 终端处理、执行辅助已拆分到 `src/repl/*`。当前支持多行输入累积、`=expr` 打印、中断处理、`isIncompleteInput` 自动续行。

**渐进增强建议**：

| 编号 | 增强 | 备注 | 状态 |
|---|---|---|---|
| 4.4.1 | 持久化历史记录（`.lua_history`） | 跨会话；启动时加载、退出时保存 | ✓ 已完成 |
| 4.4.2 | 简单 Tab 补全（全局名 + 已加载库的字段） | 利用 `LuaState->getGlobalTable()` 遍历；`completeInput()` 覆盖元命令、`.gc` 选项、全局名和 dotted table field | ✓ **已完成 — PR-59** |
| 4.4.3 | 内置 `.help` / `.bytecode <expr>` / `.ast <expr>` / `.gc` 元命令 | `.help`、`.bytecode`、`.ast` 与 `.gc` 已完成；`.gc strategy` 仅声明当前 mark-sweep 与 future incremental 边界，不做策略切换 | ✓ 已完成 |
| 4.4.4 | 颜色化错误输出（仅在 stdout/stderr 是 TTY 时） | `ErrorColorMode::Auto` 仅在 REPL 顶层循环启用；Windows 控制台通过 `SetConsoleMode(ENABLE_VIRTUAL_TERMINAL_PROCESSING)` 开启 ANSI | ✓ **已完成 — PR-60** |
| 4.4.5 | 行号 prompt：`lua:1>` / `lua:2>>` | 默认 prompt 显示当前 REPL 输入行；自定义 `_PROMPT` / `_PROMPT2` 保持字面值 | ✓ **已完成 — PR-61** |

### 4.5 审计新增：Trace 系统差异模式

**审计发现**：[Trace 系统](src/debug/trace_sink.hpp) 层次清晰（`ITraceSink` ← `JsonTraceSink` / `NullTraceSink`），`TraceEvent` 扁平 struct 携带 PC、指令操作数、源码位置、调用深度和寄存器快照上下文。PR-51 已补上差异模式：`--trace-diff` 在每条指令后输出 `changedRegisters`，避免学习者手动对比前后两帧。

**PR-51 实现形态**：`--trace-diff` 模式会在 JSONL 中增加 `changedRegisters` 字段：

```json
{
  "seq": 42, "pc": 3, "op": "ADD", "a": 1, "b": 2, "c": 3,
  "changedRegisters": [
    {"slot": 1, "old": "nil", "new": 15.0}
  ]
}
```

让学习者直观看到每条指令对寄存器栈的影响，而非通过前后两帧手动对比。

| 编号 | 增强 | 备注 | 状态 |
|---|---|---|---|
| 4.5.1 | `--trace-diff` 模式 | JSONL 中增加 `changedRegisters` 字段，记录每条指令引起的寄存器变化 | ✓ **已完成 — PR-51** |
| 4.5.2 | `TraceEvent` 统一 `funcName` 填充 | 确保 instruction / call / return / error JSONL event 都带 `funcName`；Lua `Proto` 使用 `source` 或 `source:linedefined` 标签，C 函数使用 `C function` | ✓ **已完成 — PR-53** |

---

## 阶段 5：工程实践完善

### 5.1 测试覆盖精细化

**现状**：543 个注册测试 / 2730 个断言结果 / 0 失败，目录 `tests/unit/`、`tests/lua/`。

**改进方向**（优先小步增量；覆盖矩阵作为 checklist 文档）：

1. ✓ **指令级覆盖矩阵**：PR-54 已新增 `tests/unit/vm/opcode_coverage_matrix.md`，为 38 条 opcode 显性列出正向 / 边界 / metamethod 覆盖锚点；`tools/check_opcode_coverage_matrix.ps1` 已接入质量门，新增 / 删除 / 重命名 opcode 时矩阵会失败并要求同步更新。
2. ✓ **AST Visitor 测试**：`tests/unit/compiler/test_ast_visitor.cpp` 已覆盖最小 visitor 分发与 concept 检查；后续若新增 AST dumper，再补端到端输出测试。
3. ✓ **GC 策略测试**：PR-62 已补 `GC Strategy Selection` / `GC Strategy Equivalence` / `collectgarbage Strategy`，锁住策略选择、未知策略拒绝和同根集存活语义等价。
4. ✓ **已完成 — PR-65**：REPL 增量解析测试：`REPL Commands` 已补真实 Parser 驱动样本，覆盖 EOF 可恢复输入、明确语法错误和 `=function(...)` 快速表达式 continuation。
5. ✓ **已完成 — PR-64**：Trace 文件 golden test：`VM Trace Debug` 已补 plain trace / trace-diff 两组 exact JSONL golden，锁住 `registers` 与 `changedRegisters` schema。

### 5.2 质量门（已有 `tools/run_quality_gate.ps1`）

- 把 `check_doc_drift.ps1` 接入质量门，让"代码与 README 章节不同步"成为可失败信号。
- ✓ **已完成**：PR-47 后 `check_doc_drift.ps1` 会运行 `bin\lua_test.exe`，从 `Registered Tests` / `Total Results` / `Failed` 汇总动态解析当前测试计数，再检查 README / status docs 是否同步；脚本内不再硬编码 "513" / "2497"。
- **已修正文档偏差**：`docs/architecture/runtime-services.md` 的结构体示例已补齐 `VM::DispatchStrategy* dispatchStrategy`，且 `check_doc_drift.ps1` 已加入该字段守卫。
- ✓ **已完成 — PR-54**：`run_quality_gate.ps1` 已新增 `opcode coverage matrix` 步骤，先运行 `check_opcode_coverage_matrix.ps1`，让 opcode 清单与测试覆盖 checklist 的漂移成为可失败信号。
- ✓ **已接入**：`run_quality_gate.ps1` 已包含 `clang-tidy smoke`（针对 `.clang-tidy` 中的 `bugprone-*`、`performance-*`、`portability-*`、`readability-*` 等保守规则；本机未安装 `clang-tidy` 时明确跳过）。
- CMake 路径补充 `-Wpedantic -Wconversion`（已用 MSVC `/W4`，CMake 旁路需对齐）。

### 5.3 构建一致性

- ✓ **已完成 — PR-66**：`tools/add_source.ps1` 可按 `Core` / `Repl` / `App` / `Bytecode` / `Test` 目标同步 `CMakeLists.txt`、`.vcxproj` 与 `.vcxproj.filters`，支持 `-DryRun`、`-AllowMissing`、`-Quiet` 和路径自动推断；`tools/test_quality_gate.ps1` 会在临时项目清单上做脚本烟测。

---

## 风险与回退策略

| 风险 | 影响 | 缓解 |
|---|---|---|
| VM dispatch 命令模式化引入函数指针表，调试器单步体验下降 | 影响教学 | 保留 `SwitchDispatch` 作为默认；调试构建强制使用；1.1.1 落地的独立 inline 函数进一步改善 Switch 路径单步体验 |
| `std::expected` 大范围替换异常导致调用链翻新 | 543 测试可能批量红 | 按 3.1 表格逐个函数迁移，每次 1 个函数 + 全量测试 |
| GC 去单例化破坏标准库内部对 `GarbageCollector::getInstance()` 的引用 | 编译错误广泛 | 保留 inline shim `getInstance()` 一版本，标记 `[[deprecated]]` |
| Visitor 化后 codegen 性能下降 | 不影响目标，但需观察 | 教学项目可接受；基准用 `examples/*.lua` 跑回归 |
| `ValueResult` → `std::variant` 重构引入大量访问代码 | 调用侧需逐一迁移 `std::visit` | 先做 prototype 分支验证可行性，再逐步迁移 |
| `LibRegistrar` 自注册的静态对象被 linker 优化掉 | 新库静默不加载 | 添加 `#pragma comment(linker, "/include:...")` 并在测试中验证 `openAll()` 库数量 |

---

## 推荐落地顺序（PR-05 及以后）

已落地摘要：

| PR 范围 | 已完成内容 | 对应阶段 |
|---|---|---|
| PR-05 ∼ PR-07 | `Parser::parse()` expected、bytecode 基础输出、REPL history + `.help` / `.bytecode` | 3.1 / 4.2 / 4.4 |
| PR-08 ∼ PR-10 | GC 主路径去单例化、`DispatchStrategy`、hello-world walkthrough | 2.2 / 2.3 / 4.3 |
| PR-11 ∼ PR-22 | `HandlerTable`、9 组 VM handlers、`TableDispatch`、patterns 文档 | 1.1 / 2.2 / 2.4 |
| PR-23 ∼ PR-34 | `CodeGenerator::tryGenerate()`、`VM::tryExecuteProto()`、Visitor concepts、`std::format`、`[[nodiscard]]`、`std::span`、opcode metadata、Parser `string_view` | 3.1 ∼ 3.4 |
| PR-35 ∼ PR-38 | VM 主循环、GC finalizer、CodeGenerator jump model 注释与阶段 4 注释位置收口 | 4.1 |
| PR-39 | Switch dispatch 每 opcode 独立 inline 函数，移除 switch 路径的统一 `runCurrentHandler()` lambda | 1.1.1 / 2.2 |
| PR-40 | `captureRuntimeErrors` / `mapExceptionToUnexpected`，集中 `tryExecuteProto` expected 边界异常映射 | 1.3.1 / 3.1 |
| PR-41 | CodeGenerator 职责地图与 statement/jump characterization 测试 | 2.5.1-a / 5.1 |
| PR-42 | `JumpPatcher` 抽取，集中 jump-list / pending jump / patch offset 边界 | 2.5.1-b / 5.1 |
| PR-43 | `ScopeManager` 抽取，集中 local / block / upvalue 作用域生命周期和 scope close 边界 | 2.5.1-c / 5.1 |
| PR-44 | `ExpressionEmitter` 抽取，集中 ValueResult / CondResult / CallResultInfo / LValueRef 表达式通道 | 2.5.1-d / 5.1 |
| PR-45 | `StatementEmitter` 抽取，集中 statement / block lowering 和语句级控制流编排 | 2.5.1-e / 5.1 |
| PR-46 | GC sweep / clearAll 显式接收 `StringPool&`，旧 `GarbageCollector::getInstance()` 标记 `[[deprecated]]` | 2.3 / 5.1 |
| PR-47 | `check_doc_drift.ps1` 动态解析测试计数，移除脚本内测试总数字面量；CI / 本地质量门先构建测试入口再做漂移检查 | 5.2 |
| PR-48 | `ValueResult` 兼容式 `std::variant` payload prototype，生产构造点改用工厂函数，并保留旧字段兼容读面 | 3.5.1 |
| PR-49 | `CodegenState` 命名清理：`regs` / `locals` / `blocks` / `upvalues` 改为职责名，`nactvar` 收口为 `activeVarCount` | 1.2 |
| PR-50 | `closure-and-upvalue.md` walkthrough，覆盖闭包捕获、`CLOSURE` 伪指令、`GETUPVAL` / `SETUPVAL` 和 open-to-closed upvalue 生命周期 | 4.3.2 |
| PR-51 | `--trace-diff` + `changedRegisters`，CLI 增量开关、VM 指令后差异发射、JSONL schema 与测试覆盖 | 4.5.1 |
| PR-52 | `gc-cycle.md` walkthrough，覆盖 weak table、userdata `__gc`、finalizer 复活、弱表清理和 sweep 顺序 | 4.3.3 |
| PR-53 | `TraceEvent` 统一 `funcName` 填充，instruction / call / return / error JSONL schema 与测试覆盖同步 | 4.5.2 |
| PR-54 | 指令级覆盖矩阵，覆盖 38 条 opcode 的正向 / 边界 / metamethod checklist，并接入质量门漂移检查 | 5.1.1 / 5.2 |
| PR-55 | `lua_bytecode full` 子原型递归打印，`CLOSURE` 指令显示 `proto[index]` 摘要，并补 Bytecode Printer 输出契约测试 | 4.2.4 |
| PR-56 | `lua_bytecode --diff` side-by-side 字节码差异，支持 compact / full 两种对比，并补 Bytecode Printer diff 输出契约测试 | 4.2.5 |
| PR-57 | REPL `.ast <expr|chunk>` 元命令，使用 AST visitor 打印 Chunk / Stmt / Expr 树形结构，并补 REPL Commands 输出契约测试 | 4.4.3 |
| PR-58 | REPL `.gc [stats|collect|strategy|help]` 元命令前置设计，使用 `RuntimeServices.gc` 暴露 mark-sweep 统计 / full collect / strategy 边界，并补 REPL Commands 输出契约测试 | 4.4.3 / 2.3 |
| PR-59 | REPL Tab 补全，`completeInput()` 覆盖元命令、`.gc` 选项、全局名和已加载库 dotted field，并补 REPL Commands 输出契约测试 | 4.4.2 |
| PR-60 | REPL 终端彩色错误输出，集中 `writeErrorLine()`，Auto 模式仅在 REPL TTY 中启用，Windows 开启 virtual terminal processing，并补颜色输出契约测试 | 4.4.4 |
| PR-61 | REPL 行号 prompt，默认 `_PROMPT` / `_PROMPT2` 显示 `lua:N> ` / `lua:N>> `，自定义 prompt 保持不变，并补 REPL Commands 输出契约测试 | 4.4.5 |
| PR-62 | `GCStrategy` 抽象，`MarkSweepGC` 承载真实 mark-sweep，`IncrementalGC` 作为等价行为教学占位，并补策略选择 / 等价性 / `collectgarbage("strategy")` 测试 | 2.3 / 5.1 |
| PR-63 | `lua_bytecode --cfg [full]` Mermaid CFG 输出，按 basic block 展示 fallthrough / jump / TEST companion / TFORLOOP / FORLOOP / return 边，并补 Bytecode Printer CFG 输出契约测试 | 4.2.6 / 5.1 |
| PR-64 | Trace JSONL plain / diff golden 测试，使用无指针值的小脚本精确对比 `registers` 与 `changedRegisters` 输出，防止 trace schema 漂移 | 5.1 |
| PR-65 | REPL 增量解析测试，使用真实 Parser 错误锁住 EOF-driven continuation、明确语法错误立即报告，以及 `=function(...)` 多行表达式保持 expression mode | 5.1 |
| PR-66 | `tools/add_source.ps1` 源码清单同步脚本，覆盖 CMake / VS project / filters 追加、目标自动推断、dry-run 和质量门烟测 | 5.3 |
| PR-67 | `ParserUtils::tokenString` 集中到 `parser_utils.hpp`，移除 `Parser` 类内无状态字符串 helper，并保持借用型 `StrView` 测试边界 | 1.2 |
| PR-68 | `AstVisitor<Derived, R>` 组合模板与 `VisitsAstNodes` concept，REPL AST printer 迁移到组合 visitor，并补 AST Visitor 分派契约测试 | 2.1.1 |
| PR-69 | 复用 `detail::canVisitNode()` / `detail::visitsVariantNodes()` 去重 `ExprVisitor` / `StmtVisitor` 内部 `canVisit*` 检查，并保持 Expression/StatementEmitter 私有访问边界 | 2.1.2 |

后续推荐顺序：

| PR | 编号 | 任务 | 阶段 | 依赖 / 理由 |
|---|---|---|---|---|
| PR-70 | 2.6.1 | `lib_manager.hpp` 的 `openXxx()` 包装标记 `[[deprecated]]` | 2 | P3；引导调用方使用表驱动 `openCatalogLibrary()`，减少标准库注册冗余入口 |

每个 PR 完成后跑：

```powershell
.\tools\run_quality_gate.ps1
.\bin\lua_test.exe
```

---

## 附录：完成度审计（2026-05-23）

> 审计方法：逐条对照文档任务编号，以当前代码库实际状态验证每个标记为"待执行"/"P3 待执行"/"P4 候选"/"待实现"的条目。已完成项以 ✓ 复核确认。

### 审计结果总览

| 阶段 | 文档自评 | 实际评估 | 偏差 | 关键差距 |
|---|---|---|---|---|
| 阶段 1 — 短期代码清理 | ~95% | **~96%** | +1% | `tokenString` 集中化已完成；剩余工作转入跨阶段 P3 清理 |
| 阶段 2 — 中期模式重构 | ~93% | **~90%** | −3% | GCStrategy、AstVisitor 组合模板和 Visitor 内部检查去重已落地；主要剩余是 lib_manager 声明清理 |
| 阶段 3 — 现代 C++ 特性 | ~90% | **~90%** | 0 | ValueResult variant prototype 已就位，后续 std::visit 迁移为渐进工作 |
| 阶段 4 — 教育价值增强 | ~88% | **~86%** | −2% | REPL 体验和字节码可视化主线已闭环；后续偏样例和文档深挖 |
| 阶段 5 — 工程实践 | ~82% | **~82%** | 0 | 指令级覆盖矩阵、GC 策略等价测试、CFG 输出契约测试、Trace JSONL golden、REPL 增量解析测试和 add_source 脚本已补齐；剩余为 CMake 编译选项对齐 |
| **加权综合** | **~88%** | **~86%** | **−2%** | |

---

### 逐阶段逐任务核实

#### 阶段 1 核验

| 编号 | 文档标记 | 代码库实际 | 判定 |
|---|---|---|---|
| 1.1.1 | ✓ 已完成 | `src/vm/vm_switch_dispatch.hpp` 存在，每 opcode 独立 inline 函数 | ✓ 确认 |
| 1.1.2 | ✓ 已完成 | `src/gc/` 下 `gc_mark.cpp` / `gc_sweep.cpp` / `gc_finalize.cpp` / `gc_weak.cpp` 均已存在 | ✓ 确认 |
| 1.1.3 | ✓ 已完成 | `check_doc_drift.ps1` 已含 dispatchStrategy 字段守卫 | ✓ 确认 |
| 1.2 命名清理 | ✓ 已完成 | `CodegenState` 成员已改名 `registers` / `localScope` / `blockManager` / `upvalueContext`；`activeVarCount` 已收口 | ✓ 确认 |
| 1.2 tokenString | ✓ PR-67 | `src/compiler/parser/parser_utils.hpp` 提供 `ParserUtils::tokenString()`；`parser_*.cpp` 分片显式调用该 helper，`parser.hpp` 不再携带无状态 `tokenString` 类成员 | ✓ 确认 |
| 1.3.1 | ✓ 已完成 | `VM::detail::captureRuntimeErrors<T>()` / `mapExceptionToUnexpected()` 已提取 | ✓ 确认 |

**结论**：阶段 1 原有 `tokenString` P3 遗留已收口；后续短期清理的主要价值来自跨阶段低风险去重。

#### 阶段 2 核验

| 编号 | 文档标记 | 代码库实际 | 判定 |
|---|---|---|---|
| 2.1 Visitor | ✓ 已完成 | `ExprVisitor<Derived, R>` + `StmtVisitor<Derived, R>` + `AstVisitor<Derived, R>` + concepts 完整 | ✓ 确认 |
| 2.1.1 | ✓ PR-68 | `src/compiler/ast_visitor.hpp` 提供 `AstVisitor<Derived, R>` 与 `VisitsAstNodes`；`src/repl/repl_meta.cpp` 的 `AstPrinter` 已迁移到组合 visitor | ✓ 确认 |
| 2.1.2 | ✓ PR-69 | `ExprVisitor` / `StmtVisitor` 已复用 `detail::canVisitNode()` / `detail::visitsVariantNodes()`；`expression_emitter.hpp` 与 `statement_emitter.hpp` 精确 friend 该检查函数，保持私有 `visitNode` 可验证 | ✓ 确认 |
| 2.2 VM Dispatch | ✓ 已完成 | `DispatchStrategy` + `SwitchDispatch` + `TableDispatch` + `HandlerTable` (38 条目, 9 组注册) | ✓ 确认 |
| 2.3 GC 去单例 | ✓ 已完成 | `GarbageCollector::sweep(StringPool&)` / `clearAll(StringPool&)` 已收口；`getInstance()` 标记 `[[deprecated]]` | ✓ 确认 |
| 2.3 GCStrategy | ✓ PR-62 | `src/gc/gc_strategy.hpp/.cpp` 已定义 `GCStrategy` / `MarkSweepGC` / `IncrementalGC`；`GarbageCollector::collect()` 委托当前策略；`collectgarbage("strategy",...)` 已可查询 / 切换策略边界 | ✓ 确认 |
| 2.4 模式文档 | ✓ 已完成 | `docs/architecture/patterns.md` 已记录 | ✓ 确认 |
| 2.5 CodeGen 拆分 | ✓ 已完成 | 五个子任务 (2.5.1-a ∼ 2.5.1-e) 全部落地：`jump_patcher` / `scope_manager` / `expression_emitter` / `statement_emitter` | ✓ 确认 |
| 2.6.1 | P3 待执行 | `lib_manager.hpp` 仍保留 7 个 `openXxx()` 方法声明，无 `[[deprecated]]` 标记 | ✗ 未完成 |
| 2.6.2 | P4 候选 | 代码库中不存在 `LibRegistrar` 类 | ✗ 未完成 |

**结论**：核心重构（Visitor、Dispatch、CodeGen 拆分、GCStrategy）全部落地且质量高。PR-68 / PR-69 已补齐 full-tree visitor 组合模板和内部检查去重；剩余阶段 2 差距主要是 `lib_manager.hpp` 冗余声明，属于低优先级清理。

#### 阶段 3 核验

| 编号 | 文档标记 | 代码库实际 | 判定 |
|---|---|---|---|
| 3.1 `std::expected` | ✓ 已完成 | `Parser::parse()` / `CodeGenerator::tryGenerate()` / `VM::tryExecuteProto()` 三个入口全部 expected | ✓ 确认 |
| 3.2 concepts | ✓ 已完成 | `VisitsNode` / `VisitsNodeAs` / `VisitsExprNodes` / `VisitsStmtNodes` + `visitsVariantNodes` compile-time check | ✓ 确认 |
| 3.3 `std::format` | ✓ 已完成 | debug / repl / bytecode / 标准库格式化均已迁移 | ✓ 确认 |
| 3.4 其它 | ✓ 基本完成 | `[[nodiscard]]` / `std::span` / `constexpr` opcode 表 / `string_view` 均已覆盖 | ✓ 确认 |
| 3.5.1 ValueResult variant | ✓ 已完成 | `std::variant` payload prototype 已合入，工厂函数覆盖 ExpressionEmitter / StatementEmitter 主要构造点 | ✓ 确认 |
| 3.5 后续迁移 | 隐含待执行 | 旧字段读面仍保留，调用点尚未迁移到 `std::visit` | ⚠ 渐进进行中 |

**结论**：现代 C++ 特性应用是五个阶段中完成度最高的，文档自评 90% 准确。ValueResult 的 variant 迁移是渐进式工作，prototype 阶段已达标。

#### 阶段 4 核验

| 编号 | 文档标记 | 代码库实际 | 判定 |
|---|---|---|---|
| 4.1 注释规范 | ✓ 已完成 | VM 主循环 dispatch timing note、GC `collect()` phase contract、CodeGen jump backpatching model 均已收口 | ✓ 确认 |
| 4.2.1-4.2.3 字节码基础 | ✓ 已完成 | source / numparams / is_vararg / maxStackSize / 逐条指令 (pc\|line\|OP\|A/B/C) / 常量引用 / 常量表 | ✓ 确认 |
| 4.2.4 子原型递归 | ✓ PR-55 | `bytecode_printer.cpp` 使用 `full` 递归打印 child protos；紧凑模式保持顶层输出，`CLOSURE` 注释显示 `proto[index]` 摘要 | ✓ 确认 |
| 4.2.5 `--diff` 模式 | ✓ PR-56 | `bytecode_main.cpp` 支持 `a.lua b.lua --diff [full]`；`printProtoBytecodeDiff()` 输出 left/right/mode/status/changed lines 摘要和 side-by-side 差异行，并忽略 `source:` 路径噪声 | ✓ 确认 |
| 4.2.6 Mermaid CFG | ✓ PR-63 | `bytecode_main.cpp` 支持 `script.lua --cfg [full]`；`printProtoBytecodeCfg()` 输出 Mermaid `flowchart TD`，按 basic block 展示 fallthrough / jump / TEST companion / TFORLOOP / FORLOOP / return 边，`full` 会递归 child Proto CFG | ✓ 确认 |
| 4.3 walkthrough 三篇 | ✓ 已完成 | `hello-world.md` / `closure-and-upvalue.md` / `gc-cycle.md` 全部存在，含源码 `:line` 锚点 | ✓ 确认 |
| 4.4.1 REPL 历史 | ✓ 已完成 | `.lua_history` 持久化：`loadHistory()` + `saveHistory()` + `recordHistory()` 均实现 | ✓ 确认 |
| 4.4.2 Tab 补全 | ✓ PR-59 | `completeInput()` 已覆盖元命令、`.gc` 选项、全局名和已加载库 dotted field；交互 TTY 下 Tab 调用该补全，管道输入中的 `\t` 也走同一逻辑 | ✓ 确认 |
| 4.4.3 `.ast` / `.gc` | ✓ PR-57 / PR-58 | `.help`、`.bytecode`、`.ast` 与 `.gc` 已实现；`.ast` 输出 `mode: chunk` / `mode: expression` 和 Chunk / Stmt / Expr 树形结构；`.gc` 输出当前 mark-sweep 统计、full collect 前后快照和 strategy 边界 | ✓ 确认 |
| 4.4.4 颜色化错误 | ✓ PR-60 | `ErrorColorMode::Auto` 仅在 REPL 顶层循环和 stdout/stderr TTY 下启用；强制 Always 模式用于输出契约测试；Windows 通过 `SetConsoleMode(ENABLE_VIRTUAL_TERMINAL_PROCESSING)` 启用 ANSI | ✓ 确认 |
| 4.4.5 行号 prompt | ✓ PR-61 | 默认 prompt 使用 `lua:N> ` / `lua:N>> ` 标出当前输入行；自定义 `_PROMPT` / `_PROMPT2` 保持字面值 | ✓ 确认 |
| 4.5.1 `--trace-diff` | ✓ 已完成 | `changedRegisters` 字段已落地 JSONL，CLI 开关 + VM 发射 + 测试覆盖完整 | ✓ 确认 |
| 4.5.2 `funcName` 统一 | ✓ 已完成 | `TraceEvent::funcName` 已改为拥有型 `Str`，instruction / call / return / error JSONL 均输出 `funcName`；VM 指令与返回事件使用当前/返回 `Proto` 标签，Call 事件使用 callee 标签 | ✓ 确认 — PR-53 |

**结论**：阶段 4 的 REPL 体验（4.4.1 ∼ 4.4.5）已经闭环，字节码侧 compact / full / `--diff` / `--cfg` 也已形成完整教学工具链。walkthrough 三篇、trace-diff、child proto 递归输出、`--diff` 字节码对比、Mermaid CFG、`.ast` AST 输出、`.gc` GC 状态输出、Tab 补全、终端彩色错误和行号 prompt 是亮点。

#### 阶段 5 核验

| 编号 | 文档标记 | 代码库实际 | 判定 |
|---|---|---|---|
| 5.1 opcode 覆盖矩阵 | ✓ PR-54 | `tests/unit/vm/opcode_coverage_matrix.md` 覆盖 38 条 opcode；`tools/check_opcode_coverage_matrix.ps1` 从 `opcode.hpp` 解析真实 enum 并校验矩阵行数、顺序、重复和未知 opcode | ✓ 确认 |
| 5.1 AST Visitor 测试 | ✓ | `tests/unit/compiler/test_ast_visitor.cpp` 存在 | ✓ 确认 |
| 5.1 GC 策略测试 | ✓ PR-62 | `GC Strategy Selection` / `GC Strategy Equivalence` / `collectgarbage Strategy` 已覆盖策略边界、同根集等价和标准库切换入口 | ✓ 确认 |
| 5.1 Bytecode CFG 输出契约测试 | ✓ PR-63 | `Bytecode Printer` 测试已覆盖 Mermaid CFG branch、loop 和 `full` child Proto 子图输出 | ✓ 确认 |
| 5.1 REPL 增量解析测试 | ✓ PR-65 | `REPL Commands` 已补 `Incremental Parsing Recognizes Recoverable EOF Sources`、`Rejects Definite Syntax Errors` 和 `Keeps Quick Expression Mode`，直接覆盖 `prepareInputForExecution()` + `isIncompleteInput()` 的真实 Parser 错误路径 | ✓ 确认 |
| 5.1 Trace golden 测试 | ✓ PR-64 | `Trace JSONL Plain Golden` / `Trace JSONL Diff Golden` 已精确对比实际 VM 执行产生的 JSONL，覆盖 plain `registers` 与 diff `changedRegisters` schema | ✓ 确认 |
| 5.2 质量门 | ✓ | `run_quality_gate.ps1` + `check_doc_drift.ps1` 动态解析测试计数，并已接入 opcode 覆盖矩阵漂移检查 | ✓ 确认 |
| 5.2 clang-tidy | ✓ 增量 | `.clang-tidy` 配置文件存在；`run_quality_gate.ps1` 已有 `clang-tidy smoke`，本机无工具时按增量策略跳过 | ✓ 已接入 |
| 5.2 CMake 编译选项 | 待验证 | `-Wpedantic -Wconversion` 对齐状态未确认（项目用 MSVC `/W4`） | ⚠ 待验证 |
| 5.3 `add_source.ps1` | ✓ PR-66 | `tools/add_source.ps1` 已支持按目标同步 CMake、VS project 和 filters；质量门自检会复制临时项目清单验证 Core / Bytecode / Test 追加与幂等性 | ✓ 确认 |

**结论**：质量门自动化继续加强：动态测试计数、opcode 覆盖矩阵漂移、Trace JSONL schema 漂移和新增源码清单同步脚本烟测都已变成可失败信号。后续主要差距转为低优先级构建选项对齐。

---

### 未完成项优先级排序

#### P1 — 影响教学连贯性

当前无未完成 P1 项；PR-62 已收口最后一个阻塞性策略边界。

#### P2 — 提升教学体验（建议后续跟进）

当前无未完成 P2 项；PR-65 已补齐 REPL 增量解析专项测试。

#### P3 — 代码质量收尾

| 编号 | 任务 |
|---|---|
| 2.6.1 | `lib_manager.hpp` `openXxx()` 标记 `[[deprecated]]` |

#### P4 — 锦上添花

| 编号 | 任务 |
|---|---|
| 2.6.2 | `LibRegistrar` 声明式自注册 |

---

### 亮点总结

1. **核心重构质量极高**：Visitor 模式（compile-time 全覆盖检查）、VM Dispatch 双策略、CodeGenerator 四路拆分——每个都经过 characterization 测试锁定行为后再抽取，工程纪律严明。
2. **walkthrough 三部曲完整**：hello-world → closure-and-upvalue → gc-cycle，构成从语法到执行到内存管理的完整教学链路。
3. **质量门自动化**：`check_doc_drift.ps1` 动态解析测试计数，`check_opcode_coverage_matrix.ps1` 锁住 38 条 opcode 覆盖矩阵，消除了文档与代码不同步的隐性风险。
4. **现代 C++ 应用深入**：`std::expected` 错误边界、concepts 编译期约束、`std::variant` 渐进迁移，三者均是该规模教学项目中少见的深度应用。
