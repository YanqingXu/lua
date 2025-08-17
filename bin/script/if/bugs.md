# IF语句Bug报告

## 已修复问题

### Bug #1: 跳转偏移计算错误 ✅ 已修复

**描述**: if语句跳转后尝试调用nil值或字符串，导致VM错误

**错误信息**: "attempt to call a non-function value (nil)" 或 "attempt to call a non-function value (string)"

**根本原因**: 跳转偏移计算公式错误，导致JMP指令跳转到错误位置

**修复位置**: `src/compiler/compiler_utils.cpp` 第114行

**修复内容**:
- **错误公式**: `offset = targetAddr - jumpAddr - 1`
- **正确公式**: `offset = targetAddr - jumpAddr`

**影响范围**: 所有if语句，包括if-else和嵌套if

**修复日期**: 2025-08-17

**状态**: 🟢 已修复

---

### Bug #2: 寄存器状态管理问题 ✅ 已修复

**描述**: 跳转后寄存器内容与预期不符，导致函数调用失败

**原因**: 编译器在if语句结束后没有正确管理寄存器状态

**修复位置**: `src/compiler/statement_compiler.cpp` 第438-443行

**修复内容**: 改进了if语句编译后的寄存器状态重置逻辑，使用新的`resetToStackTop`方法

**状态**: 🟢 已修复

---

### 新增功能: 寄存器管理改进 ✅ 已实现

**新增方法**: `resetToStackTop(int newStackTop)`

**实现位置**:
- 声明: `src/compiler/register_manager.hpp` 第140行
- 实现: `src/compiler/register_manager.cpp` 第231-251行

**功能**: 提供更精确的寄存器栈顶重置功能

**用途**: 确保if语句不会影响后续代码的寄存器分配

## 修复过程记录

### 调试步骤
1. **添加调试信息**: 在编译器中添加跳转地址计算的调试输出
2. **分析字节码**: 通过查看生成的字节码序列，发现跳转地址计算错误
3. **定位问题**: 发现JMP指令跳转到错误位置，跳过了GETGLOBAL指令
4. **修复验证**: 通过多个测试用例验证修复的正确性

### VM执行日志对比

**修复前**:
```
[VM] PC=5 OpCode=22 A=0 B=4 C=256
[VM] PC=10 OpCode=28 A=3 B=2 C=2
Execution error: VM Error: attempt to call a non-function value (nil)
```

**修复后**:
```
[VM] PC=5 OpCode=22 A=0 B=6 C=256
[VM] PC=11 OpCode=5 A=3 B=0 C=0
[VM] PC=12 OpCode=0 A=1 B=3 C=0
[VM] PC=13 OpCode=1 A=4 B=4 C=0
[VM] PC=14 OpCode=0 A=2 B=4 C=0
[VM] PC=15 OpCode=28 A=1 B=2 C=2
B
```

## 测试用例状态

### 基础if语句测试 ✅ 全部通过
- ✅ 简单if语句
- ✅ if-else语句
- ✅ if-elseif-else语句
- ✅ 布尔值测试
- ✅ nil值测试
- ✅ 数字0测试
- ✅ 空字符串测试

### 嵌套if语句测试 ✅ 全部通过
- ✅ 简单嵌套
- ✅ 深层嵌套
- ✅ 复杂条件嵌套
- ✅ 嵌套中的elseif
- ✅ 多重嵌套的else分支

### 逻辑表达式测试 ✅ 全部通过
- ✅ and运算符
- ✅ or运算符
- ✅ not运算符
- ✅ 复合逻辑表达式
- ✅ 短路求值测试
- ✅ 比较运算符组合
- ✅ 复杂的括号组合

## 当前状态

### ✅ 已完全修复
- if语句功能完全正常
- 所有测试用例通过
- 跳转逻辑正确
- 寄存器管理稳定

### 📝 技术细节
- **JMP指令语义**: `PC += offset`
- **跳转计算**: 目标地址 - 当前地址 = 偏移量
- **寄存器生命周期**: 通过作用域管理确保正确的分配和释放

## 遗留问题

目前if语句功能没有已知的遗留问题。所有基础功能、嵌套结构和复杂逻辑表达式都工作正常。

## 测试文件列表

以下测试文件全部通过：
- `basic_if_tests.lua` - 基础if语句功能
- `nested_if_tests.lua` - 嵌套if语句
- `logical_expressions_tests.lua` - 逻辑表达式
- `simple_if_else_test.lua` - 简单if-else测试
- `minimal_if_else_test.lua` - 最小if-else测试
