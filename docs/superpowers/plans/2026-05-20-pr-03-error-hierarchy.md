# PR-03：统一异常类型层级

## 目标

将当前分散的异常类型收敛到 `src/common/lua_error.hpp`：

- `LuaError` 作为统一基类，同时继续兼容现有 `LuaError(Value)` 的 Lua 错误对象语义。
- `ParseError` 继承 `LuaError`，保留 `getLine()` / `getColumn()` 公共接口。
- 新增 `RuntimeError` / `MemoryError`，用于替代 VM 层裸 `std::runtime_error`。
- 保留调用方对 `std::exception` / `std::runtime_error` 的兼容捕获路径，避免破坏命令行和 REPL 的报错行为。

## 拆分范围

本任务聚焦异常类型层级本身，不改变 VM、Parser、REPL 的行为语义：

1. 重构 `LuaError`
   - 从 `std::exception` 改为继承 `std::runtime_error`。
   - 保留 `explicit LuaError(Value errorObj)`，让 `pcall` / coroutine 仍可取回 Lua Value 错误对象。
   - 增加字符串构造，供 `RuntimeError` / `MemoryError` 复用。
   - 增加 `hasErrorObject()`，调用方可区分 Lua Value 错误和普通消息错误。

2. 迁移 `ParseError`
   - 从 `src/compiler/parser.hpp` 移到 `src/common/lua_error.hpp`。
   - 保持构造签名与位置访问接口不变。
   - `parser.hpp` 继续暴露 `ParseError`，现有包含 `compiler/parser.hpp` 的测试无需改入口。

3. 引入运行时异常类型
   - 新增 `RuntimeError : public LuaError`。
   - 新增 `MemoryError : public RuntimeError`，为后续内存分配失败、栈溢出等场景留出明确类型。
   - 本 PR 优先替换 `src/vm/*` 的裸 `std::runtime_error`，不一次性改动 IO、stdlib、compiler codegen 等边界模块。

4. 调整错误捕获
   - `LuaState::pcall` / coroutine 捕获 `LuaError` 时，如果存在 Lua 错误对象则直接压回 Value；否则压入 `what()` 字符串。
   - CLI / REPL 在 `ParseError` 后增加显式 `LuaError` 捕获，继续保留 `std::runtime_error` 兜底。

## 验证

- 编译 `lua_test`。
- 运行 `tools/run_quality_gate.ps1`。
- 运行 `bin/lua_test.exe`，确认完整测试套件失败数为 0。
- 运行 `git diff --check`，确认没有空白错误。

