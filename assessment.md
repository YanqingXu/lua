---
status: current
as_of: 2026-08-13
baseline_sha: d8fff0165d46cbf1bcaab692d9fe2b16b4de68f8
last_pushed_checkpoint_sha: d8fff0165d46cbf1bcaab692d9fe2b16b4de68f8
candidate_sha: pending
release_target: 0.1.0
release_state: pre-rc-blocked
---

# 推荐开发路线图：pre-RC → `v0.1.0` → `v0.2`

## 一、文档目的

本文件不再使用主观完成百分比描述项目状态，而是作为当前版本的执行路线图。每项工作都要有
明确的目标、动作、依赖、验收标准和退出条件。

本路线图当前以 `d8fff0165d46cbf1bcaab692d9fe2b16b4de68f8` 为已推送基线。它已通过 PR #19
并取得 17/17 push CI，但合并后审查仍发现 Nightly 最坏时长和 Windows 工具发现两项中风险问题，
且 Nightly、治理和三平台候选包尚未闭环，因此不是可发布候选。任何实现变更都会产生新的候选
SHA；CI、nightly、coverage、benchmark、soak 和制品证据必须全部重新绑定到最终候选 SHA，
不能沿用其他提交的绿色结果。

当前最准确的项目定位是：

> Lua 5.1 源码与公开 C API 高兼容的现代 C++23 Runtime Preview。解释器、运行时 SDK、
> 隔离 worker、测试体系和发布制品链主体已经完成，但同 SHA 发布证据、nightly、仓库治理和
> 目标环境验证尚未闭环，因此仍处于 pre-RC，不能描述为“已批准生产发布”。

短期唯一主目标是发布一个可信、可复验、可回滚的 `v0.1.0-rc.1`。在 RC 完成前暂停新增语言
特性和大规模架构重构。

## 二、当前基线

### 2.1 状态快照

| 维度 | 2026-08-13 当前事实 | 结论 |
|---|---|---|
| Git | `origin/main` 为 `d8fff016...`，PR #19 已合并；Phase A 修复位于本地 `codex/v0.1.0-rc1-readiness` | 修复必须形成新 PR，并在合并后冻结新的 `main` SHA |
| 本地实现 | Phase A Windows debugger ON Release 为 48/48 CTest、832 tests / 7140 results，OFF 为 47/47 CTest、791 tests / 6804 results；相对 `d8fff016...` 各增加 1 个 VS 发现合同 | 双配置 CTest/units 本地全绿；本机 LLVM 22 对历史头文件启用新增 portability 检查，权威 lint 与 GitHub Windows 仍须由新 SHA CI 复验 |
| 调试器范围 | 调试器已纳入路线、CHANGELOG 和独立 coverage，但属于 `v0.2` 能力线 | 0.1 正式包固定关闭，开发/CI 可独立开启 |
| Fuzz 证据 | 六目标策略已统一；合法最大输入为六目标各 1200 秒，即 120 分钟顺序 campaign | Phase A 将 job timeout 调为 160 分钟并用合同保留至少 30 分钟额外预算 |
| Coverage | `d8fff016...` 的七个组件均超过批准阈值，且原始 artifact 可复验 | 新修复 SHA 必须重新生成 coverage，旧 artifact 不能继承 |
| CI | [`d8fff016...` 的 run 31605865677](https://github.com/YanqingXu/lua/actions/runs/31605865677) 为 17/17 成功，coverage 与 benchmark artifact 未过期 | 证明已推送基线健康；Phase A 新 SHA 仍需重新取得 17/17 |
| Nightly | workflow 在 GitHub 为 `disabled_manually`，没有当前 SHA 的 soak/fuzz artifact | 发布证据链中断；修复 SHA 合并后必须恢复手动与 scheduled 两类运行 |
| 仓库治理 | 私有仓库当前套餐仍不能启用 branch protection/ruleset；[#6](https://github.com/YanqingXu/lua/issues/6) 开放 | 升级/公开，或为单次 RC 使用最长 30 天、带独立审查的结构化豁免 |
| 内存合同 | [#5](https://github.com/YanqingXu/lua/issues/5) 仍开放；callback allocator 不是全运行时 hard heap limit | 0.1 只承诺已文档化的进程边界与有限 allocator 计量，不扩大声明 |
| 发布 | release workflow 无真实运行；无 tag、GitHub Release 或 current-SHA 三平台候选包 | 状态是 `pre-rc-blocked`，不是 RC |

### 2.2 当前恢复批次

PR #19 已完成主线恢复和调试器范围收口。当前 `codex/v0.1.0-rc1-readiness` 只处理三项
合并后遗留问题，不扩展运行时或调试器能力：

1. 保留 Nightly 六目标各 600–1200 秒输入范围，把长 fuzz job timeout 调整为 160 分钟，并以
   合同验证最大 120 分钟 campaign 之外仍有至少 30 分钟安装、构建和上传余量。
2. 把 Visual Studio 环境发现抽成失败关闭模块：优先 `vswhere`，仅对真实位于 `Common7` 下的
   bundled CMake 回退路径推导，并覆盖 standalone CMake 正向和无 VS 负向合同。
3. 校正 README、路线图、RC 说明和任务台账，使所有“当前”结论区分已推送基线与未提交修复。

当前文档不预填未来 commit SHA 或远端运行结果；本地验证完成后仍须经 PR 审查，CI 固定版本的
clang-tidy 和 GitHub Windows runner 由新 SHA 的远端 jobs 给出权威结论。

### 2.3 接续顺序

1. 完成 Phase A 双配置验证、PR 审查与合并，记录新的 `main` SHA。
2. 在该 SHA 上取得 17/17 CI，并下载复验七组件 coverage 与 runtime benchmark artifact。
3. 启用 Nightly，在同一 `main` SHA 上取得一次 `workflow_dispatch` 和至少一次 scheduled run，
   复验 runtime/native-module soak、worker fault 与六目标 fuzz artifact。
4. 对 #5 记录 0.1 “有限 allocator + 进程隔离”、0.2 再评估完整 hard heap limit 的书面决策；
   对 #6 完成 ruleset 或单次 RC 豁免。
5. 运行 manual release 生成 candidate-only 三平台包；证据齐全后才创建不可变
   `v0.1.0-rc.1` annotated tag。
6. RC 后执行 7 天观察、至少三次连续 scheduled Nightly、24–72 小时业务 soak、shadow/canary
   和实际回滚演练。

#### 2026-07-26 历史快照（已失效，仅保留审计轨迹）

| 维度 | 当前事实 | 结论 |
|---|---|---|
| Git | 上一个已推送检查点为 `426c4a0`；本轮平台、制品消费与故障矩阵改动会和本文一起形成新的 `origin/main` 检查点 | SHA 不能在其自身提交内容中自引用；远端门禁通过前继续保持 `candidate_sha: pending` |
| 本地 Debug/Release | 既有全新 Debug/Release 基线保持绿色；本轮 Release 重新 configure/build 后 45/45 CTest、791 tests / 6780 assertions / 0 failures / 0 skips | 本地信号绿色；最终提交 SHA 仍须由远端多编译器、sanitizer、lint 与 nightly 证明 |
| Lua 5.1 C API | 官方 123/123 PASS，0 XFAIL，0 UNSUPPORTED | 公开 C API 兼容合同成熟 |
| 最新检查点 CI | `426c4a0` 的 [run 30196993311](https://github.com/YanqingXu/lua/actions/runs/30196993311) 为 `startup_failure`、0 jobs；页面注解明确为近期付款失败或 spending limit 不足 | 不是源码、workflow 语法或测试失败；恢复账户付款授权后必须在最终 SHA 重跑 |
| Workflow 定义 | CI、nightly、release 三份 workflow 在 GitHub 均为 active；本轮使用 actionlint v1.7.12 复核通过 | 定义层已通过本地/远端解析，但尚无本轮 SHA 的实际 runner 执行 |
| Coverage | 六个关键组件均超过硬阈值 | 已达标，但余量只有 1.88–2.91 个百分点 |
| Benchmark | 相对回归判断与绝对 SLO 均通过 | 性能不是当前关键路径 |
| Nightly | [run 30121904186](https://github.com/YanqingXu/lua/actions/runs/30121904186)、[run 30172103347](https://github.com/YanqingXu/lua/actions/runs/30172103347) 与最新 [run 30192787500](https://github.com/YanqingXu/lua/actions/runs/30192787500) 均为 `startup_failure`、0 jobs；账户账单页显示付款授权失败 | 当前没有有效长稳证据；需先由账户持有人恢复付款授权 |
| 审计基线发布门禁 | checklist 要求 exact-SHA CI/nightly/coverage/benchmark，但基线 workflow 只检查源码、普通 CTest 和打包 | 本地已实现更严格门禁，仍需候选提交和远端负向验证 |
| 仓库治理 | `#6` 仍开放；当前私有仓库套餐无法启用计划中的保护规则 | 必须升级、公开或形成限时审计豁免 |
| 发布状态 | 无 tag、无 GitHub Release；结构化治理 attestation 尚未签署 | tag 发布当前失败关闭 |
| 内存边界 | callback allocator 只覆盖已迁移切片；进程边界提供最终兜底 | 不得宣称全运行时 allocator hard limit |
| MinGW | 0.1.x 平台策略已在 CMake configure 阶段明确拒绝 MinGW | 当前正式 Windows 包只支持 x64 MSVC ABI，不再允许配置成功后才链接失败 |

#### 2026-07-26 执行进展（已被后续调试器提交取代）

以下实现与本文档在本轮收敛后一起提交并推送。下一次继续时先用 `git rev-parse origin/main`
取得精确检查点；这里不把提交 SHA 写回提交自身。`candidate_sha: pending` 表示该检查点尚未取得
同 SHA 的完整 CI、nightly 与真实治理批准，不表示本地实现未提交。

| 路线项 | 当前本地实现与验证 | 仍缺少的退出证据 |
|---|---|---|
| P0-01 格式恢复 | `src/core/value.cpp`、`src/core/value.hpp` 的已知 clang-format 违规已修复；Strict Changed 以显式 HEAD 基线检查 committed/staged/working/untracked 并通过。All/Changed/CI 均覆盖全部自有 C/C++、仅排除受完整性清单保护的上游目录 | 本次推送 SHA 的 clang-format 与 clang-tidy 真实执行；17/17 CI；历史 All 格式债在 RC 后分批清理 |
| P0-02 测试可信度 | 编译器 fixture 与四种元方法假绿路径已改为真实失败/VM 调度断言；框架区分 expected/unexpected skip；静态 test-signal checker 及负向 fixture 已落地。最新本地 Release 为 791 tests / 6780 assertions / 0 failures / 0 expected skips / 0 unexpected skips | MSVC/GCC/Clang、sanitizer 与完整远端 CI 的同 SHA 证据 |
| P0-03 本地证据 | Changed 格式范围支持显式 `-FormatBase`，严格模式拒绝无 merge-base 的空范围；测试二进制报告并校验 40-hex build SHA。build provenance v2 绑定 HEAD、源码/构建目录、生成器、目标 OS/CPU、指针宽度和配置；打包仅接受三个固定 RID，并在 clean rebuild 前后拒绝旧 SHA、脏产物、Debug 冒充 Release、32 位或 RID 冒名。显式测试筛选零命中也会失败 | 在本次推送 SHA 上由远端 CI 对同一源码集合和三类 runner 给出一致结果 |
| P0-05 发布证据 | verifier 从原始 coverage、benchmark、soak 与 fuzz payload 复算结论，并绑定 source readiness、版本/ABI、结构化治理与 annotated tag。三个精确 RID artifact 会逐包深验，再由同 RID 独立 job 下载、限制 ZIP 解压、清空 CMake registry/default path，并要求静态/共享 consumer 同时构建运行；publish 再复验三 RID×四资产与全局 checksum，manual dispatch 不能发布 | 在本次推送 SHA 上读取真实 GitHub runs/artifacts 并失败关闭；取得有效 nightly；完成真实治理授权；生成动态 RC body 与真实制品 checksum |
| P0-04 Nightly | 最新 push run 30196993311 与 nightly run 30192787500 均为账户级 `startup_failure`、0 jobs；GitHub 页面明确要求处理失败付款或提高 spending limit | 账户持有人恢复付款授权；随后对最终候选 SHA 重跑 CI、手动与 scheduled nightly，取得 soak、worker fault matrix、native-module 和 long-fuzz 证据 |
| P0-06 仓库治理 | 已移除永久布尔旁路；tag push 必须消费 `LUA_RELEASE_GOVERNANCE_ATTESTATION` 严格 JSON，精确绑定仓库/SHA/tag/版本、批准人与独立审查者、六项控制、记录 URL 和期限。manual dispatch 始终是 candidate-only，不能因选择 tag ref 而发布 | 仍需仓库所有者选择升级/公开，或为最终候选签署最长 30 天的限时豁免并设置精确 attestation；本地 schema 与负向合同不能代替真实批准 |
| RC-02 平台与候选包 | 新增机器可读 `platform-baseline.json`、CMake 失败关闭策略和 22 项验证合同；正式 RID 固定为 Windows Server 2022/MSVC、Ubuntu 24.04/GCC 14、macOS 14 ARM64/AppleClang。`426c4a0` 的隔离 Windows clean rebuild 已生成 `0.1.0-rc.1-windows-x64` ZIP/SBOM/manifest/checksum，真实 DLL 为 x64、143 个 Lua 导出且只依赖 Release MSVC/UCRT；归档解压后的静态/共享纯 C consumer 2/2 通过 | 最终 SHA 在三个固定 runner 上的真实 binary dependency 检查、下载后 consumer 和可下载 artifact |
| RC-03 故障注入 | worker 增加可安全解除的跨线程 watchdog cancellation 与结构化 host 错误；12 场景 fault matrix 校验 exit/outcome/stop reason、预算、取消、JSON 和 allocator 归零。nightly 已增加 Windows/Linux 启用 OS limits 的证据 job；运维文档明确强杀、重启、State pool、监督器分类边界 | 真实目标镜像的 OOM/CPU kill、监督器原因、重试/去重、native module、p99/RSS/恢复 SLO 与 macOS 外层监督器证据 |
| RC-05 Shadow/canary | 已冻结无副作用 shadow、分阶段 canary、停止阈值、drain 与回滚记录合同，并明确仓库测试不能替代业务环境 | 24–72 小时真实任务 soak、shadow/canary 实跑、实际回滚演练和书面 go/no-go |

本轮封板先使用此前不存在的 `build/p0-final-validation-final`，随后在最终脚本/runner 收口后
reconfigure 并复验：

- CMake configure 与封板后 reconfigure 均成功；
- 既有全新 Release/Debug ALL_BUILD 基线保持绿色；本轮 Release 全量直跑为
  791 tests / 6780 assertions / 0 failures / 0 expected skips / 0 unexpected skips；
- 最终 Release reconfigure、worker/test rebuild 和 45/45 CTest 通过；Debug runner 既有重建与
  Test Framework 定向合同通过；
- 平台策略 22 项、下载包 consumer 负向合同和 12 场景 worker fault matrix 通过；
- 隔离 Windows 包完成 clean rebuild、深度制品验证、真实 DLL import/export 审计，以及归档
  解压后静态/共享 consumer 2/2；
- governance 18 项、source-readiness 6 项、release evidence 35 项、Release body 11 项、
  tag identity 9 项及 package validator 合同通过；
- release identity、package provenance、test signal、binary SHA、benchmark SLO 与完整
  quality-gate 配置合同在适用的 Windows PowerShell 5.1 / PowerShell 7 路径通过；
- actionlint、三份 workflow YAML、PowerShell AST、文档漂移与 `git diff --check` 通过；
- 本机未安装 clang-tidy，因此本地没有伪造该结论；它仍必须由新候选 SHA 的远端 lint job
  真实执行并成功。

因此当前状态仍是 `pre-rc-blocked`。本轮推送只形成实现检查点；只有它取得完整 CI、nightly、
coverage、benchmark、制品和治理证据后，才能更新 `candidate_sha` 或判定 P0 退出。

#### 2026-07-26 接续点（已被上方当前顺序取代）

下一次不要继续增加功能，按以下顺序恢复：

1. `git pull --ff-only`，记录 `git rev-parse HEAD`，确认它等于本次推送后的 `origin/main`。
2. 处理 GitHub 账户的付款授权失败或 spending limit；在此之前，0-job `startup_failure` 不算
   测试失败或 endurance 证据。
3. 检查该 SHA 的 CI，要求 17/17 jobs 成功且 clang-format/clang-tidy 都真实执行。
4. 对同一 SHA 运行一次手动 nightly，并等待至少一次 scheduled nightly；下载并复验
   soak/fuzz/worker-fault artifact。
5. 仓库所有者选择 required ruleset，或签署最长 30 天、精确绑定该 SHA 与
   `v0.1.0-rc.1` 的 `LUA_RELEASE_GOVERNANCE_ATTESTATION`。
6. 先运行 manual release workflow，要求三平台 baseline/shared-library 验证、精确 artifact
   下载和静态/共享 consumer 矩阵全绿；它仍只能生成 candidate-only 包。所有同 SHA 证据齐全后
   才创建 annotated tag，不能移动或复用 tag。
7. 若任何 job 产生修复提交，旧 SHA 的 nightly、治理 attestation 和制品证据全部失效，重新从
   第 3 步开始。

当前明确外部阻塞只有：Actions 付款授权/额度、最终 SHA 的远端运行、仓库所有者的真实治理决策。
最低 OS/运行库基线、下载后独立 consumer 和仓内 fault matrix 已完成本地实现；下一次不得重复
实现它们。仍未完成的是三个远端 RID 的真实包证据、目标环境破坏性故障注入、24–72 小时业务
soak、shadow/canary、实际回滚和书面 go/no-go，这些也不得被本地合同误写成已经完成。

### 2.4 Coverage 基线

| 组件 | 当前行覆盖率 | 当前阈值 | 余量 |
|---|---:|---:|---:|
| bytecode verifier | 86.28% | 84% | +2.28 |
| C API | 85.91% | 83% | +2.91 |
| GC phases | 88.91% | 86% | +2.91 |
| opcode handlers | 85.14% | 84% | +1.14 |
| parser/codegen | 91.90% | 90% | +1.90 |
| sandbox denied paths | 78.86% | 76% | +2.86 |
| debugger core | 79.26% | 75% | +4.26 |

以上是 `d8fff016...` 的 run `31605865677` 原始 coverage artifact 实测值。阈值已经具备阻断
能力，但 opcode handlers 仅余 1.14 个百分点。新增代码必须同步补测试，不能通过下调阈值
消化覆盖率下降；Phase A 新 SHA 必须重新生成并复验该 artifact。

### 2.5 当前支持边界

项目当前承诺：

- Lua 5.1 语言源码兼容和公开 C API 高兼容；
- 项目自己的静态/共享 SDK、CMake consumer 和原生模块合同；
- game-server sandbox、instruction/native-work budget、deadline、取消与 metrics；
- Windows/MSVC、Linux、macOS 的构建与测试；
- 受校验的项目 binary chunk、SBOM、checksum 和安装后 consumer。

项目当前不承诺：

- 与官方 Lua 动态库二进制 ABI 直接替换；
- 与官方 `luac` 二进制块逐字节互换；
- sandbox 可以替代 Job Object、rlimit/cgroup、container 或独立进程隔离；
- callback allocator 能限制 Lua 执行期间的全部进程内存；
- MinGW 已受支持；
- `0.x` 扩展 API 永久不变。

## 三、路线原则

1. **先恢复可信信号，再增加能力。** 红色 `main`、无效 nightly 和假绿测试必须先处理。
2. **所有发布证据绑定同一 SHA。** “某个历史提交通过过”不构成当前候选证据。
3. **失败必须可见。** unexpected skip、捕获异常后成功、恒真断言和缺少 job 的 workflow 都不能算通过。
4. **发布门禁默认失败关闭。** 证据缺失、过期、来自其他 SHA 或无法读取时，结果必须是拒绝发布。
5. **冻结期只修发布阻塞问题。** 不在 RC 前进行大文件拆分、广泛类型重构或新语言特性开发。
6. **代码能力与部署保证分开表述。** SDK、sandbox、allocator 与 OS 进程边界分别给出准确合同。
7. **性能以防回归为主。** 当前性能远高于绝对 SLO，不为了微基准数字牺牲可维护性或正确性。
8. **修复进入新提交和新版本。** 不移动 tag、不覆盖历史制品、不用 rerun 掩盖新 SHA 缺失证据。

## 四、共同完成标准

任何任务只有同时满足以下条件，才可标记为完成：

- 实现、回归测试和必要文档位于同一提交；
- 工作树干净，提交 SHA 可唯一定位；
- 从干净构建目录重新 configure/build，不把旧 `bin/` 或旧 build tree 当作候选证据；
- 本地严格门禁和所有适用的远端 job 均通过；
- 测试没有 unexpected skip、无条件成功断言或吞掉异常的路径；
- 新增公开行为同时有源码树和安装树 consumer 证据；
- 失败路径验证错误分类、状态回滚、资源释放和后续可用性；
- 外部证据记录 SHA、run URL、artifact 名称和生成时间；
- 尚未关闭的限制进入公开文档和跟踪 issue。

## 五、P0：恢复可候选状态

预计代码工作量为 1–2 个工作日；nightly 与仓库治理依赖 GitHub 账户/套餐状态，整体预计
2–4 个工作日。P0 未全部退出前，不创建任何 RC tag。

### P0-01：恢复完整 CI

**目标**

消除当前 HEAD 的格式失败，确保 clang-format 与 clang-tidy 都真实执行。

**具体动作**

1. 修复 `src/core/value.cpp`、`src/core/value.hpp` 及本候选实际触及文件中的格式违规，不混入
   无关重构。
2. 使用与 CI 兼容的 clang-format 版本，检查从候选基线到工作区的完整变更集合；All 审计发现
   的历史债登记后分批处理，不在 P0 做全仓机械格式化。
3. 本地运行：
   `powershell -NoProfile -ExecutionPolicy Bypass -File tools/run_quality_gate.ps1 -Strict
   -FormatScope Changed -FormatBase <候选基线>`。
4. 从空目录执行 MSVC Debug/Release 构建和完整测试。
5. 推送后检查 lint job，确认 clang-tidy 不是因前置失败而显示 skipped。

**验收标准**

- 最终候选 SHA 的 17 个 CI jobs 全部成功；
- clang-format 通过；
- clang-tidy 实际执行且成功；
- 完整测试不少于当前 791 tests / 6780 assertions 基线，且 0 failures、0 unexpected skips；
- 无未解释 warning、sanitizer 报告或新增 skip。

**风险与控制**

- 不执行无关的全仓自动格式化，避免产生大面积机械 diff；
- 不把当前 `bin/lua_test.exe` 当作证据；它早于本次审计构建；
- 若工具版本输出不一致，以 CI 固定版本为准并在开发环境显式记录。

**退出条件**

产生新的绿色候选 SHA，并记录对应 CI run；`87c15e6` 的历史结果只保留为审计基线。

### P0-02：消除假绿测试

**目标**

确保“零失败”确实代表验证执行，而不是异常被转成成功。

**具体动作**

1. 修复 `tests/unit/compiler/test_lua_functions.cpp`：
   - tracked fixture 缺失时直接失败；
   - parser/codegen 的任何异常直接失败；
   - 删除捕获任意 `std::exception` 后 `ASSERT_TRUE(..., true)` 的逻辑；
   - 将两个 `usize >= 0` 恒真断言改成对 fixture 结构或执行结果的真实验证。
2. 修复 `tests/unit/metamethod/test_metamethod_complete.cpp`：
   - 用真实调用和返回值验证 `__concat`、`__call`、`__mod`、`__pow`；
   - 删除四个只证明 `true` 的占位断言；
   - 避免只验证“元方法已注册”而没有验证 VM 调度行为。
3. 在测试框架中区分 expected skip 与 unexpected skip；发布门禁遇到 unexpected skip 必须失败。
4. 给质量门增加静态检查，至少拒绝已知形式的 `ASSERT_TRUE(..., true)` 和“捕获异常后标记 skip”。

**验收标准**

- 删除 fixture 时对应测试失败；
- 人为制造 parser/codegen 错误时对应测试失败；
- 四种元方法均经过真实 VM/C API 路径并校验结果；
- 完整测试为 0 unexpected skip；
- 恒真断言扫描为 0；
- 新测试在 MSVC、GCC、Clang 和 sanitizer 任务中通过。

**退出条件**

测试基线数量更新，所有新增断言都能通过一个明确的负例证明其具有失败能力。

### P0-03：修复本地质量门的证据盲区

**目标**

避免干净工作树上的已提交改动逃过本地 Changed-scope 格式检查，也避免旧测试二进制被误认为
当前 HEAD 的结果。

**具体动作**

1. 候选验证固定使用 `-FormatScope Changed -FormatBase <候选基线>`，同时合并 committed、
   staged、working 与 untracked 变更；不能退回只检查未提交文件。`All` 保留“全部仓库自有
   C/C++”的诚实审计语义（包括 `lua_test` 与各类 test target，只排除由 source-integrity
   manifest 保护的只读上游 `tests/lua/official/**`），但当前仍会揭示大范围历史格式债，
   不在 P0 冻结期用机械全仓重排掩盖候选所触及文件的结论。
2. 为 `tools/run_quality_gate.ps1` 的 Changed 模式增加显式 merge-base 参数，或在非 PR 环境
   明确比较目标分支，而不是只检查未提交、暂存和未跟踪文件。
3. 让测试二进制或测试报告输出构建 SHA；门禁校验它等于 `git rev-parse HEAD`。
4. 候选验证一律从空 build directory 开始，报告编译器版本、配置和 SHA。
5. 为上述门禁本身补合同测试：已提交但未格式化的文件必须被发现，旧二进制必须被拒绝。
6. CMake configure 必须生成 build provenance，记录源码 HEAD、源码/构建目录、生成器、平台/
   toolset、单配置 build type 或多配置集合。
7. 打包必须要求当前 HEAD 与工作树干净、provenance 与请求配置一致；随后从当前源码重新
   configure、`--clean-first` 构建并复查，再执行 install，不能直接信任历史 build tree。

**验收标准**

- 干净工作树上制造一个“上一提交未格式化”的测试场景时，Changed 模式仍会失败；
- build SHA 与源码 SHA 不同会失败；
- Changed 显式 merge-base、All owned/upstream 边界和旧二进制拒绝均有自动化测试；
- build tree 来自其他 SHA/源码目录/构建目录、单配置 Debug 冒充 Release、未知多配置以及
  “脏源码构建后恢复干净”的陈旧产物都会被拒绝或由 clean HEAD 确定性重建；
- 本地输出可直接进入候选 evidence manifest。

**退出条件**

本地严格门禁与 CI 对同一源码集合给出一致结论。

### P0-04：恢复 nightly

**目标**

取得候选 SHA 的有效 endurance 证据，而不是只有 workflow 启动失败记录。

**具体动作**

1. 先检查 GitHub Actions usage、billing、spending limit、仓库 Actions 设置和组织策略。
   2026-07-26 的只读检查已确认：Actions 已启用，预算虽超额但配置为不停止使用；账户存在
   `Your payment authorization has failed` 告警。这与连续 0-job `startup_failure` 相符，
   当前首要外部动作是由账户持有人联系发卡行或更新付款方式。不得把付款授权问题误写为
   nightly 脚本失败，也不得在未执行任何 job 时计入 endurance 证据。
2. 因同期其他 workflow 也出现 synthetic `BuildFailed`，优先按账户/平台启动层问题排查，
   不在没有 job log 时盲改 nightly 脚本。
3. 增加合理的 workflow concurrency，取消同分支已过期的运行，降低私有仓库分钟消耗。
4. 恢复后手动 dispatch 最终候选 SHA，再观察至少一次 scheduled nightly。
5. 保存 runtime soak、native-module lifecycle 和四个 long-fuzz target 的 artifact。
6. artifact 元数据必须包含 workflow run、attempt、head SHA、配置、持续时间和结果。

**验收标准**

- 最终候选 SHA 的手动 nightly 完整成功；
- 随后至少一次 scheduled nightly 完整成功；
- 45 分钟 runtime soak、1000 次 native-module lifecycle、四个 fuzz target 各 600 秒完成；
- 无 crash、timeout、OOM、sanitizer、allocator 归零或取消延迟异常；
- 所有 artifact 可下载且 SHA 与候选一致。

**退出条件**

候选记录中有可复验的 nightly run URL 和 artifact 清单；0-job 的 startup failure 不计为测试执行。

### P0-05：实现 exact-SHA 发布门禁

**目标**

让 `.github/workflows/release.yml` 真正执行
[RC 与正式发布门禁](docs/release/release-checklist.md)，消除“文档要求严格、自动化允许旁路”的
差异。

**具体动作**

1. 在打包和 publish 之前增加 `verify-evidence` job。
2. 解析 tag/dispatch 对应提交，并确认：
   - SHA 是批准的 `main` 提交；
   - 当前 SHA 的完整 CI check suite 全部成功；
   - lint 中 clang-format 与 clang-tidy 均实际执行；
   - coverage artifact 含原始 LLVM export 与固定阈值，verifier 从原始文件复算七组件结果并
     拒绝覆盖范围坍缩；
   - benchmark artifact 含逐次原始采样，verifier 复算 median、paired regression、GC P99/
     max，要求 base 是 head 的严格祖先、权威 Git tree 与运行时输入声明一致，并逐次满足固定
     10 项绝对 SLO；
   - 当前 SHA 的 nightly 完整成功，soak/native-module/fuzz artifact 未过期；机器结果必须与
     workflow 参数和 GitHub job step 的权威持续时间一致；
   - RC notes、版本、ABI 和 evidence manifest 没有占位符。
3. 将 `tools/check_release_readiness.ps1` 明确定位为本地 source-readiness 检查；远端证据由独立脚本/
   job 校验，避免一个脚本名同时表达两种不同保证。
4. packages 和 publish 都必须依赖 `verify-evidence`，不能只在 publish 最后一步补检查。
5. 保留 release environment 人工批准，但人工批准不能绕过自动证据。
6. 建立机器可读的 evidence manifest，至少包含 SHA、run ID、attempt、job、artifact digest/
   过期时间、原始 payload 复算结论、生成时间与工具版本。
7. publish 不能只浅查 manifest 顶层字段；最终 Release body consumer 必须重新验证完整 schema、
   payload、timed-step、运行集合和全局 checksum 文件集，且 verifier 的真实输出要有端到端合同。
8. tag 发布前必须从 GitHub Git refs API 证明顶层对象是 annotated tag，递归 peel 后精确等于
   evidence-approved candidate；lightweight、错误 SHA、循环、过深链和 API 错误全部失败关闭。

**负向合同测试**

以下情况必须确定失败：

- CI 有任一失败、取消或 skipped required job；
- nightly 成功但来自旧 SHA；
- artifact 缺失、过期或内部 SHA 不匹配；
- artifact 只有 metadata、ZIP 路径不安全、digest 不符，或 coverage/benchmark/soak/fuzz
  机器 payload 与摘要、参数、Git tree、绝对 SLO、权威 step 时间矛盾；
- benchmark base 不是 head 的严格祖先；
- tag 不在批准的 `main` 历史上；
- tag 是 lightweight、递归 peel 到其他提交或 tag 对象链不可验证；
- RC notes/evidence manifest 仍有占位字段；
- verifier 输出与 publish consumer schema 漂移，或发布资产 checksum 集合不完整；
- GitHub API 暂时不可读或返回不完整结果。

**验收标准**

- 上述负例全部有自动化测试；
- 完整绿色 SHA 才能进入打包；
- 手动 workflow dispatch 仍只能生成验证候选，不会意外发布；
- tag 路径在治理未批准时继续失败关闭。

**退出条件**

发布 workflow 的自动约束与 release checklist 一致。

### P0-06：完成仓库治理决策

**目标**

避免未经验证的直接提交再次把 `main` 留在红色状态，并为 tag 发布建立可审计授权。

**可选路径**

1. 升级仓库套餐以启用 required checks/ruleset；
2. 调整仓库可见性以获得所需治理能力；
3. 为当前 RC 制定明确、限时、可审计的豁免。

**规则或豁免至少要覆盖**

- required CI checks；
- 合并前分支必须最新；
- 禁止 force push 和删除默认分支；
- tag 创建/删除限制；
- 至少一名独立审查者；
- 豁免的批准人、适用 SHA/版本、风险、开始日期和失效日期。

**验收标准**

- `#6` 关闭，或存在仅覆盖当前 RC 且会自动/明确到期的书面豁免；
- 决策有责任人、日期、适用范围和撤销条件；
- `LUA_RELEASE_GOVERNANCE_ATTESTATION` 只在上述条件成立后写入，并精确绑定候选 SHA/tag/版本；
- attestation 缺失、过期、字段不完整、批准人与审查者相同或控制状态不合规时，tag workflow
  确定失败；旧布尔值不能授权。

**退出条件**

仓库规则证据或限时豁免进入 release evidence manifest。

### P0-07：刷新状态文档

**目标**

使 README、RC notes、兼容性页、源码注释和实际候选状态一致。

**具体动作**

1. 在 P0 最终 SHA 确定后更新 CI/nightly 链接、公开 API 计数和候选说明。
2. 修正 `src/lib/debuglib.hpp` 中“primitive type metatable 尚未实现”的过时说明。
3. 清理 `tests/unit/vm/test_lua_state_init.cpp` 中已被实现/测试覆盖的 GC threshold TODO。
4. 将高频变化的计数与状态尽量改为脚本生成，减少手工漂移。
5. 文档中持续保留 allocator、sandbox、官方 ABI、binary chunk 和平台支持边界。

**验收标准**

- 每个“当前”“已通过”“已支持”声明都有源码、测试或 exact-SHA run；
- 不再引用被后继提交替代的候选状态；
- 文档漂移检查通过；
- `assessment.md` 的 `candidate_sha` 更新为最终 P0 SHA。

### P0 总退出门禁

只有同时满足以下条件，项目才可以进入 RC：

- 最终候选 SHA 的 17/17 CI 全绿；
- 同一 SHA 的手动与 scheduled nightly 全绿；
- exact-SHA 发布证据门禁已落地并通过负向合同测试；
- 仓库治理决策完成；
- 已知假绿测试清零，0 unexpected skip；
- 状态文档只指向最终候选 SHA；
- 无未归因 crash、data race、泄漏、allocator 归零失败或高严重度安全问题。

## 六、RC：生成 `v0.1.0-rc.1`

P0 退出后，预计 3–7 个工作日完成候选制品、目标环境验证和 RC 发布。

### RC-01：候选冻结

**具体动作**

- 宣布 feature freeze，只接受发布阻塞修复；
- 复核 CMake 与 `lua_cpp_version.h` 版本、ABI 版本、公开 API 计数和已知限制；
- 冻结最终候选 SHA，生成 evidence manifest；
- 禁止“顺手重构”、大范围重命名和格式化。

**验收标准**

- 候选提交位于批准的 `main`；
- 版本为 `0.1.0`，ABI 为 `0`；
- 工作树干净；
- 没有未归因高严重度问题；
- SHA、版本、ABI、API 数量、证据链接和已知限制可机器读取。

### RC-02：明确平台支持并生成候选包

**具体动作**

1. 明确最低支持环境：
   - Windows x64：MSVC 版本与 CRT；
   - Linux x64：最低发行版/glibc 基线；
   - macOS ARM64：最低 deployment target。
2. 在最低支持环境生成 zip、manifest、SPDX 2.3 SBOM 与 SHA-256。
3. 在全新目录解压，并分别构建/运行静态与共享纯 C consumer。
4. 检查动态依赖、导出符号、RPATH/install-name 和调试文件策略。
5. 对 MinGW 做明确决策：
   - `0.1` 推荐在 configure 阶段明确说明不支持；或
   - 修复 `_dupenv_s` 分支并加入完整 MinGW build + CTest。
   不能继续允许配置成功后在链接阶段才失败。
6. 如果 ARM64 是正式发布承诺，而不只是 CI 可移植性验证，则增加相应平台包；否则在支持矩阵中
   明确标注为 CI-only。

**验收标准**

- 每个受支持 RID 的打包、归档验证、checksum、SBOM 和 consumer 全部通过；
- 包名、manifest、SBOM 和内部 SHA 一致；
- consumer 不依赖源码树或构建缓存；
- 发布文档明确编译器、C/C++ runtime、最低 OS 与不支持项。

### RC-03：目标环境故障注入

**具体动作**

在真实 Windows/Linux worker 镜像中验证：

- success；
- instruction budget；
- native-work budget；
- deadline；
- cross-thread cancellation；
- allocator rejection；
- CPU/RSS/进程资源限制；
- 非法错误对象与异常分类；
- worker 强杀、监督器重启和流量重试；
- native module load/use/unload；
- State pool 重用与关闭归零。

macOS 若不提供进程内硬内存边界，必须验证外层监督器合同并写入限制。

**验收标准**

- JSON 结果 schema 始终可解析；
- exit code、outcome 和错误分类一致；
- allocator 在 State 关闭后 live bytes 为 0；
- 取消延迟、p99、RSS 和 allocator peak 满足已公布 SLO；
- OS limit kill 能被监督器稳定分类；
- 强杀和滚动回滚不产生跨请求状态污染；
- 每个平台保存镜像、硬件、配置、样本和结果。

### RC-04：候选说明与不可变 tag

**具体动作**

1. `docs/release/rc-notes-0.1.0.md` 只保存稳定叙述、兼容范围、已知限制和证据类型；不得回写
   候选 SHA、run ID、URL 或 checksum，否则会改变正在描述的候选提交。
2. publish job 从已深度验证的 evidence manifest 与最终制品反向生成动态 Release body，
   追加 CI/nightly、fuzz/benchmark artifact、平台文件名、SHA-256 和 SBOM；不手工猜测 checksum。
3. 在批准 SHA 创建 annotated `v0.1.0-rc.1` tag。
4. 运行 tag workflow，下载 prerelease 制品并在独立环境再次复算和消费。

**验收标准**

- tracked RC notes 无“未发布模板”、占位 SHA、动态 run URL 或待补字段；
- 动态 Release body 精确绑定最终 manifest 与全部发布资产 checksum；
- tag 不移动、不复用；
- prerelease 指向预期 SHA；
- 全部制品可下载且 checksum 正确；
- 从 GitHub Release 下载的包再次通过静态/共享 consumer。

### RC-05：shadow、canary 与回滚

**具体动作**

- 使用真实任务分布执行 24–72 小时业务 soak；
- 与旧实现或官方 Lua 进行适用范围内的 shadow 结果比较；
- 从无流量开始逐步 canary；
- 监控 p50/p95/p99、队列、RSS、allocator peak、取消延迟和失败分类；
- 实际演练 drain、停止扩大流量、切回上一不可变 SDK/worker 镜像。

**验收标准**

- 无未解释语义差异、崩溃、泄漏或错误分类漂移；
- 所有已公布 SLO 通过；
- 回滚可在约定时间内完成；
- 形成书面 go/no-go 结论。

## 七、`v0.1.0` 正式版

### 0.1-01：RC 观察期

默认观察 7 个自然日，并至少包含三次连续成功的 scheduled nightly。若业务风险或发布窗口要求缩短，
必须形成书面豁免，不能静默减少证据。

观察期内：

- 只接受 P0/P1 缺陷修复；
- 每个失败保存最小复现和 artifact；
- 修复进入新 SHA，并重新执行 P0 与 RC 门禁；
- 必要时发布 `rc.2`、`rc.3`，不覆盖 `rc.1`；
- 任何未归因 crash、data race、泄漏或资源合同破坏都会重新开始观察期。

### 0.1-02：建立 API/ABI 基线

**具体动作**

- 保存导出符号、公开头、配置结构版本和布局快照；
- 增加跨版本静态/共享 consumer 与 native-module smoke；
- 建立变更分类：源码兼容、项目 ABI、官方 Lua ABI 和内部实现；
- 记录 `0.x` 的兼容与弃用政策。

**验收标准**

- 删除公开符号、改变签名、破坏受承诺布局或未升级 ABI 版本时 CI 失败；
- Windows/Linux/macOS 快照可稳定比较；
- 不把内部 C++ 类型误纳入公开 ABI。

### 0.1-03：发布正式版

**具体动作**

1. 将 CHANGELOG 的 `0.1.0` 更新为正式日期和最终内容。
2. 从已通过 RC 的后继绿色 SHA 创建 `v0.1.0`。
3. 对正式 SHA 重新运行完整 exact-SHA 门禁、打包、SBOM、checksum 和 consumer。
4. 更新 SECURITY，明确 `0.1.x` 支持范围、报告渠道和修复策略。

**验收标准**

- 正式版 SHA 的所有门禁重新通过；
- 源版本、文档、制品、SBOM 和 checksum 一致；
- `v0.1.0` 与历史 RC 均保持不可变；
- GitHub Release 可由第三方按 evidence manifest 独立复验。

## 八、P1/P2 技术路线：RC 之后的风险收敛

以下工作不应阻塞当前格式修复，但要在 `0.2` 前完成优先级确认。若部署场景扩大了攻击面，相应任务
必须前移。

### 8.1 Bytecode verifier 的完整 CFG 验证

**当前判断**

现有 verifier 已检查 opcode、寄存器、常量、跳转目标、`SETLIST` 扩展字、`CLOSURE` pseudo
instruction 和资源上限，但尚未形成对所有可达控制流后继的统一验证。VM 允许指令流自然落到末尾，
因此需要明确“合法终止”和“非法 fallthrough”的合同。

game-server preset 已禁止脚本侧 binary chunk；可信宿主入口仍可加载已授权 binary chunk。如果产品开始
接收不可信 binary chunk，本任务立即从 P1 提升为 P0。

**具体动作**

1. 为每个 opcode 定义 CFG successor 规则：
   - `RETURN`、`TAILCALL` 为终止节点；
   - jump/test/loop 同时验证显式目标与合法 fallthrough；
   - `SETLIST` 和 `CLOSURE` 的数据字永远不能成为入口；
   - 所有可达 successor 必须位于 code 范围并指向真实指令；
   - 合法循环可以不终止，但不能通过 code end 或 pseudo data 离开 CFG。
2. 明确 unreachable instruction 的策略：拒绝、允许或只在工具中报告，三者只能选一个并写入合同。
3. 让 compiler 在每个函数生成结束时执行同一终止合同；内部生成的 Proto 不能依赖自然落尾。
4. 让 VM 在绕过 verifier 的内部入口上仍失败关闭：PC 落出 code 必须报告损坏 bytecode，不能当作
   正常 `Returned`。
5. 复用 opcode metadata，避免 verifier、compiler、VM 与 bytecode printer 各维护一套分支语义。
6. 扩展 undump/verifier fuzz corpus：末尾 fallthrough、条件 skip 落尾、跳入 pseudo data、
   嵌套 closure、test+jmp、
   loop 边界、极限寄存器和深层 Proto。
7. 为每个拒绝原因增加稳定错误类别和 PC，避免只依赖脆弱字符串。

**验收标准**

- 38/38 opcode 都有机器可读 successor/terminator 分类，新增 opcode 未分类时 CI 失败；
- 每个 opcode 至少有合法 CFG、非法目标和边界用例；
- 所有可达边都经过验证；
- malformed chunk 不会使 VM 执行到 code end 或 pseudo data；
- switch/table 两种 dispatch backend 对相同非法输入给出一致结果；
- official strict、C API differential、现有 binary chunk 测试无回归；
- ASan/UBSan fuzz 无 crash、OOM 或 timeout；
- CFG 构建保持 O(N)，输入规模翻倍时耗时不出现超线性突增；
- bytecode verifier 行覆盖率目标提升到至少 90%，branch coverage 目标至少 80%。

### 8.2 Coroutine、C-yield 与 traceback 边界

**当前判断**

这是风险驱动的兼容性补强，不代表已确认存在缺陷。先建立行为矩阵，再决定是否修改实现。

**具体动作**

- 先定义 `Running / Normal / Suspended / Dead` 状态转换表，以及 caller link、CallInfo、
  logical/physical stack top、yield results 和错误对象不变量；
- 覆盖 Lua → Lua、Lua → C、C → Lua 的 yield/resume；
- 覆盖 `pcall`、`xpcall`、error handler、metamethod、debug hook 和嵌套 coroutine；
- 验证不可 yield 的 C 边界按 Lua 5.1 语义稳定报错；
- 明确 C callback 必须以 `return lua_yield(L, n)` 结束；不承诺恢复原生 C/C++ 栈帧，
  也不引入未版本化的 continuation API；
- 验证 resume 后栈形状、返回值数量、错误对象和 traceback；
- 将 instruction budget、deadline、取消与 yield/resume 组合测试；
- 对 coroutine 创建、首次 resume、暂停后 resume、参数/结果复制执行 allocator fail-on-N；
- 对官方 Lua 5.1 可对照的行为增加 differential case。

**验收标准**

- 行为矩阵每格都有明确 PASS、文档化 deviation 或非目标；
- nesting depth 1/2/8、结果数 0/1/N、正常/yield/error/dead 组合均有自动测试；
- 每次转换都校验 status、CallInfo 深度、两个 stack top、caller link 与 error object identity；
- 不发生跨 coroutine 栈污染、悬空 CallInfo 或重复 finalizer；
- OOM 不产生部分状态，解除限制后可重试，关闭后 allocator live bytes 为 0；
- budget、deadline 和 cancellation 在 yield/resume 前后不会被意外重置；
- traceback 的 source/line/call depth 稳定；
- sanitizer 与至少 50,000 次包含三层 nested resume/yield 的 nightly transition soak 通过。

### 8.3 内存合同战略决策

`#5` 不应长期保持“部分迁移但产品表述模糊”。在 `0.2` 开始前编写 ADR，并在两条路线中选择一条：

**路线 A：worker-first，推荐**

- callback allocator 继续作为已覆盖切片的精确配额；
- 进程/Job Object/cgroup/rlimit 作为最终 hard limit；
- 强化 worker supervisor、强杀、重启、租户隔离和容量模型；
- 公开文档持续声明 `allocator-backed hard limit = unsupported`。

**路线 B：全运行时 callback hard limit**

- 迁移 AST 内部字符串/容器、codegen 临时对象、Parser diagnostics；
- 覆盖其余 stdlib、debug/trace、I/O/package 临时分配；
- 处理 native module registry 与 OS loader 的明确边界；
- 对每个实际分配 offset 做 fail-on-N 和事务性证明。

**路线 B 的完成标准**

- 每个失败点返回稳定 `LUA_ERRMEM`，无 C++ 异常越过 protected C API；
- `liveBytes <= hardLimit`，失败 realloc 后旧块仍有效；
- 栈与目标容器没有部分提交；
- 解除限制后 State 可继续使用；
- `lua_close` 后 live bytes 为 0，无 old-size mismatch 或 double free；
- Windows/Linux Debug/Release、ASan、UBSan 使用同一矩阵；
- 在全部标准满足前，公开状态仍必须是 unsupported。

**退出条件**

关闭 `#5`，README、memory contract 和运行时 API 只有一个无歧义结论。

### 8.4 覆盖率、故障注入与 fuzz 进化

**具体动作**

1. 将原六个组件阈值逐步 ratchet 到当前稳定整数下界：
   `86/85/88/86/91/78`；`debugger_core` 先保持 75%，取得连续候选证据后再评估提升。
2. 在行覆盖率之外，为 verifier、C API 错误路径、GC 和 sandbox deny paths 引入 branch coverage。
3. 将 production worker 测试拆成“OS 进程限制”和“运行时/allocator 限制”，让后者可在
   ASan/TSan 下运行。
4. 每个失败注入验证错误分类、状态可关闭/继续使用、allocator 归零和无 UAF/double free。
5. nightly fuzz 恢复上一次 corpus，定期最小化；有价值的输入经审查后进入仓库固定 corpus。
6. 记录 feature/coverage 增长趋势，不再只记录“运行了 600 秒”。

**验收标准**

- exact-SHA 七组件持续通过批准阈值；
- 安全拒绝分支至少有正常、拒绝和资源耗尽用例；
- ASan、UBSan、TSan 对适用的 runtime failure path 全绿；
- fuzz crash 可自动最小化并转成回归测试；
- 连续 nightly 能继承 corpus。

### 8.5 小而高价值的合同修复

按以下顺序处理：

1. 为 `Table::setArray(index < 1)` 明确内部前置条件：选择 assert、错误返回或异常，禁止继续静默忽略；
2. 修复 debuglib、GC threshold 等语义注释漂移；
3. 为 locale 相关测试准备固定 locale 的 CI job，避免环境性 skip；
4. 为 trace 的 VM 错误路径发射版本化 Error event；
5. 为 C closure 提供稳定 trace label；
6. 对 trace 输出增加预算，避免错误路径或大循环无限增长。

每项都必须有负例测试和文档合同，不能只修改注释。

### 8.6 架构拆分与可维护性

这部分只在 `v0.1.0` 发布并建立 API/ABI 基线后进行。

**目标**

降低单一核心 target、目录循环依赖和超大源文件带来的变更风险，不改变公开行为。

**建议顺序**

1. 生成实际 include/dependency graph，先定义允许的依赖方向；
2. 建立 bytecode contract：集中 `OpCode`、`Instruction`、decode/successor metadata，并用窄
   `ProtoView` 供 verifier/disassembler 使用；
3. 将 binary chunk reader/writer 收敛为唯一 `chunk_codec`，所有执行入口统一经过 limits + verifier；
4. 将共享代码构建为 object/core components，避免静态与共享 target 重复维护同一源清单；
5. 按稳定职责拆分大文件，而不是按行数机械切分：
   - `baselib.cpp`：loader、environment、protected calls、基础函数；
   - `lapi.cpp`：stack/value/table/call/load/debug API；
   - `stringlib.cpp`：pattern engine 与普通字符串函数；
   - `debuglib.cpp`：stack inspection、hooks、metatable/environment；
   - `lua_state.cpp`：stack/call/coroutine/metatable/runtime policy；
   - `expression_emitter.cpp`：literal/name/call/table/operator lowering。
6. 每次只拆一个边界，保持 ABI、测试数和 benchmark 不回退；
7. 增加禁止反向依赖和循环 include 的自动检查，allowlist 只能减少，新增例外必须失败。

**验收标准**

- 模块依赖图无新增环；
- chunk reader/writer 只有一份实现，所有入口共享同一验证合同；
- `vm -> compiler` 的实现级直接依赖降为 0，或进入只减不增的临时 allowlist；
- 静态/共享/安装后 consumer 全绿；
- 公开符号和 ABI 快照不变，或按版本政策显式升级；
- 每次拆分都有 characterization test；
- 编译时间、二进制大小和 runtime benchmark 没有未解释回退。

### 8.7 Trace 事件所有权与教学证据

**当前判断**

当前 `TraceEvent` 含有借用的 `Value*`、`Proto*`、`source` 和 `errorMsg` 指针。同步 JSON sink
会立即读取这些字段，但 ring/持久化 sink 复制事件后可能越过 VM 栈、Proto 或 State 生命周期。
在增加 viewer 前，应先收敛事件所有权与 schema。

**具体动作**

1. 区分同步 `TraceEventView` 与可持久化 `OwnedTraceEvent`，或在回调边界做 allocator-aware snapshot；
2. ring/异步消费者禁止保存可解引用的 VM/GC 裸指针；
3. 定义 JSONL schema v1，包含 schema version、context-local sequence、source/line、稳定逻辑 frame/proto
   ID，以及 instruction/call/return/error/yield/resume/stop 事件；
4. 为事件数、单值字节数、总字节数设置上限，并报告 truncation/dropped count；
5. 在安全基础完成后提供“源码 → AST 摘要 → disassembly/CFG → diff trace → result/error”的
   learning bundle；viewer 保持为独立消费者，不成为 runtime 依赖。

**验收标准**

- State/EngineContext 销毁后，ring 中的持久事件仍能在 ASan 下安全读取；
- 持久事件中没有 VM/GC 裸指针；
- 每个 event kind 都通过 schema validator 与稳定 golden；
- switch/table dispatch 的规范化语义 trace 一致；
- 容量为 N 的 ring 始终只保留最后 N 条，并准确报告 total/dropped；
- trace 完全关闭时，现有 benchmark 几何平均开销不超过 2%；
- trace 明确不是生产 metrics、审计日志或 Lua 兼容合同。

## 九、建议的 `0.2` 主方向

`0.2` 开始前必须用 ADR 选择唯一主方向，避免同时扩张三条高成本路线：

1. 进程隔离的生产嵌入式 Runtime；
2. 全运行时 callback allocator hard limit；
3. 教学、trace 和可视化工具。

默认建议选择 **进程隔离的生产嵌入式 Runtime**：

- 它与当前 game-server preset、worker、预算、取消、metrics 和发布制品链最一致；
- 它能用 OS 边界覆盖 callback allocator 无法控制的宿主/标准库/loader 分配；
- 它比继续迁移所有临时容器更快形成可运营价值；
- 教学文档和 trace 继续维护，但不与生产里程碑争夺主关键路径；
- 只有出现明确的同进程多租户需求时，才将全运行时 allocator hard limit 提升为主目标。

`0.2` 的建议里程碑：

| 里程碑 | 交付物 | 退出标准 |
|---|---|---|
| M2.1 产品 ADR | 单一主方向、三个真实用户故事、明确非目标 | README、issues、milestone 和成功指标一致 |
| M2.2 安全边界 | CFG verifier、coroutine/C-yield 行为矩阵 | malformed input 与边界调用均有自动化合同 |
| M2.3 内存决策 | worker-first 或 full allocator 的最终合同 | `#5` 关闭，公开描述无歧义 |
| M2.4 运行闭环 | worker/state pool、强杀、重启、滚动升级、容量模型 | 真实任务分布下达到 p99/RSS/取消/恢复 SLO |
| M2.5 测试进化 | branch coverage、sanitizer failure path、持久 fuzz corpus | 风险分支有覆盖，连续 nightly 可积累 corpus |
| M2.6 架构演进 | 依赖方向与渐进拆分 | 无 ABI 意外变化、无新增依赖环、性能不回退 |

建议按以下版本切片交付，避免一个长期分支同时承载所有变化：

| 版本 | 建议范围 |
|---|---|
| `0.2.0-alpha.1` | CFG verifier + VM fail-closed；coroutine 状态表与 Lua 5.1 oracle 矩阵 |
| `0.2.0-alpha.2` | allocator inventory/CI guard；compiler allocator 闭环；trace owned-event/schema；依赖门禁 |
| `0.2.0-beta.1` | bytecode contract + shared chunk codec；第一轮模块拆分；持久 fuzz corpus；learning bundle |
| `0.2.0-rc.1` | exact-SHA 全平台、sanitizer、fuzz、coverage、nightly、consumer、official/TestC/slow 全绿 |

## 十、依赖顺序与建议节奏

```text
P0-01 完整 CI ───────┐
P0-02 测试可信度 ────┼─→ P0 最终候选 SHA
P0-03 本地证据 ──────┘          │
                                 ├─→ P0-04 同 SHA nightly
                                 ├─→ P0-05 exact-SHA 发布门禁
                                 └─→ P0-06 仓库治理
                                              │
                                              ↓
                                  P0-07 文档与证据冻结
                                              │
                                              ↓
                           RC 制品 → 目标环境 → shadow/canary
                                              │
                                              ↓
                                       v0.1.0-rc.1
                                              │
                                 观察期 / rc.N / ABI 基线
                                              │
                                              ↓
                                          v0.1.0
                                              │
                                              ↓
                                  0.2 产品 ADR 与单一主方向
```

建议节奏：

| 时间窗 | 目标 |
|---|---|
| 第 1–2 个工作日 | 格式修复、假绿测试、本地质量门证据修复 |
| 第 2–4 个工作日 | 恢复 nightly、实现 exact-SHA 发布门禁、完成治理决策 |
| 接下来 3–7 个工作日 | 三平台包、目标环境故障注入、RC notes、`rc.1` |
| RC 后 7 个自然日 | scheduled nightly、业务 soak、shadow/canary、必要时 `rc.N` |
| 观察期通过后 | API/ABI 基线与 `v0.1.0` |
| `v0.1.0` 后 2–6 周 | 按 ADR 进入 0.2 安全、运行和架构里程碑 |

## 十一、持续度量

每个候选 SHA 至少记录以下指标：

| 类别 | 指标 | 门槛 |
|---|---|---|
| CI | required jobs | 100% success，0 skipped |
| 测试 | failures / unexpected skips | 0 / 0 |
| 兼容 | official C API | 123/123 PASS |
| 兼容 | strict/TestC/slow | 0 XFAIL，0 未归因 deviation |
| 覆盖 | 七组件 line/branch | 不低于当前批准阈值 |
| Sanitizer | ASan/UBSan/TSan | 0 report |
| Nightly | runtime/native-module/fuzz | 同 SHA 完整成功 |
| 资源 | allocator close | live bytes = 0 |
| 性能 | relative + absolute policy | 全部通过 |
| 制品 | package/SBOM/checksum/consumer | 每个平台全部通过 |
| 运行 | p99/RSS/cancel/restart | 满足公开 SLO |
| 治理 | evidence manifest | 完整、可下载、SHA 一致 |

## 十二、风险登记

| 风险 | 影响 | 优先缓解 |
|---|---|---|
| `main` 可直接变红 | 候选状态不可信 | required checks 或限时审计豁免 |
| Actions 计费/配额导致 0-job | 无 nightly 和发布证据 | 先解决账户启动层，再运行同 SHA |
| 新 release workflow 尚无候选 SHA 的远端实跑 | 本地合同正确但平台行为仍可能漂移 | 已将 `verify-evidence`、深度 manifest consumer 与 tag 身份门禁前置；候选提交后执行真实正/负路径 |
| 测试 silent skip/恒真断言 | “零失败”失去意义 | 负例合同与 unexpected-skip 门禁 |
| 旧 build artifact 被误用 | 本地证据与 HEAD 不一致 | 干净构建与 build SHA 校验 |
| 历史自有源码仍有格式债 | `FormatScope All` 尚不能作为绿色候选证据 | P0 以显式 merge-base 的 Changed 门禁覆盖全部候选改动；发布后分批格式化并最终启用 All |
| Coverage 余量较小 | 小改动即可跌破阈值 | 测试先行、逐步 ratchet、增加 branch coverage |
| allocator 过度声明 | 宿主误判隔离强度 | worker-first 合同与明确 unsupported |
| binary chunk CFG 不完整 | malformed input 进入未定义控制流 | 不可信输入禁用；完成 CFG verifier |
| 平台基线模糊 | 用户在未验证 ABI/OS 上失败 | 固定最低环境，明确 MinGW/ARM64 状态 |
| 单维护者、高提交频率 | 审查、文档和证据快速漂移 | feature freeze、独立审查、自动 evidence ledger |
| 大文件与依赖环 | 修改半径过大 | 正式版后按职责渐进拆分 |

## 十三、明确暂不做

在 `v0.1.0` 前不开展：

- Lua 5.2/5.3/5.4 新语义；
- JIT；
- 官方 Lua 动态库二进制 ABI 替换；
- 官方 `luac` 文件格式逐字节兼容；
- 大规模目录重排或全仓类型重构；
- 没有真实回归证据驱动的微优化；
- 未确定支持合同前的 MinGW 扩展；
- 与发布关键路径无关的 trace viewer 大型前端。

这些项目只有在 `v0.1.0` 发布、用户需求明确并进入独立 milestone 后才重新评估。

## 十四、发布判定

在以下四项全部完成前，不创建 RC tag，也不设置
`LUA_RELEASE_GOVERNANCE_ATTESTATION`：

1. 最终候选 SHA 的完整 CI 全绿；
2. 同一 SHA 的 nightly 恢复并生成完整、可下载的 soak/fuzz 证据；
3. exact-SHA 发布门禁已经落地并能拒绝所有缺失/旧 SHA/失败证据。
4. required ruleset 或仅覆盖当前 RC 的限时书面豁免已经批准，并以包含批准人、SHA/版本和
   失效时间的结构化治理证据进入 manifest；永久布尔变量不能替代该记录。

技术实现已经接近 RC，但下一阶段的价值不在于继续增加功能，而在于把现有能力变成一个
同 SHA、可审计、可复验、可回滚、真正发布过的产品闭环。
