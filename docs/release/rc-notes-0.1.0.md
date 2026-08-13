---
status: current
verified_against: CMakeLists.txt; src/lua_cpp_version.h; src/lua_runtime.h; CHANGELOG.md; .github/workflows/ci.yml; .github/workflows/nightly.yml; .github/workflows/release.yml; docs/release/platform-support.md; docs/release/platform-baseline.json; cmake/LuaCppPlatformBaseline.cmake; tools/verify_platform_baseline.py; tools/write_workflow_evidence.py; tools/verify_release_evidence.py; tools/verify_source_readiness_evidence.py; tools/verify_release_governance.py; tools/verify_release_tag.py; tools/build_release_body.py; tools/validate_release_artifacts.py; tools/release_identity.psm1; cmake/WriteBuildProvenance.cmake; tools/package_release.ps1; docs/release/release-checklist.md; docs/operations/production-deployment.md; docs/quality/endurance.md
last_checked: 2026-08-13
applies_to: 0.1.x release candidates and releases
---

# Lua C++ 0.1.0 Runtime Preview

## 重点

- Lua 5.1.5 官方 123/123 C API 合同通过。
- 安装后静态/共享 CMake SDK 与纯 C consumer。
- `lua_runtime.h` 生产配置：game-server sandbox、执行/资源/编译上限、取消与 metrics。
- 参考 worker 的 allocator/进程限制与结构化结果。
- sanitizer、fuzz、coverage、soak、benchmark 和 native-module 生命周期门禁。

## 兼容性

- SDK 版本：0.1.0。
- shared-library ABI：0。
- 发布 RID：Windows Server 2022 x64、Ubuntu 24.04/glibc 2.39 x64、macOS 14.0+
  ARM64；对应动态 UCRT/MSVC v143、GCC 14/libstdc++ 与系统 libc++。
- 默认 `lua_open` / `luaL_newstate` 保持 unrestricted Lua 5.1 行为。
- 有限行为必须通过 `luaL_newstate_configured` 显式选择。

## 已知限制

- 项目仍是 Runtime Preview，不宣称全运行时 callback allocator hard limit。
- 长时间不返回的原生 callback 必须主动调用 `lua_checkexecution`，或由宿主终止 worker。
- sandbox 不约束恶意宿主或已加载原生代码。
- macOS 进程硬内存边界需由部署环境提供。
- MinGW、32 位、musl、macOS x64 和较旧运行库不在 0.1.x 官方二进制支持范围；Linux
  ARM64 仅为 CI portability 目标。

## 验证

截至 2026-08-13，已推送基线的常规 CI、七组件 coverage 和 runtime benchmark 已通过；其后
的 Phase A 修复尚未形成候选 SHA。Nightly 仍为 `disabled_manually`，治理、三平台候选包、
tag 和 GitHub Release 仍未完成；因此本文件描述门禁合同，不构成 RC 已获批准的声明。精确
SHA、run URL 和 artifact 标识只由发布工作流从候选证据动态生成，不写入可复用的 RC 正文。

发布门禁把以下远端证据绑定到同一个不可变提交：

- 完整 CI push run 的 17 个 required jobs，包括真实执行并成功的 clang-format 与 clang-tidy；
- `component-coverage` 覆盖率证据与 `runtime-benchmark-evidence` 性能证据；
- 手动和 scheduled nightly 各自成功的 runtime/native-module soak 与长时 sanitizer fuzz；
- `runtime-soak-evidence` 和 `long-fuzz-evidence` 两类 nightly artifact。

tag workflow 在打包前生成 `release-evidence.json`，其中记录候选 SHA、run ID、attempt、URL、
artifact ID、GitHub SHA-256 digest、创建时间和过期时间。平台包生成后再计算包、SBOM、
package manifest 与 checksum 文件的实际 SHA-256。verifier 会认证下载并安全解析每个证据 ZIP、
复算 GitHub digest，再从原始 LLVM coverage、逐次 benchmark sample、runtime JSON、fuzz log/
corpus 和 GitHub job step 时间重新判断结果。benchmark base 必须是候选的严格祖先，运行时输入
还会与 GitHub Git tree 对象核对，每个 head 结果须满足固定 10 项绝对 SLO。

打包器以 build provenance 绑定 clean HEAD、源码/构建目录和 Release 配置，并在 install 前执行
重新 configure 与 clean rebuild；provenance v2 同时绑定目标 OS、CPU、64 位指针宽度和精确
RID，只接受 `windows-x64`、`linux-x64` 与 `macos-arm64`。三个 artifact 会下载到隔离目录，
每个平台只允许包、SBOM、manifest 和 checksum 四个规范文件；最终 consumer 会重新验证
version、RID、commit、ZIP payload、单包 checksum 及内外 SBOM 一致性，再验证全局 checksum。

source-readiness evidence 会把版本、ABI 与候选提交绑定；治理证据要求结构化 attestation，
包含批准人、独立审查者、六项控制、审计记录 URL 与有效期。manual dispatch 始终只能生成
候选包，只有指向同一候选 SHA 的 annotated tag push 才能进入发布路径。摘要和实际 checksum
清单只会动态追加到 GitHub Release body，不会回写本文件，因此不会改变正在验证的候选提交，
也不会让 checksum 文件包含自身。
