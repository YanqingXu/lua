/**
 * @file opcode.cpp
 * @brief Lua虚拟机指令集实现
 */

#include "compiler/opcode.hpp"

namespace Lua {

// =====================================================================
// 属性访问函数实现
// =====================================================================

OpMode getOpMode(OpCode op) {
    return opcodeMetadata(op).mode;
}

OpArgMask getBMode(OpCode op) {
    return opcodeMetadata(op).bMode;
}

OpArgMask getCMode(OpCode op) {
    return opcodeMetadata(op).cMode;
}

bool testAMode(OpCode op) {
    return opcodeMetadata(op).setsA;
}

bool testTMode(OpCode op) {
    return opcodeMetadata(op).isTest;
}

const char* getOpName(OpCode op) {
    return opcodeMetadata(op).name;
}

}  // namespace Lua

