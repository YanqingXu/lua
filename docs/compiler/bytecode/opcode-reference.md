# Opcode Reference — 指令集参考

## 1. 这个模块解决什么问题？

完整的 38 条 Lua 5.1 指令参考，每条指令的语义、格式、伪代码。

## 2. 指令格式

```
32位指令，三种格式：

iABC:  [OP:6][A:8][C:9][B:9]
iABx:  [OP:6][A:8][Bx:18]
iAsBx: [OP:6][A:8][sBx:18] (sBx 有符号)
```

## 3. 指令一览

### 数据移动 (Data Move)

| 指令 | 格式 | 伪代码 | 说明 |
|------|------|--------|------|
| **MOVE** | iABC | `R(A) := R(B)` | 寄存器拷贝 |
| **LOADK** | iABx | `R(A) := K(Bx)` | 加载常量 |
| **LOADBOOL** | iABC | `R(A) := (Bool)B; if (C) pc++` | 加载布尔值 |
| **LOADNIL** | iABC | `R(A), ..., R(B) := nil` | 批量置 nil |

### 变量访问 (Variable Access)

| 指令 | 格式 | 伪代码 | 说明 |
|------|------|--------|------|
| **GETGLOBAL** | iABx | `R(A) := Gbl[K(Bx)]` | 读全局变量 |
| **SETGLOBAL** | iABx | `Gbl[K(Bx)] := R(A)` | 写全局变量 |
| **GETUPVAL** | iABC | `R(A) := UpValue[B]` | 读 upvalue |
| **SETUPVAL** | iABC | `UpValue[B] := R(A)` | 写 upvalue |
| **GETTABLE** | iABC | `R(A) := R(B)[RK(C)]` | 读表 |
| **SETTABLE** | iABC | `R(A)[RK(B)] := RK(C)` | 写表 |

### 表操作 (Table)

| 指令 | 格式 | 伪代码 | 说明 |
|------|------|--------|------|
| **NEWTABLE** | iABC | `R(A) := {} (size=B,C)` | 创建表 |
| **SELF** | iABC | `R(A+1):=R(B); R(A):=R(B)[RK(C)]` | 方法调用准备 |
| **SETLIST** | iABC | 批量初始化数组部分 | 表构造器的数组填充 |

### 算术运算 (Arithmetic)

| 指令 | 格式 | 伪代码 | 可能触发元方法 |
|------|------|--------|-------------|
| **ADD** | iABC | `R(A) := RK(B) + RK(C)` | `__add` |
| **SUB** | iABC | `R(A) := RK(B) - RK(C)` | `__sub` |
| **MUL** | iABC | `R(A) := RK(B) * RK(C)` | `__mul` |
| **DIV** | iABC | `R(A) := RK(B) / RK(C)` | `__div` |
| **MOD** | iABC | `R(A) := RK(B) % RK(C)` | `__mod` |
| **POW** | iABC | `R(A) := RK(B) ^ RK(C)` | `__pow` |

### 一元运算 (Unary)

| 指令 | 格式 | 伪代码 | 可能触发元方法 |
|------|------|--------|-------------|
| **UNM** | iABC | `R(A) := -R(B)` | `__unm` |
| **NOT** | iABC | `R(A) := not R(B)` | — |
| **LEN** | iABC | `R(A) := #R(B)` | `__len` |
| **CONCAT** | iABC | `R(A) := R(B).. ... ..R(C)` | `__concat` |

### 控制流 / 比较 (Branch / Comparison)

| 指令 | 格式 | 伪代码 | 说明 |
|------|------|--------|------|
| **JMP** | iAsBx | `pc += sBx` | 无条件跳转 |
| **EQ** | iABC | `if (RK(B)==RK(C)) ~= A then pc++` | 相等比较 |
| **LT** | iABC | `if (RK(B)<RK(C)) ~= A then pc++` | 小于比较 |
| **LE** | iABC | `if (RK(B)<=RK(C)) ~= A then pc++` | 小于等于 |
| **TEST** | iABC | `if not (R(A) <=> C) then pc++` | 条件测试 |
| **TESTSET** | iABC | `if (R(B) <=> C) then R(A):=R(B) else pc++` | 测试并赋值 |

### 函数调用 (Call)

| 指令 | 格式 | 伪代码 | 说明 |
|------|------|--------|------|
| **CALL** | iABC | `R(A),... := R(A)(R(A+1),...,R(A+B-1))` | 函数调用 |
| **TAILCALL** | iABC | `return R(A)(R(A+1),...,R(A+B-1))` | 尾调用 |
| **RETURN** | iABC | `return R(A),...,R(A+B-2)` | 返回 |

### 循环 (Loop)

| 指令 | 格式 | 伪代码 | 说明 |
|------|------|--------|------|
| **FORLOOP** | iAsBx | `R(A)+=R(A+2); if R(A)<=R(A+1) then pc+=sBx` | 数值 for 循环 |
| **FORPREP** | iAsBx | `R(A)-=R(A+2); pc+=sBx` | 数值 for 准备 |
| **TFORLOOP** | iABC | 泛型 for 迭代 | 支持 C/Lua 函数迭代器 |

### 闭包 / 其他 (Closure / Misc)

| 指令 | 格式 | 伪代码 | 说明 |
|------|------|--------|------|
| **CLOSURE** | iABx | `R(A) := closure(subProto[Bx])` | 创建闭包 |
| **CLOSE** | iABC | close upvalues >= R(A) | 关闭 upvalue |
| **VARARG** | iABC | `R(A),... := vararg` | 变长参数 |

## 4. RK 寻址

```
BITRK = 256

如果操作数 < 256:
  → 寄存器: R(operand)
如果操作数 >= 256:
  → 常量: K(operand - 256)

ISK(x):  (x & BITRK) != 0
INDEXK(x): x & ~BITRK
RKASK(x): x | BITRK
```

## 5. 指令分组

| 组 | 指令 | Handler 文件 |
|----|------|-------------|
| DataMove | MOVE, LOADK, LOADBOOL, LOADNIL | `vm_handlers_data.cpp` |
| Global | GETGLOBAL, SETGLOBAL | `vm_handlers_global_upvalue.cpp` |
| Upvalue | GETUPVAL, SETUPVAL | `vm_handlers_global_upvalue.cpp` |
| Table | GETTABLE, SETTABLE, NEWTABLE, SELF, SETLIST | `vm_handlers_table.cpp` |
| Arithmetic | ADD, SUB, MUL, DIV, MOD, POW | `vm_handlers_arith.cpp` |
| Unary | UNM, NOT, LEN, CONCAT | `vm_handlers_unary.cpp` |
| Branch | JMP, TEST, TESTSET | `vm_handlers_branch.cpp` |
| Comparison | EQ, LT, LE | `vm_handlers_branch.cpp` |
| Call | CALL, TAILCALL, RETURN | `vm_handlers_call.cpp` |
| Loop | FORLOOP, FORPREP, TFORLOOP | `vm_handlers_loop.cpp` |
| Closure | CLOSURE, CLOSE | `vm_handlers_closure.cpp` |
| Vararg | VARARG | `vm_handlers_call.cpp` |
