---
status: current
verified_against: examples/production_worker.cpp; tests/production/worker_success.lua; tests/production/worker_instruction_limit.lua; tests/production/worker_native_work_limit.lua; tests/production/worker_resource_limit.lua; tests/production/worker_allocator_limit.lua; tests/production/worker_compile_error.lua; tests/production/worker_runtime_error.lua; tests/production/worker_invalid_error.lua; tests/production/verify_worker_json.py; tests/production/verify_worker_fault_matrix.py; CMakeLists.txt; .github/workflows/ci.yml; .github/workflows/nightly.yml; src/lua_runtime.h; docs/runtime/memory-contract.md
last_checked: 2026-07-26
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
{"schema":1,"outcome":"success","lua_status":0,"runtime_status":0,"duration_us":123,"allocator_live_bytes":0,"allocator_peak_bytes":65000,"allocator_limit_bytes":67108864,"process_memory_limit_bytes":536870912,"process_limits_enabled":true,"instruction_budget":1000000,"native_work_budget":8388608,"timeout_ms":50,"consumed_instructions":812,"remaining_instruction_budget":999188,"consumed_native_work":256,"remaining_native_work_budget":8388352,"last_stop_reason":0,"cancellation_requested":0,"message":""}
```

`outcome` 至少区分 `success`、`instruction_budget`、`native_work_budget`、`deadline`、`cancelled`、`resource_limit`、`allocator_limit`、`compile_error`、`runtime_error` 和宿主配置错误。执行分类与消费量来自 `lua_runtime_get_metrics`，错误字符串仅用于详情。非成功脚本结果返回非零进程码；运维系统应同时记录退出码和 JSON。

CTest 的 `production-contract` 标签在 Windows 与 Linux 运行 OS 限制安装和基础 worker 路径：

```powershell
ctest --test-dir build -C Release -L production-contract --output-on-failure
```

同一标签还用包含任意高位字节的 Lua 错误对象验证 worker 始终输出一行可解析 JSON；
非 ASCII 字节编码为 `\u00XX`，不会把无效 UTF-8 直接写入日志。

- 有限正常任务成功，关闭后 allocator live bytes 为 0；
- 无限循环以 instruction budget 停止；
- 超大 `string.rep` 在 Runtime output policy 前置拒绝；
- 放宽 output policy 后，相同增长由 1 MiB `lua_Alloc` 硬配额拒绝，且关闭归零。

独立的 `runtime-failure-contract` 使用严格 Python 驱动器覆盖十二种结果：success、任务先完成时
解除 watchdog cancellation、instruction budget、native-work budget、deadline、跨线程
cancellation、Runtime resource limit、allocator
rejection、compile error、runtime error、非法错误字节和宿主配置错误。驱动器同时校验单行
ASCII-safe JSON、字段类型、exit code/outcome/stop reason 一致性、预算消费、取消标志、allocator
peak 和关闭归零。它显式使用
`--skip-process-limits-for-tests`，因此不会让 ASan/TSan 的 shadow address space 与低
`RLIMIT_AS` 冲突；该选项只允许用于测试，不能出现在生产 worker 命令行。

```powershell
ctest --test-dir build -C Release -L runtime-failure-contract --output-on-failure
```

scheduled nightly 在 GitHub-hosted Windows/Linux runner 上再次启用 OS limit，运行同一矩阵并保存
包含 exact candidate SHA 的 `worker-fault-matrix.json`。它证明仓库参考宿主在这些 runner 上能
安装限制并保持结构化结果，但不等价于目标镜像、业务监督器或真实 OOM/CPU kill 验收。

## 目标环境故障注入

把候选 worker 和不可变镜像部署到与生产一致的 Windows/Linux 节点后，先运行可协作返回的矩阵：

```powershell
python tests/production/verify_worker_fault_matrix.py `
  --worker /opt/lua/bin/lua_production_worker `
  --scripts tests/production `
  --enforce-process-limits `
  --output worker-fault-matrix.json
```

保存候选 SHA、SDK checksum、镜像 digest、OS/运行库、CPU、内存、worker 参数、并发度和驱动器输出。
仓库矩阵不能自动证明以下目标环境合同，必须由真实监督器补充破坏性演练：

- 将 CPU/RSS/进程限制压到会由 OS 强制终止的水平；被强杀的进程通常无法写最终 worker JSON，
  因而监督器必须保存原始 exit/signal/Job Object 原因，并稳定映射为 `os_cpu_kill`、
  `os_memory_kill` 或 `operator_kill`，不能伪装成 Runtime outcome；
- 在任务执行中强杀 worker，验证 drain grace、监督器重启、幂等键去重和流量重试；随后用探针任务
  证明新进程没有继承前一请求的全局表、原生模块状态或临时文件；
- 在真实 State pool 中连续执行成功、失败、取消、成功序列，验证每次
  `lua_runtime_begin_execution` 重置预算和取消状态，并在 pool 淘汰/关闭后检查 allocator live
  bytes 为 0；
- 需要原生扩展的可信 worker 池单独执行固定哈希模块的 load/use/unload 和重复生命周期测试。
  默认 game-server worker 禁止任意原生模块，不能为完成演练而放宽该安全默认值；
- macOS 由外层容器/VM/监督器执行等价 CPU/RSS/强杀合同，并明确记录它不宣称参考 worker 的
  `RLIMIT_AS` 硬边界。

每种故障至少记录样本数、p50/p95/p99、RSS、allocator peak、cancellation-to-stop、重启恢复时间和
错误分类计数。只有目标监督器的 kill reason、重试/去重结果和恢复探针齐全时，才能将 RC-03 记为
完成。

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

## Shadow、canary 与回滚

候选版本以不可变镜像 digest 和 SDK checksum 发布。开始前冻结候选/基线版本、任务协议、结果归一化
规则、SLO、最小样本数、观察窗口、停止阈值和负责批准/回滚的人员；至少保留上一个已验证镜像。

shadow 阶段复制真实任务分布，但候选结果不得提交外部副作用。按任务 ID 比较：

- 返回值或业务摘要（先归一化时间、随机数、地址和无序字段等已声明的非确定数据）；
- success/compile/runtime/resource/cancel/OS-kill 分类及 Lua status；
- instruction/native-work 消费、duration、allocator peak 和进程 RSS。

任何未归因语义差异、崩溃、allocator 非零或错误分类漂移都停止进入 canary。确认 shadow 通过后，
canary 从健康检查但无业务流量的池开始，再按预先批准的阶梯（例如 1% → 5% → 25% → 50% →
100%）扩大；每阶必须同时满足最小样本和观察时间，比较 p50/p95/p99、queue time、RSS、
allocator peak、取消延迟、失败分类和 worker restart。百分比只是示例，实际阶梯应按故障域和业务
风险批准，不能用仓库测试代替。

以下任一事件立即停止扩流并回滚：未归因结果差异、错误分类或协议漂移、SLO 越线、crash/leak、
allocator 关闭非零、重启风暴、队列持续增长或监督器无法稳定解释 OS kill。回滚顺序是：

1. 冻结扩流并将新请求路由回上一不可变版本；
2. 对候选池执行有上限的 drain，超过 grace 的任务按幂等合同强杀并重试；
3. 验证基线池的成功率、队列、p99、RSS 和失败分类恢复；
4. 保存每个阶段的版本/digest、配置、流量比例、时间窗、样本、指标、差异、停止原因和恢复时间；
5. 由明确的发布负责人形成书面 go/no-go，未通过项进入新候选，不移动旧 tag。

不在运行中的 State 上热替换 Runtime。仓库内 fault matrix、CI 和 nightly 只提供进入 shadow 的前置
证据；24–72 小时真实任务 soak、无副作用 shadow、分阶段 canary 和实际 drain/rollback 必须在业务
环境完成。

若新版本改变公开 ABI、binary chunk、sandbox 默认值、错误分类或配置结构版本，必须显式升级协议并拒绝静默混用。SDK API 合同见 [生产运行时公开 C API](../runtime/public-runtime-api.md)，尚未被 `lua_Alloc` 完整覆盖的路径见 [内存合同](../runtime/memory-contract.md)。
