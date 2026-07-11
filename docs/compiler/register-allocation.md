---
status: current
verified_against: src/compiler/register_allocator.hpp; src/compiler/codegen/codegen.hpp; src/compiler/codegen/codegen.cpp; src/compiler/codegen/codegen_ops.hpp; src/compiler/codegen/function_compiler.hpp; src/compiler/codegen/function_compiler.cpp; src/compiler/codegen/expression_emitter.cpp; src/compiler/codegen/statement_emitter.cpp; src/compiler/codegen/codegen_stmt.cpp; src/compiler/codegen/codegen_state.hpp
last_checked: 2026-05-22
applies_to: current CodeGenerator register allocation model
---

# 寄存器分配

Lua 5.1 字节码是基于寄存器的。每个 `Proto` 记录一个 `maxStackSize`，每条指令在活跃调用帧内读取或写入编号的虚拟寄存器。因此 C++ 编译器必须在 VM 执行开始前决定局部变量、临时值、调用参数和返回值的位置。

当前实现使用 `RegisterAllocator` 作为寄存器游标的所有者：

```cpp
class RegisterAllocator {
public:
    void bind(Proto* proto) noexcept;
    i32 current() const noexcept;
    i32 alloc();
    void freeReg(i32 reg, i32 activeLocals);
    void freeRegs(i32 n);
    void checkStack(i32 n);
    void setFreeReg(i32 reg) noexcept;
    void resetToLocals(i32 activeLocals) noexcept;
    void restore(i32 saved) noexcept;
    void reserve(i32 count) noexcept;
    void ensureAtLeast(i32 reg) noexcept;
    void reset(i32 start = 0) noexcept;
};
```

`freereg_` 是私有的。代码生成代码通过上述语义方法访问它。发射器代码使用 `CodegenOps::currentReg()` / `setFreeRegAndCheck()` / `reserveRegsAndCheck()` 以及轻量的 `RegisterFrame` 辅助类来进行重复的帧状游标更新。

## 寄存器区域

在函数体的任何时刻，寄存器的组织方式如下：

```text
R(0) ... R(activeVarCount-1)     活跃的局部变量
R(activeVarCount) ... R(free-1)  临时值、调用帧、表字段
R(free) ...                      可用寄存器
```

`LocalVarScope::activeVarCount_` 是活跃局部变量的数量。`RegisterAllocator::current()` 指向下一个可用临时槽位。

## 主要规则

- 局部变量在其词法作用域的生命周期内占用固定寄存器。
- 临时值从 `current()` 分配，只有作为最新的临时值时才能释放。
- 语句边界通常将临时值重置回活跃局部变量。
- 函数调用需要连续寄存器：函数在 `base`，参数在其后，结果从 `base` 开始。
- 多返回值由 `CallResultInfo` 表示，直到外层上下文决定需要多少个结果。
- 表数组字段在表寄存器之后累积，直到 `SETLIST` 刷新它们。
- `FORPREP`、`FORLOOP` 和 `TFORLOOP` 使用 Lua 5.1 定义的固定寄存器布局。

## 值下降辅助函数

旧 `ExprDesc` / `exp2*` 模型已不再是生产编译器源码的一部分。当前辅助函数操作 `ValueResult`：

| 当前辅助函数 | 用途 |
|---|---|
| `emitValue(const Expr&)` | 将表达式下降为 `ValueResult` |
| `materializeValue(const ValueResult&, i32 reg)` | 将值强制放入特定寄存器 |
| `valueToRK(const ValueResult&)` | 可能时使用 RK 编码，否则物化 |
| `valueToAnyReg(const ValueResult&)` | 返回包含该值的寄存器 |
| `valueToNextReg(const ValueResult&)` | 在当前空闲寄存器处物化并推进 |
| `forceSingleValue(const ValueResult&)` | 将 call/vararg 多返回值转换为单值 |

## 常见流程

### 局部变量声明

对于 `local a, b = f()`：

1. 保存当前空闲寄存器。
2. 从 `activeVarCount_` 开始预留局部变量槽位。
3. 生成初始化值到局部变量基址。
4. 如果最后一个初始化器是调用或 vararg，设置其期望的结果数量。
5. 用 `LOADNIL` 填充缺失的局部变量。
6. 用 `adjustLocalVars` 激活局部变量。

重要不变式是：局部变量槽位和初始化结果槽位在局部变量变为活跃之前必须对齐。

### 函数调用表达式

对于 `print(type(x))`：

1. 将被调用者下降到寄存器。
2. 将每个参数连续放置在被调用者之后。
3. 如果最后一个参数是调用或 vararg，决定是否应为开放式。
4. 发射 `CALL`。
5. 返回 `CallResultInfo`，让父上下文决定保留一个结果还是多个结果。

### 返回语句

对于 `return f()`：

- 如果返回的表达式是尾位置的单个调用，代码生成可发射 `TAILCALL`。
- 如果最后一个返回表达式是 call/vararg 且需要多个值，发射 `RETURN` 并设置 `B = 0`。
- 否则将每个结果物化到连续寄存器中并发射固定计数的 `RETURN`。

### 数值 For 循环

数值 for 循环使用：

```text
R(base)     内部索引
R(base + 1) 上界
R(base + 2) 步长
R(base + 3) 可见循环变量
```

编译器在循环体之前发射 `FORPREP`，在循环体之后发射 `FORLOOP`。寄存器分配器在生成循环体期间保持循环控制范围被预留。

### 泛型 For 循环

泛型 for 循环使用：

```text
R(base)     生成器函数
R(base + 1) 状态
R(base + 2) 控制变量
R(base + 3) 第一个可见循环变量
```

`TFORLOOP` 从 `base + 3` 开始写入迭代器结果。

## 调试寄存器问题

大多数寄存器问题属于以下类别之一：

- 临时值未被释放，后续局部变量向上偏移。
- 保存的空闲寄存器恢复过早或过晚。
- 调用参数区域未保持连续。
- 多返回值调用被意外强制为单值。
- 循环布局复用了 Lua 的保留控制寄存器。

实用测试：

```powershell
bin\lua_test.exe --filter "Value Pipeline"
bin\lua_test.exe --filter "Call Pipeline"
bin\lua_test.exe --filter "Codegen MultiRet"
bin\lua_test.exe --filter "Function Codegen"
```
