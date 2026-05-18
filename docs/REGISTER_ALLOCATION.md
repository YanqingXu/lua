---
status: current
verified_against: docs/PROJECT_STATUS.md; src/compiler/register_allocator.hpp; src/compiler/codegen.cpp
last_checked: 2026-05-18
applies_to: current register allocation model
---

# 寄存器分配详解：`freereg_` 的完整生命周期

本文从寄存器分配的角度，以 `freereg_` 这一核心字段为主线，系统梳理 `CodeGenerator` 在各类代码生成场景下如何操控寄存器空间，以及每一次 `freereg_` 变动背后的真实原因。

---

## 1. 理解寄存器模型的起点

Lua 5.1 VM 是一台基于寄存器的虚拟机。每个函数的"执行栈"在编译期被分配成一段固定宽度的连续寄存器数组，VM 在运行期不做动态扩容，所以代码生成器必须在编译时就计算出"这个函数最多需要多少个寄存器"，并写入 `Proto::maxStackSize`。

`freereg_` 正是那根"游标"，永远指向下一个可以被安全使用的寄存器槽位。

```text
寄存器布局（某一时刻的快照）

 R(0)  R(1)  R(2)  R(3)  R(4)  R(5)  ...
╔═════╦═════╦═════╦═════╦─────╦─────╦─────
║ loc ║ loc ║ loc ║tmp  ║     ║     ║
╚═════╩═════╩═════╩═════╩─────╩─────╩─────
                          ↑
                      freereg_ == 4
                      nactvar_ == 3
```

---

## 2. `freereg_` 的修改者全景

把所有改动 `freereg_` 的代码按"动作类型"分类，得到以下八类操作：

| 操作 | 变化方向 | 关键函数/位置 |
|------|---------|--------------|
| 分配一个临时寄存器 | +1 | `allocReg()` |
| 释放末尾临时寄存器 | −1 | `freeReg()` |
| 注册局部变量（预分配） | +1 / slot | `addLocalVar()` |
| 激活局部变量（对齐游标） | 对齐到 `nactvar_` | `adjustLocalVars()` |
| 作用域退出（回收局部） | 收缩到 `nactvar_` | `removeLocalVars()`, `leaveBlock()` |
| 语句结束后硬回收 | = `nactvar_` | `CallStmt`、`leaveBlock()` |
| 临时重定向（save / restore） | 先降后升 | `LocalStmt`、`ReturnStmt`、`CallExpr`、… |
| 显式固定（循环控制寄存器） | 精确赋值 | `ForNumStmt`、`ForInStmt`、`luaK_self()` |

---

## 3. 基础操作：`allocReg()` 与 `freeReg()`

### 3.1 `allocReg()`

```cpp
i32 CodeGenerator::allocReg() {
    i32 reg = freereg_++;
    if (freereg_ > proto_->getMaxStackSize())
        proto_->setMaxStackSize(freereg_);
    return reg;
}
```

逻辑非常直白：

1. 返回当前 `freereg_`（就是新分配的寄存器编号）
2. `freereg_` 自增
3. 顺便更新 `maxStackSize`（只增不减）

每次临时表达式需要一个槽位时都走这里。

### 3.2 `freeReg()`

```cpp
void CodeGenerator::freeReg(i32 reg) {
    if (reg >= nactvar_ && reg == freereg_ - 1)
        freereg_--;
}
```

这里有两个关键约束：

- `reg >= nactvar_`：只允许回收临时寄存器，局部变量寄存器绝对不能被回收
- `reg == freereg_ - 1`：只允许回收"栈顶"寄存器

这两个约束合起来意味着：`freeReg()` 只能做"弹栈"操作，不支持任意释放。如果中间某个寄存器先用完，必须等到比它高地址的寄存器都释放之后，它才能被释放。

这种设计是刻意的，因为当前这套寄存器分配策略天然假设"临时寄存器是线性压栈的"。一旦出现中间跳空的情况（比如嵌套调用导致非连续布局），就需要靠更上层的机制（save/restore、显式移动）来保证正确性。

### 3.3 `freeRegs(n)`

批量从栈顶弹 `n` 个寄存器，对 `for` 循环结束后释放控制变量区非常有用。

---

## 4. `checkStack(n)`：不改 `freereg_`，但同样重要

```cpp
void CodeGenerator::checkStack(i32 n) {
    i32 newstack = freereg_ + n;
    if (newstack > proto_->getMaxStackSize())
        proto_->setMaxStackSize(newstack);
}
```

`checkStack(n)` 本身**不改变 `freereg_`**，它只是对 `maxStackSize` 做"前瞻性"更新：

> 如果接下来要往 `freereg_` 之后再放 `n` 个值，那 VM 的栈就需要相应地增长。

生成器里大量的 `checkStack(0)` 看起来是"空操作"，实际是在每次手动改写 `freereg_` 之后，把新的 `freereg_` 值同步给 `maxStackSize`——因为手动赋值的 `freereg_ = X` 不会经过 `allocReg()` 的自动更新路径。

简单规则：

- 经过 `allocReg()` 的路径：`maxStackSize` 自动更新
- 手动赋值 `freereg_ = X`：必须手动跟一个 `checkStack(0)`

---

## 5. 局部变量如何占据固定寄存器

### 5.1 `addLocalVar(name)` — 预分配

```cpp
i32 CodeGenerator::addLocalVar(const Str& name) {
    i32 reg = freereg_;
    localVars_.emplace_back(name, reg, ...);
    freereg_++;
    checkStack(0);
    return reg;
}
```

这个函数会把 `freereg_` 当前位置"标记"给新局部变量，然后自增 `freereg_`，并登记到 `localVars_`。

**注意**：仅仅调用 `addLocalVar()` 还不代表这个变量被"激活了"——它还不在 `nactvar_` 的保护范围内，此时的 `freereg_` 是"已经前进了，但 nactvar_ 还没跟上"的中间状态。

### 5.2 `adjustLocalVars(nvars)` — 激活

```cpp
void CodeGenerator::adjustLocalVars(i32 nvars) {
    nactvar_ += nvars;
    freereg_ = nactvar_;
    checkStack(0);
}
```

这一步是把 `nactvar_` 推进到包含新变量的位置，然后把 `freereg_` 对齐到 `nactvar_`。

执行完之后，新局部变量的寄存器就被纳入"受保护区"，`freeReg()` 永远不会回收它们。

### 5.3 `removeLocalVars(tolevel)` — 作用域退出时回收

```cpp
void CodeGenerator::removeLocalVars(i32 tolevel) {
    while (nactvar_ > tolevel) {
        nactvar_--;
        // ... 记录 endpc
    }
    freereg_ = nactvar_;
    checkStack(0);
}
```

离开一个作用域时，把 `nactvar_` 收缩到进入前的水位，`freereg_` 同步收缩。

这保证了下一条语句不会被上一个作用域的局部变量"残留"影响。

### 5.4 三步完整生命周期追踪示例

```lua
do
  local a, b = 1, 2
  -- 使用 a, b
end
-- a, b 作用域结束
```

```text
进入 do 块前       nactvar_=0  freereg_=0

addLocalVar("a")   → freereg_=1  (nactvar_ 还是 0)
addLocalVar("b")   → freereg_=2  (nactvar_ 还是 0)

（重置 freereg_=0 准备编译初始化表达式）

discharge(1, R(0)) → LOADK R(0) 1
discharge(2, R(1)) → LOADK R(1) 2

adjustLocalVars(2) → nactvar_=2  freereg_=2

使用 a, b 阶段      nactvar_=2  freereg_=2...(临时区在 freereg_>=2)

离开 do 块
removeLocalVars(0) → nactvar_=0  freereg_=0
```

---

## 6. `freereg_` 的 Save/Restore 模式

这是整个生成器里最值得单独理解的一个"设计模式"。

很多语句在生成过程中需要对 `freereg_` 做"临时修改"，但生成完之后又要回到修改前的状态。这种情况的标准做法是：

```cpp
i32 savedFreereg = freereg_;
    // ... 临时调整 freereg_ 并生成子表达式 / 指令 ...
freereg_ = savedFreereg;
// 再 checkStack(0) 同步 maxStackSize
```

以下逐一分析每个使用这个模式的场景。

---

## 7. `LocalStmt`：最复杂的 `freereg_` 管理

这是整个生成器里 `freereg_` 变化最复杂的地方，也是最容易出 bug 的地方。

代码完整流程如下：

```text
进入 LocalStmt：   base = nactvar_   savedFreereg = freereg_

Step 1: freereg_ = base
        addLocalVar("a") → freereg_=base+1
        addLocalVar("b") → freereg_=base+2
        addLocalVar("c") → freereg_=base+3
  （此时 nactvar_ 还没动，变量只是预登记，不受保护）

Step 2: freereg_ = base    ← 重新把游标拨回 base，准备编译右值
  （关键：让右值表达式从 R(base) 开始分配，直接落到局部目标区域）

Step 3: 编译右值表达式并 discharge 到 R(base), R(base+1), ...

Step 4: freereg_ = savedFreereg   ← 恢复为外层保存的游标

Step 5: adjustLocalVars(nvars)    → nactvar_ 向前跳 nvars，freereg_ = nactvar_
```

### 7.1 为什么要先 `addLocalVar` 再退回？

`addLocalVar()` 的作用是把名字和寄存器号登记进 `localVars_`，以便后续名字解析时能找到它。

但登记完之后如果不退回 `freereg_`，右值表达式就会从 `base + nvars` 开始分配临时寄存器，这意味着它算出来的结果在"局部变量区之后"，还需要额外一轮 MOVE 搬回来——效率低，且复杂度高。

退回 `freereg_ = base` 之后，右值表达式自然从 `base` 开始往后落，直接对齐之后的局部目标槽位。

### 7.2 为什么最后是先 `freereg_ = savedFreereg` 再 `adjustLocalVars`？

`adjustLocalVars(nvars)` 会执行 `freereg_ = nactvar_`，而 `nactvar_` 因为新增了 nvars 个变量，会跳到 `base + nvars`，这个值有可能比 `savedFreereg` 更低。

如果 `savedFreereg` 代表着"外层调用占用的更高寄存器区"，那直接 `adjustLocalVars` 会把游标错误地拉低。

不过实际上，`freereg_ = savedFreereg` 之后，`adjustLocalVars` 中的 `freereg_ = nactvar_` 又会再次覆盖——所以最终 `freereg_` 等于 `nactvar_`（即 `base + nvars`）。

这里的 `freereg_ = savedFreereg` 主要是一个防御动作，确保即使 `hasLastArrayExpr` 之类的特殊分支执行后，外层状态仍然一致。

### 7.3 图示：`local a, b = f()` 中 `freereg_` 的轨迹

假设进入 `LocalStmt` 前 `nactvar_ = 3`，`freereg_ = 3`。

```text
saved = 3
base  = 3

Step 1: addLocalVar("a") → freereg_=4
        addLocalVar("b") → freereg_=5

Step 2: freereg_ = 3               ← 退回 base

Step 3: 编译 f()
        - 函数表达式 f 解析 → Global，还没指令
        - 某个临时寄存器用于 GETGLOBAL：下文会在 R(3) 生成
        - CALL R(3) 1 3             ← 期望 2 个返回值
        - freereg_ 经过 CallExpr 内部逻辑后，最终落在合理区间

Step 4: 回补 CALL 的 C 参数为 3（2 个返回值）
        freereg_ = saved = 3

Step 5: adjustLocalVars(2) → nactvar_=5, freereg_=5
```

---

## 8. `ReturnStmt`：局部临时区归零后生成返回值

```cpp
void CodeGenerator::emitStmt(const ReturnStmt& s) {
    i32 base = nactvar_;
    i32 savedFreereg = freereg_;
    freereg_ = base;
    checkStack(nret);
    for (i32 i = 0; i < nret; i++) {
        ExprDesc val;
        expr(*s.values[i], val);
        discharge(val, base + i);
    }
    codeABC(OpCode::RETURN, base, nret + 1, 0);
    freereg_ = savedFreereg;
}
```

模式和 `LocalStmt` 类似：把 `freereg_` 压到 `base = nactvar_`，在那之后生成返回值，发指令后再恢复。

原因：返回值是一组临时槽位，应从 `nactvar_` 后开始排，而不能和局部变量混在一起。

---

## 9. `CallExpr`：嵌套调用下最复杂的保护策略

函数调用是整个生成器里 `freereg_` 更改最多、逻辑最精细的地方。

完整的变化过程如下：

```text
进入 emitExpr(CallExpr):
  savedFreeReg = freereg_

1. 计算函数表达式 → 得到 base（函数所在寄存器）

2. 安全检查：
   if base < savedFreeReg:
     把函数（和可能的 self）搬到 savedFreeReg 开始的区域
     base = savedFreeReg

3. firstArgReg = hasImplicitSelf ? base+2 : base+1
   freereg_ = firstArgReg           ← 对齐参数起点
   checkStack(explicitArgCount)     ← 预留参数区

4. 逐个生成参数:
   for 每个 arg:
     discharge(arg, firstArgReg + i)
     if freereg_ < firstArgReg + i + 1:
       freereg_ = firstArgReg + i + 1    ← 锁住已写入区域

5. 发 CALL base (nargs+1) 2

6. freereg_ = max(savedFreeReg, base+1)  ← 恢复到外层水位
   checkStack(0)
```

### 9.1 为什么 Step 2 要把函数"搬高"

考虑 `f(g())` 的情形：

```text
编译 g():
  g 在 R(0)，经过 GETGLOBAL 之类当前 base = 0
  参数区从 R(1) 开始
  调用结束：freereg_ = max(savedFreeReg, 1) = 1

编译外层 f():
  f 在 R(0)，但 R(0) 现在被内层 g 的 base 占据！
  更糟的是，内层 g 还需要 R(1) 作为返回值
  如果直接在 R(0) 发 CALL，VM 会错误读取调用帧
```

Step 2 的"搬高"策略就是为了解决这个冲突：一旦发现 `base < savedFreeReg`，就把整个函数调用帧向高寄存器区搬移，让它不会和"外层已保留区"发生踩踏。

### 9.2 为什么 Step 4 要逐步锁住 `freereg_`

在逐一生成参数的过程中，如果某个参数本身也是函数调用，那它的 `base` 会从当前 `freereg_` 开始分配。如果前面的参数寄存器没有被"锁住"，后续嵌套调用可能会覆盖前面参数的内容。

```text
print(type(print))
          ↑
      这个 print 是参数，需要先算 type(print)
      type(print) 是函数调用，会使用临时寄存器
      如果 freereg_ 没有被锁到"第一个参数之后"，
      type(print) 的调用帧就可能把 print 这个参数的结果覆盖掉
```

逐步 `freereg_ = targetReg + 1` 的作用就是：确保每个参数写完之后，它所在的寄存器不会再被下一个参数求值时的嵌套调用覆盖。

### 9.3 Step 6 恢复的精确逻辑

```cpp
freereg_ = (savedFreeReg > (base + 1)) ? savedFreeReg : (base + 1);
```

解读：

- `base + 1`：调用结束后，`R(base)` 存放返回值，`freereg_` 至少要在 `base+1`
- `savedFreeReg`：如果外层上下文已经保留了更高的寄存器区，不能把游标回退到更低的位置，否则外层已分配的槽位会被下一次 `allocReg()` 重复使用

取两者最大值，确保两个约束都不被破坏。

---

## 10. `CallStmt`：语句级调用后硬回收

```cpp
void CodeGenerator::emitStmt(const CallStmt& s) {
    ExprDesc desc;
    expr(*s.call, desc);
    // ... 把 CALL 指令的 C 改为 1（丢弃返回值）
    freeReg(desc.u.s.info);

    freereg_ = nactvar_;   // ← 硬回收
}
```

语句级调用结束后，所有临时寄存器都已无意义，所以直接 `freereg_ = nactvar_`。

这比 `CallExpr` 的"保留外层水位"逻辑更激进，因为这里没有"外层需要消费返回值"的需求。

---

## 11. `luaK_self()`：一次性占两个连续寄存器

```cpp
void CodeGenerator::luaK_self(ExprDesc& e, ExprDesc& key) {
    exp2AnyReg(e);
    if (e.kind == ExprKind::NonRelocatable)
        freeReg(e.u.s.info);

    i32 func = freereg_;
    freereg_ += 2;          // ← 手动占两个
    if (freereg_ > proto_->getMaxStackSize())
        proto_->setMaxStackSize(freereg_);

    codeABC(OpCode::SELF, func, e.u.s.info, exp2RK(key));
    ...
    e.u.s.info = func;
}
```

`SELF A B C` 约定：`R(A)` 放函数，`R(A+1)` 放 `self`——两个寄存器必须连续。

所以这里不走 `allocReg()` 两次（因为两次 `allocReg` 之间如果有逃逸代码插入，就可能打断连续性），而是直接 `freereg_ += 2`，一次性锁住两个槽位，再手动更新 `maxStackSize`。

---

## 12. 表构造器：哈希字段 vs 数组字段的不同策略

### 12.1 哈希字段：每次 save/restore

```cpp
i32 savedFreereg = freereg_;
// 计算 key / value
codeABC(OpCode::SETTABLE, ...);
freereg_ = savedFreereg;
checkStack(0);
```

哈希字段 key 和 value 是完全临时的，用完即丢，所以每次都完整 save/restore。

### 12.2 数组字段：累积在 `tableReg+1` 之后

```text
tableReg   = R(tableReg)     ← 表对象
tableReg+1 = R(tableReg+1)  ← 第 1 个数组元素
tableReg+2 = R(tableReg+2)  ← 第 2 个数组元素
...
```

数组字段通过 `exp2NextReg()` 依次推入，每个元素顺序堆在 `tableReg` 之后，直到 `SETLIST` 批量写入或到末尾。

批量写完之后：

```cpp
freereg_ = tableReg + 1;
checkStack(0);
```

把游标收到 `tableReg+1`，释放临时堆积的元素区域。

### 12.3 最后一个数组字段为多返回值时的特殊协调

此时会提前设置 `forcedCallBase_ = tableReg + tostore`，强制内层 `CallExpr` 把调用基址对齐到这个位置。发完 `SETLIST` 后同样把 `freereg_` 拉回 `tableReg + 1`。

---

## 13. `for` 循环：显式指定循环控制寄存器

### 13.1 数值 `for`

```text
base = freereg_             ← 记录循环基址

exp2NextReg(init)           → R(base)    freereg_=base+1
exp2NextReg(limit)          → R(base+1)  freereg_=base+2
exp2NextReg(step)           → R(base+2)  freereg_=base+3

freereg_ = base             ← 退回基址，让 addLocalVar 重新从 base 分配
addLocalVar("(for index)")  → R(base)    freereg_=base+1
addLocalVar("(for limit)")  → R(base+1)  freereg_=base+2
addLocalVar("(for step)")   → R(base+2)  freereg_=base+3
addLocalVar(s.var)          → R(base+3)  freereg_=base+4

adjustLocalVars(4)          → nactvar_+=4, freereg_=nactvar_

freereg_ = base + 4         ← 显式保证，防止 adjustLocalVars 拉低
checkStack(0)
```

退回再前进的目的：`exp2NextReg` 已经把 init/limit/step 真正写到寄存器里，但 `addLocalVar` 需要从 `freereg_` 处登记——如果不退，登记的寄存器号就会从 `base+3` 开始，而不是和实际内容对齐的 `base`。

### 13.2 泛型 `for in`

```text
base = freereg_

discharge(iterCall, base)   ← 把迭代器函数调用结果落到 R(base)
freereg_ = base + 3        ← 手动预留 3 个控制变量槽（func/state/ctrl）

freereg_ = base            ← 退回，让 addLocalVar 从 base 重新登记
addLocalVar("(for generator)")  → R(base)
addLocalVar("(for state)")      → R(base+1)
addLocalVar("(for control)")    → R(base+2)
addLocalVar(s.vars[0])          → R(base+3)
... 更多循环变量 ...

adjustLocalVars(3 + nvars)  → nactvar_ 推进

freereg_ = base + 3 + nvars ← 固定到循环变量之后，循环体临时区从这里开始
checkStack(0)
```

---

## 14. `leaveBlock()`：块退出的最终收尾

```cpp
void CodeGenerator::leaveBlock() {
    BlockInfo* bl = currentBlock_;
    currentBlock_ = bl->previous;
    removeLocalVars(bl->nactvar);
    freereg_ = nactvar_;
    checkStack(0);
    patchtohere(bl->breaklist);
    delete bl;
}
```

`removeLocalVars()` 已经把 `freereg_` = `nactvar_` 了，这里的 `freereg_ = nactvar_` 是双重保险。

实际效果是：离开任何块之后，`freereg_` 一定等于 `nactvar_`，临时区被完整清空。

---

## 15. 子函数的 `freereg_` 独立不共享

```cpp
CodeGenerator child(pool_);
child.parent_ = this;
child.freereg_ = 0;   // ← 从 0 开始，与父函数完全独立
```

每次编译子函数（`compileFunction()`）都用独立的 `CodeGenerator` 对象，有独立的 `freereg_`，寄存器空间互不干扰。

只有 upvalue 解析通过 `parent_` 指针跨越边界。

编译完子函数后，把子 `freereg_` 作为 `maxStackSize` 的下界写回：

```cpp
if (child.freereg_ > newProto->getMaxStackSize())
    newProto->setMaxStackSize(child.freereg_);
```

---

## 16. `exp2NextReg()`：最后一次机会的优化

```cpp
void CodeGenerator::exp2NextReg(ExprDesc& desc) {
    exp2Val(desc);

    if (desc.kind == ExprKind::NonRelocatable &&
        desc.u.s.info == freereg_ - 1) {
        return;   // ← 已经在正确位置，不需要分配新寄存器
    }

    i32 reg = allocReg();
    discharge(desc, reg);
}
```

如果表达式已经是 `NonRelocatable`，且正好在 `freereg_ - 1`，就说明它已经是"最近分配的那个槽位"，不需要再额外搬移。

这个优化在"参数和返回值都需要按顺序排列的上下文"（如局部初始化、表构造器数组部分）里非常有效，避免了大量多余的 `MOVE`。

---

## 17. `freereg_` 手动赋值全景表

以下汇总所有直接赋值 `freereg_ = X` 的位置，方便定位时快速找到对应上下文：

| 位置 | 表达式 | 含义 |
|------|--------|------|
| `CodeGenerator` 构造 / `generate()` | `= 0` | 函数初始化 |
| `compileFunction()` 子生成器初始化 | `= 0` | 子函数独立起点 |
| `adjustLocalVars()` | `= nactvar_` | 对齐到局部变量水位 |
| `removeLocalVars()` | `= nactvar_` | 作用域退出收缩 |
| `leaveBlock()` | `= nactvar_` | 双重保险 |
| `CallStmt` 结束 | `= nactvar_` | 语句级调用后硬回收 |
| `LocalStmt` Step 2 | `= base` | 退回让右值从目标区开始 |
| `LocalStmt` Step 2'（addLocalVar 前） | `= base` | 再次退回让预分配从 base |
| `LocalStmt` restore | `= savedFreereg` | 恢复外层水位 |
| `ReturnStmt` 初始化 | `= base(=nactvar_)` | 从局部区之后排返回值 |
| `ReturnStmt` restore | `= savedFreereg` | 恢复 |
| `CallExpr` 参数起点 | `= firstArgReg` | 对齐到参数起点 |
| `CallExpr` 参数锁住 | `= targetReg+1` | 锁住已写入参数区域 |
| `CallExpr` restore | `= max(saved, base+1)` | 保留返回值且不破坏外层 |
| `luaK_self()` | `+= 2` | 手动占两个连续槽位 |
| `ForNumStmt` 退回 | `= base` | 让 addLocalVar 从 base 登记 |
| `ForNumStmt` 固定 | `= base + 4` | 显式撑开到循环变量区之后 |
| `ForInStmt` 预留 | `= base + 3` | 手动预留 3 个控制槽 |
| `ForInStmt` 退回 | `= base` | 让 addLocalVar 重新登记 |
| `ForInStmt` 固定 | `= base + 3 + nvars` | 撑开到循环变量区之后 |
| `TableExpr` 哈希字段 restore | `= savedFreereg` | 每字段完整恢复 |
| `TableExpr` SETLIST 后 | `= tableReg + 1` | 释放数组临时堆积区 |
| `FunctionStmt` restore | `= savedFreereg` | 函数语句编译完恢复 |

---

## 18. 什么情况下 bug 会从 `freereg_` 走出来

结合以上分析，可以归纳几类典型 bug 模式：

### 18.1 Save 了但没 Restore

某段路径提前 `return` 或抛异常，skip 了 `freereg_ = savedFreereg`。

后果：`freereg_` 偏高，之后的某些 `allocReg()` 当回收了实际上还需要的寄存器。

### 18.2 在应该退回 `base` 之前多分配了一个临时寄存器

最常见于局部初始化和 for 循环的"登记 + 生成"两步之间。

后果：`addLocalVar()` 的寄存器号和实际存放值的寄存器号错位，名字解析查到的寄存器取到了错误的值。

### 18.3 CallExpr 没有正确锁参数区

某个参数算完后没把 `freereg_` 推进，下一个参数（若是函数调用）就在"未锁定区"分配了调用帧，覆盖了前一个参数。

### 18.4 离开块之后没有正确收缩 `freereg_`

临时寄存器在下一条语句仍被当成"已用"，后续分配只能继续往高处走，资源持续泄漏，最终 `maxStackSize` 超限。

### 18.5 直接改 `freereg_` 后忘了 `checkStack(0)`

`freereg_` 比上次 `maxStackSize` 还要低的话没问题，但一旦有某路径把游标推高，不更新 `maxStackSize` 就会导致 VM 在运行期写越界。

---

## 19. 一句话总结

`freereg_` 的所有变化只干三件事：

1. **推进**：分配新临时或新局部变量
2. **收缩**：释放临时区或退出作用域
3. **重定向**：把游标临时拨回某个基址，让接下来的分配落到指定区域

所有复杂的 Save/Restore、显式赋值、锁定等操作，都是在这三件事的基础上，针对不同语言结构（调用、多赋值、for 循环、表构造器）做的精确控制。

只要每次修改都能回答清楚"我在做三件事里的哪一件、出于什么原因"，就能准确判断一处 `freereg_` 的改动是否正确。

---

## 20. 手把手调试示例

本节用最简单的 Lua 代码，逐步追踪 `freereg_`（以及 `nactvar_`）在每一条生成指令前后的精确数值，帮助在调试器里设断点核对状态。

> 约定：每行格式为  
> `[事件]  nactvar_=X  freereg_=Y   → 发出指令（若有）`

---

### 20.1 最简示例：`local x = 1`

```lua
local x = 1
```

这是寄存器模型最小化的完整走一遍。

```text
── 进入 emitStmt(LocalStmt) ──
  saved   = freereg_ = 0
  base    = nactvar_ = 0

[addLocalVar("x")]    nactvar_=0  freereg_=1
                      →（仅登记，还没激活）

[重置游标到 base]      nactvar_=0  freereg_=0

[编译右值 1]
  ExprKind=IntLiteral，不占寄存器，ExprDesc.u.ival=1

[discharge 到 R(0)]   nactvar_=0  freereg_=0
                      → LOADK  R(0)  <const 1>
                      freereg_ 不变（discharge 不改游标）

[restore]             nactvar_=0  freereg_=saved=0

[adjustLocalVars(1)]  → nactvar_=1  freereg_=1   checkStack(0)

── 离开 LocalStmt ──
最终状态: nactvar_=1  freereg_=1

寄存器布局:
 R(0)=x
      ↑
  freereg_=1
```

**常见断点位置**：`adjustLocalVars()` 入口，进入前 `nactvar_=0 freereg_=0`，退出后 `nactvar_=1 freereg_=1`。

---

### 20.2 两个局部变量：`local a, b = 10, 20`

```lua
local a, b = 10, 20
```

```text
── 进入 emitStmt(LocalStmt) ──
  saved   = freereg_ = 0
  base    = nactvar_ = 0

[addLocalVar("a")]    nactvar_=0  freereg_=1
[addLocalVar("b")]    nactvar_=0  freereg_=2

[重置游标到 base]      nactvar_=0  freereg_=0

[编译右值 10，discharge 到 R(0)]
                      → LOADK  R(0)  <const 10>
[编译右值 20，discharge 到 R(1)]
                      → LOADK  R(1)  <const 20>

[restore]             nactvar_=0  freereg_=saved=0

[adjustLocalVars(2)]  → nactvar_=2  freereg_=2   checkStack(0)

── 离开 LocalStmt ──
最终状态: nactvar_=2  freereg_=2

寄存器布局:
 R(0)=a   R(1)=b
               ↑
           freereg_=2
```

---

### 20.3 简单函数调用语句：`print("hello")`

```lua
print("hello")
```

这是 `CallStmt` + `CallExpr` 的组合。假设初始 `nactvar_=0 freereg_=0`。

```text
── 进入 emitStmt(CallStmt) ──

── 进入 emitExpr(CallExpr) ──
  savedFreeReg = freereg_ = 0

[解析函数名 print → Global]
  exp2AnyReg → GETGLOBAL  R(0)  <"print">
  allocReg() → freereg_=1，函数 base=0

[base=0 >= savedFreeReg=0，无需搬移]

[firstArgReg = base+1 = 1]
  freereg_ = firstArgReg = 1
  checkStack(1)              ← 预留 1 个参数槽

[编译参数 "hello"]
  ExprKind=StringLiteral，discharge 到 R(1)
                      → LOADK  R(1)  <"hello">
  freereg_ = max(freereg_, 1+1) = 2   ← 锁住参数区

[发 CALL]             → CALL   R(0)  2  2
  （2 个参数包含函数本身+1个实参，期望1个返回值）

[restore]
  freereg_ = max(savedFreeReg=0, base+1=1) = 1

── 离开 emitExpr(CallExpr) ──

[CallStmt 硬回收]
  freereg_ = nactvar_ = 0

── 离开 emitStmt(CallStmt) ──
最终状态: nactvar_=0  freereg_=0
```

可以看到：调用完成后，`freereg_` 被彻底归零，寄存器被完整回收。

---

### 20.4 嵌套调用：`print(type(1))`

```lua
print(type(1))
```

这是最典型的"需要保护外层参数区"的场景。

```text
── 进入外层 CallExpr（print） ──
  savedFreeReg_outer = 0

[解析 print → GETGLOBAL R(0)]   freereg_=1，base_outer=0
[firstArgReg_outer = 1]
  freereg_ = 1，checkStack(1)

── 需要编译第 1 个参数：type(1)（又是一个 CallExpr）──

  ── 进入内层 CallExpr（type） ──
    savedFreeReg_inner = freereg_ = 1

  [解析 type → GETGLOBAL R(1)]  freereg_=2，base_inner=1

  [base_inner=1 >= savedFreeReg_inner=1，无需搬移]

  [firstArgReg_inner = 2]
    freereg_ = 2，checkStack(1)

  [编译参数 1 → discharge R(2)]
                        → LOADK  R(2)  <const 1>
    freereg_ = 3        ← 锁住内层参数区

  [发 CALL]             → CALL   R(1)  2  2
  [restore inner]
    freereg_ = max(saved=1, base_inner+1=2) = 2

  ── 离开内层 CallExpr──
  结果落在 R(1)，kind=NonRelocatable，info=1

[外层参数 0 写入 R(1)：已在正确位置]
  freereg_ = max(freereg_, 1+1) = 2  ← 锁住外层参数区

[发 CALL]               → CALL   R(0)  2  2
[restore outer]
  freereg_ = max(saved=0, base_outer+1=1) = 1

── 离开外层 CallExpr ──

[CallStmt 硬回收]
  freereg_ = nactvar_ = 0

最终寄存器使用峰值：freereg_ 最高到达 3 → maxStackSize ≥ 3

指令序列:
  GETGLOBAL  R(0)  "print"
  GETGLOBAL  R(1)  "type"
  LOADK      R(2)  1
  CALL       R(1)  2  2
  CALL       R(0)  2  1
```

---

### 20.5 局部变量 + 临时表达式：`local x = 1 + 2`

```lua
local x = 1 + 2
```

二元运算需要占用临时寄存器。

```text
── 进入 LocalStmt ──
  saved=0  base=nactvar_=0

[addLocalVar("x")]    freereg_=1
[重置游标]            freereg_=0

── 编译右值 1+2（BinaryExpr，OpAdd）──

  [编译左操作数 1]
    ExprKind=IntLiteral，暂不分配寄存器

  [exp2RK(1)]
    → 常量 1 进入常量表 K[0]，返回 RK = 256+0 = 256（RK索引，非寄存器）

  [编译右操作数 2]
    ExprKind=IntLiteral

  [exp2RK(2)]
    → 常量 2 进入常量表 K[1]，返回 RK = 257

  [目标是 R(0)（就是局部变量 x 的槽位）]
  [discharge BinaryExpr 到 R(0)]
                      → ADD  R(0)  K[0]  K[1]
    freereg_ 不因 discharge 本身改变

[restore]             freereg_=saved=0
[adjustLocalVars(1)]  nactvar_=1  freereg_=1

最终状态: nactvar_=1  freereg_=1
指令: ADD  R(0)  K[0]  K[1]    （其中 K[0]=1，K[1]=2）
```

注意：两个整数字面量都走了 RK 路径（常量表 + 256 偏移），没有占用任何临时寄存器，`freereg_` 在右值编译阶段始终为 0。

---

### 20.6 数值 for 循环：`for i = 1, 3 do end`

```lua
for i = 1, 3 do
end
```

```text
── 进入 ForNumStmt ──
  nactvar_=0  freereg_=0
  base = freereg_ = 0

[exp2NextReg(init=1)]   → LOADK  R(0)  <1>    freereg_=1
[exp2NextReg(limit=3)]  → LOADK  R(1)  <3>    freereg_=2
[exp2NextReg(step=1)]   → LOADK  R(2)  <1>    freereg_=3
                          （step 默认为 1）

[发 FORPREP]            → FORPREP  R(0)  <循环体偏移>
  （FORPREP 负责做首次 init 操作并跳到循环末尾测试）

[退回游标]              freereg_ = base = 0

[addLocalVar("(for index)")]  freereg_=1  → R(0)
[addLocalVar("(for limit)")]  freereg_=2  → R(1)
[addLocalVar("(for step)")]   freereg_=3  → R(2)
[addLocalVar("i")]            freereg_=4  → R(3)

[adjustLocalVars(4)]  nactvar_=4  freereg_=4
[强制固定]            freereg_ = base+4 = 4   checkStack(0)

   ↑ R(0..2) 是隐藏控制变量，R(3) 是用户变量 i

── 编译循环体（空） ──

[发 FORLOOP]            → FORLOOP  R(0)  <回跳偏移>

── 离开 ForNum 块，leaveBlock() ──
  removeLocalVars(0)   → nactvar_=0  freereg_=0

最终状态: nactvar_=0  freereg_=0

寄存器使用峰值：4（maxStackSize ≥ 4）
```

注意：R(0/1/2) 里存着 init/limit/step 的实际值，R(3) 才是 Lua 代码里 `i` 的寄存器。FORLOOP 每次迭代会把 R(0) += R(2)，然后把 R(3) = R(0)。

---

### 20.7 带局部变量的函数体：`local a = 1; local b = a + 1`

```lua
local a = 1
local b = a + 1
```

演示多条语句之间 `freereg_` 的延续。

```text
── 语句 1：local a = 1 ──
  saved=0  base=0
  addLocalVar("a") → freereg_=1
  重置 freereg_=0
  discharge(1) → LOADK R(0) <1>
  restore → freereg_=0
  adjustLocalVars(1) → nactvar_=1  freereg_=1

状态: nactvar_=1  freereg_=1

── 语句 2：local b = a + 1 ──
  saved=1  base=nactvar_=1
  addLocalVar("b") → freereg_=2
  重置 freereg_=1

  编译右值 a+1：
    [a] → 名字解析，找到局部变量 R(0)，ExprKind=Local，info=0
    exp2RK(a)：Local 且 info=0 < nactvar_=1 → 直接返回 RK=0（寄存器引用）
    [1] → exp2RK(1)：常量 → K[0]=1，RK=256

    目标 R(1)（b 的槽位）
    → ADD  R(1)  R(0)  K[0]

  restore → freereg_=saved=1
  adjustLocalVars(1) → nactvar_=2  freereg_=2

最终状态: nactvar_=2  freereg_=2

寄存器布局:
 R(0)=a   R(1)=b
               ↑
           freereg_=2

指令序列:
  LOADK  R(0)  <const 1>     ; a = 1
  ADD    R(1)  R(0)  K[0]    ; b = a + 1 （K[0]=1）
```

---

### 20.8 表构造器：`local t = {10, 20, x = 3}`

```lua
local t = {10, 20, x = 3}
```

同时有数组字段和哈希字段的混合表。

```text
── 进入 LocalStmt ──
  saved=0  base=nactvar_=0

[addLocalVar("t")]  → freereg_=1
[重置]              → freereg_=0

── 编译右值 TableExpr ──

  [NEWTABLE 指令]  → NEWTABLE  R(0)  2  1
                     （2 个数组字段，1 个哈希字段）
    freereg_ = 1    （tableReg=0，表已分配）

  ── 数组字段 10 ──
    exp2NextReg(10) → LOADK R(1) <10>   freereg_=2
  ── 数组字段 20 ──
    exp2NextReg(20) → LOADK R(2) <20>   freereg_=3

  [SETLIST]        → SETLIST R(0) 2 1
                     （把 R(1..2) 的 2 个值写入表索引 1..2）
    freereg_ = tableReg+1 = 1   ← 释放数组临时区

  ── 哈希字段 x=3 ──
    savedH = freereg_ = 1

    [exp2RK("x")]  → 字符串常量 K[1]="x"，RK=257
    [exp2RK(3)]    → 数字常量 K[2]=3，RK=258

    → SETTABLE  R(0)  K[1]  K[2]

    freereg_ = savedH = 1       ← 哈希字段完整恢复

── 离开 TableExpr ──，结果 ExprKind=NonRelocatable，info=0

[restore]         freereg_=saved=0
[adjustLocalVars] nactvar_=1  freereg_=1

最终状态: nactvar_=1  freereg_=1

寄存器使用峰值：3（数组字段推入时峰值）

指令序列:
  NEWTABLE  R(0)  2  1
  LOADK     R(1)  <10>
  LOADK     R(2)  <20>
  SETLIST   R(0)  2  1
  SETTABLE  R(0)  K["x"]  K[3]
```

---

### 20.9 调试技巧速查

在 CodeGenerator 相关代码里打断点时，以下几个位置覆盖了 `freereg_` 的绝大多数变化：

| 断点位置 | 能观察到的事件 |
|----------|---------------|
| `allocReg()` 入口 | 每次临时寄存器分配 |
| `freeReg()` 入口 | 每次临时寄存器释放 |
| `adjustLocalVars()` 入口/出口 | 局部变量激活、`nactvar_` 向前跳 |
| `removeLocalVars()` 入口/出口 | 作用域退出、寄存器回收 |
| `emitExpr(CallExpr)` 中 `freereg_ = firstArgReg` | 参数区起点对齐 |
| `emitExpr(CallExpr)` 中 restore | 调用后游标恢复 |
| `leaveBlock()` 末尾 | 块退出后最终水位 |

最有用的调试方式：在 `allocReg()` 或任意 `freereg_ =` 赋值处打条件断点，打印调用栈，就能快速定位是哪条 Lua 语句触发了寄存器变化。
