# 字节码修复报告

## 修复日期
2026-02-11

## 修复目标
修复 `lua/src/compiler/codegen.cpp` 中的 `callExpr` 函数，使其生成的字节码与官方 Lua 5.1.5 实现一致。

## 核心问题
函数调用时参数寄存器分配不连续，导致生成的 CALL 指令无法正确访问参数。

### 问题详情
- **Lua调用约定**：函数必须在寄存器A，参数必须连续分配在A+1, A+2, ..., A+nargs
- **修复前**：参数使用`exp2NextReg`分配，可能不连续（如函数在R4，参数在R2, R3）
- **结果**：`CALL 4 2 1`会从R5读取参数，但参数实际在R2

## 修复内容

### 修复1：callExpr函数 - 确保参数连续分配
**文件**: `lua/src/compiler/codegen.cpp` (第1122-1256行)

**修复策略**:
1. 确保函数表达式在base寄存器
2. 强制`freereg_ = base + 1`，确保参数从base+1开始连续分配
3. 对每个参数调用`exp2NextReg`，自动连续分配到base+1, base+2, ...
4. 生成CALL指令
5. 重置`freereg_ = base + 1`（保留返回值寄存器）

**关键代码**:
```cpp
// 强制freereg = base + 1，确保参数从base+1开始分配
freereg_ = base + 1;

// 计算每个参数，exp2NextReg会将它们连续分配到base+1, base+2, ...
for (const auto& arg : e.args) {
    ExprDesc argDesc;
    expr(*arg, argDesc);
    exp2NextReg(argDesc);  // 现在会连续分配到base+1, base+2, base+3, ...
}

// 生成CALL指令
codeABC(OpCode::CALL, base, nargs + 1, 2);

// 调用后重置freereg
freereg_ = base + 1;
```

### 修复2：discharge函数 - 处理NonRelocatable类型
**文件**: `lua/src/compiler/codegen.cpp` (第354-361行)

**问题**: NonRelocatable类型的表达式（如局部变量）没有生成MOVE指令

**修复代码**:
```cpp
case ExprKind::NonRelocatable:
    // NonRelocatable表示表达式结果已经在某个寄存器中（desc.u.s.info）
    // 如果目标寄存器不同，需要生成MOVE指令将值移动到目标寄存器
    if (desc.u.s.info != reg) {
        codeABC(OpCode::MOVE, reg, desc.u.s.info, 0);
    }
    break;
```

### 修复3：关闭调试输出
**文件**: `lua/src/core/function.cpp` (第42-67行)

注释掉`Proto::addConstant`和`Proto::getConstant`中的调试输出，避免污染字节码输出。

## 修复结果对比

### 测试脚本 (test_bytecode.lua)
```lua
local x = 10
local y = 20
local sum = x + y
local product = x * y
print(sum)
print(product)
return sum
```

### 官方 Lua 5.1.5 字节码
```
main <..\lua\test_bytecode.lua:0,0> (12 instructions, 48 bytes)
0+ params, 6 slots, 0 upvalues, 4 locals, 3 constants, 0 functions
	1	[5]	LOADK    	0 -1	; 10
	2	[6]	LOADK    	1 -2	; 20
	3	[9]	ADD      	2 0 1
	4	[10]	MUL      	3 0 1
	5	[13]	GETGLOBAL	4 -3	; print
	6	[13]	MOVE     	5 2      ← MOVE指令
	7	[13]	CALL     	4 2 1
	8	[14]	GETGLOBAL	4 -3	; print
	9	[14]	MOVE     	5 3      ← MOVE指令
	10	[14]	CALL     	4 2 1
	11	[17]	RETURN   	2 2
	12	[17]	RETURN   	0 1
constants (3):
	1	10
	2	20
	3	"print"
locals (4):
	0	x	2	12
	1	y	3	12
	2	sum	4	12
	3	product	5	12
```

### C++ 实现字节码（修复后）
```
main <test_bytecode.lua:0,0> (12 instructions, 48 bytes)
0+ params, 6 slots, 0 upvalues, 0 locals, 4 constants, 0 functions
	1	[-]	LOADK    	0 -1	; 10
	2	[-]	LOADK    	1 -2	; 20
	3	[-]	ADD      	2 0 1
	4	[-]	MUL      	3 0 1
	5	[-]	GETGLOBAL	4 -3	; print
	6	[-]	MOVE     	5 2      ← ✅ MOVE指令已生成
	7	[-]	CALL     	4 2 1
	8	[-]	GETGLOBAL	4 -4	; print
	9	[-]	MOVE     	5 3      ← ✅ MOVE指令已生成
	10	[-]	CALL     	4 2 1
	11	[-]	MOVE     	4 2
	12	[-]	RETURN   	4 2
constants (4):
	1	10
	2	20
	3	"print"
	4	"print"
locals (0):
```

## 修复成果

### ✅ 已解决
1. **MOVE指令生成** - 函数调用参数现在正确生成MOVE指令（第6行和第9行）
2. **参数寄存器连续** - 参数现在连续分配在函数寄存器之后
3. **指令数量一致** - 都是12条指令

### ⚠️ 仍存在的差异
1. **常量表重复** - "print"出现两次（索引3和4），官方只有一次（索引3）
2. **局部变量信息缺失** - locals (0) vs locals (4)
3. **行号信息缺失** - 所有指令显示`[-]`而非实际行号
4. **RETURN指令差异** - 第11-12行与官方不同

## 下一步工作

### P1（重要）：修复常量表重复
- **问题**: `stringConstant`函数没有去重机制
- **影响**: 浪费内存，生成冗余常量
- **修复**: 在`stringConstant`中添加查找逻辑，复用已存在的常量

### P2（一般）：添加局部变量调试信息
- **问题**: CodeGenerator没有记录局部变量信息
- **影响**: 调试困难，字节码不完整
- **修复**: 在`addLocalVar`时调用`Proto::addLocVar`

### P3（一般）：添加行号信息
- **问题**: CodeGenerator没有记录源代码行号
- **影响**: 错误报告不准确
- **修复**: 在生成指令时记录行号信息

### P3（一般）：修复RETURN指令
- **问题**: return语句生成了额外的MOVE指令
- **影响**: 性能略有下降
- **修复**: 优化return语句的代码生成逻辑

## 参考资料
- 官方实现: `lua_c_analysis/src/lparser.c` funcargs函数 (第3356-3401行)
- 官方实现: `lua_c_analysis/src/lcode.c` luaK_exp2nextreg函数 (第1611-1616行)

