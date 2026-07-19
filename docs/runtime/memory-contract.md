---
status: current
verified_against: src/runtime/lua_allocator.hpp; src/gc/garbage_collector.hpp; src/gc/garbage_collector.cpp; src/gc/gc_mark.cpp; src/gc/gc_finalize.cpp; src/core/gc_string.hpp; src/core/string_pool.hpp; src/core/table.cpp; src/core/function.cpp; src/vm/vm_table.cpp; src/api/lapi.cpp; src/compiler/lexer/lexer.hpp; src/compiler/lexer/lexer.cpp; src/compiler/lexer/lexer_cursor.hpp; src/compiler/lexer/lexer_cursor.cpp; src/compiler/parser/token.hpp; src/compiler/parser/parser_utils.hpp; src/compiler/parser/parser.cpp; src/lib/testlib.cpp; src/runtime/native_module_registry.hpp; tests/unit/api/test_lua_c_api.cpp; tests/unit/compiler/test_parser_boundaries.cpp; tests/unit/gc/test_gc.cpp
last_checked: 2026-07-16
applies_to: GC managed-size accounting、lua_Alloc callback 与宿主内存上限声明
---

# 运行时内存合同

当前实现明确区分三种数字，任何文档和宿主集成都不得把它们互换：

| 机制 | 负责什么 | 是否是 hard limit |
|---|---|---|
| GC debt / automatic threshold | 决定何时推进自动 GC；依据 GC object 自报的 managed size | 否 |
| TestC managed-memory budget | `T.totalmem` 使用的兼容测试故障注入预算；由 `getManagedMemoryBudgetBytes()` 暴露 | 否 |
| host `lua_Alloc` live bytes | 宿主 callback 对实际收到的 allocate/reallocate/free 请求计账并拒绝超限请求 | 只有覆盖全部目标分配且满足下述事务条件时才可称 hard limit；当前项目尚未达到 |

`GarbageCollector::getTotalMemory()` 是所有 `GCObject::getSize()` 的求和，`getAccountedMemory()` 是同一口径的 O(1) ledger。两者用于 GC pacing、诊断和 TestC，不是 allocator 的精确 live-byte 数。

## 当前支持边界

自定义 `lua_Alloc` 已覆盖 State/Context、许多 GC object、userdata payload、Stack/CallInfo、collector-owned 工作列表，以及下面这个已经闭环的核心切片：

- `GCString` 对象走 callback；最多 23 字节的内容与终止符内联在对象中，更长的不可变载荷使用精确 `length + 1` callback 块，析构回传同一 `osize`，不再依赖标准库 SSO/capacity；StringPool 节点走 callback，key 直接引用不可变 `GCString` 内容，不再保存第二份默认 allocator 字符串；
- Table 数组和 Proto 的 constants/code/subProto/lineInfo/LocVar/upvalue-name 数组使用 `LuaReallocVector`，扩容发出真正的 `(ptr, osize, nsize)` realloc 请求；
- Table 数组单值和 `SETLIST` 范围写入在 realloc 失败时不改变逻辑大小与旧值；Table hash 插入使用强保证的 `try_emplace`，raw C API 与 VM `SETTABLE` 都逐点证明失败时不发布目标 entry；
- `lua_load` 用 `LuaStdAllocator<char>` 支撑的 source buffer 聚合 reader 分片，缓冲增长 OOM 会恢复调用者栈并发布 fixed memory error；
- services-backed Parser 把同一 callback 的值快照传给 Lexer；Lexer 的整源副本与词素缓冲、Token 的 lexeme/decoded-value/error 字符串使用按值快照 callback、23 字节内联且外部容量精确记账的 `LuaBasicString` / `LuaOwnedString`，`InputCursor` 源码重放缓存使用 `LuaVector<i32>`；这些缓存的 OOM 与 reader source buffer OOM 进入相同的 protected `LUA_ERRMEM` 回滚边界；
- State value stack 与 CallInfo 数组使用同一 callback 的 `LuaVector`；`lua_checkstack` 分配失败不改变逻辑栈，protected CallInfo 扩容失败恢复原调用深度并发布 fixed `LUA_ERRMEM`；
- VM 调用非函数值的 `__call` 元方法时，插入 callable self 前的宽实参暂存使用 State callback 支撑的 `LuaVector<Value>`，分配失败发生在参数搬移前；
- VM `OP_CONCAT` 直接借用字符串操作数、在栈上格式化数字，并用直接维护 callback 容量与精确 deallocation 字节数的 `LuaReallocVector<char>` 建立最终拼接缓冲，再以 view 交给 allocator-backed StringPool；
- 标准库 `table.concat` 同样借用字符串元素和分隔符、在栈上格式化数字元素，并用 State callback 支撑的 `LuaVector<char>` 渐进建立最终结果；
- 标准库 `table.sort` 的排序工作副本与每次 comparator 调用的完整栈快照使用 State callback 支撑的 `LuaVector<Value>`；工作副本成功排序后才写回目标表，比较器内存异常在恢复调用栈后保持 `LUA_ERRMEM` 分类；
- Proto 常量数组与去重 map 的多步插入在 map 分配失败时回滚常量槽；
- `roots_`、gray queue、weak-table queue、pending-finalizer queue 和 cross-collector `externalMarked_` 都使用同一个 `lua_Alloc`；完整收集的 finalizer drain copy 复用该 allocator；
- 标记开始前先 reserve gray/weak 容量，单对象标记先发布 queue entry 再改颜色，终结器先入队再发布 `FINALIZED`，drain copy 成功后才设置 reentrancy guard，因此失败后可安全重试；
- protected C API 会把 `MemoryError` / `std::bad_alloc` 转成 `LUA_ERRMEM`，并使用预先固定的错误对象避免 OOM 路径二次分配。

`tests/unit/api/test_lua_c_api.cpp` 的 host ledger 记录 `liveBytes`、`peakBytes`、`hardLimit` 与每块 `osize`。定向测试逐点扫描长字符串、Proto 常量插入、raw/VM Table hash 插入、State stack/CallInfo、宽 `__call` 实参暂存、长 VM `OP_CONCAT`、标准库 `table.concat`/`table.sort`、分片 reader source buffer、services-backed Lexer 整源/长词素/Token/`InputCursor` 缓存和一次同时经过 gray/weak/pending-finalizer/drain-copy 的完整收集，直接拒绝相应缓冲/Table/Proto/GC worklist 增长；`tests/unit/gc/test_gc.cpp` 另覆盖 root 与 cross-collector queue。`loadbuffer` 的长标识符矩阵另在零余量 hard limit 下覆盖 loader/compiler 缓存增长；State stack/CallInfo 门禁同时证明逻辑栈或调用深度不部分提交、解除限制可重试且随后仍可调用；宽 `__call` 门禁覆盖实参缓冲与 callable-self 插入所需栈扩容的全部实测 offset，并证明重试后参数完整；两条长 concat 门禁分别覆盖 VM 和 table library 的结果扩容、GCString 与池增长全部实测 offset，并证明重试内容完整；`table.sort` 门禁覆盖工作副本与全部实测 comparator 快照 offset，并证明失败不写回部分排序结果；`tests/unit/compiler/test_parser_boundaries.cpp` 证明原 `LuaAllocator` 对象销毁后 Token 快照仍可读，Token 销毁后 callback live bytes 归零。测试证明已覆盖切片满足 `liveBytes <= hardLimit`、旧块保留、逻辑内容不部分提交、终结器不丢失也不重复、解除限制后可继续使用，以及 `lua_close` 后 `liveBytes == 0`。该组测试已在 Windows MSVC Debug/Release strict 运行，在线矩阵待本提交验证；此前字符串/Table/Proto 切片另有 MSVC ASan 证据。它是切片证据，不是全运行时 hard-limit 声明。

但以下容量仍可能绕过 callback，或尚无完整 fail-on-N/事务性证明：

- reader callback 自身的 host-owned storage，以及 Token 借用 view 被复制到 parser/AST/codegen 后的临时 `Str` / `Vec`、智能指针控制块和诊断对象；
- standard library 的其余路径、debug/trace、I/O、package 等执行路径中的临时 `Str` / `Vec` / `std::string`；
- MSVC Debug 标准容器的 `_Container_proxy` 实现元数据（当前有意放在 callback 之外）；
- `NativeModuleRegistry` 的路径/cache 元数据与操作系统 loader 自身分配。

因此当前支持状态是：`allocator-backed hard limit = unsupported`。宿主可以用 callback 限制已经路由到 `lua_Alloc` 的字节，但不能据此宣称限制了 Lua 执行期间的全部进程内存或单次编译峰值。模块 registry 与 OS loader 明确属于宿主/进程管理开销；若目标是进程级硬上限，还必须使用进程、job/cgroup 或等价的外部资源治理。

## allocator callback 约束

- callback 不得抛出异常；实现会在边界兜底，但抛出会让所有权语义无法可靠证明。
- `nsize == 0` 表示释放，返回值应为 `nullptr`。
- realloc 失败必须返回 `nullptr` 并保持旧块有效；成功后才转移旧块所有权。
- `osize` 必须按宿主 ledger 校验，不能据不可信大小越界访问。
- `lua_setallocf` 替换 allocator 时，新 callback 必须能释放此前 callback 创建的仍存活块；当前实现以共享 ledger 测试这种兼容路径。
- allocator 的计账结构本身不得依赖受同一 hard limit 控制、且可能在失败路径继续分配的动态容器。

## hard-limit 完成标准

在状态改为 supported 前，每个实际分配 offset 都必须进行 fail-on-N 扫描，至少覆盖长字符串、raw/VM table growth、`SETTABLE`、`SETLIST`、userdata、thread 与栈/CallInfo、Proto metadata、GC 工作列表和分片 `lua_load`。每个失败点都必须同时证明：

1. 返回 `LUA_ERRMEM`，没有 C++ 异常越过 protected C API；
2. allocator `liveBytes <= limit`，失败 realloc 后旧块仍有效；
3. 栈形状和目标容器逻辑内容不发生部分提交；
4. 解除限制后 State 可继续使用；
5. `lua_close` 后 allocator `liveBytes == 0`，且无 old-size mismatch 或重复释放；
6. Windows/Linux 的 Debug/Release、ASan 和 UBSan 均通过同一矩阵。

在这些条件闭环前，GC managed-size budget 只保留其 TestC/诊断名称，不再以“内存硬限制”描述。
