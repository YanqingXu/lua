---
status: current
verified_against: src/core/function.hpp; src/core/function.cpp; src/core/upvalue.hpp; src/core/upvalue.cpp; src/core/thread.hpp; src/vm/vm_call.cpp; src/vm/vm_frame.cpp; src/compiler/codegen/function_compiler.cpp; tests/unit/vm/test_function_call.cpp; tests/unit/compiler/test_function_codegen.cpp; tests/lua/functions/; src/core/; src/runtime/; src/vm/; tests/unit/core/; tests/unit/vm/; tests/lua/runtime/
last_checked: 2026-07-11
applies_to: Function、Proto、Closure、Upvalue、vararg 与尾调用
---

# Function、Proto、Closure、Upvalue、vararg 与尾调用

函数子系统连接 Compiler 产物与 VM 调用协议。`Proto` 描述可共享的编译结果，`Function` 描述一次运行时可调用实体，`Upvalue` 描述闭包共享的词法变量身份。

## 对象关系

```text
Proto (GCObject)
├── code / constants / lineInfo
├── subProtos
├── upvalue descriptors
└── params / vararg / maxStackSize

Function (GCObject)
├── Lua function: Proto* + Vec<Upvalue*>
└── native function: CFunction

Upvalue (GCObject)
├── open: 观察 LuaState 栈槽
└── closed: 值迁移到自身存储
```

同一 Proto 可以创建多个 Function，每个 Function 捕获不同环境。多个 closure 捕获同一活动局部变量时共享同一个 Upvalue，而不是复制 Value。

## Proto 的编译期职责

Proto 保存指令、常量、嵌套 Proto、局部变量调试范围、upvalue 描述、源码和行号映射。它不保存某次调用的 PC、参数或寄存器值；这些属于 `CallInfo` 与 LuaState。

子函数编译时只记录捕获来源：来自父函数局部槽，或转发父函数已有 upvalue。VM 执行 `CLOSURE` 时才把描述解析为具体 Upvalue 对象。

## Function 的两种形态

Lua Function 持有 Proto 与捕获数组，由 VM 解释执行。Native Function 持有 `CFunction`，遵循 LuaState 栈 API：从参数区读取值、向栈顶写结果、返回结果数量。

两种形态最终共用 callable 解析、`__call` fallback、参数规范化和结果归一化。差异只在执行体：Lua function 推入新 frame，native function 直接调用 C++ 函数。

## Upvalue 状态机

```text
capture local
   ↓ find-or-create by stack slot
OPEN ── frame remains active ──→ OPEN
   └── scope/frame exits ───────→ CLOSED
                                  value stored inside Upvalue
```

open upvalue 通过栈索引或受控指针观察活动槽，并登记在 LuaState 的 open-upvalue 集合中。相同槽位的重复捕获必须返回同一对象。

关闭时先复制当前 Value 到 Upvalue 自身存储，再切换观察位置。此后原栈槽可被复用，而所有 closure 仍看到共享值。正常 return、break/跳出 captured scope、tailcall 和异常展开都必须触发等价关闭。

## Vararg

Proto 记录固定形参数量和 vararg 标志。创建 frame 时，固定参数进入寄存器窗口，额外参数形成 vararg 区。`VARARG A B` 把它们加载到 `R(A...)`：B 表达固定数量或开放数量。

vararg 与普通多返回值共享“fixed vs open”协议。只有表达式列表末项能传播开放数量，否则必须收敛为一个值。

## Tail call

当 return 的唯一开放表达式是函数调用时，CodeGen 可以发射 `TAILCALL`。VM 重用当前 frame 或执行等价优化，但仍要：

- 关闭当前函数的 open upvalue；
- 正确移动 callable 与参数；
- 继承调用者期望结果数；
- 兼容 Lua/native/`__call` 三种 callable；
- 为 traceback 累计 tail-call 信息。

因此尾调用优化是调用帧变换，不是简单省略 RETURN。

## GC 与所有权

Proto、Function 和 Upvalue 都是 GC 图节点。原始指针在这里表达“由 GC 跟踪、当前不拥有”，并不意味着生命周期无约束。root 集必须覆盖活动 frame 中的 function、开放 upvalue、全局可达 closure 与编译后尚未挂接的临时对象。

用 `shared_ptr` 替换整张对象图会改变循环、弱引用与 Lua GC 时机，不适合作为等价现代化。更有教学价值的做法是明确 owner/observer/root 三种关系，并让 RAII 管理非 GC 资源与临时注册。

## 验证不变量

- Proto 不包含每次调用状态，Function 不复制 Proto code。
- 相同 open 栈槽只有一个 Upvalue 身份。
- close 后不再访问失效栈槽，多个 closure 仍共享更新。
- Lua/native call 的参数和结果协议一致。
- vararg、multret 和 tailcall 在固定/开放数量上闭合。
- GC 周期中 closure、Proto 与 upvalue 的边保持可追踪。

完整捕获路径见 [Closure 与 Upvalue 全链路](closure-upvalue-walkthrough.md)，VM 帧与调用细节见 [VM Runtime](../../vm/runtime/overview.md)。
