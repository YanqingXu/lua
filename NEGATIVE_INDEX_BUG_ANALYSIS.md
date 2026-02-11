# 负索引 arg[-1] 导致 VM 寄存器越界问题分析

## 问题描述
执行 `local val = arg[-1]` 时，VM 抛出 "register index out of range" 错误。

## 测试文件 (test_arg_negative.lua)
```lua
print("Testing arg[-1]")
local val = arg[-1]
print("arg[-1] =")
print(val)
```

## 字节码对比

### 官方 Lua 5.1.5 字节码
```
main <lua\test_arg_negative.lua:0,0> (12 instructions, 48 bytes)
0+ params, 3 slots, 0 upvalues, 1 local, 5 constants, 0 functions
        1       [2]     GETGLOBAL       0 -1    ; print
        2       [2]     LOADK           1 -2    ; "Testing arg[-1]"
        3       [2]     CALL            0 2 1
        4       [3]     GETGLOBAL       0 -3    ; arg
        5       [3]     GETTABLE        0 0 -4  ; -1      ← 关键：使用 RK(-4) 表示常量 -1
        6       [4]     GETGLOBAL       1 -1    ; print
        7       [4]     LOADK           2 -5    ; "arg[-1] ="
        8       [4]     CALL            1 2 1
        9       [5]     GETGLOBAL       1 -1    ; print
        10      [5]     MOVE            2 0
        11      [5]     CALL            1 2 1
        12      [5]     RETURN          0 1

常量表 (5个):
        1       "print"
        2       "Testing arg[-1]"
        3       "arg"
        4       -1                      ← 常量 -1 存储在索引 3 (0-based)
        5       "arg[-1] ="
```

### C++ 实现字节码
```
main <lua\test_arg_negative.lua:0,0> (13 instructions, 52 bytes)
0+ params, 3 slots, 0 upvalues, 0 locals, 7 constants, 0 functions
        1       [-]     GETGLOBAL       0 -1    ; print
        2       [-]     LOADK           1 -2    ; "Testing arg[-1]"
        3       [-]     CALL            0 2 1
        4       [-]     GETGLOBAL       1 -3    ; arg
        5       [-]     UNM             2 -4    ← 错误：生成了 UNM (取负) 指令！
        6       [-]     GETTABLE        0 1 2   ← 错误：使用寄存器 R2 而非常量
        7       [-]     GETGLOBAL       1 -5    ; print
        8       [-]     LOADK           2 -6    ; "arg[-1] ="
        9       [-]     CALL            1 2 1
        10      [-]     GETGLOBAL       1 -7    ; print
        11      [-]     MOVE            2 0
        12      [-]     CALL            1 2 1
        13      [-]     RETURN          0 1

常量表 (7个):
        1       "print"
        2       "Testing arg[-1]"
        3       "arg"
        4       1                       ← 错误：存储的是 1 而非 -1
        5       "print"
        6       "arg[-1] ="
        7       "print"
```

## 问题根因分析

### 核心问题
**编译器将 `-1` 解析为一元负号表达式 `-(1)`，而非数字常量 `-1`**

### 详细分析

#### 官方 Lua 5.1.5 的处理方式
1. **词法分析阶段**：`-1` 被识别为单个 NUMBER token，值为 -1
2. **语法分析阶段**：直接作为数字常量处理
3. **代码生成阶段**：
   - 将 `-1` 添加到常量表（索引 3）
   - 生成 `GETTABLE 0 0 -4`，其中 `-4` = `RKASK(3)` = `256 + 3 = 259`
   - GETTABLE 的 C 参数使用 RK 寻址，直接从常量表读取 -1

#### C++ 实现的错误处理方式
1. **词法分析阶段**：`-` 和 `1` 被识别为两个 token（MINUS 和 NUMBER）
2. **语法分析阶段**：解析为 UnaryExpr (op=MINUS, operand=1)
3. **代码生成阶段**：
   - 将 `1` 添加到常量表（索引 3）
   - 生成 `UNM 2 -4`（对常量 1 取负，结果存入 R2）
   - 生成 `GETTABLE 0 1 2`（使用寄存器 R2 作为索引）
   - **问题**：GETTABLE 期望 C 参数是 RK 值，但 R2 可能包含运行时计算的值

### 为什么会导致寄存器越界？

**UNM 指令的问题**：
```
UNM 2 -4
```
- A = 2 (目标寄存器)
- B = -4 (RK 操作数，表示常量索引 3，值为 1)

**VM 执行 UNM 时**：
- 尝试从 RK(-4) 读取值
- RK(-4) 解码为常量索引 3
- 但 VM 的 RK 函数可能错误地将 -4 解释为寄存器索引
- 导致访问不存在的寄存器，抛出 "register index out of range"

## 修复方案

### 方案1：修复词法分析器（推荐）
**位置**：`lua/src/compiler/lexer.cpp`

**修改**：在词法分析阶段识别负数字面量
- 当遇到 `-` 后紧跟数字时，将其作为单个 NUMBER token
- 直接返回负数值

**优点**：
- 与官方 Lua 行为完全一致
- 生成的字节码更优化（少一条 UNM 指令）
- 避免运行时计算

### 方案2：修复代码生成器
**位置**：`lua/src/compiler/codegen.cpp`

**修改**：在 exp2RK 中特殊处理 UnaryExpr(MINUS, Number)
- 检测到 `-(数字常量)` 模式时
- 直接将负数添加到常量表
- 返回常量索引的 RK 编码

**优点**：
- 改动较小
- 不影响词法分析器

**缺点**：
- 仍然会在 AST 中保留 UnaryExpr 节点
- 不如方案1优雅

### 方案3：修复 VM 的 UNM 指令处理
**位置**：`lua/src/vm/vm.cpp`

**修改**：确保 UNM 指令正确处理 RK 操作数

**注意**：这不能解决根本问题，因为问题在于编译器生成了错误的指令序列

## 推荐修复方案

**采用方案1（修复词法分析器）+ 方案2（代码生成器优化）**

1. **立即修复**：使用方案2快速修复问题
2. **长期优化**：实现方案1以完全匹配官方行为

## 下一步行动

1. 检查 `lua/src/compiler/lexer.cpp` 中的数字解析逻辑
2. 实现方案2：在 `codegen.cpp` 中优化 UnaryExpr(MINUS, Number) 的处理
3. 重新编译并测试
4. 验证 `arg[-1]`、`arg[0]`、`arg[1]` 都能正常工作

