---
status: current
verified_against: src/lua.h; src/lauxlib.h; src/lualib.h; src/api/lapi.cpp; src/runtime/lua_allocator.hpp; src/runtime/runtime_services.hpp; src/runtime/native_module_registry.hpp; src/common/lua_error.hpp; src/vm/state/global_state.cpp; src/vm/state/lua_state.cpp; src/gc/garbage_collector.hpp; src/gc/garbage_collector.cpp; src/core/userdata.cpp; src/lib/testlib.cpp; tests/compatibility/lua51-public-api-contract.json; tests/compatibility/public_api_c_compile.c; tests/compatibility/public_api_cpp_consumer.cpp; tests/compatibility/public_native_module.c; tests/unit/api/test_lua_c_api.cpp; tools/check_lua51_public_api_contract.py; tests/lua/official/api.lua; tests/lua/official/code.lua
last_checked: 2026-07-15
applies_to: Lua 5.1 C API 原型、项目内直接测试与官方 testC 覆盖边界
---

# Lua 5.1 C API 覆盖矩阵

本表区分“已有公开入口”“项目内直接测试”和“原始官方 `api.lua` 经项目 `T` helper 验证”。机器合同 `tests/compatibility/lua51-public-api-contract.json` 以官方 5.1.5 头文件为全集，要求每个符号处于 `PASS / XFAIL / UNSUPPORTED` 三态之一，并为 PASS 记录 C compile、link 和直接公开调用证据。项目版 `T` 位于 C++ 测试库中，因此 TestC helper 不能替代同名公开 API 符号。

| 能力组 | 当前实现 | 项目内直接测试 | 官方测试覆盖 | 状态 |
|---|---|---|---|---|
| State 生命周期 | `lua_newstate`、`lua_open`、`lua_close`；每个公开主 State 拥有独立 `EngineContext`，自定义 allocator 分配 State/Context；`lua_close` 为 `noexcept` 并将 coroutine 归一到 main State | 双 State 隔离；创建中途失败回滚；close-time `__gc`、单个 finalizer 错误隔离、coroutine 关闭整个 Runtime、持续 OOM 关闭归零 | 原始 `api.lua` 通过 state/thread 创建、remote state 和低内存循环 | 已实现并形成直接 + TestC 证据 |
| 栈顶与容量 | `lua_gettop`、`lua_settop`、`lua_checkstack` | 正/负 `settop`、扩栈 nil 填充、主线程虚拟槽隔离、最大容量拒绝 | 原始 `api.lua` 的 `T.testC` 栈协议 exact PASS | 已实现并形成直接 + TestC 证据 |
| 普通索引 | 正索引、负索引、invalid index、`lua_pushvalue` | 正/负/越界读取与复制 | 原始 `api.lua` 覆盖普通索引和栈顶相对参数 | 已实现并形成直接 + TestC 证据 |
| 栈重排 | `lua_insert`、`lua_remove`、`lua_replace` | 正/负位置、C 回调帧内 remove | 原始 `api.lua` 覆盖 `insert/remove/replace` 命令 | 已实现并形成直接 + TestC 证据 |
| pseudo-index 与环境 | `LUA_REGISTRYINDEX`、`LUA_GLOBALSINDEX`、`LUA_ENVIRONINDEX`、upvalue pseudo-index；userdata 保存并标记 environment | registry/global table 读写、closure upvalue；userdata environment 由库路径验证 | 原始 `api.lua` 的 `G/E/R/U<n>` 命令和 userdata environment 路径通过 | 当前已用路径通过；公开 `lua_getfenv/lua_setfenv` 尚未声明 |
| C closure upvalue | `lua_upvalueindex(n)`、`lua_pushcclosure(fn,n)`、pseudo-index replace | 捕获顺序、消费栈值、回调读取、持久修改 | 原始 `api.lua` 的 closure/upvalue 命令通过 | 已实现并形成直接 + TestC 证据 |
| 类型与转换 | `lua_type`、`lua_typename`、number/string/boolean/userdata 转换及部分 `is*` | 栈、registry、userdata 与 upvalue 用例覆盖 | 原始 `api.lua` 的 `is*`、`tobool`、`tonumber`、`tostring`、`objsize` 命令通过 | 当前公开子集已验证；未声明符号不算支持 |
| 表与全局 | `createtable`、`gettable/settable`、`rawgeti/rawseti`、global get/set | registry raw access、globals pseudo-index、userdata metatable | 原始 `api.lua` 的 table/global/raw/next 语义通过项目 `T` helper | 当前公开子集已验证；`lua_next` 等 helper 命令不代表同名公开符号已提供 |
| 调用、错误与 yield | `lua_call`、`lua_pcall`、`lua_error`、`lua_newthread`、`lua_resume`、`lua_yield`、`lua_status`；protected status API 在 C++ 头中为 `noexcept`，unprotected long-jump 风格入口保持可抛 | 原始非字符串 error object、栈前缀恢复、正/负 handler 索引、C/Lua handler、`LUA_ERRERR`/`LUA_ERRMEM`、C→Lua→C、Lua/C function coroutine、reader/writer/C callback 的标准与非标准异常、持久 allocator failure、死协程 traceback | 官方 `errors.lua` tail、`db.lua` 和原始 `api.lua` exact PASS | `lua_pcall`、`lua_resume`、`lua_newthread`、`lua_checkstack` 已形成关闭异常边界；Lua error 保留 error object/traceback，宿主异常使用固定 emergency error 与强回滚 |
| load/dump | `lua_load`、`lua_dump`、`luaL_loadbuffer`、`luaL_loadstring`、`luaL_loadfile` 均为关闭异常边界 | reader 分片、源码/文件编译、语法/文件状态、binary chunk 往返、reader/writer 的 `std::exception`、非标准异常与 `bad_alloc`、满栈和持久 OOM | 原始 `api.lua` 的 loadstring/loadfile/dump/undump/低内存路径通过 | 项目本地 chunk 闭环已实现；不宣称官方 `luac` 字节兼容 |
| closure introspection | `lua_getupvalue`、`lua_setupvalue` | C closure 空名称、Lua closure debug name、读写栈效应和持久修改 | 原始 `api.lua` 的 `T.upvalue` 路径通过 | 已实现并形成直接 + TestC 证据 |
| userdata | `lua_pushlightuserdata`、`lua_newuserdata`、`lua_touserdata`、`lua_objlen`、metatable、environment、`__gc` | light/full/零长度 payload、8 字节对齐、metatable 往返/移除、字符串/表长度、普通 GC 与 close-time 终结器一次执行、payload 可见性及错误隔离 | 原始 `api.lua` 的 userdata 值、environment、GC 和低内存路径通过 | 已实现当前公开子集；finalizer 重入与 Runtime 关闭另有回归测试 |
| registry refs | `luaL_ref/unref`、`luaL_getref` | nil reference、存取、释放后复用 | 原始 `api.lua` 的 `ref/getref/unref` 路径通过 | 已实现并形成直接 + TestC 证据 |
| allocator 与内存故障 | `lua_newstate` 保存/调用 callback；`lua_getallocf/setallocf`；主/协程 State、Context、GC object、userdata payload，以及 Stack/CallInfo、GC 工作列表、StringPool 索引、Table、Proto/Function 容量；当前 allocator 负责关闭释放 | callback/userdata 往返、allocator 替换、初始化分配失败、协程创建每一实际分配点、protected-call/GC object/payload 运行期失败、长字符串内容、Table/SETLIST 与 Proto 真实 realloc、fail-on-N、old-size/关闭归零 | 原始 `api.lua` 的 `T.totalmem` 低内存循环 exact PASS | 长字符串与核心 Table/Proto 切片已形成 allocator hard-limit/事务证据；parser/lexer/AST/codegen、stdlib/I/O/package/debug 临时容器与 GC worklist 全矩阵仍不支持全运行时 hard-limit 声明，详见内存合同 |
| State/协程组移动 | `lua_xmove` | 同组双向顺序保持；拒绝独立 `EngineContext` State | `api.lua` 覆盖 remote/new-stack 语义，但不作为公开 `lua_xmove` 的替代证据 | 移动语义与独立所有权已直接测试 |
| 原生 C 模块 | `package.loadlib` / C searcher 使用 context-owned `NativeModuleRegistry` | 纯 C `lua.h` fixture 由 `lua_app`、公开 API embedding host 和双-context host 动态加载；open/init 错误、per-state 状态、lease cache、关闭与重载；模块内 `__gc` 写入外部标记证明 finalizer 先于卸载 | 官方模块源码不作为本项目二进制 ABI 替代证据 | Windows CMake 已验证 host import table；Linux/Windows CI 均运行 `api-contract` / `native-module` CTest |
| 官方 helper | 项目版 `T.testC`、`T.listcode` 及 state/userdata/ref/memory helpers 位于 `src/lib/testlib.cpp` | 独立 `official-testc` 通道实际打开 `T`，不再自跳过 | 原始 `api.lua` exact PASS；`code.lua` 的 5.1.5 oracle 来源已锁定，校正通用 5.1 fixture 后仍有一个编译器 parity XFAIL | API TestC 契约闭环；剩余 code XFAIL 已与 fixture 版本差异分离 |

## 当前证据

2026-07-15 的直接门禁：

```powershell
bin\lua_test.exe --filter "Lua C API"
```

结果为 40 个测试、953 个断言、0 failures。机器合同另包含 123 个官方公共函数：60 个 `PASS`、0 个 `XFAIL`、63 个显式 `UNSUPPORTED`。原始 `api.lua` 另以以下 exact TestC 门禁通过：

```powershell
bin\lua_test.exe --filter "api.lua with T module"
```

该次结果为 1 个选中测试、1 个断言、0 failures，并完整执行到官方脚本的 `OK`。完整套件总数由测试运行器实时报告；本页不冻结一个可能随新增回归变化的全局计数。

独立模块证据由以下 CTest 标签持续执行：

```powershell
ctest --test-dir build -C Debug -L api-contract --output-on-failure
ctest --test-dir build -C Debug -L native-module --output-on-failure
```

## 下一批失败驱动任务

1. 将已经闭环的长字符串、Table/SETLIST 与 Proto allocator 切片扩展到 parser/lexer/AST/codegen 临时容器、标准库临时对象和 GC worklist 全矩阵；在此之前保持全运行时 hard limit 为 `UNSUPPORTED`，由 [#5](https://github.com/YanqingXu/lua/issues/5) 跟踪。
2. 明确并逐项补齐尚未声明的 Lua 5.1 C API 符号（例如公开 environment、遍历和比较入口）；不能用内部 TestC 命令替代公开 API 证据。
3. 扩展 coroutine 的嵌套 resume 与 Lua 5.1 不提供的 C yield continuation 边界，同时保持死协程 traceback 与关闭异常边界回归。
4. 保持原始 `api.lua` exact 门禁；`code.lua` 保持 upstream 文件字节不变，运行时只应用清单登记的 5.1.5 oracle 校正，剩余 repeat-condition 编译器 parity gap 继续按精确诊断收敛。
