# My Mental Model — 我的心智模型

> 这是个人学习笔记，记录我对这个 Lua 解释器项目的理解。

## 我对这个项目的理解

这个项目是一个用现代 C++ 重写的 Lua 5.1 解释器。它的核心流程是:

```
Lua 源码 → Lexer 拆成 Token → Parser 变成 AST 树
→ CodeGen 生成字节码 → VM 一条条执行 → 产生结果
```

## 最重要的几个概念

1. **Value = variant**: 所有 Lua 值都是 `std::variant`，类型安全
2. **Stack = 寄存器**: VM 的"寄存器"就是栈上的位置 `R(i) = stack[base+i]`
3. **Proto vs Closure**: Proto 是编译产物（字节码），Closure 是运行时对象（Proto + Upvalue）
4. **Upvalue**: 闭包捕获的外部变量，有 Open（指向栈）/ Closed（独立存储）两种状态
5. **RK 寻址**: 操作数 < 256 是寄存器，>= 256 是常量
6. **CallInfo**: 每次函数调用的上下文，func/base/top/savedpc

## 执行流程一句话总结

> Parser 把代码变成 AST，CodeGen 把 AST 变成指令序列，VM 在一个大 `while` 循环里 `switch(opcode)` 逐条执行指令。
