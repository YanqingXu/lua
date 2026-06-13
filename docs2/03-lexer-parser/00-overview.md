---
status: current
verified_against: docs/compiler/lexer.md; docs/compiler/parser.md; docs/compiler/bytecode-generation.md; src/compiler/lexer/; src/compiler/parser/; src/compiler/ast.hpp
last_checked: 2026-06-13
applies_to: Chinese lexer and parser overview
---

# Lexer & Parser Overview

## 1. 这个模块解决什么问题？

回答：**Lua 源码如何被读成语法结构（AST）？**

## 2. 在整体执行链路中的位置

```
Lua Source → Lexer → Parser → Compiler → VM → Runtime
               ↑        ↑
             词法     语法
```

## 3. 核心文件

| 文件 | 作用 |
|------|------|
| `src/compiler/lexer/lexer.hpp/cpp` | 词法分析器 |
| `src/compiler/lexer/lexer_cursor.hpp/cpp` | 字符游标 |
| `src/compiler/parser/parser.hpp/cpp` | 语法分析器入口 |
| `src/compiler/parser/parser_expr.cpp` | 表达式解析 |
| `src/compiler/parser/parser_stmt.cpp` | 语句解析 |
| `src/compiler/parser/parser_func.cpp` | 函数定义解析 |
| `src/compiler/parser/parser_primary.cpp` | 基本表达式解析 |
| `src/compiler/parser/parser_table.cpp` | 表构造器解析 |
| `src/compiler/parser/token.hpp` | Token 定义 |
| `src/compiler/ast.hpp` | AST 节点定义 |

## 4. 数据流

```
源代码字符串
  ↓
Lexer::nextToken() → Token 流
  ↓
Parser::parse() → AST (Chunk)
  ↓
CodeGenerator::generate() → Proto
```

## 5. Lexer 核心设计

- **单遍扫描**：逐字符读取，一次生成一个 Token
- **LL(1) 前瞻**：`peekToken()` 预读一个 Token 不消费
- **哈希表关键字**：O(1) 关键字识别
- **状态快照**：支持回溯（长字符串检测失败恢复）

## 6. Parser 核心设计

- **递归下降**：每个语法规则一个函数
- **Pratt Parser**：表达式按优先级解析
- **FailFast vs StatementBoundary**：两种错误恢复策略
- **智能指针管理 AST**：`unique_ptr` 自动释放
