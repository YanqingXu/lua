# Function Call — 函数调用机制

## 1. 这个模块解决什么问题？

VM 如何执行 `CALL` 指令，包括 Lua 函数调用、C 函数调用和尾调用。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/vm/vm_call.cpp` | 函数调用实现 |
| `src/vm/vm_handlers/vm_handlers_call.cpp` | CALL/TAILCALL/RETURN handler |

## 3. CALL 指令

```
CALL A B C

A: 函数所在的寄存器索引
B: 参数数量 (不含函数本身)
C: 期望的返回值数量 - 1

语义: 调用 R(A)(R(A+1), ..., R(A+B-1))
     结果放在 R(A), ..., R(A+C-1) (取决于实际返回值)
```

## 4. Lua 函数调用流程

```cpp
void execOpCall(OpExecutionContext& ctx, Instruction inst) {
    i32 a = GETARG_A(inst);
    i32 b = GETARG_B(inst);
    i32 c = GETARG_C(inst);
    i32 nparams = b - 1;       // 参数个数
    i32 nresults = c - 1;      // 期望返回值数
    
    Value& funcVal = R(a);
    
    if (funcVal.isFunction()) {
        Function* func = funcVal.asFunction();
        
        if (func->isLua()) {
            // Lua 函数调用
            Proto* proto = func->getProto();
            
            // 1. 调整参数数量
            adjustParams(nparams, proto->numParams, proto->isVararg);
            
            // 2. 创建新 CallInfo
            CallInfo newCI;
            newCI.func = a;
            newCI.base = a + 1;
            newCI.top = a + 1 + proto->maxStackSize;
            newCI.nresults = nresults;
            newCI.savedpc = nullptr;
            L->pushCallInfo(newCI);
            
            // 3. 重入主循环
            ctx.status = HandlerStatus::Reenter;
            
        } else {
            // C 函数调用
            callCFunction(func, a, nparams, nresults);
            ctx.status = HandlerStatus::Continue;
        }
    }
}
```

## 5. C 函数调用流程

```cpp
void callCFunction(LuaState* L, Function* func, i32 funcIdx, i32 nparams, i32 nresults) {
        lua_CFunction cfunc = func->getCFunction();
    
    // C 函数直接操作栈
    // 参数已经在栈上 (R(funcIdx+1) ...)
    // C 函数通过 lua_push* 返回结果
    
    i32 actualResults = cfunc(L);
    
    // 根据 nresults 调整返回值数量
    adjustResults(funcIdx, actualResults, nresults);
}
```

## 6. 尾调用 (TAILCALL)

```cpp
// TAILCALL: return f(args)
// 不创建新帧，复用当前帧

void execOpTailCall(OpExecutionContext& ctx, Instruction inst) {
    // 1. 将 args 移到当前帧的 func 位置
    moveArgsToFuncPosition(a, b);
    
    // 2. 更新 CallInfo
    ci.tailcalls++;
    
    // 3. 重入主循环 (在 f 的上下文中)
    ctx.status = HandlerStatus::Reenter;
}
```

## 7. 调用流程图

```
CALL 指令
  ↓
是 Lua 函数?
  ├── Yes → 创建新 CallInfo → goto reentry
  └── No  → 是 C 函数?
              ├── Yes → 调用 C 函数 → 继续执行
              └── No  → 检查 __call 元方法
                          └── 调用元方法
```
