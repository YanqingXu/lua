---
status: current
verified_against: src/lua.h; src/lauxlib.h; src/lualib.h; src/api/lapi.cpp; src/lib/testlib.cpp; tests/unit/api/test_lua_c_api.cpp; tests/lua/official/api.lua; tests/lua/official/code.lua
last_checked: 2026-07-10
applies_to: Lua 5.1 C API 原型、项目内直接测试与官方 testC 覆盖边界
---

# Lua 5.1 C API 覆盖矩阵

本表区分“已有入口”“项目内直接测试”和“官方 `api.lua` / `ltests.c` 验证”。只有后两列都有证据时，才可将能力视为官方兼容闭环。

| 能力组 | 当前实现 | 项目内直接测试 | 官方测试覆盖 | 状态 |
|---|---|---|---|---|
| State 生命周期 | `lua_newstate`、`lua_open`、`lua_close` | `Lua C API` suite 创建/关闭 State | `api.lua` 未启用真实 `T.testC` 路径 | 部分实现；allocator 尚未接入 |
| 栈顶 | `lua_gettop`、`lua_settop` | 正/负 `settop`、扩栈 nil 填充、主线程虚拟槽隔离 | 未覆盖 | 已实现、已直接测试 |
| 普通索引 | 正索引、负索引、invalid index、`lua_pushvalue` | 正/负/越界读取与复制 | 未覆盖 | 已实现、已直接测试 |
| 栈重排 | `lua_insert`、`lua_remove`、`lua_replace` | 正/负位置、C 回调帧内 remove | 未覆盖 | 已实现、已直接测试 |
| pseudo-index | `LUA_REGISTRYINDEX`、`LUA_GLOBALSINDEX`、`LUA_ENVIRONINDEX` | registry/global table 读写；environment 仅实现 | 未覆盖 | registry/global 已测试；environment 待补 |
| C closure upvalue | `lua_upvalueindex(n)`、`lua_pushcclosure(fn,n)`、pseudo-index replace | 捕获顺序、消费栈值、回调读取、持久修改 | 未覆盖 | 已实现、已直接测试 |
| 类型与转换 | `lua_type`、number/string/boolean 转换及部分 `is*` | 由栈、registry 与 upvalue 用例间接覆盖 | 未覆盖 | 部分实现 |
| 表与全局 | `createtable`、`gettable/settable`、`rawgeti/rawseti`、global get/set | registry raw access、globals pseudo-index | 未覆盖 | 部分实现 |
| 调用与保护调用 | `lua_call`、`lua_pcall` | C closure 普通调用已覆盖；错误函数和栈恢复未独立覆盖 | 未覆盖 | 部分实现 |
| C closure introspection | `lua_getupvalue`、`lua_setupvalue` | 无入口 | 未覆盖 | 未实现 |
| userdata | light/full userdata、metatable、`__gc`、`lua_objlen` | 内部运行时有 userdata 测试，C API 无直接测试 | 未覆盖 | C API 未闭环 |
| registry refs | `luaL_ref/unref`、getref | 无入口 | 未覆盖 | 未实现 |
| allocator | allocator 保存/调用、`lua_getallocf/setallocf`、失败路径 | 无 | 未覆盖 | 未实现 |
| 多 State | 独立 `EngineContext`、`lua_xmove`、隔离与所有权 | 内部 `EngineContext` 有测试，C API 无闭环 | 未覆盖 | 未实现 |
| 官方 helper | 项目版 `T.testC`、`T.listcode` 原型位于 `src/lib/testlib.cpp` | 非完整 `ltests.c` 契约 | staged suite 仍输出 `testC not active` | 原型存在，未完成官方验证 |

## 当前证据

2026-07-10 的直接门禁：

```powershell
bin\lua_test.exe --filter "Lua C API"
```

结果为 5 个测试、30 个断言、0 failures。完整测试为 673 个 registered tests、3434 个 assertion results、0 failures。

## 下一批失败驱动任务

1. 实现 `lua_getupvalue` / `lua_setupvalue`，补齐 C closure introspection。
2. 建立 light/full userdata、userdata metatable、`__gc` 与 `lua_objlen` 的 C API 用例。
3. 完成 `lua_error`、error function、嵌套 C → Lua → C 和 `lua_pcall` 栈恢复测试。
4. 接入 allocator 与独立 `EngineContext` 所有权，增加双 State 隔离和分配失败测试。
5. 按 `api.lua` 的第一个真实失败逐项扩展 `T.testC`，每个修复沉淀为独立回归用例。
