# Register Model — 寄存器模型

## 1. 这个模块解决什么问题？

Lua VM 使用基于栈的寄存器模型。理解寄存器和栈的关系对理解 VM 至关重要。

## 2. 核心概念

```
Lua 的"寄存器"不是在 CPU 中，而是在栈上。

R(i) = stack[base + i]

即: 寄存器索引 i 对应栈上 base 偏移 i 的位置。
```

## 3. 栈 = 寄存器文件

```
Stack (物理):
  +0: [值]  ← R(0) = base+0
  +1: [值]  ← R(1) = base+1
  +2: [值]  ← R(2) = base+2
  +3: [值]  ← R(3) = base+3
  ...

当前帧的 base 指针决定了寄存器在栈上的起始位置。
```

## 4. 寄存器访问函数

```cpp
// R(A) — 纯寄存器访问
Value& R(i32 index) {
    return base[index];
}

// RK(B) — 寄存器或常量
Value RK(i32 rk) {
    if (ISK(rk)) {
        // rk >= 256: 常量索引
        return constants[INDEXK(rk)];
    } else {
        // rk < 256: 寄存器
        return base[rk];
    }
}

// K(Bx) — 纯常量访问
Value& K(i32 index) {
    return constants[index];
}
```

## 5. 寄存器用途分类

```
R(0) ~ R(numParams-1)  : 函数参数
R(numParams) ~ R(maxStack-1) : 局部变量 + 临时值
```

## 6. 示例：寄存器分配

```lua
function add(a, b)
    local c = a + b
    return c * 2
end
```

```
寄存器和参数映射:
  R(0) = a      (参数1)
  R(1) = b      (参数2)
  R(2) = a + b  (临时: 加法结果)
  R(2) = c      (local c: 复用 R(2))
  R(3) = c * 2  (临时: 乘法结果 — 或复用 R(2))
```

## 7. 参数布局

```
函数调用时的栈布局:

  [func]          ← R(-1) 或 ci.func
  [arg1]          ← R(0)  参数1
  [arg2]          ← R(1)  参数2
  [arg3]          ← R(2)  参数3
  ...
  
  base 指向 arg1 (ci.base)
```

## 8. 寄存器数量限制

```
A 字段: 8 bits → 最多 256 个寄存器
maxStackSize: 编译时确定的最大寄存器数
```
