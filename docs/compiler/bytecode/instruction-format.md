# Instruction Format — 指令格式详解

## 1. 这个模块解决什么问题？

详细说明 32 位指令的位布局和编解码方式。

## 2. 位布局

```
32-bit Instruction:

iABC:
  Bit 0-5   : OP (6 bits)  — 操作码 (0-37)
  Bit 6-13  : A  (8 bits)  — 目标寄存器 (0-255)
  Bit 14-22 : C  (9 bits)  — 第三操作数 (0-511)
  Bit 23-31 : B  (9 bits)  — 第二操作数 (0-511)

iABx:
  Bit 0-5   : OP (6 bits)
  Bit 6-13  : A  (8 bits)
  Bit 14-31 : Bx (18 bits) — 大索引 (0-262143)

iAsBx:
  Bit 0-5   : OP (6 bits)
  Bit 6-13  : A  (8 bits)
  Bit 14-31 : sBx (18 bits, signed) — 有符号偏移 (-131071 to 131071)
```

## 3. 编码函数

```cpp
// iABC 格式
Instruction CREATE_ABC(OpCode op, i32 a, i32 b, i32 c) {
    return (static_cast<u32>(op) << 0)
         | (static_cast<u32>(a) << 6)
         | (static_cast<u32>(b) << 23)
         | (static_cast<u32>(c) << 14);
}

// iABx 格式
Instruction CREATE_ABx(OpCode op, i32 a, i32 bx) {
    return (static_cast<u32>(op) << 0)
         | (static_cast<u32>(a) << 6)
         | (static_cast<u32>(bx) << 14);
}

// iAsBx 格式
Instruction CREATE_AsBx(OpCode op, i32 a, i32 sbx) {
    return CREATE_ABx(op, a, sbx + 131071);  // 偏置为无符号
}
```

## 4. 解码函数

```cpp
OpCode GET_OPCODE(u32 i) { return (OpCode)((i >> 0) & 0x3F); }
i32 GETARG_A(u32 i)      { return (i >> 6) & 0xFF; }
i32 GETARG_B(u32 i)      { return (i >> 23) & 0x1FF; }
i32 GETARG_C(u32 i)      { return (i >> 14) & 0x1FF; }
i32 GETARG_Bx(u32 i)     { return (i >> 14) & 0x3FFFF; }
i32 GETARG_sBx(u32 i)    { return GETARG_Bx(i) - 131071; }
```

## 5. 参数范围

| 参数 | 位数 | 范围 |
|------|------|------|
| OP | 6 | 0-63 (当前用 0-37) |
| A | 8 | 0-255 |
| B | 9 | 0-511 |
| C | 9 | 0-511 |
| Bx | 18 | 0-262143 |
| sBx | 18 (signed) | -131071 到 131071 |

## 6. RK 寻址

```
第 9 位用作标志位:

B 参数:
  BITRK = 256 (第 8 位为 1)
  if (B & 0x100): 常量索引 = B & 0xFF
  else:           寄存器索引 = B

最大常量索引: 255
最大寄存器索引: 255
```

## 7. 特殊常量

```
NO_REG = 255  — 表示"无寄存器"
                 (用于 RETURN R(A) 的 B=1 表示返回 0 个值)
```
