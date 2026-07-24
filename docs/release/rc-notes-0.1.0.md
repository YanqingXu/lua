---
status: current
verified_against: CMakeLists.txt; src/lua_cpp_version.h; src/lua_runtime.h; CHANGELOG.md; docs/release/release-checklist.md; docs/operations/production-deployment.md; docs/quality/endurance.md
last_checked: 2026-07-24
applies_to: unpublished 0.1.0 release candidates
---

# Lua C++ 0.1.0 Runtime Preview

这是首个候选发布说明模板；在 RC tag 创建前，状态保持为“未发布”。

## 重点

- Lua 5.1.5 官方 123/123 C API 合同通过。
- 安装后静态/共享 CMake SDK 与纯 C consumer。
- `lua_runtime.h` 生产配置：game-server sandbox、执行/资源/编译上限、取消与 metrics。
- 参考 worker 的 allocator/进程限制与结构化结果。
- sanitizer、fuzz、coverage、soak、benchmark 和 native-module 生命周期门禁。

## 兼容性

- SDK 版本：0.1.0。
- shared-library ABI：0。
- 默认 `lua_open` / `luaL_newstate` 保持 unrestricted Lua 5.1 行为。
- 有限行为必须通过 `luaL_newstate_configured` 显式选择。

## 已知限制

- 项目仍是 Runtime Preview，不宣称全运行时 callback allocator hard limit。
- 长时间不返回的原生 callback 必须主动调用 `lua_checkexecution`，或由宿主终止 worker。
- sandbox 不约束恶意宿主或已加载原生代码。
- macOS 进程硬内存边界需由部署环境提供。

## 验证

正式 RC 说明必须补入候选 SHA、CI run、nightly run、长 fuzz、benchmark artifact、每个平台包名、SHA-256 与 SBOM 文件。
