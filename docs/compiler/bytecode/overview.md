---
status: current
verified_against: docs/compiler/bytecode-generation.md; docs/compiler/codegen-responsibility-map.md; docs/vm/instruction-set.md; src/compiler/opcode.hpp; src/compiler/codegen/
last_checked: 2026-06-13
applies_to: Chinese bytecode compiler overview
---

# Bytecode Compiler Overview — 字节码编译概览

## 1. 这个模块解决什么问题？

回答：**Parser 之后如何将 AST 转换为可执行的 Proto（字节码指令）？**

## 2. 在整体执行链路中的位置

```
Lua Source → Lexer → Parser → Compiler → VM → Runtime
                                 ↑
                            字节码编译
```

## 3. 核心文件

| 文件 | 作用 |
|------|------|
| `src/compiler/opcode.hpp` | 38 条指令定义、格式、编解码 |
| `src/compiler/opcode.cpp` | 操作码辅助函数 |
| `src/compiler/codegen/codegen.cpp` | 编译入口 |
| `src/compiler/codegen/expression_emitter.cpp` | 表达式 → 字节码 |
| `src/compiler/codegen/statement_emitter.cpp` | 语句 → 字节码 |
| `src/compiler/codegen/function_compiler.cpp` | 函数级编译 |
| `src/compiler/codegen/scope_manager.cpp` | 作用域/寄存器管理 |
| `src/compiler/codegen/jump_patcher.cpp` | 跳转回填 |
| `src/compiler/bytecode_printer.cpp` | 字节码打印/反汇编 |

## 4. 编译流程

```
AST (Chunk)
  ↓
CodeGenerator::generate()
  ↓
FunctionCompiler::compileMain()
  ├── compileStatements()  → 每条语句 → 指令序列
  ├── manageConstants()    → 常量去重
  ├── allocateRegisters()  → 局部变量 → 寄存器
  ├── patchJumps()         → 回填跳转目标
  └── 输出 Proto
```

## 5. Proto 输出

```
Proto {
    code: Vec<Instruction>      // 字节码序列
    constants: Vec<Value>       // 常量表
    locals: Vec<LocalVarInfo>   // 局部变量信息（名称、起止PC）
    subProtos: Vec<Proto*>      // 子函数原型
    upvalues: Vec<UpvalueDesc>  // Upvalue 描述
    maxStackSize: usize          // 最大寄存器数
    numParams: i32              // 参数个数
    isVararg: bool              // 是否可变参数
    source: Str                 // 源文件名（调试用）
    lineInfo: Vec<i32>          // 行号信息（调试用）
}
```

## 6. CodegenOps 收口

项目中 `CodegenOps` 负责封装低层指令发射：
- `emitABC(OpCode, A, B, C)` → 发射 ABC 格式指令
- `emitABx(OpCode, A, Bx)` → 发射 ABx 格式指令
- `emitAsBx(OpCode, A, sBx)` → 发射 AsBx 格式指令
- `emitJump(OpCode)` → 发射跳转指令，返回 PC 用于回填
- `patchJump(pc, target)` → 回填跳转目标
