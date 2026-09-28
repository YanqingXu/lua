---
status: current
verified_against: tests/soak/runtime_soak.cpp; tests/compatibility/public_native_module_host.cpp; tests/fuzz/; tests/fuzz/corpus/; CMakeLists.txt; .github/workflows/ci.yml; .github/workflows/nightly.yml
last_checked: 2026-09-28
applies_to: runtime soak, cancellation latency, native-module lifecycle, bounded PR fuzzing, and scheduled long fuzzing
---

# 长稳与 Fuzz 证据合同

快速 required checks 与长时间耐久验证承担不同职责。每个 PR 运行确定有界的 sanitizer fuzz smoke 和 20 轮 runtime soak；默认分支每天运行更长的 fuzz campaign、45 分钟 runtime soak 和 1,000 轮 native-module load/unload 生命周期。

## Nightly 执行环境

GitHub 继续负责 cron 调度、手动触发、日志和 artifact 存储；计算在本地自建 runner 上执行。
cron `31 18 * * *` 为每天 UTC 18:31（北京时间次日 02:31），实际触发可能延迟。
任务仅在 `YanqingXu/lua` 的 `main` 分支运行，手动触发也须选择 `main`。
PR/push CI 和 release 工作流继续使用各自原有的 runner。

所需的两个 runner 均注册到本仓库，添加自定义标签 `lua-nightly`：

| 标签 | 用途 | 预装工具 |
| --- | --- | --- |
| `self-hosted, Linux, X64, lua-nightly` | runtime/native-module soak、Linux fault matrix、long fuzz | Ubuntu 24.04 x64（支持 WSL 2）、Git、CMake、GCC/G++、Python 3（含 `python` 命令）、PowerShell 7、Clang 19、`libfuzzer-19-dev` |
| `self-hosted, Windows, X64, lua-nightly` | Windows fault matrix | Git、CMake、Visual Studio C++ 工具链、Python 3（`python` 命令）、PowerShell 7 |

runner 使用独立的工作目录，不能指向开发者的源码工作区。依赖由机器管理员预先安装；
workflow 不执行 `sudo apt-get`，缺少 Clang/libFuzzer 时会失败并显示诊断。
Windows 服务账户的 PATH 必须能访问真实的 Python 3 安装，不能依赖用户的 WindowsApps 别名。
每个 runner 同时只执行一个 job；只有一个 Linux runner 时，三个 Linux job 排队执行，
默认整轮约需 105 分钟再加编译时间。可增加带有同样标签的 Linux runner 提高并发。

部署顺序：

1. 在仓库 Settings → Actions → Runners 中注册 Windows 和 Linux runner，添加 `lua-nightly` 标签。
2. 配置 Windows runner 服务及 Linux runner 服务；使用 WSL 时还须配置当前 Windows 用户登录后
   启动该发行版并保持 WSL 会话运行。仅启用 Linux systemd 服务不足以保证 WSL 持续在线。
3. 在 GitHub 确认两台 runner 都为 **Idle**，核对上述标签，然后合并本地 runner 配置。
4. 对 `main` 手动触发 Nightly endurance，确认四个 job 的 runner 名称及四份 artifact。

注册命令与对应平台的下载包、校验值可从
[GitHub 的 New self-hosted runner 页面](https://github.com/YanqingXu/lua/settings/actions/runners/new)
获取。使用独立的 Windows 安装目录与 WSL Linux 安装目录；不要复用其他仓库的 runner 目录。
注册时设置不同名称（例如 `lua-nightly-windows` 和 `lua-nightly-linux`），都添加 `lua-nightly`
标签，并使用各自默认的 `_work` 目录。Windows runner 选择安装为服务时需要管理员 PowerShell。

Ubuntu 依赖安装示例（由管理员执行，不能放入 workflow）：

```bash
sudo apt-get update
sudo apt-get install -y git cmake build-essential python3 python-is-python3 clang-19 libfuzzer-19-dev
```

PowerShell 7 按 [Microsoft 的 Ubuntu 安装说明](https://learn.microsoft.com/powershell/scripting/install/install-ubuntu)
预装，确保 `pwsh --version` 成功。Linux runner 注册完成后，在其安装目录运行
`sudo ./svc.sh install <runner-user>` 和 `sudo ./svc.sh start`，其中 `<runner-user>` 是实际 Linux 用户。

WSL 还需要 Windows 登录启动任务。以下命令在拥有该 WSL 发行版的用户会话中执行；
若系统要求提升权限，使用同一用户的管理员 PowerShell。发行版名称须与 `wsl --list --quiet` 一致。
该任务保持 Linux 会话在线，已启用的 runner systemd 服务随发行版启动：

```powershell
$action = New-ScheduledTaskAction -Execute "$env:WINDIR\System32\wsl.exe" `
    -Argument '-d Ubuntu-24.04 -- /bin/sleep infinity'
$user = [Security.Principal.WindowsIdentity]::GetCurrent().Name
$trigger = New-ScheduledTaskTrigger -AtLogOn -User $user
$principal = New-ScheduledTaskPrincipal -UserId $user -LogonType Interactive -RunLevel Limited
$settings = New-ScheduledTaskSettingsSet -ExecutionTimeLimit ([TimeSpan]::Zero) `
    -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries `
    -RestartCount 3 -RestartInterval (New-TimeSpan -Minutes 1)
Register-ScheduledTask -TaskName 'Lua nightly WSL runner' -Action $action `
    -Trigger $trigger -Principal $principal -Settings $settings
Start-ScheduledTask -TaskName 'Lua nightly WSL runner'
```

执行期间电脑必须保持开机、联网且不休眠；WSL 登录启动方案还要求用户保持登录。
本地 runner 离线时任务会排队，不会自动回退到 GitHub 托管 runner。
runner 注册凭据留在机器本地，不得提交到仓库。

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
