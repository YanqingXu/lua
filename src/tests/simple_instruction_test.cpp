#include <iostream>
#include "../vm/instruction.hpp"
#include "../common/opcodes.hpp"

using namespace Lua;

int main() {
    std::cout << "=== 简单指令兼容性测试 ===" << std::endl;

    // 测试基本常量
    std::cout << "SIZE_OP = " << static_cast<int>(SIZE_OP) << std::endl;
    std::cout << "SIZE_A = " << static_cast<int>(SIZE_A) << std::endl;
    std::cout << "SIZE_B = " << static_cast<int>(SIZE_B) << std::endl;
    std::cout << "SIZE_C = " << static_cast<int>(SIZE_C) << std::endl;
    std::cout << "BITRK = " << BITRK << std::endl;

    // 测试指令创建
    Instruction moveInstr = Instruction::createMOVE(1, 2);
    std::cout << "MOVE指令: OpCode=" << static_cast<int>(moveInstr.getOpCode()) 
              << " A=" << static_cast<int>(moveInstr.getA())
              << " B=" << static_cast<int>(moveInstr.getB()) << std::endl;

    // 测试宏函数
    u32 testCode = CREATE_ABC(OpCode::ADD, 5, 10, 15);
    std::cout << "ADD指令编码: 0x" << std::hex << testCode << std::dec << std::endl;
    std::cout << "解码: OpCode=" << static_cast<int>(GET_OPCODE(testCode))
              << " A=" << static_cast<int>(GETARG_A(testCode))
              << " B=" << static_cast<int>(GETARG_B(testCode))
              << " C=" << static_cast<int>(GETARG_C(testCode)) << std::endl;

    // 测试指令模式
    std::cout << "MOVE指令模式: " << static_cast<int>(getOpMode(OpCode::MOVE)) << std::endl;
    std::cout << "LOADK指令模式: " << static_cast<int>(getOpMode(OpCode::LOADK)) << std::endl;

    std::cout << "测试完成！" << std::endl;
    return 0;
}
