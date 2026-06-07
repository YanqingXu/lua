# Expression Emission — 表达式字节码发射

## 1. 这个模块解决什么问题？

各种 Lua 表达式如何被翻译为字节码指令序列。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/compiler/codegen/expression_emitter.cpp` | 表达式 → 指令 |

## 3. 各表达式类型的发射策略

### 字面量
```lua
nil       → LOADNIL  R(A)
true      → LOADBOOL R(A) 1 0
false     → LOADBOOL R(A) 0 0
42        → LOADK    R(A) K(42)
"hello"   → LOADK    R(A) K("hello")
```

### 变量引用
```lua
local x   → MOVE R(A) R(x_reg)
global y  → GETGLOBAL R(A) K("y")
upvalue z → GETUPVAL R(A) upvalueIdx
```

### 二元运算
```lua
a + b     → ADD R(A) RK(a) RK(b)
a - b     → SUB R(A) RK(a) RK(b)
a * b     → MUL R(A) RK(a) RK(b)
a / b     → DIV R(A) RK(a) RK(b)
a % b     → MOD R(A) RK(a) RK(b)
a ^ b     → POW R(A) RK(a) RK(b)
a .. b    → CONCAT R(A) R(a) R(b)  -- 连续拼接可合并
a and b   → TEST + JMP 组合
a or b    → TEST + JMP 组合
```

### 一元运算
```lua
-a        → UNM R(A) R(a)
not a     → NOT R(A) R(a)
#a        → LEN R(A) R(a)
```

### 比较运算（短路逻辑）
```lua
-- a and b 的字节码:
TEST R(a) 0       -- if not a then skip to JMP
JMP → after       -- skip value b
MOVE R(result) R(b)

-- a or b 的字节码:
TEST R(a) 1       -- if a then skip to JMP
JMP → after
MOVE R(result) R(b)
```

### 函数调用
```lua
f(a, b)  → 先将 f, a, b 放入连续寄存器
           CALL R(f) B C
              A: f 的寄存器位置
              B: 参数个数
              C: 期望返回值数 - 1

print("hello") → R(0)=print, R(1)="hello"
                 CALL R(0) 1 1
```

### 表构造器
```lua
{1, 2, key="val"}
  → NEWTABLE R(A) arraySize hashSize
  → SETLIST (批量填充数组部分)
  → SETTABLE (逐一设置哈希部分)
```

### 闭包创建
```lua
function(x) return x end
  → 编译子函数 proto
  → CLOSURE R(A) subProtoIdx
  → (绑定 upvalue)
```
