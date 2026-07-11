---
status: current
verified_against: README.md; docs/status/project-status.md; docs/compatibility/lua51.md; docs/compatibility/lua51-full-compatibility-audit.md; docs/roadmap/current.md; src/lib; src/vm; src/gc; src/core; src/runtime; tests/lua/official/all.lua; tests/unit/vm/opcode_coverage_matrix.md; tools/run_lua51_official_slow.ps1
last_checked: 2026-07-11
applies_to: Lua 5.1.5 官方 staged smoke skip 表清零后的最大兼容性补完
---

# Lua 5.1.5 最大兼容性计划

本文记录官方 Lua 5.1 测试套件 staged smoke 全绿之后的兼容性补完任务，以及 2026-06-01
兼容性审计后确认的长期目标：**最大化兼容 Lua 5.1.5**。

这里的“最大化兼容”表示：只要成本和项目边界允许，运行时语义、标准库行为、官方测试覆盖、
C API、binary chunk、VM opcode 形状、GC 调度和嵌入式隔离都应优先向 Lua 5.1.5 官方实现靠拢。
项目可以保留学习型实现和扩展能力，但不能用这些扩展替代 Lua 5.1.5 strict 兼容声明。

当前基线：

<!-- live-facts:start -->

- 最近一次完整绿跑基线：`bin\lua_test.exe` 为 668 个 registered tests，3406 个 assertion results，0 failures。
- 当前 runner 安全基线：`bin\lua_test.exe` 默认安装 512 MB 进程内存硬上限，支持
  `--max-memory-mb <mb>` 覆盖和 `--no-memory-limit` 显式关闭；当前已注册 668 个测试，
  其中新增 `closure.lua weak GC loop cap` 与 `post-vararg tail split guard`，用于阻止 official tail
  在弱表 GC 等待循环或四脚本串联路径中无界造垃圾。
- `tests/lua/official/all.lua`：已接入 `Lua 5.1 Official Suite` staged smoke。
- 官方子脚本 skip 表：0。
- 重要限制：`api.lua` / `code.lua` 仍在无上游 `testC` helper 模块时运行可选跳过分支；官方 staged smoke 使用 `_soft=true`，并保留 `constructs.lua` 等压力路径的 harness 裁剪。

<!-- live-facts:end -->

2026-06-01 目标修订：

- 项目目标从“staged smoke 全绿后的阶段性兼容补完”提升为“最大化兼容 Lua 5.1.5”。
- `testC` / Lua C API shim、`T.listcode` codegen parity、官方 Lua 5.1 binary chunk、未裁剪官方 suite、
  精确 GC 调度和 runtime singleton fallback 收口均进入长期兼容路线，不再作为永久非目标处理。
- 在这些边界真正实现并通过测试前，项目仍不声明“完整 Lua 5.1.5 等价”。

2026-05-31 收敛状态：

- 默认测试门禁保持在已验证的 staged smoke 路径，不默认注册 `code.lua` 的 `T.listcode` opcode 精确检查。
- 已确认 `code.lua` 的 `T.listcode` 探针会暴露编译器 opcode 序列与 Lua 5.1 官方 `lcode.c` 优化策略的差异；在完成系统化 codegen parity 前，不把该探针纳入 `bin\lua_test.exe` 默认通过标准。
- `api.lua` 的 `T.testC` 路径需要接近上游 `ltests.c` 的 Lua C API helper surface；它不是当前默认门禁，但已纳入最大兼容路线。
- 当前门禁目标是：主线编译通过、默认单元/官方 staged smoke 运行无失败；长期目标由本文后续阶段追踪。
- 2026-06-02 安全修订：在继续排查 official post-vararg tail 前，禁止无内存上限地运行
  `bin\lua_test.exe` 全量或官方长路径；必须依赖 runner 默认 512 MB cap 或显式更小的
  `--max-memory-mb`。
- 2026-06-02 post-vararg 拆分：post-vararg tail 已拆成 `closure.lua/errors.lua/math.lua/files.lua`
  单脚本 tail；`closure.lua`、`errors.lua`、`math.lua`、`files.lua` 在 `--max-memory-mb 128`
  下通过。closure 后 global cleanup tail 仍作为 GC/cleanup 交叉 known gap 隔离。

## 阶段目标

1. 优先修复官方 smoke 未暴露但会产生可观察错误行为的 Lua 5.1 语义差异。
2. 逐步减少 `_soft=true`、脚本截断和无 `testC` 分支带来的覆盖盲区。
3. 每个兼容性修复都配套最小回归测试，再跑官方 staged smoke。
4. 把 P0/P1 可见语义、P2 字节码/二进制边界和 P3 GC/运行时架构分层推进。
5. 保持 `bin\lua_test.exe`、官方 staged smoke、兼容性审计文档和文档漂移检查作为阶段验收门。

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

## 2026-06-01 审计转行动总览

`docs/compatibility/lua51-full-compatibility-audit.md` 中的 `L51-AUDIT-*` 项目按以下路线进入
roadmap：

| Workstream | Priority | Audit IDs | Goal | Gate |
|---|---|---|---|---|
| 官方 suite 去裁剪 | P0 | L51-AUDIT-001, L51-AUDIT-002 | 逐步撤销 `_soft` 裁剪和 all.lua 截断 | 未裁剪脚本稳定通过，失败项拆成 P0/P1 |
| Lua C API / `testC` | P1 | L51-AUDIT-003, L51-AUDIT-004, L51-AUDIT-014 | 提供 Lua 5.1 C API shim 并注册 `T` 模块 | `api.lua` 不再走 `T == nil` 跳过分支 |
| 动态 C 模块与 debug/package/io/os 边界 | P1 | L51-AUDIT-005, L51-AUDIT-006 | 支持官方 `lua_CFunction` ABI 和未裁剪官方边界 | 官方 `libs/*.c` 与相关脚本通过 |
| Codegen parity | P2 | L51-AUDIT-007, L51-AUDIT-008 | 复刻 `lcode.c` 关键 opcode 形状 | `code.lua` 的 `T.listcode` 检查通过 |
| Binary chunk 互通 | P2 | L51-AUDIT-009, L51-AUDIT-010 | 支持官方 Lua 5.1 chunk 或明确 strict/extension 双格式 | 同 ABI 官方 chunk 双向互通 |
| GC 与 runtime isolation | P3 | L51-AUDIT-011, L51-AUDIT-012, L51-AUDIT-013 | 对齐 GC 调度并移除生产路径 singleton fallback | 多 `EngineContext` 隔离和 GC step 对照测试通过 |

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

**状态：长期兼容目标已确认；当前默认门禁暂不启用 `T` helper。**

- [x] **L51-0301：决定如何支持上游 `testC` / `ltests.c`。**
  - 历史决策：2026-05-31 本轮采用方案 C，明确记录 C API helper 暂不属于当时阶段实现目标。
  - 目标修订：2026-06-01 起，Lua C API shim 与 `testC` helper 进入最大兼容路线的 P1 工作流。
  - 理由：当前项目尚无 Lua C API 兼容层；直接移植 `ltests.c` 会牵动 ABI、动态库加载、MSBuild/CMake 产物和 API surface。

- [ ] **L51-0302：启用 `T` 后运行 `api.lua`。**
  - 延期到 Lua C API shim 或项目内 `T` 模块可用之后。
  - 收敛记录：`api.lua` 不是单个 Lua helper 函数即可覆盖的脚本；它依赖 `T.testC`、refs、userdata、stack/state、内存限制、C closure/upvalue 等接近完整 `ltests.c` 的接口集合。

- [ ] **L51-0303：启用 `T` 后运行 `code.lua` opcode 检查。**
  - 延期到 `T` 模块策略落地之后。
  - 收敛记录：最小 `T.listcode` 探索已证明该脚本会检查 Lua 5.1 官方代码生成的精确 opcode 序列，而当前编译器尚未完整复刻连续 `LOADNIL` 消除、直接局部/表赋值寄存器复用、布尔条件规约等 `lcode.c` 优化；`LOADNIL` range merge、常量折叠和 concat 合并已进入项目内 characterization gate。
  - 2026-06-01 进展：已在 `Codegen Characterization` 建立独立 codegen parity 已知差异护栏；后续逐项修复并验证 opcode 序列后，再把 `code.lua` 的 `T.listcode` 路径注册进默认官方 suite。

- [ ] **L51-0304：建立 Lua 5.1 C API shim 第一阶段。**
  - 来源：L51-AUDIT-003、L51-AUDIT-014。
  - 目标：提供 `lua.h`、`lauxlib.h`、`lualib.h` 的最小兼容入口，覆盖 `lua_State`、stack index、registry、error/pcall、userdata 和 C closure/upvalue 的第一批 API。
  - 验收：官方 `ltests.c` 能在项目构建系统中进入编译阶段；未实现 API 以明确 linker/test failure 暴露，不再被文档误判为已覆盖。

- [ ] **L51-0305：让 `package.loadlib` 支持官方 `lua_CFunction` ABI。**
  - 来源：L51-AUDIT-005。
  - 目标：动态 C 模块加载路径支持 `lua_CFunction(lua_State*)`，项目内部 `CFunction(LuaState*)` 作为兼容或私有 ABI 保留。
  - 验收：官方 `tests/lua/official/libs/*.c` 可编译并被官方 package/loadlib 路径加载。

## Phase 4：P2 VM 与字节码边界锁定

**状态：已完成本轮 VM/opcode 执行覆盖目标；codegen parity 和官方 binary chunk 是下一层 P2 目标。**

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

- [x] **L51-0405：建立 `T.listcode` / codegen parity characterization suite。**
  - 来源：L51-AUDIT-007。
  - 目标：先在项目测试中暴露 `code.lua` 关心的 opcode 形状，不立即要求全部通过。
  - 验收：`LOADNIL`、concat、`not not`、direct local/table assignment、constant folding 和 `a = a` 自赋值均有独立失败/通过用例。
  - 已落地：`tests/unit/compiler/test_codegen_characterization.cpp` 新增 6 个 Lua 5.1 parity characterization 用例，显式锁住 `LOADNIL` range merge、arithmetic constant folding、direct contiguous local return、self-assignment elision、concat chain merge 和常量 `not not` LOADBOOL 规约目标形状，以及动态 `not not` TEST/JMP/LOADBOOL 形状、局部/表赋值临时寄存器等剩余差异。
  - 验证：`bin\lua_test.exe --filter "Codegen Characterization"` 运行 9 个 registered tests / 68 个 assertion results / 0 failures。

- [ ] **L51-0406：复刻 `lcode.c` 关键 codegen 优化。**
  - 来源：L51-AUDIT-008。
  - 目标：逐项补齐 `LOADNIL` merge/elide、constant folding、concat merge、boolean/jump normalization 和 register reuse。
  - 验收：`code.lua` 的 `T.listcode` 精确 opcode 检查进入默认 official suite 并通过。
  - 2026-06-01 进展：显式 nil local 初始化（如 `local a,b,c = nil,nil,nil`）已合并为单条 `LOADNIL A=0 B=2` 区间指令；数值字面量算术常量折叠已接入，并保留 Lua 5.1 `lcode.c` 风格的 div/mod by zero 与 NaN 不折叠边界；连续局部变量 return 已直接复用原寄存器；单局部 `a = a` 自赋值已消除无效 MOVE；三段及以上 concat chain 已合并为单条 `CONCAT`，并先把 active locals/upvalues/params 复制到 scratch operand range，避免 VM `CONCAT` 改写源寄存器；常量 `not not nil/false/true/1` 已折叠为单条 `LOADBOOL`，对应官方 `code.lua` 明确列出的 `T.listcode` 断言。
  - 剩余：动态 boolean/jump normalization、table/local assignment register reuse，以及 broader
    `T.listcode` fixture parity 仍未完成。无初始化且不可观察的局部变量已经不再发出 `LOADNIL`；
    普通赋 nil、闭包捕获和循环可观察路径仍由 characterization 测试守卫。

- [ ] **L51-0407：实现官方 Lua 5.1 binary chunk 互通策略。**
  - 来源：L51-AUDIT-009、L51-AUDIT-010。
  - 目标：保留 `"LC++"` 项目本地格式，同时新增 official Lua 5.1 chunk reader/writer 或 strict mode 策略。
  - 验收：同 ABI 官方 Lua 5.1 dump -> 项目 load、项目 official dump -> 官方 Lua 5.1 load 双向 fixture 通过。

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

- [ ] **L51-0505：对齐官方 Lua 5.1 GC phase/debt 调度。**
  - 来源：L51-AUDIT-011、L51-AUDIT-012。
  - 目标：让 `IncrementalGC::collect()` 不再只是 full mark-sweep alias，并建立接近官方 `luaC_step` 的 debt/threshold/phase 模型。
  - 验收：`collectgarbage("step")`、自动 GC、弱表、finalizer 和写屏障组合边界有官方对照测试；文档明确仍保留的实现差异。

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

- [ ] **L51-0604：收口生产路径 singleton fallback。**
  - 来源：L51-AUDIT-013。
  - 目标：将生产入口全部迁移到 explicit `RuntimeServices` / owning `EngineContext`，把 singleton fallback 限制到测试或明确 deprecated shim。
  - 验收：多 `EngineContext` 隔离测试覆盖 parser/codegen/VM/lib/GC/package/debug 组合路径；`rg "fromSingletons|GlobalState::getInstance|StringPool::getInstance" src` 的剩余项均有 documented exception。
  - 2026-06-01 进展：`tests/unit/official/test_official_suite.cpp` 已改为每个 official gate 使用独立
    `EngineContext`，修复 `all.lua staged` 后同进程运行 post-vararg tail 门禁时的 singleton 状态污染。
  - 2026-06-01 进展：`docs/architecture/runtime-services.md` 已新增 `src/` singleton fallback documented exception 清单；
    当前剩余项集中在 legacy constructors / service-less VM overloads / collector-only fallback / metatable helper fallback，尚未达到本项完成标准。

## Phase 7：文档与阶段出口

**状态：本阶段文档出口已更新；长期兼容边界保留为后续任务。**

- [x] **L51-0701：新增兼容性矩阵文档。**
  - 位置：`docs/compatibility/lua51.md`。

- [x] **L51-0702：每阶段同步项目状态文档。**
  - 已更新 README 和 `docs/status/project-status.md` 的测试数量、已知缺口和兼容边界。

- [x] **L51-0703：定义本阶段完成标准。**
  - P0/P1/P2 标准库项已完成。
  - P4 VM/opcode 覆盖项和 codegen parity characterization 已完成。
  - P5/P6 架构切片已完成。
- P3 `testC`、官方 Lua 5.1 binary chunk、`IncrementalGC` 策略入口精确语义和 P6 singleton fallback 完全收口仍未完成，不应宣称完整 Lua 5.1.5 兼容。
- 2026-06-01 收敛补充：`Codegen Characterization` 已覆盖 `T.listcode` 关心的主要已知差异；`code.lua` 的 `T.listcode` 精确 opcode 检查与 `api.lua` 的 `T.testC` 路径仍是明确未完成目标，默认 `bin\lua_test.exe` 不应包含会失败的探索性 `T` 测试。
  - 本阶段可交付标准：完整 `bin\lua_test.exe`、官方 staged smoke、opcode matrix 和文档漂移检查均通过；未完成中长期项保留为明确后续边界。

- [x] **L51-0704：将 2026-06-01 完整兼容审计转入 roadmap。**
  - 来源：`docs/compatibility/lua51-full-compatibility-audit.md`。
  - 已将 `L51-AUDIT-*` 清单映射为 P0-P3 workstream，并把项目目标明确为最大化兼容 Lua 5.1.5。

## Phase 8：P0 官方 suite 去裁剪

**状态：进行中。** 本阶段从 L51-AUDIT-001 / L51-AUDIT-002 开始，把 all.lua 中此前被截断或裁剪的
路径逐步拆成可控门禁。

- [x] **L51-0801：拆分 post-vararg tail 官方脚本门禁。**
  - 位置：`tests/unit/official/test_official_suite.cpp` 的 `post-vararg tail split guard` 与
    `post-vararg <script>.lua tail`。
  - 覆盖：`closure.lua`、`errors.lua`、`math.lua`、`files.lua` 经
    `loadfile -> string.dump -> loadstring` 单脚本路径运行。
  - 2026-06-02 验证：`bin\lua_test.exe --max-memory-mb 128 --filter "post-vararg"` 为
    5 selected results / 5 passed / 0 failed；四个 post-vararg tail 脚本均真实执行通过。

- [x] **L51-0802：新增 all.lua 风格 global cleanup/debug hook 门禁。**
  - 位置：`tests/unit/official/test_official_suite.cpp` 的 `global cleanup tail`。
  - 覆盖：`debug.sethook(..., "cr")` 后清空 `_G`，再通过局部捕获的 `collectgarbage`、`showmem`、`print`、
    `format`、`assert`、`type` 完成 all.lua tail cleanup。
  - 验证：2026-06-01 使用 `Lua 5.1 Official Suite` 的 `global cleanup tail` 门禁通过。

- [x] **L51-0803：修复 `closure.lua` 后立即执行 global cleanup 的 native crash。**
  - 当前结果：`closure.lua` standalone 与 global cleanup standalone 的 native crash 修复仍保留；
    `closure.lua -> global cleanup` 合并 tail 当前仍隔离为 known gap guard，等 GC/cleanup 交叉路径继续收敛后再恢复真实执行门禁。
  - 根因：unreachable suspended coroutine 与 open upvalue 的 GC 生命周期顺序。
    单趟 sweep 可能先删除仍指向协程栈的 open upvalue，再销毁 owning `Thread` / `LuaState`，导致后续关闭或标记路径访问悬空栈。
  - 已落地源码修复：完整 sweep 先回收 unreachable `Thread`，再回收非线程对象；增量 sweep 遇到仍 open 的 `Upvalue` 时延后一轮；
    `Upvalue::close()` 在转入 closed 状态后清空 `ownerStack_`。
  - 已新增回归：`Coroutine Library / pending coroutine open upvalue GC` 覆盖官方 `closure.lua` “leaving a pending coroutine open” 形态。
  - 历史验证：2026-06-01 使用 `D:\VS2026\MSBuild\Current\Bin\MSBuild.exe` 重建 `lua_test.vcxproj`
    Release|x64，0 warnings / 0 errors；当时完整 `bin\lua_test.exe` 为 654 registered tests /
    3340 assertion results / 0 failures。当前动态测试基线见本文开头和
    `docs/status/project-status.md`。2026-06-02 安全复核后，closure 合并 tail 不再作为默认真实执行门禁。

- [x] **L51-0804：为 `sort.lua` 与 `verybig.lua` 建立 slow official gate。**
  - 当前结果：默认 smoke 保持快速，`sort.lua` 与 `verybig.lua` 的 dump/undump 压力路径由显式 slow gate 承担。
  - 位置：`tools/run_lua51_official_slow.ps1`。
  - 覆盖：在 `tests/lua/official` 下通过 `loadfile -> string.dump -> loadstring` 运行 `sort.lua`，并断言
    `verybig.lua` round-trip 返回 `10`。
  - 验证：2026-06-01 使用 Release `bin\lua_app.exe` 执行 `tools/run_lua51_official_slow.ps1` 通过；
    本机耗时约 `sort` 1.6s、`verybig` 118.87s，因此仍作为 opt-in slow gate，不纳入默认 `lua_test`。

- [x] **L51-0805：修复 `closure.lua` dump/load tail known gap。**
  - 根因：`AssignStmt` 的全 nil 赋值删除优化只依据线性 future-read，错误删除了循环体内的
    `first = nil`。官方 `closure.lua` dump/load 后再次执行该函数时，下一轮循环看不到 `first` 变为 nil，
    导致闭包压力路径无界运行并触发内存上限。
  - 已落地源码修复：普通 `a = nil` 赋值不再做路径不敏感 dead-store 删除；仅保留局部声明初始化的保守省略。
  - 关联 binary chunk 修复：`loadstring` 反序列化本地 binary chunk 时改为按槽位追加常量，避免重复常量被
    `addConstant()` 去重后破坏 `LOADK` 常量索引。
  - 验证：`bin\lua_test.exe --max-memory-mb 128 --filter "post-vararg closure.lua tail"` 通过；
    `bin\lua_test.exe --max-memory-mb 128 --filter "post-vararg"` 为 5 selected results / 5 passed / 0 failed。

## 验证命令

开发时先跑最小切片，阶段收口前跑完整门禁。

```powershell
bin\lua_test.exe --max-memory-mb 128 --filter "Lua 5.1 Compatibility"
bin\lua_test.exe --max-memory-mb 128 --filter "post-vararg"
bin\lua_test.exe --filter "Lua 5.1 Official Suite"
bin\lua_test.exe --max-memory-mb 128 --filter "VM Dispatch"
bin\lua_test.exe --max-memory-mb 128 --filter "Metamethod"
bin\lua_test.exe --max-memory-mb 128 --filter "GC"
Push-Location tests\lua\official; ..\..\..\bin\lua_app.exe -e "gcinfo=gcinfo or function() return collectgarbage('count') end; dofile=function(n) local f=assert(loadfile(n)); local b=string.dump(f); f=assert(loadstring(b)); return f() end; dofile('db.lua')"; Pop-Location
bin\lua_test.exe
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_lua51_official_slow.ps1
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

- 短期内不把尚未实现的 `T` helper、官方 binary chunk 和 `T.listcode` parity 纳入默认必须通过门禁。
- 不声称官方 Lua 5.1 binary chunk 兼容，除非真的实现并测试官方格式。
- 不声称完整 Lua 5.1.5 等价，除非 `testC`、未裁剪官方 suite、codegen parity、binary chunk、GC 调度和 runtime isolation 都有验收证据。
- 不在 owning runtime context 有测试和迁移文档前移除 singleton 兼容 API。
