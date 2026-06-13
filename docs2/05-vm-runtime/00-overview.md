---
status: current
verified_against: docs/vm/instruction-set.md; docs/vm/trace-system.md; docs/architecture/runtime-services.md; src/vm/; src/runtime/runtime_services.hpp
last_checked: 2026-06-13
applies_to: Chinese VM runtime overview
---

# VM Runtime Overview — 虚拟机运行概览

## 1. 这个模块解决什么问题？

回答：**字节码是如何被真正执行的？** VM 是整个解释器的心脏。

## 2. 在整体执行链路中的位置

```
Lua Source → Lexer → Parser → Compiler → VM → Runtime
                                           ↑
                                      虚拟机执行
```

## 3. 核心文件

| 文件 | 作用 |
|------|------|
| `src/vm/vm.cpp` | VM 入口 + 主循环 |
| `src/vm/vm_entry.cpp` | executeProto 入口 |
| `src/vm/vm_loop.cpp` | 主循环实现 |
| `src/vm/vm_frame.cpp` | 调用帧管理 |
| `src/vm/vm_call.cpp` | 函数调用 |
| `src/vm/vm_arith.cpp` | 算术运算 |
| `src/vm/vm_table.cpp` | 表操作 |
| `src/vm/vm_trace.cpp` | 追踪/调试 |
| `src/vm/vm_handlers/` | 38条指令的具体实现 |
| `src/vm/state/lua_state.cpp` | LuaState |
| `src/vm/state/global_state.cpp` | GlobalState |
| `src/vm/state/stack.cpp` | Value 栈 |
| `src/vm/state/call_info.hpp` | 调用帧 |

## 4. VM 架构

```
VM::execute(L, func)
  ↓
VM::executeProto(L, proto, nexeccalls)
  ↓
executeProtoUnchecked(services, L, proto, nexeccalls)
  ↓
DispatchStrategy::run(context)
  ├── SwitchDispatch  (switch-case)
  └── TableDispatch   (函数指针表)
  ↓
runDispatchBackend() — 主执行循环
  while (pc < code.size()):
    fetch instruction
    switch/dispatch opcode
    handle hooks (count, line)
    handle trace
```

## 5. VM 三大核心状态

```
LuaState (线程执行环境)
  ├── Stack (值栈)
  ├── Vec<CallInfo> (调用栈)
  ├── Upvalue 链表
  └── ThreadStatus

GlobalState (全局资源 — 单例)
  ├── StringPool
  ├── GarbageCollector
  ├── Registry (C 代码存储)
  └── Metatables (基础类型元表)

CallInfo (单个调用帧)
  ├── func  (函数在栈中的索引)
  ├── base  (参数基址)
  ├── top   (栈顶)
  ├── savedpc (恢复用的 PC)
  └── nresults (期望返回值数)
```
