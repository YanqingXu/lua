#pragma once

#include "../common/types.hpp"
#include "../common/opcodes.hpp"
#include <iostream>

namespace Lua {
    // Lua 5.1 official constant indexing definitions
    // Based on Lua 5.1.5 lopcodes.h but adapted for 8-bit operands
    
    // For 8-bit operands (our current implementation)
    static const u8 BITRK_8 = (1 << 7);  // 128, this bit 1 means constant for 8-bit
    static const u8 MAXINDEXRK_8 = (BITRK_8 - 1);  // 127, maximum constant index for 8-bit
    
    // For 9-bit operands (Lua 5.1 official)
    static const u16 BITRK_9 = (1 << 8);  // 256, this bit 1 means constant for 9-bit
    static const u16 MAXINDEXRK_9 = (BITRK_9 - 1);  // 255, maximum constant index for 9-bit
    
    // 8-bit versions (for our current instruction format)
    inline bool ISK(u8 x) {
        return (x & BITRK_8) != 0;
    }
    
    inline u8 RKASK(u8 x) {
        return x | BITRK_8;
    }
    
    inline u8 INDEXK(u8 r) {
        return r & ~BITRK_8;
    }
    
    // Legacy compatibility - use RKASK instead
    inline u8 RK(u8 x) {
        return RKASK(x);
    }
    
    // 16-bit versions (for potential future Lua 5.1 full compatibility)
    inline bool ISK(u16 x) {
        return (x & BITRK_9) != 0;
    }
    
    inline u16 RKASK(u16 x) {
        return x | BITRK_9;
    }
    
    inline u16 INDEXK(u16 r) {
        return r & ~BITRK_9;
    }
    
    // Instruction format (simplified based on Lua 5.1 instruction format)
    // 32-bit instruction containing opcode and operands
    struct Instruction {
        u32 code;
        
        // Constructor
        Instruction() : code(0) {}
        explicit Instruction(u32 c) : code(c) {}
        
        // Get opcode
        OpCode getOpCode() const {
            return static_cast<OpCode>((code >> 24) & 0xFF);
        }
        
        // Set opcode
        void setOpCode(OpCode op) {
            code = (code & 0x00FFFFFF) | (static_cast<u32>(op) << 24);
        }
        
        // Get A operand (8 bits)
        u8 getA() const {
            return (code >> 16) & 0xFF;
        }
        
        // Set A operand
        void setA(u8 a) {
            code = (code & 0xFF00FFFF) | (static_cast<u32>(a) << 16);
        }
        
        // Get B operand (8 bits)
        u8 getB() const {
            return (code >> 8) & 0xFF;
        }
        
        // Set B operand
        void setB(u8 b) {
            code = (code & 0xFFFF00FF) | (static_cast<u32>(b) << 8);
        }
        
        // Get C operand (8 bits)
        u8 getC() const {
            return code & 0xFF;
        }
        
        // Set C operand
        void setC(u8 c) {
            code = (code & 0xFFFFFF00) | c;
        }
        
        // Get Bx operand (16-bit unsigned)
        u16 getBx() const {
            return code & 0xFFFF;
        }
        
        // Set Bx operand
        void setBx(u16 bx) {
            code = (code & 0xFFFF0000) | bx;
        }
        
        // Get sBx operand (16-bit signed, offset by 32768)
        i16 getSBx() const {
            return getBx() - 32768;
        }
        
        // Set sBx operand
        void setSBx(i16 sbx) {
            setBx(sbx + 32768);
        }
        
        // Static methods to create various instructions
        static Instruction createMOVE(u8 a, u8 b) {
            Instruction i;
            i.setOpCode(OpCode::MOVE);
            i.setA(a);
            i.setB(b);
            return i;
        }
        
        static Instruction createLOADK(u8 a, u16 bx) {
            Instruction i;
            i.setOpCode(OpCode::LOADK);
            i.setA(a);
            i.setBx(bx);
            return i;
        }
        
        static Instruction createGETGLOBAL(u8 a, u16 bx) {
            Instruction i;
            i.setOpCode(OpCode::GETGLOBAL);
            i.setA(a);
            i.setBx(bx);
            return i;
        }
        
        static Instruction createSETGLOBAL(u8 a, u16 bx) {
            Instruction i;
            i.setOpCode(OpCode::SETGLOBAL);
            i.setA(a);
            i.setBx(bx);
            return i;
        }
        
        static Instruction createGETTABLE(u8 a, u8 b, u8 c) {
            Instruction i;
            i.setOpCode(OpCode::GETTABLE);
            i.setA(a);
            i.setB(b);
            i.setC(c);
            return i;
        }
        
        static Instruction createSETTABLE(u8 a, u8 b, u8 c) {
            Instruction i;
            i.setOpCode(OpCode::SETTABLE);
            i.setA(a);
            i.setB(b);
            i.setC(c);
            return i;
        }
        
        static Instruction createNEWTABLE(u8 a, u8 b, u8 c) {
            Instruction i;
            i.setOpCode(OpCode::NEWTABLE);
            i.setA(a);
            i.setB(b);
            i.setC(c);
            return i;
        }
        
        static Instruction createCALL(u8 a, u8 b, u8 c) {
            Instruction i;
            i.setOpCode(OpCode::CALL);
            i.setA(a);
            i.setB(b);
            i.setC(c);
            return i;
        }
        
        static Instruction createRETURN(u8 a, u8 b) {
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
        
        static Instruction createPOP(u8 a) { Instruction i; i.setOpCode(OpCode::POP); i.setA(a); return i; }
        
        // String concatenation
        static Instruction createCONCAT(u8 a, u8 b, u8 c) {
            Instruction i; i.setOpCode(OpCode::CONCAT); i.setA(a); i.setB(b); i.setC(c); return i; }

        // Metamethod-aware operations
        static Instruction createGETTABLE_MM(u8 a, u8 b, u8 c) {
            Instruction i; i.setOpCode(OpCode::GETTABLE_MM); i.setA(a); i.setB(b); i.setC(c); return i; }

        static Instruction createSETTABLE_MM(u8 a, u8 b, u8 c) {
            Instruction i; i.setOpCode(OpCode::SETTABLE_MM); i.setA(a); i.setB(b); i.setC(c); return i; }

        static Instruction createCALL_MM(u8 a, u8 b, u8 c) {
            Instruction i; i.setOpCode(OpCode::CALL_MM); i.setA(a); i.setB(b); i.setC(c); return i; }

        static Instruction createADD_MM(u8 a, u8 b, u8 c) {
            Instruction i; i.setOpCode(OpCode::ADD_MM); i.setA(a); i.setB(b); i.setC(c); return i; }

        static Instruction createSUB_MM(u8 a, u8 b, u8 c) {
            Instruction i; i.setOpCode(OpCode::SUB_MM); i.setA(a); i.setB(b); i.setC(c); return i; }

        static Instruction createMUL_MM(u8 a, u8 b, u8 c) {
            Instruction i; i.setOpCode(OpCode::MUL_MM); i.setA(a); i.setB(b); i.setC(c); return i; }

        static Instruction createDIV_MM(u8 a, u8 b, u8 c) {
            Instruction i; i.setOpCode(OpCode::DIV_MM); i.setA(a); i.setB(b); i.setC(c); return i; }

        static Instruction createMOD_MM(u8 a, u8 b, u8 c) {
            Instruction i; i.setOpCode(OpCode::MOD_MM); i.setA(a); i.setB(b); i.setC(c); return i; }

        static Instruction createPOW_MM(u8 a, u8 b, u8 c) {
            Instruction i; i.setOpCode(OpCode::POW_MM); i.setA(a); i.setB(b); i.setC(c); return i; }

        static Instruction createUNM_MM(u8 a, u8 b) {
            Instruction i; i.setOpCode(OpCode::UNM_MM); i.setA(a); i.setB(b); return i; }

        static Instruction createCONCAT_MM(u8 a, u8 b, u8 c) {
            Instruction i; i.setOpCode(OpCode::CONCAT_MM); i.setA(a); i.setB(b); i.setC(c); return i; }

        static Instruction createEQ_MM(u8 a, u8 b, u8 c) {
            Instruction i; i.setOpCode(OpCode::EQ_MM); i.setA(a); i.setB(b); i.setC(c); return i; }

        static Instruction createLT_MM(u8 a, u8 b, u8 c) {
            Instruction i; i.setOpCode(OpCode::LT_MM); i.setA(a); i.setB(b); i.setC(c); return i; }

        static Instruction createLE_MM(u8 a, u8 b, u8 c) {
            Instruction i; i.setOpCode(OpCode::LE_MM); i.setA(a); i.setB(b); i.setC(c); return i; }
    };
}
