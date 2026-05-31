---
status: current
verified_against: README.md; docs/status/project-status.md; docs/roadmap/lua51-compatibility-next-stage.md; tests/lua/official/all.lua; tests/unit/official/test_official_suite.cpp; tests/unit/stdlib/test_baselib.cpp; tests/unit/vm/test_vm_dispatch.cpp; src/lib; src/vm; src/gc; src/core; src/runtime
last_checked: 2026-05-31
applies_to: Lua 5.1.5 compatibility status after staged official smoke reached zero script skips
---

# Lua 5.1.5 Compatibility Matrix

本文按 Lua 5.1 手册章节记录当前兼容性状态。这里的“完成”表示项目内测试、官方 staged smoke 和已知回归探针覆盖的行为稳定；它不是逐字节、逐错误文本的官方实现等价声明。

当前验证基线：

- `bin\lua_test.exe`：639 registered tests，3188 assertion results，0 failures。
- `Lua 5.1 Official Suite`：`tests/lua/official/all.lua` staged smoke 通过，官方子脚本 skip 表为 0。
- 重要限制：官方 `api.lua` / `code.lua` 的 `testC` helper 路径未执行；staged smoke 使用 `_soft=true`，并保留少量压力路径裁剪。

## 状态标记

| 标记 | 含义 |
|---|---|
| Complete | 当前实现和测试覆盖足以作为项目内稳定行为 |
| Partial | 主路径可用，但存在已知 Lua 5.1.5 差异或未覆盖边界 |
| Deferred | 已明确不在本阶段实现 |

## Language Core

| Lua 5.1 区域 | 状态 | 证据与边界 |
|---|---|---|
| Values and types | Complete | `Value`、Table、Function、Thread、Userdata 主体已覆盖；NaN table key 拒绝和 nil table key 写入报错已补探针 |
| Lexical conventions | Complete | 官方 `literals.lua` 通过；长字符串/注释、shebang、数字字面量和错误诊断边界已覆盖 |
| Statements and blocks | Complete | 官方 `locals.lua`、`constructs.lua`、`vararg.lua`、`closure.lua` 通过 |
| Expressions and arithmetic | Complete | 字符串数字转换、Lua floor-mod、double 除零行为、比较和短路表达式已覆盖 |
| Tables | Complete | `next/pairs/ipairs`、table 库、`#t` 边界和 table `__len` strict 5.1 策略已覆盖；有洞表仍遵循 Lua 5.1 未指定边界语义 |
| Functions and closures | Complete | Upvalue、闭包环境、tail call、vararg、新旧 `arg` 边界已由官方脚本和单测覆盖 |
| Coroutines | Complete | `closure.lua` 与 coroutine 单测覆盖 resume/yield/wrap/error object 主路径 |
| Metatables and metamethods | Complete | `events.lua` 通过；table/userdata 与基础类型元表、算术/比较/拼接/call/index/newindex 主路径可用 |
| Garbage collection language semantics | Complete | 官方 `gc.lua` standalone 和 staged smoke 通过；弱表、`__gc` 两阶段终结、复活主路径可用。`setpause/setstepmul` 已保存控制参数并影响自动阈值/step 工作预算；`collectgarbage("step")` 已按 pause/propagate/atomic/sweep/finalize 分阶段推进；保守写屏障已覆盖 table/metatable/function/upvalue/global-state 引用。`IncrementalGC` 策略入口仍保留教学占位边界，`collect()` 本身仍走完整 mark-sweep |

## Standard Libraries

| Library | 状态 | 证据与边界 |
|---|---|---|
| Base | Complete | `_G`、`_VERSION`、print/type/tostring/tonumber/error/assert/pcall/xpcall/load/loadfile/dofile/getfenv/setfenv/newproxy 等主路径可用；无参 stdin、C 函数环境、错误对象和 xpcall handler 已补探针 |
| Coroutine | Complete | 6/6 函数主体实现并通过官方/单测组合路径 |
| Package/module | Partial | `require`、`module`、`package.loadlib`、C loader 和 all-in-one loader 主路径可用；系统 loader 搜索失败文本仍可能与官方逐字不同 |
| String | Complete | 官方 `strings.lua`、`pm.lua` 通过；dump/load 使用项目本地 binary 格式 |
| Table | Complete | Lua 5.1 核心函数、`table.getn/foreach/foreachi` 和数字字符串参数已覆盖 |
| Math | Complete | 28/28 函数主体实现；字符串数字参数转换和 `math.huge` 等边界已覆盖 |
| IO | Complete | `files.lua` 通过；`io.lines/file:lines` read format 参数已接入 |
| OS | Complete | `files.lua` 通过；`remove/rename` 失败返回 `nil, message, errno` |
| Debug | Partial | `db.lua` 通过；getinfo/local/upvalue/hook/traceback/thread env/C env 主路径可用。逐字 traceback 文本和少量栈层级极端边界仍可继续加探针 |

## VM And Bytecode

| 区域 | 状态 | 证据与边界 |
|---|---|---|
| Lua 5.1 opcode execution | Complete | 38 条 opcode 均有执行分支；官方脚本覆盖主路径；`VM Dispatch` 单测通过 |
| Opcode matrix edge rows | Complete | `tests/unit/vm/opcode_coverage_matrix.md` 已清掉可执行 TODO；`VM Dispatch` 覆盖 handler 边界，`Metamethod` 覆盖运行时元方法 opcode 路径 |
| `VM::call()` C/Lua function path | Complete | C/Lua 函数和元方法调用主路径已统一 |
| `VM::execute()` C function policy | Deferred | `execute()` 仍作为 Lua function / Proto 执行入口；C function 通过 `VM::call()` 执行 |
| Binary chunks | Deferred | `string.dump` / `loadstring` 使用项目本地格式；不声明官方 Lua 5.1 binary chunk 兼容 |

## C API And Official Helpers

| 区域 | 状态 | 证据与边界 |
|---|---|---|
| Official `testC` / `ltests.c` | Deferred | 当前没有 Lua C API 兼容 shim；`api.lua` / `code.lua` 在 `T == nil` 分支通过 |
| Embedding API | Partial | `LuaState` 提供项目内 API；不是官方 `lua_State` C ABI |
| Dynamic C modules | Partial | `package.loadlib` 和 C loader 主路径可用；不是完整 Lua C API 兼容声明 |

## Runtime Isolation

| 区域 | 状态 | 证据与边界 |
|---|---|---|
| `RuntimeServices` explicit bundle | Complete | 已作为显式服务束存在，并可由 singleton 或 owning `EngineContext` 生成 |
| Owning engine context | Partial | `EngineContext` 已拥有独立 `StringPool`、`GlobalState`、GC、registry、基础类型元表和 main-thread bookkeeping；`lua_app` / `lua_bytecode` 已迁移，更多兼容重载仍保留 singleton fallback |
| Singleton fallback removal | Partial | `RuntimeServices::fromSingletons()` 仍是 legacy 兼容入口；新入口应优先选择 `EngineContext` |

## Recommended Next Work

1. 选择 `testC` 策略：移植 `ltests.c`、实现项目内 `T` 模块，或继续明确为非目标。
2. 明确 `IncrementalGC` 策略入口的长期目标：保持教学占位，或让完整 `collect()` 也采用分阶段调度。
3. 将更多兼容重载和测试夹具迁移到 `EngineContext`，逐步减少 singleton fallback。
