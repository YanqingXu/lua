---
status: current
verified_against: src/lua_runtime.h; src/api/lapi.cpp; src/runtime/runtime_configuration.hpp; src/runtime/runtime_services.hpp; src/runtime/execution_policy.hpp; src/runtime/sandbox_policy.hpp; src/runtime/resource_policy.hpp; src/runtime/compilation_policy.hpp; tests/unit/api/test_lua_c_api.cpp; tests/packaging/consumer/main.c
last_checked: 2026-07-24
applies_to: installed C SDK runtime creation, sandbox/resource configuration, per-request execution windows, and cross-thread cancellation
---

# 生产运行时公开 C API

安装后的 `lua_runtime.h` 是宿主配置生产运行时的稳定 C ABI。它把创建期 sandbox、执行预算、资源上限和编译上限放进一个版本化结构，并提供每个请求重新开始执行窗口、跨线程取消与请求结束指标快照。调用方不需要包含任何 C++ 内部头文件。

## 最小 game-server 宿主

```c
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <lua_runtime.h>
#include <string.h>

int run_request(const char* source) {
    lua_RuntimeConfig config;
    lua_runtime_config_init_gameserver(&config);

    int runtime_status = LUA_RUNTIME_OK;
    lua_State* L = luaL_newstate_configured(&config, &runtime_status);
    if (L == NULL) {
        return runtime_status;
    }
    luaL_openlibs(L);

    /* luaL_loadbuffer 是可信宿主入口；脚本侧 load/loadstring 不会暴露。 */
    if (luaL_loadbuffer(L, source, strlen(source), "request") != LUA_OK) {
        lua_close(L);
        return LUA_ERRSYNTAX;
    }

    lua_RuntimeExecutionLimits limits;
    lua_runtime_execution_limits_init(&limits);
    limits.instruction_budget = 1000000;
    limits.native_work_budget = 8 * 1024 * 1024;
    limits.finalizer_budget_per_drain = 128;
    limits.timeout_ms = 50;

    if (lua_runtime_begin_execution(L, &limits) != LUA_RUNTIME_OK) {
        lua_close(L);
        return LUA_ERRRUN;
    }

    const int status = lua_pcall(L, 0, 0, 0);
    lua_close(L);
    return status;
}
```

生产宿主应在每个独立请求、tick 或 job 前调用 `lua_runtime_begin_execution`。该调用只允许在 owner thread 且 State 未执行时发生；它会重新填充 instruction/native-work 预算、建立新的 monotonic deadline，并清除上一个窗口的取消标志。协程和 Lua→C→Lua 重入共享这一个窗口，不会隐式补充配额。

## 创建期配置

所有配置结构必须先由对应的初始化函数填充，再覆盖需要修改的字段。库会严格验证 `struct_size`、`api_version`、未知位和最低栈容量；不兼容结构会在 State 创建前返回确定的 `LUA_RUNTIME_ERR_*`，不会尝试猜测调用方布局。

`lua_runtime_config_init` 保持 Lua 5.1 兼容行为：全部库、全部能力和无限预算。它适用于可信本地脚本，不是面向不可信输入的安全默认值。

`lua_runtime_config_init_gameserver` 是有限预置：

| 分类 | 预置 |
|---|---|
| 标准库 | base、math、string、table、coroutine、package |
| 脚本能力 | filesystem/process/native modules/runtime compilation/binary chunks/GC control 全部关闭 |
| 初始执行窗口 | 10,000,000 VM 指令、64 MiB native work、1,024 finalizer、5 秒 |
| 字符串/输出/源码/Proto | 各 8 MiB |
| table array/hash、stack、sort elements | 各 250,000 |
| 返回值 | 100,000 |
| sort comparisons / pattern steps | 8,000,000 / 4,000,000 |
| reader pieces | 100,000 |
| 编译 | 250,000 tokens/AST/constants/instructions，4,096 functions，128 nesting |

预置是保守起点，不是所有业务的通用 SLO。宿主必须根据真实请求大小、延迟目标和并发度收紧或有依据地放宽字段，并在部署前进行压力与故障注入验证。

## 可信宿主与脚本边界

公开 `luaL_loadbuffer`、`luaL_loadstring` 和 `luaL_loadfile` 是可信宿主加载入口：它们仍执行资源与编译上限，但不会因为脚本的 runtime-compilation、binary-chunk 或 filesystem capability 被关闭而拒绝。脚本环境中的 `load`、`loadstring`、`loadfile`、`dofile` 仍由 sandbox 决定，game-server 预置不会发布这些函数。

这一区分允许宿主加载已经授权的任务，同时阻止任务在运行中继续生成或读取代码。它不授权把不可信路径直接传给宿主 `luaL_loadfile`；路径校验、内容来源和签名验证仍属于宿主职责。

## 跨线程取消

owner thread 可用 `lua_runtime_get_cancellation_handle` 取得 opaque handle。其他线程只可调用 `lua_runtime_request_cancellation`；不得在 foreign thread 使用 `lua_State*`、栈、GC 或配置 API。请求由 VM 指令检查点或原生 callback 的 `lua_checkexecution` 观察。

handle 不拥有 State，内部使用弱生命周期状态。State 已关闭后，迟到的取消请求是安全无操作；调用方仍须以 `lua_runtime_release_cancellation_handle` 释放 handle 本身。取消是协作式的：永久阻塞且不轮询的原生 callback 不能由 Lua VM 异步抢占。

## 请求指标

在 `lua_pcall` / `lua_resume` 返回且 State 空闲后，owner thread 可读取一致快照：

```c
lua_RuntimeMetrics metrics;
lua_runtime_metrics_init(&metrics);
if (lua_runtime_get_metrics(L, &metrics) == LUA_RUNTIME_OK) {
    emit_request_metrics(metrics.consumed_instructions,
                         metrics.consumed_native_work,
                         metrics.last_stop_reason);
}
```

快照包含 instruction/native-work 的初始值、余量和消费量，finalizer drain 预算，是否配置 deadline、取消标志，以及最近的 `NONE / INSTRUCTION_BUDGET / NATIVE_WORK_BUDGET / DEADLINE / CANCELLED` 停止分类。读取运行中的 State 返回 `LUA_RUNTIME_ERR_BUSY`，foreign thread 返回 `LUA_RUNTIME_ERR_THREAD`；指标不是并发采样器。

这些字段适合低基数请求日志和聚合指标。源码、脚本错误对象或用户数据不得直接作为无限基数 label；详细错误应进入受限日志。

## 明确不提供的边界

- 配置中的字符串、容器、编译和输出限制不是进程总内存硬上限；完整边界见 [内存合同](memory-contract.md)。
- sandbox 不是 OS 隔离。原生模块、宿主 callback、系统调用、文件描述符和进程级 CPU/RSS 必须由 worker process/container/Job Object/cgroup 等外层控制。
- `timeout_ms` 使用 monotonic clock，只能在 Lua VM 检查点或协作式原生轮询处生效。
- 单个 State 固定 owner thread；并发任务需要独立 State/worker，不能并发访问同一个 `lua_State*`。

内部 C++ `EngineContext` 合同见 [RuntimeServices](services.md)，脚本能力矩阵见 [SandboxPolicy](sandbox-policy.md)，执行停止语义见 [ExecutionPolicy](execution-policy.md)。
