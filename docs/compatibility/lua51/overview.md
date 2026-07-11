---
status: current
verified_against: src/compiler/; src/vm/; src/core/; src/lib/; src/gc/; tests/lua/official/; tests/unit/official/; tests/lua/regressions/
last_checked: 2026-07-11
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

1. official Lua 脚本验证规范行为；不修改预期来迎合实现。
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
