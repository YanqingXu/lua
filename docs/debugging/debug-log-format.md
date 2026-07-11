# Debug Log Format

## JSONL Trace 格式

```jsonl
{"event":"instruction","pc":0,"opcode":"LOADK","A":0,"Bx":0,"registers":{"R0":1.0},"line":1}
{"event":"instruction","pc":1,"opcode":"GETGLOBAL","A":1,"Bx":1,"registers":{"R0":1.0,"R1":"<function>"},"line":1}
{"event":"instruction","pc":2,"opcode":"CALL","A":1,"B":1,"C":1,"registers":{...},"line":1}
{"event":"call","func":"print","depth":1}
{"event":"return","depth":1}
```

## Trace Viewer

`bin/trace_viewer.html` 可以加载 `out.jsonl` 并提供:
- 指令时间线
- 寄存器状态变化
- 函数调用图
- 按行号/操作码过滤
