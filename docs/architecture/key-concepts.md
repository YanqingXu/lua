---
status: current
verified_against: src/compiler/; src/vm/; src/runtime/; src/core/; src/gc/; tests/lua/regressions/; tests/lua/integration/
last_checked: 2026-07-11
applies_to: Key Concepts — 核心概念
---

# Key Concepts — 核心概念

## 1. 这个模块解决什么问题？

解释项目中最重要的几个核心概念，避免阅读代码时产生理解偏差。

## 2. Proto vs Closure vs Function

这是最容易被混淆的三个概念：

```
Proto (函数原型)
  ├── 编译时产物，不可变
  ├── 包含：字节码指令、常量表、局部变量信息、子 Proto 列表
  ├── 对应源码中的一个 function 定义体
  └── 可以被多个 Closure 共享

Closure (闭包)
  ├── 运行时对象，可变
  ├── 包装一个 Proto + 一组 Upvalue
  ├── Lua Closure: Proto + captured upvalues
  └── C Closure: C++ function pointer + upvalues

Function (函数对象)
  ├── 继承自 GCObject
  ├── 内部是 Closure + Proto 的变体
  └── 在 Value 中以 Function* 指针形式存储
```

## 3. Upvalue（上值）

```
Upvalue = 闭包捕获的外部局部变量

Open Upvalue:
  - 指向栈上的变量（外部函数还在执行中）
  - v_ 指针指向栈上的 Value

Closed Upvalue:
  - 独立存储（外部函数已返回）
  - closedValue_ 保存变量的副本
  - v_ 指向 closedValue_

多个闭包可以共享同一个 Upvalue：
  function outer()
      local x = 0
      return function() x = x + 1; return x end,
             function() x = x - 1; return x end
  end
  -- 两个闭包共享同一个 x 的 Upvalue
```

## 4. Value（值）

```
Value = 运行时所有数据的统一表示

使用 std::variant 实现，支持 9 种类型：
  ┌──────────┬──────────────────┐
  │ Nil      │ std::monostate   │
  │ Boolean  │ bool             │
  │ Number   │ double           │
  │ String   │ GCString*        │
  │ Table    │ Table*           │
  │ Function │ Function*        │
  │ Userdata │ Userdata*        │
  │ Thread   │ Thread*          │
  │ LightUD  │ void*            │
  └──────────┴──────────────────┘

Value 是值语义：拷贝 Value 是浅拷贝（GC 对象指针拷贝）。
```

## 5. CallInfo（调用帧）

```
每个函数调用都有一个 CallInfo 描述其栈帧：

  ┌─────────────┐ ← top (栈顶)
  │  局部变量3  │
  │  局部变量2  │
  │  局部变量1  │
  ├─────────────┤ ← base (基址)
  │   参数2     │
  │   参数1     │
  │  函数对象   │ ← func
  └─────────────┘

  CallInfo {
    func:     函数在栈中的索引
    base:     参数基址索引
    top:      栈顶索引
    savedpc:  程序计数器（用于恢复执行）
    nresults: 期望的返回值数量
  }
```

## 6. Stack（值栈）

```
Lua 的"寄存器"实际上是栈上的位置：

  Stack:
  ┌────────┐
  │ R(0)   │  局部变量 0
  │ R(1)   │  局部变量 1
  │ R(2)   │  临时值
  │ ...    │
  │ R(n)   │
  └────────┘

  VM 指令中的 A, B, C 参数是寄存器索引
  R(A) = stack[base + A]
  RK(B) = B < 256 ? stack[base + B] : constants[B - 256]
```

## 7. RK 寻址

```
BITRK = 256 (= 0x100)

操作数 < 256:
  → 寄存器索引：直接访问 stack[base + operand]

操作数 >= 256:
  → 常量索引：访问 constants[operand - 256]
  → ISK(x) 判断: (x & BITRK) != 0
  → INDEXK(x): x & ~BITRK
```

## 8. GC 三色标记

```
White → 未访问，可能被回收
Gray  → 已访问但未扫描子引用
Black → 已访问且已扫描所有子引用

标记阶段：
  1. 根集标记为 Gray
  2. 遍历 Gray 对象，标记其子引用为 Gray
  3. 自身标记为 Black
  4. 重复直到没有 Gray 对象

清除阶段：
  遍历所有对象，回收 White 对象
```

## 9. Metatable / Metamethod

```
每个 Table 和 Userdata 可以有元表（metatable）：
  - __index:    读取不存在的 key 时调用
  - __newindex: 写入不存在的 key 时调用
  - __add:      + 运算符
  - __call:     把 table 当函数调用
  - __gc:       GC 回收时的终结器
  - __mode:     弱表标记 ("k", "v", "kv")

基础类型也可以有元表（通过 GlobalState 管理）
```

## 10. Instruction 格式

```
32位指令，三种格式：

iABC:  [OP:6][A:8][C:9][B:9]
  示例: ADD  R(A) = RK(B) + RK(C)

iABx:  [OP:6][A:8][Bx:18]
  示例: LOADK  R(A) = K(Bx)

iAsBx: [OP:6][A:8][sBx:18] (sBx 有符号)
  示例: JMP  pc += sBx
```
