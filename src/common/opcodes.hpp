#pragma once

#include "../types.hpp"

namespace Lua {
    // 简化版操作码 (用于构建字节码)
    #define OP_CONSTANT     0x01
    #define OP_NIL          0x02
    #define OP_TRUE         0x03
    #define OP_FALSE        0x04
    #define OP_POP          0x05
    #define OP_GET_LOCAL    0x06
    #define OP_SET_LOCAL    0x07
    #define OP_GET_GLOBAL   0x08
    #define OP_SET_GLOBAL   0x09
    #define OP_EQUAL        0x0A
    #define OP_LESS         0x0B
    #define OP_GREATER      0x0C
    #define OP_ADD          0x0D
    #define OP_SUBTRACT     0x0E
    #define OP_MULTIPLY     0x0F
    #define OP_DIVIDE       0x10
    #define OP_MODULO       0x11
    #define OP_NOT          0x12
    #define OP_NEGATE       0x13
    #define OP_CALL         0x14
    #define OP_RETURN       0x15
    #define OP_CLOSE_UPVALUE 0x16
    
    // 操作码定义
    enum class OpCode : u8 {
        // 基本操作
        MOVE,       // 将一个寄存器复制到另一个寄存器
        LOADK,      // 从常量表加载常量到寄存器
        LOADBOOL,   // 加载布尔值到寄存器
        LOADNIL,    // 加载nil到寄存器
        
        // 表操作
        GETGLOBAL,  // 获取全局变量
        SETGLOBAL,  // 设置全局变量
        GETTABLE,   // 获取表元素
        SETTABLE,   // 设置表元素
        NEWTABLE,   // 创建新表
        
        // 算术操作
        ADD,        // 加法
        SUB,        // 减法
        MUL,        // 乘法
        DIV,        // 除法
        MOD,        // 取模
        POW,        // 幂运算
        UNM,        // 一元减法
        NOT,        // 逻辑非
        LEN,        // 长度操作
        
        // 比较操作
        EQ,         // 相等
        LT,         // 小于
        LE,         // 小于等于
        
        // 跳转
        JMP,        // 跳转
        
        // 函数调用
        CALL,       // 调用函数
        RETURN,     // 函数返回
        
        // 闭包操作
        CLOSURE,    // 创建闭包
        
        // 其他操作
        CONCAT,     // 字符串连接
        CLOSE       // 关闭upvalue
    };
    
    // 指令格式
    // 指令格式: 高8位操作码, 余下24位为操作数
    constexpr OpCode GET_OPCODE(Instruction i) {
        return static_cast<OpCode>((i >> 24) & 0xFF);
    }
    
    constexpr void SET_OPCODE(Instruction& i, OpCode op) {
        i = (i & 0x00FFFFFF) | (static_cast<Instruction>(op) << 24);
    }
    
    // A, B, C操作数获取宏 (9, 8, 9位宽度)
    constexpr u32 GET_A(Instruction i) { return (i >> 15) & 0x1FF; }
    constexpr u32 GET_B(Instruction i) { return (i >> 7) & 0xFF; }
    constexpr u32 GET_C(Instruction i) { return i & 0x7F; }
    
    // Bx操作数 (17位宽度)
    constexpr u32 GET_Bx(Instruction i) { return i & 0x1FFFF; }
    
    // sBx操作数 (有符号的Bx, -131071 到 131071)
    constexpr i32 GET_sBx(Instruction i) { return static_cast<i32>(GET_Bx(i)) - 131071; }
    
    // 设置操作数
    constexpr void SET_A(Instruction& i, u32 a) {
        i = (i & ~(0x1FF << 15)) | ((a & 0x1FF) << 15);
    }
    
    constexpr void SET_B(Instruction& i, u32 b) {
        i = (i & ~(0xFF << 7)) | ((b & 0xFF) << 7);
    }
    
    constexpr void SET_C(Instruction& i, u32 c) {
        i = (i & ~0x7F) | (c & 0x7F);
    }
    
    constexpr void SET_Bx(Instruction& i, u32 bx) {
        i = (i & ~0x1FFFF) | (bx & 0x1FFFF);
    }
    
    constexpr void SET_sBx(Instruction& i, i32 sbx) {
        SET_Bx(i, static_cast<u32>(sbx + 131071));
    }
    
    // 创建指令
    constexpr Instruction CREATE_ABC(OpCode op, u32 a, u32 b, u32 c) {
        Instruction i = 0;
        SET_OPCODE(i, op);
        SET_A(i, a);
        SET_B(i, b);
        SET_C(i, c);
        return i;
    }
    
    constexpr Instruction CREATE_ABx(OpCode op, u32 a, u32 bx) {
        Instruction i = 0;
        SET_OPCODE(i, op);
        SET_A(i, a);
        SET_Bx(i, bx);
        return i;
    }
    
    constexpr Instruction CREATE_AsBx(OpCode op, u32 a, i32 sbx) {
        Instruction i = 0;
        SET_OPCODE(i, op);
        SET_A(i, a);
        SET_sBx(i, sbx);
        return i;
    }
}
