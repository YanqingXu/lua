---
status: current
verified_against: CMakeLists.txt; src/lua_cpp_version.h; CHANGELOG.md; SECURITY.md; .github/workflows/ci.yml; .github/workflows/nightly.yml; .github/workflows/release.yml; docs/release/platform-support.md; docs/release/platform-baseline.json; cmake/LuaCppPlatformBaseline.cmake; tools/verify_platform_baseline.py; tools/test_verify_platform_baseline.py; tools/check_release_readiness.ps1; tools/release_identity.psm1; tools/test_release_identity.ps1; tools/verify_source_readiness_evidence.py; tools/test_verify_source_readiness_evidence.py; tools/verify_release_governance.py; tools/test_verify_release_governance.py; tools/write_workflow_evidence.py; tools/test_write_workflow_evidence.py; tools/verify_release_evidence.py; tools/test_verify_release_evidence.py; tools/build_release_body.py; tools/test_build_release_body.py; tools/verify_release_tag.py; tools/test_verify_release_tag.py; cmake/WriteBuildProvenance.cmake; tools/build_provenance.psm1; tools/visual_studio_environment.psm1; tools/test_visual_studio_environment.ps1; tools/test_package_build_provenance.ps1; tools/package_release.ps1; tools/generate_sbom.py; tools/validate_release_artifacts.py; tools/verify_release_package_consumer.py; tools/test_verify_release_package_consumer.py; tests/packaging/consumer/CMakeLists.txt; tests/packaging/consumer/main.c
last_checked: 2026-08-13
applies_to: 0.1.x release candidates and releases
---

# RC 与正式发布门禁

发布是对同一个不可变提交的证据汇总，不是“本地看起来通过”后直接打 tag。

## 进入候选前

- `main` 工作树干净，版本在 CMake 与 `lua_cpp_version.h` 完全一致。
- 当前 SHA 的 Windows/Linux/macOS/ARM64 构建、官方 strict、C API、安装消费者、sanitizer、allocator、coverage、benchmark 与 lint required checks 全绿。
- 当前 SHA 的 nightly runtime/native-module soak 与长 fuzz 全绿；artifact 未被其他 SHA 替代。
- coverage 每个组件达到 `tests/coverage/component-thresholds.json`。
- head benchmark 同时满足相对回归策略和 `runtime-benchmark-absolute-policy.json`。
- `CHANGELOG.md`、候选说明、公开 API 计数、ABI 版本和已知限制已复核。
- 没有未归因 crash、数据竞争、泄漏、allocator 归零失败或高严重度 Runtime 报告。

## Exact-SHA 自动证据

`.github/workflows/release.yml` 的 `verify_evidence` job 是所有平台 `packages` job 的前置依赖。
它只授予 `contents: read` 与 `actions: read`，并运行
`tools/verify_release_evidence.py`。任何 API 错误、分页不完整、字段缺失或证据歧义都会失败关闭。

脚本必须同时证明：

- 候选是 `main` 历史中的完整 40 位提交；
- 同一 SHA、`main` 分支、push 事件的最新完整 CI run 成功；
- CI 精确包含 17 个预期 job，全部为 completed/success，lint 中 clang-format 与
  clang-tidy 两个步骤均真实执行并成功；
- `component-coverage` 与 `runtime-benchmark-evidence` artifact 存在、未过期，属于该 run/SHA，
  且 GitHub 提供有效的 SHA-256 digest；
- 同一 SHA 的 workflow_dispatch nightly 与 scheduled nightly 均成功，各自包含
  runtime/native-module soak 和 long sanitizer fuzz 两个成功 job；
- 两个 nightly run 各自具有 `runtime-soak-evidence` 与 `long-fuzz-evidence`，并满足相同的
  run/SHA、过期时间和 SHA-256 digest 合同；
- verifier 使用认证请求下载每个 artifact ZIP，拒绝绝对路径、`..`、重复 basename、重复 JSON
  key 与非标准数值，并复算下载字节的 GitHub SHA-256 digest；
- 内部元数据的 schema、kind、repository、candidate SHA、run ID/attempt、event、workflow、
  job、passed result 和时间必须精确绑定选中的 run；CI 参数必须为空；
- coverage ZIP 必须包含原始 `llvm.coverage.json.export`、固定阈值文件、组件摘要和 HTML。
  verifier 从每个原始 file summary 重新聚合七组件 line coverage，要求阈值文件与仓库批准策略
  精确一致，并用最小文件数/可覆盖行数拒绝伪造的 scope 收缩；
- benchmark ZIP 必须包含 comparison、run-order 及每次 base/head 原始结果。verifier 重新计算
  median、paired regression 与合并 GC pause 的 p99/max，要求 base 是 candidate 的严格祖先，
  并从 GitHub commit/root-tree API 独立证明 `CMakeLists.txt`、`cmake/`、`src/` 的对象身份。
  每个 head 原始结果还必须满足 Linux/ci/Release 固定范围内的 10 项绝对 SLO；
- 两类 nightly 内部参数必须分别证明至少 45 分钟 runtime soak、1000 次 native-module
  lifecycle，以及 `undump`、`bytecode_verifier`、`parser`、`stdlib_numeric_arguments`、
  `remote_protocol`、`debugger_expression`
  六个目标各至少 600 秒 fuzz。机器 JSON/log/corpus 必须与声明一致，且 GitHub jobs API 中
  对应 step 的真实开始/结束时间必须达到下限；自报时长不能替代权威 step 时间。

成功后上传 `release-evidence` Actions artifact，其中的 `release-evidence.json` 记录 schema、
候选 SHA、生成时间、run ID、attempt、URL、job、artifact ID/digest/创建与过期时间、已验证的
内部参数、payload 复算结果与权威 timed-step。历史 SHA 的绿色 run、metadata-only ZIP、自报
成功但机器结果矛盾的 payload，或其他 artifact 都不得替代当前候选证据。

publish job 还要用 `tools/build_release_body.py` 对 verifier 的真实 manifest 输出做第二次完整
schema/payload/run-set/timed-step 校验。它只接受
`windows-x64`、`linux-x64`、`macos-arm64` 各一套四个规范资产与一份
`release-evidence.json`，逐包重验 manifest 的 version/RID/commit、单包 checksum、ZIP 与包内/
包外 SBOM，再校验全局 `SHA256SUMS` 精确覆盖该文件集。顶层字段或后缀数量浅检查不构成发布
证据。

tracked `docs/release/rc-notes-0.1.0.md` 只保存稳定叙述、兼容范围、已知限制和所需证据类型，
不得写入候选 SHA、run ID 或 URL；把这些值回写后会改变被描述的提交。verifier 会拒绝
TODO/TBD、未发布说明、模板文字和其他占位符。publish job 从 manifest 动态追加精确 SHA 与
run 链接，因此最终 tag 的 Release body 不含占位内容。

## 仓库治理

tag 发布路径默认失败关闭。仓库变量 `LUA_RELEASE_GOVERNANCE_ATTESTATION` 必须是一行严格 JSON，
并精确绑定 repository、40 位候选 SHA、tag、version、`approved_by`、不同的
`independent_reviewer`、批准/失效时间和本仓库的 GitHub 审批记录 URL。decision 只能是：

1. `protected-ruleset`：六项控制全部为 `enforced`；或
2. `time-limited-waiver`：最长 30 天，至少一项为 `waived`，并提供非空风险接受与补偿控制。

六项控制是 required CI、合并前分支最新、默认分支禁止 force push/删除、tag 创建/删除限制。
旧的永久布尔变量不能授权发布。tag push 会把规范化治理记录写入 release evidence；手动
workflow dispatch 即使从 tag ref 启动也只能生成 `candidate-only` 证据和候选包，`verify_tag`
与 `publish` 还会独立要求事件本身是 tag push。

## 制品

三个正式 RID 必须满足
[`platform-baseline.json`](platform-baseline.json) 的固定合同：`windows-2022`/MSVC
19.40–19.x/动态 UCRT v143，`ubuntu-24.04`/GCC 14.x/glibc 2.39，以及
`macos-15`/AppleClang 16.x–17.x/`CMAKE_OSX_DEPLOYMENT_TARGET=14.0`。release configure
必须传入精确 RID 与 runner，生成 platform evidence；构建后 verifier 再从 DLL/ELF/Mach-O
检查 CRT、symbol-version 上限、`minos` 和动态依赖。任何 `latest` runner、MinGW、32 位、
musl 或 CI-only Linux ARM64 都不能生成 0.1.x 官方包。

每个平台包必须包含：

- 静态库、共享库、五个公开头、CMake package 文件、LICENSE；
- `CHANGELOG.md`、`SECURITY.md` 与发布说明；
- 包内 SPDX 2.3 JSON SBOM；
- 包外同名 SBOM 与 SHA-256 清单。

候选包由 `tools/package_release.ps1` 从 CMake install tree 生成。打包器先要求工作树干净且
build provenance v2 的 HEAD、源码/构建目录、生成器、目标 OS/CPU、64 位指针和单/多配置类型
与请求完全一致；RID 只允许 `windows-x64`、`linux-x64`、`macos-arm64`。随后从当前 clean
HEAD 重新 configure、`--clean-first` 构建并复查 provenance/工作树，防止旧 SHA、脏源码产物、
Debug SDK、32 位目标或 RID 冒名被误标为 Release。之后调用
`tools/validate_release_artifacts.py` 验证归档根路径、必需文件、manifest、外部/包内 SBOM
一致性、SBOM 文件全集、逐文件 SHA-256 与 SPDX `packageVerificationCode`。发布 workflow
还必须在该 clean rebuild/打包之后重新运行完整 Release CTest，成功后才能上传制品，并
按精确 artifact 名隔离下载每个 RID，拒绝额外文件、符号链接、缺失项和目标文件碰撞，并在
publish runner 上再次调用同一深度 validator。每个平台 artifact 上传完成后，独立矩阵 job
会在相同 RID runner 下载精确四文件集合；consumer verifier 将 ZIP 安全解压到临时目录，把
可信 consumer 源码复制到仓库外的新目录，清除 CMake prefix/project hook 环境，关闭 package
registry/default-path 查找，并核对 `CMakeCache.txt` 中的 `LuaCpp_DIR` 精确指向该解压包。
静态和共享纯 C consumer 必须同时被 CTest 发现、全新构建并运行，缺少任一测试或任何回退都
失败关闭。之后才把 `release-evidence.json` 与平台文件
汇总为不可变 release asset，重新计算并检查实际发布 asset 的全局 `SHA256SUMS`。该索引明确
排除自身；Release body 在 checksum 完成后动态生成，不是 release asset，因此不会形成
checksum 自引用。
本地打包阶段也执行同一个 ZIP consumer verifier，避免只有远端下载路径才暴露归档可消费性。

## Tag 与放量

1. 在绿色 `main` SHA 上运行手动候选打包，验证全部压缩包。
2. 创建 annotated `v0.1.0-rc.N` tag；tag 不移动、不复用。tag job 与最终发布命令紧邻前都要
   通过 GitHub Git refs API 验证：顶层对象必须是 annotated tag，递归 peel 后必须精确等于
   evidence-approved candidate；lightweight、错误提交、循环/过深对象链或 API 错误均拒绝。
3. tag workflow 在相同 SHA 重跑 `verify_evidence`；三个 package job 必须等待它成功。
4. publish job 深度校验完整 manifest 与发布资产集合，把精确 run/payload 摘要和实际 package/
   SBOM/manifest SHA-256 追加到最终 Release body，再生成 prerelease。
5. 在目标服务执行 shadow/canary，比较结果一致性、失败分类、p99、allocator peak、进程内存与取消延迟。
6. 修复必须进入新提交和新 RC；不得覆盖已有制品。
7. 正式 `v0.1.0` 仅从已通过 RC 的后继绿色 SHA 创建。

## 回滚

保留上一已验证 SDK/worker 镜像与配置。出现语义差异、崩溃、资源水位、延迟或错误分类异常时，停止扩大流量，drain 当前 worker，切回上一不可变版本。不要在运行中的 State 上替换共享库，也不要移动 tag。
