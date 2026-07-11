---
status: current
verified_against: src/core/value.hpp; src/core/value.cpp; src/core/gc_object.hpp; src/core/gc_string.hpp; src/core/string_pool.hpp; src/core/string_pool.cpp; src/core/userdata.hpp; src/debug/value_serializer.cpp; tests/unit/core/; tests/unit/metamethod/; tests/lua/basic/; src/core/; src/runtime/; src/vm/; tests/unit/vm/; tests/lua/runtime/
last_checked: 2026-07-11
applies_to: Value 表示、对象引用、相等性、转换与调试输出
---

# Value 表示、对象引用、相等性、转换与调试输出

`Value` 是 Compiler 常量池、VM 寄存器、LuaState 栈、table 键值和 C++ API 之间的共同数据单位。设计目标是让标量值复制廉价，让对象身份稳定，并把 Lua 语义与 C++ 存储细节明确分层。

## 1. 值分类

| Lua 类别 | C++ 表示要点 | 身份/所有权 |
|---|---|---|
| nil | 空状态 | 无对象身份 |
| boolean | `bool` | 按值 |
| number | `f64` | 按值；注意 NaN、±0 与整数外观 |
| string | `GCString*` | interned GC 对象 |
| table | `Table*` | GC 对象身份 |
| function | `Function*` | GC 对象身份 |
| userdata | `Userdata*` 或 light userdata | full userdata 被 GC 跟踪；light userdata 是非拥有地址值 |
| thread | `LuaState*`/线程对象 | GC/全局状态管理 |

`Value` 中的对象指针是观察引用：复制 Value 不复制对象，也不转移所有权。对象存活由 GC root 与可达边决定。

## 2. 基本语义

Lua 只有 nil 和 false 为假；number 0 与空字符串都为真。类型判断和真值转换必须使用 Value API，不能依赖 C++ 的 `if (pointer)` 或 `double != 0` 直觉。

number 采用浮点表示。字符串到数字的显式/隐式转换使用统一 number conversion 边界，避免 stdlib、VM 算术和 API 各自接受不同格式。

字符串驻留由 `StringPool` 保证相同字节序列映射到规范 `GCString`，因此内部快速路径可以比较指针；语言层相等性仍定义为字符串内容相等，不能把实现优化写成对外语义。

## 3. 相等性

原始相等遵守：

- nil 只等于 nil；boolean 按布尔值；number 按数值；
- string 按内容（驻留后通常退化为身份比较）；
- table、function、thread、userdata 默认按对象身份；
- 不同类型通常不相等。

table/userdata 的 `__eq` 是额外的 metamethod 路径，由 VM 比较操作负责协调。`Value::operator==` 不应隐式执行 Lua 代码，否则哈希容器与内部不变量会产生隐藏副作用。

NaN 不能作为稳定的 Lua table key；number key 的 -0/+0 规范化必须与哈希和相等使用同一规则。

## 4. 转换边界

转换分为三类：

| 类型 | 示例 | 失败处理 |
|---|---|---|
| 无损查询 | `isNumber()/asNumber()` | 前置类型断言或受控错误 |
| Lua 兼容转换 | number-like string → number | 使用共享解析规则，返回可失败结果 |
| 展示转换 | debug/错误消息/print | 不改变语言对象，不作为 round-trip 格式 |

不要让 `toString()` 同时承担语言 `tostring` metamethod、内部诊断和序列化；三者的副作用、稳定性与循环处理要求不同。

## 5. 对象引用与 GC

```text
Value in root/GC object
        │ observer edge
        ▼
     GCObject
```

GC 扫描已知 Value 容器并追踪对象指针。栈、全局表、注册表、活动 closure/upvalue 和临时保护对象构成 roots。C++ 局部 Value 若跨越可能触发 GC 的调用，必须通过项目的 root/protection 机制保持可达，不能假设局部变量本身会被收集器发现。

RAII 适合管理 root guard 的登记与撤销，以及 userdata 持有的外部资源；它不替代 Lua 对象图的 mark/sweep 语义。

## 6. Userdata 与 native 地址

full userdata 是带 payload、metatable 与可能 finalizer 的 GC 对象。light userdata 只是地址型标量，不拥有目标，也不会自动延长目标生命周期。公开 API 必须明确地址的来源和有效期。

userdata finalizer 可能让对象进入待终结队列并延迟真正释放，因此“不可从普通 root 到达”不等于“本周期立刻析构”。

## 7. 调试输出

`src/debug/value_serializer.cpp` 为 trace 生成结构化、有限深度的 Value 表示。序列化必须：

- 区分类型与展示文本；
- 对对象使用稳定的本次运行身份标识；
- 限制 table 深度和元素数，检测循环；
- 不调用可能执行 Lua 代码的 metamethod；
- 不把地址或日志字符串当作兼容性契约。

## 8. 核心不变量

- Value 复制不复制 GC 对象，移动后不会产生双重所有权。
- 哈希、相等与 number-key 规范化一致。
- string pool 与 GC 协作，不留下指向已回收字符串的驻留项。
- observer 指针只在对象被 root/可达关系保护时使用。
- 语言转换、API 类型检查和调试展示是三个独立层次。

Table 对 Value 的使用见 [Table 与 Metatable](../table/overview.md)，对象回收见 [GC 实现](../../gc/implementation.md)。
