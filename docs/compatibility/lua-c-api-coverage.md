---
status: current
verified_against: CMakeLists.txt; cmake/LuaCppConfig.cmake.in; .github/workflows/ci.yml; src/lua.h; src/lauxlib.h; src/lualib.h; src/lua_runtime.h; src/lua_cpp_version.h; src/api/lapi.cpp; src/api/lauxlib.cpp; src/lib/debuglib.cpp; src/lib/iolib.cpp; src/runtime/lua_allocator.hpp; src/runtime/runtime_services.hpp; src/runtime/execution_policy.hpp; src/runtime/native_module_registry.hpp; src/common/lua_error.hpp; src/compiler/ast.hpp; src/compiler/parser/parser_impl.hpp; src/vm/state/global_state.cpp; src/vm/state/lua_state.cpp; src/gc/garbage_collector.hpp; src/gc/garbage_collector.cpp; src/core/userdata.cpp; src/lib/testlib.cpp; tests/packaging/; tests/compatibility/lua51-public-api-contract.json; tests/compatibility/lua_public_api_exports.def; tests/compatibility/lua_public_api_exports.map; tests/compatibility/public_api_c_compile.c; tests/compatibility/public_api_cpp_consumer.cpp; tests/compatibility/lua51_c_api_differential_probe.c; tests/compatibility/public_native_module.c; tests/unit/api/test_lua_c_api.cpp; tests/unit/compiler/test_parser_boundaries.cpp; tools/check_lua51_public_api_contract.py; tools/run_lua51_c_api_differential.ps1; tests/lua/official/api.lua; tests/lua/official/code.lua
last_checked: 2026-07-26
applies_to: Lua 5.1 C API 原型、项目内直接测试与官方 testC 覆盖边界
---

# Lua 5.1 C API 覆盖矩阵

本表区分“已有公开入口”“项目内直接测试”和“原始官方 `api.lua` 经项目 `T` helper 验证”。机器合同 `tests/compatibility/lua51-public-api-contract.json` 以官方 5.1.5 头文件为全集，要求每个符号处于 `PASS / XFAIL / UNSUPPORTED` 三态之一，并为 PASS 记录 C compile、link 和直接公开调用证据。项目版 `T` 位于 C++ 测试库中，因此 TestC helper 不能替代同名公开 API 符号。

函数兼容状态与项目实际导出面是两个独立集合：官方函数合同当前为 123/123 个 `PASS`，而项目公开头文件声明 143 个真实函数。合同检查器会从头文件自动枚举这 143 个函数，并要求 C 链接探针、C++ 精确签名断言、Windows `.def` 和 Linux version script 与之完全一致；项目额外入口除兼容/安全扩展外，还包含 `lua_runtime.h` 的 11 个创建期配置、执行窗口、取消与 metrics 入口。相同消费者分别链接 `lua_core` 静态库和 `lua_public_api_shared` 动态库，避免静态链接掩盖导出缺口。

| 能力组 | 当前实现 | 项目内直接测试 | 官方测试覆盖 | 状态 |
|---|---|---|---|---|
| State 生命周期 | `lua_newstate`、`lua_open`、`lua_close`；每个公开主 State 拥有独立 `EngineContext` 并固定构造线程为 owner，自定义 allocator 分配 State/Context；`lua_close` 为 `noexcept` 并将 coroutine 归一到 main State | 双 State 隔离；创建中途失败回滚；close-time `__gc`、单个 finalizer 错误隔离、coroutine 关闭整个 Runtime、持续 OOM 关闭归零；foreign-thread C API/VM 拒绝且栈不变，`lua_close` 无操作后由 owner 完成释放 | 原始 `api.lua` 通过 state/thread 创建、remote state 和低内存循环 | 已实现并形成直接 + TestC 证据 |
| 生产运行时扩展 | `lua_runtime.h` 的版本化创建配置、unrestricted/game-server 初始化器、每请求 instruction/native-work/finalizer/deadline 窗口、生命周期安全取消句柄和空闲期治理指标 | ABI/version/位域/栈上限拒绝、有限库面、可信宿主 loader、资源错误、指令预算、foreign-thread/busy metrics 拒绝、消费量/停止分类、跨线程取消和 close 后迟到请求；安装后纯 C consumer 对静态/共享目标执行同一合同 | 非 Lua 5.1 官方面，不参与官方差分 | 11/11 入口具备 C/C++ 编译、导出、源码树与安装树执行证据 |
| 栈顶与容量 | `lua_gettop`、`lua_settop`、`lua_checkstack` | 正/负 `settop`、扩栈 nil 填充、主线程虚拟槽隔离、最大容量拒绝 | 原始 `api.lua` 的 `T.testC` 栈协议 exact PASS | 已实现并形成直接 + TestC 证据 |
| 普通索引 | 正索引、负索引、invalid index、`lua_pushvalue` | 正/负/越界读取与复制 | 原始 `api.lua` 覆盖普通索引和栈顶相对参数 | 已实现并形成直接 + TestC 证据 |
| 栈重排 | `lua_insert`、`lua_remove`、`lua_replace` | 正/负位置、C 回调帧内 remove | 原始 `api.lua` 覆盖 `insert/remove/replace` 命令 | 已实现并形成直接 + TestC 证据 |
| pseudo-index 与环境 | `LUA_REGISTRYINDEX`、`LUA_GLOBALSINDEX`、`LUA_ENVIRONINDEX`、upvalue pseudo-index；`lua_getfenv/lua_setfenv` 覆盖 function、userdata、thread 与 unsupported value | registry/global table 读写、closure upvalue；公开环境入口的返回值、消费栈与 GETGLOBAL 行为 | 原始 `api.lua` 的 `G/E/R/U<n>` 命令和 userdata environment 路径通过；纯 C probe 与官方结果一致 | 已实现并形成直接 + TestC + 官方差分证据 |
| C closure upvalue | `lua_upvalueindex(n)`、`lua_pushcclosure(fn,n)`、pseudo-index replace | 捕获顺序、消费栈值、回调读取、持久修改 | 原始 `api.lua` 的 closure/upvalue 命令通过 | 已实现并形成直接 + TestC 证据 |
| 类型、转换与对象身份 | `lua_type`、`lua_typename`、number/integer/string/boolean/C function/thread/userdata 转换、`lua_topointer` 及部分 `is*` | 数值与字符串转 integer、C callback 身份、table/function/thread/userdata/light userdata 稳定身份及 primitive 拒绝 | 原始 `api.lua` 的 `is*`、`tobool`、`tonumber`、`tostring`、`objsize` 命令通过 | 当前公开子集已验证；新增入口另与官方 Lua 5.1 纯 C probe 做差分 |
| 线程身份与 C GC 控制 | `lua_pushthread`；`lua_gc` 的 stop/restart/collect/count/countb/step/setpause/setstepmul | main/coroutine 自身 round-trip 与返回标志；内存计数、控制参数旧值、自动收集状态、step 与非法操作 | 纯 C probe 同时链接官方 Lua 5.1 和本项目并逐字节比较稳定输出 | 公开入口、直接测试和官方差分证据闭环 |
| 表与全局 | `createtable`、`gettable/settable`、`getfield/setfield`、`rawget/rawset`、`rawgeti/rawseti`、`next`、global get/set | 精确栈效果、正负索引、registry/global pseudo-index、`__index/__newindex` 与 raw 绕过、数组/哈希遍历和终止弹栈 | 原始 `api.lua` 的 table/global/raw/next 语义通过项目 `T` helper | 公开入口、直接测试与 TestC 证据闭环 |
| 比较与拼接 | `lua_equal`、`lua_rawequal`、`lua_lessthan`、`lua_concat` | primitive、invalid index、共享 `__eq/__lt`、raw identity、0/1/N 参数、数字转换、embedded NUL、`__concat` 与错误栈 | 原始 `api.lua` 的 equal/less/concat 命令通过项目 `T` helper | 公开入口、直接测试与 TestC 证据闭环 |
| 调用、错误与 yield | `lua_call`、`lua_pcall`、`lua_cpcall`、`lua_error`、`lua_newthread`、`lua_trynewthread`、`lua_checkexecution`、`lua_resume`、`lua_yield`、`lua_status`；官方 `lua_newthread` 是可抛的未保护入口，项目扩展 `lua_trynewthread` 才是 `noexcept` 安全入口 | 原始非字符串 error object、栈前缀恢复、正/负 handler 索引、C/Lua handler、`LUA_ERRERR`/`LUA_ERRMEM`、C→Lua→C、`lua_cpcall` lightuserdata/零结果/错误对象、Lua/C function coroutine、thread 每个 allocator 失败点、父栈发布失败、reader/writer/C callback 的标准与非标准异常、持久 allocator failure、死协程 traceback；原生 callback 的 deadline/atomic cancellation、fixed error object、owner thread 与 instruction-budget 非消费 | 官方 `errors.lua` tail、`db.lua` 和原始 `api.lua` exact PASS；纯 C probe 验证 `lua_cpcall` | `lua_newthread` 在分配失败时完成强回滚后传播错误，匹配 Lua 5.1 未保护语义；`lua_trynewthread` 复用同一事务并以 `nullptr` 包含异常。`lua_checkexecution` 是可抛的 cooperative 项目扩展；`lua_pcall`、`lua_cpcall`、`lua_resume`、`lua_checkstack` 保持关闭异常边界 |
| panic、格式化与调用层级 | `lua_atpanic` 保存 runtime 共享回调；`lua_pushvfstring/lua_pushfstring` 支持 `%s/%c/%d/%f/%p/%%` 与未知格式原样保留；`lua_setlevel` 复制 C/Lua 重入深度 | handler 替换/跨线程共享、va_list 与完整格式词法、返回字符串身份、host-call depth 复制 | 同一纯 C probe 分别链接官方 Lua 5.1 与项目实现，稳定结果逐字节一致 | 5/5 入口已形成声明、静态/共享导出、直接测试和官方差分闭环 |
| load/dump | `lua_load`、`lua_dump`、`luaL_loadbuffer`、`luaL_loadstring`、`luaL_loadfile` 均为关闭异常边界 | reader 分片、source buffer、services-backed Lexer 整源/长词素/Token/`InputCursor` 缓存与 Parser 函数语法作用域/局部名/捕获名的逐分配点/零余量 hard limit、源码/文件编译、语法/文件状态、binary chunk 往返、reader/writer 的 `std::exception`、非标准异常与 `bad_alloc`、满栈和持久 OOM | 原始 `api.lua` 的 loadstring/loadfile/dump/undump/低内存路径通过 | 项目本地 chunk 闭环已实现；不宣称官方 `luac` 字节兼容 |
| closure introspection | `lua_getupvalue`、`lua_setupvalue` | C closure 空名称、Lua closure debug name、读写栈效应和持久修改 | 原始 `api.lua` 的 `T.upvalue` 路径通过 | 已实现并形成直接 + TestC 证据 |
| userdata | `lua_pushlightuserdata`、`lua_newuserdata`、`lua_touserdata`、`lua_objlen`、metatable、environment、`__gc` | light/full/零长度 payload、8 字节对齐、metatable 往返/移除、字符串/表长度、普通 GC 与 close-time 终结器一次执行、payload 可见性及错误隔离 | 原始 `api.lua` 的 userdata 值、environment、GC 和低内存路径通过 | 已实现当前公开子集；finalizer 重入与 Runtime 关闭另有回归测试 |
| registry refs | `luaL_ref/unref`、`luaL_getref` | nil reference、存取、释放后复用 | 原始 `api.lua` 的 `ref/getref/unref` 路径通过 | 已实现并形成直接 + TestC 证据 |
| auxlib 公共层 | 34 个 Lua 5.1 auxlib 函数全部公开；包含 openlib/register、类型与可选参数检查、命名 metatable/userdata 检查、错误位置、findtable/gsub 及通用 buffer | upvalue 注册、global/_LOADED 身份、函数名与源码行错误、默认参数、错误分支、metafield 调用、userdata 身份、dotted conflict、跨 flush embedded NUL buffer | 同一纯 C probe 分别链接官方 Lua 5.1 与项目实现，registration/metatable/check/buffer/error/newstate 结果逐字节一致 | 34/34 官方 auxlib 函数已形成声明、导出、直接测试和官方差分闭环 |
| 调试 C API | `lua_Debug`/`lua_Hook` 精确公开布局；`lua_getstack/getinfo/getlocal/setlocal` 与四个 hook 管理入口；C hook 和语言层 hook 共用单槽、count mask 与重入保护 | `>SufL` 栈副作用、C/Lua/tail activation、活动 local 读写、active lines、call/return/line/count/tail-return 事件、zero-mask 禁用和 getter | 同一纯 C probe 在官方与项目 Runtime 上逐项比较 function/stack/local/hook/config/event/disable/invalid-level 结果 | 8/8 本批官方调试函数已形成声明、布局、导出、直接测试和官方差分闭环 |
| 标准库公开入口 | `lualib.h` 的 8 个 `luaopen_*` 函数、官方库名/文件句柄宏；`luaopen_base` 同时打开 base/coroutine 并返回两个表，其余入口返回对应全局库表；默认 sandbox unrestricted，配置 profile 后 opener 在发布前执行库策略 | C activation 调用形态、精确返回栈、全局表身份、`luaL_openlibs` 栈保持与全库注册；禁用库 opener 栈不变且不发布全局，base/coroutine 成对预检 | 同一纯 C probe 分别链接官方 Lua 5.1 与项目实现，8 个 opener、宏与 openlibs 结果逐字节一致 | 默认行为保持 8/8 官方差分闭环；配置后的项目 sandbox 另有直接策略测试 |
| allocator 与内存故障 | `lua_newstate` 保存/调用 callback；`lua_getallocf/setallocf`；主/协程 State、Context、GC object、userdata payload，以及 Stack/CallInfo、VM `__call` 实参暂存与 `OP_CONCAT` 结果、标准库 `table.concat` 结果、`table.sort` 工作副本/比较器快照和 I/O read buffer、GC 工作列表、StringPool 索引、Table、Proto/Function 容量、reader source buffer、services-backed Lexer/Token 缓存、Parser 函数语法作用域/名称与 AST Expr/Stmt 节点对象；当前 allocator 负责关闭释放 | callback/userdata 往返、allocator 替换、初始化分配失败、协程创建每一实际分配点、protected-call/GC object/payload 运行期失败、长字符串内容、Table/SETLIST 与 Proto 真实 realloc、Stack/CallInfo、宽 `__call`、VM concat、`table.concat`、`table.sort` 与 `file:read("*a")` 的 fail-on-N/零余量 hard limit、loader/compiler fail-on-N、Token/AST allocator 对象外存活、old-size/关闭归零 | 原始 `api.lua` 的 `T.totalmem` 低内存循环 exact PASS | 长字符串、核心 Table/Proto、State stack/CallInfo、VM `__call`/concat、标准库 `table.concat`/`table.sort`、I/O read buffer、GC worklist、reader source buffer、Lexer/Token buffer、Parser syntax-scope 与 AST 节点对象切片已形成 allocator hard-limit/事务证据；AST 内部字符串/向量载荷及其余 codegen、stdlib/I/O/package/debug 临时容器仍不支持全运行时 hard-limit 声明，详见内存合同 |
| State/协程组移动 | `lua_xmove` | 同组双向顺序保持；拒绝独立 `EngineContext` State | `api.lua` 覆盖 remote/new-stack 语义，但不作为公开 `lua_xmove` 的替代证据 | 移动语义与独立所有权已直接测试 |
| 原生 C 模块 | `package.loadlib` / C searcher 使用 context-owned `NativeModuleRegistry` | 纯 C `lua.h` fixture 由 `lua_app`、公开 API embedding host 和双-context host 动态加载；open/init 错误、per-state 状态、lease cache、关闭与重载；模块内 `__gc` 写入外部标记证明 finalizer 先于卸载 | 官方模块源码不作为本项目二进制 ABI 替代证据 | Windows CMake 已验证 host import table；Linux/Windows CI 均运行 `api-contract` / `native-module` CTest |
| 官方 helper | 项目版 `T.testC`、`T.listcode` 及 state/userdata/ref/memory helpers 位于 `src/lib/testlib.cpp` | 独立 `official-testc` 通道实际打开 `T`，不再自跳过 | 原始 `api.lua` exact PASS；`code.lua` 的 5.1.5 oracle 来源已锁定，校正通用 5.1 fixture 后 required PASS | API 与 code TestC 契约闭环；该通道不保留已知 XFAIL |

## 当前证据

2026-07-26 的直接门禁：

<!-- public-api-surface: functions=143 macros=61 enum-constants=55 typedefs=15 -->

```powershell
bin\lua_test.exe --filter "Lua C API"
```

当前本地 Release 结果为 61 个测试、2910 个断言、0 failures。机器合同包含 123 个官方公共函数：
123 个 `PASS`、0 个 `XFAIL`、0 个 `UNSUPPORTED`。项目头文件的当前公开面另由 143 个真实函数、
61 个宏、55 个枚举常量和 15 个 typedef 的穷尽式编译合同保护。当前完整 Release 套件为
791 个测试、6773 个断言、0 failures、0 expected skips、0 unexpected skips。修复提交
`4b0bc71` 已在 [PR #14 的 Actions run 29993098262](https://github.com/YanqingXu/lua/actions/runs/29993098262)
取得此前基线的 17/17 jobs 全绿；该历史结果不替代当前候选所需的同 SHA API、官方 strict、
差分、平台构建、sanitizer、fuzz、coverage、allocator、ARM64、macOS、benchmark 与 lint
证据。原始 `api.lua` 另以以下 exact TestC 门禁通过：

```powershell
bin\lua_test.exe --filter "api.lua with T module"
```

该次结果为 1 个选中测试、1 个断言、0 failures，并完整执行到官方脚本的 `OK`。完整套件总数由测试运行器实时报告；本页不冻结一个可能随新增回归变化的全局计数。

独立模块证据由以下 CTest 标签持续执行：

```powershell
ctest --test-dir build -C Debug -L api-contract --output-on-failure
ctest --test-dir build -C Debug -L native-module --output-on-failure
```

`api-contract` 标签同时运行源码树内的静态/共享消费者、安装后的 `find_package(LuaCpp)` 静态/共享纯 C 源码 consumer 和候选 C API probe；Windows DLL 的导出面必须与 `.def` 中的 143 个符号完全一致，Linux shared object 由版本脚本只公开同一集合，macOS 的 Mach-O export list 则由 `.def` 自动生成以避免第三份手写清单漂移。安装 consumer 还实际创建 game-server State，并验证库面、资源限制、每请求指令预算、metrics 和 State 生命周期外取消安全。Linux Clang Debug 另外将 `lua51_c_api_differential_probe.c` 分别链接官方 Lua 5.1 和本项目，逐字节比较退出码、stdout 与 stderr，并上传 JSON 证据。

## 下一批失败驱动任务

1. 将已经闭环的长字符串、Table/SETLIST、Proto、GC worklist、reader source buffer、Lexer/Token、Parser syntax-scope 与 AST 节点对象 allocator 切片扩展到 AST 内部字符串/向量载荷、其余 codegen 临时容器、Parser 诊断对象和标准库临时对象；在此之前保持全运行时 hard limit 为 `UNSUPPORTED`，由 [#5](https://github.com/YanqingXu/lua/issues/5) 跟踪。
2. 扩展 coroutine 的嵌套 resume 与 Lua 5.1 不提供的 C yield continuation 边界，同时保持死协程 traceback 与关闭异常边界回归。
3. 保持原始 `api.lua` exact 门禁；`code.lua` 保持 upstream 文件字节不变，运行时只应用清单登记的 5.1.5 oracle 校正，并继续拒绝新增编译器 parity XFAIL。
