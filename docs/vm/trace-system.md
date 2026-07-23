---
status: current
verified_against: src/debug/trace_types.hpp; src/debug/trace_sink.hpp; src/debug/json_trace_sink.cpp; src/debug/value_serializer.cpp; src/vm/vm_trace.cpp; src/vm/vm.cpp; src/main.cpp; src/app/app_options.cpp; tests/unit/vm/test_vm_trace_debug.cpp; src/compiler/opcode.hpp; src/vm/; src/vm/vm_handlers/; tests/unit/vm/; tests/unit/vm/opcode_coverage_matrix.md
last_checked: 2026-07-23
applies_to: current JSONL VM trace system
---

# VM Trace 系统

VM trace 系统从字节码解释器记录执行事件并以 JSONL 格式写入。它默认关闭，通过安装 `ITraceSink` 来启用。

应用入口提供 full trace 与 diff trace 两种 JSONL 模式：前者记录完整寄存器状态，后者主要记录变化字段。具体参数由可执行程序帮助维护。

尚无提交的 HTML trace 查看器。早期的查看器设想是历史性的；当前已实现的输出面是 JSONL。

## 组件

| 组件 | 文件 | 职责 |
|---|---|---|
| `TraceEventKind` / `TraceEvent` | `src/debug/trace_types.hpp` | 共享事件形态 |
| `ITraceSink` | `src/debug/trace_sink.hpp` | Trace sink 接口 |
| `NullTraceSink` | `src/debug/trace_sink.hpp` | 空操作 sink |
| `JsonTraceSink` | `src/debug/json_trace_sink.*` | JSONL 写入器 |
| 值序列化 | `src/debug/value_serializer.*` | 将 `Value` 和寄存器转换为 JSON 兼容文本 |
| VM trace hook | `src/vm/vm_trace.cpp` | 构建和发射 instruction/call/return 事件 |
| CLI 装配 | `src/app/app_options.cpp`, `src/main.cpp` | 解析 `--trace <file>` / `--trace-diff <file>` 并安装 sink |

## 事件类型

`TraceEventKind` 当前定义：

- `Instruction`
- `Call`
- `Return`
- `Error`

`JsonTraceSink` 实现了 `onError`，但当前 VM 路径尚未发射运行时错误 trace 事件。将 error 事件视为保留的 schema 支持。

`VM Trace Debug` 为一个微型脚本包含两个精确 JSONL golden 测试：

- `Trace JSONL Plain Golden` 锁定带完整 `registers` 快照的普通 trace 序列。
- `Trace JSONL Diff Golden` 锁定带 `changedRegisters` 且无完整 `registers` 快照的 `--trace-diff` 序列。

Golden 脚本有意仅使用数字和局部变量，使输出不包含指针形式的 function、table、userdata 或 thread 值。

## Instruction 事件

Instruction 事件包含解码后的操作数、源码位置和当前函数标签。Plain `--trace` 在 VM 可提供时还包含完整帧寄存器快照：

```json
{"seq":0,"kind":"instruction","funcName":"examples/hello.lua","pc":0,"op":"LOADK","a":0,"b":0,"c":0,"bx":0,"sbx":0,"line":1,"source":"examples/hello.lua","callDepth":1,"registers":[]}
```

字段：

| 字段 | 含义 |
|---|---|
| `seq` | 单调递增的事件序列号 |
| `kind` | `"instruction"` |
| `funcName` | 可读的当前函数标签，通常为 `source` 或 `source:linedefined` |
| `pc` | 当前 `Proto` 内的程序计数器 |
| `op` | 操作码名称 |
| `a`、`b`、`c`、`bx`、`sbx` | 解码后的指令操作数 |
| `line` | `Proto` 行信息的源码行号 |
| `source` | 活跃 `Proto` 的源码名称 |
| `callDepth` | 当前逻辑 VM 调用深度 |
| `registers` | Plain trace 模式下的序列化帧寄存器快照 |
| `changedRegisters` | Diff trace 模式下的寄存器变更 |

## Call 和 Return 事件

Call 事件在可见 VM 调用点周围发射：

```json
{"seq":3,"kind":"call","funcName":"examples/hello.lua:1","source":"examples/hello.lua","line":1,"callDepth":2}
```

对于 call 事件，`funcName` 在已知时命名被调用者。对于 instruction 和 return 事件，它命名活跃或正在返回的 `Proto`。

Return 事件包含相同的函数标签和源码位置：

```json
{"seq":8,"kind":"return","funcName":"examples/hello.lua:1","source":"examples/hello.lua","line":2,"callDepth":1}
```

## 寄存器快照

Instruction 事件可包含 `registers`。每个元素包含：

| 字段 | 含义 |
|---|---|
| `slot` | 相对于当前帧的寄存器索引 |
| `name` | debug info 可解析时的局部变量名 |
| `type` | Lua 值类型字符串 |
| `value` | 序列化的值 |

序列化器是仅观察的：它读取 VM 值，不修改栈状态。

## Trace Diff 模式

`--trace-diff <file>` 在每条指令前捕获帧，执行处理器，然后仅写入值发生变化的槽位。Diff instruction 事件省略完整 `registers` 快照并添加 `changedRegisters`：

```json
{"seq":1,"kind":"instruction","funcName":"examples/math.lua","pc":1,"op":"ADD","a":0,"b":0,"c":1,"bx":1,"sbx":-131070,"line":2,"source":"examples/math.lua","callDepth":1,"changedRegisters":[{"slot":0,"name":"x","old":1,"new":3,"oldType":"number","newType":"number"}]}
```

每个 changed register 条目包含：

| 字段 | 含义 |
|---|---|
| `slot` | 相对于捕获帧的寄存器索引 |
| `name` | debug info 可解析时的局部变量名，否则为 `null` |
| `old` | 指令前的序列化值 |
| `new` | 指令后的序列化值 |
| `oldType` | 指令前的 Lua 值类型 |
| `newType` | 指令后的 Lua 值类型 |

## 控制流

```text
main.cpp
  -> 解析 --trace <file> 或 --trace-diff <file>
  -> 创建 JsonTraceSink
  -> 可选启用 VM::setTraceDiffEnabled(true)
  -> VM::setTraceSink(...)
  -> VM::executeProto(...)
  -> vm_trace.cpp 发射事件
  -> JsonTraceSink 写入 JSONL
```

现代 VM 路径通过 `VM::setTraceSink(RuntimeServices&, ...)` 与
`VM::setTraceDiffEnabled(RuntimeServices&, ...)` 修改当前 `GlobalState::TraceRuntime`；
不接收 services 的重载只操作兼容 singleton。`VM::setTraceSink(services, nullptr)` 禁用
指定上下文的 trace 输出。

## Schema 与实现边界

- Error 事件在 schema 中有表示但尚未由 VM 错误路径发射。
- C 函数调用事件使用通用 `C function` 标签，因为 C 闭包当前不携带调试名称。
- Trace sink、diff 开关与事件序列号属于各 `GlobalState` 的 `TraceRuntime`；不同
  `EngineContext` 互不共享。无 services 的旧重载仅保留进程 singleton 兼容行为。

这些是读取现有 trace 时必须遵守的事实边界；消费者不能假设每种 schema event 都一定出现，也不能把通用 C function 标签当作稳定函数身份。

## 验证

`tests/unit/vm/test_vm_trace_debug.cpp` 锁定 plain/diff JSONL schema、寄存器变化、函数名与事件顺序。测试断言结构化字段，不依赖输出文件位置或命令行文本。
