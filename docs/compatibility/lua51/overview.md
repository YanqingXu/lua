---
status: current
verified_against: src/compiler/; src/vm/; src/core/; src/lib/; src/gc/; tests/lua/official/; tests/unit/official/; tests/compatibility/; tools/check_lua51_official_sources.ps1; tools/run_lua51_official_strict.ps1; tests/lua/regressions/
last_checked: 2026-07-15
applies_to: Lua 5.1 兼容性边界与验证矩阵
---

# Lua 5.1 兼容性边界与验证矩阵

兼容性不是“语法能解析”的单点结论，而是 Compiler、VM、Runtime、GC、stdlib 和调试接口共同形成的可观察行为。本页统一维护支持面、已知差异和验证方法，避免矩阵、差异清单与测试说明互相漂移。

## 兼容层次

| 层次 | 主要契约 | 证据 |
|---|---|---|
| 词法/语法 | token 边界、优先级、作用域、vararg、table constructor | parser 单测、official syntax scripts |
| 字节码语义 | 38 个 Lua 5.1 风格 opcode 的字段与行为 | opcode coverage matrix、VM 单测 |
| 运行时值 | truthiness、number/string、table、closure、error object | core/metamethod/function tests |
| 调用协议 | fixed/multret、vararg、tailcall、native call | call pipeline 与 Lua function tests |
| 标准库 | base/string/table/math/coroutine/package/io/os/debug | stdlib 单测与 official scripts |
| GC | 可达性、弱表、userdata finalizer、字符串池 | GC/official/regression tests |
| Debug API | source/line、stack level、tailcall、hook | debuglib 与 trace tests |

## 当前实现策略

- 语言目标是 Lua 5.1 可观察语义，内部实现采用 C++23 类型和模块边界，不追求复刻 Lua C 源码布局。
- bytecode 使用 Lua 5.1 风格 38 opcode，但项目内部 Proto 二进制布局不承诺与官方 `luac` 文件格式直接互换。
- Lua number 使用项目定义的 `f64` 路径；格式化和字符串转数值以兼容测试而非宿主 locale 为准。
- table 内部可使用现代容器，只要键规范化、相等、metamethod 与遍历允许范围符合语言契约。
- GC 可采用项目策略接口和增量状态机，但 roots、弱引用、finalizer 和对象身份必须保持 Lua 行为。

## 高风险差异面

### 数值与字符串

需要重点对照 NaN、±0、指数格式、十六进制/边界数字、concat 与 `tostring`。不能直接把 `std::to_string` 或 locale 解析当作 Lua 格式。

### 表与 Metamethod

unordered 容器顺序不是兼容承诺。验证重点是 `__index/__newindex` 链、`__eq` 触发条件、`__le` fallback、`__call` 参数插入、弱模式与非法 key。

### 调用和多返回值

表达式列表末项展开、vararg、native 返回数量和 tailcall 是最容易跨 Compiler/VM 漂移的区域。必须以同一个 fixed/open 结果协议验证。

### 错误对象与调试信息

Lua 允许 `error()` 携带任意 Value。C++ 异常边界不得强制字符串化。source chunk id、line、traceback level 与 tail-call 占位属于可观察调试行为，但自由格式 trace JSON 不等于 Lua API 合同。

### GC 与终结器

finalizer 的队列与执行时机、弱表清理顺序和字符串驻留都可能让“值最终相同”的测试漏掉生命周期差异。对应脚本需要跨至少两个完整 collection cycle 观察状态。

## 不承诺的内部兼容

- C API 的二进制 ABI 与官方 Lua 动态库；
- 官方私有 struct 布局、指针地址、哈希桶顺序；
- `luac` 二进制块的逐字节互换，除非专门测试声明支持；
- 错误消息中非语义性的标点、内部类型名或地址；
- GC 每次 step 的精确对象数量和宿主分配时刻。

## 验证方法

官方套件分为两个不能混称的通道：

- `official-smoke` 通过 `lua_test --filter "Lua 5.1 Official Smoke"` 执行受控、分阶段的快速验证。它会缩减压力并拆分脚本，全部改写登记在 `tests/compatibility/lua51-official-smoke-deviations.json`；“外部 skip 表为 0”不代表 upstream 原样全量通过。
- `official-strict` 通过 `tools/run_lua51_official_strict.ps1` 在临时目录执行 SHA-256 清单锁定的原样 `all.lua`。该通道禁止源码改写，并已提升为 Linux Clang Release 的 required PASS；runner 在不改脚本的前提下把 stdout banner 记录为 `stageProfile`，保留每个官方脚本的开始时间和到下一阶段的耗时。`tests/compatibility/lua51-official-strict-xfails.json` 现在是合法空清单。
- `official-testc` 显式打开项目内部 `T` 模块，`api.lua` 原样 exact PASS。仓库中的 `code.lua` 仍由 upstream SHA 清单逐字节锁定；执行前只在内存中应用三项由 Lua 5.1.5 `luac` 证明的 opcode oracle 校正，再评估项目编译器。校正后的首个项目 parity gap 是该通道唯一 XFAIL，登记在 `lua51-official-testc-xfails.json`。
- `official-slow` 单独运行未经改写的 `sort.lua` 与 `verybig.lua`，两者现在都是 CI required gate；`lua51-official-slow-xfails.json` 保留合法空清单，防止已修复的超时被重新接受为 XFAIL。
- `lua51-differential` 使用官方 Lua 5.1 和本解释器运行同一批 Lua 探针，比较 stdout、stderr、退出码，并在 stdout 中编码返回类型、错误类别与 GC 弱引用副作用；`lua51-c-api-differential` 另将同一纯 C probe 分别链接两套 Runtime，覆盖核心表、遍历、比较、拼接、类型/线程/GC、auxlib、stack/info/local/hook 调试入口，以及 8 个 `luaopen_*` 标准库入口。

因此当前准确结论是：Lua 5.1 官方套件 staged smoke 在受控改写和压力缩减条件下通过，原始 TestC `api.lua` 已完整执行到 `OK`，slow `sort.lua`/`verybig.lua` 均为必过门禁且没有 XFAIL；Release strict `all.lua` 也已从 timeout XFAIL 提升为 upstream 原样全量 PASS。

### TestC 当前差异

Lua.org 对 5.1 发布的是通用 `lua5.1-tests.tar.gz`（2016-01-18，SHA-256 `49e4ca…f18ad`），而不是 5.1.5 专用测试包；Lua 5.1.5 源码包 SHA-256 为 `2640fc…95333`。`lua51-official-sources.json` 同时锁定两个官方 URL/哈希、最小 fixture 哈希和 `luac -l -p` 结果，`check_lua51_official_sources.ps1 -LuacPath <5.1.5-luac>` 会实际核验三组 nested Proto：`LOADNIL/LOADNIL/RETURN`、`LOADK/LOADBOOL/TEST/JMP/RETURN`、`LOADNIL/LOADBOOL/TEST/JMP/RETURN`。这三处与通用 suite 的旧期望不同，因此测试保持 upstream `code.lua` 字节不变，只做精确且唯一匹配的内存 oracle 校正。

校正 fixture 后，XFAIL 已重分类为 `code.lua-lua515-compiler-parity`：`repeat ... until nil` 在 Lua 5.1.5 中发射 `LOADNIL/LOADBOOL/TEST/JMP/RETURN`，本项目当前省略 `LOADBOOL`，首个精确诊断为 `expected LOADBOOL / actual TEST`。这是真实编译器 parity gap，不再把实现差异误报成 fixture 来源问题；其他断言不能冒充该 XFAIL。

截至 2026-07-15，[#3](https://github.com/YanqingXu/lua/issues/3) 的 strict timeout 已闭环：profile 证明前置脚本均快速完成，但 `gc.lua` 的 stop 状态被错误保留，使 `closure.lua` 的弱表等待循环无法推进；无参数完整 `collectgarbage()` 恢复 Lua 5.1 的自动 GC 重启语义后，原样 `all.lua` 完整执行到 `final OK !!!`。[#4](https://github.com/YanqingXu/lua/issues/4) 的 fixture/oracle 来源问题也已闭环并转为追踪校正后暴露的 repeat-condition 编译器 gap；`api.lua` 过去的 stack-shape XFAIL 已因 exact PASS 从清单移除。

1. strict official Lua 脚本验证规范行为，不修改预期来迎合实现；smoke 的任何改写必须登记 deviation。
2. unit tests 锁定 C++ 内部不变量，如 opcode 双向覆盖、frame 窗口与错误对象保留。
3. regressions 为每个修复保存最小 Lua 输入与可观察结果。
4. 差异探针在官方 Lua 5.1 与本解释器上运行，比较类型、值、错误类别和副作用；只在稳定时比较文本。
5. 失败先按 Compiler → VM → Runtime → GC 分层定位，再判断是缺陷、明确不支持还是实现自由度。

## 接受差异的门槛

差异只有在以下信息齐全时才可记录为边界，而不是临时跳过：

- 官方 Lua 5.1 的参照行为；
- 项目当前行为与最小复现；
- 差异属于规范、实现细节还是尚未实现；
- 对 Compiler/VM/Runtime/GC/stdlib 的影响范围；
- 对应自动化测试或明确的不可测试理由。

技术入口见 [文档索引](../../index.md)，源码责任区见 [源码与文档映射](../../knowledge/source-document-map.md)。
