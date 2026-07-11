# VM Trace — VM 执行追踪

## 1. 这个模块解决什么问题？

VM 的执行追踪系统，用于调试和理解程序行为。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/vm/vm_trace.cpp` | Trace 实现 |
| `src/debug/json_trace_sink.cpp` | JSON 格式 Trace 输出 |
| `src/debug/value_serializer.cpp` | Value 序列化 |

## 3. Trace 系统架构

```
ITraceSink (接口)
  ├── JsonTraceSink → JSON 文件输出 (bin/out.jsonl)
  └── (可扩展: 实时显示、远程发送)

VM::setTraceSink(sink)
VM::getTraceSink()
```

## 4. 开启 Trace

```bash
# 命令行
lua_app --trace script.lua

# 输出到 bin/out.jsonl
# 可在 trace_viewer.html 中可视化查看
```

## 5. Trace 事件类型

| 事件 | 内容 | 频率 |
|------|------|------|
| `instruction` | 每条指令的执行信息 | 每条指令 |
| `call` | 函数调用进入 | 每次调用 |
| `return` | 函数调用返回 | 每次返回 |
| `error` | 运行时错误 | 每次错误 |

## 6. Instruction Trace 内容

```json
{
    "event": "instruction",
    "pc": 5,
    "opcode": "ADD",
    "A": 2, "B": 0, "C": 1,
    "registers_before": {
        "R0": 2.0,
        "R1": 3.0,
        "R2": null
    },
    "registers_after": {
        "R0": 2.0,
        "R1": 3.0,
        "R2": 5.0
    },
    "callDepth": 1,
    "sourceLine": 4
}
```

## 7. Diff Mode

```
Trace Diff Mode: 比较指令执行前后的寄存器变化

开启: VM::setTraceDiffEnabled(true)

输出:
  changedRegisters: { "R2": { "before": null, "after": 5.0 } }
```

## 8. Trace 性能影响

```
Trace 会显著降低执行速度 (5-50x)
仅在调试时使用

建议:
  - 小脚本可以全量 trace
  - 大脚本选择性地 trace 关键函数
  - 生产环境关闭 trace
```
