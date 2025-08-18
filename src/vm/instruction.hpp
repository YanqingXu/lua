#pragma once

#include "../common/types.hpp"
#include "../common/opcodes.hpp"
#include <iostream>

namespace Lua {
    // === 官方Lua 5.1指令格式常量定义 ===
    // 基于lua-5.1.5/src/lopcodes.h

    // 指令字段大小定义
    static const u8 SIZE_C = 9;
    static const u8 SIZE_B = 9;
    static const u8 SIZE_Bx = (SIZE_C + SIZE_B);  // 18
    static const u8 SIZE_A = 8;
    static const u8 SIZE_OP = 6;

    // 指令字段位置定义
    static const u8 POS_OP = 0;
    static const u8 POS_A = (POS_OP + SIZE_OP);    // 6
    static const u8 POS_C = (POS_A + SIZE_A);      // 14
    static const u8 POS_B = (POS_C + SIZE_C);      // 23
    static const u8 POS_Bx = POS_C;                // 14

    // 参数最大值定义
    static const u32 MAXARG_Bx = ((1 << SIZE_Bx) - 1);      // 262143
    static const i32 MAXARG_sBx = (MAXARG_Bx >> 1);         // 131071
    static const u32 MAXARG_A = ((1 << SIZE_A) - 1);        // 255
    static const u32 MAXARG_B = ((1 << SIZE_B) - 1);        // 511
    static const u32 MAXARG_C = ((1 << SIZE_C) - 1);        // 511

    // RK常量索引定义（9位操作数）
    static const u16 BITRK = (1 << (SIZE_B - 1));  // 256, this bit 1 means constant
    static const u16 MAXINDEXRK = (BITRK - 1);     // 255, maximum constant index

    // === 官方Lua 5.1指令操作宏 ===
    // 基于lua-5.1.5/src/lopcodes.h的宏定义

    // 位掩码创建宏
    inline u32 MASK1(u8 n, u8 p) {
        return ((~((~static_cast<u32>(0)) << n)) << p);
    }

    inline u32 MASK0(u8 n, u8 p) {
        return (~MASK1(n, p));
    }

    // 指令操作宏（与官方Lua 5.1完全一致）
    inline OpCode GET_OPCODE(u32 i) {
        return static_cast<OpCode>((i >> POS_OP) & MASK1(SIZE_OP, 0));
    }

    inline void SET_OPCODE(u32& i, OpCode o) {
        i = ((i & MASK0(SIZE_OP, POS_OP)) |
             ((static_cast<u32>(o) << POS_OP) & MASK1(SIZE_OP, POS_OP)));
    }

    inline u8 GETARG_A(u32 i) {
        return static_cast<u8>((i >> POS_A) & MASK1(SIZE_A, 0));
    }

    inline void SETARG_A(u32& i, u8 u) {
        i = ((i & MASK0(SIZE_A, POS_A)) |
             ((static_cast<u32>(u) << POS_A) & MASK1(SIZE_A, POS_A)));
    }

    inline u16 GETARG_B(u32 i) {
        return static_cast<u16>((i >> POS_B) & MASK1(SIZE_B, 0));
    }

    inline void SETARG_B(u32& i, u16 b) {
        i = ((i & MASK0(SIZE_B, POS_B)) |
             ((static_cast<u32>(b) << POS_B) & MASK1(SIZE_B, POS_B)));
    }

    inline u16 GETARG_C(u32 i) {
        return static_cast<u16>((i >> POS_C) & MASK1(SIZE_C, 0));
    }

    inline void SETARG_C(u32& i, u16 c) {
        i = ((i & MASK0(SIZE_C, POS_C)) |
             ((static_cast<u32>(c) << POS_C) & MASK1(SIZE_C, POS_C)));
    }

    inline u32 GETARG_Bx(u32 i) {
        return (i >> POS_Bx) & MASK1(SIZE_Bx, 0);
    }

    inline void SETARG_Bx(u32& i, u32 bx) {
        i = ((i & MASK0(SIZE_Bx, POS_Bx)) |
             ((bx << POS_Bx) & MASK1(SIZE_Bx, POS_Bx)));
    }

    inline i32 GETARG_sBx(u32 i) {
        return static_cast<i32>(GETARG_Bx(i)) - MAXARG_sBx;
    }

    inline void SETARG_sBx(u32& i, i32 sbx) {
        SETARG_Bx(i, static_cast<u32>(sbx + MAXARG_sBx));
    }

    // 指令创建宏（注意：官方Lua 5.1中B在高位，C在低位）
    inline u32 CREATE_ABC(OpCode o, u8 a, u16 b, u16 c) {
        return ((static_cast<u32>(o) << POS_OP) |
                (static_cast<u32>(a) << POS_A) |
                (static_cast<u32>(b) << POS_B) |
                (static_cast<u32>(c) << POS_C));
    }

    inline u32 CREATE_ABx(OpCode o, u8 a, u32 bc) {
        return ((static_cast<u32>(o) << POS_OP) |
                (static_cast<u32>(a) << POS_A) |
                (bc << POS_Bx));
    }

    // RK操作函数（9位操作数兼容）
    inline bool ISK(u16 x) {
        return (x & BITRK) != 0;
    }

    inline u16 RKASK(u16 x) {
        return x | BITRK;
    }

    inline u16 INDEXK(u16 r) {
        return r & ~BITRK;
    }

    // Legacy compatibility - use RKASK instead
    inline u16 RK(u16 x) {
        return RKASK(x);
    }

    // 无效寄存器定义
    static const u8 NO_REG = MAXARG_A;
    
    // === 官方Lua 5.1指令结构体 ===
    // 32位指令，包含操作码和操作数
    // 位布局：[31-23: C(9bit)] [22-14: B(9bit)] [13-6: A(8bit)] [5-0: OpCode(6bit)]
    struct Instruction {
        u32 code;

        // 构造函数
        Instruction() : code(0) {}
        explicit Instruction(u32 c) : code(c) {}

        // === 使用官方Lua 5.1宏的访问器方法 ===

        // 获取操作码（6位，位置0-5）
        OpCode getOpCode() const {
            return GET_OPCODE(code);
        }

        // 设置操作码（6位，位置0-5）
        void setOpCode(OpCode op) {
            SET_OPCODE(code, op);
        }

        // 获取A操作数（8位，位置6-13）
        u8 getA() const {
            return GETARG_A(code);
        }

        // 设置A操作数（8位，位置6-13）
        void setA(u8 a) {
            SETARG_A(code, a);
        }

        // 获取B操作数（9位，位置14-22）
        u16 getB() const {
            return GETARG_B(code);
        }

        // 设置B操作数（9位，位置14-22）
        void setB(u16 b) {
            SETARG_B(code, b);
        }

        // 获取C操作数（9位，位置23-31）
        u16 getC() const {
            return GETARG_C(code);
        }

        // 设置C操作数（9位，位置23-31）
        void setC(u16 c) {
            SETARG_C(code, c);
        }

        // 获取Bx操作数（18位无符号，B和C组合，位置14-31）
        u32 getBx() const {
            return GETARG_Bx(code);
        }

        // 设置Bx操作数（18位无符号，B和C组合，位置14-31）
        void setBx(u32 bx) {
            SETARG_Bx(code, bx);
        }

        // 获取sBx操作数（18位有符号，偏移131071）
        i32 getSBx() const {
            return GETARG_sBx(code);
        }

        // 设置sBx操作数（18位有符号，偏移131071）
        void setSBx(i32 sbx) {
            SETARG_sBx(code, sbx);
        }
        
        // === 使用官方Lua 5.1宏的指令创建方法 ===

        // 基本指令创建（使用CREATE_ABC和CREATE_ABx宏）
        static Instruction createMOVE(u8 a, u16 b) {
            return Instruction(CREATE_ABC(OpCode::MOVE, a, b, 0));
        }

        static Instruction createLOADK(u8 a, u32 bx) {
            return Instruction(CREATE_ABx(OpCode::LOADK, a, bx));
        }

        static Instruction createGETGLOBAL(u8 a, u32 bx) {
            return Instruction(CREATE_ABx(OpCode::GETGLOBAL, a, bx));
        }

        static Instruction createSETGLOBAL(u8 a, u32 bx) {
            return Instruction(CREATE_ABx(OpCode::SETGLOBAL, a, bx));
        }

        static Instruction createGETTABLE(u8 a, u16 b, u16 c) {
            return Instruction(CREATE_ABC(OpCode::GETTABLE, a, b, c));
        }

        static Instruction createSETTABLE(u8 a, u16 b, u16 c) {
            return Instruction(CREATE_ABC(OpCode::SETTABLE, a, b, c));
        }

        static Instruction createNEWTABLE(u8 a, u16 b, u16 c) {
            return Instruction(CREATE_ABC(OpCode::NEWTABLE, a, b, c));
        }
        
        static Instruction createCALL(u8 a, u16 b, u16 c) {
            return Instruction(CREATE_ABC(OpCode::CALL, a, b, c));
        }

        static Instruction createRETURN(u8 a, u16 b) {
            return Instruction(CREATE_ABC(OpCode::RETURN, a, b, 0));
        }

        static Instruction createVARARG(u8 a, u8 b) {
            return Instruction(CREATE_ABC(OpCode::VARARG, a, b, 0));
        }

        // 加载nil
        static Instruction createLOADNIL(u8 a) {
            return Instruction(CREATE_ABC(OpCode::LOADNIL, a, 0, 0));
        }

        // 加载布尔值（Lua LOADBOOL使用B作为值(0/1)，C作为跳过标志）
        static Instruction createLOADBOOL(u8 a, bool value) {
            return Instruction(CREATE_ABC(OpCode::LOADBOOL, a, static_cast<u8>(value), 0));
        }
        
        // 算术运算
        static Instruction createADD(u8 a, u8 b, u8 c) {
            return Instruction(CREATE_ABC(OpCode::ADD, a, b, c));
        }
        static Instruction createSUB(u8 a, u8 b, u8 c) {
            return Instruction(CREATE_ABC(OpCode::SUB, a, b, c));
        }
        static Instruction createMUL(u8 a, u8 b, u8 c) {
            return Instruction(CREATE_ABC(OpCode::MUL, a, b, c));
        }
        static Instruction createDIV(u8 a, u8 b, u8 c) {
            return Instruction(CREATE_ABC(OpCode::DIV, a, b, c));
        }
        static Instruction createMOD(u8 a, u8 b, u8 c) {
            return Instruction(CREATE_ABC(OpCode::MOD, a, b, c));
        }
        static Instruction createPOW(u8 a, u8 b, u8 c) {
            return Instruction(CREATE_ABC(OpCode::POW, a, b, c));
        }

        // 一元运算
        static Instruction createUNM(u8 a, u8 b) { // dst=a, operand=b
            return Instruction(CREATE_ABC(OpCode::UNM, a, b, 0));
        }
        static Instruction createNOT(u8 a, u8 b) {
            return Instruction(CREATE_ABC(OpCode::NOT, a, b, 0));
        }
        static Instruction createLEN(u8 a, u8 b) {
            return Instruction(CREATE_ABC(OpCode::LEN, a, b, 0));
        }

        // 比较运算
        static Instruction createEQ(u8 a, u8 b, u8 c) {
            return Instruction(CREATE_ABC(OpCode::EQ, a, b, c));
        }
        static Instruction createLT(u8 a, u8 b, u8 c) {
            return Instruction(CREATE_ABC(OpCode::LT, a, b, c));
        }
        static Instruction createLE(u8 a, u8 b, u8 c) {
            return Instruction(CREATE_ABC(OpCode::LE, a, b, c));
        }
        
        // 跳转指令（sBx）
        static Instruction createJMP(i32 sbx) {
            Instruction i;
            i.setOpCode(OpCode::JMP);
            i.setSBx(sbx);
            return i;
        }

        // 循环指令
        static Instruction createFORPREP(u8 a, i32 sbx) {
            Instruction i;
            i.setOpCode(OpCode::FORPREP);
            i.setA(a);
            i.setSBx(sbx);
            return i;
        }

        static Instruction createFORLOOP(u8 a, i32 sbx) {
            Instruction i;
            i.setOpCode(OpCode::FORLOOP);
            i.setA(a);
            i.setSBx(sbx);
            return i;
        }

        // 测试指令（A = 要测试的寄存器，C = 测试失败时跳过下一条指令）
        static Instruction createTEST(u8 a, u8 c) {
            return Instruction(CREATE_ABC(OpCode::TEST, a, 0, c));
        }

        // 闭包创建（A = 目标寄存器，Bx = 函数原型索引）
        static Instruction createCLOSURE(u8 a, u32 bx) {
            return Instruction(CREATE_ABx(OpCode::CLOSURE, a, bx));
        }

        // Upvalue操作
        static Instruction createGETUPVAL(u8 a, u8 b) {
            return Instruction(CREATE_ABC(OpCode::GETUPVAL, a, b, 0));
        }

        static Instruction createSETUPVAL(u8 a, u8 b) {
            return Instruction(CREATE_ABC(OpCode::SETUPVAL, a, b, 0));
        }

        static Instruction createCLOSE(u8 a) {
            return Instruction(CREATE_ABC(OpCode::CLOSE, a, 0, 0));
        }

        // 字符串连接
        static Instruction createCONCAT(u8 a, u8 b, u8 c) {
            return Instruction(CREATE_ABC(OpCode::CONCAT, a, b, c));
        }

        // === 官方Lua 5.1操作码创建函数 ===

        static Instruction createSELF(u8 a, u8 b, u8 c) {
            return Instruction(CREATE_ABC(OpCode::SELF, a, b, c));
        }

        static Instruction createTESTSET(u8 a, u8 b, u8 c) {
            return Instruction(CREATE_ABC(OpCode::TESTSET, a, b, c));
        }

        static Instruction createTAILCALL(u8 a, u8 b, u8 c) {
            return Instruction(CREATE_ABC(OpCode::TAILCALL, a, b, c));
        }

        static Instruction createTFORLOOP(u8 a, u8 c) {
            return Instruction(CREATE_ABC(OpCode::TFORLOOP, a, 0, c));
        }

        static Instruction createSETLIST(u8 a, u8 b, u8 c) {
            return Instruction(CREATE_ABC(OpCode::SETLIST, a, b, c));
        }
    };
}
