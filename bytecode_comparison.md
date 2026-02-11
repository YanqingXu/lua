# 字节码对比分析报告

## 测试脚本
```lua
-- test_bytecode.lua
local x = 10
local y = 20
local sum = x + y
local product = x * y
print(sum)
print(product)
return sum
```

## 官方 Lua 5.1.5 字节码 (lua_c_analysis)
```
main <..\lua\test_bytecode.lua:0,0> (12 instructions, 48 bytes at 000001BE8E8B5510)
0+ params, 6 slots, 0 upvalues, 4 locals, 3 constants, 0 functions
	1	[5]	LOADK    	0 -1	; 10
	2	[6]	LOADK    	1 -2	; 20
	3	[9]	ADD      	2 0 1
	4	[10]	MUL      	3 0 1
	5	[13]	GETGLOBAL	4 -3	; print
	6	[13]	MOVE     	5 2
	7	[13]	CALL     	4 2 1
	8	[14]	GETGLOBAL	4 -3	; print
	9	[14]	MOVE     	5 3
	10	[14]	CALL     	4 2 1
	11	[17]	RETURN   	2 2
	12	[17]	RETURN   	0 1
constants (3) for 000001BE8E8B5510:
	1	10
	2	20
	3	"print"
locals (4) for 000001BE8E8B5510:
	0	x	2	12
	1	y	3	12
	2	sum	4	12
	3	product	5	12
```

## C++ 实现字节码 (lua_in_cpp)
```
main <test_bytecode.lua:0,0> (9 instructions, 36 bytes at 000001DA64FE9E50)
0+ params, 6 slots, 0 upvalues, 0 locals, 4 constants, 0 functions
	1	[-]	LOADK    	0 -1	; 10
	2	[-]	LOADK    	1 -2	; 20
	3	[-]	ADD      	2 0 1
	4	[-]	MUL      	3 0 1
	5	[-]	GETGLOBAL	4 -3	; print
	6	[-]	CALL     	4 2 1
	7	[-]	GETGLOBAL	4 -4	; print
	8	[-]	CALL     	4 2 1
	9	[-]	RETURN   	4 2
constants (4) for 000001DA64FE9E50:
	1	10
	2	20
	3	"print"
	4	"print"
locals (0) for 000001DA64FE9E50:
```

## 差异分析

### 差异1：指令数量不同
- **官方**: 12条指令
- **C++实现**: 9条指令
- **差异**: 少了3条指令

### 差异2：缺少 MOVE 指令
**官方字节码**:
```
5	[13]	GETGLOBAL	4 -3	; print
6	[13]	MOVE     	5 2      ; 将sum移动到寄存器5
7	[13]	CALL     	4 2 1
8	[14]	GETGLOBAL	4 -3	; print
9	[14]	MOVE     	5 3      ; 将product移动到寄存器5
10	[14]	CALL     	4 2 1
```

**C++实现字节码**:
```
5	[-]	GETGLOBAL	4 -3	; print
6	[-]	CALL     	4 2 1    ; 缺少MOVE指令
7	[-]	GETGLOBAL	4 -4	; print
8	[-]	CALL     	4 2 1    ; 缺少MOVE指令
```

**问题**: C++实现在调用print函数时，没有生成MOVE指令将参数移动到正确的寄存器位置。

### 差异3：RETURN 指令不同
**官方字节码**:
```
11	[17]	RETURN   	2 2      ; 返回sum (寄存器2)
12	[17]	RETURN   	0 1      ; 最终返回
```

**C++实现字节码**:
```
9	[-]	RETURN   	4 2      ; 错误的寄存器索引
```

**问题**: C++实现的RETURN指令使用了错误的寄存器索引(4而非2)。

### 差异4：常量表重复
**官方字节码**:
```
constants (3):
	1	10
	2	20
	3	"print"
```

**C++实现字节码**:
```
constants (4):
	1	10
	2	20
	3	"print"
	4	"print"  ; 重复的常量
```

**问题**: C++实现没有正确去重常量，导致"print"被添加了两次。

### 差异5：局部变量信息缺失
**官方字节码**:
```
locals (4):
	0	x	2	12
	1	y	3	12
	2	sum	4	12
	3	product	5	12
```

**C++实现字节码**:
```
locals (0):  ; 完全缺失
```

**问题**: C++实现没有记录局部变量的调试信息。

### 差异6：行号信息缺失
**官方字节码**: 每条指令都有行号 `[5]`, `[6]`, `[9]`, `[10]`, `[13]`, `[14]`, `[17]`
**C++实现字节码**: 所有指令都显示 `[-]`

**问题**: C++实现没有正确记录源代码行号信息。

## 根本原因总结

1. **函数调用参数处理错误**: CodeGenerator在生成函数调用时，没有正确处理参数的寄存器分配
2. **常量池管理问题**: 没有实现常量去重机制
3. **调试信息缺失**: 没有记录局部变量和行号信息
4. **RETURN指令生成错误**: 使用了错误的寄存器索引

## 问题定位详解

### 问题1：函数调用参数位置错误

**当前实现** (`lua/src/compiler/codegen.cpp` 第1122-1178行):
```cpp
void CodeGenerator::callExpr(const CallExpr& e, ExprDesc& desc) {
    i32 funcReg;
    i32 nargs = static_cast<i32>(e.args.size());

    // ... 方法调用处理 ...

    // 普通函数调用
    ExprDesc func;
    expr(*e.func, func);
    funcReg = exp2AnyReg(func);  // 第1157行

    // 计算参数 - 问题：参数不保证连续
    for (const auto& arg : e.args) {
        ExprDesc argDesc;
        expr(*arg, argDesc);
        exp2NextReg(argDesc);  // 第1164行 - 只是放入下一个寄存器
    }

    // 生成CALL指令
    codeABC(OpCode::CALL, funcReg, nargs + 1, 2);  // 第1170行

    // 释放参数寄存器
    freeRegs(nargs);  // 第1173行

    desc.kind = ExprKind::Call;
    desc.u.s.info = funcReg;
}
```

**问题分析**:
- 第1157行：`funcReg = exp2AnyReg(func)` 将函数放入某个寄存器（假设是R4）
- 第1164行：`exp2NextReg(argDesc)` 将参数放入下一个空闲寄存器
- **但是**：如果在计算函数表达式时已经占用了一些寄存器，参数可能不会紧跟在funcReg之后
- 例如：funcReg=4，但参数可能在R5, R6...，也可能在R2, R3...（如果之前有寄存器被释放）

**官方Lua实现** (`lua_c_analysis/src/lparser.c` 第3356-3401行):
```c
static void funcargs (LexState *ls, expdesc *f) {
    FuncState *fs = ls->fs;
    expdesc args;
    int base, nparams;

    // ... 解析参数 ...

    lua_assert(f->k == VNONRELOC);  // 确保函数在寄存器中
    base = f->u.s.info;  // 获取函数寄存器作为base

    if (args.k != VVOID)
        luaK_exp2nextreg(fs, &args);  // 参数放入下一个寄存器

    nparams = fs->freereg - (base+1);  // 计算参数数量

    init_exp(f, VCALL, luaK_codeABC(fs, OP_CALL, base, nparams+1, 2));
    fs->freereg = base+1;  // 重置freereg到base+1
}
```

**关键差异**:
1. 官方实现确保函数在base寄存器
2. 参数从base+1开始连续分配
3. 通过 `fs->freereg - (base+1)` 计算参数数量
4. 调用后重置 `freereg = base+1`

### 修复方案

需要修改 `callExpr` 函数，确保：
1. 函数在base寄存器
2. 参数连续分配在base+1, base+2, ..., base+nargs
3. 生成CALL指令时使用正确的寄存器布局

**修复代码**:
```cpp
void CodeGenerator::callExpr(const CallExpr& e, ExprDesc& desc) {
    i32 nargs = static_cast<i32>(e.args.size());
    i32 base;

    if (e.isMethodCall) {
        // 方法调用处理（保持不变）
        // ...
        base = funcReg;
    } else {
        // 普通函数调用
        ExprDesc func;
        expr(*e.func, func);

        // 确保函数在寄存器中
        base = exp2AnyReg(func);
    }

    // 关键修复：确保参数连续分配在base+1开始的位置
    // 保存当前freereg，确保参数从base+1开始
    i32 savedFreeReg = freereg_;
    freereg_ = base + 1;  // 强制参数从base+1开始分配

    // 计算参数
    for (const auto& arg : e.args) {
        ExprDesc argDesc;
        expr(*arg, argDesc);
        exp2NextReg(argDesc);  // 现在会连续分配到base+1, base+2, ...
    }

    // 恢复freereg（如果需要）
    // freereg现在应该是base+1+nargs

    // 生成CALL指令
    codeABC(OpCode::CALL, base, nargs + 1, 2);

    // 释放参数寄存器，保留base寄存器（存放返回值）
    freereg_ = base + 1;

    desc.kind = ExprKind::Call;
    desc.u.s.info = base;
}
```

