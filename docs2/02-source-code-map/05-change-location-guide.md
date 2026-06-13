---
status: current
verified_against: docs/guides/development.md; docs2/02-source-code-map/00-directory-map.md; src/compiler/; src/core/; src/vm/; src/gc/; src/lib/
last_checked: 2026-06-13
applies_to: Chinese change-location guide for common interpreter tasks
---

# Change Location Guide — 改动位置指南

## 1. 这个模块解决什么问题？

回答：**我想改XXX功能，应该去改哪些文件？**

## 2. 按功能查找

### 我想修改词法规则

**涉及文件：**
- `src/compiler/lexer/lexer.cpp` — 词法扫描核心逻辑
- `src/compiler/lexer/lexer.hpp` — Lexer 类定义
- `src/compiler/parser/token.hpp` — Token 类型定义

**相关测试：**
- `tests/unit/compiler/test_lexer_number.cpp`
- `tests/unit/compiler/test_lexer_lookahead.cpp`

**步骤：**
1. 确认新 Token 类型（如果是新关键字）
2. 在 `token.hpp` 添加 TokenType
3. 在 `lexer.cpp` 添加关键字到哈希表
4. 在 `lexer.cpp` 的 `scanToken()` 或 `identifier()` 中识别
5. 更新 Parser 以处理新 Token
6. 写测试

### 我想新增一个 Lua 语法

**涉及文件：**
- `src/compiler/parser/token.hpp` — 如需要新 Token
- `src/compiler/ast.hpp` — 定义新的 AST 节点
- `src/compiler/parser/parser.cpp` — 解析入口
- `src/compiler/parser/parser_stmt.cpp` — 如果是新语句
- `src/compiler/parser/parser_expr.cpp` — 如果是新表达式
- `src/compiler/codegen/codegen_stmt.cpp` 或 `expression_emitter.cpp` — 字节码发射
- `src/compiler/opcode.hpp` — 如需要新指令
- `src/vm/vm.cpp` 或对应的 `vm_handlers/` — VM 执行
- `src/compiler/ast_visitor.hpp` — 更新访问者

**步骤：**
1. 定义 AST 节点
2. Parser 解析新语法
3. CodeGen 生成对应字节码（可能需新指令）
4. VM 执行新指令
5. 更新 ast_visitor
6. 写测试

### 我想修改函数调用行为

**涉及文件：**
- `src/vm/vm_call.cpp` — 函数调用辅助
- `src/vm/vm_handlers/vm_handlers_call.cpp` — CALL/TAILCALL/RETURN handler
- `src/vm/state/call_info.hpp` — CallInfo 结构
- `src/core/function.hpp/cpp` — Closure 定义

**关键函数：**
- `VM::call()` — 通用调用入口
- `execOpCall()` — CALL 指令实现
- `execOpReturn()` — RETURN 指令实现
- `execOpTailCall()` — 尾调用实现

### 我想修改 Table 存取行为

**涉及文件：**
- `src/core/table.cpp` — Table 实现
- `src/vm/vm_table.cpp` — VM 表操作
- `src/vm/vm_handlers/vm_handlers_table.cpp` — GETTABLE/SETTABLE handler
- `src/core/metatable.cpp` — 元表查找

**关键函数：**
- `Table::get(key)` / `Table::set(key, value)`
- `execOpGetTable()` / `execOpSetTable()`

### 我想修改 GC 行为

**涉及文件：**
- `src/gc/garbage_collector.cpp` — GC 主控制
- `src/gc/gc_mark.cpp` — 标记阶段
- `src/gc/gc_sweep.cpp` — 清除阶段
- `src/gc/gc_finalize.cpp` — 终结器
- `src/gc/gc_weak.cpp` — 弱表
- `src/gc/gc_strategy.cpp` — 策略
- `src/core/gc_object.hpp/cpp` — GCObject 基类

### 我想新增一个标准库函数

**涉及文件：**
- 对应库文件（如 `src/lib/baselib.cpp`）
- `src/lib/lib_catalog.cpp` — 如果添加新库

**步骤：**
1. 实现 C++ 函数
2. 在库注册函数中注册
3. 写测试

### 我想修改 REPL 行为

**涉及文件：**
- `src/repl/repl_comp.cpp` — Tab 补全
- `src/repl/repl_ctx.cpp` — REPL 上下文
- `src/repl/repl_hist.cpp` — 历史
- `src/repl/repl_meta.cpp` — 元命令
- `src/repl/repl_prompt.cpp` — 提示符

## 3. 按错误类型查找

| 错误类型 | 优先查看 |
|---------|---------|
| 词法错误 (LexerError) | `src/compiler/lexer/lexer.cpp` |
| 语法错误 (ParseError) | `src/compiler/parser/parser*.cpp` |
| 编译错误 (CompileError) | `src/compiler/codegen/*.cpp` |
| 运行时错误 (RuntimeError) | `src/vm/vm.cpp`, `src/vm/vm_handlers/*.cpp` |
| 内存错误 (MemoryError) | `src/gc/*.cpp` |
