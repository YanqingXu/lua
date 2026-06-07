# VM Main Loop — VM 主循环

## 1. 这个模块解决什么问题？

VM 主循环如何逐条执行字节码指令。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/vm/vm.cpp` | `runDispatchBackend()` 主循环 |
| `src/vm/vm_entry.cpp` | `executeProtoUnchecked()` 入口 |
| `src/vm/vm_loop.cpp` | 循环辅助 |

## 3. VM 执行入口

```cpp
// 高层入口
VM::execute(L, func);
// → 设置 CallInfo, 调用 executeProto

// executeProto 入口
ExecResult executeProto(LuaState* L, Proto* proto, i32 nexeccalls) {
    // 1. 深度检查（防止栈溢出）
    if (nexeccalls >= MAX_CALLS)
        throw MemoryError("stack overflow");
    
    // 2. 创建 VMContext
    VMContext context{services, L, proto, nexeccalls};
    
    // 3. 选择分发策略并执行
    DispatchStrategy& strategy = ...;
    return strategy.run(context);
}
```

## 4. 主循环伪代码

```cpp
ExecResult runDispatchBackend(VMContext& context, DispatchBackend backend) {
    LuaState* L = context.state;
    Proto* proto = context.proto;
    
    // ---- 局部执行状态 (与 Lua C 实现一致) ----
    Function* func = nullptr;
    Value* base = nullptr;
    usize pc = 0;
    
reentry: // ⭐ 重入点：从 CallInfo 恢复状态
    {
        CallInfo& ci = L->getCurrentCallInfo();
        Stack& stack = L->getStack();
        
        // 恢复 func
        func = stack[ci.func].asFunction();
        proto = func->getProto();
        
        // 恢复 PC
        pc = ci.savedpc ? (ci.savedpc - code.data()) : 0;
        
        // 确保栈空间
        usize requiredTop = ci.base + proto->getMaxStackSize();
        stack.ensureSpace(requiredTop);
        
        // 刷新 base 指针
        base = &stack[ci.base];
    }
    
    // ---- 主执行循环 ----
    const auto code = proto->getInstructionSpan();
    while (pc < code.size()) {
        usize instructionPc = pc;
        Instruction inst = code[pc];
        OpCode op = GET_OPCODE(inst);
        pc++; // 默认前进
        
        // 保存 PC (用于 hook 和错误恢复)
        ci.savedpc = code.data() + pc;
        
        // Count Hook
        dispatchCountHook(L);
        base = refreshBase(L);
        
        // Line Hook
        dispatchLineHook(L, proto, instructionPc);
        base = refreshBase(L);
        
        // Trace
        emitInstructionTrace(...);
        
        // 执行指令
        switch (op) {
        case OpCode::MOVE:    R(A) = R(B);                    break;
        case OpCode::LOADK:   R(A) = K(Bx);                   break;
        case OpCode::ADD:     arith_op(ADD);                  break;
        case OpCode::CALL:    call_handler(...);              break;
        // ... 其他 34 条指令
        }
        
        // 处理 handler 状态
        switch (status) {
        case Continue:  continue;      // 下一条指令
        case Reenter:   goto reentry;  // 函数返回后重入
        case Yielded:   return Yielded;
        case Returned:  return Returned;
        }
    }
    
    return ExecResult::Returned;
}
```

## 5. PC 如何变化？

```
默认: pc++ (顺序执行)
JMP:      pc += sBx       (相对跳转)
EQ/LT/LE: 满足条件时 pc++ (跳过下一条)
TEST:     满足条件时 pc++ (跳过下一条)
CALL:     进入新帧 → 新帧的 pc = 0
RETURN:   goto reentry → 恢复调用者的 pc
FORLOOP: 继续循环时 pc += sBx
```

## 6. 重入点 (reentry)

```
CALL 指令:
  1. 保存当前状态到 CallInfo (savedpc)
  2. 创建新的 CallInfo (func, base, top)
  3. goto reentry → 在新帧的上下文中重新开始循环

RETURN 指令:
  1. 将返回值放回调用者栈
  2. 弹出当前 CallInfo
  3. goto reentry → 在调用者帧的上下文中恢复执行
```

## 7. 错误中断执行

```
运行时错误 (如除零):
  → throw RuntimeError(...)
  → 被 tryExecuteProto() 捕获
  → 返回 std::unexpected(RuntimeError)
  → 上层可以通过 pcall 捕获
```
