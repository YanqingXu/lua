#pragma once

#include "../common/types.hpp"
#include "../common/opcodes.hpp"
#include <iostream>

namespace Lua {
    // Lua 5.1 official constant indexing definitions
    // Based on Lua 5.1.5 lopcodes.h with 9-bit operands

    // For 9-bit operands (Lua 5.1 official)
    static const u16 BITRK = (1 << 8);  // 256, this bit 1 means constant for 9-bit
    static const u16 MAXINDEXRK = (BITRK - 1);  // 255, maximum constant index for 9-bit

    // 9-bit versions (Lua 5.1 compatible)
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
    
    // Instruction format (Lua 5.1 compatible)
    // 32-bit instruction containing opcode and operands
    // Bit layout: [31-26: unused] [25-23: C(9bit)] [22-14: B(9bit)] [13-6: A(8bit)] [5-0: OpCode(6bit)]
    struct Instruction {
        u32 code;

        // Constructor
        Instruction() : code(0) {}
        explicit Instruction(u32 c) : code(c) {}

        // Get opcode (6 bits, positions 0-5)
        OpCode getOpCode() const {
            return static_cast<OpCode>(code & 0x3F);
        }

        // Set opcode (6 bits, positions 0-5)
        void setOpCode(OpCode op) {
            code = (code & 0xFFFFFFC0) | (static_cast<u32>(op) & 0x3F);
        }
        
        // Get A operand (8 bits, positions 6-13)
        u8 getA() const {
            return (code >> 6) & 0xFF;
        }

        // Set A operand (8 bits, positions 6-13)
        void setA(u8 a) {
            code = (code & 0xFFFFC03F) | ((static_cast<u32>(a) & 0xFF) << 6);
        }
        
        // Get B operand (9 bits, positions 14-22)
        u16 getB() const {
            return (code >> 14) & 0x1FF;
        }

        // Set B operand (9 bits, positions 14-22)
        void setB(u16 b) {
            code = (code & 0xFF803FFF) | ((static_cast<u32>(b) & 0x1FF) << 14);
        }

        // Get C operand (9 bits, positions 23-31)
        u16 getC() const {
            return (code >> 23) & 0x1FF;
        }

        // Set C operand (9 bits, positions 23-31)
        void setC(u16 c) {
            code = (code & 0x007FFFFF) | ((static_cast<u32>(c) & 0x1FF) << 23);
        }
        
        // Get Bx operand (18-bit unsigned, B and C combined, positions 14-31)
        u32 getBx() const {
            return (code >> 14) & 0x3FFFF;
        }

        // Set Bx operand (18-bit unsigned, B and C combined, positions 14-31)
        void setBx(u32 bx) {
            code = (code & 0x00003FFF) | ((bx & 0x3FFFF) << 14);
        }

        // Get sBx operand (18-bit signed, offset by 131071)
        i32 getSBx() const {
            return static_cast<i32>(getBx()) - 131071;
        }

        // Set sBx operand (18-bit signed, offset by 131071)
        void setSBx(i32 sbx) {
            setBx(static_cast<u32>(sbx + 131071));
        }
        
        // Static methods to create various instructions (Lua 5.1 compatible)
        static Instruction createMOVE(u8 a, u16 b) {
            Instruction i;
            i.setOpCode(OpCode::MOVE);
            i.setA(a);
            i.setB(b);
            return i;
        }

        static Instruction createLOADK(u8 a, u32 bx) {
            Instruction i;
            i.setOpCode(OpCode::LOADK);
            i.setA(a);
            i.setBx(bx);
            return i;
        }

        static Instruction createGETGLOBAL(u8 a, u32 bx) {
            Instruction i;
            i.setOpCode(OpCode::GETGLOBAL);
            i.setA(a);
            i.setBx(bx);
            return i;
        }
        
        static Instruction createSETGLOBAL(u8 a, u32 bx) {
            Instruction i;
            i.setOpCode(OpCode::SETGLOBAL);
            i.setA(a);
            i.setBx(bx);
            return i;
        }

        static Instruction createGETTABLE(u8 a, u16 b, u16 c) {
            Instruction i;
            i.setOpCode(OpCode::GETTABLE);
            i.setA(a);
            i.setB(b);
            i.setC(c);
            return i;
        }

        static Instruction createSETTABLE(u8 a, u16 b, u16 c) {
            Instruction i;
            i.setOpCode(OpCode::SETTABLE);
            i.setA(a);
            i.setB(b);
            i.setC(c);
            return i;
        }

        static Instruction createNEWTABLE(u8 a, u16 b, u16 c) {
            Instruction i;
            i.setOpCode(OpCode::NEWTABLE);
            i.setA(a);
            i.setB(b);
            i.setC(c);
            return i;
        }
        
        static Instruction createCALL(u8 a, u16 b, u16 c) {
            Instruction i;
            i.setOpCode(OpCode::CALL);
            i.setA(a);
            i.setB(b);
            i.setC(c);
            return i;
        }

        static Instruction createRETURN(u8 a, u16 b) {
            Instruction i;
            i.setOpCode(OpCode::RETURN);
            i.setA(a);
            i.setB(b);
            return i;
        }

        static Instruction createVARARG(u8 a, u8 b) {
            Instruction i;
            i.setOpCode(OpCode::VARARG);
            i.setA(a);
            i.setB(b);
            return i;
        }
        
        // Load nil
        static Instruction createLOADNIL(u8 a) {
            Instruction i;
            i.setOpCode(OpCode::LOADNIL);
            i.setA(a);
            return i;
        }
        
        // Load boolean value (Lua LOADBOOL uses B as value (0/1) and C as skip flag)
        static Instruction createLOADBOOL(u8 a, bool value) {
            Instruction i;
            i.setOpCode(OpCode::LOADBOOL);
            i.setA(a);
            i.setB(static_cast<u8>(value));
            i.setC(0);
            return i;
        }
        
        // Arithmetic
        static Instruction createADD(u8 a, u8 b, u8 c) {
            Instruction i;
            i.setOpCode(OpCode::ADD);
            i.setA(a);
            i.setB(b);
            i.setC(c);
            return i;
        }
        static Instruction createSUB(u8 a, u8 b, u8 c) {
            Instruction i; i.setOpCode(OpCode::SUB); i.setA(a); i.setB(b); i.setC(c); return i; }
        static Instruction createMUL(u8 a, u8 b, u8 c) {
            Instruction i; i.setOpCode(OpCode::MUL); i.setA(a); i.setB(b); i.setC(c); return i; }
        static Instruction createDIV(u8 a, u8 b, u8 c) {
            Instruction i; i.setOpCode(OpCode::DIV); i.setA(a); i.setB(b); i.setC(c); return i; }
        static Instruction createMOD(u8 a, u8 b, u8 c) {
            Instruction i; i.setOpCode(OpCode::MOD); i.setA(a); i.setB(b); i.setC(c); return i; }
        static Instruction createPOW(u8 a, u8 b, u8 c) {
            Instruction i; i.setOpCode(OpCode::POW); i.setA(a); i.setB(b); i.setC(c); return i; }
        
        // Unary operations
        static Instruction createUNM(u8 a, u8 b) { // dst=a, operand=b
            Instruction i; i.setOpCode(OpCode::UNM); i.setA(a); i.setB(b); return i; }
        static Instruction createNOT(u8 a, u8 b) {
            Instruction i; i.setOpCode(OpCode::NOT); i.setA(a); i.setB(b); return i; }
        static Instruction createLEN(u8 a, u8 b) {
            Instruction i; i.setOpCode(OpCode::LEN); i.setA(a); i.setB(b); return i; }
        
        // Comparison
        static Instruction createEQ(u8 a, u8 b, u8 c) { Instruction i; i.setOpCode(OpCode::EQ); i.setA(a); i.setB(b); i.setC(c); return i; }
        static Instruction createLT(u8 a, u8 b, u8 c) { Instruction i; i.setOpCode(OpCode::LT); i.setA(a); i.setB(b); i.setC(c); return i; }
        static Instruction createLE(u8 a, u8 b, u8 c) { Instruction i; i.setOpCode(OpCode::LE); i.setA(a); i.setB(b); i.setC(c); return i; }
        
        // Jump (sBx)
        static Instruction createJMP(i16 sbx) {
            Instruction i; i.setOpCode(OpCode::JMP); i.setSBx(sbx); return i; }

        // For loop instructions
        static Instruction createFORPREP(u8 a, i16 sbx) {
            Instruction i; i.setOpCode(OpCode::FORPREP); i.setA(a); i.setSBx(sbx); return i; }

        static Instruction createFORLOOP(u8 a, i16 sbx) {
            Instruction i; i.setOpCode(OpCode::FORLOOP); i.setA(a); i.setSBx(sbx); return i; }

        // Test instruction (A = register to test, C = skip next instruction if test fails)
        static Instruction createTEST(u8 a, u8 c) {
            Instruction i; i.setOpCode(OpCode::TEST); i.setA(a); i.setC(c); return i; }
        
        // Closure creation (A = target register, Bx = function prototype index)
        static Instruction createCLOSURE(u8 a, u16 bx) {
            Instruction i; i.setOpCode(OpCode::CLOSURE); i.setA(a); i.setBx(bx); return i; }
        
        // Upvalue operations
        static Instruction createGETUPVAL(u8 a, u8 b) {
            Instruction i; i.setOpCode(OpCode::GETUPVAL); i.setA(a); i.setB(b); return i; }
        
        static Instruction createSETUPVAL(u8 a, u8 b) {
            Instruction i; i.setOpCode(OpCode::SETUPVAL); i.setA(a); i.setB(b); return i; }
        
        static Instruction createCLOSE(u8 a) {
            Instruction i; i.setOpCode(OpCode::CLOSE); i.setA(a); return i; }

        // String concatenation
        static Instruction createCONCAT(u8 a, u8 b, u8 c) {
            Instruction i; i.setOpCode(OpCode::CONCAT); i.setA(a); i.setB(b); i.setC(c); return i; }

        // === 新增的官方Lua 5.1操作码创建函数 ===

        static Instruction createSELF(u8 a, u8 b, u8 c) {
            Instruction i; i.setOpCode(OpCode::SELF); i.setA(a); i.setB(b); i.setC(c); return i; }

        static Instruction createTESTSET(u8 a, u8 b, u8 c) {
            Instruction i; i.setOpCode(OpCode::TESTSET); i.setA(a); i.setB(b); i.setC(c); return i; }

        static Instruction createTAILCALL(u8 a, u8 b, u8 c) {
            Instruction i; i.setOpCode(OpCode::TAILCALL); i.setA(a); i.setB(b); i.setC(c); return i; }

        static Instruction createTFORLOOP(u8 a, u8 c) {
            Instruction i; i.setOpCode(OpCode::TFORLOOP); i.setA(a); i.setC(c); return i; }

        static Instruction createSETLIST(u8 a, u8 b, u8 c) {
            Instruction i; i.setOpCode(OpCode::SETLIST); i.setA(a); i.setB(b); i.setC(c); return i; }
    };
}
