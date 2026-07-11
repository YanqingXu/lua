# VM Trace Mode

> 详见 [VM Trace](../vm/runtime/vm-trace.md)

## 快速使用

```bash
# 开启 trace
lua_app --trace script.lua

# 输出: bin/out.jsonl
# 可视化: 浏览器打开 bin/trace_viewer.html
```

## Trace 事件

- `instruction`: 每条指令执行前后的寄存器状态
- `call`: 函数调用
- `return`: 函数返回
- `error`: 运行时错误
