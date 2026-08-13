---
status: active
created_at: 2026-08-13
updated_at: 2026-08-13T20:38:00+08:00
release_target: 0.1.0
next_release: 0.1.0-rc.1
candidate_sha: pending
candidate_tree: pending
evidence_baseline_sha: f04c890d80a89739eb8dc28ddaeb1ae5e5993273
evidence_baseline_tree: d1d5603c53dd22604cddc16afa4f4364a27a27ac
candidate_state: pr-ci-second-followup-validated-awaiting-commit
production_state: pre-rc
---

# `v0.1.0` 生产上线任务清单

## 1. 目标与使用方法

本文把当前项目从 pre-RC 推进到受控生产上线所需的工作拆成可执行任务。执行者应逐项填写负责人、日期、SHA、run URL、artifact 与审批记录；只有验收标准全部满足后才能勾选任务。

当前推荐路径为：

```text
修复合并后遗留问题
  -> 冻结最终候选 SHA
  -> 同 SHA 17/17 CI
  -> 手动 + scheduled Nightly
  -> 仓库治理批准
  -> candidate-only 三平台包
  -> 目标环境故障注入
  -> v0.1.0-rc.1
  -> 7 天观察 + 24-72 小时业务 soak
  -> shadow + canary + 实际回滚
  -> v0.1.0 正式发布
```

预计时间（所有门禁一次通过时）：

- 进入 `v0.1.0-rc.1`：3–7 个工作日；
- 进入受控生产 canary：约 10–14 个自然日；
- 较大范围放量：约 2–3 周；
- 任一修复产生新提交时，旧 SHA 的 CI、Nightly、制品和治理证明不得继续作为候选证据。

相关规范：

- [RC 与正式发布门禁](docs/release/release-checklist.md)
- [生产部署合同](docs/operations/production-deployment.md)
- [长稳与 Fuzz 证据合同](docs/quality/endurance.md)
- [平台支持基线](docs/release/platform-support.md)
- [运行时内存合同](docs/runtime/memory-contract.md)
- [SandboxPolicy](docs/runtime/sandbox-policy.md)
- [当前路线图](assessment.md)

## 2. 当前基线

截至 2026-08-13 20:38（Asia/Shanghai），已确认：

- 远端 `main` 为 `f04c890d80a89739eb8dc28ddaeb1ae5e5993273`，tree 为
  `d1d5603c53dd22604cddc16afa4f4364a27a27ac`；严格门禁修复及首轮 CI follow-up 已形成提交
  `014c69bb61a22cb523abfa01a3d04c1391436c89`、`44d7891504eb938474ace32f9c99d67455522511`，
  推送到 `codex/v0.1.0-rc1-quality-gate`，draft PR #21 保持开放；
- Phase A 修复已通过 PR #20 合并；merge SHA 为 `f04c890d...`，PR head 与 merge commit 的
  tree 相同；
- 该 SHA 的 push CI run `31661881457`、attempt 1 为精确 17/17 jobs 成功；clang-format、
  clang-tidy、ASan、UBSan、TSan、allocator、两类 ARM64 portability、official strict、
  Lua differential、C API differential、coverage 与 benchmark 均成功；
- `component-coverage` 与 `runtime-benchmark-evidence` artifact 均存在、未过期、带 SHA-256
  digest，且绑定 `f04c890d...`；
- 已在独立干净 checkout 对最终 merge SHA 完成 B1 fresh configure/build：debugger ON/OFF 均以
  Visual Studio 18 2026、MSVC 19.51.36252.0 构建成功；按清单默认参数（benchmark OFF）顺序
  CTest 均为 46/46，通过完整单元测试分别为 832 tests / 7140 results 与 791 tests / 6804
  results，全部 0 failures、0 expected skips、0 unexpected skips；
- B1 source-readiness 已通过并生成 SHA 一致的规范化证据；对严格质量门的两个阻塞已完成最小修复：
  `.clang-tidy` 与 compile-database lint 均显式排除项目不采用的 LLVM 22 新增
  `portability-avoid-pragma-once`，standalone smoke 以 `__FILE__` 为 `LUA_TEST_BUILD_GIT_SHA`
  提供无 shell 引号歧义的字符串占位，并新增参数捕获契约；
- 初始修复工作树已用原生 LLVM 22.1.3 执行完整 strict changed-scope 门禁并以 exit 0 通过；
  MSBuild、二进制 SHA 检查和完整 832 tests / 7140 results 均通过。该结果只证明对应修复有效；
  PR 通过并合并后仍须把最终 `main` SHA 作为候选，重新执行 B1/B2 与 Nightly；
- Lua 5.1 公开 C API 为 123/123 PASS，0 XFAIL，0 UNSUPPORTED；版本为 `0.1.0`，
  shared-library ABI 为 `0`；
- Nightly workflow 已恢复为 `active`。同 SHA 的手动 Nightly run `31663816824`、attempt 1
  已成功，参数为 45 分钟 runtime soak、1000 次 native-module lifecycle、六个 fuzz 目标各
  600 秒，runtime、long-fuzz 与 Windows/Linux worker fault artifact 均存在且未过期；
- scheduled Nightly 尚未运行；18:36 复核远端 schedule event 列表仍为 0。当前 cron 为每天
  `18:31 UTC`（Asia/Shanghai 次日 02:31），当前基线的首次预期 schedule 窗口为
  2026-08-14 02:31；
- Release workflow 尚无真实运行，没有 tag、GitHub Release 或三平台候选包；
- `main` 的远端状态为 `protected: false`。私有仓库当前套餐查询 branch protection/ruleset 返回
  HTTP 403，仓库 Actions variables 为 0；GitHub issue #6 仍开放；
- GitHub issue #5（完整 allocator hard limit）仍开放；PR #19 的 2 个中等级、1 个低等级线程
  仍未在 GitHub 标记 resolved。PR #20 没有 review thread，但只有 `augmentcode` 的
  `COMMENTED` review，没有独立批准记录；
- `README.md`、`assessment.md` 与 RC notes 已在提交 `014c69bb...` 同步到 `f04c890d...` 的
  17/17 CI、手动 Nightly 成功、scheduled 尚无运行和严格门禁修复状态；质量门新增防陈旧合同，
  禁止回退到旧 SHA/run 或 `disabled_manually` 描述；
- PR #21 首轮 CI run `31692577961` 精确绑定 `014c69bb...`，15/17 jobs 成功；Windows Debug 与
  Release 均在 `Quality gate contract tests` 失败。日志证明新增中文匹配字面量位于 UTF-8 无 BOM
  `.ps1` 中，被 Windows PowerShell 5.1 按本地代码页读取后产生 parser error；其余 15 jobs，
  包括 clang-tidy、sanitizer、coverage、benchmark、allocator 与 portability，均成功；
- 本地后续修复已把该合同脚本恢复为纯 ASCII，用等价 ASCII marker/regex 检查中文状态文档；
  Windows PowerShell 5.1 与 PowerShell 7 两条合同测试路径均 exit 0。follow-up 修复与本台账由
  同一提交承载；精确提交 SHA 和新 CI run 由 PR 外部证据记录，避免提交自引用；
- PR #21 第二轮 CI run `31697950031` 精确绑定 `44d7891504eb938474ace32f9c99d67455522511`，
  再次为 15/17 jobs 成功；Windows Debug/Release 均执行到质量门合同的状态文档 whole-file 断言后失败。
  远端 head 与 merge ref 的 `assessment.md` 均含目标 Nightly 行；失败来自 Windows checkout 的 CRLF
  与以 `|$` 收尾、仅在 LF 工作树验证过的正则组合，其余 15 jobs 继续成功；
- 第二轮本地 follow-up 已在 whole-file 断言入口统一把 CRLF、CR 正规化为 LF，并让自检复用同一函数；
  脚本保持纯 ASCII，Windows PowerShell 5.1、PowerShell 7 与文档漂移检查均 exit 0；原生 LLVM
  22.1.3 完整 strict changed-scope 复验也以 exit 0 通过，MSBuild、二进制 SHA 与 832/7140 全绿。
  形成新提交前不能把该结果当作精确 SHA 的 PR CI 证据；
- 尚无目标环境故障注入、治理批准、业务 soak、shadow、canary 或实际回滚证据。

`f04c890d...` 现在是严格门禁修复前的证据基线，不代表已经授权发布，也不应再冻结为最终候选。
当前修复一旦提交，现有 push CI 与手动 Nightly 只能保留为历史证据，必须对新 SHA 重跑阶段 B/C。
候选 SHA 无法在其自身提交内容中自引用，最终 SHA、run URL 与审批记录应同时保存到不改变候选
commit 的外部审计位置。

## 3. 全局不可跳过规则

- [ ] 所有候选证据精确绑定同一个 40 位 `main` SHA。
- [ ] 候选工作树必须干净；不得使用旧 build tree、旧测试二进制或其他 SHA 的 artifact。
- [ ] 发布冻结后只允许修复发布阻塞问题；不得混入新语言特性、调试器扩展或大规模重构。
- [ ] 任一 required job 失败、取消或 unexpected skip 都阻止候选继续推进。
- [ ] 任一新提交都使旧 SHA 的 CI、Nightly、治理 attestation、候选包和目标环境结论失效。
- [ ] `SandboxPolicy` 不能替代进程隔离；不可信脚本必须运行在受 OS CPU/RSS/句柄限制的 worker 中。
- [ ] `lua_Alloc` 当前不能宣称覆盖全运行时 hard heap limit；生产容量和隔离必须依赖 Job Object、cgroup/container、VM 或等价机制。
- [ ] game-server 配置必须保持 runtime compilation、binary chunk、filesystem、process、native modules 与 GC control 默认关闭。
- [ ] 不可信 binary chunk 在完整 CFG verifier 完成前不得进入生产输入面。
- [ ] 不在运行中的 State 上热替换 Runtime；回滚必须切换到上一不可变镜像/SDK。
- [ ] 所有外部审批、故障注入、shadow、canary 和回滚证据必须保存，不能用仓库单测代替。

## 4. 阶段 A：修复合并后遗留问题

目标：形成没有未解决中高风险审查意见、文档与实际测试一致的新候选变更。

### A1. 修复 Nightly fuzz 时长与 job timeout 不一致

问题：`.github/workflows/nightly.yml` 接受每个 fuzz target 最长 1200 秒；六个目标顺序执行需要 120 分钟，当前 `long-fuzz` job 的 timeout 只有 90 分钟，合法输入会在 evidence 写入前被取消。

- [x] 保留 600–1200 秒输入范围，将 timeout 提升到 160 分钟，覆盖最长 120 分钟 campaign 与 40 分钟安装、构建、证据写入和上传余量。
- [x] 默认 600 秒为 60 分钟 campaign，最大 1200 秒为 120 分钟 campaign；合同要求额外预算严格大于 30 分钟。
- [x] 增加 workflow/policy 合同测试，确保“接受的最大输入所需时长 + 30 分钟 < job timeout”。
- [x] 确认 release verifier 对每目标至少 600 秒的要求不被放松。

验收：最大合法输入不会因 job timeout 必然失败；非法输入仍失败关闭；默认与 scheduled 路径保持至少六目标各 600 秒。

### A2. 修复独立 CMake 安装下的 Visual Studio 环境发现

问题：`tools/test_package_build_provenance.ps1` 在 `cl.exe` 不在 PATH 时，从 `cmake.exe` 路径反推 `Common7`。标准 `C:\Program Files\CMake\bin\cmake.exe` 不包含该片段，脚本会在 Ninja provenance 合同执行前失败。

- [x] 当需要 MSVC 环境时，优先使用 `vswhere.exe` 查找带 x64 C++ tools 的 Visual Studio installation path。
- [x] 只有 CMake 确实来自 Visual Studio bundle 时才允许从其路径推导 VS 根目录。
- [x] 为“standalone CMake + cl 不在 PATH + 已安装 Visual Studio”增加正向合同测试。
- [x] 为“找不到 VS/VsDevCmd”保留明确、失败关闭的错误。
- [x] 在本地 standalone CMake 形态路径复验真实 `vswhere` / `VsDevCmd.bat` 导入。
- [x] 在新 SHA 的 GitHub Windows runner 复验。

证据：2026-08-13 在 `cl.exe` 初始不在 PATH 时，以 standalone CMake 形态路径调用真实
`vswhere`/`VsDevCmd.bat`，成功导入 `D:\VS2026\...\cl.exe`；新 SHA 的 push CI run
`31661881457` 中 Windows Debug/Release jobs 均通过包含
`visual_studio_environment_contract` 的合同测试。

验收：standalone CMake 场景能加载正确的 `VsDevCmd.bat` 并执行 single-config Ninja provenance 合同；错误 VS 路径不会被静默接受。

### A3. 消除验证计数与状态文档漂移

- [x] 以已推送基线和当前本地修复的真实状态更新 `README.md`、`assessment.md` 和 RC 说明；未来候选 SHA 仍保持 pending。
- [x] 以真实输出确认 debugger ON：832/7140；debugger OFF：791/6804；新增 PowerShell 合同不改变单元测试计数。
- [x] 更新 `assessment.md` 的 `as_of`、baseline、当前 `main` 状态和接续顺序。
- [x] 高频单元测试计数由 `check_doc_drift.ps1` 动态执行指定测试二进制校验，质量门合同禁止把计数硬编码进检查脚本。
- [x] debugger ON/OFF 文档漂移检查和 `git diff --check` 已通过。

本地实测：debugger ON 为 48/48 CTest、832/7140 单元结果；debugger OFF 为 47/47 CTest、
791/6804 单元结果，全部 0 failures、0 expected skips、0 unexpected skips。

状态复核：以上勾选项记录 Phase A 合并输入在当时的文档一致性。当前修复分支已进一步同步
`README.md`、`assessment.md` 与 RC notes 到 `f04c890d...` 的 CI/Nightly 和严格门禁修复状态，
并增加防陈旧合同；文档漂移、质量门合同与完整 strict 门禁均通过。提交后产生的新 SHA 必须作为
新候选并重跑阶段 B/C。

验收：所有“当前、已通过、候选”声明都有新 SHA 的源码或远端证据；不再引用已失效的合并前状态。

### A4. 提交与审查

- [x] 从最新远端 `main` 创建仅包含 A1–A3 的修复分支 `codex/v0.1.0-rc1-readiness`。
- [x] 本地 diff 审计确认只包含 A1–A3、契约接线与 `task.md` 台账，无运行时功能扩展或无关格式化。
- [x] 创建 PR，等待完整 CI。
- [ ] 至少一名独立审查者检查 Nightly 时间模型、Windows 工具发现和文档一致性。
- [ ] 所有中/高风险 review thread 必须在合并前解决。
- [x] 合并后记录新的 `main` SHA，作为候选冻结输入。

Phase A 历史记录：

```text
PR URL: https://github.com/YanqingXu/lua/pull/20
Reviewer: augmentcode COMMENTED（无 suggestions；不是独立批准）
Merge SHA: f04c890d80a89739eb8dc28ddaeb1ae5e5993273
Merged at: 2026-08-13T02:47:04Z
```

审查缺口：PR #19 的三个历史线程虽已由 PR #20 的实现覆盖，但仍未在 GitHub 标记 resolved；
PR #20 合并前没有可审计的独立批准。进入 RC 前需要独立审查者对当前候选形成书面结论，并明确
记录历史线程的处置方式。

## 5. 阶段 B：冻结最终候选 SHA 并取得完整 CI

目标：选择一个不可变 `main` 提交作为 `v0.1.0-rc.1` 候选，并取得同 SHA 快速门禁证据。

### B1. 本地干净验证

- [ ] 同步远端 `main`，确认本地 HEAD 等于远端 `main`。
- [ ] 记录候选 SHA、tree SHA、工具链版本和验证时间。
- [ ] 从新的构建目录分别验证 debugger ON 与正式包 debugger OFF。
- [ ] 运行严格 changed-scope 质量门；clang-format、clang-tidy 不得因缺工具而被误写为成功。
- [ ] 运行 source-readiness 检查。
- [ ] 确认最终 `git status --short` 为空。

修复前证据基线记录（不满足下一候选的复选框）：

```text
Evidence baseline SHA: f04c890d80a89739eb8dc28ddaeb1ae5e5993273
Evidence baseline tree: d1d5603c53dd22604cddc16afa4f4364a27a27ac
Validation checkout: G:\github\lua\build\codex-b1-f04c890d（独立 clone，detached HEAD）
Toolchains: CMake/CTest 4.3.1-msvc1；Visual Studio 18 2026 18.8.2；MSVC 19.51.36252.0；Windows SDK 10.0.26100.0；PowerShell 7.6.4；Python 3.14.4；Git 2.54.0.windows.1
Fresh build: ON/OFF 均 clean-first Release build 成功；默认 LUA_CPP_BUILD_BENCHMARKS=OFF
Sequential CTest: ON 46/46；OFF 46/46
Full unit results: ON 832/7140；OFF 791/6804；均为 0 failures / 0 expected skips / 0 unexpected skips
Source readiness: passed；evidence SHA-256 48ebbeefcb25fe3ddce50bebf7699a012a6d9a4d0a923aa553dc8413ae1777e0；generated_at 2026-08-13T09:53:20Z
Validation time: 2026-08-13T18:10:00+08:00
Working tree: validation checkout 的最终 git status 为空；主工作树已切到 codex/v0.1.0-rc1-quality-gate，并含 8 个未提交文件
```

失败与归因记录：

- 首次并行执行两套 CTest 时，ON 的 `lua_test` 因共享临时文件
  `lua_cpp_trace_diff_golden_test.jsonl` 被另一进程占用而失败；OFF 的 `official_smoke` 因共享
  `uld8.0` 临时目录发生 `attrib.lua:63` 失败。改为顺序隔离执行后两套均为 46/46，失败标记为
  `superseded-by-sequential-isolated-rerun`；以后不得并行运行这两套本机 CTest。
- LLVM 22.1.3 的原始严格质量门在 clang-tidy step 失败；LLVM 18.1.8 Windows 工具又与
  VS 2026 STL 明确要求 Clang 20+ 不兼容。另行确认 smoke 对 `test_runner.cpp` 缺少
  `LUA_TEST_BUILD_GIT_SHA` 编译定义。使用 `-SkipClangTidy` 的诊断运行只证明其余门禁通过，
  不能勾选严格质量门；权威远端 LLVM 18 lint 已成功。
- 修复分支第一次以带引号的 `quality-gate` 宏值运行原生 LLVM 22 时，Windows 原生命令行剥离了
  引号，`test_runner.cpp:108` 以 `expected expression` 失败；改用标准字符串宏 `__FILE__` 后
  clang-tidy 全部通过。契约测试会捕获并断言该参数，避免以后再次遗漏。
- 一次完整门禁被 60 秒外层命令超时中断，遗留 MSBuild/CL 继续运行；立即重跑时因两个进程争用
  `lua_app\\x64\\Debug\\vc145.pdb` 触发 C1041。等待遗留进程自然结束并确认没有仓库构建进程后，
  串行重跑以 exit 0 完成；该失败标记为 `superseded-by-serialized-rerun`。
- 第二轮 CI follow-up 的首次 strict 启动因新 PowerShell 进程未继承 VS LLVM 目录，在 clang-format
  前置检查立即停止，未执行代码检查。显式把 `D:\VS2026\VC\Tools\Llvm\x64\bin`
  固定到 PATH 后以同一命令串行重跑并通过；该调用环境失败标记为
  `superseded-by-pinned-llvm-rerun`。

修复工作树验证（不能替代新候选 SHA 的干净验证）：

```text
Branch: codex/v0.1.0-rc1-quality-gate（base f04c890d...；head 44d78915... 已推送）
Draft PR: https://github.com/YanqingXu/lua/pull/21
Changed implementation/contracts: .clang-tidy；tools/run_clang_tidy.py；tools/run_quality_gate.ps1；tools/test_quality_gate.ps1
Changed status docs: README.md；assessment.md；docs/release/rc-notes-0.1.0.md；task.md
Contracts: tools/check_doc_drift.ps1 exit 0；tools/test_quality_gate.ps1 exit 0
Strict command: tools/run_quality_gate.ps1 -Strict -FormatScope Changed -FormatBase f04c890d...
Strict tool: D:\VS2026\VC\Tools\Llvm\x64\bin\clang-tidy.exe，LLVM 22.1.3，未使用临时包装器
Strict result: exit 0；clang-tidy / MSBuild / source guards / binary SHA / full unit tests 全部通过
Unit result: 832 registered / 7140 results / 0 failures / 0 expected skips / 0 unexpected skips
Validated at: 2026-08-13T18:36:00+08:00
Initial PR CI: run 31692577961，15/17；Windows Debug/Release PowerShell 5.1 parser failure
First local follow-up: tools/test_quality_gate.ps1 纯 ASCII；Windows PowerShell 5.1 / PowerShell 7 均 exit 0
First follow-up strict: exit 0；LLVM 22 / MSBuild / doc drift / binary SHA / 832/7140 全部通过
First follow-up commit: 44d7891504eb938474ace32f9c99d67455522511；validated at 2026-08-13T19:22:00+08:00
Second PR CI: run 31697950031，15/17；Windows Debug/Release whole-file regex 不兼容 CRLF checkout
Second local follow-up: 断言入口统一正规化 CRLF/CR 为 LF；同函数混合换行自检；脚本纯 ASCII；PS 5.1 / PS 7 / doc drift 均 exit 0
Second follow-up strict: exit 0；LLVM 22.1.3 / MSBuild / doc drift / binary SHA / 832/7140 全部通过
Second follow-up validated at: 2026-08-13T20:38:00+08:00
Next required action: 获得明确授权后提交并推送 task.md、tools/test_quality_gate.ps1，再观察 PR CI
```

参考命令：

```powershell
$candidateSha = (git rev-parse HEAD).Trim()
git status --short

powershell -NoProfile -ExecutionPolicy Bypass -File tools/run_quality_gate.ps1 `
  -Strict -FormatScope Changed -FormatBase <merge-base>

powershell -NoProfile -ExecutionPolicy Bypass -File tools/check_release_readiness.ps1 `
  -ExpectedVersion 0.1.0-rc.1 `
  -ExpectedCommit $candidateSha

cmake -S . -B build/rc-debugger-on -DLUA_CPP_BUILD_DEBUGGER=ON
cmake --build build/rc-debugger-on --config Release --clean-first
ctest --test-dir build/rc-debugger-on -C Release --output-on-failure

cmake -S . -B build/rc-debugger-off -DLUA_CPP_BUILD_DEBUGGER=OFF
cmake --build build/rc-debugger-off --config Release --clean-first
ctest --test-dir build/rc-debugger-off -C Release --output-on-failure
```

### B2. 远端 push CI

- [ ] 候选 SHA 的 `main` push CI 必须成功。
- [ ] 精确包含 17 个预期 jobs，17/17 completed/success。
- [ ] clang-format 和 clang-tidy 两个步骤均真实执行且成功。
- [ ] ASan、UBSan、TSan 无未归因报告。
- [ ] Windows/Linux allocator failure contract 通过。
- [ ] Linux ARM64 与 macOS ARM64 portability 通过。
- [ ] official strict、Lua differential 和 C API differential 通过。
- [ ] `component-coverage` 与 `runtime-benchmark-evidence` artifact 存在、未过期且 SHA 一致。
- [ ] 七组件 coverage 都不低于批准阈值，且 scope/file/coverable-line 最小值没有收缩。
- [ ] benchmark 同时通过相对回归策略与 10 项绝对 SLO。

修复前 CI 证据基线记录（不满足下一候选的复选框）：

```text
Evidence baseline SHA: f04c890d80a89739eb8dc28ddaeb1ae5e5993273
Evidence baseline tree: d1d5603c53dd22604cddc16afa4f4364a27a27ac
CI run URL: https://github.com/YanqingXu/lua/actions/runs/31661881457
CI run ID / attempt: 31661881457 / 1
Coverage artifact ID / digest / expiry: 9166598709 / sha256:78d6ad70be42e273c4f083513829834622a761d0fcbd2c6f974709cc9046508a / 2026-11-11T02:47:07Z
Benchmark artifact ID / digest / expiry: 9166514754 / sha256:203ed769a5e4d01208a08c9ab18361ff8e03c960a66e135dc69a6057cd83e9ef / 2026-11-11T02:47:07Z
Validated by: Codex remote evidence review
Validated at: 2026-08-13T16:46:00+08:00
```

退出条件：候选 SHA 已冻结，B1/B2 全部通过。冻结后如有任何源文件修改，回到阶段 B 起点。

## 6. 阶段 C：恢复 Nightly 并取得同 SHA endurance 证据

目标：取得一次手动 Nightly 和至少一次 scheduled Nightly，二者都绑定候选 SHA。

### C1. 恢复 workflow

- [x] 确认 GitHub Actions billing、spending limit 和 runner 启动能力正常。
- [x] 启用 `.github/workflows/nightly.yml`。
- [x] 确认 schedule 仍为预期时区/UTC 时间，且不会与发布窗口冲突。
- [x] 确认 `workflow_dispatch` 与 `schedule` 使用不同 concurrency group，不会互相取消。

可选命令（需要仓库管理权限）：

```powershell
gh workflow enable nightly.yml --repo YanqingXu/lua
gh workflow view nightly.yml --repo YanqingXu/lua
```

### C2. 手动 Nightly

- [ ] 在候选仍为 `main` HEAD 时运行手动 Nightly。
- [ ] 使用不少于：45 分钟 runtime soak、1000 次 native-module lifecycle、六个 fuzz 目标各 600 秒。
- [ ] dispatch 后立即核对 run 的 `head_sha` 精确等于候选 SHA；不一致则废弃该 run。

参考命令：

```powershell
gh workflow run nightly.yml --repo YanqingXu/lua --ref main `
  -f soak_minutes=45 `
  -f fuzz_seconds_per_target=600 `
  -f native_module_iterations=1000
```

### C3. Scheduled Nightly

- [ ] 等待至少一次 schedule 事件在同一候选 SHA 上完整运行。
- [ ] 在等待期间冻结 `main`；如果 `main` 移动，重新确定候选并回到阶段 B。
- [ ] 确认 scheduled run 不是 0-job startup failure，也没有被 concurrency 取消。

### C4. Nightly 证据复验

- [ ] 手动和 scheduled run 的 `runtime-soak`、`worker-fault-matrix`、`long-fuzz` 全部成功。
- [ ] runtime soak 权威 step 时间不少于 45 分钟。
- [ ] native-module lifecycle 不少于 1000 次。
- [ ] 六个 fuzz 目标各自权威 step 时间不少于 600 秒。
- [ ] 无 crash、timeout、OOM、sanitizer、allocator 非零、取消延迟异常或 corpus 未归因变化。
- [ ] `runtime-soak-evidence` 与 `long-fuzz-evidence` 可下载、未过期、digest 有效、内部 SHA/run/event 参数一致。
- [ ] Windows/Linux worker fault matrix 的 OS-limit 证据已保存。

修复前 Nightly 证据基线记录（不满足下一候选的复选框）：

```text
Manual Nightly run URL / ID / attempt: https://github.com/YanqingXu/lua/actions/runs/31663816824 / 31663816824 / 1
Scheduled Nightly run URL / ID / attempt: pending；首次预期窗口 2026-08-14 02:31 Asia/Shanghai
Runtime-soak artifact ID / digest / expiry: 9168015582 / sha256:0bda44d37e9d54231ab506996537ab6e34293e1044bb4ec0308f3d12953ab9ab / 2026-09-12T04:11:37Z
Long-fuzz artifact ID / digest / expiry: 9168319854 / sha256:bdf0715d33b53691fdf49d09efe31aba95eb7e35f5287c48d224e652d2686f50 / 2026-09-12T04:28:01Z
Worker-fault artifacts: windows-x64 9167268555 / sha256:6e05ac996b18b0a15b827ce0592ad56e604e6f6dc1f0ba1c2ffd91d6da53598c；linux-x64 9167214215 / sha256:a583a405c695f8937736f29d63b68a8fd69032248762a15921b66a30b51b4bfc；均于 2026-09-12 到期
Reviewed by: Codex remote evidence review（manual only；scheduled pending）
```

退出条件：同一候选 SHA 同时拥有合格的 manual 与 scheduled Nightly 证据。

## 7. 阶段 D：完成仓库治理与发布授权

目标：在创建 tag 前建立不能由单一提交者静默绕过的治理证据。

### D1. 选择治理路径

- [ ] 首选：升级套餐或公开仓库，启用 protected ruleset。
- [ ] 备选：仅为当前 RC 签署最长 30 天的结构化、限时豁免。
- [ ] 决策必须有责任人、日期、风险接受、补偿控制和撤销条件。

当前状态：`main` 为 `protected: false`；私有仓库当前套餐读取 branch protection 与 ruleset 均
返回 HTTP 403（需要升级套餐或公开仓库）；Actions repository variables 为 0，issue #6 仍开放。
尚未选择 ruleset 或限时豁免路径。

### D2. 必需控制

- [ ] required CI checks。
- [ ] 合并前分支必须与 `main` 最新状态同步。
- [ ] 禁止 force push 默认分支。
- [ ] 禁止删除默认分支。
- [ ] 限制 release tag 创建。
- [ ] 限制 release tag 删除/移动。
- [ ] 发布批准人与 independent reviewer 必须是不同人员。

### D3. Attestation

- [ ] 按 `tools/verify_release_governance.py` 的严格 schema 生成单行 JSON。
- [ ] 精确绑定 repository、候选 SHA、版本 `0.1.0-rc.1` 和候选/tag 阶段。
- [ ] 填写 `approved_by`、不同的 `independent_reviewer`、批准与失效时间。
- [ ] 记录六项控制状态、审批 URL、风险接受和补偿控制。
- [ ] 在设置仓库变量前本地运行 governance verifier 正负合同。
- [ ] 将批准记录保存到长期审计位置。
- [ ] ruleset 生效或限时豁免完成后关闭/更新 GitHub issue #6。

记录：

```text
Decision: protected-ruleset | time-limited-waiver
Approval record URL:
Approved by:
Independent reviewer:
Valid from / expires at:
Repository variable updated at:
```

退出条件：发布 workflow 能读取合法 attestation；缺失、过期、同人审批或错误 SHA 的负例全部被拒绝。

## 8. 阶段 E：生成并验证 candidate-only 三平台包

目标：在创建 tag 前证明候选 SHA 能从固定 runner 生成可安装、可消费、可审计的正式包。

当前状态：Release workflow 为 `active`，但截至 2026-08-13T16:46:00+08:00 没有真实 run、
candidate artifact、tag 或 GitHub Release。

### E1. 手动 candidate-only release

- [ ] 确认 `main` HEAD 仍等于候选 SHA。
- [ ] 确认 CI、两类 Nightly 和治理证据未过期。
- [ ] 手动运行 release workflow，输入版本 `0.1.0-rc.1`。
- [ ] dispatch 后立即核对 workflow 的 candidate SHA；不一致则取消并回到阶段 B。

参考命令：

```powershell
gh workflow run release.yml --repo YanqingXu/lua --ref main `
  -f version=0.1.0-rc.1
```

注意：manual dispatch 必须保持 candidate-only，不能创建 GitHub Release。

### E2. 平台包验证

- [ ] `windows-x64`：Windows Server 2022、MSVC 19.40–19.x、动态 UCRT/v143。
- [ ] `linux-x64`：Ubuntu 24.04、GCC 14.x、glibc 2.39。
- [ ] `macos-arm64`：macOS 14 deployment target、AppleClang 16.x–17.x。
- [ ] 三个平台均从 clean HEAD 重新 configure、`--clean-first` build、CTest、install、package。
- [ ] `LUA_CPP_BUILD_DEBUGGER=OFF` 被强制执行，调试器实现不进入 0.1.x 包。
- [ ] 每个平台 artifact 精确包含 ZIP、外部 SBOM、package manifest、单包 checksum 四类文件。
- [ ] ZIP 包含静态库、共享库、五个公开头、CMake package 文件、LICENSE、CHANGELOG、SECURITY、RC notes、包内 SBOM 和平台 evidence。
- [ ] validator 重算逐文件 SHA-256、SPDX packageVerificationCode、内外 SBOM 一致性。
- [ ] 下载后在隔离目录构建并运行纯 C static/shared consumer。
- [ ] consumer 不得回退到源码树、CMake registry、默认搜索路径或旧 build cache。
- [ ] Windows DLL、ELF、Mach-O 的运行库、symbol version、minos 和动态依赖满足平台策略。

记录：

```text
Candidate-only release run URL / ID / attempt:
windows-x64 artifact / digest:
linux-x64 artifact / digest:
macos-arm64 artifact / digest:
release-evidence artifact / digest:
Consumer verification jobs:
```

退出条件：candidate-only workflow 全绿，三个包都可下载、可复验、可安装、可被 static/shared consumer 使用。

## 9. 阶段 F：目标环境故障注入与上线前验证

目标：在真实生产镜像、目标硬件、监督器和网络/存储边界下验证仓库测试无法证明的行为。

当前状态：`task.md` 和仓库中没有可审计的目标环境报告、镜像 digest、SLO 记录或 go/no-go
结论；外部若已执行，仍需按本阶段合同补充证据。

### F1. 冻结部署合同

- [ ] 明确生产采用独立 worker process，不把不可信脚本嵌入主服务进程。
- [ ] 明确 Windows Job Object、Linux cgroup/rlimit/container、macOS 外层 VM/container/监督器实现。
- [ ] 固定 worker 镜像 digest、SDK digest、配置 schema、sandbox profile 和允许的脚本来源。
- [ ] 默认禁止任意 native module；若业务必须启用，固定 allowlist、hash/signature 和独立 worker 池。
- [ ] 固定幂等键、重试、结果提交、drain grace 和最大任务时长。
- [ ] 定义 p50/p95/p99、queue time、RSS、allocator peak、取消延迟、重启恢复时间等 SLO。
- [ ] 指定发布负责人、值班负责人和回滚决策人。

### F2. 功能与错误分类矩阵

- [ ] 正常有限任务成功，退出码与结构化 JSON 一致。
- [ ] compile error、runtime error、invalid error object 分类稳定。
- [ ] instruction budget、native-work budget、deadline、cancelled 分类稳定。
- [ ] runtime resource limit 与 allocator limit 分类稳定。
- [ ] 所有失败后 State/worker 能安全关闭，allocator live bytes 归零。
- [ ] 输出为合法、ASCII-safe JSON，字段类型和 schema 固定。

### F3. 破坏性故障注入

- [ ] 把 CPU 限制压到 OS 强杀，监督器稳定记录并映射 `os_cpu_kill`。
- [ ] 把内存/RSS/地址空间限制压到 OS 强杀，稳定映射 `os_memory_kill`。
- [ ] 触发句柄/文件描述符限制，确认 worker 失败关闭。
- [ ] 在任务执行中强杀 worker，验证 drain grace、进程重启、幂等去重和请求重试。
- [ ] 注入宿主 callback 长时间不返回，验证协作轮询或外层强杀。
- [ ] 验证取消到停止延迟，并覆盖最坏脚本形态。
- [ ] 验证 State pool 重用前 `lua_runtime_begin_execution` 重置预算与取消状态。
- [ ] 验证 pool 淘汰与关闭后 allocator live bytes 为 0。
- [ ] macOS 单独验证外层 CPU/RSS/强杀机制，不宣称 `RLIMIT_AS` 等价硬边界。
- [ ] 如启用业务原生模块，验证并发、静态状态、异常、ABI、最后引用、卸载与 module-owned `__gc`。

### F4. 证据与判定

- [ ] 每个场景记录镜像/SDK digest、配置、样本数、退出码、signal/Job Object 原因和 worker JSON。
- [ ] 记录 p50/p95/p99、queue time、RSS、allocator peak、cancellation-to-stop、restart recovery。
- [ ] 所有未归因 crash、leak、OOM、错误分类漂移或重启风暴都阻止进入 RC。
- [ ] 对发现的问题建立 issue、回归测试和责任人；任何代码修复都回到阶段 B。

记录：

```text
Environment / image digest:
Fault-injection report URL:
SLO policy URL:
Observed p50/p95/p99:
Peak RSS / allocator peak:
Worst cancellation latency:
Worst restart recovery:
Go/no-go owner and decision:
```

退出条件：目标环境故障矩阵全绿，所有 OS kill 都能由监督器稳定归因、恢复和审计。

## 10. 阶段 G：创建并发布 `v0.1.0-rc.1`

目标：从已通过 B–F 的同一候选 SHA 创建不可变 RC。

- [ ] 再次确认候选 SHA 位于 `main` 历史，所有证据未过期。
- [ ] 复核 `CHANGELOG.md`、`SECURITY.md`、RC notes、公开 API 计数、ABI 0 和已知限制。
- [ ] RC notes 明确 Runtime Preview、allocator hard-limit 边界、sandbox/OS 隔离边界和平台限制。
- [ ] 创建 annotated tag `v0.1.0-rc.1`，tag 对象 peel 后精确指向候选 SHA。
- [ ] 禁止 lightweight tag；禁止移动、删除或复用已有 tag。
- [ ] tag push 触发 release workflow，并重新运行 governance、exact-SHA evidence、三平台包和下载后 consumer。
- [ ] publish job 重新深验三 RID × 四资产、全局 `SHA256SUMS` 和 release evidence。
- [ ] GitHub Release 必须标记为 prerelease，Release body 动态包含精确 SHA、run URL 和 digest。
- [ ] 从公开 Release 页面重新下载资产并独立校验 checksum 与 consumer。

参考命令：

```powershell
git tag -a v0.1.0-rc.1 <candidate-sha> -m "Lua C++ 0.1.0-rc.1"
git show v0.1.0-rc.1 --no-patch
```

推送 tag 是外部发布动作，执行前必须由发布负责人单独批准。

记录：

```text
Tag object SHA:
Peeled candidate SHA:
Tag workflow URL / ID / attempt:
GitHub prerelease URL:
Global SHA256SUMS digest:
Published by / approved by:
```

退出条件：不可变 prerelease 已发布，所有资产和证据可从 GitHub 下载并独立复验。

## 11. 阶段 H：RC 观察、业务 soak、shadow、canary 与回滚

目标：证明 RC 在真实业务流量、故障域和监督器下可持续运行，并且可以安全撤回。

### H1. 观察期

- [ ] RC 至少观察 7 个自然日。
- [ ] 至少三次连续 scheduled Nightly 全绿。
- [ ] 所有失败 artifact 已归因；无法归因的失败阻止继续。
- [ ] 没有新的高严重度 Runtime、安全、数据竞争、泄漏或 allocator 归零问题。

### H2. 24–72 小时业务 soak

- [ ] 使用生产镜像、目标硬件、真实并发度和脱敏后的真实任务分布。
- [ ] 覆盖短任务、接近预算任务、编译错误、资源拒绝、取消、worker 强杀和重启。
- [ ] 持续记录成功率、错误分类、p50/p95/p99、queue time、RSS、allocator peak、取消延迟和 restart rate。
- [ ] 与上一已验证基线比较，不接受无法解释的性能、内存或错误分类漂移。

### H3. Shadow

- [ ] 复制真实任务到 RC worker，但候选结果不得提交外部副作用。
- [ ] 按任务 ID 比较结果、错误类别、输出截断、预算消费、duration、allocator peak 和 RSS。
- [ ] 任何未归因语义差异、crash、allocator 非零或协议漂移都停止进入 canary。

### H4. Canary

- [ ] 从只有健康检查、无业务流量的 RC 池开始。
- [ ] 预先批准流量阶梯；可参考 1% -> 5% -> 25% -> 50% -> 100%，实际值按故障域确定。
- [ ] 每个阶梯同时规定最小样本数和最短观察时间。
- [ ] 每阶检查成功率、错误分类、p50/p95/p99、queue time、RSS、allocator peak、取消延迟和 worker restart。
- [ ] 未归因结果差异、SLO 越线、crash/leak、allocator 非零、队列增长、重启风暴或 OS kill 无法归因时立即停止扩流。

### H5. 实际回滚演练

- [ ] 保留上一不可变 SDK/worker 镜像和配置。
- [ ] 冻结扩流并把新请求路由回上一版本。
- [ ] 对 RC 池执行有上限的 drain；超过 grace 的任务按幂等合同强杀并重试。
- [ ] 确认基线池成功率、queue、p99、RSS 和失败分类恢复。
- [ ] 保存版本/digest、流量比例、时间窗、停止原因和恢复时间。
- [ ] 回滚演练必须真实执行，不能只审查文档或脚本。

记录：

```text
RC observation start / end:
Scheduled Nightly runs:
Business soak report:
Shadow report:
Canary stages and metrics:
Rollback drill report:
Go/no-go decision URL:
```

退出条件：7 天观察、三次连续 Nightly、24–72 小时业务 soak、shadow、分阶段 canary 和实际回滚全部通过。

## 12. 阶段 I：正式发布 `v0.1.0`

目标：在 RC 已证明稳定后建立正式 API/ABI 基线并发布不可变正式版。

- [ ] 汇总 RC 观察期全部问题；需要修复时发布新的 `rc.N`，不得覆盖 `rc.1`。
- [ ] 明确 0.1.x 公开头、导出符号、CMake target、配置 schema、错误分类与 ABI 0 基线。
- [ ] 确认正式包继续关闭 debugger，且不扩大已声明的平台范围。
- [ ] 更新版本、CHANGELOG、正式 release notes 和支持策略。
- [ ] 版本或文档提交产生新 SHA 后，重新执行阶段 B–E 的同 SHA CI、Nightly、治理和三平台候选包门禁。
- [ ] 对正式 SHA 执行必要的目标环境回归，确认与通过观察期的 RC 无未解释差异。
- [ ] 创建不可变 annotated tag `v0.1.0`。
- [ ] tag workflow 全绿后发布正式 GitHub Release，而不是 prerelease。
- [ ] 独立下载验证三平台资产、SBOM、manifest、checksum 和 static/shared consumer。
- [ ] 发布负责人形成书面 go/no-go，并记录已知限制和回滚版本。

退出条件：`v0.1.0` 正式 Release 可下载、可审计、可消费，生产放量和回滚负责人均已签字确认。

## 13. 发布停止条件

出现以下任一情况立即停止推进；如果需要代码修改，回到阶段 B：

- 任一 required CI/Nightly/release job 失败、取消、unexpected skip 或未真实执行；
- evidence/artifact 缺失、过期、digest 不符、内部 SHA 或参数不一致；
- clang-format/clang-tidy、sanitizer、coverage 或 benchmark 未达到批准策略；
- 未归因 crash、data race、UAF、泄漏、allocator 关闭非零或资源上限绕过；
- target package 不能由隔离 static/shared consumer 构建运行；
- 平台动态依赖、symbol version、CRT 或 deployment target 超出批准基线；
- 仓库治理 attestation 缺失、过期、审批人与审查者相同或 scope 不匹配；
- 不可信输入能够启用 binary chunk、runtime compilation、filesystem、process 或任意 native module；
- p99、RSS、allocator peak、取消延迟、错误分类或重启率出现无法解释的漂移；
- shadow 出现语义差异，或 canary 出现队列增长、重启风暴、SLO 越线；
- 实际回滚不能在批准的恢复时间内完成。

## 14. 最终 Go/No-Go 清单

只有以下项目全部为 `[x]` 才允许正式生产放量：

- [ ] 候选 SHA 已冻结且工作树干净。
- [ ] 候选 SHA 的 17/17 push CI 全绿。
- [ ] 七组件 coverage 与 benchmark 策略通过。
- [ ] 同 SHA manual Nightly 全绿。
- [ ] 同 SHA scheduled Nightly 全绿。
- [ ] required ruleset 或限时治理豁免已批准。
- [ ] candidate-only 三平台包与下载后 consumer 全绿。
- [ ] 目标环境故障注入全绿。
- [ ] `v0.1.0-rc.1` annotated tag 与 prerelease 已发布。
- [ ] RC 观察至少 7 天且至少三次连续 scheduled Nightly 全绿。
- [ ] 24–72 小时业务 soak 通过。
- [ ] shadow 无未归因差异。
- [ ] canary 各阶梯满足样本、时间与 SLO。
- [ ] 实际 drain/rollback 演练通过。
- [ ] 正式 API/ABI 和平台支持边界已冻结。
- [ ] `v0.1.0` 的同 SHA 发布证据全部通过。
- [ ] 发布负责人、独立审查者、值班负责人和回滚负责人完成书面批准。

最终记录：

```text
Final release SHA:
Final tag object SHA:
Release URL:
Production image digest:
Previous rollback image digest:
Release owner:
Independent reviewer:
Operations owner:
Rollback owner:
Go/no-go decision:
Decision time:
```

## 15. 证据台账

执行过程中持续追加，不要删除失败记录；失败记录应标记为 superseded 或 resolved，并链接修复提交。

| 阶段 | SHA/版本 | Run/记录 URL | Artifact/digest | 结果 | 审查人 | 时间 | 备注 |
|---|---|---|---|---|---|---|---|
| A：本地合同初跑 | `d8fff016...` + working tree | local | CTest | resolved | Codex | 2026-08-13 | RC notes 含固定 SHA/run URL，2 个发布正文稳定性合同失败；移除动态字段后复验通过 |
| A：本地全量验证 | `f04c890d...` tree-equivalent pre-merge tree | local | ON 48/48；OFF 47/47；unit 832/7140、791/6804 | passed-tree / exact-SHA B1 pending | Codex | 2026-08-13 | 文档漂移、质量门合同、YAML 解析、PS AST、diff check 通过；最终 merge SHA 的 fresh-build 留证待补 |
| A：严格静态分析 | `d8fff016...` + working tree | local | LLVM 22.1.3 clang-tidy smoke | superseded-by-exact-SHA-CI | Codex | 2026-08-13 | 历史工具对未改动头文件启用新增 portability 检查；未做范围外清理；`f04c890d...` 固定 CI lint 已全绿 |
| A：PR 与合并 | `f04c890d...` | https://github.com/YanqingXu/lua/pull/20 | PR CI 31659922541 | merged-with-review-gap | augmentcode COMMENTED | 2026-08-13 | PR head 与 merge tree 相同；无独立批准；PR #19 三个历史线程仍未标记 resolved |
| B：本地并行 CTest 初跑 | `f04c890d...` | local isolated checkout | ON lua_test；OFF official_smoke | superseded | Codex | 2026-08-13 | 两配置并行争用系统临时文件；ON trace JSONL 被占用，OFF `uld8.0`/attrib 失败；改为顺序重跑 |
| B：本地 fresh build/CTest | `f04c890d...` | local isolated checkout | ON/OFF 46/46；unit 832/7140、791/6804 | passed | Codex | 2026-08-13 | VS 18.8.2 / MSVC 19.51.36252.0；默认 benchmark OFF；顺序执行；最终 checkout 干净 |
| B：source readiness | `f04c890d...` / `0.1.0-rc.1` | local isolated checkout | sha256:48ebbeefcb25fe3ddce50bebf7699a012a6d9a4d0a923aa553dc8413ae1777e0 | passed | Codex | 2026-08-13 | public C API 123/123；project 0.1.0；ABI 0；规范化 evidence 绑定仓库与候选 SHA |
| B：本地严格质量门初跑 | `f04c890d...` | local isolated checkout | LLVM 22.1.3 strict；其余门禁诊断 | superseded-by-local-repair | Codex | 2026-08-13 | 原始 strict 因新增 pragma-once 检查及 smoke 缺少 `LUA_TEST_BUILD_GIT_SHA` 失败；显式 skip-tidy 后其余步骤全绿 |
| B：严格门禁修复迭代 | `f04c890d...` + working tree | local branch | quoted macro / MSBuild C1041 | superseded | Codex | 2026-08-13 | 首次宏值被 Windows 引号规则破坏；一次外层超时遗留编译进程并导致并行重跑 PDB 争用；均已归因并由串行重跑取代 |
| B：严格门禁修复验证 | `f04c890d...` + working tree | local branch | LLVM 22.1.3 strict；832/7140 | passed-working-tree / candidate-pending | Codex | 2026-08-13 | 原生 clang-tidy、MSBuild、SHA 和全量单测 exit 0；质量门合同 exit 0；修复未提交，不能作为 exact-SHA 候选证据 |
| B：状态文档同步 | `f04c890d...` + working tree | local branch | doc drift + stale-status contracts | passed-working-tree | Codex | 2026-08-13 | README/assessment/RC notes 已同步 17/17 CI、manual Nightly success、scheduled=0 与修复待提交；完整 strict 重跑 exit 0 |
| B：修复提交与 PR | `014c69bb...` | https://github.com/YanqingXu/lua/pull/21 | commit pushed | draft-open | Codex | 2026-08-13 | 8 个审计文件已提交并推送；base main；head/base merge state 创建时为 CLEAN |
| B：PR CI 初跑 | `014c69bb...` | https://github.com/YanqingXu/lua/actions/runs/31692577961 | 15/17 jobs | failed-attributed | Codex | 2026-08-13 | Windows Debug/Release 在质量门合同解析失败；UTF-8 无 BOM 中文字面量不兼容 Windows PowerShell 5.1；其余 15 jobs 成功 |
| B：首轮 CI follow-up | `44d78915...` | local + pushed commit | PS 5.1 + PS 7 contracts；strict 832/7140 | passed-local / pushed | Codex | 2026-08-13 | 合同脚本恢复纯 ASCII；两宿主和完整 strict 均 exit 0；修复提交已推送到现有 draft PR |
| B：PR CI 第二轮 | `44d78915...` | https://github.com/YanqingXu/lua/actions/runs/31697950031 | 15/17 jobs | failed-attributed | Codex | 2026-08-13 | Windows Debug/Release whole-file regex 在 CRLF checkout 失败；远端文件内容存在；其余 15 jobs 成功 |
| B：第二轮 CI follow-up | `44d78915...` + working tree | local branch | newline normalization；PS 5.1 + PS 7 contracts；strict 832/7140 | passed-local / awaiting-commit | Codex | 2026-08-13 | whole-file 断言入口与混合换行自检共用 CRLF/CR 到 LF 正规化；LLVM 22.1.3 完整 strict exit 0；首次启动缺 LLVM PATH 已由固定工具链重跑取代；待授权提交 |
| B：CI | `f04c890d...` | https://github.com/YanqingXu/lua/actions/runs/31661881457 | coverage 9166598709；benchmark 9166514754 | passed 17/17 | Codex evidence review | 2026-08-13 | exact-SHA push CI；lint/sanitizers/portability/differentials/coverage/benchmark 全绿 |
| C：Manual Nightly | `f04c890d...` | https://github.com/YanqingXu/lua/actions/runs/31663816824 | runtime 9168015582；fuzz 9168319854；workers 9167268555/9167214215 | passed | Codex evidence review | 2026-08-13 | attempt 1；45m/1000/6x600s；4 artifacts 未过期且 SHA 一致 |
| C：Scheduled Nightly | `f04c890d...` | pending | pending | pending | pending | pending | 首次预期 schedule 窗口 2026-08-14 02:31 Asia/Shanghai；期间冻结 main |
| D：Governance | `f04c890d...` | https://github.com/YanqingXu/lua/issues/6 | repository variables: 0 | blocked-decision | pending | 2026-08-13 | main unprotected；私有仓库当前套餐 protection/ruleset API 返回 403；需 ruleset 或限时 waiver |
| E：Candidate packages | pending | pending | pending | pending | pending | pending | 3 RIDs |
| F：Fault injection | pending | pending | pending | pending | pending | pending | target environment |
| G：RC release | pending | pending | pending | pending | pending | pending | v0.1.0-rc.1 |
| H：Business soak | pending | pending | pending | pending | pending | pending | 24-72h |
| H：Shadow/canary | pending | pending | pending | pending | pending | pending | staged traffic |
| H：Rollback | pending | pending | pending | pending | pending | pending | actual drill |
| I：Final release | pending | pending | pending | pending | pending | pending | v0.1.0 |
