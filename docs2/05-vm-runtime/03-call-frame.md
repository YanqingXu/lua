# Call Frame — 调用帧

## 1. 这个模块解决什么问题？

CallInfo 是每次函数调用的上下文记录，管理栈帧的生命周期。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/vm/state/call_info.hpp` | CallInfo 结构定义 |
| `src/vm/vm_frame.cpp` | 调用帧管理 |
| `src/vm/vm_call.cpp` | 函数调用实现 |

## 3. CallInfo 结构

```cpp
struct CallInfo {
    i32 func;          // 函数对象在栈中的索引
    i32 base;          // 参数基址（也是寄存器 R(0) 的位置）
    i32 top;           // 栈顶（局部变量 + 临时值的上界）
    i32 nresults;      // 期望的返回值数量
    const Instruction* savedpc;  // 程序计数器（用于恢复执行）
    i32 tailcalls;     // 尾调用计数
};
```

## 4. 栈帧布局

```
┌─────────────┐ ← top (栈顶)
│  局部变量3  │
│  局部变量2  │
│  局部变量1  │
├─────────────┤ ← base + maxStackSize
│  ...        │
├─────────────┤ ← base + numParams
│  参数3      │
│  参数2      │
│  参数1      │ ← base (= R(0))
├─────────────┤ ← func
│  函数对象   │
└─────────────┘
```

## 5. 调用时创建 CallInfo

```cpp
// luaV_execute / VM::call 中:
CallInfo newCI;
newCI.func = funcIndex;       // 函数在栈中的位置
newCI.base = funcIndex + 1;   // 参数紧接函数后面
newCI.top = newCI.base + proto->getMaxStackSize();
newCI.nresults = nresults;
newCI.savedpc = nullptr;      // 新帧从 PC=0 开始
newCI.tailcalls = 0;

L->pushCallInfo(newCI);
```

## 6. 返回时恢复 CallInfo

```cpp
// RETURN 指令:
// 1. 将返回值放入调用者栈（从 ci.func 位置开始）
// 2. 弹出当前 CallInfo
// 3. 恢复调用者的 CallInfo
// 4. goto reentry (恢复调用者的执行)

CallInfo& prevCI = L->popCallInfo();
// 现在 L->getCurrentCallInfo() 是调用者
// VM 会从 prevCI.savedpc 恢复 pc
```

## 7. savedpc 的作用

```
savedpc 存在的原因:
  - 函数被调用时，调用者的 PC 需要保存
  - 函数返回时，调用者从 savedpc 继续执行
  - Hook 执行时，savedpc 提供准确的调用栈信息

savedpc 的值:
  - 新帧: nullptr (从 PC=0 开始)
  - 执行中: 指向下一条要执行的指令
  - 调用子函数后: 指向 CALL 之后的指令
```

## 8. 调用栈

```
LuaState 维护一个 CallInfo 数组:

[0] 虚拟主函数 (程序的入口)
[1] 第一个被调用的函数
[2] 嵌套调用
...

深度限制: MAX_CALLS (防止 C 栈溢出)
```
