# Execution Pipeline Overview

## 1. 这个模块解决什么问题？

回答：**一个 `.lua` 文件从输入到执行结束，中间发生了什么？**

## 2. 它在整体架构中的位置

```
Lua Source (.lua 文件或 REPL 输入)
   ↓
[01] Load Source     — 读取源代码
   ↓
[02] Tokenize        — Lexer 将源码转为 Token 流
   ↓
[03] Parse           — Parser 将 Token 流转为 AST
   ↓
[04] Compile         — CodeGen 将 AST 转为 Proto/字节码
   ↓
[05] Create Function — 创建运行时 Closure
   ↓
[06] VM Execute      — VM 解释执行字节码
   ↓
[07] Return Result   — 返回值返回给调用者
```

## 3. 七个阶段详解

| 阶段 | 输入 | 输出 | 核心文件 |
|------|------|------|---------|
| **Load Source** | 文件路径或字符串 | 源代码文本 | `src/io/file_loader.cpp` |
| **Tokenize** | 源代码文本 | Token 流 | `src/compiler/lexer/lexer.cpp` |
| **Parse** | Token 流 | AST (Chunk) | `src/compiler/parser/parser.cpp` |
| **Compile** | AST | Proto (字节码) | `src/compiler/codegen/codegen.cpp` |
| **Create Function** | Proto | Function (Closure) | `src/core/function.cpp` |
| **VM Execute** | Function | 执行副作用 | `src/vm/vm.cpp` |
| **Return Result** | 执行状态 | 返回值 | `src/vm/vm.cpp` |

## 4. 核心文件

| 文件 | 作用 |
|------|------|
| `src/compiler/lexer/lexer.cpp` | 词法分析：源码 → Token |
| `src/compiler/lexer/lexer_cursor.cpp` | 字符游标：管理字符级读取 |
| `src/compiler/parser/parser.cpp` | 语法分析入口 |
| `src/compiler/parser/parser_expr.cpp` | 表达式解析 |
| `src/compiler/parser/parser_stmt.cpp` | 语句解析 |
| `src/compiler/codegen/codegen.cpp` | 字节码生成入口 |
| `src/compiler/codegen/expression_emitter.cpp` | 表达式字节码发射 |
| `src/compiler/codegen/statement_emitter.cpp` | 语句字节码发射 |
| `src/compiler/codegen/function_compiler.cpp` | 函数级编译 |
| `src/compiler/opcode.hpp` | 指令定义 |
| `src/core/function.cpp` | Proto / Function / Closure |
| `src/vm/vm.cpp` | VM 主执行循环 |
| `src/vm/vm_handlers/` | 各指令 handler 实现 |
| `src/vm/state/lua_state.cpp` | LuaState 管理 |

## 5. 各阶段的错误处理

| 阶段 | 错误类型 | 处理方式 |
|------|---------|---------|
| Load Source | 文件不存在 | `FileError` |
| Tokenize | 非法字符/未闭合字符串 | `LexerError` |
| Parse | 语法错误 | `ParseError`（含行号列号） |
| Compile | 语义错误 | `CompileError` |
| VM Execute | 运行时错误 | `RuntimeError`（含栈追溯） |

## 6. 可选编译入口

### 文件执行（lua_app）
```
main() → file_loader → Parser → CodeGen → VM::execute
```

### REPL（交互式）
```
REPL::run() → 逐行读取 → Parser → CodeGen → VM::execute
```

### 字节码工具（lua_bytecode）
```
main() → Parser → CodeGen → BytecodePrinter::print()
```
