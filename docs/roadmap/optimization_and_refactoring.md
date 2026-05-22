# Lua 解释器优化与重构路线图

> 适用范围：`g:\github\lua`（现代 C++ Lua 5.1.5 解释器）
> 设计目标：**可读性 > 可维护性 > 教育价值 > 性能**
> 约束：保持 412 测试 / 1629 断言 100% 通过、不破坏 `LuaState` / `VM` public API。

---

## 阶段划分总览

| 阶段 | 周期定位 | 关注点 | 是否触发 API 变化 |
|---|---|---|---|
| **阶段 1 — 短期代码清理** | 1–2 个迭代 | 删冗余、统一命名、收紧错误处理、文档与代码同步 | 否 |
| **阶段 2 — 中期模式重构** | 3–5 个迭代 | 引入访问者 / 策略 / 命令模式，VM dispatch 与 GC 抽象化 | 内部 API 调整、public 不变 |
| **阶段 3 — 长期教育资源建设** | 持续推进 | 自解释代码、字节码可视化、执行链路教学文档、REPL 体验 | 否（增量增强） |

---

## 阶段 1：短期代码清理（1–2 个迭代）

### 1.1 架构边界微调

**现状**

- `src/compiler/` 已按 `parser_*.cpp` / `codegen_binding|expr|jump|stmt.cpp` 分片，`codegen_state.hpp` / `codegen_context.hpp` 已隔离状态。
- `src/vm/vm.cpp` 仍保留约 2000 行的主 dispatch `switch`，虽然已有 `vm_entry.cpp` / `vm_call.cpp` / `vm_ops.cpp` / `vm_dispatch.hpp` 等分片。
- `src/gc/garbage_collector.cpp` 仍为单文件 + 单例。

**任务**

| 编号 | 任务 | 文件 | 验证 |
|---|---|---|---|
| 1.1.1 | 把 `vm.cpp` 主 `switch` 内每个 case 体（>10 行的）提取为 `vm_ops.cpp` 中已有命名风格的 inline helper，例如 `execOpAdd(...)`、`execOpGetTable(...)` | `src/vm/vm.cpp` → 分组到 `vm_arith.cpp` / `vm_table.cpp` 等 | `vm_dispatch.hpp` 中 `OpcodeGroup` 一一对应；现有 `test_vm_dispatch.cpp` 通过 |
| 1.1.2 | 拆分 `garbage_collector.cpp`，按 `gc_mark.cpp` / `gc_sweep.cpp` / `gc_finalize.cpp` / `gc_weak.cpp` 分片 | `src/gc/` | GC 相关测试全绿 |
| 1.1.3 | 在 `docs/architecture/overview.md` 的表格中同步以上分片，避免文档漂移（已有 `tools/check_doc_drift.ps1`） | `docs/` | `check_doc_drift.ps1` 通过 |

### 1.2 命名与可读性收紧

- 统一 GC 内部成员命名：`allObjects_` / `roots_` / `grayList_` 已是 trailing-underscore 风格，但 `finalizersRunning_` 与 `objectCount_` 使用方式不一致，建议补一轮 `clang-format` + 命名审查。
- `Parser::tokenString` 等 static helper 已散布在 `parser.cpp`，可以集中到 `parser_utils.hpp`。
- 删除 `ExprDesc` 历史残留（`docs/archive/history/exprdesc.md` 表明已迁出，但代码中仍可能有死引用）：用 `view ... search_query_regex` 扫一遍 `ExprDesc` 与 `ExprKind` 残余。

### 1.3 错误处理收紧

**现状**：`Parser::parse()` 抛 `ParseError`，VM 部分使用 `std::runtime_error`，`executeREPLInput` 用 `try/catch` 包裹。

**任务**：

- 统一一组面向用户的异常类型层级，例如：`LuaError` ← `ParseError` / `RuntimeError` / `MemoryError`，集中在 `src/common/errors.hpp`。
- 移除 `vm_entry.cpp` 中 `throw std::runtime_error("VM::execute: null function")` 这类裸 `runtime_error`。
- **不要**马上引入 `std::expected`（见阶段 2），先把异常类型规整完。

---

## 阶段 2：中期模式重构（3–5 个迭代）

### 2.1 访问者模式（Visitor）应用到 AST

**现状**

`src/compiler/ast.hpp` 使用 `std::variant<...>` + `Stmt::variant` / `Expr::variant`，目前 `CodeGenerator` 内部应该是用 `std::visit` 或 `if constexpr` 来分派。这套结构 **已经具备 Visitor 模式的能力**，但缺乏一个"标准 Visitor 基设施"。

**重构目标**

提供一个统一的 `AstVisitor<R>` 模板基类，使得未来的 AST 用户（CodeGen、Pretty Printer、Linter、教学版 AST Dumper）都能复用同一遍历框架。

**实施步骤**

1. 在 `src/compiler/ast_visitor.hpp` 新增：

   ```cpp
   template <typename Derived, typename R = void>
   struct ExprVisitor {
       R visit(const Expr& e) {
           return std::visit([this](auto const& node) -> R {
               return static_cast<Derived*>(this)->visitNode(node);
           }, e.variant);
       }
   };
   ```

   `StmtVisitor` 同理。

2. 让 `CodeGenerator` 的 expr / stmt 分派改为继承 `ExprVisitor` / `StmtVisitor`，把现在分散在 `codegen_expr.cpp` 的 `if constexpr` 分支替换成 `visitNode(const BinaryExpr&)` 等重载。

3. **教育价值副产物**：在 `tools/` 下新增一个 `ast_dumper` Visitor 实现（不必是新的 vcxproj，可作为 `lua_bytecode` 的子命令），展示同一个 AST 如何被多个 Visitor 复用。

### 2.2 命令模式 + 策略模式 应用到 VM Dispatch

**现状**

`vm.cpp` 主循环是经典 `while + switch(op)`。`vm_dispatch.hpp` 已经定义了 `OpcodeGroup` 枚举，是天然的"按组分派"切入点。

**重构目标**

把单一巨型 `switch` 演化为 **指令处理器表（命令模式）+ 可切换的 dispatch 策略**，但**默认仍是 switch**（保护性能与可调试性）。


**实施步骤**

1. 定义命令接口（不必虚函数，用 free function 表足以）：

   ```cpp
   // src/vm/vm_handlers.hpp
   using OpHandler = void(*)(VMContext& ctx, Instruction inst);
   inline constexpr std::array<OpHandler, OP_COUNT> kHandlers = { ... };
   ```

2. 在 `vm_handlers/` 子目录下，每个 opcode 一个 `.cpp` 实现（适合教学，每个文件可独立讲解）。

3. 引入策略模式封装 dispatch 算法：

   ```cpp
   class DispatchStrategy {
   public:
       virtual ExecResult run(VMContext&) = 0;
   };
   class SwitchDispatch : public DispatchStrategy { ... };   // 当前实现
   class TableDispatch  : public DispatchStrategy { ... };   // 函数指针表
   ```

   通过 `RuntimeServices` 注入。教学场景中可以切换两种策略对比。

4. **不要**强行做 computed-goto / threaded code —— 它牺牲可读性，与项目目标冲突。

### 2.3 策略模式应用到 GC

**现状**

`GarbageCollector` 是单例，固定执行 mark-sweep，无算法插拔。

**重构目标**

1. 去单例化：把 `GarbageCollector` 实例放入 `GlobalState`（已经持有它的引用），仅保留兼容 `getInstance()` 一段时期再删除。
2. 抽象 `GCStrategy`：

   ```cpp
   class GCStrategy {
   public:
       virtual ~GCStrategy() = default;
       virtual usize collect(GCContext& ctx) = 0;
       virtual const char* name() const = 0;
   };
   class MarkSweepGC : public GCStrategy { ... };   // 当前实现
   class IncrementalGC : public GCStrategy { ... }; // 教学预留，先空实现 + TODO
   ```

3. **教学价值**：标准库新增 `collectgarbage("strategy", "mark-sweep" | "incremental")` 子命令，让学习者直接在 REPL 中切换算法。

### 2.4 编译期/编辑期模式登记

- 在 `docs/architecture/patterns.md`（新文件，唯一被允许新增的文档）记录每种模式在代码中的位置：
  - Visitor → `ast_visitor.hpp` + `codegen_*.cpp`
  - Command + Strategy → `vm_handlers.hpp` + `DispatchStrategy`
  - Strategy → `GCStrategy`
  - Singleton → `GlobalState`（保留并解释为何用单例）
  - Builder → `bytecode_builder.hpp`（已存在，标注即可）

---

## 阶段 3：现代 C++ 特性升级（穿插于阶段 1 & 2）

> 项目 toolchain 是 MSVC + VS 2026，已具备 C++23 能力。**升级原则：只在能显著简化代码时引入，不为新语法而新语法。**

### 3.1 `std::expected<T, LuaError>` 替换部分异常

**优先替换的位置**（高频路径、错误为"可预期结果"）：

| 当前签名 | 建议签名 |
|---|---|
| `Chunk Parser::parse()` 抛 `ParseError` | `std::expected<Chunk, ParseError> Parser::parse()` |
| `Proto* CodeGenerator::generate(...)`（返回 nullptr 表错） | `std::expected<Proto*, CodegenError>` |
| `ExecResult VM::executeProto(...)` | 改为 `std::expected<ExecResult, RuntimeError>`（若 `ExecResult` 已含错误码可暂缓） |

**保留异常的位置**：REPL 顶层、`std::bad_alloc` 等真正异常路径。

### 3.2 `concepts` 约束模板

`ast_visitor.hpp` 的 `Derived` 完美适合 concept 约束：

```cpp
template <typename V, typename Node>
concept VisitsNode = requires(V v, const Node& n) {
    v.visitNode(n);
};
```

类似的，`RuntimeServices` 中接受任意 service 的注册接口可用 concept 约束 `service.name()` / `service.init()` 存在。

### 3.3 `std::format` 替换 `std::ostringstream`

**目标文件（按出现频率）**：

- `src/debug/*.cpp`：trace JSON 行的拼接
- `src/repl.cpp`：错误信息 `stdin:line: message`
- `src/bytecode/bytecode_main.cpp` 的 `printProtoBytecode`
- 标准库格式化输出（`src/lib/string_lib.cpp` 中的 `string.format` 内部辅助）

**注意**：MSVC 的 `std::format` 在含 `wchar_t` 的语境下偶有诊断，保留必要的 fallback。

### 3.4 其它适配点

- `[[nodiscard]]` 全面覆盖 `parse()` / `generate()` / GC `collect()` 返回值。
- `std::span<const Instruction>` 替换 `const Vec<Instruction>&` 在 VM 主循环、`bytecode_main` 中的只读传递。
- `constexpr` 化 opcode 元数据表（指令格式、参数数量、是否有 metamethod 路径），与 `vm_dispatch.hpp` 的 `OpcodeGroup` 合并到一张编译期表。
- `std::string_view` 已大量使用，但确认 `Parser::tokenString` 返回 `Str`（拷贝）的位置是否能改成 `std::string_view`（注意 string pool 生命周期）。

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

| 主题 | 推荐落点 | 说明 |
|---|---|
| `src/vm/vm.cpp` 主循环时序 | `runDispatchBackend()` 前的 dispatch timing note | 解释 `pc++` / `savedpc` / hook 时序，不在 loop 内穿插长注释 |
| `src/gc/garbage_collector.cpp` `collect()` finalizer 顺序 | `collect(LuaState*)` 前的 phase contract | 解释 `prepareFinalizers` → `propagateMarks` → sweep → `runFinalizers` 的顺序原因 |
| `src/compiler/codegen/codegen_jump.cpp` 回填算法 | 文件级 backpatching model | 用 ASCII 图示解释 jump list 链表如何形成，helper 内部不再放教学段落 |
| `src/core/value.hpp` `Value::Tag` | enum / struct 上方类型旁表格 | 说明 variant 索引与 Lua 5.1 `LUA_T*` 常量对照关系 |

### 4.2 `lua_bytecode` 工具可视化升级

**现状**：`bytecode_main.cpp` 只调用 `printProtoBytecode(proto, std::cout, full)`，`docs/guides/bytecode-tool.md` 已列出"需要完成的"项。

**升级方案（分子任务）**：

1. **基础信息块**：source name、`numparams`、`is_vararg`、`maxStackSize`、upvalue 列表。
2. **指令解码**：
   - 每条指令打印 `pc | line | OP_XXX | A=.. B=.. C=..`
   - 对 `LOADK` / `GETGLOBAL` 等同时显示其引用的常量内容
   - 对 `JMP` 计算并显示绝对目标 PC
3. **常量表**：分类型打印（带颜色/标记），如 `K[0] = number 42`。
4. **子原型递归**：`full` 模式下深入打印 child protos，缩进可视化层级。
5. **diff 模式**：`bin\lua_bytecode.exe a.lua b.lua --diff`，并排展示两段字节码差异 —— 对教学"为什么这样写更高效"极有帮助。
6. **可选 Graphviz/Mermaid 输出**：`--cfg` 子命令输出函数的基本块控制流图（Mermaid 格式），可直接贴入文档。

### 4.3 核心执行链路逻辑文档

在 `docs/walkthroughs/`（已存在目录）补三篇"端到端追踪"文章：

1. **`hello-world.md`**：从 `print("hello")` 走完 Lexer → Parser → AST → CodeGen → Bytecode → VM dispatch → C 函数调用 → I/O。
2. **`closure-and-upvalue.md`**：闭包从语法结构到 `OP_CLOSURE` + `OP_GETUPVAL`，Open/Closed Upvalue 状态转换。
3. **`gc-cycle.md`**：构造一个含 weak table + `__gc` 的最小例子，逐步追踪 mark → finalizer prepare → sweep。

每篇文章应同时引用源码 `:line` 锚点和 `lua_bytecode` 工具输出，做到"边看代码边看产物"。

### 4.4 REPL 体验改进（`lua_app`）

**现状**：`src/repl.cpp` 支持多行输入累积、`=expr` 打印、中断处理、`isIncompleteInput` 自动续行。

**渐进增强建议**：

| 编号 | 增强 | 备注 |
|---|---|---|
| 4.4.1 | 持久化历史记录（`.lua_history`） | 跨会话；启动时加载、退出时保存 |
| 4.4.2 | 简单 Tab 补全（全局名 + 已加载库的字段） | 利用 `LuaState->getGlobalTable()` 遍历 |
| 4.4.3 | 内置 `.help` / `.bytecode <expr>` / `.ast <expr>` / `.gc` 元命令 | 与 `lua_bytecode`、`ast_dumper` 复用代码 |
| 4.4.4 | 颜色化错误输出（仅在 stdout 是 TTY 时） | Windows 控制台需 `SetConsoleMode(ENABLE_VIRTUAL_TERMINAL_PROCESSING)` |
| 4.4.5 | 行号 prompt：`lua:1>` / `lua:2>>` | 多行时显示当前行 |

---

## 阶段 5：工程实践完善

### 5.1 测试覆盖精细化

**现状**：412 测试 / 1629 断言，目录 `tests/unit/`、`tests/lua/`。

**改进方向**（仅修改现有测试文件，不主动新建）：

1. **指令级覆盖矩阵**：为 38 条 opcode 建立"每条至少 1 个正向 + 1 个边界 + 1 个 metamethod 路径"用例。建议在 `tests/unit/vm/` 下整理一张 `opcode_coverage_matrix.md`（仅作 checklist，不是 test runner）。
2. **AST Visitor 测试**：阶段 2.1 落地后，为 `ExprVisitor` 写一个最小 dumper 测试，确保所有节点类型都有对应 `visitNode` 重载（编译期 concept 检查）。
3. **GC 策略测试**：阶段 2.3 引入 `GCStrategy` 后，给每个 strategy 写"等价性测试"——同样根集应产生同样的存活集（不要求时间一致）。
4. **REPL 增量解析测试**：把 `isIncompleteInput` 的判定从 `repl.cpp` 中抽出为可单测函数。
5. **Trace 文件 golden test**：`src/debug/*` 的 JSONL trace 容易回归，建议固化一组 golden 文件。

### 5.2 质量门（已有 `tools/run_quality_gate.ps1`）

- 把 `check_doc_drift.ps1` 接入质量门，让"代码与 README 章节不同步"成为可失败信号。
- 加一条 `clang-tidy` 检查（针对 modernize-*、readability-*、bugprone-*），不强制全过，但持续记录。
- CMake 路径补充 `-Wpedantic -Wconversion`（已用 MSVC `/W4`，CMake 旁路需对齐）。

### 5.3 构建一致性

- VS solution 与 `CMakeLists.txt` 文件清单偏差是已知风险。阶段 1 拆分文件时，**每次新增 .cpp 必须同步 4 处**：`CMakeLists.txt`、`lua.vcxproj`、`lua.vcxproj.filters`、`lua_test.vcxproj`（已有 roadmap 文档规定，建议固化为脚本 `tools/add_source.ps1`）。

---

## 风险与回退策略

| 风险 | 影响 | 缓解 |
|---|---|---|
| VM dispatch 命令模式化引入函数指针表，调试器单步体验下降 | 影响教学 | 保留 `SwitchDispatch` 作为默认；调试构建强制使用 |
| `std::expected` 大范围替换异常导致调用链翻新 | 412 测试可能批量红 | 按 4.1 表格逐个函数迁移，每次 1 个函数 + 全量测试 |
| GC 去单例化破坏标准库内部对 `GarbageCollector::getInstance()` 的引用 | 编译错误广泛 | 保留 inline shim `getInstance()` 一版本，标记 `[[deprecated]]` |
| Visitor 化后 codegen 性能下降 | 不影响目标，但需观察 | 教学项目可接受；基准用 `examples/*.lua` 跑回归 |

---

## 推荐落地顺序（首 10 个可执行 PR）

1. PR-01：拆分 `garbage_collector.cpp` 为 `gc_mark/sweep/finalize/weak.cpp`（阶段 1.1.2）
2. PR-02：抽取 `vm.cpp` 主 switch 中算术分支到 `vm_arith.cpp`（阶段 1.1.1 子集）
3. PR-03：统一异常类型层级 `src/common/errors.hpp`（阶段 1.3）
4. PR-04：新增 `src/compiler/ast_visitor.hpp` 并改造 `codegen_expr.cpp` 一个分支作为示例（阶段 2.1）
5. PR-05：`Parser::parse()` → `std::expected<Chunk, ParseError>`（阶段 3.1）
6. PR-06：`lua_bytecode` 输出补全 source/numparams/maxStackSize/常量表（阶段 4.2 第 1–3 步）
7. PR-07：REPL 持久化历史 + `.help` / `.bytecode` 元命令（阶段 4.4.1 + 4.4.3）
8. PR-08：`GarbageCollector` 去单例化（保留 deprecated shim）（阶段 2.3 第 1 步）
9. PR-09：抽象 `DispatchStrategy`，默认 `SwitchDispatch`（阶段 2.2 第 3 步）
10. PR-10：补 `docs/walkthroughs/hello-world.md` 端到端教学（阶段 4.3 第 1 篇）

每个 PR 完成后跑：

```powershell
.\tools\run_quality_gate.ps1
.\bin\lua_test.exe
```
