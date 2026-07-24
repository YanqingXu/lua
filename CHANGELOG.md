# Changelog

本文件记录面向 SDK 使用者的可见变化。版本遵循 Semantic Versioning；`0.x` 阶段仍可能调整扩展 API，但任何 ABI 或配置结构变化都必须显式记录。

## [Unreleased]

### Added

- 安装 `lua_runtime.h`，提供版本化创建配置、有限 game-server 预置、每请求执行窗口、生命周期安全取消句柄和只读治理指标。
- 静态/共享安装消费者实际验证 sandbox、资源限制、指令预算、取消与 metrics。
- `lua_production_worker` 参考宿主，组合 callback allocator 配额与进程级 CPU/内存边界，并输出单行 JSON 结果。
- runtime/coroutine/weak-table/finalizer/cancellation/multi-context soak 与 native-module 重复生命周期驱动。
- scheduled 长 fuzz、组件覆盖率硬阈值和 Release benchmark 绝对 SLO。
- RC SDK 打包、SHA-256、SPDX 2.3 SBOM、发布前检查和失败关闭的治理开关。

### Changed

- 公开 `luaL_loadbuffer`、`luaL_loadstring` 与 `luaL_loadfile` 明确作为可信宿主 loader：不受脚本 capability 关闭影响，但仍受资源和编译上限约束。
- game-server State 可以在创建前一次性应用 sandbox、执行、资源与编译策略。

### Fixed

- 脚本 sandbox 收紧后，可信宿主仍可载入已经授权的任务源码。
- worker 将任意错误字节编码为合法 JSON 转义，不会把无效 UTF-8 直接写入结构化结果。
- 发布页保留 manifest 引用的平台校验文件，SPDX package verification code 按规范独立重算。

## [0.1.0] - Unreleased

首个 Runtime Preview SDK。只有在候选 SHA 的全部 required checks、长稳/长 fuzz、制品验证和仓库治理条件满足后才创建 RC/正式 tag。
