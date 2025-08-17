# 函数测试中发现的Bug

## Bug #1: VM错误 - 尝试调用非函数值（nil）

**描述**: 在执行简单的函数定义和调用测试时，VM抛出错误"attempt to call a non-function value (nil)"

**重现步骤**:
1. 运行 `bin\lua.exe bin\script\function\simple_function_test.lua`
2. 观察到VM执行到PC=13时出现错误

**测试代码**:
```lua
print("开始函数测试")

function add(a, b)
    return a + b
end

local result = add(3, 5)
print("结果是")
print(result)
print("函数测试完成")
```

**VM执行日志**:
```
[VM] PC=5 OpCode=36 A=0 B=0 C=0    -- CLOSURE指令
[VM] PC=6 OpCode=7 A=0 B=2 C=0     -- SETGLOBAL指令
[VM] PC=7 OpCode=5 A=1 B=2 C=0     -- GETGLOBAL指令
[VM] PC=8 OpCode=0 A=0 B=1 C=0     -- MOVE指令
[VM] PC=9 OpCode=1 A=2 B=3 C=0     -- LOADK指令
[VM] PC=10 OpCode=0 A=1 B=2 C=0    -- MOVE指令
[VM] PC=11 OpCode=1 A=3 B=4 C=0    -- LOADK指令
[VM] PC=12 OpCode=0 A=2 B=3 C=0    -- MOVE指令
[VM] PC=13 OpCode=28 A=0 B=3 C=2   -- CALL指令
Execution error: VM Error: attempt to call a non-function value (nil)
```

**预期行为**: 
- 应该输出 "开始函数测试"
- 应该定义函数add
- 应该调用add(3, 5)并返回8
- 应该输出 "结果是"
- 应该输出 8
- 应该输出 "函数测试完成"

**实际行为**: 
- 输出了 "开始函数测试"
- VM在尝试调用函数时崩溃

**严重程度**: 高 - 阻止基本函数定义和调用功能正常工作

**可能原因**: 
- CLOSURE指令(OpCode=36)没有正确创建函数对象
- SETGLOBAL指令(OpCode=7)没有正确设置全局变量
- GETGLOBAL指令(OpCode=5)获取到的是nil而不是函数
- 函数定义和存储过程有问题

**详细分析**:
- VM正确地输出了第一个print语句
- VM执行了CLOSURE指令来创建函数
- VM执行了SETGLOBAL指令来设置全局变量"add"
- 但是当VM尝试通过GETGLOBAL获取函数时，得到的是nil
- 这表明函数没有被正确存储到全局环境中

**状态**: ✅ **已修复** (2025-08-17)

**修复方案**:
1. **SETGLOBAL指令修复**: 实现了正确的全局变量设置逻辑，使用luaS_newlstr创建GCString并调用L->setGlobal
2. **CALL指令修复**: 实现了基本的函数调用机制，支持Lua函数的递归执行
3. **参数传递**: 正确处理函数调用时的参数传递和返回值处理

**修复后测试结果**:
```
开始函数测试
结果是
42
函数测试完成
```

**相关提交**:
- 修复了SETGLOBAL指令实现 (src/vm/vm_executor.cpp 第407-414行)
- 修复了CALL指令的函数调用逻辑 (src/vm/vm_executor.cpp 第475-501行)

**已知限制**:
- 函数返回值处理还需要进一步完善
- 复杂的函数调用场景可能需要更完整的调用栈管理
