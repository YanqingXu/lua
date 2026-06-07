# Control Flow Overview — 控制流

## 1. 控制流语句

| 语句 | VM 指令 | 说明 |
|------|---------|------|
| `if` / `elseif` / `else` | EQ, LT, LE, TEST, JMP | 条件分支 |
| `while ... do` | EQ, TEST, JMP | 前置条件循环 |
| `repeat ... until` | EQ, TEST, JMP | 后置条件循环 |
| `for i = a, b, c` | FORPREP, FORLOOP | 数值 for |
| `for k, v in iterator` | TFORLOOP | 泛型 for |
| `break` | JMP (+ CLOSE) | 跳出循环 |
| `return` | RETURN (+ CLOSE) | 函数返回 |

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/compiler/codegen/codegen_stmt.cpp` | 控制流语句的字节码发射 |
| `src/compiler/codegen/jump_patcher.cpp` | 跳转回填 |
| `src/vm/vm_handlers/vm_handlers_branch.cpp` | 分支指令 (JMP, EQ, LT, LE, TEST, TESTSET) |
| `src/vm/vm_handlers/vm_handlers_loop.cpp` | 循环指令 (FORLOOP, FORPREP, TFORLOOP) |
| `src/vm/vm_loop.cpp` | 循环辅助 |

## 3. 控制流核心概念

- **跳转指令**: JMP (无条件), EQ/LT/LE (条件跳过下一条), TEST (布尔测试)
- **回填**: 跳转目标在编译时未知，后续回填
- **OP_CLOSE**: 跳出作用域时关闭 upvalue
