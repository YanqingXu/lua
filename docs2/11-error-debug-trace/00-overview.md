# Error & Debug Overview

## 1. 错误类型

| 错误类型 | 对应类 | 触发阶段 |
|---------|--------|---------|
| **词法错误** | LexerError (token 中) | Lexer |
| **语法错误** | ParseError | Parser |
| **编译错误** | CompileError | CodeGen |
| **运行时错误** | RuntimeError | VM |
| **内存错误** | MemoryError | GC / Stack |

## 2. 错误处理链

```
Lua 源码
  ↓
Lexer → LexerError (非法字符/未闭合字符串)
  ↓
Parser → ParseError (语法错误, 含行号列号)
  ↓
CodeGen → CompileError (语义错误)
  ↓
VM → RuntimeError (运行时错误: 除零/类型错误/栈溢出)
  ↓
pcall 捕获 → 返回 false, error_msg
```

## 3. 核心文件

| 文件 | 作用 |
|------|------|
| `src/common/lua_error.hpp` | 错误类型定义 |
| `src/compiler/parser/parser.cpp` | 语法错误报告 |
| `src/vm/vm.cpp` | 运行时错误 |
| `src/vm/vm_trace.cpp` | 执行追踪 |
| `src/debug/json_trace_sink.cpp` | JSON trace 输出 |
| `src/debug/value_serializer.cpp` | 值序列化 |
