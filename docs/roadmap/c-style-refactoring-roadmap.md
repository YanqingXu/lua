---
status: active
verified_against: docs/roadmap/current.md; src/compiler/lexer/lexer.cpp; src/compiler/parser/parser_stmt.cpp; src/compiler/parser/parser_primary.cpp; src/compiler/codegen/codegen.hpp; src/compiler/codegen/codegen.cpp; src/compiler/codegen/function_compiler.cpp; src/compiler/codegen/codegen_context.hpp; src/compiler/codegen/scope_manager.cpp; src/compiler/opcode.hpp; src/bytecode/bytecode_main.cpp; src/bytecode/bytecode_printer.cpp; src/vm/vm.cpp; src/vm/vm_call.cpp; src/vm/vm_frame.cpp; src/vm/vm_ops.cpp; src/vm/vm_handlers.cpp; src/vm/vm_switch_dispatch.hpp; src/vm/state/global_state.cpp; src/vm/state/global_state.hpp; src/vm/state/lua_state.cpp; src/gc/garbage_collector.cpp; src/gc/gc_sweep.cpp; src/gc/gc_mark.cpp; src/gc/gc_strategy.cpp; src/core/string_pool.cpp; src/core/thread.cpp; src/core/upvalue.cpp; src/core/userdata.cpp; src/core/userdata.hpp; src/common/macros.hpp; src/common/number_conversion.hpp; src/main.cpp; src/app/app_options.hpp; src/lib/baselib.cpp; src/lib/iolib.cpp; src/lib/iolib.hpp; src/lib/mathlib.cpp; src/lib/oslib.cpp; src/lib/packagelib.cpp; src/lib/stringlib.cpp; src/lib/tablelib.cpp; tests/unit/official/test_official_suite.cpp
last_checked: 2026-06-01
applies_to: src/ C-style pattern audit and C++23 refactoring roadmap
---

# C 风格残留重构路线图

> **给后续协作会话使用：** 本文档记录 `src/` 当前残留的 C 风格编码模式，并把它们拆成可独立提交、可回归验证的 C++23 重构阶段。实施前先确认命中清单仍然有效；每完成一个阶段，都要补充实际修改文件、验证命令和新增护栏。

**目标：** 在不改变 Lua 5.1 行为和现有 GC 语义的前提下，逐步清理 `src/` 中的手动内存管理、原生数组/`char*`、C 风格转换、简单宏、空指针哨兵和错误码式异常状态。

**总体策略：** 先用低风险的常量、数组和转换替换建立节奏，再收敛 Codegen/VM 中的局部手动所有权，随后把 GC 对象创建集中到显式工厂，最后处理标准库和入口层的 C API 兼容边界。GC 托管对象可以继续以裸指针作为观察句柄，但对象所有权必须由 `GarbageCollector` 或局部 RAII guard 明确承接。

**技术栈：** C++23-preview/MSVC `/W4`、`std::expected`、`std::optional`、`std::array`、`std::span`、`std::string_view`、RAII guard、PowerShell 质量门、`bin\lua_test.exe`。

**完成状态（2026-06-01）：** 阶段 0-6 已实施完成，并追加现代教学审计落地项。当前 C-style 护栏剩余登记值为 `NULL=0`、裸 `new=0`、裸 `delete=1`、`std::free/free=1`、简单 `#define=51`、`(void*)=0`、`return nullptr=60`；裸 `delete` 与 `std::free` 已改为位置 allowlist，raw array / `char* end` / 测试目录手动所有权进入 warning 规则。最终验证通过 `tools\run_quality_gate.ps1`、`bin\lua_test.exe` 全量 `644 registered / 3290 results / 0 failures`、`lua_app.vcxproj` Debug|x64 构建和 `git diff --check`。

---

## 审计摘要

本次初始扫描没有发现 `NULL`。原始风险集中在：

- `new Type(...)` 实际生产命中 45 处，分布为 VM 7 处、Bytecode/Codegen 3 处、GC/Core 5 处、Lib/App/Repl 30 处。
- `delete obj` / `delete block` 实际生产命中 8 处，分布为 GC/Core 4 处、Bytecode/Codegen 4 处。
- `std::free` 命中 2 处：`src/core/userdata.cpp:68` 和 `src/lib/oslib.cpp:187`。
- `#define` 命中 67 处；其中大量来自 `src/common/macros.hpp` 的函数式宏，另有 `src/vm/vm.cpp:243`、`src/main.cpp:61`、`src/main.cpp:69`、`src/lib/mathlib.cpp:29` 这类具体业务宏。
- C 风格转换主要是 `(void)` 丢弃返回值/参数和 `src/vm/vm.cpp:163` 的 `(void*)proto`；普通数值转换多数已使用 `static_cast`。
- `return nullptr` 在核心模块仍有 VM 6 处、Parser 3 处、GC/Core 9 处，标准库/入口层还有更多 C API 风格查找失败路径。
- `return false`、`return -1`、`return LUA_ERRRUN` 在 Lexer、Parser、VM、GC、Bytecode 中仍承担“无结果/错误/状态”混合语义，需要分批换成 `std::optional` 或 `std::expected`。

### 完整手动分配命中清单

| 区域 | `new` 位置 |
|---|---|
| VM | `src/vm/vm_frame.cpp:38`; `src/vm/vm_call.cpp:224`; `src/vm/vm_handlers/vm_handlers_table.cpp:64`; `src/vm/state/global_state.cpp:61`; `src/vm/state/lua_state.cpp:132`; `src/vm/state/lua_state.cpp:143`; `src/vm/state/lua_state.cpp:213` |
| Bytecode/Codegen | `src/compiler/codegen/codegen.cpp:84`; `src/compiler/codegen/function_compiler.cpp:148`; `src/compiler/codegen/codegen_context.hpp:178` |
| GC/Core | `src/core/string_pool.cpp:39`; `src/core/upvalue.cpp:20`; `src/core/upvalue.cpp:24`; `src/core/userdata.cpp:33`; `src/core/thread.cpp:47` |
| App/Repl | `src/main.cpp:187`; `src/repl.cpp:274`; `src/repl/repl_exe.cpp:57` |
| Lib | `src/lib/baselib.cpp:490`; `src/lib/baselib.cpp:563`; `src/lib/baselib.cpp:624`; `src/lib/baselib.cpp:951`; `src/lib/baselib.cpp:1021`; `src/lib/coroutinelib.cpp:190`; `src/lib/debuglib.cpp:303`; `src/lib/debuglib.cpp:820`; `src/lib/iolib.cpp:157`; `src/lib/iolib.cpp:1055`; `src/lib/lib_registry.cpp:30`; `src/lib/lib_registry.cpp:76`; `src/lib/oslib.cpp:402`; `src/lib/packagelib.cpp:260`; `src/lib/packagelib.cpp:481`; `src/lib/packagelib.cpp:623`; `src/lib/packagelib.cpp:1099`; `src/lib/packagelib.cpp:1150`; `src/lib/packagelib.cpp:1161`; `src/lib/packagelib.cpp:1184`; `src/lib/packagelib.cpp:1190`; `src/lib/packagelib.cpp:1195`; `src/lib/packagelib.cpp:1200`; `src/lib/packagelib.cpp:1205`; `src/lib/stringlib.cpp:1084`; `src/lib/stringlib.cpp:1443`; `src/lib/tablelib.cpp:420` |

| 区域 | `delete` / `free` 位置 |
|---|---|
| GC/Core `delete` | `src/gc/gc_sweep.cpp:43`; `src/gc/garbage_collector.cpp:431`; `src/gc/garbage_collector.cpp:570`; `src/core/thread.cpp:33` |
| Bytecode/Codegen `delete` | `src/compiler/codegen/codegen.cpp:115`; `src/compiler/codegen/codegen_context.hpp:202`; `src/compiler/codegen/codegen_context.hpp:208`; `src/compiler/codegen/scope_manager.cpp:117` |
| C allocator `free` | `src/core/userdata.cpp:68`; `src/lib/oslib.cpp:187` |

## 审计报告

### Lexer

| 类别 | 位置 | 当前模式 | 建议替代 |
|---|---|---|---|
| 原生数组 | `src/compiler/lexer/lexer.cpp:50` | `constexpr SimpleEscape kSimpleEscapes[]` | `constexpr std::array<SimpleEscape, N>` |
| `char*` 字符串扫描 | `src/compiler/lexer/lexer.cpp:78-79` | `constexpr const char*` + `std::strchr` | `constexpr std::string_view` + `find()` |
| 哨兵错误值 | `src/compiler/lexer/lexer.cpp:343-376` | `readLongBracketDelimiter()` 用 `-1` 表示无效分隔符 | `Opt<i32>`，调用方用 `has_value()` |
| C 字符串数值解析 | `src/compiler/lexer/lexer.cpp:439-455`、`src/compiler/lexer/lexer.cpp:487-490` | `char* end` + `std::strtod` / `std::strtoll` | 统一到 `luaStringToNumber()` 或新增 `parseLuaNumber(StrView) -> std::expected<LuaNumber, LexError>` |

Lexer 已经在空白/注释处理上大量使用 `Opt<Token>`，因此优先把 `-1` 分隔符哨兵和局部 `char* end` 收拢到同一风格。

### Parser

| 类别 | 位置 | 当前模式 | 建议替代 |
|---|---|---|---|
| `nullptr` 错误返回 | `src/compiler/parser/parser_stmt.cpp:249` | `error(...)` 后返回 `nullptr` | 让 `error()` / `errorAt()` 保持 `[[noreturn]]`，删除不可达返回或用 `std::unreachable()` |
| `nullptr` 错误返回 | `src/compiler/parser/parser_stmt.cpp:383` | `errorAt(...)` 后返回 `nullptr` | 同上 |
| `nullptr` 错误返回 | `src/compiler/parser/parser_primary.cpp:98` | `error("unexpected symbol")` 后返回 `nullptr` | 同上 |
| 结果通道 | `src/compiler/parser/parser.hpp:92` | 对外已是 `std::expected<Chunk, ParseError>` | 保持；清理内部死返回，避免新代码误以为 `ExprPtr` / `StmtPtr` 可为空表示错误 |

Parser 的对外接口已经较现代，重构目标是消除内部“抛异常 + 继续返回空指针”的混合信号。

### Bytecode / Codegen

这里把 `src/bytecode` 工具、`src/compiler/codegen` 和 opcode metadata 作为同一条字节码生成链处理。

| 类别 | 位置 | 当前模式 | 建议替代 |
|---|---|---|---|
| 手动创建 GC 对象 | `src/compiler/codegen/codegen.cpp:84` | `state_.proto = new Proto(); registerObject(...)` | `services.gc.create<Proto>()` 或 `GCAllocationGuard<Proto>` |
| 手动失败清理 | `src/compiler/codegen/codegen.cpp:115` | `unregisterObject(failedProto); delete failedProto;` | `GCAllocationGuard` 在 commit 前自动 unregister/delete，或 collector 接管为唯一所有者 |
| 手动创建子原型 | `src/compiler/codegen/function_compiler.cpp:148` | `new Proto()` 后注册 | 同上，子原型创建必须和父 proto 设置保持异常安全 |
| 手动 block 栈 | `src/compiler/codegen/codegen_context.hpp:178`、`:202`、`:208` | `new BlockInfo` / `delete` 链表 | `UPtr<BlockInfo>`、`Vec<BlockInfo>` 栈，或 `std::vector<BlockInfo>` + 索引 |
| 手动 block 释放 | `src/compiler/codegen/scope_manager.cpp:117` | `delete block` | 与 `BlockManager` 同步改为 RAII |
| 原始返回所有权 | `src/compiler/codegen/codegen.hpp:83`、`:90`、`src/compiler/codegen/function_compiler.hpp:29` | `Proto*` 表示 GC 托管结果 | 保留观察指针但命名和文档说明为 non-owning；内部使用 RAII guard |
| C 风格丢弃 | `src/compiler/codegen/expression_emitter.cpp:744` | `(void)ops_.codeRaw(...)` | `[[maybe_unused]] const i32 blockPc = ...;` 或让 helper 明确返回 `void` |
| `char*` metadata | `src/compiler/opcode.hpp:324`、`:336`、`:475` | opcode 名称为 `const char*` | `std::string_view`，对 C 接口再提供 `.data()` |
| Bytecode CLI 错误码 | `src/bytecode/bytecode_main.cpp:33-70` | `parseOptions(...) -> bool` 并写 stderr | `std::expected<BytecodeToolOptions, Str>`，`main()` 负责呈现错误 |
| Bytecode CLI `char**` | `src/bytecode/bytecode_main.cpp:33`、`:97` | 直接在解析函数中使用 `argc/argv` | `std::span<char*>` 或入口处复制为 `Vec<StrView>` |
| 编译失败空指针 | `src/bytecode/bytecode_main.cpp:80-91` | `CodeGenerator::generate()` 返回 `Proto*` 后判空 | 改走 `tryGenerate()`，失败路径用 `expected` 传递 |
| C 字符串遍历 | `src/bytecode/bytecode_printer.cpp:20-22` | `escapeString(const char*)` 指针循环 | `escapeString(std::string_view)` |

### VM

| 类别 | 位置 | 当前模式 | 建议替代 |
|---|---|---|---|
| 手动创建闭包 | `src/vm/vm_frame.cpp:38` | `new Function(childProto)` 后注册 | `gc.create<Function>(childProto)`，返回 non-owning `Function*` |
| 手动创建 vararg 表 | `src/vm/vm_call.cpp:224` | `new Table()` 后注册 | `gc.create<Table>()`，并在赋给寄存器前用临时 root/guard 防止异常泄漏 |
| 手动创建表 | `src/vm/vm_handlers/vm_handlers_table.cpp:64` | `new Table()` 后注册 | 同上 |
| 手动创建状态 | `src/vm/state/lua_state.cpp:132`、`:143` | `new LuaState(...)` 返回裸指针 | `UPtr<LuaState>` 作为内部所有权；Lua C API 兼容层可返回观察指针 |
| 手动创建全局表 | `src/vm/state/lua_state.cpp:213` | `new Table()` 后注册并 root | `gc.createRoot<Table>()` 或 `RootGuard` |
| 手动创建 registry | `src/vm/state/global_state.cpp:61` | `new Table()` 后注册/root/fixed | `gc.createFixedRoot<Table>()` |
| 原生数组 | `src/vm/state/global_state.hpp:237`、`:240` | `Table* metatables_[9]`、`GCString* tmname_[...]` | `std::array<Table*, static_cast<usize>(ValueType::Thread)+1>`、`std::array<GCString*, ...>` |
| 原生字符串表 | `src/vm/state/global_state.cpp:148`、`:182`、`src/vm/state/lua_state.cpp:876` | `static const char* ...[]` | `constexpr std::array<std::string_view, N>` |
| 固定缓冲区 | `src/vm/vm_ops.cpp:41`、`src/vm/state/lua_state.cpp:916` | `char buffer[64]` + `std::snprintf` | 共享 `formatLuaNumber()`，优先 `std::to_chars` 或 `std::format` |
| C 风格转换 | `src/vm/vm.cpp:163` | `(void*)proto` | `static_cast<const void*>(proto)` 或 `std::format` 指针输出 |
| 函数式宏 | `src/vm/vm.cpp:243` | `LUA_VM_RUN_SWITCH_OP(handlerName)` | `runSwitchHandler(handler)` inline helper 或 lambda |
| 空指针查找失败 | `src/vm/vm_handlers.cpp:51`、`src/vm/vm_switch_dispatch.hpp:207` | handler lookup 返回 `nullptr` | `Opt<OpHandler>`，热路径可用 `has_value()` 后解包 |
| 空指针查找失败 | `src/vm/state/global_state.cpp:85`、`:172` | 越界返回 `nullptr` | `Opt<Table*>` / `Opt<GCString*>`，或越界时 `std::nullopt` |
| API 错误码 | `src/vm/state/lua_state.cpp:591-718` | `pcall()` 返回 `LUA_ERRRUN` | 保留 Lua C API facade，新增 `tryPCall(...) -> std::expected<i32, RuntimeError>` 作为核心路径 |
| 捕获异常后返回空 | `src/vm/state/lua_state.cpp:922-924` | `toString()` 捕获异常返回 `nullptr` | `tryToString() -> Opt<std::string_view>`，旧 API 包装兼容 |

VM 已经有 `tryExecuteProto() -> std::expected<ExecResult, RuntimeError>`，后续应把 pcall、handler lookup、LuaState typed accessors 逐步接到同一错误通道上。

### GC

| 类别 | 位置 | 当前模式 | 建议替代 |
|---|---|---|---|
| 手动对象销毁 | `src/gc/gc_sweep.cpp:43` | sweep 直接 `delete obj` | `destroyObject(obj, stringPool)` helper，后续迁移到 collector-owned `unique_ptr` 存储 |
| 手动对象销毁 | `src/gc/garbage_collector.cpp:431`、`:570` | mark-sweep / clearAll 直接 `delete obj` | 同上，集中析构、string pool 移除和计数更新 |
| 旧手动 delete 兼容 | `src/gc/garbage_collector.hpp:121` | 注释承认仍有手动 delete 路径 | 作为阶段验收：删除该兼容说明或改为“仅 collector 内部销毁” |
| C 风格丢弃 | `src/gc/gc_mark.cpp:219`、`src/gc/garbage_collector.cpp:460`、`:471` | `(void)propagateMarks(...)` / `(void)sweepStep(...)` | 使用命名的 `[[maybe_unused]]` 结果，或让无结果 wrapper 调用有结果实现 |
| 查找失败空指针 | `src/gc/gc_strategy.cpp:48` | `findGCStrategy()` 返回 `nullptr` | `Opt<std::reference_wrapper<const GCStrategy>>` |
| 查找失败空指针 | `src/gc/gc_mark.cpp:42`、`:170` | root/frame 对象解析失败返回 `nullptr` | `Opt<Function*>` / `Opt<GCObject*>` |
| 工厂分散 | `src/core/string_pool.cpp:39` | `new GCString(str)` 后注册 | `gc.create<GCString>(str)` |
| 工厂分散 | `src/core/upvalue.cpp:20`、`:24` | `new Upvalue(...)` | `gc.create<Upvalue>(...)` 或 `UpvalueFactory`，确保 owner 明确 |
| 工厂分散 | `src/core/userdata.cpp:33` | `return new Userdata(size)` | `Userdata::create(...) -> std::expected<Userdata*, AllocError>` 并由 GC 工厂注册 |
| 手动 state 删除 | `src/core/thread.cpp:33` | `delete state_` | `UPtr<LuaState> state_` |
| C allocator 释放 | `src/core/userdata.cpp:68` | `std::free(data_)` | `unique_ptr<std::byte, AlignedDeleter>` 或 `AlignedAllocation` RAII 成员 |

GC 重构的原则是“所有权现代化，观察关系保持轻量”：`Value`、`Table`、`Function` 等运行时对象可以继续保存裸指针作为非拥有引用，但对象生命周期只能由 GC 或明确 RAII 类型控制。

### 支撑模块

这些不属于五个核心模块，但在 `src/` 全量审计中命中较多，建议在核心模式稳定后处理。

| 区域 | 位置 | 当前模式 | 建议替代 |
|---|---|---|---|
| App/Main 宏常量 | `src/main.cpp:61`、`:69` | `#define LUA_TEST_SCRIPT_PATH` / `LUA_TRACE_TEST_SCRIPT_OUTPUT` | 生成式配置头或 `constexpr std::string_view` 包装，宏只留在 build boundary |
| App/Main argv | `src/main.cpp:183`、`:665`、`src/app/app_options.hpp:28-33`、`src/app/app_options.cpp:7` | `char* argv[]`、`char** argv`、`const char*` 字段 | `std::span<char*>` 入口适配，`AppOptions` 存 `Str` / `StrView` |
| App/Main 分配 | `src/main.cpp:187` | `new Table()` | `gc.create<Table>()` |
| Common 宏 | `src/common/macros.hpp:43-371` | 断言、日志、unused、cast、bit、align 函数式宏 | 分批替换为 `inline` / `constexpr` / `consteval` 函数；平台属性宏单独保留 |
| Common 数值解析 | `src/common/number_conversion.hpp:11-29` | `char* end` + `errno` | 先保留内部实现，外层提供 `expected` 结果；C 指针限制在一个 helper 内 |
| Math 常量 | `src/lib/mathlib.cpp:29` | `#define M_PI` | `std::numbers::pi_v<f64>` |
| OS env | `src/lib/oslib.cpp:181-187` | `_dupenv_s` 缓冲区由 `std::free` 释放 | `unique_ptr<char, decltype(&std::free)>` |
| IO 文件句柄 | `src/lib/iolib.cpp:44-47`、`:61`、`:990`、`:1011` | `FILE*`、`char path[1024]`、固定缓冲区 | `FileHandle` RAII wrapper、`std::filesystem::path` / `Str`、`std::array<char,N>` |
| Package 路径 | `src/lib/packagelib.cpp:54-92`、`:179`、`:186`、`:239`、`:1223` | `static const char*` 常量、`PATH_MAX` 缓冲区、nullptr 结尾数组 | `constexpr std::string_view`、`std::array<char,N>` 或 `std::filesystem::path`、`std::array<std::string_view,N>` |
| String 模式匹配 | `src/lib/stringlib.cpp:428-589` | 大量 `const char*` 指针游标，`nullptr` 表示匹配失败 | 单独封装 `PatternCursor` / `MatchResult`；短期先不要和核心 GC PR 混在一起 |
| Lib 分配 | `src/lib/*.cpp` 多处 | `new Table()` / `new Function()` 后注册 | 统一 `LibraryObjectFactory`，内部走 GC factory |

## 重构目标

### 内存管理

- 新增 `GarbageCollector::create<T>(Args&&...) -> T*`，内部先用 `std::make_unique<T>()` 临时持有，注册成功后交给 collector；构造或注册失败时自动释放。
- 新增 `GarbageCollector::createRoot<T>()` 和 `createFixedRoot<T>()`，覆盖 registry、global table、metatable name 等固定对象。
- 为 Codegen 的 `Proto` 创建引入 `GCAllocationGuard<T>`，在 `generateUnchecked()` 完成前保护失败路径，替代手写 `unregisterObject()` + `delete`。
- 将 Codegen block 链从裸 `new/delete` 改成 `std::vector<BlockInfo>` 栈或 `UPtr<BlockInfo>` 链。
- 为 `Userdata` 和 OS env 缓冲区引入专用 RAII deleter，避免裸 `std::free` 散落。

### 数据结构与字符串

- 用 `std::array` 替代固定大小原生数组；数组大小来自 enum 时使用 `static_cast<usize>(Enum::Count)`。
- 用 `std::string_view` 表达只读字符串常量和 opcode/type/metamethod 名称；仅在需要 NUL 终止的 C API 边界调用 `.data()`。
- 用 `std::span<char*>` 包装 `argc/argv`，入口层立即转成 `Str` / `StrView` 传递。
- 固定格式化缓冲区优先改为 `std::format`；性能敏感数字格式化统一为 `formatLuaNumber()`，内部可用 `std::to_chars`。

### 类型转换

- 将 `(void*)ptr` 替换为 `static_cast<const void*>(ptr)` 或 `std::format` 指针输出。
- 将 `(void)expr` 替换为 `[[maybe_unused]] const auto ignored = expr;`、`std::ignore = expr;`，或调整 helper 返回 `void`。
- 删除 `LUA_STATIC_CAST`、`LUA_DYNAMIC_CAST`、`LUA_CONST_CAST`、`LUA_REINTERPRET_CAST` 这类简单 cast 宏，直接使用标准 cast。

### 常量与宏

- 简单常量宏改为 `inline constexpr` / `constexpr std::string_view`。
- 简单函数宏改为 `inline` 或 `constexpr` 函数模板，例如 bit/align helper。
- 日志/断言宏保留必要的 `__FILE__` / `__LINE__` 能力时，迁移到 `std::source_location`。
- `LUA_VM_RUN_SWITCH_OP` 改为 inline helper/lambda，保持热路径可内联。
- include guard 宏在已有 `#pragma once` 的头文件中逐步删除。

### 空指针

- `NULL` 已清零，继续用质量门守住。
- `nullptr` 作为“没有对象”的普通值可以保留，但“失败/错误/查找不到”应改为 `std::optional`、`std::expected` 或明确命名的 nullable observer。
- 对 public API 中必须兼容 Lua C 风格的返回值，新增现代核心接口，旧接口只做薄包装。

### 错误处理

- Lexer：哨兵 `-1` 改为 `Opt<i32>`；数值解析错误改为结构化 `LexError` 或 token error。
- Parser：删除异常路径后的空指针返回；保持 `parse() -> std::expected<Chunk, ParseError>`。
- Bytecode 工具：`parseOptions()` 改为 `std::expected<BytecodeToolOptions, Str>`。
- VM：以 `tryExecuteProto()` 为样板，为 `pcall`、handler lookup 和 typed accessor 提供 expected/optional 版本。
- GC：策略查找、frame root 查找和 object extraction 改为 optional，避免 `nullptr` 同时表示“合法没有”和“异常失败”。

## 实施步骤

### 阶段 0：基线与护栏

**目标：** 固定当前命中面，避免新 C 风格模式继续扩散。

涉及文件：

- 创建：`tools/check_c_style_patterns.ps1`
- 修改：`tools/run_quality_gate.ps1`
- 修改：`tools/test_quality_gate.ps1`
- 修改：`docs/roadmap/c-style-refactoring-roadmap.md`

步骤：

- [x] 增加只读扫描脚本，检查 `NULL`、新增裸 `new/delete`、新增简单 `#define`、新增 `(void*)`。
- [x] 初始 allowlist 使用本文档列出的命中位置，脚本输出“当前允许数量”和新增违规。
- [x] 将脚本加入 `run_quality_gate.ps1`，但第一版只阻止新增，不要求立即清零。
- [x] 运行 `powershell -NoProfile -ExecutionPolicy Bypass -File tools\test_quality_gate.ps1` 确认门禁自检覆盖新脚本。

实际修改文件：

- `tools/check_c_style_patterns.ps1`
- `tools/run_quality_gate.ps1`
- `tools/test_quality_gate.ps1`

验证命令：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\check_c_style_patterns.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\test_quality_gate.ps1
```

剩余命中数：

- `NULL`: 0
- 裸 `new`: 45
- 裸 `delete`: 8
- `std::free` / `free`: 2
- 简单 `#define`: 67
- `(void*)` 风格转换: 1
- `return nullptr`: 70

### 阶段 1：低风险常量、数组和转换清理

**目标：** 不触碰生命周期和语义，先清除最容易验证的 C 风格表面。

优先文件：

- `src/compiler/lexer/lexer.cpp`
- `src/vm/state/global_state.hpp`
- `src/vm/state/global_state.cpp`
- `src/vm/state/lua_state.cpp`
- `src/vm/vm_ops.cpp`
- `src/vm/vm.cpp`
- `src/lib/mathlib.cpp`
- `src/common/macros.hpp`

步骤：

- [x] `kSimpleEscapes`、`metamethodNames`、`reservedWords`、`typeNames` 改为 `std::array` / `std::string_view`。
- [x] `kSingleCharTokens` 改为 `std::string_view`，替代 `std::strchr`。
- [x] `M_PI` 改为 `std::numbers::pi_v<f64>`。
- [x] `src/vm/vm.cpp:163` 的 `(void*)proto` 改为标准 cast 或 `std::format`。
- [x] 将 `(void)` 丢弃返回值替换为命名的 ignored result；未使用参数优先用 `[[maybe_unused]]`。
- [x] 为 `macros.hpp` 中 bit/align/cast 宏建立 `constexpr` helper，并先迁移调用点最少的一组。

实际修改文件：

- `src/common/number_conversion.hpp`
- `src/common/macros.hpp`
- `src/compiler/lexer/lexer.cpp`
- `src/compiler/codegen/expression_emitter.cpp`
- `src/gc/gc_mark.cpp`
- `src/gc/garbage_collector.cpp`
- `src/lib/mathlib.cpp`
- `src/vm/vm.cpp`
- `src/vm/vm_ops.cpp`
- `src/vm/state/global_state.hpp`
- `src/vm/state/global_state.cpp`
- `src/vm/state/lua_state.cpp`
- `src/vm/vm_handlers/vm_handlers_global_upvalue.cpp`
- `src/vm/vm_handlers/vm_handlers_table.cpp`
- `tools/check_c_style_patterns.ps1`

验证命令：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\check_c_style_patterns.ps1
MSBuild.exe lua_test.vcxproj /m /p:Configuration=Debug /p:Platform=x64
bin\lua_test.exe --filter "Lexer Number"
bin\lua_test.exe --filter "VM Core"
bin\lua_test.exe --filter "VM Dispatch"
bin\lua_test.exe --filter "Function Codegen"
bin\lua_test.exe --filter "GC"
bin\lua_test.exe --filter "Math"
```

剩余命中数：

- `NULL`: 0
- 裸 `new`: 45
- 裸 `delete`: 8
- `std::free` / `free`: 2
- 简单 `#define`: 51
- `(void*)` 风格转换: 0
- `return nullptr`: 70

验证：

```powershell
bin\lua_test.exe --filter "Lexer Number"
bin\lua_test.exe --filter "VM Core"
bin\lua_test.exe
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1
```

### 阶段 2：Codegen 局部所有权收敛

**目标：** 在进入全局 GC 所有权改造前，先消除 Codegen 中独立的手动 `new/delete`。

优先文件：

- `src/compiler/codegen/codegen_context.hpp`
- `src/compiler/codegen/scope_manager.cpp`
- `src/compiler/codegen/codegen.cpp`
- `src/compiler/codegen/function_compiler.cpp`
- `src/compiler/codegen/codegen.hpp`
- `src/compiler/codegen/function_compiler.hpp`

步骤：

- [x] 将 `BlockManager` / `ScopeManager` 的 block 链改为 RAII 存储，不再手动 `delete`。
- [x] 新增 `GCAllocationGuard<Proto>`，覆盖 `CodeGenerator::generateUnchecked()` 的主 proto。
- [x] 子函数 `FunctionCompiler::compile()` 也走同一 guard，只有在挂到父 proto 后 commit。
- [x] 将 `generate()` 文档改为“返回 GC 托管 non-owning `Proto*`”；`tryGenerate()` 作为首选入口。
- [x] Bytecode 工具改用 `tryGenerate()`，删除 `if (!proto)` 异常拼接路径。

实际修改文件：

- `src/compiler/codegen/gc_allocation_guard.hpp`
- `src/compiler/codegen/codegen_context.hpp`
- `src/compiler/codegen/codegen.cpp`
- `src/compiler/codegen/codegen.hpp`
- `src/compiler/codegen/codegen_stmt.cpp`
- `src/compiler/codegen/function_compiler.cpp`
- `src/compiler/codegen/function_compiler.hpp`
- `src/compiler/codegen/expression_emitter.cpp`
- `src/compiler/codegen/expression_emitter.hpp`
- `src/compiler/codegen/statement_emitter.cpp`
- `src/compiler/codegen/statement_emitter.hpp`
- `src/compiler/codegen/scope_manager.cpp`
- `src/bytecode/bytecode_main.cpp`
- `CMakeLists.txt`
- `lua.vcxproj`
- `lua.vcxproj.filters`

验证命令：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\check_c_style_patterns.ps1
MSBuild.exe lua_test.vcxproj /m /p:Configuration=Debug /p:Platform=x64
MSBuild.exe lua_bytecode.vcxproj /m /p:Configuration=Debug /p:Platform=x64
bin\lua_test.exe --filter "Function Codegen"
bin\lua_test.exe --filter "Lua File Compilation"
bin\lua_test.exe --filter "Codegen Result Types"
bin\lua_test.exe --filter "Scope Manager"
```

剩余命中数：

- `NULL`: 0
- 裸 `new`: 42
- 裸 `delete`: 4
- `std::free` / `free`: 2
- 简单 `#define`: 51
- `(void*)` 风格转换: 0
- `return nullptr`: 70

验证：

```powershell
bin\lua_test.exe --filter "Function Codegen"
bin\lua_test.exe --filter "Lua File Compilation"
bin\lua_test.exe --filter "Codegen Result Types"
bin\lua_test.exe
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1
```

### 阶段 3：GC 对象创建工厂

**目标：** 让 `new Table()` / `new Function()` / `new Proto()` / `new GCString()` 不再散落在 VM、Core 和标准库里。

优先文件：

- `src/gc/garbage_collector.hpp`
- `src/gc/garbage_collector.cpp`
- `src/core/string_pool.cpp`
- `src/vm/vm_frame.cpp`
- `src/vm/vm_call.cpp`
- `src/vm/vm_handlers/vm_handlers_table.cpp`
- `src/vm/state/global_state.cpp`
- `src/vm/state/lua_state.cpp`
- `src/main.cpp`
- `src/lib/lib_registry.cpp`

步骤：

- [x] 增加 `GarbageCollector::create<T>()`，单测覆盖构造成功、构造抛异常、注册后 object count。
- [x] 增加 `createRoot<T>()` / `createFixedRoot<T>()`，迁移 registry、global table、arg table。
- [x] 迁移 VM 热路径表/闭包创建，确保创建后寄存器和 base refresh 逻辑不变。
- [x] 迁移 `StringPool::intern()` 中 `GCString` 创建，保持 string pool 去重语义。
- [x] 迁移标准库公共 helper：`createCClosure()`、`createPackageTableObject()`、`createGCManagedTable()`。
- [x] 更新 allowlist，禁止新增 `new Table` / `new Function` / `new Proto`。

实际修改文件：

- `src/gc/garbage_collector.hpp`
- `src/core/string_pool.cpp`
- `src/vm/vm_frame.cpp`
- `src/vm/vm_call.cpp`
- `src/vm/vm_handlers/vm_handlers_table.cpp`
- `src/vm/state/global_state.cpp`
- `src/vm/state/lua_state.cpp`
- `src/main.cpp`
- `src/repl.cpp`
- `src/repl/repl_exe.cpp`
- `src/lib/baselib.cpp`
- `src/lib/coroutinelib.cpp`
- `src/lib/debuglib.cpp`
- `src/lib/iolib.cpp`
- `src/lib/lib_registry.cpp`
- `src/lib/oslib.cpp`
- `src/lib/packagelib.cpp`
- `src/lib/stringlib.cpp`
- `src/lib/tablelib.cpp`
- `tests/unit/gc/test_gc.cpp`
- `tools/check_c_style_patterns.ps1`

验证命令：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\check_c_style_patterns.ps1
MSBuild.exe lua_test.vcxproj /m /p:Configuration=Debug /p:Platform=x64
MSBuild.exe lua_app.vcxproj /m /p:Configuration=Debug /p:Platform=x64
bin\lua_test.exe --filter "GC Create Factories"
bin\lua_test.exe --filter "GC"
bin\lua_test.exe --filter "StringPool"
bin\lua_test.exe --filter "VM Core"
bin\lua_test.exe --filter "Standard Library Catalog"
```

剩余命中数：

- `NULL`: 0
- 裸 `new`: 6
- 裸 `delete`: 4
- `std::free` / `free`: 2
- 简单 `#define`: 51
- `(void*)` 风格转换: 0
- `return nullptr`: 70

验证：

```powershell
bin\lua_test.exe --filter "GC"
bin\lua_test.exe --filter "StringPool"
bin\lua_test.exe --filter "VM Core"
bin\lua_test.exe --filter "Standard Library Catalog"
bin\lua_test.exe
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1
```

### 阶段 4：GC 销毁路径和 Userdata RAII

**目标：** 集中对象销毁策略，去掉 collector 外部和核心对象中的裸释放。

优先文件：

- `src/gc/gc_sweep.cpp`
- `src/gc/garbage_collector.cpp`
- `src/gc/garbage_collector.hpp`
- `src/core/userdata.hpp`
- `src/core/userdata.cpp`
- `src/core/thread.hpp`
- `src/core/thread.cpp`

步骤：

- [x] 增加 `GarbageCollector::destroyObject(GCObject*)` 私有 helper，集中 string pool remove、owner 清理和 delete。
- [x] 将 sweep、incremental sweep、clearAll 改为调用 helper。
- [x] `Thread` 持有 `UPtr<LuaState>`，删除 `delete state_`。
- [x] `Userdata` 的 `data_` 改为 RAII wrapper，Windows `_aligned_free` 和 POSIX `std::free` 只出现在 deleter。
- [x] 重新审查 `GarbageCollector::unregisterObject()` 注释，去掉“兼容仍然手动 delete”表述。

实际修改文件：

- `src/gc/garbage_collector.hpp`
- `src/gc/garbage_collector.cpp`
- `src/gc/gc_sweep.cpp`
- `src/core/thread.hpp`
- `src/core/thread.cpp`
- `src/core/upvalue.hpp`
- `src/core/upvalue.cpp`
- `src/core/userdata.hpp`
- `src/core/userdata.cpp`
- `src/vm/state/lua_state.cpp`
- `src/lib/baselib.cpp`
- `src/lib/coroutinelib.cpp`
- `src/lib/iolib.cpp`
- `src/lib/stringlib.cpp`
- `tools/check_c_style_patterns.ps1`

验证命令：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\check_c_style_patterns.ps1
MSBuild.exe lua_test.vcxproj /m /p:Configuration=Debug /p:Platform=x64
bin\lua_test.exe --filter "GC"
bin\lua_test.exe --filter "Userdata"
bin\lua_test.exe --filter "Coroutine"
```

剩余命中数：

- `NULL`: 0
- 裸 `new`: 2
- 裸 `delete`: 1
- `std::free` / `free`: 2
- 简单 `#define`: 51
- `(void*)` 风格转换: 0
- `return nullptr`: 70

验证：

```powershell
bin\lua_test.exe --filter "GC"
bin\lua_test.exe --filter "Userdata"
bin\lua_test.exe --filter "Coroutine"
bin\lua_test.exe
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1
```

### 阶段 5：错误通道现代化

**目标：** 把“错误码 / `nullptr` / `false`”拆成正常布尔判断、optional 缺失和 expected 错误三类。

优先文件：

- `src/compiler/lexer/lexer.cpp`
- `src/compiler/parser/parser_impl.hpp`
- `src/compiler/parser/parser_stmt.cpp`
- `src/compiler/parser/parser_primary.cpp`
- `src/bytecode/bytecode_main.cpp`
- `src/gc/gc_strategy.cpp`
- `src/vm/vm_handlers.cpp`
- `src/vm/vm_switch_dispatch.hpp`
- `src/vm/state/lua_state.hpp`
- `src/vm/state/lua_state.cpp`

步骤：

- [x] Lexer `readLongBracketDelimiter()` 返回 `Opt<i32>`。
- [x] Parser 删除 `error()` 后不可达的 `return nullptr`，必要时用 `std::unreachable()` 表明控制流。
- [x] `findGCStrategy()` 返回 optional reference，`GarbageCollector::useStrategy()` 解包并保持 bool public API。
- [x] `handlerFor()` / `switchHandlerFor()` 返回 `Opt<OpHandler>` 或引入 `mustHandlerFor()` 抛 `RuntimeError`。
- [x] `LuaState` 新增 `tryPCall()`、`tryToString()`，旧 `pcall()` / `toString()` 作为兼容适配层。
- [x] `bytecode_main.cpp` 的 `parseOptions()` 返回 `std::expected<BytecodeToolOptions, Str>`。

实际修改文件：

- `src/compiler/lexer/lexer.hpp`
- `src/compiler/lexer/lexer.cpp`
- `src/compiler/parser/parser_impl.hpp`
- `src/compiler/parser/parser.cpp`
- `src/compiler/parser/parser_primary.cpp`
- `src/compiler/parser/parser_stmt.cpp`
- `src/gc/gc_strategy.hpp`
- `src/gc/gc_strategy.cpp`
- `src/gc/garbage_collector.cpp`
- `src/vm/vm_handlers.hpp`
- `src/vm/vm_handlers.cpp`
- `src/vm/vm_switch_dispatch.hpp`
- `src/vm/state/lua_state.hpp`
- `src/vm/state/lua_state.cpp`
- `src/bytecode/bytecode_main.cpp`
- `tests/unit/vm/test_vm_dispatch.cpp`
- `tools/check_c_style_patterns.ps1`

验证命令：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\check_c_style_patterns.ps1
MSBuild.exe lua_test.vcxproj /m /p:Configuration=Debug /p:Platform=x64
MSBuild.exe lua_bytecode.vcxproj /m /p:Configuration=Debug /p:Platform=x64
bin\lua_test.exe --filter "Lexer"
bin\lua_test.exe --filter "Parser"
bin\lua_test.exe --filter "VM Core"
bin\lua_test.exe --filter "VM Dispatch"
bin\lua_test.exe --filter "GC Strategy"
bin\lua_test.exe --filter "Lua File Compilation"
```

剩余命中数：

- `NULL`: 0
- 裸 `new`: 2
- 裸 `delete`: 1
- `std::free` / `free`: 2
- 简单 `#define`: 51
- `(void*)` 风格转换: 0
- `return nullptr`: 64

验证：

```powershell
bin\lua_test.exe --filter "Lexer"
bin\lua_test.exe --filter "Parser"
bin\lua_test.exe --filter "VM Core"
bin\lua_test.exe --filter "GC Strategy"
bin\lua_test.exe
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1
```

### 阶段 6：标准库和入口层收尾

**目标：** 清理 `lib/`、`main.cpp`、`app_options`、`repl` 中的 C 字符串、固定缓冲区和 C allocator 边界。

优先文件：

- `src/app/app_options.hpp`
- `src/app/app_options.cpp`
- `src/main.cpp`
- `src/lib/iolib.cpp`
- `src/lib/oslib.cpp`
- `src/lib/packagelib.cpp`
- `src/lib/stringlib.cpp`
- `src/lib/tablelib.cpp`
- `src/lib/baselib.cpp`

步骤：

- [x] `AppOptions` 保存 `Str` / `StrView`，入口用 `std::span<char*>` 适配 argv。
- [x] `FileHandleData::path` 改为 `Str` 或 `std::filesystem::path`，`FILE*` 包装到 RAII handle。
- [x] OS env 缓冲区改为 `unique_ptr<char, FreeDeleter>`。
- [x] `packagelib` 常量改为 `constexpr std::string_view`；路径缓冲区改为 `std::array<char,N>` 或 filesystem helper。
- [x] `stringlib` 模式匹配单独抽取 `PatternCursor`，把 `nullptr` 匹配失败换成 `Opt<const char*>` 或 `MatchResult`。
- [x] 标准库错误消息固定 `char buffer[N]` 改为 `std::format` 或共享 helper。

实际修改文件：

- `src/app/app_options.hpp`
- `src/app/app_options.cpp`
- `tests/unit/app/test_app_options.cpp`
- `src/main.cpp`
- `src/core/userdata.hpp`
- `src/core/userdata.cpp`
- `src/lib/baselib.cpp`
- `src/lib/iolib.cpp`
- `src/lib/mathlib.cpp`
- `src/lib/oslib.cpp`
- `src/lib/packagelib.cpp`
- `src/lib/stringlib.cpp`
- `src/lib/tablelib.cpp`

实施记录：

- `AppOptions` 现在持有 argv 拷贝后的 `Str`，`parseArgs(std::span<char* const>)` 作为现代入口，旧 `parseArgs(int, char**)` 仅保留薄适配层。
- `Userdata` 新增 typed placement construction / destruction 支持，允许 userdata 缓冲区安全保存非平凡 C++ 对象。
- `FileHandleData` 改为保存 `std::unique_ptr<FILE, FileCloser>` 和 `Str path`；显式 close 与 GC 析构兜底共享同一关闭逻辑。
- `oslib` 的 Windows 环境变量释放边界改为 `unique_ptr<char, std::free>`；时间、临时名和错误消息缓冲区迁移到 `std::array` / `std::format`。
- `packagelib` 的包路径常量迁移到 `constexpr StrView`，路径拼装缓冲区使用 `std::array`，标准库哨兵数组改为 `std::array<StrView, N>`。
- `stringlib` 外层模式匹配入口引入 `PatternCursor` / `MatchResult`。内部 Lua 5.1 模式匹配引擎仍保留局部 `const char*` 游标，范围限制在匹配实现内部，不再作为 public/outer 错误通道扩散。
- `baselib`、`mathlib`、`tablelib`、`iolib` 的固定格式化缓冲区迁移到 `std::format` 或 `luaNumberToString()`。

验证命令：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\check_c_style_patterns.ps1
MSBuild.exe lua_test.vcxproj /m /p:Configuration=Debug /p:Platform=x64
MSBuild.exe lua_app.vcxproj /m /p:Configuration=Debug /p:Platform=x64
bin\lua_test.exe --filter "AppOptions"
bin\lua_test.exe --filter "Base Library"
bin\lua_test.exe --filter "String Library"
bin\lua_test.exe --filter "Lua 5.1 Compatibility"
bin\lua_test.exe --filter "OS Library"
bin\lua_test.exe --filter "Package Library"
bin\lua_test.exe --filter "Table Library"
bin\lua_test.exe --filter "Math Library"
bin\lua_test.exe --filter "VM Core"
bin\lua_test.exe --filter "GC"
```

剩余命中数：

- `NULL`: 0
- 裸 `new`: 2
- 裸 `delete`: 1
- `std::free` / `free`: 1
- 简单 `#define`: 51
- `(void*)` 风格转换: 0
- `return nullptr`: 64

验证：

```powershell
bin\lua_test.exe --filter "Base Library"
bin\lua_test.exe --filter "String Library"
bin\lua_test.exe --filter "Lua 5.1 Compatibility"
bin\lua_test.exe --filter "OS Library"
bin\lua_test.exe --filter "Package Library"
bin\lua_test.exe --filter "AppOptions"
bin\lua_test.exe
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1
```

## 验证流程

每个阶段完成后至少运行：

```powershell
bin\lua_test.exe
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1
git diff --check
```

核心逻辑阶段还要运行对应过滤测试：

```powershell
bin\lua_test.exe --filter "Lexer Number"
bin\lua_test.exe --filter "Function Codegen"
bin\lua_test.exe --filter "VM Core"
bin\lua_test.exe --filter "GC"
```

质量门期望：

- `bin\lua_test.exe` 全量 0 failures。
- `tools\run_quality_gate.ps1` 构建 `lua_test.vcxproj` 并运行当前注册测试；本机没有 `clang-format` / `clang-tidy` 时按既有策略跳过对应项。
- `git diff --check` 无尾随空白或冲突标记。
- 新增 C 风格护栏脚本不允许新增裸 `new/delete`、`NULL`、简单常量宏、`(void*)` 转换或未登记的 `return nullptr` 错误通道。

## 维护规则

- 每完成一个阶段，在本文档对应阶段打勾并追加“实际修改文件 / 验证命令 / 剩余命中数”。
- 如果阶段引入新 helper，优先补单元测试，再迁移调用点。
- 对 Lua C API 兼容面，不强行删除错误码；必须新增现代核心接口，并让旧 API 成为薄包装。
- 对 GC 托管对象，不把 `Value` 或容器里所有观察关系改成 `shared_ptr`；生命周期由 GC 统一管理，调用面只保留 non-owning 指针。
- 每次清理一类模式后，更新 `tools/check_c_style_patterns.ps1` allowlist，防止同类模式回流。
