#pragma once

#include "../common/types.hpp"
#include "../common/opcodes.hpp"

namespace Lua {
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
        
        static Instruction createCLOSE(u8 a) {
            Instruction i; i.setOpCode(OpCode::CLOSE); i.setA(a); return i; }
        
        static Instruction createPOP(u8 a) { Instruction i; i.setOpCode(OpCode::POP); i.setA(a); return i; }
        
        // String concatenation
        static Instruction createCONCAT(u8 a, u8 b, u8 c) {
            Instruction i; i.setOpCode(OpCode::CONCAT); i.setA(a); i.setB(b); i.setC(c); return i; }
    };
}
