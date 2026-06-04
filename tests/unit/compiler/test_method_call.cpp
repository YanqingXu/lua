/**
 * @file test_method_call.cpp
 * @brief 测试方法调用的代码生成（obj:method(args)）
 *
 * 测试 luaK_self 函数以及方法调用的完整处理
 * 验证第一阶段第二项的实现：方法调用支持
 */

#include "../framework/test_framework.hpp"
#include "compiler/lexer/lexer.hpp"
#include "compiler/parser/parser.hpp"
#include "compiler/codegen/codegen.hpp"
#include "compiler/opcode.hpp"
#include "core/gc_string.hpp"
#include "core/string_pool.hpp"
#include "core/function.hpp"
#include <iostream>
#include <string>

using namespace Lua;
using namespace LuaTest;

/**
 * @brief 测试简单方法调用（无参数）
 * 代码: local result = obj:method()
 * 注意：假设 obj 是全局变量
 */
void testSimpleMethodCall(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();

    const char* code = "local result = obj:method()";
    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");
    ASSERT_TRUE(suite, proto->getInstructionCount() > 0, "Has instructions");

    // 验证生成了 SELF 指令
    bool hasSelf = false;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == OpCode::SELF) {
            hasSelf = true;
            break;
        }
    }
    ASSERT_TRUE(suite, hasSelf, "Generated SELF instruction");

    // 验证生成了 CALL 指令
    bool hasCall = false;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == OpCode::CALL) {
            hasCall = true;
            break;
        }
    }
    ASSERT_TRUE(suite, hasCall, "Generated CALL instruction");

}

/**
 * @brief 测试带参数的方法调用
 * 代码: local result = obj:method(a, b)
 * 注意：假设 obj, a, b 都是全局变量
 */
void testMethodCallWithArgs(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();

    const char* code = "local result = obj:method(a, b)";
    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");

    // 验证生成了 SELF 指令
    bool hasSelf = false;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == OpCode::SELF) {
            hasSelf = true;
            break;
        }
    }
    ASSERT_TRUE(suite, hasSelf, "Generated SELF instruction");

    // 验证生成了 CALL 指令
    bool hasCall = false;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == OpCode::CALL) {
            hasCall = true;
            break;
        }
    }
    ASSERT_TRUE(suite, hasCall, "Generated CALL instruction");

}

/**
 * @brief 测试嵌套方法调用（链式调用）
 * 代码: local result = obj:method1():method2()
 * 注意：假设 obj 是全局变量
 */
void testChainedMethodCall(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();

    const char* code = "local result = obj:method1():method2()";
    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");

    // 验证生成了两个 SELF 指令
    int selfCount = 0;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == OpCode::SELF) {
            selfCount++;
        }
    }
    ASSERT_TRUE(suite, selfCount >= 2, "Generated two SELF instructions");

    // 验证生成了两个 CALL 指令
    int callCount = 0;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == OpCode::CALL) {
            callCount++;
        }
    }
    ASSERT_TRUE(suite, callCount >= 2, "Generated two CALL instructions");

}

/**
 * @brief 测试方法调用与普通调用混合
 * 代码: local result = obj:method(func())
 * 注意：假设 obj 和 func 都是全局变量
 */
void testMethodCallWithFunctionArg(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();

    const char* code = "local result = obj:method(func())";
    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");

    // 验证生成了 SELF 指令（用于方法调用）
    bool hasSelf = false;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == OpCode::SELF) {
            hasSelf = true;
            break;
        }
    }
    ASSERT_TRUE(suite, hasSelf, "Generated SELF instruction");

    // 验证生成了两个 CALL 指令
    int callCount = 0;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == OpCode::CALL) {
            callCount++;
        }
    }
    ASSERT_TRUE(suite, callCount >= 2, "Generated two CALL instructions");

}

/**
 * @brief 测试 SELF 指令的字节码格式
 * 代码: local result = obj:method()
 * 注意：假设 obj 是全局变量
 */
void testSelfInstructionFormat(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();

    const char* code = "local result = obj:method()";
    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");

    // 查找 SELF 指令
    int selfIdx = -1;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == OpCode::SELF) {
            selfIdx = static_cast<int>(i);
            break;
        }
    }
    ASSERT_TRUE(suite, selfIdx >= 0, "Found SELF instruction");

    if (selfIdx >= 0) {
        Instruction selfInst = proto->getInstruction(selfIdx);

        // 验证指令格式
        int A = GETARG_A(selfInst);
        int B = GETARG_B(selfInst);
        int C = GETARG_C(selfInst);

        // A 应该是函数寄存器
        ASSERT_TRUE(suite, A >= 0, "SELF A register is valid");

        // B 应该是对象寄存器
        ASSERT_TRUE(suite, B >= 0, "SELF B register is valid");

        // C 应该是方法名（可能是常量索引）
        ASSERT_TRUE(suite, C >= 0, "SELF C operand is valid");
    }

}

/**
 * @brief 测试方法调用的常量表
 * 代码: local result = obj:method()
 * 注意：假设 obj 是全局变量
 */
void testMethodNameInConstants(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();

    const char* code = "local result = obj:method()";
    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");
    ASSERT_TRUE(suite, proto->getConstantCount() > 0, "Has constants");

    // 验证常量表中有字符串常量（方法名和对象名）
    bool hasStringConstant = false;
    for (size_t i = 0; i < proto->getConstantCount(); i++) {
        Value constant = proto->getConstant(i);
        if (constant.isString()) {
            hasStringConstant = true;
            break;
        }
    }
    ASSERT_TRUE(suite, hasStringConstant, "Constants include string");

}

/**
 * @brief 测试多参数方法调用
 * 代码: local result = obj:method(1, 2, 3)
 * 注意：假设 obj 是全局变量
 */
void testMethodCallMultipleArgs(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();

    const char* code = "local result = obj:method(1, 2, 3)";
    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");

    // 验证生成了 SELF 指令
    bool hasSelf = false;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == OpCode::SELF) {
            hasSelf = true;
            break;
        }
    }
    ASSERT_TRUE(suite, hasSelf, "Generated SELF instruction");

}

/**
 * @brief 测试方法名常量超过 RK 直接编码范围时会先落到寄存器
 */
void testWideMethodNameConstantUsesRegister(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();

    std::string code = "local obj = {}\nlocal sink = {\n";
    for (int i = 0; i < 270; ++i) {
        code += "\"k" + std::to_string(i) + "\",\n";
    }
    code += "}\nlocal result = obj:target()\n";

    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");

    int targetConst = -1;
    for (size_t i = 0; i < proto->getConstantCount(); i++) {
        Value constant = proto->getConstant(i);
        if (constant.isString() && constant.asString()->getData() == "target") {
            targetConst = static_cast<int>(i);
            break;
        }
    }

    ASSERT_TRUE(suite, targetConst > MAXINDEXRK, "Method name constant exceeds RK range");

    int selfIdx = -1;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        if (GET_OPCODE(proto->getInstruction(i)) == OpCode::SELF) {
            selfIdx = static_cast<int>(i);
            break;
        }
    }

    ASSERT_TRUE(suite, selfIdx >= 0, "Found SELF instruction");

    if (selfIdx >= 0 && targetConst >= 0) {
        Instruction selfInst = proto->getInstruction(static_cast<usize>(selfIdx));
        int keyReg = GETARG_C(selfInst);
        ASSERT_TRUE(suite, !ISK(keyReg), "SELF uses register operand for wide method name");

        bool loadedTargetKey = false;
        for (int i = 0; i < selfIdx; ++i) {
            Instruction inst = proto->getInstruction(static_cast<usize>(i));
            if (GET_OPCODE(inst) == OpCode::LOADK &&
                GETARG_A(inst) == keyReg &&
                GETARG_Bx(inst) == targetConst) {
                loadedTargetKey = true;
                break;
            }
        }
        ASSERT_TRUE(suite, loadedTargetKey, "Method name is loaded before SELF");
    }

}

/**
 * @brief 测试方法定义写回字段超过 RK 范围时会先落到寄存器
 */
void testWideMethodDefinitionKeyUsesRegister(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();

    std::string code = "local obj = {}\nlocal sink = {\n";
    for (int i = 0; i < 270; ++i) {
        code += "\"k" + std::to_string(i) + "\",\n";
    }
    code += "}\nfunction obj:target() return self end\n";

    Parser parser(code);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");

    int targetConst = -1;
    for (size_t i = 0; i < proto->getConstantCount(); i++) {
        Value constant = proto->getConstant(i);
        if (constant.isString() && constant.asString()->getData() == "target") {
            targetConst = static_cast<int>(i);
            break;
        }
    }

    ASSERT_TRUE(suite, targetConst > MAXINDEXRK, "Method definition key exceeds RK range");

    bool storesWithLoadedKey = false;
    for (size_t i = 0; i < proto->getInstructionCount(); i++) {
        Instruction inst = proto->getInstruction(i);
        if (GET_OPCODE(inst) != OpCode::SETTABLE) {
            continue;
        }

        int keyReg = GETARG_B(inst);
        if (ISK(keyReg)) {
            continue;
        }

        for (size_t j = 0; j < i; ++j) {
            Instruction before = proto->getInstruction(j);
            if (GET_OPCODE(before) == OpCode::LOADK &&
                GETARG_A(before) == keyReg &&
                GETARG_Bx(before) == targetConst) {
                storesWithLoadedKey = true;
                break;
            }
        }
    }

    ASSERT_TRUE(suite, storesWithLoadedKey, "Method definition key is loaded before SETTABLE");

}

/**
 * @brief 注册所有方法调用测试
 */
void registerMethodCallTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest("Method Call", "Simple Method Call", testSimpleMethodCall);
    registry.registerTest("Method Call", "Method Call With Args", testMethodCallWithArgs);
    registry.registerTest("Method Call", "Chained Method Call", testChainedMethodCall);
    registry.registerTest("Method Call", "Method Call With Function Arg", testMethodCallWithFunctionArg);
    registry.registerTest("Method Call", "SELF Instruction Format", testSelfInstructionFormat);
    registry.registerTest("Method Call", "Method Name In Constants", testMethodNameInConstants);
    registry.registerTest("Method Call", "Multiple Args", testMethodCallMultipleArgs);
    registry.registerTest("Method Call", "Wide Method Name Constant Uses Register", testWideMethodNameConstantUsesRegister);
    registry.registerTest("Method Call", "Wide Method Definition Key Uses Register", testWideMethodDefinitionKeyUsesRegister);
}


