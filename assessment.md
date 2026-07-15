## 审计快照结论

我把本次审计锁定在 `main` 最新提交 [`a9d4e4e`](https://github.com/YanqingXu/lua/commit/a9d4e4e2e3b3bf4b2ccb498010257ac9a41ffc3a)。

项目已经跨过“把 Lua 解释器主体做出来”的阶段。当前最准确的定位是：

> 优秀的现代 C++ Lua 5.1 语义研究项目 + 有实际嵌入能力的 Runtime Preview，但还不是 Lua 5.1 drop-in replacement，也不宜直接进入游戏服务器生产环境。

你当前选择的主方向——“Lua 5.1 公共接口一致性 + 游戏服务器 Runtime 加固”——是正确的。下一阶段需要暂停横向增加普通功能，转向证明四件事：

1. 公共接口真的兼容；
2. State 关闭和资源释放真的正确；
3. 内存、执行时间和脚本权限真的可控；
4. 每项结论都有对应 SHA 的在线验证证据。

## 审计后执行状态

以下状态记录审计建议在后续提交中的落实情况；下文“成熟度”和“深度审计发现”保留 `a9d4e4e` 快照判断，不再冒充当前实现状态。

* [`aeced59`](https://github.com/YanqingXu/lua/commit/aeced59) 已修复 `lua_close` 的 main-state/coroutine、close-time `__gc`、finalizer 错误隔离、原生模块卸载顺序和持续 OOM 关闭语义。
* [`23e34c0`](https://github.com/YanqingXu/lua/commit/23e34c0) 已把公开合同扩展为头文件、C 链接探针、C++ 精确签名、Windows `.def`、Linux version script 和独立 shared consumer 的穷尽式一致性检查。
* [`da3c81e`](https://github.com/YanqingXu/lua/commit/da3c81e) 已恢复官方 `lua_newthread` 的未保护传播语义，并新增事务式、`noexcept` 的项目扩展 `lua_trynewthread`。
* 本文件所在提交新增本地质量门 `-Strict` 模式；缺少 `git`、clang-format、clang-tidy、MSBuild、配置的 smoke 输入或测试产物时不再静默成功。显式 `-SkipBuild`、`-SkipClangTidy`、`-FormatScope Off` 仍保留为可审计的调用者选择。
* 本文件所在提交补齐 `lua_atpanic`、`lua_pushvfstring/lua_pushfstring`、`lua_getfenv/lua_setfenv`、`lua_cpcall` 与 `lua_setlevel`，使官方 Lua 5.1 公共函数合同达到 123/123 PASS；格式化、环境、错误对象和线程层级栈效应均进入同一纯 C 官方差分 probe。
* 当前本地 Debug/Release strict 基线为 **744 tests / 4803 assertions / 0 failures**，C API 为 **49 tests / 1247 assertions / 0 failures**；官方函数合同为 123 PASS、0 XFAIL、0 UNSUPPORTED，项目实际公开面为 130 个函数。核心表/比较、类型/线程/GC、完整 auxlib、调试入口、标准库 opener 与最后 panic/格式化/环境/cpcall/setlevel 批次，均已具备精确签名、静态/共享导出、直接语义测试和官方 Lua 5.1 纯 C 差分证据。
* **同 SHA 在线证据尚未完成。** 当前 CI 仅由 `main/master` push 或 pull request 触发；截至本次核查，当前修复分支的 `da3c81e` 没有 Actions run、check run 或 commit status，因此不能用本地绿跑替代在线验收。

剩余 P0 是为当前修复头提交触发并保留 Windows 2、Linux 4、ASan/UBSan、lint、strict、benchmark 的同 SHA Actions 与 artifact；P1 的 C API 扩面、ExecutionPolicy 和 allocator hard limit 仍按后文路线推进。

## 审计快照成熟度

| 维度                          |        评价 | 判断                                                      |
| --------------------------- | --------: | ------------------------------------------------------- |
| 现代 C++ 解释器教学                |    8.8/10 | `variant/expected/span`、显式结果类型、RuntimeServices 分层已经接近标杆 |
| Lua 5.1 源码语义                |    8.5/10 | 主链路完整，官方 strict 已配置为零 XFAIL，但最新 SHA 缺在线运行证据             |
| 嵌入式 Runtime Preview         |    7.3/10 | C API、allocator、独立 State、原生模块已真正可用，但关闭语义仍有缺陷            |
| 游戏服务器生产候选                   |    6.5/10 | 缺 hard limit、执行预算、取消、sandbox、线程合同和长期压力证据                |
| Lua 5.1 drop-in replacement |    4.5/10 | 123 个官方函数仅 60 PASS，而且宏、类型布局、ABI 尚未纳入合同                  |
| CI 配置 / CI 实证               | 9.0 / 5.5 | workflow 设计很强，但没有取得最新 SHA 的线上全绿证据                       |

drop-in 评分较低不是项目倒退，而是最新机器合同终于把真实缺口量化出来了。

## `a9d4e4e` 已经取得的实质进展

* Lexer、Parser、AST、CodeGen、38 条 Lua 5.1 opcode、VM、标准库、弱表、finalizer、增量 GC、REPL 和字节码工具主链路已经完整。
* 仓库记录的本地基线为 **734 tests / 4487 assertions / 0 failures**；C API 为 **39 tests / 931 assertions**。[README](https://github.com/YanqingXu/lua/blob/a9d4e4e2e3b3bf4b2ccb498010257ac9a41ffc3a/README.md)
* official strict 的 XFAIL 已清空；`api.lua` 记录为 exact PASS；`code.lua` 只剩一条明确的 repeat-until 条件编译字节码差异。[strict XFAIL](https://github.com/YanqingXu/lua/blob/a9d4e4e2e3b3bf4b2ccb498010257ac9a41ffc3a/tests/compatibility/lua51-official-strict-xfails.json)、[TestC XFAIL](https://github.com/YanqingXu/lua/blob/a9d4e4e2e3b3bf4b2ccb498010257ac9a41ffc3a/tests/compatibility/lua51-official-testc-xfails.json)
* protected C API 已建立统一异常边界，覆盖 `MemoryError`、`bad_alloc`、Lua 错误、普通及非标准异常。
* 原生模块 handle/cache 已从进程静态表迁入 `EngineContext`，并增加纯 C `.dll/.so` fixture。[NativeModuleRegistry](https://github.com/YanqingXu/lua/blob/a9d4e4e2e3b3bf4b2ccb498010257ac9a41ffc3a/src/runtime/native_module_registry.cpp)
* CI 已配置 Windows MSBuild、Linux GCC/Clang、ASan/UBSan、strict/differential、lint 和 base-vs-head benchmark。[CI 配置](https://github.com/YanqingXu/lua/blob/a9d4e4e2e3b3bf4b2ccb498010257ac9a41ffc3a/.github/workflows/ci.yml)

这些都不是“文档规划”，而是已经能在实现和测试入口中看到的工程化落地。

## `a9d4e4e` 深度审计发现的主要问题

### 1. 最新提交过大，且没有取得在线全绿证据

`a9d4e4e` 一次修改了 73 个文件：

* 总计 `+13,415/-9,255`；
* 排除重生成的 C-style baseline，仍有 `+5,445/-1,369`；
* 同时涉及 C API、allocator、GC、native module、官方套件、benchmark、CI 和文档。

它没有对应 PR。GitHub 连接器也没有返回该 SHA 的 workflow run 或 commit status。这个结果不能绝对证明 push run 不存在，但可以确认：目前无法用取得的证据证明最新 SHA 在线全绿。

与此同时，[assessment.md](https://github.com/YanqingXu/lua/blob/a9d4e4e2e3b3bf4b2ccb498010257ac9a41ffc3a/assessment.md) 仍把 `d956830` 写成最新提交，并保留 726/4353、strict 未完成、模块未隔离、benchmark 未完成等旧结论。这说明文档漂移门只能检查格式和部分数字，不能保证事实一致。

### 2. C API 的“60 PASS”不能等同于 49% 完整兼容率

当前合同统计如下：[公共函数合同](https://github.com/YanqingXu/lua/blob/a9d4e4e2e3b3bf4b2ccb498010257ac9a41ffc3a/tests/compatibility/lua51-public-api-contract.json)

| 头文件         | PASS | UNSUPPORTED |
| ----------- | ---: | ----------: |
| `lua.h`     |   50 |          30 |
| `lauxlib.h` |    9 |          25 |
| `lualib.h`  |    1 |           8 |
| 合计          |   60 |          63 |

而且当前合同只覆盖“函数名”，没有完整覆盖：

* 宏和常量；
* `lua_Debug`、`luaL_Buffer` 等结构和布局；
* 函数的精确签名与调用约定；
* shared-library ABI；
* 官方 Lua 5.1 与当前实现的 C API 差分行为。

还有一个具体合同漏洞：公开头文件额外声明了以下 6 个真实函数，但 Windows `.def` 没有导出它们：

* `lua_open`
* `lua_getglobal`
* `lua_setglobal`
* `luaL_argcheck`
* `luaL_checkint`
* `luaL_checkstring`

静态 consumer 可以通过，但独立 DLL 使用这些常见入口可能链接失败。

### 3. `lua_close` 存在已确认的生命周期缺口

当前调用链是：

```text
lua_close
→ LuaState::destroyState
→ EngineContext 析构
→ GarbageCollector::~GarbageCollector
→ clearAll
→ 清空 pendingFinalizers
→ 直接销毁对象
```

这条关闭路径没有进入 `runFinalizers()`。[lapi.cpp](https://github.com/YanqingXu/lua/blob/a9d4e4e2e3b3bf4b2ccb498010257ac9a41ffc3a/src/api/lapi.cpp)、[GarbageCollector](https://github.com/YanqingXu/lua/blob/a9d4e4e2e3b3bf4b2ccb498010257ac9a41ffc3a/src/gc/garbage_collector.cpp)

因此，尚未经过一次 GC 的 userdata，其 `__gc` 在 `lua_close` 时不会执行。依靠 `__gc` 释放文件、socket、数据库句柄或原生模块资源时，会产生语义错误甚至资源泄漏。

另一个高风险路径是：`lua_close` 收到 coroutine state 时直接销毁该 State，而不是转向所属 main state。需要增加 ASan 回归验证其是否会让 `Thread` 内部保留悬空 State。

在这个问题修复前，`lua_close` 不应该继续被视为严格语义 PASS。

### 4. `lua_newthread` 的“安全语义”与官方语义存在冲突

当前 `lua_newthread` 被声明为 `noexcept`，捕获所有异常后返回 `nullptr`。这对游戏服务器宿主更安全，但官方 Lua 5.1 在内存错误时走未保护错误传播，不是普通的 `nullptr` 返回。

这暴露了项目当前两个目标间的冲突：

* 严格 Lua 5.1 drop-in；
* 更安全的现代 C++ 嵌入接口。

建议保留官方语义的 `lua_newthread`，另提供项目扩展 `lua_trynewthread`；或者至少将当前函数标记为显式 XFAIL/SAFE_DIVERGENCE，不能继续叫语义 PASS。

### 5. 游戏服务器运行时治理尚未完成

[内存合同](https://github.com/YanqingXu/lua/blob/a9d4e4e2e3b3bf4b2ccb498010257ac9a41ffc3a/docs/runtime/memory-contract.md) 已明确承认：

> `allocator-backed hard limit = unsupported`

当前 allocator 只闭环了字符串、部分 Table/Proto、State、栈、userdata 等核心切片。仍可能绕过 callback 的部分包括：

* reader/lexer/parser/AST/codegen 临时容器；
* stdlib、I/O、debug、package 临时字符串和容器；
* Table hash、GC worklist；
* native module registry 和 OS loader。

此外目前还没有：

* 每 State 指令预算；
* monotonic deadline；
* atomic cancellation；
* 跨 coroutine 的预算继承；
* sandbox profile；
* finalizer 单轮预算；
* Runtime owner-thread 合同；
* TSan 和长时间并发 soak。

`vm_trace.cpp` 中的 trace sink、sequence 和开关仍是进程全局变量，会串扰多个 EngineContext，也不具备线程安全性。[vm_trace.cpp](https://github.com/YanqingXu/lua/blob/a9d4e4e2e3b3bf4b2ccb498010257ac9a41ffc3a/src/vm/vm_trace.cpp)

## 推荐开发路线

### P0：先稳定最新快照

暂停继续增加普通功能，先完成：

1. 取得 `a9d4e4e` 的 Windows 2、Linux 4、ASan/UBSan、lint、strict、benchmark 全部在线运行证据。
2. 修正 `assessment.md`、README 和 Issues 的状态矛盾。
3. 给本地 quality gate 增加严格模式：缺少工具或测试产物直接失败，不能静默 SKIP。
4. 后续 C API、allocator、benchmark、CI 分成独立 PR。

验收条件：所有结果和 artifact 都能定位到同一个 SHA。

### P0：修复 C API 生命周期和合同真实性

建议立即建立三个独立 Issue：

1. `Correct lua_close main-state and __gc semantics`
2. `Make public header/export contract exhaustive`
3. `Resolve lua_newthread strict-vs-safe semantics`

其中 `lua_close` 验收至少包含：

* 未手动 GC 的 userdata 在 close 时执行一次 `__gc`；
* 一个 finalizer 报错不阻断其他 finalizer；
* finalizer 发生在原生模块卸载之前；
* `lua_close(coroutine)` 安全关闭整个 Runtime；
* allocator 持续 OOM 时 close 仍不抛异常、不泄漏。

### P1：完成真正的 C API/ABI 合同

按以下批次推进 [#9](https://github.com/YanqingXu/lua/issues/9)：

1. 核心表和比较：`getfield/setfield/rawget/rawset/next/concat/equal/rawequal/lessthan`。
2. 类型、线程和 GC：`tointeger/tocfunction/tothread/topointer/pushthread/lua_gc`。
3. auxlib：注册、参数检查、metatable、buffer API。
4. debug C API：stack/info/local/hook。
5. `luaopen_*` 标准库导出。

合同同时扩展到：

* 精确函数签名；
* 宏、常量、typedef、结构布局；
* 每个 PASS 对应明确的测试 ID，而不是只在测试文件里搜索函数名；
* 官方 Lua 5.1 与本实现的同一 C probe 差分；
* Windows DLL 与 Linux shared-library 导出面。

### P1：建立游戏服务器 ExecutionPolicy

在 `EngineContext` 或 `LuaState` 增加：

* instruction budget；
* monotonic deadline；
* atomic cancel flag；
* finalizer budget；
* sandbox/module policy。

验收场景应包括：

* `while true do end` 在预算内退出；
* coroutine yield/resume 继承预算；
* C→Lua→C 不重置预算；
* 外部线程只能设置 atomic cancel，不能直接访问 State；
* 超限通过 protected API 返回稳定错误对象；
* Release 热路径开销进入 base-vs-head 门禁。

### P1：完成 allocator hard limit

推荐使用 allocator-aware container 或 `std::pmr::memory_resource`，为 Parser/AST/CodeGen 建立一次编译专属 arena，再逐步迁移 GC worklist 和标准库临时容器。

完成标准不是“更多路径接入 allocator”，而是所有 fail-on-N 点同时证明：

* `liveBytes <= limit`；
* 失败操作无部分提交；
* State 可继续运行；
* close 后归零；
* Windows/Linux Debug/Release、ASan/UBSan 使用同一矩阵通过。

### P2：生产级证据与结构收敛

* 增加 parser、binary chunk、C API stack sequence fuzzing；
* 建立 llvm-cov 趋势，但不要把测试数量当覆盖率；
* 每线程独立 EngineContext 的 TSan soak；
* State 创建/关闭、coroutine/closure/table/finalizer 24 小时压力；
* 将 trace 全局状态迁入 EngineContext；
* 明确一个 EngineContext 只允许一个 root State，或维护完整 root-State 集合；
* 在行为合同冻结后，再拆分超过千行的 `lapi.cpp`、base/string/debug/io/package 文件。

## 最终建议

建议把路线拆成两个清晰里程碑：

* **v0.9 Server Runtime Preview**：`lua_close` 正确、hard limit、ExecutionPolicy、sandbox、线程合同和 soak 完成。
* **v1.0 Lua 5.1 Compatibility**：123/123 公共函数、完整头文件/ABI 合同、零 TestC XFAIL、共享库 drop-in 验证。

如果下一步只能选择一个开发任务，我建议先做：

> **修复 `lua_close` 的 main-state、coroutine 和 close-time `__gc` 语义，并用纯 C 动态模块 + ASan 建立回归测试。**

这是目前兼容性、资源安全和游戏服务器可用性三条主线共同的最高价值任务。
