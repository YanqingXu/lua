---
status: current
verified_against: tests/soak/runtime_soak.cpp; tests/compatibility/public_native_module_host.cpp; tests/fuzz/; tests/fuzz/corpus/; CMakeLists.txt; .github/workflows/ci.yml; .github/workflows/nightly.yml
last_checked: 2026-08-13
applies_to: runtime soak, cancellation latency, native-module lifecycle, bounded PR fuzzing, and scheduled long fuzzing
---

# 长稳与 Fuzz 证据合同

快速 required checks 与长时间耐久验证承担不同职责。每个 PR 运行确定有界的 sanitizer fuzz smoke 和 20 轮 runtime soak；默认分支每天运行更长的 fuzz campaign、45 分钟 runtime soak 和 1,000 轮 native-module load/unload 生命周期。

## Runtime soak

`lua_runtime_soak` 只使用公开 C API，并为每轮创建两个独立 game-server State。每一轮验证：

- 两个 context 的全局表隔离；
- 16 次 coroutine create/yield/resume/complete；
- weak-value table 在宿主完整 GC 后清除不可达对象；
- 32 个 userdata `__gc` 恰好执行并可安全关闭；
- 由 foreign thread 通过 opaque handle 取消无限循环；
- cancellation-to-stop 不超过 250 ms；
- 每个 State 使用 64 MiB callback allocator 配额，关闭后 live bytes 必须归零。

快速门禁：

```powershell
ctest --test-dir build -C Release -L soak-smoke --output-on-failure
```

本地长跑：

```powershell
build\Release\lua_runtime_soak.exe `
  --iterations 0 `
  --duration-seconds 3600 `
  --max-cancel-latency-ms 250 `
  --json build\runtime-soak.json
```

JSON 证据包含迭代数、State 创建/关闭数、coroutine、weak value、finalizer、取消检查、最大取消延迟、allocator 峰值和总时长。任一不变量失败立即停止并写出 `status=failed` 与错误。

## Native module soak

`lua_public_module_host <module> [iterations]` 在同一进程中重复完整的双-context lease/cache、最后引用卸载、重新加载状态归零，以及 module-owned `__gc` 先于动态库卸载的合同。普通 CTest 使用 1 轮，nightly 使用 1,000 轮。

该测试只证明仓库 fixture 和平台 loader 路径；生产原生扩展还需各自的并发、静态状态、异常、ABI 与卸载安全验证。面向不可信脚本的默认 worker 应保持 native modules 关闭。

## Fuzz 分层

PR/push 的 `linux-fuzzers` 对 undump、bytecode verifier、parser 和标准库数值参数目标各运行 30 秒，并在 ASan+UBSan 下拒绝 crash、timeout、OOM 与 sanitizer 报告。该门禁适合快速回归，不构成长期稳定性证明。

`.github/workflows/nightly.yml` 默认对六个目标各运行 600 秒，`workflow_dispatch` 最多可提高到
每目标 1200 秒。六目标顺序执行的最大 campaign 为 120 分钟；job timeout 为 160 分钟，另留
40 分钟用于依赖安装、configure/build、证据写入、上传和 runner 抖动，合同测试要求额外预算
不得低于 30 分钟。每个目标使用独立可增长 corpus，并保留 final stats、完整日志、扩展 corpus
和 crash artifact 30 天。发布候选应至少完成一次与候选 SHA 对应的 campaign，不得用其他提交
的 artifact 替代；release verifier 仍要求每目标至少 600 秒，未因 timeout 调整而放松。

## Sanitizer 与进程边界

ASan/TSan 运行时需要预留大块影子地址空间，与 production worker 故意设置的低
`RLIMIT_AS` 不兼容。CI 因此只在这两个 sanitizer 配置中排除
`production-contract` 标签；UBSan 和所有非 sanitizer Linux/Windows 配置仍运行完整
worker 合同。该排除不会改变 worker 二进制，也不会放松普通构建的失败关闭行为。

## 发布判定

生产候选必须同时满足：

- 当前 SHA 的 required PR/push 矩阵全绿；
- 当前 SHA 最近一次 nightly runtime/native-module soak 全绿；
- 当前 SHA 长 fuzz 无 crash/sanitizer/timeout；
- cancellation latency、allocator peak 和进程 RSS 没有相对已发布版本出现无法解释的漂移；
- 所有失败 artifact 已归因、最小化并加入 corpus/回归测试，或明确阻止发布。

nightly 是持续证据，不等于一次 24–72 小时业务压测。正式大流量上线仍需在生产镜像、目标硬件、真实任务分布和 worker 监督器下完成独立 soak/canary。
