#pragma once

#include "../types.hpp"
#include "../common/opcodes.hpp"

namespace Lua {
    // 指令格式（基于Lua 5.1指令格式简化）
    // 32位指令，包含操作码和操作数
    struct Instruction {
        u32 code;
        
        // 构造函数
        Instruction() : code(0) {}
        explicit Instruction(u32 c) : code(c) {}
        
        // 获取操作码
        OpCode getOpCode() const {
            return static_cast<OpCode>((code >> 24) & 0xFF);
        }
        
        // 设置操作码
        void setOpCode(OpCode op) {
            code = (code & 0x00FFFFFF) | (static_cast<u32>(op) << 24);
        }
        
        // 获取A操作数 (8位)
        u8 getA() const {
            return (code >> 16) & 0xFF;
        }
        
        // 设置A操作数
        void setA(u8 a) {
            code = (code & 0xFF00FFFF) | (static_cast<u32>(a) << 16);
        }
        
        // 获取B操作数 (8位)
        u8 getB() const {
            return (code >> 8) & 0xFF;
        }
        
        // 设置B操作数
        void setB(u8 b) {
            code = (code & 0xFFFF00FF) | (static_cast<u32>(b) << 8);
        }
        
        // 获取C操作数 (8位)
        u8 getC() const {
            return code & 0xFF;
        }
        
        // 设置C操作数
        void setC(u8 c) {
            code = (code & 0xFFFFFF00) | c;
        }
        
        // 获取Bx操作数 (16位无符号)
        u16 getBx() const {
            return code & 0xFFFF;
        }
        
        // 设置Bx操作数
        void setBx(u16 bx) {
            code = (code & 0xFFFF0000) | bx;
        }
        
        // 获取sBx操作数 (16位有符号，偏移32768)
        i16 getSBx() const {
            return getBx() - 32768;
        }
        
        // 设置sBx操作数
        void setSBx(i16 sbx) {
            setBx(sbx + 32768);
        }
        
        // 创建各种指令的静态方法
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
        
        // 加载nil
        static Instruction createLOADNIL(u8 a) {
            Instruction i;
            i.setOpCode(OpCode::LOADNIL);
            i.setA(a);
            return i;
        }
        
        // 加载布尔值 (Lua LOADBOOL uses B as value (0/1) and C as skip flag)
        static Instruction createLOADBOOL(u8 a, bool value) {
            Instruction i;
            i.setOpCode(OpCode::LOADBOOL);
            i.setA(a);
            i.setB(static_cast<u8>(value));
            i.setC(0);
            return i;
        }
        
        // 算术
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
        
        // 一元运算
        static Instruction createUNM(u8 a, u8 b) { // dst=a, operand=b
            Instruction i; i.setOpCode(OpCode::UNM); i.setA(a); i.setB(b); return i; }
        static Instruction createNOT(u8 a, u8 b) {
            Instruction i; i.setOpCode(OpCode::NOT); i.setA(a); i.setB(b); return i; }
        
        // 比较
        static Instruction createEQ(u8 a, u8 b, u8 c) { Instruction i; i.setOpCode(OpCode::EQ); i.setA(a); i.setB(b); i.setC(c); return i; }
        static Instruction createLT(u8 a, u8 b, u8 c) { Instruction i; i.setOpCode(OpCode::LT); i.setA(a); i.setB(b); i.setC(c); return i; }
        static Instruction createLE(u8 a, u8 b, u8 c) { Instruction i; i.setOpCode(OpCode::LE); i.setA(a); i.setB(b); i.setC(c); return i; }
        
        // 跳转 (sBx)
        static Instruction createJMP(i16 sbx) {
            Instruction i; i.setOpCode(OpCode::JMP); i.setSBx(sbx); return i; }
        
        static Instruction createCLOSE(u8 a) {
            Instruction i; i.setOpCode(OpCode::CLOSE); i.setA(a); return i; }
        
        static Instruction createPOP(u8 a) { Instruction i; i.setOpCode(OpCode::POP); i.setA(a); return i; }
    };
}
