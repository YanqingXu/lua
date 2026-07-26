---
status: current
verified_against: docs/release/platform-baseline.json; CMakeLists.txt; cmake/LuaCppPlatformBaseline.cmake; cmake/WriteBuildProvenance.cmake; .github/workflows/ci.yml; .github/workflows/release.yml; tools/verify_platform_baseline.py; tools/test_verify_platform_baseline.py; tools/package_release.ps1; tests/packaging/consumer/
last_checked: 2026-07-26
applies_to: 0.1.x source builds, release packages, and installed SDK consumers
---

# 0.1.x 平台、运行库与工具链基线

本页区分“源码可构建”“CI 可移植性检查”和“发布包支持”三个不同承诺。只有
[`platform-baseline.json`](platform-baseline.json) 中的三个 RID 会生成 0.1.x 官方候选包；
`latest` runner、未记录的本地编译器结果或只通过源码编译，都不能扩大正式支持范围。

## 官方发布包

| RID | 最低运行环境 | 固定构建环境 | 运行库合同 |
|---|---|---|---|
| `windows-x64` | Windows Server 2022 x64，版本 `10.0.20348` | `windows-2022`、Visual Studio 17 2022、MSVC 19.40–19.x | 动态 Universal CRT 与 MSVC v143；包的共享库不得导入 Debug CRT |
| `linux-x64` | Ubuntu 24.04 LTS x64、glibc 2.39 | `ubuntu-24.04`、Ninja、GCC 14.x | 动态 glibc/libstdc++；最高允许 `GLIBC_2.39`、`GLIBCXX_3.4.33`、`CXXABI_1.3.15` |
| `macos-arm64` | macOS 14.0 ARM64 | `macos-15` ARM64、Ninja、AppleClang 16.x–17.x | `CMAKE_OSX_DEPLOYMENT_TARGET=14.0`，仅依赖系统 libc++/libSystem |

Windows 客户端版本、较旧 Linux 发行版/glibc、musl、macOS x64 和 32 位目标未进入本次
发布矩阵，不能仅凭“可能可运行”写成受支持。Linux ARM64 目前只属于 CI portability 检查，
不生成 0.1.x 包。

## 源码和 consumer

- 工程本身要求 CMake 3.20+、C99 与 C++23。Windows 路径只支持 MSVC ABI；MinGW 会在
  configure 阶段直接失败，不再允许走到 `_dupenv_s` 链接失败。
- CI 仍用 MSVC、GCC、Clang/LLVM libc++ 与 AppleClang 检查源码可移植性；这类检查不会自动
  产生新的正式 RID。
- 已安装 SDK 的公开头可由纯 C consumer 编译。`LuaCpp::Lua` 静态库内部由 C++ 实现，因此
  最终链接器和运行库必须与对应平台包兼容；`LuaCpp::Shared` 仍需要同 RID 的系统动态运行库。
- 官方 ZIP 的 static/shared consumer 必须在同 RID 的全新目录中构建并运行，且不得回退到
  源码树、CMake package registry 或旧构建缓存。

## 机器验证

`CMakeLists.txt` 的 `LUA_CPP_RELEASE_RID` 与 `LUA_CPP_RELEASE_RUNNER` 只用于规范候选包。
启用后，`cmake/LuaCppPlatformBaseline.cmake` 会拒绝错误 OS/CPU/位宽、浮动 runner、错误
generator/编译器、错误 glibc、静态 MSVC CRT 或错误 macOS deployment target，并生成
`lua-cpp-platform-evidence.json`。

构建共享库后必须再运行：

```text
python3 tools/verify_platform_baseline.py \
  --policy docs/release/platform-baseline.json \
  --evidence build/release/lua-cpp-platform-evidence.json \
  --expected-rid <rid> \
  --shared-library <built-shared-library>
```

验证器会再次检查策略与 evidence 的严格 schema，并用 `dumpbin`/`llvm-readobj`、`readelf`
或 `otool` 从真实共享库复核 CRT、ELF symbol-version 上限、Mach-O `minos` 与动态依赖。
策略和 evidence 会装入 ZIP 的 `share/lua_cpp/release/`，供下载后审计；实际远端 runner、
编译器补丁版本和依赖输出仍必须绑定最终候选 SHA 保存，不能由本页替代。
