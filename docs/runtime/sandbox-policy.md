---
status: current
verified_against: src/runtime/sandbox_policy.hpp; src/runtime/runtime_services.hpp; src/runtime/native_module_registry.cpp; src/vm/state/global_state.cpp; src/vm/state/lua_state.cpp; src/lib/lib_manager.cpp; src/lib/baselib.cpp; src/lib/iolib.cpp; src/lib/oslib.cpp; src/lib/packagelib.cpp; src/lib/debuglib.cpp; tests/unit/vm/test_runtime_services.cpp
last_checked: 2026-07-15
applies_to: context-owned standard-library exposure and script-visible filesystem, process, and native-module capabilities
---

# SandboxPolicy 脚本能力合同

`SandboxPolicy` 属于一个 `EngineContext` 的 `GlobalState`，主 State 和所有 coroutine 共享同一份策略。它控制两层边界：标准库是否能够发布到脚本环境，以及已经发布或捕获的特权函数是否仍可执行文件系统、进程或原生模块操作。

默认策略是 `SandboxProfile::unrestricted()`，保持现有 Lua 5.1 标准库行为。面向不可信游戏逻辑的预置策略是 `SandboxProfile::gameServer()`：仅开放 base、math、string、table、coroutine 和 package，禁用 io、os、debug，并把 package 限制为宿主 preload 模块。

## 配置顺序

宿主必须在 owner thread、且没有 Lua 代码执行时配置策略：

```cpp
EngineContext context;
context.sandboxPolicy().configure(SandboxProfile::gameServer());

UPtr<LuaState> state = LuaState::create(context);
StandardLibrary::openAll(state.get());
```

库暴露在打开时判定。若某个库被禁用，`openAll` 会跳过它，直接 `luaopen_*` 也会在发布全局表之前失败；`luaopen_base` 会先同时检查 base 和 coroutine，避免只发布一半。策略后来收紧时不会删除已经发布的表或函数，因此需要“从未可见”的宿主必须先配置再开库。

能力判定同时发生在每次特权操作入口。脚本即使在收紧策略前保存了 `loadfile`、`io.open`、`os.execute` 或 `package.loadlib`，之后调用仍会得到拒绝。文件句柄的 `close` 和 GC 清理始终保留，避免撤销能力后无法释放已有资源。

## 能力矩阵

| 控制项 | 允许时 | 拒绝时 |
|---|---|---|
| 标准库集合 | 对应库可由 catalog 或公开 opener 注册 | 库表和函数不发布；直接 opener 原子失败 |
| `Filesystem` | base 文件加载、io 文件操作、os 文件操作、package Lua 文件搜索 | `loadfile`/`dofile` 不注册；相关 io/os/package 操作在入口拒绝 |
| `Process` | `io.popen`、`os.execute/exit/getenv/setlocale`、`debug.debug` | 相应函数不注册；已捕获函数在入口拒绝 |
| `NativeModules` | `package.loadlib`、C/C-all-in-one searcher、context module registry 新加载 | `package.loadlib` 和 C searcher 不注册；registry 在 OS loader 前拒绝 |

在 game-server profile 中，`package.path` 与 `package.cpath` 为空，`package.loaders` 只保留 preload searcher。宿主写入 `package.preload` 的模块仍可由 `require` 加载；这些回调与其闭包捕获的宿主能力被视为可信代码。

## 稳定错误

四类拒绝使用 context 初始化时预分配并 fixed 的 Lua 字符串，不在拒绝路径临时分配：

- `sandbox: standard library disabled`
- `sandbox: filesystem access denied`
- `sandbox: process access denied`
- `sandbox: native module access denied`

Lua protected call 返回 `LUA_ERRRUN` 并保留对应错误对象；内部可抛入口使用携带同一 `Value` 的 `RuntimeError`。拒绝发生在文件、进程或 OS loader 副作用之前。

## 信任与非目标

该策略是脚本能力边界，不是操作系统容器，也不防御恶意宿主或已经加载的原生代码：

- 公开 C API 的 `luaL_loadfile` 是可信宿主入口，不受脚本文件系统能力控制；脚本可见的 base `loadfile`/`dofile` 受控。
- 宿主注册的 C callback 可以直接调用操作系统或公开 C API；它必须自行遵守宿主安全合同。
- 禁止 `NativeModules` 只阻止新的动态加载，不能撤销已执行模块代码、已有函数指针或动态库静态状态。
- 多 context 之间策略与 module registry 独立，但动态库的进程静态数据仍可能由 OS loader 共享。

owner-thread、指令预算、deadline、取消和 finalizer 预算见 [ExecutionPolicy](execution-policy.md)。完整 allocator hard limit 仍是独立的未完成边界，见 [内存合同](memory-contract.md)。
