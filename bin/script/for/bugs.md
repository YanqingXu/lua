# FOR循环测试中发现的Bug

## Bug #1: VM错误 - 尝试调用非函数值（数字）

**描述**: 在执行简单的for循环测试时，VM抛出错误"attempt to call a non-function value (number)"

**重现步骤**:
1. 运行 `bin\lua.exe bin\script\for\simple_for_test.lua`
2. 观察到VM执行到PC=15时出现错误

**测试代码**:
```lua
print("开始for循环测试")
for i = 1, 3 do
    print(i)
end
print("for循环测试完成")
```

**VM执行日志**:
```
[VM] PC=10 OpCode=32 A=0 B=4 C=256
[VM] PC=15 OpCode=28 A=4 B=2 C=2
Execution error: VM Error: attempt to call a non-function value (number)
```

**预期行为**: 
- 应该输出 "开始for循环测试"
- 应该输出 1, 2, 3
- 应该输出 "for循环测试完成"

**实际行为**: 
- 输出了 "开始for循环测试"
- VM在执行for循环时崩溃

**严重程度**: 高 - 阻止基本for循环功能正常工作

**可能原因**: 
- VM在处理for循环的FORLOOP指令(OpCode=32)后出现问题
- 跳转指令可能导致错误的寄存器状态
- CALL指令尝试调用数字值而不是函数

**状态**: ✅ **已修复** (2025-08-17)

**修复方案**:
- 移除了编译器中多余的MOVE指令，该指令错误地将内部索引复制到用户可见变量
- FORLOOP指令会自动更新用户可见的循环变量(R(A+3))，不需要额外的MOVE指令
- 参考Lua 5.1官方源码实现，确保FORLOOP指令的正确行为

**修复后测试结果**:
```
开始for循环测试
1
2
3
for循环测试完成
```

**相关提交**: 修复了FORLOOP无限循环问题，移除了src/compiler/statement_compiler.cpp中第520-521行的多余MOVE指令
