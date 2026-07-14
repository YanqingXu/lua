---
status: current
verified_against: src/lua.h; src/lauxlib.h; src/lualib.h; src/api/lapi.cpp; src/runtime/lua_allocator.hpp; src/runtime/runtime_services.hpp; src/common/lua_error.hpp; src/vm/state/global_state.cpp; src/vm/state/lua_state.cpp; src/gc/garbage_collector.hpp; src/gc/garbage_collector.cpp; src/core/userdata.cpp; src/lib/testlib.cpp; tests/unit/api/test_lua_c_api.cpp; tests/lua/official/api.lua; tests/lua/official/code.lua
last_checked: 2026-07-14
applies_to: Lua 5.1 C API 原型、项目内直接测试与官方 testC 覆盖边界
---

# Lua 5.1 C API 覆盖矩阵

本表区分“已有公开入口”“项目内直接测试”和“原始官方 `api.lua` 经项目 `T` helper 验证”。`api.lua` 的 exact 运行已经通过，但项目版 `T` 位于 C++ 测试库中；这份证据验证 TestC 命令及其可观察语义，不等同于已经提供官方 `ltests.c` 的二进制 ABI，也不把尚未声明的 Lua 5.1 C API 符号算作已实现。

| 能力组 | 当前实现 | 项目内直接测试 | 官方测试覆盖 | 状态 |
|---|---|---|---|---|
| State 生命周期 | `lua_newstate`、`lua_open`、`lua_close`；每个公开主 State 拥有独立 `EngineContext`，自定义 allocator 分配 State/Context | 双 State 隔离；创建中途失败回滚；关闭释放普通与 fixed roots | 原始 `api.lua` 通过 state/thread 创建、remote state 和低内存循环 | 已实现并形成直接 + TestC 证据 |
| 栈顶与容量 | `lua_gettop`、`lua_settop`、`lua_checkstack` | 正/负 `settop`、扩栈 nil 填充、主线程虚拟槽隔离、最大容量拒绝 | 原始 `api.lua` 的 `T.testC` 栈协议 exact PASS | 已实现并形成直接 + TestC 证据 |
| 普通索引 | 正索引、负索引、invalid index、`lua_pushvalue` | 正/负/越界读取与复制 | 原始 `api.lua` 覆盖普通索引和栈顶相对参数 | 已实现并形成直接 + TestC 证据 |
| 栈重排 | `lua_insert`、`lua_remove`、`lua_replace` | 正/负位置、C 回调帧内 remove | 原始 `api.lua` 覆盖 `insert/remove/replace` 命令 | 已实现并形成直接 + TestC 证据 |
| pseudo-index 与环境 | `LUA_REGISTRYINDEX`、`LUA_GLOBALSINDEX`、`LUA_ENVIRONINDEX`、upvalue pseudo-index；userdata 保存并标记 environment | registry/global table 读写、closure upvalue；userdata environment 由库路径验证 | 原始 `api.lua` 的 `G/E/R/U<n>` 命令和 userdata environment 路径通过 | 当前已用路径通过；公开 `lua_getfenv/lua_setfenv` 尚未声明 |
| C closure upvalue | `lua_upvalueindex(n)`、`lua_pushcclosure(fn,n)`、pseudo-index replace | 捕获顺序、消费栈值、回调读取、持久修改 | 原始 `api.lua` 的 closure/upvalue 命令通过 | 已实现并形成直接 + TestC 证据 |
| 类型与转换 | `lua_type`、`lua_typename`、number/string/boolean/userdata 转换及部分 `is*` | 栈、registry、userdata 与 upvalue 用例覆盖 | 原始 `api.lua` 的 `is*`、`tobool`、`tonumber`、`tostring`、`objsize` 命令通过 | 当前公开子集已验证；未声明符号不算支持 |
| 表与全局 | `createtable`、`gettable/settable`、`rawgeti/rawseti`、global get/set | registry raw access、globals pseudo-index、userdata metatable | 原始 `api.lua` 的 table/global/raw/next 语义通过项目 `T` helper | 当前公开子集已验证；`lua_next` 等 helper 命令不代表同名公开符号已提供 |
| 调用、错误与 yield | `lua_call`、`lua_pcall`、`lua_error`、`lua_newthread`、`lua_resume`、`lua_yield`、`lua_status`；C++ 头显式声明 C-linkage API 为 `noexcept(false)` | 原始非字符串 error object、栈前缀恢复、正/负 handler 索引、C/Lua handler、`LUA_ERRERR`/`LUA_ERRMEM`、C→Lua→C、Lua/C function coroutine 入口、yield/带参恢复/错误/dead resume、MSVC Release 异常展开和回滚后 GC 根释放 | 官方 `errors.lua` tail 与原始 `api.lua` exact PASS | 公开 coroutine 入口与主要 protected-call 不变量已直接测试；状态型 API 的全异常封闭由 [#8](https://github.com/YanqingXu/lua/issues/8) 跟踪 |
| load/dump | `lua_load`、`lua_dump`、`luaL_loadbuffer`、`luaL_loadstring`、`luaL_loadfile` | reader 分片、源码/文件编译、语法/文件状态、现有目录错误、项目本地 binary chunk dump/load 往返、宿主栈保持 | 原始 `api.lua` 的 loadstring/loadfile/dump/undump/低内存路径通过 | 项目本地 chunk 闭环已实现；不宣称官方 `luac` 字节兼容 |
| closure introspection | `lua_getupvalue`、`lua_setupvalue` | C closure 空名称、Lua closure debug name、读写栈效应和持久修改 | 原始 `api.lua` 的 `T.upvalue` 路径通过 | 已实现并形成直接 + TestC 证据 |
| userdata | `lua_pushlightuserdata`、`lua_newuserdata`、`lua_touserdata`、`lua_objlen`、metatable、environment、`__gc` | light/full/零长度 payload、8 字节对齐、metatable 往返/移除、字符串/表长度、终结器一次执行与 payload 可见性 | 原始 `api.lua` 的 userdata 值、environment、GC 和低内存路径通过 | 已实现当前公开子集；finalizer 重入另有回归测试 |
| registry refs | `luaL_ref/unref`、`luaL_getref` | nil reference、存取、释放后复用 | 原始 `api.lua` 的 `ref/getref/unref` 路径通过 | 已实现并形成直接 + TestC 证据 |
| allocator 与内存故障 | `lua_newstate` 保存/调用 callback；`lua_getallocf/setallocf`；主/协程 State、Context、GC object、userdata payload，以及 Stack/CallInfo、GC 工作列表、StringPool 索引、Table、Proto/Function 容量；当前 allocator 负责关闭释放 | callback/userdata 往返、allocator 替换、初始化分配失败、协程创建每一实际分配点、protected-call/GC object/payload 运行期失败、old-size/无泄漏/无重复释放 | 原始 `api.lua` 的 `T.totalmem` 低内存循环 exact PASS | 主要对象和容器已接入；`T.totalmem` 是运行时预算故障注入，不单独证明每个动态字节均经 `lua_Alloc`；真实 realloc 与字符串内容容量仍待闭环 |
| State/协程组移动 | `lua_xmove` | 同组双向顺序保持；拒绝独立 `EngineContext` State | `api.lua` 覆盖 remote/new-stack 语义，但不作为公开 `lua_xmove` 的替代证据 | 移动语义与独立所有权已直接测试 |
| 官方 helper | 项目版 `T.testC`、`T.listcode` 及 state/userdata/ref/memory helpers 位于 `src/lib/testlib.cpp` | 独立 `official-testc` 通道实际打开 `T`，不再自跳过 | 原始 `api.lua` exact PASS；`code.lua` 是唯一 TestC XFAIL | API TestC 契约闭环；剩余 code XFAIL 是字节码期望版本差异 |

## 当前证据

2026-07-14 的直接门禁：

```powershell
bin\lua_test.exe --filter "Lua C API"
```

结果为 33 个测试、805 个断言、0 failures。原始 `api.lua` 另以以下 exact TestC 门禁通过：

```powershell
bin\lua_test.exe --filter "api.lua with T module"
```

该次结果为 1 个选中测试、1 个断言、0 failures，并完整执行到官方脚本的 `OK`。完整套件总数由测试运行器实时报告；本页不冻结一个可能随新增回归变化的全局计数。

## 下一批失败驱动任务

1. 补齐 GCString/StringPool key 字符串内容等剩余动态容量，并对真实 realloc 建立 allocator 证据；硬限额与软 GC 预算的语义闭环由 [#5](https://github.com/YanqingXu/lua/issues/5) 跟踪。
2. 明确并逐项补齐尚未声明的 Lua 5.1 C API 符号（例如公开 environment、遍历和比较入口）；不能用内部 TestC 命令替代公开 API 证据。
3. 扩展 coroutine 的嵌套 resume、C yield continuation 与错误边界；C function 首次入口已经进入直接回归，状态型 C API 的完整异常封闭由 [#8](https://github.com/YanqingXu/lua/issues/8) 跟踪。
4. 保持原始 `api.lua` exact 门禁；对 `code.lua` 唯一 XFAIL 先确认测试源与 Lua 5.1.5 字节码 oracle 的版本关系，再决定修复 fixture 还是实现。
