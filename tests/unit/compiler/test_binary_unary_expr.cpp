/**
 * @file test_binary_unary_expr.cpp
 * @brief 测试二元和一元表达式的代码生成
 */

#include "../framework/test_framework.hpp"
#include "compiler/lexer.hpp"
#include "compiler/parser.hpp"
#include "compiler/codegen.hpp"
#include "compiler/opcode.hpp"
#include "core/string_pool.hpp"
#include "core/function.hpp"
#include <iostream>
#include <cassert>
#include <array>

using namespace Lua;
using namespace LuaTest;

namespace {

Proto* generateProto(const char* code) {
    StringPool& pool = StringPool::getInstance();
    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    CodeGenerator codegen(&pool);
    return codegen.generate(chunk);
}

bool hasOpcode(const Proto* proto, OpCode op) {
    for (usize i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == op) {
            return true;
        }
    }
    return false;
}

bool hasComparisonMaterializationPattern(const Proto* proto, OpCode compareOp) {
    for (usize i = 0; i + 3 < proto->getInstructionCount(); i++) {
        Instruction compareInst = proto->getInstruction(i);
        Instruction jmpInst = proto->getInstruction(i + 1);
        Instruction falseInst = proto->getInstruction(i + 2);
        Instruction trueInst = proto->getInstruction(i + 3);

        if (GET_OPCODE(compareInst) != compareOp) {
            continue;
        }
        if (GET_OPCODE(jmpInst) != OpCode::JMP) {
            continue;
        }
        if (GET_OPCODE(falseInst) != OpCode::LOADBOOL || GETARG_B(falseInst) != 0 || GETARG_C(falseInst) != 1) {
            continue;
        }
        if (GET_OPCODE(trueInst) != OpCode::LOADBOOL || GETARG_B(trueInst) != 1 || GETARG_C(trueInst) != 0) {
            continue;
        }
        return true;
    }
    return false;
}

bool hasSelfLoopJump(const Proto* proto) {
    for (usize i = 0; i < proto->getInstructionCount(); i++) {
        Instruction inst = proto->getInstruction(i);
        if (GET_OPCODE(inst) == OpCode::JMP && GETARG_sBx(inst) == -1) {
            return true;
        }
    }
    return false;
}

bool hasResolvedTestJumpPattern(const Proto* proto) {
    for (usize i = 0; i + 2 < proto->getInstructionCount(); i++) {
        Instruction testInst = proto->getInstruction(i);
        Instruction jmpInst = proto->getInstruction(i + 1);
        Instruction followInst = proto->getInstruction(i + 2);

        if (GET_OPCODE(testInst) != OpCode::TEST) {
            continue;
        }
        if (GET_OPCODE(jmpInst) != OpCode::JMP || GETARG_sBx(jmpInst) == -1) {
            continue;
        }
        if (GET_OPCODE(followInst) != OpCode::MOVE) {
            continue;
        }
        return true;
    }
    return false;
}

bool matchesExactComparisonBooleanSequence(const Proto* proto, OpCode compareOp) {
    if (proto->getInstructionCount() < 5) {
        return false;
    }

    Instruction compareInst = proto->getInstruction(0);
    Instruction jmpInst = proto->getInstruction(1);
    Instruction falseInst = proto->getInstruction(2);
    Instruction trueInst = proto->getInstruction(3);

    return GET_OPCODE(compareInst) == compareOp
        && GET_OPCODE(jmpInst) == OpCode::JMP
        && GETARG_sBx(jmpInst) == 1
        && GET_OPCODE(falseInst) == OpCode::LOADBOOL
        && GETARG_B(falseInst) == 0
        && GETARG_C(falseInst) == 1
        && GET_OPCODE(trueInst) == OpCode::LOADBOOL
        && GETARG_B(trueInst) == 1
        && GETARG_C(trueInst) == 0;
}

} // namespace

void testBinaryArithmetic(TestSuite& suite) {
    // 测试: local x = 1 + 2
    const char* code = "local x = 1 + 2";
    Proto* proto = generateProto(code);

    // 验证生成的字节码
    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");
    ASSERT_TRUE(suite, proto->getInstructionCount() > 0, "Has instructions");

    delete proto;
}

void testBinaryComparison(TestSuite& suite) {
    // 测试: local x = 1 < 2
    const char* code = "local x = 1 < 2";
    Proto* proto = generateProto(code);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");
    ASSERT_TRUE(suite, proto->getInstructionCount() > 0, "Has instructions");
    ASSERT_TRUE(suite, hasOpcode(proto, OpCode::LT), "Has LT instruction");
    ASSERT_TRUE(suite, hasComparisonMaterializationPattern(proto, OpCode::LT), "Comparison materializes to booleans");
    ASSERT_FALSE(suite, hasSelfLoopJump(proto), "Comparison has no self-loop JMP");

    delete proto;
}

void testComparisonOperatorsMaterializeToBoolean(TestSuite& suite) {
    struct Case {
        const char* code;
        OpCode compareOp;
        const char* opcodeMessage;
        const char* patternMessage;
        const char* jumpMessage;
    };

    const std::array<Case, 3> cases{{
        {"local x = 1 <= 2", OpCode::LE, "Has LE instruction for <=", "Comparison <= materializes to booleans", "<= has no self-loop JMP"},
        {"local x = 1 ~= 2", OpCode::EQ, "Has EQ instruction for ~=", "Comparison ~= materializes to booleans", "~= has no self-loop JMP"},
        {"local x = 1 == 1", OpCode::EQ, "Has EQ instruction for ==", "Comparison == materializes to booleans", "== has no self-loop JMP"},
    }};

    for (const auto& testCase : cases) {
        Proto* proto = generateProto(testCase.code);

        ASSERT_TRUE(suite, proto != nullptr, "Comparison proto generated");
        ASSERT_TRUE(suite, proto->getInstructionCount() > 0, "Comparison proto has instructions");
        ASSERT_TRUE(suite, hasOpcode(proto, testCase.compareOp), testCase.opcodeMessage);
        ASSERT_TRUE(suite, hasComparisonMaterializationPattern(proto, testCase.compareOp), testCase.patternMessage);
        ASSERT_FALSE(suite, hasSelfLoopJump(proto), testCase.jumpMessage);

        delete proto;
    }
}

void testGreaterComparisonsMaterializeToBoolean(TestSuite& suite) {
    {
        Proto* proto = generateProto("local x = 2 > 1");

        ASSERT_TRUE(suite, proto != nullptr, "> proto generated");
        ASSERT_TRUE(suite, proto->getInstructionCount() >= 5, "> proto has expected instruction count");
        ASSERT_TRUE(suite, matchesExactComparisonBooleanSequence(proto, OpCode::LT), "> lowers to LT + boolean materialization");
        ASSERT_FALSE(suite, hasSelfLoopJump(proto), "> has no self-loop JMP");

        delete proto;
    }

    {
        Proto* proto = generateProto("local x = 2 >= 1");

        ASSERT_TRUE(suite, proto != nullptr, ">= proto generated");
        ASSERT_TRUE(suite, proto->getInstructionCount() >= 5, ">= proto has expected instruction count");
        ASSERT_TRUE(suite, matchesExactComparisonBooleanSequence(proto, OpCode::LE), ">= lowers to LE + boolean materialization");
        ASSERT_FALSE(suite, hasSelfLoopJump(proto), ">= has no self-loop JMP");

        delete proto;
    }
}

void testBinaryLogical(TestSuite& suite) {
    const char* code =
        "local a = ...\n"
        "local b = ...\n"
        "local x = a and b\n"
        "local y = a or b\n"
        "local z = not a\n";
    Proto* proto = generateProto(code);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");
    ASSERT_TRUE(suite, proto->getInstructionCount() > 0, "Has instructions");
    ASSERT_TRUE(suite, hasOpcode(proto, OpCode::TEST), "Logical expression uses TEST");
    ASSERT_TRUE(suite, hasResolvedTestJumpPattern(proto), "Logical short-circuit jump is resolved");
    ASSERT_FALSE(suite, hasOpcode(proto, OpCode::NOT), "Logical not now avoids OP_NOT");
    ASSERT_FALSE(suite, hasSelfLoopJump(proto), "Logical expressions have no self-loop JMP");

    delete proto;
}

void testLogicalValueExpressions(TestSuite& suite) {
    {
        Proto* proto = generateProto("local a = ...\nlocal b = ...\nlocal x = a and b\n");

        ASSERT_TRUE(suite, proto != nullptr, "Logical proto generated");
        ASSERT_TRUE(suite, proto->getInstructionCount() > 0, "Logical proto has instructions");
        ASSERT_TRUE(suite, hasOpcode(proto, OpCode::TEST), "and expression uses TEST");
        ASSERT_TRUE(suite, hasResolvedTestJumpPattern(proto), "and expression TEST/JMP pattern is resolved");
        ASSERT_FALSE(suite, hasSelfLoopJump(proto), "and expression has no self-loop JMP");

        delete proto;
    }

    {
        Proto* proto = generateProto("local a = ...\nlocal b = ...\nlocal x = a or b\n");

        ASSERT_TRUE(suite, proto != nullptr, "Logical proto generated");
        ASSERT_TRUE(suite, proto->getInstructionCount() > 0, "Logical proto has instructions");
        ASSERT_TRUE(suite, hasOpcode(proto, OpCode::TEST), "or expression uses TEST");
        ASSERT_TRUE(suite, hasResolvedTestJumpPattern(proto), "or expression TEST/JMP pattern is resolved");
        ASSERT_FALSE(suite, hasSelfLoopJump(proto), "or expression has no self-loop JMP");

        delete proto;
    }

    {
        Proto* proto = generateProto("local a = ...\nlocal x = not a\n");

        ASSERT_TRUE(suite, proto != nullptr, "Logical proto generated");
        ASSERT_TRUE(suite, proto->getInstructionCount() > 0, "Logical proto has instructions");
        ASSERT_TRUE(suite, hasOpcode(proto, OpCode::TEST), "not expression uses condition TEST");
        ASSERT_FALSE(suite, hasOpcode(proto, OpCode::NOT), "not expression avoids OP_NOT");
        ASSERT_FALSE(suite, hasSelfLoopJump(proto), "not expression has no self-loop JMP");

        delete proto;
    }
}

void testUnaryExpressions(TestSuite& suite) {
    // 测试: local x = -42
    const char* code = "local x = -42";
    Proto* proto = generateProto(code);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");
    ASSERT_TRUE(suite, proto->getInstructionCount() > 0, "Has instructions");

    delete proto;
}

void testComplexExpression(TestSuite& suite) {
    // 测试: local x = (1 + 2) * 3 - 4
    const char* code = "local x = (1 + 2) * 3 - 4";
    Proto* proto = generateProto(code);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");
    ASSERT_TRUE(suite, proto->getInstructionCount() > 0, "Has instructions");

    delete proto;
}

void registerBinaryUnaryExprTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest("Binary/Unary Expressions", "Binary Arithmetic", testBinaryArithmetic);
    registry.registerTest("Binary/Unary Expressions", "Binary Comparison", testBinaryComparison);
    registry.registerTest("Binary/Unary Expressions", "Comparison Operators Materialize To Boolean", testComparisonOperatorsMaterializeToBoolean);
    registry.registerTest("Binary/Unary Expressions", "Greater Comparisons Materialize To Boolean", testGreaterComparisonsMaterializeToBoolean);
    registry.registerTest("Binary/Unary Expressions", "Binary Logical", testBinaryLogical);
    registry.registerTest("Binary/Unary Expressions", "Logical Value Expressions", testLogicalValueExpressions);
    registry.registerTest("Binary/Unary Expressions", "Unary Expressions", testUnaryExpressions);
    registry.registerTest("Binary/Unary Expressions", "Complex Expression", testComplexExpression);
}

