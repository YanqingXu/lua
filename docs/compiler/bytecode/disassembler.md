---
status: current
verified_against: src/bytecode/bytecode_printer.hpp; src/bytecode/bytecode_printer.cpp; src/bytecode/bytecode_main.cpp; src/core/function.hpp; src/compiler/opcode.hpp; tests/unit/bytecode/
last_checked: 2026-07-11
applies_to: Proto disassembly and control-flow rendering
---

# Proto 反汇编与控制流展示

反汇编器把 `Proto` 的编码状态转换成可审查证据，用于区分 CodeGen 错误与 VM 执行错误。它只读访问 Proto，不改变指令、常量或 GC 关系。

## 输出层次

| 视图 | 稳定信息 | 用途 |
|---|---|---|
| compact | PC、opcode、A/B/C/Bx/sBx | 快速比较指令序列 |
| full | Proto 属性、常量、局部范围、upvalue、行号和注释 | 诊断寄存器、binding 与 multret |
| diff | 两个规范化 Proto 视图的结构差异 | 评估 CodeGen 变更 |
| CFG | basic block 与 jump edge | 审查分支、循环和不可达代码 |

具体命令参数由 `bytecode_main.cpp` 的帮助入口维护，技术百科只定义输出应表达的语义。

## 指令注释

printer 通过 `src/compiler/opcode.hpp` 解码字段，并结合常量池/Proto 信息显示：

```text
pc | source line | opcode | raw operands | resolved meaning
```

RK operand 应区分 `R(n)` 与 `K(n)`；ABx 指令解析常量或子 Proto；AsBx 指令显示相对偏移和绝对目标。注释是派生信息，原始字段仍必须保留，避免 printer 的解释错误掩盖编码事实。

## Proto 信息

full view 至少关联以下数据：

- source、line defined、parameters、vararg flags、maxStackSize；
- code 与逐 PC lineInfo；
- constants 及其 Value 类型；
- local variable 的 start/end PC 与 register；
- upvalue descriptor 与 nested Proto。

对象地址和 unordered 容器顺序不属于稳定输出。字符串和 table-like debug 值需要转义、深度限制与确定性排序策略。

## CFG

basic block 边界来自入口、jump target、条件跳过后的 fallthrough 和终止指令之后。edge 应区分 unconditional、true/false、loop backedge 和 fallthrough。CALL 通常不是 CFG 终点；RETURN/TAILCALL 是。

CFG 是控制流近似，不执行 metamethod、异常或 coroutine 转移。它用于验证 patch 后目标与结构，不应被描述为完整动态调用图。

## 验证不变量

- 所有 PC 只访问 Proto code/lineInfo 的合法范围。
- 解码与 `opcode.hpp` 使用同一位宽、偏置和 RK 规则。
- diff 忽略地址/路径等非语义噪声，但保留 opcode、operand、constant 和 debug range 差异。
- CFG 的每个 jump target 都是合法指令边界；fallthrough 不越过 code 末尾。
- printer 不触发 Lua metamethod，不改变栈、GC roots 或 Proto。

字节码生成模型见 [字节码生成](../bytecode-generation.md)，opcode 语义见 [VM 指令集](../../vm/instruction-set.md)。
