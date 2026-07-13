---
status: current
verified_against: src/lua.h; src/lauxlib.h; src/lualib.h; src/api/lapi.cpp; src/runtime/lua_allocator.hpp; src/runtime/runtime_services.hpp; src/common/lua_error.hpp; src/vm/state/global_state.cpp; src/vm/state/lua_state.cpp; src/gc/garbage_collector.hpp; src/gc/garbage_collector.cpp; src/core/userdata.cpp; src/lib/testlib.cpp; tests/unit/api/test_lua_c_api.cpp; tests/lua/official/api.lua; tests/lua/official/code.lua
last_checked: 2026-07-13
applies_to: Lua 5.1 C API 原型、项目内直接测试与官方 testC 覆盖边界
---

# Lua 5.1 C API 覆盖矩阵

本表区分“已有入口”“项目内直接测试”和“官方 `api.lua` / `ltests.c` 验证”。只有后两列都有证据时，才可将能力视为官方兼容闭环。

| 能力组 | 当前实现 | 项目内直接测试 | 官方测试覆盖 | 状态 |
|---|---|---|---|---|
| State 生命周期 | `lua_newstate`、`lua_open`、`lua_close`；每个公开主 State 拥有独立 `EngineContext`，自定义 allocator 分配 State/Context | 双 State 隔离；创建中途失败回滚；关闭释放普通与 fixed roots | `api.lua` 已打开 `T`，当前在更早的栈形状断言 XFAIL | 多 State 与核心生命周期已直接测试 |
| 栈顶与容量 | `lua_gettop`、`lua_settop`、`lua_checkstack` | 正/负 `settop`、扩栈 nil 填充、主线程虚拟槽隔离、最大容量拒绝 | 未覆盖 | 已实现、已直接测试 |
| 普通索引 | 正索引、负索引、invalid index、`lua_pushvalue` | 正/负/越界读取与复制 | 未覆盖 | 已实现、已直接测试 |
| 栈重排 | `lua_insert`、`lua_remove`、`lua_replace` | 正/负位置、C 回调帧内 remove | 未覆盖 | 已实现、已直接测试 |
| pseudo-index | `LUA_REGISTRYINDEX`、`LUA_GLOBALSINDEX`、`LUA_ENVIRONINDEX` | registry/global table 读写；environment 仅实现 | 未覆盖 | registry/global 已测试；environment 待补 |
| C closure upvalue | `lua_upvalueindex(n)`、`lua_pushcclosure(fn,n)`、pseudo-index replace | 捕获顺序、消费栈值、回调读取、持久修改 | 未覆盖 | 已实现、已直接测试 |
| 类型与转换 | `lua_type`、number/string/boolean 转换及部分 `is*` | 由栈、registry 与 upvalue 用例间接覆盖 | 未覆盖 | 部分实现 |
| 表与全局 | `createtable`、`gettable/settable`、`rawgeti/rawseti`、global get/set | registry raw access、globals pseudo-index | 未覆盖 | 部分实现 |
| 调用、错误与 yield | `lua_call`、`lua_pcall`、`lua_error`、`lua_newthread`、`lua_resume`、`lua_yield`、`lua_status` | 原始非字符串 error object、栈前缀恢复、正/负 handler 索引、C/Lua handler、`LUA_ERRERR`/`LUA_ERRMEM`、C→Lua→C、公开 thread 正常/多返回/yield/带参恢复/错误/dead resume | 官方 `errors.lua` tail 通过；`api.lua` 已打开 `T`，但尚未执行到该段 | Lua closure 入口的公开 resume 已直接测试；C function 入口待补 |
| load/dump | `lua_load`、`lua_dump`、`luaL_loadbuffer`、`luaL_loadstring`、`luaL_loadfile` | reader 分片、源码/文件编译、语法/文件状态、项目本地 binary chunk dump/load 往返、宿主栈保持 | `api.lua` 已打开 `T`，当前在更早的栈形状断言 XFAIL | 项目本地 chunk 闭环已实现；不宣称官方 `luac` 字节兼容 |
| closure introspection | `lua_getupvalue`、`lua_setupvalue` | C closure 空名称、Lua closure debug name、读写栈效应和持久修改 | 未覆盖 | 已实现、已直接测试 |
| userdata | `lua_pushlightuserdata`、`lua_newuserdata`、`lua_touserdata`、`lua_objlen`、metatable、`__gc` | light/full/零长度 payload、8 字节对齐、metatable 往返/移除、字符串/表长度、终结器一次执行与 payload 可见性 | 未覆盖 | 已实现、已直接测试 |
| registry refs | `luaL_ref/unref`、`luaL_getref` | nil reference、存取、释放后复用 | `api.lua` 已打开 `T`，尚未执行到 refs 段 | 已实现、已直接测试 |
| allocator | `lua_newstate` 保存/调用 callback；`lua_getallocf/setallocf`；主/协程 State、Context、GC object、userdata payload，以及 Stack/CallInfo、GC 工作列表、StringPool 索引、Table、Proto/Function 容量；当前 allocator 负责关闭释放 | callback/userdata 往返、allocator 替换、初始化分配失败、协程创建每一实际分配点、protected-call/GC object/payload 运行期失败、old-size/无泄漏/无重复释放 | 官方低内存 `T.totalmem` 路径未启用 | 主要运行时对象与容器已接入；真实 realloc、GCString/StringPool key 字符串内容容量仍待闭环 |
| State/协程组移动 | `lua_xmove` | 同组双向顺序保持；拒绝独立 `EngineContext` State | 未覆盖 | 移动语义与独立所有权已测试 |
| 官方 helper | 项目版 `T.testC`、`T.listcode` 位于 `src/lib/testlib.cpp` | 独立 `official-testc` 通道实际打开 `T` | `code.lua`、`api.lua` 已执行并分别在首个差异处 XFAIL | 路径已恢复，契约仍需逐项补齐 |

## 当前证据

2026-07-10 的直接门禁：

```powershell
bin\lua_test.exe --filter "Lua C API"
```

结果为 24 个测试、244 个断言、0 failures。完整测试为 694 个 registered tests、3650 个 assertion results、0 failures。

## 下一批失败驱动任务

1. 补齐 GCString/StringPool key 字符串内容等剩余动态容量，并验证真实 realloc/官方低内存路径。
2. 补齐 function environment 与剩余 table/type 边界。
3. 补齐 C function coroutine 入口及更深的嵌套 resume 边界。
4. 按 `api.lua` 的第一个真实失败逐项扩展 `T.testC`，每个修复沉淀为独立回归用例。
