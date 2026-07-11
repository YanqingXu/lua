---
status: current
verified_against: docs/runtime/functions/closure-upvalue-walkthrough.md; docs/compiler/bytecode-generation.md; src/core/function.hpp; src/core/upvalue.hpp; src/vm/vm_call.cpp; src/vm/vm_frame.cpp
last_checked: 2026-06-13
applies_to: Chinese function, closure, and upvalue overview
---

# Function & Closure Overview

## 1. 这个模块解决什么问题？

函数、闭包和 Upvalue 是解释器中最难也是最值得理解清楚的部分。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/core/function.hpp/cpp` | Proto, Closure, Function (1096行) |
| `src/core/upvalue.hpp/cpp` | Upvalue Open/Closed 管理 (419行) |
| `src/compiler/codegen/function_compiler.cpp` | 函数编译 |
| `src/vm/vm_handlers/vm_handlers_closure.cpp` | CLOSURE/CLOSE handler |

## 3. 三件套关系

```
Proto (编译时)
  ├── 字节码 + 常量表 + 局部变量信息
  ├── 不可变
  └── 可被多个 Closure 共享

Closure (运行时)
  ├── 包装 Proto + Upvalue 列表
  ├── 可变 (upvalue 值可变)
  └── Lua Closure vs C Closure

Function (GC 对象)
  ├── 包装 Closure (或 C function pointer)
  ├── 包含环境表 (env)
  └── Value 中以 Function* 存储
```

## 4. 关键概念

- **Proto**: 编译产物，类似 C 的 `.o` 文件
- **Closure**: Proto + 捕获的变量，类似 C++ 的 lambda
- **Upvalue**: 被闭包捕获的外部局部变量
- **Open Upvalue**: 指向栈上的还在活跃的变量
- **Closed Upvalue**: 变量被复制到堆上（外部函数已返回）
