---
status: current
verified_against: CMakeLists.txt; src/lua_cpp_version.h; CHANGELOG.md; SECURITY.md; .github/workflows/ci.yml; .github/workflows/nightly.yml; .github/workflows/release.yml; tools/check_release_readiness.ps1; tools/package_release.ps1; tools/generate_sbom.py; tools/validate_release_artifacts.py
last_checked: 2026-07-24
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

## 仓库治理

tag 发布路径默认失败关闭。`.github/workflows/release.yml` 只在仓库变量 `LUA_RELEASE_GOVERNANCE_APPROVED=true` 时允许 tag 发布；该变量只能在以下之一有书面记录后设置：

1. 默认分支保护/ruleset 已要求 CI required checks、禁止直接破坏性更新并限制 tag；或
2. 仓库所有者对当前平台套餐限制作出明确、限时、可审计的发布豁免。

当前已知平台套餐不能配置原计划的保护规则，因此在完成上述决策前不得创建 RC tag。手动 workflow dispatch 只能生成候选包供验证，不会发布 release。

## 制品

每个平台包必须包含：

- 静态库、共享库、五个公开头、CMake package 文件、LICENSE；
- `CHANGELOG.md`、`SECURITY.md` 与发布说明；
- 包内 SPDX 2.3 JSON SBOM；
- 包外同名 SBOM 与 SHA-256 清单。

候选包由 `tools/package_release.ps1` 从 CMake install tree 生成，并在完成前调用
`tools/validate_release_artifacts.py` 验证归档根路径、必需文件、manifest、外部/包内 SBOM
一致性、SBOM 文件全集、逐文件 SHA-256 与 SPDX `packageVerificationCode`。发布 workflow
重新计算各平台及全局 `SHA256SUMS`；
下载后仍应独立复算并运行一个安装后静态/共享 consumer。

## Tag 与放量

1. 在绿色 `main` SHA 上运行手动候选打包，验证全部压缩包。
2. 创建 annotated `v0.1.0-rc.N` tag；tag 不移动、不复用。
3. tag workflow 在相同 SHA 重跑发布门禁并生成 prerelease。
4. 在目标服务执行 shadow/canary，比较结果一致性、失败分类、p99、allocator peak、进程内存与取消延迟。
5. 修复必须进入新提交和新 RC；不得覆盖已有制品。
6. 正式 `v0.1.0` 仅从已通过 RC 的后继绿色 SHA 创建。

## 回滚

保留上一已验证 SDK/worker 镜像与配置。出现语义差异、崩溃、资源水位、延迟或错误分类异常时，停止扩大流量，drain 当前 worker，切回上一不可变版本。不要在运行中的 State 上替换共享库，也不要移动 tag。
