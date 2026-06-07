# Disassembler — 字节码反汇编器

## 1. 这个模块解决什么问题？

如何查看和调试编译产生的字节码。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/bytecode/bytecode_printer.hpp/cpp` | 字节码打印工具 |
| `src/bytecode/bytecode_main.cpp` | lua_bytecode 入口 |

## 3. 使用方式

### 命令行工具
```bash
# 基础输出
lua_bytecode hello.lua

# 紧凑输出
lua_bytecode --format compact hello.lua

# 完整输出（含常量表、局部变量）
lua_bytecode --format full hello.lua

# 对比两个文件的字节码
lua_bytecode --diff file1.lua file2.lua

# 输出 Mermaid 控制流图
lua_bytecode --cfg hello.lua
```

### REPL 中
```
> local x = 1
> .bytecode
;; 显示当前会话的编译结果

> .ast
;; 显示 AST
```

## 4. 输出格式

### Compact
```
main <hello.lua:0,0> (4 instructions, 3 locals)
  [0] LOADK     0 0       ; R0 = K0(1)
  [1] GETGLOBAL 1 1       ; R1 = Gbl["print"]
  [2] MOVE      2 0       ; R2 = R0
  [3] CALL      1 1 1     ; R1(R2)
```

### Full
```
main <hello.lua:0,0> (4 instructions)
  maxStackSize: 3
  params: 0, isVararg: true
  constants (2):
    K0: 1
    K1: "print"
  locals (1):
    0: "x" [0, 4]

  code:
  [0] LOADK     0 0       ; R0 = K0(1)
  [1] GETGLOBAL 1 1       ; R1 = Gbl["print"]
  [2] MOVE      2 0       ; R2 = R0
  [3] CALL      1 1 1     ; R1(R2)
```

## 5. Mermaid CFG 输出

```
--cfg 参数输出 Mermaid 格式的控制流图:

graph TD
    B0["Block 0 (pc 0-2)"]
    B1["Block 1 (pc 3-5)"]
    B2["Block 2 (pc 6)"]
    B0 -->|"JMP if true"| B1
    B0 -->|"JMP if false"| B2
    B1 --> B2
```
