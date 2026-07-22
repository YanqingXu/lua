# 当前项目进展评估

评估快照为本地 `HEAD 3346124` 加本工作树中的收敛修复。基准提交对应的 [Actions run 29651528378](https://github.com/YanqingXu/lua/actions/runs/29651528378) 在 17 个矩阵实例中仅 allocator Linux lane 通过；本工作树已经按日志逐项修复共同根因和平台问题，但新的远端矩阵尚未运行，因此本文严格区分“本地通过”和“线上待验证”。

当前最准确的项目定位是：

> 现代 C++23 Lua 5.1 高兼容 Runtime Preview，已经具备完整公开 C API、官方套件证据、受控运行时边界和可消费的 0.1.0 CMake SDK；尚不能宣称全运行时 allocator hard limit 或生产级跨平台发布。

## 一、当前结论

| 维度 | 当前状态 | 判断 |
|---|---|---|
| Lexer / Parser / AST / CodeGen | 主链路完整 | 进入兼容性与内存合同收尾 |
| VM 指令 | 38/38 有实现与覆盖合同 | bytecode verifier 已补嵌套 Proto 诊断与 open-vararg 边界回归 |
| Lua 5.1 官方 strict | 原样 `all.lua` PASS | 本地 Release 约 10 秒，slow `sort.lua` / `verybig.lua` 也通过 |
| TestC | `api.lua` 与校正后的 `code.lua` required PASS | 已知 XFAIL 清单为空 |
| C API 函数合同 | 官方 123/123 PASS | 0 XFAIL / 0 UNSUPPORTED |
| 项目公开面 | 132 函数、57 宏、26 枚举、11 typedef | `lua_tryclose` 已进入 C/C++/DLL/SO 合同 |
| 测试基线 | 788 tests / 6681 assertions / 0 failures | C API 为 59 / 2822 |
| Runtime 治理 | budget、deadline、取消、finalizer budget、owner-thread、sandbox | 已有较强生产边界，但仍需 soak |
| allocator hard limit | 多个核心切片闭环，整体仍为 unsupported | 本轮新增 I/O read buffer 三个增长 offset 的事务证据 |
| 发布工程 | CMake install/export/PackageConfig 已实现 | 0.1.0 静态与共享目标均由外部纯 C consumer 验证 |
| CI | 本地等价门禁通过，远端待验证 | 不能把工作树写成线上全绿 |

## 二、本轮按顺序完成的收敛

### 1. Bytecode verifier 与回归

- open-result `VARARG` 允许从 `maxStack` 边界开始，不再把合法 `{...}` lowering 判成越界；
- verifier 错误包含嵌套 Proto 的 source/line 上下文；
- 增加 verifier 和 `string.dump` / undump 回归。

### 2. 公开 API 与原生模块路径

- `lua_tryclose` 加入纯 C 编译、C++ 精确签名、Windows `.def` 和 Linux version script；
- `LUA_ERRTHREAD` / `LUA_ERRBUSY` 进入编译合同；
- native-module host 对 fixture 路径统一做绝对规范化，避免 loader alias 与相对路径产生重复 lease。

本地机器合同结果为：官方 123/123 PASS；项目公开面 132 函数、57 宏、26 枚举、11 typedef；API-contract CTest 全部通过。

### 3. 弃用接口与质量门

- 测试代码全部迁移到显式 `RuntimeServices` 的 CodeGenerator / VM 调用；
- 清除 MSBuild 弃用告警；
- 同步 sanitizer、TestC、文档漂移和 C-style allowlist 合同；
- allowlist 固定使用 LF，避免 PowerShell 平台换行造成全文件漂移。

本地 strict quality gate 和 quality-gate contract 均通过。Windows clang-tidy 使用 MSVC compile database 时仍会在 UCRT `offsetof` 宏上报告前端不兼容；Linux CI 使用 libc++/clang 的真实 lint 配置，需以远端结果作为最终证据。

### 4. CI 日志驱动修复

- fuzz/coverage lane 显式使用 clang-18、libc++ 与 libc++abi；
- macOS ARM64 禁用不可用的 `RLIMIT_AS`，allocator 专用 lane 继续提供内存上限证据；
- userdata 默认 payload 使用 `malloc/free` 并保持 `max_align_t >= 8` 合同，移除 MinGW 不可用的 aligned allocation；
- benchmark trace scope 改为使用被测 `lua_State` 的 context-local trace runtime，恢复真实指令计数。

这些修改都来自 run 29651528378 的失败日志，但 fuzz、coverage、macOS 与 ARM64 只能由新远端矩阵最终确认。

### 5. SDK、版本与 ABI 包装

- 项目版本为 `0.1.0`，公开 `lua_cpp_version.h` 和 ABI 版本 0；
- 安装 `lua.h`、`lauxlib.h`、`lualib.h` 和版本头；
- 导出静态 `LuaCpp::Lua` 与共享 `LuaCpp::Shared`；
- 共享目标设置 `VERSION` / `SOVERSION`，Windows/Linux 使用穷尽式符号清单，macOS 从同一清单生成 Mach-O export list；
- 生成 `LuaCppConfig.cmake`、版本文件和 namespaced targets；
- `cmake_package_consumer` 先安装当前构建，再让独立纯 C 源码 consumer 分别 `find_package`、链接静态/共享目标并执行；工程启用 C++ linker language 以满足静态实现的运行库依赖。

本地 Release 的安装目录已包含 DLL、静态库/import library、四个公开头、LICENSE 和完整 CMake package 文件；静态与共享外部 consumer 均通过。

### 6. allocator hard-limit 新切片

I/O 标准库的 `readLine`、定长 `readChars`、`readAll` 和数字 token 缓冲已从默认 `std::string` 迁到 State callback 支撑的 `LuaString`。新增 `file:read("*a")` 门禁用 96 字节输入测得三个实际增长分配，并逐点证明：

- 每个 fail-on-N 返回 `LUA_ERRMEM`；
- 零余量时 `liveBytes` / `peakBytes` 不越过 hard limit；
- 解除失败或限制后可用新句柄重试并保持内容；
- `lua_close` 后块数和 live bytes 归零；
- 无 old-size mismatch 或重复释放。

这仍是切片证据，不把整体状态升级为 supported。

## 三、本地验证证据

当前工作树已经完成：

- 根 MSBuild Debug 与 Release 构建；
- CMake Release `lua_test`、共享库和 benchmark 构建；
- 788 tests / 6681 assertions / 0 failures；
- CTest 全套（含 API、native module、benchmark、examples 与安装后 consumer）；
- 官方 Release strict `all.lua`；
- slow `sort.lua` / `verybig.lua`；
- 123/123 公共 API 合同；
- strict format / doc drift / quality-gate contract；
- 三对交错 base/head benchmark 自比较。

这些结果证明同一 Windows 源码快照在主要本地门禁上闭环；Linux、sanitizer、fuzz、coverage、ARM64 和 macOS 仍须提交后由 CI 复核。

## 四、真实剩余风险

### 1. EngineContext 仍允许多个 root State

`LuaState::create(EngineContext&)` 没有拒绝同一 context 的第二个 root，而 `GlobalState` 只保存一个 `mainThread_`。这会使 GC 根扫描和 close 语义存在不完整支持。建议明确合同为“一个 EngineContext 恰好一个 root LuaState，coroutine 从 root 派生”，并为二次创建、强制 GC 和关闭增加回归。

Trace 已迁入 `GlobalState::TraceRuntime`；无 services 的重载仅是 singleton 兼容层，因此旧报告中的“生产 trace 是进程全局状态”不再成立。

### 2. allocator hard limit 尚未覆盖全运行时

剩余主要包括：

- AST 内部字符串/向量载荷、Parser diagnostic 与 CodeGen 临时容器；
- debug/trace、package 和其余 stdlib 临时容器；
- I/O 错误格式化与进程级句柄 registry；
- allocator callback 的 userdata 生命周期合同；
- 明确属于宿主/进程预算的 reader storage、OS loader 和 NativeModuleRegistry 元数据。

后两类不应机械塞入 `lua_Alloc`。文档应继续区分 ScriptRuntimeBudget 与 HostProcessBudget，并使用 Job Object/cgroup 等治理进程级上限。

### 3. 新 CI 快照尚无线上绿色证据

基准 run 的 16 个失败已有本地对应修复，但在新 run 完成前不能关闭平台问题。优先观察：

- Linux Clang/libc++ 的 ASan、UBSan、TSan；
- libFuzzer 与 llvm-cov 工具链；
- macOS ARM64 loader/RLIMIT 行为；
- MinGW/Windows 共享导出与安装 consumer；
- benchmark 的离散度。

### 4. 长期稳定性仍不足

现有 fuzz 是 bounded smoke，benchmark 是相对回归门，仍缺少 coroutine、weak table、finalizer、取消延迟、多 context 与 native-module 生命周期的长时间 soak。Runtime Preview 可以发布，生产级或 v1.0 仍需这些证据。

## 五、下一阶段优先级

1. 提交当前快照并跑完整 17-lane CI，按新日志只修真实剩余平台问题；
2. 收紧一个 EngineContext 一个 root State 的不变量；
3. 继续 allocator 切片：CodeGen temporaries → package/debug → allocator userdata 生命周期；
4. 增加 sanitizer/fuzz/coverage 趋势和多 context soak；
5. 在 0.1.x Runtime Preview 中验证真实游戏服务器嵌入：sandbox、预算、deadline、取消、preload-only package 与 owner-thread shutdown。

## 最终判断

- 现代 C++ 教学解释器：约 93%；
- Lua 5.1 源码与 C API 高兼容实现：约 91%；
- 可消费 Runtime Preview SDK：约 85%；
- 游戏服务器生产运行时：约 76%。

项目已经越过“只有源码和单测”的阶段：公开 API、官方套件、运行时治理、故障注入和 SDK 消费都有机器证据。下一步的价值不在继续扩展横向功能，而在把 context 不变量、剩余 allocator 路径和跨平台线上矩阵变成同样可重复的证据。
