---
status: current
verified_against: examples/production_worker.cpp; tests/production/worker_success.lua; tests/production/worker_instruction_limit.lua; tests/production/worker_resource_limit.lua; tests/production/worker_allocator_limit.lua; tests/production/worker_invalid_error.lua; tests/production/verify_worker_json.py; CMakeLists.txt; .github/workflows/ci.yml; src/lua_runtime.h; docs/runtime/memory-contract.md
last_checked: 2026-07-23
applies_to: untrusted-script worker topology, allocator and process limits, structured outcomes, deployment health, and rollback
---

# 不可信脚本生产部署合同

Lua State 内部的 sandbox、预算和资源策略必须与进程外层隔离同时使用。仓库提供 `lua_production_worker` 作为只依赖公开 C API 的参考宿主；它不是要求业务采用的 RPC 协议，而是把生产边界变成可编译、可执行的最低合同。

## 防护层次

| 层 | 参考实现 | 防护内容 | 不能替代 |
|---|---|---|---|
| 脚本能力 | `lua_runtime_config_init_gameserver` | 库面、filesystem/process/native module/runtime compilation/binary chunk/GC control | OS 权限、容器、恶意宿主 callback |
| 请求执行 | `lua_runtime_begin_execution` | VM instruction、native work、monotonic deadline、finalizer drain | 永久阻塞且不轮询的原生代码 |
| Runtime 资源 | `lua_RuntimeConfig` | string/output/source/Proto/table/stack/sort/pattern/compiler 上限 | 进程总 RSS/VA、第三方库与宿主分配 |
| Lua allocator | `quotaAllocate` 示例 | 由 `lua_Alloc` 接管路径的 live/peak 硬配额和关闭归零 | 尚未迁入 allocator 的宿主/运行时临时对象 |
| OS 进程 | Windows Job Object / Linux `RLIMIT_AS`、`RLIMIT_CPU`、`RLIMIT_NOFILE` | 进程地址空间/内存、CPU 时间、句柄数量的最后防线 | 宿主权限、容器身份与调度配额 |

任何一层不可用时，worker 默认失败关闭。macOS 不宣称 `RLIMIT_AS` 等价支持；生产部署应由受支持的容器/VM/监督器施加内存和 CPU 上限，或将参考 worker 的 OS 适配替换成已验证的平台机制。

## 参考 worker

构建与运行：

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target lua_production_worker

build/lua_production_worker `
  --process-memory-mb 512 `
  --allocator-memory-mb 64 `
  --instruction-budget 1000000 `
  --native-work-budget 8388608 `
  --timeout-ms 50 `
  --cpu-seconds 2 `
  --max-output-bytes 1048576 `
  request.lua
```

脚本文件由可信宿主读取，并在读取前检查创建期 source 上限；脚本环境没有文件系统能力。worker 在创建 State 前施加 OS 限制，自定义 allocator 在每次增长前检查配额，State 使用 game-server profile，并在执行前启动独立窗口。

每次运行只向 stdout 写一行 JSON：

```json
{"schema":1,"outcome":"success","lua_status":0,"runtime_status":0,"duration_us":123,"allocator_live_bytes":0,"allocator_peak_bytes":65000,"allocator_limit_bytes":67108864,"process_memory_limit_bytes":536870912,"instruction_budget":1000000,"native_work_budget":8388608,"timeout_ms":50,"consumed_instructions":812,"remaining_instruction_budget":999188,"consumed_native_work":256,"remaining_native_work_budget":8388352,"last_stop_reason":0,"message":""}
```

`outcome` 至少区分 `success`、`instruction_budget`、`native_work_budget`、`deadline`、`cancelled`、`resource_limit`、`allocator_limit`、`compile_error`、`runtime_error` 和宿主配置错误。执行分类与消费量来自 `lua_runtime_get_metrics`，错误字符串仅用于详情。非成功脚本结果返回非零进程码；运维系统应同时记录退出码和 JSON。

CTest 的 `production-contract` 标签在 Windows 与 Linux 运行四条路径：

```powershell
ctest --test-dir build -C Release -L production-contract --output-on-failure
```

同一标签还用包含任意高位字节的 Lua 错误对象验证 worker 始终输出一行可解析 JSON；
非 ASCII 字节编码为 `\u00XX`，不会把无效 UTF-8 直接写入日志。

- 有限正常任务成功，关闭后 allocator live bytes 为 0；
- 无限循环以 instruction budget 停止；
- 超大 `string.rep` 在 Runtime output policy 前置拒绝；
- 放宽 output policy 后，相同增长由 1 MiB `lua_Alloc` 硬配额拒绝，且关闭归零。

OS 进程限制是否真正阻止超额进程由平台机制负责；部署验收还必须在目标镜像中运行破坏性隔离测试，并确认监督器观察到预期 OOM/CPU 终止原因。

## 服务拓扑

- 一个 worker process 内只运行受控数量的独立 State；不要让互不信任的租户共享原生模块或宿主静态状态。
- 同一个 State 固定 owner thread，任务不得并发进入。并行度通过 State pool 或 worker pool 提供。
- watchdog 线程只能持有 `lua_CancellationHandle`；超时后先请求协作取消，超过 grace period 仍未退出则由监督器终止整个 worker。
- worker 不持有关键业务状态。输入、幂等键与结果提交由上层服务管理，以便强杀和重试。
- 文件系统能力保持关闭；确需读取的输入由可信宿主完成来源与大小校验后传入 Runtime。
- 禁止在处理不可信任务的 worker 中加载任意原生模块。需要原生扩展时固定允许清单、哈希/签名和镜像版本，并按扩展风险拆分 worker 池。

## 健康、告警与容量

最低指标：

- 请求量、成功率和按 `outcome` 分类的失败率；
- p50/p95/p99 duration 与 queue time；
- instruction/native-work/timeout 配额配置分布；
- allocator peak、进程 RSS/VA、worker 重启和 OS limit kill；
- compile/runtime/resource/sandbox 错误的限频聚合；
- State 创建/关闭计数与关闭后 allocator 非零事件；
- watchdog cancellation-to-stop 延迟。

容量测试应使用生产镜像和真实并发度，分别覆盖短任务、接近预算任务、编译错误、资源拒绝、取消、worker 强杀与重启。不要用单进程微基准代替端到端延迟和内存水位。

## 发布与回滚

候选版本以不可变镜像和 SDK 校验和发布。先在 shadow/canary worker 池验证结果一致性、错误分类、p99、RSS 与取消延迟，再分批扩大流量。至少保留上一个已验证镜像和兼容的任务协议；回滚通过流量切换与 worker drain 完成，不在运行中的 State 上热替换 Runtime。

若新版本改变公开 ABI、binary chunk、sandbox 默认值、错误分类或配置结构版本，必须显式升级协议并拒绝静默混用。SDK API 合同见 [生产运行时公开 C API](../runtime/public-runtime-api.md)，尚未被 `lua_Alloc` 完整覆盖的路径见 [内存合同](../runtime/memory-contract.md)。
