---
status: current
verified_against: README.md; docs/status/project-status.md; docs/compatibility/lua51.md; docs/roadmap/current.md; src/lib; src/vm; src/gc; src/core; src/runtime; tests/lua/official/all.lua; tests/unit/vm/opcode_coverage_matrix.md
last_checked: 2026-05-31
applies_to: Lua 5.1.5 官方 staged smoke skip 表清零后的下一阶段兼容性补完
---

# Lua 5.1.5 下一阶段兼容性计划

本文记录官方 Lua 5.1 测试套件 staged smoke 全绿之后的兼容性补完任务，以及本轮执行结果。

当前基线：

- `bin\lua_test.exe`：639 个 registered tests，3188 个 assertion results，0 failures。
- `tests/lua/official/all.lua`：已接入 `Lua 5.1 Official Suite` staged smoke。
- 官方子脚本 skip 表：0。
- 重要限制：`api.lua` / `code.lua` 仍在无上游 `testC` helper 模块时运行可选跳过分支；官方 staged smoke 使用 `_soft=true`，并保留 `constructs.lua` 等压力路径的 harness 裁剪。

2026-05-31 收敛状态：

- 默认测试门禁保持在已验证的 staged smoke 路径，不默认注册 `code.lua` 的 `T.listcode` opcode 精确检查。
- 已确认 `code.lua` 的 `T.listcode` 探针会暴露编译器 opcode 序列与 Lua 5.1 官方 `lcode.c` 优化策略的差异；在完成系统化 codegen parity 前，不把该探针纳入 `bin\lua_test.exe` 默认通过标准。
- `api.lua` 的 `T.testC` 路径需要接近上游 `ltests.c` 的 Lua C API helper surface，不作为当前收敛目标。
- 当前收敛目标是：主线编译通过、默认单元/官方 staged smoke 运行无失败，未完成项保留在 Phase 3 后续任务中。

## 阶段目标

1. 优先修复官方 smoke 未暴露但会产生可观察错误行为的 Lua 5.1 语义差异。
2. 把 README 中的过期 TODO 与真实源码状态重新对齐。
3. 每个兼容性修复都配套最小回归测试，再跑官方 staged smoke。
4. 把短期标准库/VM 修补和长期 GC/运行时隔离工作分开推进。
5. 保持 `bin\lua_test.exe`、官方 staged smoke 和文档漂移检查作为阶段验收门。

## 本轮完成摘要

- 新增 `Lua 5.1 Compatibility` C++ 探针套件，覆盖 P0/P1 标准库和 VM 可见语义边界。
- 修复 nil table key 写入、数字字符串转换、除零/取模零策略、table `__len` 与长度边界。
- 补齐 `loadfile()` / `dofile()` 无参 stdin、`io.lines/file:lines` 格式参数、`os.remove/os.rename` 失败三元组、C 函数环境和 `error/xpcall` 探针。
- 修复 `gc.lua` standalone 深结构路径的 GC 注册性能问题。
- 补齐 Phase 4 opcode coverage matrix 的可执行 TODO，并新增运行时元方法 opcode 探针。
- 落地 Phase 5/6 架构切片：`setpause/setstepmul` 状态化、`collectgarbage("step")` 分阶段推进、保守写屏障、debug hook 内手动 GC 栈根保护，以及 owning `EngineContext` / `LuaState::newState(context)`。
- 更新 README、项目状态和兼容性矩阵，明确 `testC`、binary chunk、`IncrementalGC` 策略占位和更多 runtime 入口迁移仍是后续边界。

## 优先级定义

| 优先级 | 含义 |
|---|---|
| P0 | 可观察 Lua 5.1 语义不一致，或静默错误行为 |
| P1 | 标准库公开 API 缺口，或高价值 debug/C API 边界 |
| P2 | VM/opcode 边界测试、binary chunk 兼容策略、文档漂移 |
| P3 | 更大的运行时架构工作，例如精确 GC 工作量模型和多实例隔离收口 |

## Phase 0：基线与文档校准

**状态：已完成本轮目标。**

- [x] **L51-0001：更新 README 缺失功能表。**
  - 已移除 `LuaState::pcall()` error handler 仍是 TODO 的过期描述。
  - 已将已实现项和真实缺口拆开，真实剩余缺口集中在 `testC`、binary chunk、`IncrementalGC` 策略占位和 runtime 隔离收口。
  - 已保留“测试数量不是 Lua 5.1.5 精确兼容率”的说明。

- [x] **L51-0002：新增小型兼容性探针测试套件。**
  - 位置：`tests/unit/stdlib/test_baselib.cpp` 的 `Lua 5.1 Compatibility` suite。
  - 覆盖：nil table key 写入、除零策略、table `__len`、无参 `loadfile()` / `dofile()`、`os.remove` 失败返回、`io.lines` 格式参数、C 函数环境和错误对象。
  - 验收：`bin\lua_test.exe --filter "Lua 5.1 Compatibility"` 通过，9 tests / 35 assertions / 0 failures。

- [x] **L51-0003：单独记录 `testC` 可选覆盖状态。**
  - 已在 README 和 `docs/compatibility/lua51.md` 记录 `api.lua` / `code.lua` 在 `T == nil` 时跳过官方 C API/helper 检查段。
  - 决策：本轮不移植 `ltests.c`，保持 staged smoke 覆盖无 `T` 分支。

## Phase 1：P0 运行时语义修复

**状态：已完成。**

- [x] **L51-0101：普通表赋值遇到 nil key 必须报错。**
  - `Table::set()` 对 nil key 抛 `RuntimeError("table index is nil")`。
  - `rawset(t, nil, 1)` 仍报错，`t[k] = nil` 删除键行为保持。

- [x] **L51-0102：统一数字字符串转换策略。**
  - 新增 `src/common/number_conversion.hpp`。
  - `LuaState::isNumber()` / `toNumber()`、VM arithmetic、math/string/table 参数路径复用共享转换。
  - 修复 `tonumber("99", 8)` 等 base 转换边界。

- [x] **L51-0103：决定并锁定除零/取模零策略。**
  - 决策：匹配 Lua 5.1 默认 double 构建，除零继承 C 浮点行为；取模零通过 `luaModulo` 得到 NaN。
  - 已补探针覆盖 `1/0`、`-1/0`、`0/0`、`1%0`。

- [x] **L51-0104：对齐 table 长度与 table `__len` 策略。**
  - 决策：strict Lua 5.1 下 table `__len` 被忽略；非 table 值仍允许 `__len` 慢路径。
  - `Table::length()` 改为数组边界 + hash 正整数边界搜索。

## Phase 2：P1 标准库补完

**状态：已完成本轮公开 API 边界。**

- [x] **L51-0201：实现 `loadfile()` / `dofile()` 无参 stdin 模式。**
  - 无 filename 时可从 redirected/piped stdin 编译或执行 chunk。
  - 交互式 stdin 下返回受控错误，避免单元测试挂起。

- [x] **L51-0202：补齐 `io.lines` / `file:lines` 格式参数。**
  - 迭代器接受与 `file:read` 相同的 read format。
  - 覆盖 `io.lines(path, "*n")`、`f:lines("*l", "*n")` 和自动关闭文件。

- [x] **L51-0203：`os.remove` / `os.rename` 失败返回完整三元组。**
  - 失败返回 `nil, errmsg, errno`。
  - Windows 文件共享 workaround 保持。

- [x] **L51-0204：补齐 `getfenv/setfenv` 的 C 函数环境边界。**
  - C closure 创建时设置默认全局环境。
  - `getfenv(print)` / `setfenv(cfunc, env)` 主路径已覆盖。

- [x] **L51-0205：复核并锁定 `error` / `pcall` / `xpcall`。**
  - 已补 `error({x=1}, 0)` 保留 table 对象。
  - 已补 `xpcall(function() error("x") end, handler)` 返回 `false, handlerResult`。
  - error handler 自身出错时返回 fallback 文本。

## Phase 3：P1 官方 C API Helper 覆盖

**状态：已决策，执行延期；当前默认门禁不启用 `T` helper。**

- [x] **L51-0301：决定如何支持上游 `testC` / `ltests.c`。**
  - 本轮采用方案 C：明确记录 C API helper 暂不属于本阶段实现目标。
  - 理由：当前项目尚无 Lua C API 兼容层；直接移植 `ltests.c` 会牵动 ABI、动态库加载、MSBuild/CMake 产物和 API surface。

- [ ] **L51-0302：启用 `T` 后运行 `api.lua`。**
  - 延期到 Lua C API shim 或项目内 `T` 模块可用之后。
  - 收敛记录：`api.lua` 不是单个 Lua helper 函数即可覆盖的脚本；它依赖 `T.testC`、refs、userdata、stack/state、内存限制、C closure/upvalue 等接近完整 `ltests.c` 的接口集合。

- [ ] **L51-0303：启用 `T` 后运行 `code.lua` opcode 检查。**
  - 延期到 `T` 模块策略落地之后。
  - 收敛记录：最小 `T.listcode` 探索已证明该脚本会检查 Lua 5.1 官方代码生成的精确 opcode 序列，而当前编译器尚未完整复刻连续 `LOADNIL` 合并/消除、常量折叠、concat 合并、直接局部/表赋值寄存器复用、布尔条件规约等 `lcode.c` 优化。
  - 后续建议：先建立独立的 codegen parity characterization suite，逐项修复并验证 opcode 序列，再把 `code.lua` 的 `T.listcode` 路径注册进默认官方 suite。

## Phase 4：P2 VM 与字节码边界锁定

**状态：已完成本轮 VM/opcode 覆盖目标。**

- [x] **L51-0401：补齐 opcode coverage matrix TODO 行。**
  - 已扩展 `tests/unit/vm/test_vm_dispatch.cpp`。
  - 覆盖 MOVE alias copy、最高 LOADK Bx、最高配置 upvalue slot、缺失 GETGLOBAL、GETTABLE absent key、SETGLOBAL overwrite、SETUPVAL closed overwrite、SETTABLE nil assignment、非零 NEWTABLE operands、算术错误路径、RK 混合操作、DIV/MOD 零策略、POW fractional/negative exponent 和 NOT truthiness split。

- [x] **L51-0402：补齐运行时元方法 opcode 测试。**
  - 已新增 `tests/unit/metamethod/test_metamethod_arith.cpp` 的 `Runtime metamethod opcode execution`。
  - 覆盖运行时 `__unm`、`__mod`、`__pow`、`__concat`，以及通过 `__call` 的 tailcall。

- [x] **L51-0403：决定 binary chunk 兼容范围。**
  - 决策：本项目当前保持项目本地 dump/load 格式，不声明官方 Lua 5.1 binary chunk 兼容。
  - 状态已写入 README 和 `docs/compatibility/lua51.md`。

- [x] **L51-0404：明确 `VM::execute()` 对 C 函数的策略。**
  - 决策：`VM::execute()` 仍只面向 Lua function / Proto 执行入口；C function 通过 `VM::call()` 路径执行。
  - 该边界记录在兼容性矩阵中。

## Phase 5：P2/P3 GC 兼容性

**状态：已完成本阶段 GC 兼容性目标；更精确的 Lua 5.1 GC 工作量模型仍可作为后续优化。**

- [x] **L51-0501：设计真实增量 GC 阶段。**
  - 已在 `docs/architecture/gc.md` 定义目标状态：pause、propagate、atomic、sweep-string、sweep-object、finalize。
  - 已记录三色不变式、弱表、finalizer/复活、根集和当前实现差距。

- [x] **L51-0502：实现写屏障。**
  - 已新增 `GarbageCollector::writeBarrier()` / `writeRootBarrier()`。
  - 已覆盖 table 写入、table/userdata metatable、function env/upvalue、upvalue value/close 和 GlobalState root 引用。
  - 已新增 `GC` 写屏障回归测试，验证黑色 owner 接入白色子图后 sweep 不会误回收。

- [x] **L51-0503：实现 `setpause` / `setstepmul` / 真实 `step` 节奏。**
  - `setpause` / `setstepmul` 已保存并返回旧值；pause 影响自动 GC 阈值，step multiplier 影响 step 工作预算。
  - `step` 已按 pause、propagate、atomic、sweep、finalize 分阶段推进；小步长可多次推进同一周期，大步长会完成当前周期。
  - 边界：工作量单位仍是项目本地预算模型，不声明逐字节复刻 Lua 5.1 的 GC debt 算法。

- [x] **L51-0504：增强 finalizer 顺序与复活测试。**
  - 官方 `gc.lua` standalone 与 staged smoke 已通过。
  - 本轮还修复了深结构注册性能，并用 `GC` 单元测试覆盖分阶段 step 和 finalizer 主路径。

## Phase 6：P3 运行时隔离与嵌入边界

**状态：本阶段入口迁移已完成；legacy compatibility overload 仍作为显式边界保留。**

- [x] **L51-0601：引入拥有资源的 engine/runtime context。**
  - 已新增 `EngineContext`，拥有独立 `StringPool`、`GlobalState` 和 `GarbageCollector`。
  - `GlobalState` 可由指定 `StringPool` 构造；`StringPool` 可创建独立实例，同时保留 legacy singleton。

- [x] **L51-0602：让 `LuaState::newState(context)` 默认可隔离。**
  - 已新增 `LuaState::newState(EngineContext&)`。
  - 已新增 `Runtime Services` 测试，确认两个 context 的 global state、string pool、GC、同文本 interned string 和 context main thread 彼此隔离。

- [x] **L51-0603：减少直接 singleton fallback。**
  - `lua_app` 和 `lua_bytecode` 已改为创建 `EngineContext`；base/debug/package/string/table/TFORLOOP 等运行时入口已优先沿当前 `RuntimeServices` 调用编译器或 VM。
  - VM/compiler 兼容重载和部分旧测试夹具仍保留 `RuntimeServices::fromSingletons()`，作为 legacy path 明确保留并由 `Runtime Services` 测试覆盖。

## Phase 7：文档与阶段出口

**状态：本阶段文档出口已更新；长期兼容边界保留为后续任务。**

- [x] **L51-0701：新增兼容性矩阵文档。**
  - 位置：`docs/compatibility/lua51.md`。

- [x] **L51-0702：每阶段同步项目状态文档。**
  - 已更新 README 和 `docs/status/project-status.md` 的测试数量、已知缺口和兼容边界。

- [x] **L51-0703：定义本阶段完成标准。**
  - P0/P1/P2 标准库项已完成。
  - P4 VM/opcode 覆盖项已完成。
  - P5/P6 架构切片已完成。
- P3 `testC`、官方 Lua 5.1 binary chunk、`IncrementalGC` 策略入口精确语义和 P6 singleton fallback 完全收口仍未完成，不应宣称完整 Lua 5.1.5 兼容。
- 2026-05-31 收敛补充：`code.lua` 的 `T.listcode` 精确 opcode 检查与 `api.lua` 的 `T.testC` 路径仍是明确未完成目标；默认 `bin\lua_test.exe` 不应包含会失败的探索性 `T` 测试。
  - 本阶段可交付标准：完整 `bin\lua_test.exe`、官方 staged smoke、opcode matrix 和文档漂移检查均通过；未完成中长期项保留为明确后续边界。

## 验证命令

开发时先跑最小切片，阶段收口前跑完整门禁。

```powershell
bin\lua_test.exe --filter "Lua 5.1 Compatibility"
bin\lua_test.exe --filter "Lua 5.1 Official Suite"
bin\lua_test.exe --filter "VM Dispatch"
bin\lua_test.exe --filter "Metamethod"
bin\lua_test.exe --filter "GC"
Push-Location tests\lua\official; ..\..\..\bin\lua_app.exe -e "gcinfo=gcinfo or function() return collectgarbage('count') end; dofile=function(n) local f=assert(loadfile(n)); local b=string.dump(f); f=assert(loadstring(b)); return f() end; dofile('db.lua')"; Pop-Location
bin\lua_test.exe
powershell -NoProfile -ExecutionPolicy Bypass -File tools\check_opcode_coverage_matrix.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\check_doc_drift.ps1
```

P0/P1 实现时可用的脚本探针：

```powershell
bin\lua_app.exe -e "local ok,err=pcall(function() local t={} t[nil]=1 end); print(ok, err)"
bin\lua_app.exe -e "local ok,err=pcall(function() return 1/0 end); print(ok, err)"
bin\lua_app.exe -e "local t=setmetatable({1,2,3},{__len=function() return 99 end}); print(#t)"
bin\lua_app.exe -e "local ok,a,b=pcall(loadfile); print(ok,a,b)"
bin\lua_app.exe -e "local ok,a,b,c=pcall(os.remove,'__definitely_missing_file__'); print(ok,a,b,c)"
bin\lua_app.exe -e "local ok,err=pcall(function() return io.lines('README.md','*l') end); print(ok, err)"
```

## 本阶段非目标

- 不在 P0/P1 修复中重写编译器管线。
- 不替换已经全绿的官方 staged smoke harness；只做增量扩展。
- 不声称官方 Lua 5.1 binary chunk 兼容，除非真的实现并测试官方格式。
- 不在 owning runtime context 有测试和迁移文档前移除 singleton 兼容 API。
