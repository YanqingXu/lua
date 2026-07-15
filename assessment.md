# 评估结论

截至 **2026 年 7 月 14 日**，`main` 最新提交为 `d956830`，近期提交集中在 sanitizer、跨平台 CI、Lua C API、allocator、协程和嵌入能力。这个仓库已经不再处于“把解释器主要功能做出来”的阶段，而是进入了：

> **Lua 5.1 公共接口一致性、嵌入式运行时加固和生产级证据建设阶段。**

编译器、38 条 VM 指令、运行时对象、标准库、GC、REPL 和字节码工具的主链路已经完整；继续横向增加普通语法、REPL 命令或教学展示功能，边际收益已经很低。下一阶段主线应明确收敛为：

> **Lua 5.1 Public API Conformance & Game-Server Runtime Hardening**

也就是：公共 C API 一致性、资源预算、异常边界、多 Runtime 隔离、原生模块生命周期和性能回归门禁。

## 当前成熟度判断

以下是工程定位评分，不是测试覆盖率：

| 定位                          |       当前评分 | 判断                                  |
| --------------------------- | ---------: | ----------------------------------- |
| 现代 C++ 解释器教学/源码研究项目         | **9.0/10** | 已接近标杆级                              |
| Lua 5.1 语义研究与实验平台           | **8.5/10** | 核心语义和验证体系较完整                        |
| 可嵌入 Runtime Preview         | **7.5/10** | 已有真正可用的 C API MVP                   |
| 游戏服务器生产候选 Runtime           | **6.5/10** | 还缺资源治理、模块生命周期和长期稳定性证据               |
| Lua 5.1 drop-in replacement | **5.5/10** | 公共 API、ABI、官方套件和 binary module 仍未闭环 |

相较仓库中 7 月 12 日的旧评估，提升最明显的是嵌入能力和工程证据：自定义 allocator、独立 `EngineContext`、userdata、registry、load/dump、protected call、coroutine、TestC、跨平台 CI、sanitizer 和 benchmark 都已经实质落地。

# 已完成到什么程度

## 1. 解释器主链路：基本完成

仓库已经覆盖 Lexer、Parser、AST、CodeGen、Lua 5.1 风格 38 条 opcode、寄存器 VM、标准库、弱表、userdata finalizer、增量 GC、REPL 和字节码分析工具。核心工作现在不再是补普通语言功能，而是验证边缘语义和宿主接口。

## 2. C API：已经达到可演示且可实际嵌入的 MVP

当前已经实现并直接测试了：

* 独立 State 和 `EngineContext`；
* 栈、普通索引和 pseudo-index；
* C closure upvalue；
* userdata、metatable、environment、registry ref；
* 自定义 allocator 与初始化失败回滚；
* `lua_load`、`lua_dump` 和 `luaL_load*`；
* `lua_call`、`lua_pcall`；
* `lua_newthread`、`lua_resume`、`lua_yield`；
* Lua/C function coroutine 入口；
* 多 State 隔离和 `lua_xmove`。

当前记录的 C API suite 为 **33 个测试、805 个断言、0 failures**；原始 `api.lua` 通过项目的 `T` helper 完整执行到 `OK`。不过仓库自己的文档也正确地强调：这不等于官方 `ltests.c` 二进制 ABI，也不等于所有 Lua 5.1 公共符号都已实现。

## 3. 测试和 CI：已经从弱项变成强项

当前 CI 设计已经包含：

* Windows MSVC Debug/Release；
* Linux GCC/Clang Debug/Release；
* CMake/CTest；
* ASan 和 UBSan；
* clang-format 和 clang-tidy；
* 官方 smoke、TestC、slow suite；
* Lua 5.1 differential evidence；
* strict suite 的独立证据产物；
* Release runtime benchmark。

README 记录的本地完整 Debug 基线是 **726 registered tests、4353 assertions、0 failures**。

需要区分“workflow 已配置”和“最新 SHA 已在线绿跑”。当前连接器没有返回该最新提交对应的 PR-triggered workflow run，因此这里不能替仓库确认最新 Actions 全绿，只能确认门禁设计和仓库记录的本地绿跑。

## 4. 性能证据：测量框架完整，但回归政策未完成

benchmark 已覆盖解析/编译吞吐、VM 指令吞吐、C++→Lua、Lua→C++、coroutine resume/yield、table 操作、10 万 closure 生命周期、allocator 吞吐、GC P50/P95/P99/max pause 和长期 heap slope。它已经是一个不错的游戏服务器场景基准框架。

当前缺的是 **base-vs-head 回归阈值**：现有门禁会验证证据格式、正确性和堆稳定性，但不会因吞吐下降或 P99 延迟恶化自动失败，这正由 issue #7 跟踪。

# 当前最重要的未闭环问题

## P0：公共 C API 还不是严格 Lua 5.1 一致实现

这不只是“缺少一些函数”，当前已经能看到具体的公共语义差异：

* 项目中的 `lua_pushstring(L, nullptr)` 会压入空字符串；官方 Lua 5.1.5 会压入 `nil`。
* 项目中的 `lua_objlen` 对 number 返回 `0`；官方 Lua 5.1.5 会先将 number 转成字符串，然后返回字符串长度。  ([Lua][1])

此外，当前公共头文件仍缺少 `lua_getfenv`、`lua_setfenv`、遍历、比较以及其他 Lua 5.1 API 符号；仓库文档对此已有明确记录。

这说明下一步不应该继续零散补 API，而应建立一份**机器可验证的官方 API 合同**：

```text
官方符号
→ 项目是否声明
→ 是否实现
→ 是否有纯 C 编译证据
→ 是否有直接运行时测试
→ 是否与官方可观察语义一致
→ 未实现时对应哪个 XFAIL/issue
```

## P0：protected C API 的异常边界仍不完整

Issue #8 已经准确识别了问题：`lua_pcall`、`lua_resume`、`lua_load`、`lua_dump`、`lua_newthread` 等状态型 API 应成为关闭的 C++ 异常边界。

例如当前 `lua_load` 调用宿主 reader 时只捕获 `std::bad_alloc`；reader 抛出普通 `std::exception` 或非标准异常时仍可能穿过 `extern "C"` 边界。

建议不要逐函数继续堆 `try/catch`，而是引入统一边界模板，例如：

```cpp
apiStatusBoundary(L, fallbackStatus, [&] {
    // implementation
});
```

统一处理：

* `MemoryError` / `std::bad_alloc` → `LUA_ERRMEM`；
* `LuaError` → 保留原始 Lua error object；
* `std::exception` → allocation-safe runtime error；
* 非标准异常 → 固定 emergency error；
* stack shape 回滚；
* 不在 OOM 路径二次分配。

## P0：内存预算的语义仍不明确

当前 allocator 接入范围已经很大，但 issue #5 指出的核心问题仍成立：GC memory counter、allocator 实际 live bytes 和“宿主硬内存上限”是三种不同概念。字符串内容、部分动态容量和增长后的回滚仍未形成严格事务性证据。

建议明确拆成两个合同：

1. **GC debt / soft budget**：只负责何时推进 GC。
2. **allocator live-byte hard limit**：任何失败操作之后都保证 `used <= limit`，且不留下部分提交的容器扩容。

在第二个合同完成前，不应把当前机制命名为“硬内存限制”。

## P0：原生 C 模块生命周期没有纳入 Runtime 隔离

这是代码检查中最值得新增 issue 的地方。

当前动态库句柄保存在进程级 `static std::unordered_map` 中，并按完整路径永久复用。该 registry 不属于 `EngineContext`，所以多个独立 Runtime 仍共享同一动态模块句柄生命周期。当前可见实现只展示了 `LoadLibrary`/`dlopen` 和缓存，没有与 State/Context 析构绑定的卸载路径。

现有 C loader 测试也主要是从 `lua_test` 自身解析导出的 `luaopen_*`，而且 fixture 内部直接把 `lua_State*` 转成项目内部 `LuaState*`。这验证了 loader 搜索和调用流程，但没有证明“独立编译、只包含公开 `lua.h`、只调用 `lua_*` 符号”的第三方二进制模块可以直接加载。

Unix 构建目前仅对 `lua_test` 设置了 `-rdynamic`，实际 `lua_app` 的公共符号导出链路也尚未形成同等测试证据。

建议：

* 将动态库 registry 移入 `EngineContext` 或专门的 `ModuleRuntime`；
* 定义 context-close 时的引用计数和卸载策略；
* 明确 hot reload 是“同句柄重执行入口”还是“真正卸载并重新装载”；
* 构建一个独立 `.dll/.so` fixture，只依赖公共头文件；
* 用 `lua_app` 和 embedding executable 同时加载；
* 在 Windows 和 Linux 验证符号导出、错误路径和 State 关闭。

## P1：严格官方 Lua 5.1 套件仍未完成

当前 smoke、TestC、`sort.lua` 和 `verybig.lua` 的证据已经比较强；但 unmodified `all.lua` 在 300 秒预算内仍会在 closure/coroutine 阶段超时，所以 CI 仍以预期失败方式运行 strict lane。

这里应先做性能定位，而不是直接扩大超时时间：

* Release strict 是否能完成；
* 每个官方脚本耗时；
* GC cycle 数量和暂停；
* closure/upvalue 创建、关闭和回收复杂度；
* coroutine resume 路径是否存在重复复制；
* stack slot 清理是否形成非必要的 O(n) 扫描。

如果总套件确实不适合单 job，再拆成**未经修改的确定性 shards**，而不是改写官方源码。

`code.lua` 的唯一 LOADNIL XFAIL 则应通过锁定测试源 SHA、官方 `luac` 版本和最小 chunk listing 来解决；现有证据更像 fixture/oracle 版本错配，而不是运行时语义缺陷。

# 推荐开发路线

## 第一阶段：公共 API 与边界闭环

优先完成：

1. 建立官方 `lua.h`、`lauxlib.h`、`lualib.h` 的机器可读符号清单。
2. 修复 `lua_pushstring(nullptr)`、numeric `lua_objlen` 等已发现差异。
3. 完成 issue #8 的统一异常边界。
4. 增加纯 C header compile、C++ `/O2 /EHsc` consumer 和链接测试。
5. 增加独立 `.dll/.so` 公共 API 模块 fixture。
6. 将动态库句柄生命周期移入 `EngineContext`。
7. 在合同建立后，再按 state/stack/value/table/call/load/auxlib 拆分当前集中式 `lapi.cpp`。

阶段验收标准应是：

* status-returning API 不泄漏 C++ 异常；
* 官方公共符号全部处于 `PASS / explicit XFAIL / unsupported` 三态之一；
* Windows/Linux 都能加载只依赖公开头文件的独立模块；
* 两个 EngineContext 的模块状态和关闭路径可验证隔离；
* ASan/UBSan 全绿。

## 第二阶段：严格兼容与内存合同

随后完成：

1. issue #5：硬内存限制和 GC soft budget 分离；
2. allocator fail-on-N、realloc、字符串内容、table growth、Proto metadata 测试；
3. strict suite Release profiling 和原样 shard；
4. issue #4 的 fixture/oracle 来源锁定；
5. 将 differential runner 扩展到公共 C API stack trace，而不仅是 Lua 脚本输出。

这里建议把每个 C API 探针规范化成：

```text
status
stack top
每个 slot 的 Lua type
稳定值表示
error object 类型
allocator live bytes
GC object count
```

然后在官方 Lua 5.1.5 和当前实现之间比较。

## 第三阶段：游戏服务器运行时能力

完成兼容性合同后，再进入真正的服务器运行时建设：

* 每 State 指令预算和 deadline；
* 宿主主动取消；
* 内存 hard limit；
* finalizer 单轮执行预算；
* 禁用或替换 `io`、`os`、`debug`、C loader 的 sandbox profile；
* 模块热更新和旧 closure 生命周期；
* 数千次 State 创建/销毁压力；
* 长时间 coroutine/table/closure churn soak；
* 多宿主线程、每线程独立 EngineContext 的并行压力；
* base-vs-head benchmark，并对 C++↔Lua 调用、P99 GC pause、closure churn 和 coroutine resume 设置回归预算。

性能门禁应采用同一 runner 上 base/head 交错运行，而不是把 GitHub Hosted Runner 的绝对数字固化为阈值，这也与当前 benchmark 文档和 issue #7 的方向一致。

# 建议的实际优先级

按开发价值和风险排序：

1. **Issue #8：protected C API exception boundary**
2. **新增：Official Public C API Conformance Contract**
3. **新增：EngineContext-owned Native Module Registry**
4. **Issue #5：allocator-backed memory contract**
5. **Issue #3：unmodified strict suite**
6. **Issue #4：code.lua oracle provenance**
7. **Issue #7：base-vs-head performance thresholds**
8. **Issue #6：main required checks / branch protection**

最终判断是：**解释器本体已经足够完整，下一步的关键不是“做更多”，而是证明公共接口严格、资源可控、Runtime 可隔离、模块可管理、性能不会回退。** 当上述 P0/P1 闭环后，这个项目才会从“优秀的现代 C++ Lua 实现”迈入“可信的游戏服务器嵌入式 Runtime 候选”。

[1]: https://www.lua.org/source/5.1/lapi.c.html "Lua 5.1.5 source code - lapi.c"
