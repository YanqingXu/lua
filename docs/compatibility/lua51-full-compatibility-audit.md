---
status: current
verified_against: docs/compatibility/lua51.md; docs/roadmap/lua51-compatibility-next-stage.md; tests/lua/official/all.lua; tests/lua/official/code.lua; tests/lua/official/api.lua; tests/unit/official/test_official_suite.cpp; tests/lua/official/etc/ltests.c; src/vm; src/compiler/codegen; src/lib; src/gc; src/runtime; src/core
last_checked: 2026-06-02
applies_to: Lua 5.1.5 full compatibility gap audit
---

# Lua 5.1.5 Full Compatibility Gap Audit

本文基于 `docs/compatibility/lua51.md` 与
`docs/roadmap/lua51-compatibility-next-stage.md` 中记录的基线，对当前源码与
Lua 5.1.5 完整等价目标之间的剩余差距做一次专项审计。

结论：当前项目已经具备较强的 Lua 5.1 主路径语义兼容性，默认单测、
`Lua 5.1 Compatibility` 探针和官方 staged smoke 均保持通过；但它仍不应被表述为
Lua 5.1.5 完全等价实现。主要缺口集中在：

- `api.lua` / `code.lua` 的官方 `testC` 路径未执行。
- `code.lua` 的 `T.listcode` 会暴露当前 codegen 与官方 `lcode.c` 优化策略的 opcode
  形状差异。
- `string.dump` / `loadstring` 仅支持项目本地 binary chunk 格式。
- `IncrementalGC` 策略入口仍保留教学占位边界，`collect()` 仍走完整 mark-sweep。
- `EngineContext` 已落地，但 legacy singleton fallback 仍未完全收口。
- 官方 staged smoke 使用 `_soft=true`，并保留对压力路径和 all.lua 后段脚本的裁剪。

## Current Verification Snapshot

2026-06-01 / 2026-06-02 复核命令：

```powershell
bin\lua_test.exe --filter "Lua 5.1 Official Suite"
bin\lua_test.exe --max-memory-mb 128 --filter "Lua 5.1 Compatibility"
bin\lua_test.exe --max-memory-mb 128 --filter "GC"
bin\lua_test.exe --max-memory-mb 128 --filter "post-vararg"
bin\lua_test.exe --max-memory-mb 128 --filter "closure.lua weak GC loop cap"
```

复核结果：

| Suite | Selected | Results | Failures | Notes |
|---|---:|---:|---:|---|
| Lua 5.1 Official Suite | 10 | 10 | 0 | default 512 MB cap 下 staged all.lua、global cleanup tail、post-vararg split guard 均通过；`closure.lua`、`errors.lua`、`math.lua`、`files.lua` post-vararg dump/load tail 在 128 MB cap 下通过；closure 后 cleanup tail 当前隔离为 known gap guard；输出确认 `code.lua` / `api.lua` 走 `testC not active` 分支 |
| Lua 5.1 Compatibility | 9 | 35 | 0 | P0/P1 可见语义探针通过 |
| GC | 31 | 138 | 0 | GC 单元与 REPL GC 探针通过 |

最近一次完整绿跑显示 registered tests 为 654，assertion results 为 3340；2026-06-02 新增安全门禁后，
当前 test binary 注册测试数为 659，并默认安装 512 MB 进程内存硬上限。旧文档中的较早测试计数是此前快照，
不影响本次审计结论。

## Priority Model

沿用 roadmap 中的 P0-P3 体系：

| Priority | Meaning |
|---|---|
| P0 | 可观察 Lua 5.1 语义不一致，或静默错误行为 |
| P1 | 标准库公开 API 缺口，或高价值 debug/C API 边界 |
| P2 | VM/opcode 边界测试、binary chunk 兼容策略、文档漂移 |
| P3 | 更大的运行时架构工作，例如精确 GC 工作量模型和多实例隔离收口 |

## 1. VM And Instruction-Set Optimization

### Current State

`src/vm/` 的执行层已经覆盖 Lua 5.1 风格的 38 条 opcode。dispatch、switch handler 和
分模块 handler 均能覆盖主执行路径，这一层不是当前完整等价的主要风险。

主要差距在 compiler/codegen。`tests/lua/official/code.lua` 并不只是测试语义结果，它通过
`T.listcode` 检查官方 Lua 5.1 编译器生成的精确 opcode 序列。当前 staged suite 中
`T == nil`，因此该路径直接跳过。

### Gap Against Official `lcode.c`

官方 Lua 5.1 `lcode.c` 在生成字节码时做了多类形状优化，`code.lua` 会直接观察这些形状：

- 连续 `LOADNIL` 合并与无效 nil 初始化消除。
- 数值常量折叠。
- `..` 拼接表达式的 `CONCAT` 合并。
- 布尔条件与 `not not` 规约。
- 直接 local/table assignment 的寄存器复用。
- `a = a` 这类自赋值不发无效代码。
- 比较、跳转和 return 组合的精确规约。

当前 codegen 更偏语义正确优先：

- `LocalStmt` 已对无初始化局部变量和显式 nil-only local 初始化发出区间 `LOADNIL`。
- assignment 路径会冻结 lvalue，再使用 scratch/value base 回写，容易产生额外 `MOVE`。
- 三段及以上 concat chain 当前会先物化到 scratch operand range，再发单条 `CONCAT target,target,last`；这是为了兼顾 `code.lua` 关心的合并形状和当前 VM `CONCAT` 会改写 `B..C` 源寄存器的事实。
- 二元算术路径已对数值字面量执行 Lua 5.1 风格常量折叠，并保留 div/mod by zero 与 NaN 不折叠边界；非字面量路径仍使用 RK operand 发算术 opcode。
- 连续局部变量 return 已直接复用原寄存器；单局部 `a = a` 自赋值已作为 no-op 消除。
- 布尔 materialization 和条件跳转能执行正确语义，但不保证与官方 opcode 序列相同。

### Risk

这是 P2 缺口。它通常不造成 Lua 程序可见语义错误，但会阻断：

- 官方 `code.lua` 的 `T.listcode` 精确检查。
- 与官方 `luac -l` 级别的字节码形状对齐。
- 将项目称为“完整 Lua 5.1.5 等价”的声明。

### Required Path

2026-06-01 进展：`tests/unit/compiler/test_codegen_characterization.cpp` 已建立独立
codegen parity characterization suite，锁住显式 nil local 的 `LOADNIL` range merge
目标，以及 concat、`not not`、direct local/table assignment、constant folding 和
`a = a` 自赋值形状。随后已完成显式 nil local 的 `LOADNIL` range merge、
数值字面量 arithmetic constant folding、direct contiguous local return 和单局部
`a = a` 自赋值消除；三段及以上 concat chain 也已合并为单条 `CONCAT`，并通过
scratch operand range 避免覆盖 active locals/params/upvalues；常量 `not not nil/false/true/1`
已折叠为单条 `LOADBOOL`，对齐 `code.lua` 中的对应 `T.listcode` 断言形状。

后续仍需逐项实现：

1. 暴露项目版 `T.listcode`，让 `code.lua` 可运行到真实断言。
2. 以官方 `code.lua` 断言为最小目标继续细化失败用例。
3. 继续补 `LOADNIL` elide、动态 boolean/jump normalization。
4. 梳理 register allocator 与 assignment emission，使直接 local/table 赋值形状接近官方。
5. 最后把 `code.lua` 的 `T.listcode` 路径加入默认 official suite gate。

## 2. C API And `testC` Helper Module

### Current State

`tests/lua/official/etc/ltests.c` 和 `ltests.h` 已随官方测试文件存在，但项目没有官方 Lua C API
兼容头与 ABI shim。当前 `api.lua` 因 `T == nil` 直接跳过，所以 staged smoke 通过不能证明
C API 边界等价。

`api.lua` 依赖的不是一个简单 Lua helper，而是接近完整的 `ltests.c` surface：

- stack API：`gettop`、`settop`、`insert`、`remove`、`replace`、`pushvalue`。
- call API：`call`、`rawcall`、`pcall`、error propagation。
- table API：`gettable`、`settable`、`next`、registry/ref/unref/getref。
- userdata API：`newuserdata`、`udataval`、`objsize`、metatable、`__gc`。
- closure/upvalue API：`pushcclosure`、`lua_getupvalue`、`lua_setupvalue`。
- environment API：global env、function env、C function env。
- multi-state API：`newstate`、`closestate`、`doremote`、`loadlib`。
- allocator/memory API：`totalmem` 与低内存路径。
- thread/hook/debug/auxlib API：new thread、hooks、`luaL_gsub` 等。

### Gap

当前 `src/lib/packagelib.cpp` 的动态 C 模块加载会把符号转换成项目内部
`CFunction`/`LibCFunction`，也就是 `i32 (*)(LuaState*)`。这不是官方
`lua_CFunction(lua_State*)` ABI。因此即使 package 动态库主路径存在，它也不能加载按
Lua 5.1 官方 C API 编译的模块。

### Risk

这是 P1 缺口。原因是它直接影响：

- 官方 `api.lua` 完整覆盖。
- 嵌入式使用者对 `lua_State`、stack index、registry、refs、userdata、allocator 的预期。
- 官方 C module 和官方测试 `libs/*.c` 的可加载性。

### Required Path

完整路径应是实现官方 C API compatibility shim：

1. 新增或生成 `lua.h`、`lauxlib.h`、`lualib.h` 兼容头。
2. 定义 `lua_State` 到项目 `LuaState` / `EngineContext` 的稳定映射。
3. 实现 Lua 5.1 stack discipline，包括正负 index、pseudo-index、registry、upvalue index。
4. 实现 refs、userdata、C closure/upvalue、function env、error/pcall、allocator hooks。
5. 让 `ltests.c` 编译为项目内 `T` 模块。
6. 改造 `package.loadlib`，支持官方 `lua_CFunction` ABI，同时保留项目内部 ABI 的迁移策略。

## 3. Binary Chunk Compatibility

### Current State

`string.dump` 当前写出项目本地格式：

- 头部包含 Lua 5.1 风格 signature/version marker。
- payload 中追加 `"LC++"` marker。
- 后续序列化的是项目自己的 `Proto`、instruction、constant、subproto、debug info。

`loadstring` / `loadfile` / `load` 只在识别到项目本地 chunk 时走 binary loader；否则按源码解析。
这能支持项目内部 dump/load round-trip，但不声明官方 Lua 5.1 binary chunk 兼容。

### Gap Against Official Lua 5.1

官方 Lua 5.1 binary chunk 格式由 `ldump.c` / `lundump.c` 定义：

- header 由 `luaU_header` 生成，包含 signature、version、format、endianness、`sizeof(int)`、
  `sizeof(size_t)`、`sizeof(Instruction)`、`sizeof(lua_Number)`、integral-number flag。
- function body 按官方 `DumpFunction` / `LoadFunction` 顺序写入。
- string 使用 native `size_t` 长度并包含尾 NUL。
- 常量、Proto、lineinfo、locvars、upvalue names 的顺序和 encoding 都有固定约定。

当前本地格式使用 `"LC++"` marker 和项目私有 encoding，因此：

- 不能读取官方 Lua 5.1 `string.dump` 或 `luac` 产物。
- 官方 Lua 5.1 不能读取项目 `string.dump` 产物。
- 虽然版本 byte 为 `0x51`，但语义上不是官方 Lua 5.1 binary chunk。

### Risk

这是 P2 缺口。它影响二进制互通和“完整等价”声明，但不影响源码执行主路径。

### Required Path

建议采用双格式策略：

1. 保留 `"LC++"` 项目本地格式，作为内部 round-trip 与调试格式。
2. 新增 official Lua 5.1 chunk reader/writer，按官方 header 和 `DumpFunction` 顺序实现。
3. 明确 ABI 限制：Lua 5.1 binary chunk 本身依赖 endian 和 `sizeof`，兼容目标应是“同 ABI 官方
   Lua 5.1.5 互通”。
4. 增加 fixture：官方 Lua 5.1 dump -> 项目 load；项目 official dump -> 官方 Lua 5.1 load。

## 4. Memory Management And Runtime Isolation

### Incremental GC

当前 GC 已实现较多 Lua 可见语义：

- mark-sweep 主收集路径。
- weak table 清理。
- userdata `__gc` 两阶段终结与复活主路径。
- `collectgarbage("setpause")` / `setstepmul` 状态。
- `collectgarbage("step")` 的 pause/propagate/atomic/sweep/finalize 分阶段推进。
- table/metatable/function/upvalue/global-state 的保守写屏障。

剩余差距是策略级别精确性：

- `IncrementalGC::collect()` 仍直接调用 full `collectMarkSweep()`。
- 当前 phase 模型没有完整复刻官方 Lua 5.1 的 `GCSpropagate`、`GCSsweepstring`、
  `GCSsweep`、`GCSfinalize` 以及 threshold/debt 调度。
- 写屏障策略偏保守，可保护对象图，但不等价于官方 gray/black list 与 barrierback 模型。

### EngineContext And Singleton Fallback

`EngineContext` 和显式 `RuntimeServices` 已落地，这是正确方向。但以下 legacy path 仍存在：

- `RuntimeServices::fromSingletons()`。
- `LuaState::newState()` 默认 singleton 入口。
- VM/compiler 的兼容 overload。
- metatable、GC finalizer/weak table、string pool 相关 fallback。

### Risk

这是 P3 缺口。它通常不影响单实例脚本主路径，但会影响：

- 多实例隔离。
- 嵌入式生命周期管理。
- 精确复现官方 GC 工作量、触发时机、终结顺序和 step 返回值边界。

### Required Path

1. 让所有公开入口优先或强制要求 `EngineContext` / explicit `RuntimeServices`。
2. 给 legacy singleton path 加 deprecation gate，并逐步迁移测试夹具。
3. 移除 `GlobalState::getInstance()` / `StringPool::getInstance()` 在生产路径的 fallback。
4. 将 `IncrementalGC` 策略入口接到真实 incremental scheduler，而不是 full collect alias。
5. 建立 GC debt/threshold/phase 的对照测试，覆盖 weak/finalizer/barrier/step 组合边界。

## 5. Standard Library Edge Behavior

### Current State

`src/lib/lib_catalog.cpp` 已注册 9 个标准库：

- base
- math
- io
- string
- table
- os
- coroutine
- debug
- package

主路径覆盖强，`Lua 5.1 Compatibility` suite 也补了多项此前 staged smoke 不容易暴露的 P0/P1
行为：nil table key、数字字符串转换、除零/取模零、table length、stdin loadfile/dofile、
io.lines format、os failure triple、C function env、error/xpcall。

### Soft Harness And Trimmed Official Paths

官方 staged smoke 当前并非原样 all.lua：

- `_soft=true`。
- `code.lua` 和 `api.lua` 因缺少 `T` 模块跳过内部测试。
- `newproxy(true)` finalizer 压力块被移除。
- `verybig.lua` 的部分 `collectgarbage()` / `showmem()` 行为被裁剪。
- `showmem()` 被替换为 no-op。
- all.lua 在 `dofile('vararg.lua')` 后截断，未运行后续：
  - `closure.lua`
  - `errors.lua`
  - `math.lua`
  - `sort.lua`
  - `verybig.lua`
  - `files.lua`
  - debug hook tail
  - 清空 `_G`
  - 最终多轮 GC / showmem
- `constructs.lua` 的动态编译压力循环被 capped 到 32。
- `gc.lua` 的 run-until-collection 路径被替换为 64 次 bounded probe。

### Additional Library Boundaries

完全等价还需要继续审计：

- strict Lua 5.1 模式是否应隐藏 additive APIs，例如 `table.pack`、`table.unpack`、`table.move`。
- `debug.getinfo/getlocal/setlocal/sethook/gethook/traceback` 的文本、level、tail-call、C frame 边界。
- `package.loadlib` / `require` 对官方 C 模块 ABI、错误文本和平台 loader 行为的兼容。
- `io` / `os` 在 Windows CRT 和 Lua 5.1 官方测试中的文件句柄、rename/remove、tmpname 边界。
- `string.format`、pattern、`gsub` auxlib 路径与 `T.gsub` 的覆盖。
- 低内存、长字符串、超大 table、排序比较器异常等压力行为。

### Risk

这些缺口分布在 P0/P1：

- 若去掉 harness 裁剪后出现可见语义错误，则升为 P0。
- debug/package/io/os/string 等公开 API 细节不一致通常是 P1。

## Unfinished Item Inventory

| ID | Priority | Area | Item | Completion Signal |
|---|---|---|---|---|
| L51-AUDIT-001 | P0 | Official suite | 去掉 `_soft` 裁剪后运行完整 all.lua tail | 原样 all.lua 或最小注入 harness 通过，无脚本截断 |
| L51-AUDIT-002 | P0 | Standard libs | 恢复 `closure/errors/math/sort/verybig/files` 后段覆盖 | 后段脚本稳定通过，失败项拆成具体 P0/P1 修复 |
| L51-AUDIT-003 | P1 | C API | 实现 Lua 5.1 C API shim | 官方 `lua.h/lauxlib.h/lualib.h` 兼容入口可编译 |
| L51-AUDIT-004 | P1 | `testC` | 编译并注册官方 `ltests.c` 风格 `T` 模块 | `api.lua` 不再走 `T == nil` 跳过分支 |
| L51-AUDIT-005 | P1 | Dynamic C modules | `package.loadlib` 支持官方 `lua_CFunction` ABI | 官方测试 `libs/*.c` 可加载并通过 |
| L51-AUDIT-006 | P1 | Debug/package/io/os | 跑通未裁剪官方 debug/files/package 边界 | 错误文本和返回值差异有明确策略 |
| L51-AUDIT-007 | P2 | Codegen | 建立 `T.listcode` parity suite | `code.lua` 可运行到真实 opcode 断言 |
| L51-AUDIT-008 | P2 | Codegen | 复刻 `lcode.c` 关键 peephole/codegen 优化 | `LOADNIL`、constant folding、concat、boolean、assignment 形状对齐 |
| L51-AUDIT-009 | P2 | Binary chunks | 新增官方 Lua 5.1 chunk reader/writer | 同 ABI 官方 Lua 5.1 chunk 双向互通 |
| L51-AUDIT-010 | P2 | Compatibility mode | 定义 strict 5.1 与 project extension 模式 | additive APIs 在 strict 模式有明确行为 |
| L51-AUDIT-011 | P3 | GC | `IncrementalGC::collect()` 接入真实 incremental scheduler | 不再是 full mark-sweep alias |
| L51-AUDIT-012 | P3 | GC | 对齐官方 GC phase/debt/threshold 模型 | step/auto GC 工作量边界有对照测试 |
| L51-AUDIT-013 | P3 | Runtime isolation | 移除生产路径 singleton fallback | 多 `EngineContext` 隔离测试通过 |
| L51-AUDIT-014 | P3 | Embedding | 生命周期、allocator、registry、thread 与 C API 统一 | 嵌入式 smoke + `api.lua` state tests 通过 |

## Critical Path To Full Equivalence

推荐实现顺序：

1. **先解锁观测能力。**
   实现 C API shim 和 `T` 模块，让 `api.lua` / `code.lua` 从跳过变成真实失败。没有这一步，
   “完整兼容性”缺口只能靠推断。

2. **再收 P0/P1 可见语义。**
   逐步撤销 `_soft` 裁剪和 all.lua 截断，先让源码级官方 suite 原样或近原样通过。

3. **随后做 P2 字节码等价。**
   以 `code.lua` 为验收，系统修 codegen opcode shape；以官方 chunk fixture 为验收，补
   binary dump/undump。

4. **最后做 P3 架构等价。**
   完成 GC scheduler 与 runtime isolation 收口。该阶段对普通 Lua 脚本主路径影响较小，但对
   完整等价、嵌入和多实例可靠性是必要条件。

## References

- Lua 5.1.5 source: https://www.lua.org/source/5.1/
- Official code generator: https://www.lua.org/source/5.1/lcode.c.html
- Official dumper: https://www.lua.org/source/5.1/ldump.c.html
- Official undumper: https://www.lua.org/source/5.1/lundump.c.html
- Official GC implementation: https://www.lua.org/source/5.1/lgc.c.html
