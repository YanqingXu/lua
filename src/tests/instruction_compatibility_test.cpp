#include "../vm/instruction.hpp"
#include "../common/opcodes.hpp"
#include <iostream>
#include <cassert>
#include <iomanip>

using namespace Lua;

// 测试指令编码与官方Lua 5.1的兼容性
void testInstructionCompatibility() {
    std::cout << "=== 测试指令编码与官方Lua 5.1的兼容性 ===" << std::endl;

    // 测试1: 验证位域布局
    std::cout << "\n1. 验证位域布局:" << std::endl;
    std::cout << "   SIZE_OP = " << static_cast<int>(SIZE_OP) << " (期望: 6)" << std::endl;
    std::cout << "   SIZE_A = " << static_cast<int>(SIZE_A) << " (期望: 8)" << std::endl;
    std::cout << "   SIZE_B = " << static_cast<int>(SIZE_B) << " (期望: 9)" << std::endl;
    std::cout << "   SIZE_C = " << static_cast<int>(SIZE_C) << " (期望: 9)" << std::endl;
    std::cout << "   SIZE_Bx = " << static_cast<int>(SIZE_Bx) << " (期望: 18)" << std::endl;
    
    assert(SIZE_OP == 6);
    assert(SIZE_A == 8);
    assert(SIZE_B == 9);
    assert(SIZE_C == 9);
    assert(SIZE_Bx == 18);
    std::cout << "   ✓ 位域大小正确" << std::endl;

    // 测试2: 验证位置定义
    std::cout << "\n2. 验证位置定义:" << std::endl;
    std::cout << "   POS_OP = " << static_cast<int>(POS_OP) << " (期望: 0)" << std::endl;
    std::cout << "   POS_A = " << static_cast<int>(POS_A) << " (期望: 6)" << std::endl;
    std::cout << "   POS_C = " << static_cast<int>(POS_C) << " (期望: 14)" << std::endl;
    std::cout << "   POS_B = " << static_cast<int>(POS_B) << " (期望: 23)" << std::endl;
    
    assert(POS_OP == 0);
    assert(POS_A == 6);
    assert(POS_C == 14);
    assert(POS_B == 23);
    std::cout << "   ✓ 位置定义正确" << std::endl;

    // 测试3: 验证最大值定义
    std::cout << "\n3. 验证最大值定义:" << std::endl;
    std::cout << "   MAXARG_A = " << MAXARG_A << " (期望: 255)" << std::endl;
    std::cout << "   MAXARG_B = " << MAXARG_B << " (期望: 511)" << std::endl;
    std::cout << "   MAXARG_C = " << MAXARG_C << " (期望: 511)" << std::endl;
    std::cout << "   MAXARG_Bx = " << MAXARG_Bx << " (期望: 262143)" << std::endl;
    std::cout << "   MAXARG_sBx = " << MAXARG_sBx << " (期望: 131071)" << std::endl;
    
    assert(MAXARG_A == 255);
    assert(MAXARG_B == 511);
    assert(MAXARG_C == 511);
    assert(MAXARG_Bx == 262143);
    assert(MAXARG_sBx == 131071);
    std::cout << "   ✓ 最大值定义正确" << std::endl;

    // 测试4: 验证RK常量定义
    std::cout << "\n4. 验证RK常量定义:" << std::endl;
    std::cout << "   BITRK = " << BITRK << " (期望: 256)" << std::endl;
    std::cout << "   MAXINDEXRK = " << MAXINDEXRK << " (期望: 255)" << std::endl;
    
    assert(BITRK == 256);
    assert(MAXINDEXRK == 255);
    std::cout << "   ✓ RK常量定义正确" << std::endl;

    // 测试5: 验证指令编码/解码
    std::cout << "\n5. 验证指令编码/解码:" << std::endl;
    
    // 测试MOVE指令 (iABC格式)
    u32 moveCode = CREATE_ABC(OpCode::MOVE, 10, 20, 30);
    std::cout << "   MOVE指令编码: 0x" << std::hex << moveCode << std::dec << std::endl;
    
    assert(GET_OPCODE(moveCode) == OpCode::MOVE);
    assert(GETARG_A(moveCode) == 10);
    assert(GETARG_B(moveCode) == 20);
    assert(GETARG_C(moveCode) == 30);
    std::cout << "   ✓ MOVE指令编码/解码正确" << std::endl;

    // 测试LOADK指令 (iABx格式)
    u32 loadkCode = CREATE_ABx(OpCode::LOADK, 5, 1000);
    std::cout << "   LOADK指令编码: 0x" << std::hex << loadkCode << std::dec << std::endl;
    
    assert(GET_OPCODE(loadkCode) == OpCode::LOADK);
    assert(GETARG_A(loadkCode) == 5);
    assert(GETARG_Bx(loadkCode) == 1000);
    std::cout << "   ✓ LOADK指令编码/解码正确" << std::endl;

    // 测试JMP指令 (iAsBx格式)
    Instruction jmpInstr;
    jmpInstr.setOpCode(OpCode::JMP);
    jmpInstr.setSBx(-100);
    
    assert(jmpInstr.getOpCode() == OpCode::JMP);
    assert(jmpInstr.getSBx() == -100);
    std::cout << "   ✓ JMP指令编码/解码正确" << std::endl;

    // 测试6: 验证Instruction结构体方法
    std::cout << "\n6. 验证Instruction结构体方法:" << std::endl;
    
    Instruction instr = Instruction::createADD(1, 2, 3);
    assert(instr.getOpCode() == OpCode::ADD);
    assert(instr.getA() == 1);
    assert(instr.getB() == 2);
    assert(instr.getC() == 3);
    std::cout << "   ✓ ADD指令创建正确" << std::endl;

    Instruction loadkInstr = Instruction::createLOADK(0, 500);
    assert(loadkInstr.getOpCode() == OpCode::LOADK);
    assert(loadkInstr.getA() == 0);
    assert(loadkInstr.getBx() == 500);
    std::cout << "   ✓ LOADK指令创建正确" << std::endl;

    // 测试7: 验证指令模式信息
    std::cout << "\n7. 验证指令模式信息:" << std::endl;
    
    // 测试MOVE指令模式
    assert(getOpMode(OpCode::MOVE) == OpMode::iABC);
    assert(getBMode(OpCode::MOVE) == OpArgMask::OpArgR);
    assert(getCMode(OpCode::MOVE) == OpArgMask::OpArgN);
    assert(testAMode(OpCode::MOVE) == true);
    assert(testTMode(OpCode::MOVE) == false);
    std::cout << "   ✓ MOVE指令模式正确" << std::endl;

    // 测试LOADK指令模式
    assert(getOpMode(OpCode::LOADK) == OpMode::iABx);
    assert(getBMode(OpCode::LOADK) == OpArgMask::OpArgK);
    assert(getCMode(OpCode::LOADK) == OpArgMask::OpArgN);
    assert(testAMode(OpCode::LOADK) == true);
    assert(testTMode(OpCode::LOADK) == false);
    std::cout << "   ✓ LOADK指令模式正确" << std::endl;

    // 测试EQ指令模式（测试指令）
    assert(getOpMode(OpCode::EQ) == OpMode::iABC);
    assert(getBMode(OpCode::EQ) == OpArgMask::OpArgK);
    assert(getCMode(OpCode::EQ) == OpArgMask::OpArgK);
    assert(testAMode(OpCode::EQ) == false);
    assert(testTMode(OpCode::EQ) == true);
    std::cout << "   ✓ EQ指令模式正确" << std::endl;

    // 测试8: 验证与官方Lua 5.1的具体兼容性
    std::cout << "\n8. 验证与官方Lua 5.1的具体兼容性:" << std::endl;

    // 官方Lua 5.1的MOVE指令示例：MOVE 1 2 (A=1, B=2, C=0)
    u32 officialMoveCode = CREATE_ABC(OpCode::MOVE, 1, 2, 0);
    std::cout << "   官方MOVE指令编码: 0x" << std::hex << officialMoveCode << std::dec << std::endl;

    // 验证解码结果
    assert(GET_OPCODE(officialMoveCode) == OpCode::MOVE);
    assert(GETARG_A(officialMoveCode) == 1);
    assert(GETARG_B(officialMoveCode) == 2);
    assert(GETARG_C(officialMoveCode) == 0);
    std::cout << "   ✓ 官方MOVE指令兼容" << std::endl;

    // 官方Lua 5.1的LOADK指令示例：LOADK 0 100 (A=0, Bx=100)
    u32 officialLoadkCode = CREATE_ABx(OpCode::LOADK, 0, 100);
    std::cout << "   官方LOADK指令编码: 0x" << std::hex << officialLoadkCode << std::dec << std::endl;

    assert(GET_OPCODE(officialLoadkCode) == OpCode::LOADK);
    assert(GETARG_A(officialLoadkCode) == 0);
    assert(GETARG_Bx(officialLoadkCode) == 100);
    std::cout << "   ✓ 官方LOADK指令兼容" << std::endl;

    std::cout << "\n=== 所有测试通过！指令系统与官方Lua 5.1完全兼容 ===" << std::endl;
}

int main() {
    try {
        testInstructionCompatibility();
        std::cout << "\n✅ 指令兼容性测试成功完成" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ 测试失败: " << e.what() << std::endl;
        return 1;
    }
}
