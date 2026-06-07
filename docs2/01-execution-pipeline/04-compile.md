# Compile — 字节码编译

## 1. 这个模块解决什么问题？

将 AST 转换为 Proto（函数原型 + 字节码指令）。

## 2. 在整体执行链路中的位置

```
Load Source → Tokenize → Parse → Compile → VM Execute
                                     ↑
                                (第四阶段)
```

## 3. 核心文件

| 文件 | 作用 |
|------|------|
| `src/compiler/codegen/codegen.cpp` | 编译入口和顶层调度 |
| `src/compiler/codegen/codegen_stmt.cpp` | 语句编译 |
| `src/compiler/codegen/expression_emitter.cpp` | 表达式编译 |
| `src/compiler/codegen/statement_emitter.cpp` | 语句发射器 |
| `src/compiler/codegen/function_compiler.cpp` | 函数级编译 |
| `src/compiler/codegen/scope_manager.cpp` | 作用域管理 |
| `src/compiler/codegen/jump_patcher.cpp` | 跳转回填 |
| `src/compiler/codegen/codegen_binding.cpp` | 符号绑定 |
| `src/compiler/codegen/name_binder.cpp` | 名称绑定 |

## 4. 编译流程

```
CodeGenerator::generate(chunk)
  ↓
FunctionCompiler::compileMain(chunk)
  ├── 分配寄存器（局部变量 → 寄存器索引）
  ├── 翻译每条语句 → 字节码指令序列
  ├── 管理常量表（去重）
  ├── 回填跳转指令
  └── 输出 Proto
```

## 5. Proto 结构

```
Proto {
    Vec<Instruction> code;          // 字节码指令列表
    Vec<Value> constants;           // 常量表 (nil, bool, number, string)
    Vec<LocalVarInfo> locals;       // 局部变量信息
    Vec<Proto*> subProtos;          // 子函数原型
    Vec<UpvalueDesc> upvalues;      // Upvalue 描述
    usize maxStackSize;             // 最大栈使用量
    i32 numParams;                  // 参数数量
    bool isVararg;                  // 是否可变参数
}
```

## 6. 寄存器分配

```
每个活动的局部变量占用一个寄存器。
临时表达式值也占用寄存器。

寄存器分配策略：
  allocReg() → 分配新寄存器
  freeReg(reg) → 释放寄存器
  getTopReg() → 当前栈顶寄存器

示例：
  local a = 1    → R(0) = 1
  local b = 2    → R(1) = 2
  local c = a+b  → R(2) = ADD R(0) R(1)
```

## 7. 常量池管理

```
addConstant(value):
  1. 检查常量表中是否已存在（去重）
  2. 如果不存在，添加
  3. 返回索引

常量类型支持：
  - nil, boolean, number, string

RK 编码：
  - 如果索引 < 256：直接作为操作数
  - 如果索引 >= 256：使用 RKASK(x) = x | BITRK
```

## 8. 跳转回填

```
编译控制流时的标准模式：

1. 遇到 if/while/for → emitJump(JMP) 或 emitJump(EQ)
   → 记录跳转位置（当前 PC）
   → 留空跳转目标

2. 编译完分支体 → patchJump(pc, 当前PC)
   → 回填跳转目标

示例（if-then-else）：
  EQ R(0) false → (跳过 then 到 else)  ← 需要回填
  ... then body ...
  JMP → (跳过 else 到 end)              ← 需要回填
  ... else body ...
  (end)
```

## 9. 各语句编译要点

| 语句 | 关键编译逻辑 |
|------|------------|
| **赋值** | 先编译 RHS 所有表达式，再写入 LHS 目标 |
| **local** | 分配寄存器，先求值 RHS，后引入新 local |
| **if** | EQ/TEST 条件跳转 + JMP 分支跳转 |
| **while** | JMP 回到条件检测 |
| **for num** | FORPREP 初始化 + FORLOOP 循环 |
| **for in** | TFORLOOP 泛型迭代 |
| **function** | 编译子函数为 Proto，生成 CLOSURE 指令 |
| **return** | RETURN 指令 |
