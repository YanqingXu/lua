---
status: current
verified_against: src/common/lua_error.hpp; src/compiler/lexer/; src/compiler/parser/; src/compiler/codegen/; src/vm/; src/debug/; tests/unit/compiler/test_parser_error_recovery.cpp; tests/unit/compiler/test_codegen_characterization.cpp; tests/unit/vm/test_vm_trace_debug.cpp; src/compiler/
last_checked: 2026-07-11
applies_to: 错误分类、诊断流程与典型故障
---

# 错误分类、诊断流程与典型故障

本文给出解释器从源码输入到运行时失败的统一诊断模型。目标不是罗列调试命令，而是说明错误在哪个边界产生、应观察什么证据，以及如何把症状收敛到可验证的源码责任区。

## 1. 错误模型

### 1.1 异常层次

`src/common/lua_error.hpp` 以 `LuaError` 统一解释器异常，同时保持与 `std::runtime_error` 的 C++ 边界兼容：

```text
std::runtime_error
└── LuaError
    ├── ParseError        # 语法错误，携带行列
    ├── CodegenError      # 字节码生成失败
    └── RuntimeError      # VM 或运行时语义失败
        └── MemoryError   # 栈、对象或资源耗尽
```

词法器没有独立的 `LexerError` 类。非法输入首先表示为错误 token，再由解析边界转换成带源码位置的 `ParseError`。这使 lexer 保持流式扫描职责，同时让 parser 成为统一的编译期诊断出口。

`LuaError` 还能保存任意 Lua `Value` 作为错误对象。这一点很重要：`error(table)` 不应在穿过 C++ 异常边界时被过早压扁成字符串；只有标准异常接口需要文本时，才生成兼容消息。

### 1.2 按流水线定位

| 阶段 | 常见表现 | 第一证据 | 主要责任区 |
|---|---|---|---|
| Lexer | 非法字符、数字格式异常、字符串未闭合 | token 类型、行列、字面量 | `src/compiler/lexer/` |
| Parser | 缺失分隔符、非法语句、恢复后级联报错 | `ParseError`、当前/前瞻 token | `src/compiler/parser/` |
| CodeGen | 寄存器、跳转、赋值目标或 multret 契约破坏 | AST 节点、生成位置、指令序列 | `src/compiler/codegen/` |
| VM | 类型错误、调用协议、非法 opcode、栈边界 | PC、opcode、寄存器/调用帧、trace | `src/vm/` |
| Runtime | table、metamethod、closure、coroutine 语义偏差 | `Value` 类型、对象身份、upvalue 状态 | `src/core/`、`src/runtime/` |
| GC/资源 | 悬空引用、错误回收、栈溢出、对象增长 | root 集、对象标记、栈容量、生命周期 | `src/gc/`、`src/core/gc_object.hpp` |

一个症状可能跨越多个阶段。例如“函数返回值错误”可能来自 parser 对表达式列表的结构化错误、codegen 对 multret 的错误编码，也可能来自 VM 对 `CALL`/`RETURN` 的栈顶协议实现错误。诊断时应以最早出现分歧的证据为准，而不是以最终报错位置为准。

## 2. 证据层

解释器提供三种互补证据，粒度从静态到动态逐级增加：

1. AST 与编译错误：确认输入是否被理解为预期语法结构。
2. 字节码与函数原型：确认 opcode、常量池、寄存器和跳转目标是否符合生成契约。
3. VM trace：确认每条指令执行前后的 PC、栈、调用帧和值状态。

字节码生成与反汇编见 [字节码生成](../compiler/bytecode-generation.md) 和 [反汇编器](../compiler/bytecode/disassembler.md)，动态事件模型见 [VM Trace 系统](../vm/trace-system.md)。trace 采用结构化事件，而不是把人类可读日志当作稳定接口；`src/debug/trace_types.hpp` 定义事件，sink 负责 JSON 等输出形式。

建议始终保留“源码片段 → AST/Proto → trace → 最终值”的关联。只有最终错误消息而没有中间表示，通常不足以区分编译器错误和 VM 错误。

## 3. 标准排查流程

### 3.1 固化最小失败样例

删除与失败无关的函数、分支和库调用，直到样例只保留一个语义点。最小样例必须同时记录：

- Lua 源码与预期结果；
- 实际结果或异常类型；
- 失败属于解析、生成还是执行阶段；
- 若是行为差异，Lua 5.1 的参照结果。

缩减过程中若故障消失，最近删除的结构往往暴露了真正的交互条件，例如尾调用与可变返回值同时出现。

### 3.2 建立阶段二分

按以下顺序寻找最早分歧点：

```text
token → AST → Proto/bytecode → VM instruction state → runtime object graph
```

- token 或 AST 已错：停在 frontend，不要修改 VM。
- AST 正确但 Proto 错：检查 codegen 的寄存器、常量池、跳转回填和返回值模式。
- Proto 正确但执行错：对照 opcode handler 前后的 PC、base、top 和寄存器。
- 指令状态正确但对象行为错：检查 table/metamethod、upvalue 或 GC 生命周期。

### 3.3 比较状态变化而非日志文本

对 VM 故障，最有价值的比较单位是“一条指令造成的状态变化”：

```text
(pc, opcode, base, top, registers_before)
                    ↓
(next_pc, base, top, registers_after, frame/upvalue changes)
```

结构化 trace 可以稳定比较这些字段；自由格式文本容易因展示调整产生噪声。测试应断言语义字段或最终可观察行为，而不是整段日志字符串。

### 3.4 把修复落到正确层

修复位置遵循两个原则：

- 语法约束由 lexer/parser 负责，不能依赖 VM 在运行时兜底。
- 指令契约由 codegen 与 handler 共同维护，不能通过外围特殊分支掩盖不一致。

修复后至少补一个最小单元测试和一个 Lua 回归用例。前者锁定内部契约，后者锁定语言可观察行为。

## 4. 典型故障案例

### 4.1 合法输入被拒绝或错误输入被接受

先检查 lexer 的前瞻与 token 边界，再看 parser 的同步点。连续出现多个 `ParseError` 时，第一个错误最可信，后续错误可能只是错误恢复造成的级联。解析恢复的现有基准见 `tests/unit/compiler/test_parser_error_recovery.cpp`。

不要通过放宽某个通用匹配函数来修复单一语法；这可能让错误输入进入 codegen，造成位置更晚、信息更差的失败。

### 4.2 字节码正确性与 VM 行为不一致

先反汇编最小函数，检查：

- opcode 是否与 AST 节点匹配；
- `A/B/C/Bx/sBx` 字段和 RK 编码是否正确；
- 跳转目标是否落在合法指令边界；
- 目标寄存器是否仍在分配器生命周期内。

若 Proto 正确，再对照 `src/vm/vm_handlers/` 中对应 handler。重点检查 PC 的增量归属：dispatch 与 handler 只能有一个层次负责某次 PC 更新，否则容易出现跳过或重复执行。

### 4.3 调用、返回值数量或尾调用错误

Lua 的调用协议同时编码参数数量和结果数量；“固定数量”与“开放数量”不能都用普通容器长度推断。排查 `CALL`、`TAILCALL`、`RETURN` 时应同时记录 frame 的 `base/top`、期望结果模式和调用前后的栈布局。

典型最小样例应覆盖：零返回值、单返回值、多返回值、表达式列表末项展开，以及尾调用直接转发。对应实现说明见 [VM 调用与返回协议](../vm/runtime/overview.md)。

### 4.4 闭包读到旧值或 upvalue 生命周期异常

先区分 upvalue 处于 open 还是 closed：

- open upvalue 指向活动栈槽；
- 栈帧退出时必须关闭并把值迁移到自身存储；
- 多个闭包捕获同一局部变量时必须共享同一 upvalue 身份。

若值只在 GC 后错误，重点检查对象是否进入 root 集，以及关闭 upvalue 时写屏障/追踪关系是否完整；不要只在闭包读取路径加缓存。

### 4.5 table 或 metamethod 行为偏差

先确认原始 table 操作，再检查 metamethod 分派，避免把二者混在一次追踪里。应分别验证键规范化、数组/哈希部分、`__index`/`__newindex` 链和循环保护。行为正确但性能异常时，再检查重复哈希、临时 `Value` 和无意义所有权转换。

### 4.6 内存增长、悬空对象或栈溢出

把资源错误分为三类：

| 类型 | 主要检查项 |
|---|---|
| Lua 栈增长 | `top`、frame 边界、扩容前后的引用有效性 |
| GC 对象增长 | 分配计数、root 集、mark/sweep 状态和回收阈值 |
| C++ 生命周期错误 | 裸指针观察关系、所有者、移动后状态和异常路径清理 |

现代 C++ 教学上，RAII 适合表达资源所有权，但 GC 对象图不能简单替换为 `shared_ptr` 图：Lua GC 的可达性语义、弱引用和循环回收必须由收集器模型表达。诊断文档和源码都应明确区分“拥有”“追踪”和“临时观察”三种关系。

## 5. 错误边界、源码位置与调用栈

编译期以 `Parser::parse() -> std::expected<Chunk, ParseError>` 和 `CodeGenerator::tryGenerate() -> std::expected<Proto*, CodegenError>` 作为公开失败边界。内部递归实现可以抛分类异常快速退出，模块 API 再把它转换为显式结果。

运行期 handler 抛 `RuntimeError`，`tryExecuteProto()` 将其规范化为 `std::expected<ExecResult, RuntimeError>`。`LuaState::pcall()` 在恢复 frame/stack 前关闭被展开帧的 open upvalue；若 `LuaError` 携带任意 Lua `Value`，保护调用保留该对象而不是强制字符串化。

位置数据从 token 的 line/column 进入 `ParseError`；CodeGen 为每条指令写 `Proto::lineInfo`；VM 的 `CallInfo::savedpc` 指向下一条指令，因此 traceback 以 `savedpc - code.data() - 1` 反查当前行。运行期只有行级信息，不能从 PC 恢复已经丢弃的列。

`debug.traceback` 遍历 `CallInfo` 数组，并把逻辑 stack level 映射到物理 frame。函数名是根据调用点和 Proto 推断的辅助信息，稳定证据应以 source、line、frame 顺序和 tail-call 占位为主。

结构化执行证据见 [VM Trace 系统](../vm/trace-system.md)。具体故障的第一落点仍由“最早分歧阶段”决定。
