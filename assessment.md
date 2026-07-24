# 当前项目进展评估

本评估以 2026-07-24 当前候选变更和实际执行结果为准。`5fd447e` 是本轮生产化工作的
基线提交，不是可发布候选 SHA；历史 Actions 全绿记录只能证明旧基线，不能替代候选
分支最终提交的同 SHA 复核。

当前最准确的项目定位是：

> 现代 C++23 Lua 5.1 高兼容 Runtime Preview，生产宿主所需的公开配置、执行边界、
> 资源与进程兜底、观测、耐久测试及发布制品链已经落地并通过 Windows 本地门禁；
> 在当前改动取得全平台 CI、nightly 和仓库治理批准前，仍是“生产候选”，不是
> “已批准上线”。

## 一、结论概览

| 维度 | 当前证据 | 判断 |
|---|---|---|
| Lua 5.1 C API | 官方 123/123 PASS，0 XFAIL，0 UNSUPPORTED | 兼容合同完整 |
| 项目公开面 | 143 函数、61 宏、55 枚举常量、15 typedef | 静态/共享导出和 C/C++ 编译合同完整 |
| 完整测试基线 | 790 tests / 6752 assertions / 0 failures | Windows Release 全绿 |
| 生产配置 API | 已安装 `lua_runtime.h`，ABI 版本化结构 | 可由纯 C 宿主配置 |
| 执行治理 | instruction/native budget、deadline、取消、finalizer budget、owner-thread | 可按请求开启新执行窗口 |
| 观测 | 预算初值/余量/消耗、deadline、取消与停止原因 | 可形成稳定结果分类 |
| 脚本能力 | game-server preset 与显式库/能力位 | 默认兼容行为和受限行为分离 |
| 内存边界 | callback allocator 配额 + worker 进程边界 | 有双层兜底；不宣称所有内部临时容器都走 callback allocator |
| 耐久性 | 1000 轮 runtime soak + 1000 次 native-module 生命周期 | 本地长跑通过，远端定时长跑待执行 |
| 覆盖率 | 六个组件的硬阈值和失败合同已实现 | 新阈值尚待当前 SHA 的 coverage job 证明 |
| 性能 | 10 项绝对 SLO + 原有相对回归门 | Windows 本地绝对 SLO 通过 |
| SDK 制品 | 静态/共享 CMake consumer、SPDX 2.3 SBOM、SHA-256、归档验证器 | Windows 候选包已自校验并从归档复测 |
| 发布治理 | tag 路径默认失败关闭，要求明确批准 | 未批准前不得创建 RC tag |

## 二、已完成的生产化改动

### 1. 公开、版本化的生产配置面

新增安装头 `src/lua_runtime.h`，提供：

- `lua_RuntimeConfig`、unrestricted 初始化和 game-server preset；
- 标准库、脚本能力、资源上限和编译上限；
- `luaL_newstate_configured` / `lua_newstate_configured`；
- `lua_runtime_begin_execution`，为每个请求重置预算、deadline 与取消状态；
- 生命周期安全的跨线程 cancellation handle；
- `lua_runtime_get_metrics`，在 owner thread 且 State idle 时取得一致快照。

默认 `lua_open` / `luaL_newstate` 继续保持 Lua 5.1 的 unrestricted 行为，生产限制必须由
宿主显式选择。宿主调用 `luaL_loadbuffer` 等 loader 被视为可信装载路径，仍受资源与编译
上限约束，但不会被脚本侧 runtime-compilation 能力位误伤。

### 2. 双层资源与故障边界

`lua_production_worker` 只依赖公开 C API，并组合：

- callback allocator hard cap；
- Windows Job Object 或 Linux rlimit 的进程级 CPU/内存边界；
- instruction、native work、monotonic timeout 和输出限制；
- success、instruction budget、resource limit、allocator limit 等稳定结果分类；
- 单行 JSON 指标与 State 关闭后的 allocator 归零证据。

worker 在无法安装要求的进程边界时失败关闭。macOS 的进程硬内存边界仍由部署环境提供，
这项限制已写入候选说明。

### 3. 耐久测试与长任务

`lua_runtime_soak` 每轮覆盖两个隔离 context、coroutine resume/yield、weak value 回收、
finalizer、跨线程取消、allocator quota 和关闭归零。本地 1000 轮结果：

- 2000 State 创建并关闭；
- 16000 次 coroutine cycle；
- 1000 个 weak value 回收；
- 32000 次 finalizer；
- 1000 次 cancellation check；
- 最大取消延迟 2092 µs；
- allocator peak 128718 bytes；
- 0 错误。

同一构建另完成 1000 次 native-module load/use/unload 生命周期。新增 nightly workflow
每天运行 45 分钟 runtime soak、1000 次 native-module 生命周期和四个目标各 600 秒
的 sanitizer fuzz，并保存证据 artifact。

### 4. 覆盖率与性能硬门槛

组件覆盖率不再只有总报告，以下最低行覆盖率会直接令任务失败：

- bytecode verifier 84%；
- C API 83%；
- GC phases 86%；
- opcode handlers 84%；
- parser/codegen 90%；
- sandbox denied paths 76%。

benchmark 在原有同 runner、交错 base/head 的相对回归判断外，增加 10 项绝对 SLO。
本地 Release 结果包括：VM 80.0M instructions/s、C++→Lua 283 ns/call、
Lua→C++ 180 ns/call、coroutine round-trip 328 ns、GC p99 0.6 µs、GC max 1.8 µs，
全部通过绝对门槛且关闭后 allocator live bytes 为 0。

### 5. 可验证发布制品

发布工具现在从 CMake install tree 生成平台 zip、外部 SPDX 2.3 JSON SBOM、SHA-256
清单和 manifest。归档验证器会拒绝：

- 越出单一归档根目录的成员；
- 缺失的库、五个公开头或 CMake package 文件；
- manifest 与文件名、版本、平台或提交不一致；
- 被修改或漏列的 checksum；
- 包内外 SBOM 不一致；
- SBOM 文件全集、逐文件 SHA-256、CONTAINS 关系或 SPDX `packageVerificationCode`
  不一致。

本地 Windows 候选包包含 18 个文件，zip 为 5,843,559 bytes；归档自校验通过。将该
zip 解压到全新目录后，独立静态与共享纯 C consumer 均重新配置、链接并执行通过。

### 6. 发布流程与操作合同

仓库已增加：

- RC checklist、候选说明、CHANGELOG 和 Runtime 报告策略；
- 生产 worker 的容量、观测、故障分类和回滚合同；
- Windows/Linux/macOS 候选包矩阵；
- tag 发布前的治理批准开关；
- 不可变候选、checksum、SBOM 和 release environment 流程。

手动 workflow dispatch 只生成验证包；tag 路径在治理未批准时必定失败，不会误发布。

## 三、本地验证证据

当前工作树已实际完成：

- 根 MSBuild Release：0 warnings / 0 errors；
- CMake Release 全量构建；
- 790 tests / 6752 assertions / 0 failures；
- 27/27 CTest，包括静态/共享 API consumer、安装后 consumer、差分、native module、
  production worker 四类结果、质量合同、soak smoke、examples 与 benchmark；
- Lua 5.1 official strict `all.lua` PASS；
- slow `sort.lua` / `verybig.lua` PASS；
- 123/123 官方 C API 合同；
- strict quality gate，包括 clang-format、clang-tidy smoke、C-style、opcode、文档漂移、
  MSBuild 与完整单测；
- 1000 轮 runtime soak 和 1000 次 native-module 生命周期；
- benchmark 绝对 SLO；
- 发布 readiness、SBOM/归档篡改拒绝合同；
- Windows 候选 zip 自校验与解压后静态/共享 consumer。

## 四、尚未满足的生产批准条件

### 1. 当前改动尚无同 SHA 全平台证据

本次生产化改动尚未取得候选分支最终提交的全平台证据。Windows 本地证据不能代替
Linux、macOS、ARM64、sanitizer、fuzz、coverage、lint、相对 benchmark 和 nightly
的同 SHA 结果。提交并推送后必须等待全部 required jobs 与首轮 nightly 通过；失败项
应修复后重新生成完整证据，不能只重跑到绿色。

### 2. 仓库治理决策仍未完成

既定发布标准要求默认分支和 tag 的可审计保护。当前仓库套餐不能配置原计划的规则，
因此只能在以下三项中明确选择一项：

1. 调整仓库能力以启用规则；
2. 改变仓库可见性以启用规则；
3. 由所有者给出限时、书面、可审计的 RC 豁免。

在决定完成并设置 `LUA_RELEASE_GOVERNANCE_APPROVED=true` 前，不创建 RC tag。

### 3. 目标环境仍需验证

Windows 本地已经验证 worker 的成功、instruction、resource 与 allocator 四条路径。
Linux 进程边界、macOS 外层内存边界以及实际服务镜像仍须在目标环境执行同样的失败注入，
并完成 shadow/canary、容量曲线、p99、allocator peak、进程内存、取消延迟和回滚演练。

### 4. callback allocator 不是全运行时声明

AST/codegen、部分标准库、diagnostic、debug/trace、loader 元数据等仍有不经过
`lua_Alloc` 的宿主侧临时存储。当前生产方案通过 worker 进程边界提供最终兜底，因此可以
作为隔离 worker 候选；但不能把 SDK 单独描述成“所有内存都受 callback allocator
hard limit”。该限制在公开文档和候选说明中保留。

## 五、发布顺序

1. 将当前改动形成可审查提交并推送；
2. 取得当前 SHA 的全部 CI、coverage、benchmark 与 nightly 证据；
3. 完成仓库治理选择；
4. 手动生成三个平台候选包并复核 checksum、SBOM 与归档 consumer；
5. 只在上述条件全部满足后创建不可变 `v0.1.0-rc.N` tag；
6. 完成目标环境 shadow/canary 和回滚演练；
7. 修复进入新提交与新 RC，不覆盖旧 tag 或旧制品。

## 最终判断

- 现代 C++ 教学解释器：约 95%；
- Lua 5.1 源码与 C API 高兼容实现：约 93%；
- 可消费 Runtime Preview SDK：约 92%；
- 隔离式游戏服务器生产候选代码：约 87%；
- 实际生产发布批准：尚未取得。

与上一版评估相比，缺口已从“缺少生产配置、硬边界、soak、绝对门槛和制品治理”
收敛为“取得当前 SHA 的全平台证据、完成仓库治理选择并执行目标环境放量”。在这些证据
完成前，技术实现可以称为 production candidate，发布状态仍必须保持暂停。
