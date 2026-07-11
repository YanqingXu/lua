# Instruction Dispatch — 指令分发

## 1. 这个模块解决什么问题？

VM 如何根据操作码选择执行对应的指令实现。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/vm/vm_dispatch_strategy.hpp/cpp` | 分发策略接口 |
| `src/vm/vm_switch_dispatch.hpp` | Switch 分发 |
| `src/vm/vm_handlers.hpp/cpp` | Table 分发 (handler 注册) |
| `src/vm/vm.cpp` | runDispatchBackend 中的分发逻辑 |

## 3. 两种分发策略

### Switch Dispatch (默认)

```cpp
ExecResult SwitchDispatch::run(VMContext& context) {
    return runDispatchBackend(context, DispatchBackend::Switch);
}

// 内部:
switch (op) {
    case OpCode::MOVE:   execOpMove(...);   break;
    case OpCode::LOADK:  execOpLoadK(...);  break;
    case OpCode::ADD:    execOpAdd(...);    break;
    // ... 38 条指令
    default: throw RuntimeError("unsupported opcode");
}
```

编译器可以根据目标平台和优化级别把 switch-case 降低为跳转表或比较分支；源码不依赖某一种机器码形态。

### Table Dispatch

```cpp
ExecResult TableDispatch::run(VMContext& context) {
    return runDispatchBackend(context, DispatchBackend::Table);
}

// 内部:
HandlerStatus status = VM::runHandler(opContext, inst);
switch (status) {
    case HandlerStatus::Continue:  continue;
    case HandlerStatus::Reenter:   goto reentry;
    case HandlerStatus::Yielded:   return ExecResult::Yielded;
    case HandlerStatus::Returned:  return ExecResult::Returned;
}
```

Table dispatch 使用函数指针表，每条指令对应一个 handler 函数。

## 4. Handler 状态

```cpp
enum class HandlerStatus : u8 {
    Continue,  // 继续执行下一条指令
    Reenter,   // 重入主循环 (函数调用后)
    Yielded,   // 协程挂起
    Returned   // 函数返回
};
```

## 5. OpExecutionContext

```cpp
struct OpExecutionContext {
    RuntimeServices& services;
    LuaState* L;
    Function* func;
    Proto* proto;
    Value* base;
    usize& pc;
    usize instructionPc;
    i32& nexeccalls;
    
    // 辅助方法:
    Value& R(i32 idx) { return base[idx]; }
    Value RK(i32 rk) { /* ISK 判断 */ }
    Value& K(i32 idx) { return proto->getConstants()[idx]; }
};
```

## 6. Handler 注册 (Table Dispatch)

```cpp
// vm_handlers.cpp
using OpHandler = HandlerStatus (*)(OpExecutionContext&, Instruction);

static OpHandler handlerTable[38] = {};

void registerHandler(OpCode op, OpHandler handler) {
    handlerTable[static_cast<usize>(op)] = handler;
}

// 初始化时注册所有 handler:
registerHandler(OpCode::MOVE, VM::detail::execOpMove);
registerHandler(OpCode::ADD,  VM::detail::execOpAdd);
// ... 38 条
```

## 7. 选择分发策略

```cpp
DispatchStrategy& strategy = 
    services.dispatchStrategy != nullptr
    ? *services.dispatchStrategy
    : defaultDispatchStrategy();  // SwitchDispatch
```
