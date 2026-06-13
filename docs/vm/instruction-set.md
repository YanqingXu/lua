---
status: current
verified_against: src/compiler/opcode.hpp; src/compiler/opcode.cpp; src/vm/vm.cpp; src/vm/vm_ops.cpp; src/vm/vm_call.cpp; src/vm/vm_table.cpp; src/vm/vm_frame.cpp; src/vm/vm_loop.cpp
last_checked: 2026-05-19
applies_to: current Lua 5.1-style VM opcode set
---

# VM 指令集

VM 使用 Lua 5.1 风格的寄存器字节码。指令在 `src/compiler/opcode.hpp` 中编码，由 VM 分发循环和 `src/vm/` 下的辅助文件执行。

`NUM_OPCODES` 当前为 38。

## 编码

| 编码 | 含义 |
|---|---|
| iABC | 操作码 + A + B + C |
| iABx | 操作码 + A + Bx |
| iAsBx | 操作码 + A + 有符号 Bx |

`RK` 操作数可指向寄存器或常量表槽位。`BITRK` 标记常量；`ISK()` 和 `INDEXK()` 解码值。

## 操作码分组

| 分组 | 操作码 |
|---|---|
| 数据移动 | `MOVE`、`LOADK`、`LOADBOOL`、`LOADNIL` |
| 变量读取 | `GETUPVAL`、`GETGLOBAL`、`GETTABLE` |
| 变量写入 | `SETGLOBAL`、`SETUPVAL`、`SETTABLE` |
| 表构建 | `NEWTABLE`、`SELF`、`SETLIST` |
| 算术/一元 | `ADD`、`SUB`、`MUL`、`DIV`、`MOD`、`POW`、`UNM`、`NOT`、`LEN` |
| 字符串 | `CONCAT` |
| 分支 | `JMP`、`EQ`、`LT`、`LE`、`TEST`、`TESTSET` |
| 调用 | `CALL`、`TAILCALL`、`RETURN` |
| 循环 | `FORLOOP`、`FORPREP`、`TFORLOOP` |
| 闭包/上值 | `CLOSE`、`CLOSURE` |
| 变长参数 | `VARARG` |

## 语义摘要

| 操作码 | 简要行为 |
|---|---|
| `MOVE` | `R(A) := R(B)` |
| `LOADK` | `R(A) := K(Bx)` |
| `LOADBOOL` | `R(A) := bool(B)`；若 `C != 0` 则跳过下一条指令 |
| `LOADNIL` | 将 `R(A)` 到 `R(B)` 置为 nil |
| `GETUPVAL` | 将上值 `B` 加载到 `R(A)` |
| `GETGLOBAL` | 将全局变量 `K(Bx)` 加载到 `R(A)` |
| `GETTABLE` | 将 `R(B)[RK(C)]` 加载到 `R(A)` |
| `SETGLOBAL` | 将 `R(A)` 存储到全局变量 `K(Bx)` |
| `SETUPVAL` | 将 `R(A)` 存储到上值 `B` |
| `SETTABLE` | 将 `RK(C)` 存储到 `R(A)[RK(B)]` |
| `NEWTABLE` | 在 `R(A)` 中创建表 |
| `SELF` | 准备方法调用接收者和方法函数 |
| `ADD`..`POW` | 对 `RK(B)` 和 `RK(C)` 执行算术运算，在适用时使用元方法回退 |
| `UNM`、`LEN` | 一元取负和长度，在适用时支持元方法 |
| `NOT` | Lua 真值取反 |
| `CONCAT` | 拼接寄存器 `R(B)` 到 `R(C)` |
| `JMP` | 将有符号偏移 `sBx` 加到 PC |
| `EQ`、`LT`、`LE` | 条件比较和跳过 |
| `TEST`、`TESTSET` | 用于条件和短路表达式的真值测试 |
| `CALL` | 调用 `R(A)` 中的函数，使用编码的 arg/result 计数 |
| `TAILCALL` | 尾调用 `R(A)` 中的函数并在可能时复用当前帧 |
| `RETURN` | 从帧返回固定或开放值 |
| `FORPREP`、`FORLOOP` | 数值 for 循环设置和迭代 |
| `TFORLOOP` | 泛型 for 循环迭代器调用 |
| `SETLIST` | 批量写入数组字段到表 |
| `CLOSE` | 关闭 `R(A)` 及以上位置的 open upvalue |
| `CLOSURE` | 从子 proto `Bx` 创建闭包并捕获上值 |
| `VARARG` | 将变长参数值加载到寄存器 |

## 阅读指引

- 编码辅助：`src/compiler/opcode.hpp`
- 主分发：`src/vm/vm.cpp`
- 算术、比较、元方法辅助：`src/vm/vm_ops.cpp`
- 调用和尾调用：`src/vm/vm_call.cpp`
- 闭包和 vararg：`src/vm/vm_frame.cpp`
- 泛型 for：`src/vm/vm_loop.cpp`
- SETLIST：`src/vm/vm_table.cpp`

实用测试：

```powershell
bin\lua_test.exe --filter "VM Dispatch"
bin\lua_test.exe --filter "VM Internal"
bin\lua_test.exe --filter "Function Call"
```
