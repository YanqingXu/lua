# VM Execute — 虚拟机执行

## 1. 这个模块解决什么问题？

解释执行字节码指令，产生运行时行为。

## 2. 在整体执行链路中的位置

```
Load Source → Tokenize → Parse → Compile → Create Function → VM Execute
                                                                  ↑
                                                             (第六阶段)
```

## 3. 核心文件

| 文件 | 作用 |
|------|------|
| `src/vm/vm.cpp` | VM 主执行循环 |
| `src/vm/vm_handlers.cpp` | Handler 注册与路由 |
| `src/vm/vm_handlers/vm_handlers_arith.cpp` | 算术指令 handler |
| `src/vm/vm_handlers/vm_handlers_branch.cpp` | 分支指令 handler |
| `src/vm/vm_handlers/vm_handlers_call.cpp` | 调用/返回 handler |
| `src/vm/vm_handlers/vm_handlers_closure.cpp` | 闭包 handler |
| `src/vm/vm_handlers/vm_handlers_data.cpp` | 数据移动 handler |
| `src/vm/vm_handlers/vm_handlers_loop.cpp` | 循环 handler |
| `src/vm/vm_handlers/vm_handlers_table.cpp` | 表操作 handler |
| `src/vm/vm_handlers/vm_handlers_unary.cpp` | 一元运算 handler |
| `src/vm/vm_dispatch_strategy.cpp` | 分发策略 |
| `src/vm/vm_switch_dispatch.hpp` | Switch 分发实现 |

## 4. VM 执行入口

```cpp
// 执行一个 Lua 函数
VM::execute(L, func);

// 内部调用
VM::executeProto(L, proto, nexeccalls);
  → 设置 CallInfo
  → 准备栈空间
  → 进入主执行循环
```

## 5. 主执行循环伪代码

```cpp
ExecResult executeProto(LuaState* L, Proto* proto, i32 nexeccalls) {
    // 恢复执行状态
    CallInfo& ci = L->getCurrentCallInfo();
    Function* func = ci.func;
    Value* base = &stack[ci.base];
    usize pc = ci.savedpc ? ... : 0;
    
    // 确保栈空间
    stack.ensureSpace(ci.base + proto->maxStackSize);
    
    // 主循环
    const auto code = proto->getInstructionSpan();
    while (pc < code.size()) {
        Instruction inst = code[pc];
        OpCode op = GET_OPCODE(inst);
        pc++;
        
        // 保存 PC 到 CallInfo（用于 hook 和错误恢复）
        ci.savedpc = code.data() + pc;
        
        // 执行指令
        switch (op) {
        case OpCode::MOVE:    R(A) = R(B);                        break;
        case OpCode::LOADK:   R(A) = K(Bx);                       break;
        case OpCode::ADD:     R(A) = RK(B) + RK(C);               break;
        case OpCode::CALL:    /* 函数调用，可能进入新帧 */         break;
        case OpCode::RETURN:  /* 返回，可能退出当前帧 */           break;
        // ... 其他 34 条指令
        }
    }
    
    return ExecResult::Returned;
}
```

## 6. 两种分发策略

| 策略 | 实现 | 特点 |
|------|------|------|
| **Switch Dispatch** | `switch-case` | 编译器优化为跳转表，指令少时高效 |
| **Table Dispatch** | 函数指针表 | `handlerTable[op](ctx, inst)`，可扩展 |

```cpp
// Table Dispatch
HandlerStatus status = VM::runHandler(opContext, inst);
switch (status) {
    case HandlerStatus::Continue:  continue;   // 正常执行下一条
    case HandlerStatus::Reenter:   goto reentry; // 函数调用返回后重入
    case HandlerStatus::Yielded:   return Yielded;
    case HandlerStatus::Returned:  return Returned;
}
```

## 7. 寄存器访问

```cpp
// R(A) — 寄存器访问
Value& R(i32 index) { return base[index]; }

// RK(B) — 寄存器或常量
Value RK(i32 rk) {
    if (ISK(rk)) return constants[INDEXK(rk)];
    else return base[rk];
}

// K(Bx) — 常量访问
Value& K(i32 index) { return constants[index]; }
```

## 8. PC 变化规则

```
默认: pc++ (顺序执行下一条)
JMP:  pc += sBx (相对跳转)
EQ/LT/LE: pc++ 如果条件不满足 (跳过下一条)
TEST: pc++ 如果条件不满足
FORLOOP: 如果继续循环 pc += sBx
CALL: 进入被调用函数后重新设置 pc
RETURN: 退出当前帧，恢复调用者的 pc
```
