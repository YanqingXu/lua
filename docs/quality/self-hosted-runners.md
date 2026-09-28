---
status: current
verified_against: .github/workflows/ci.yml; .github/workflows/nightly.yml; .github/workflows/release.yml; docs/release/platform-baseline.json; lua_test.vcxproj; lua.vcxproj
last_checked: 2026-09-28
applies_to: repository self-hosted GitHub Actions runner setup and operation
---

# 本机 GitHub Actions runner

CI、Nightly 和 Release 的全部 job 均配置为 `self-hosted`。GitHub 仍负责触发、调度、
日志和 artifact 存储；构建与测试在注册到本仓库的机器上执行。完整测试矩阵保留，缺少匹配
runner 或 runner 离线时任务等待调度，不会回退到 GitHub 托管的 Azure runner。
修改工作流并不等于已部署：须将配置应用到 GitHub，并注册、启动对应 runner，才能验证实际执行。

## 标签与执行环境

同一行的全部标签必须同时匹配；`Windows`、`Linux`、`macOS` 和 CPU 架构须与实际环境一致。

| 用途 | runner 标签 | 实际环境 |
| --- | --- | --- |
| Windows CI | `self-hosted, Windows, X64, lua-ci` | Windows x64、Visual Studio 2026（MSVC v145） |
| Linux CI、Release 证据验证及发布编排 | `self-hosted, Linux, X64, lua-ci` | Ubuntu 24.04 x64；可使用 WSL 2 |
| Linux ARM64 portability | `self-hosted, Linux, ARM64, lua-ci` | 独立的 Linux ARM64 原生或仿真环境，GCC 14 |
| macOS ARM64 portability | `self-hosted, macOS, ARM64, lua-ci` | Apple Silicon Mac，AppleClang |
| Windows Nightly | `self-hosted, Windows, X64, lua-nightly` | Windows x64 |
| Linux Nightly | `self-hosted, Linux, X64, lua-nightly` | Ubuntu 24.04 x64；可使用 WSL 2 |
| Windows SDK 打包及消费验证 | `self-hosted, Windows, X64, lua-release, windows-2022` | 兼容 Windows x64，主机版本至少 `10.0.20348`，满足发布基线 |
| Linux SDK 打包及消费验证 | `self-hosted, Linux, X64, lua-release, ubuntu-24.04` | Ubuntu 24.04 x64，满足发布基线 |
| macOS SDK 打包及消费验证 | `self-hosted, macOS, ARM64, lua-release, macos-15` | macOS 15 ARM64，满足发布基线 |

CI allocator check 名称保留 `ubuntu-latest` / `windows-latest`，以维持既有 check 身份；
实际调度使用独立的 self-hosted 标签。Release 的 `windows-2022`、`ubuntu-24.04`、`macos-15`
同时是发布基线标识和自定义标签，不能仅给不合格机器添加标签来满足合同。具体 OS、编译器、
glibc 和部署目标要求见 [发布平台基线](../release/platform-support.md)。
Windows 的最低运行基线是 Windows Server 2022；构建主机校验接受版本至少 `10.0.20348`
且满足其余工具链与运行库约束的 Windows，不要求产品名称必须为 Windows Server 2022。

一台满足条件的 runner 可同时添加 `lua-ci`、`lua-nightly` 或 `lua-release` 标签，
但每个 runner 同时只执行一个 job。Windows x64 主机不能通过添加 Linux、macOS 或 ARM64
标签替代这些环境。WSL 的 Linux runner 必须安装在对应 Linux 发行版内；macOS 任务需要 Mac。
暂未配置的平台保持等待，不删减对应测试。

## 预装工具

依赖由管理员预先安装；workflow 检查并使用已有工具。安装后以 **runner 实际运行账户**
确认工具在 PATH 中可见，再重启 runner 服务。

- 所有环境：Git、CMake 3.20+、Python 3、PowerShell 7（`pwsh`）；使用 Ninja 的任务须安装 Ninja。
- Windows CI：Visual Studio 2026 的 C++ 桌面开发工具、MSBuild、MSVC v145、Windows SDK，
  与现有 `.vcxproj` 工具集一致。Windows Release 另需 Visual Studio 2022、MSVC v143（19.40+）；
  发布工作流显式选择 VS 2022。承担两种用途时须并行安装对应工具链。
  `python` 必须指向真实 Python 3，不能依赖 WindowsApps 的商店别名。
- Windows MSBuild CI 使用 Windows PowerShell 5.1；job 将 `PSModulePath` 置空，让 shell
  使用自身的内置模块。否则由 PowerShell 7 启动的 runner 可能把 7.x 模块路径传给 5.1，
  导致 `Get-FileHash` 等自带命令无法加载。不要通过跳过源码完整性检查来规避此问题。
- Linux x64：GCC/G++ 14、Clang 18 与 libc++/libc++abi 18、LLVM 18、Clang 19 与 libFuzzer、
  clang-format、clang-tidy、pkg-config、Lua 5.1 解释器和开发库。
- Linux ARM64：GCC/G++ 14 和通用构建工具，均须在 ARM64 环境内可用。
- macOS ARM64：Xcode/Command Line Tools、Ninja 和通用工具；发布包要求 AppleClang 16.x–17.x，
  并保留 `CMAKE_OSX_DEPLOYMENT_TARGET=14.0`。
- Release：Linux 编排 runner 及三个平台的 `lua-release` runner 都需要 GitHub CLI（`gh`）。

Ubuntu 24.04 x64 依赖示例（管理员执行）：

```bash
sudo apt-get update
sudo apt-get install -y git cmake ninja-build build-essential gcc-14 g++-14 \
  python3 python-is-python3 pkg-config clang-18 libc++-18-dev libc++abi-18-dev \
  llvm-18 clang-19 libfuzzer-19-dev clang-format clang-tidy lua5.1 liblua5.1-0-dev
```

安装 `gcc-14` / `g++-14` 不会自动切换 Ubuntu 的默认编译器。须为 runner 账户配置默认
`gcc`、`g++`、`cc`、`c++` 使用 GCC 14，并分别检查其 `--version`；CI 的部分任务使用无版本
命令或 CMake 默认编译器，仅安装带版本号的可执行文件不足以完成配置。可在 runner 专用工具
目录中创建对应链接，并置于 runner 服务 PATH 的最前端，无须修改系统默认编译器。

PowerShell 7 按 [Microsoft 安装说明](https://learn.microsoft.com/powershell/scripting/install/install-ubuntu)
预装；GitHub CLI 按 [官方安装说明](https://github.com/cli/cli/blob/trunk/docs/install_linux.md)
预装。以 runner 账户确认 `pwsh --version`、`python --version`、`gh --version`。
工作流中的 `gh` 使用 GitHub 提供的任务令牌，无需在 runner 上保存个人 GitHub 登录凭据。

## 注册与验收

1. 在 [本仓库 Settings → Actions → Runners](https://github.com/YanqingXu/lua/settings/actions/runners)
   选择 **New self-hosted runner**，按实际操作系统和架构获取下载、校验与注册命令。
2. 为本仓库创建独立安装目录和工作目录，不指向开发者源码工作区，不复用其他仓库的 runner。
   使用清晰且唯一的名称，例如 `lua-windows-x64`、`lua-linux-x64`；保留默认 OS/架构标签，
   按表添加用途标签。临时注册令牌和 runner 凭据仅保存在机器本地，不提交到仓库。
3. 配置 runner 持续运行。Windows 安装服务需要管理员 PowerShell；无管理员权限时，可用当前
   用户的交互式计划任务执行 `run.cmd`，设置登录触发、隐藏窗口、失败重启及不限运行时长。
   启动脚本须明确设置工具 PATH；该方式要求用户保持登录，注销后 runner 将离线。
   Linux 在 runner 目录中执行
   `sudo ./svc.sh install <runner-user>` 和 `sudo ./svc.sh start`。Windows 服务使用可访问所需
   工具的账户。WSL 还需启动发行版并保持会话在线，具体步骤见 [Nightly 执行环境](endurance.md#nightly-执行环境)。
4. 在 GitHub 确认 runner 为 **Idle**，核对标签；将工作流改动应用到仓库后，使用可信分支提交
   触发 CI。展开各 job 的 **Set up job**，检查 runner 名称、OS/架构及测试结果。
5. Nightly 按其文档对 `main` 手动验收。Release 仅在满足既有发布治理与证据要求时执行，
   不因测试 runner 而绕过发布门禁。未注册环境应显示等待匹配 runner，不能算作验证通过。

保留 runner 自动更新，并维护为兼容工作流中固定版本 Actions 的最新 runner 版本。
电脑必须保持开机、联网且不休眠；采用登录启动的 WSL 方案时还须保持对应用户登录。
当前 CI 的 `if` 条件仅允许同仓库分支的 PR 自动执行；这只是工作流内的限制，PR 本身也能修改
工作流，不能将该条件当作完整安全边界。维护者应在仓库 Actions 设置中要求批准外部贡献者的
工作流运行。外部贡献须先审查，再由维护者将可信改动放入本仓库分支进行完整测试。
