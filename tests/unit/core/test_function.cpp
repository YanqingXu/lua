/**
 * @file test_function.cpp
 * @brief Function和Proto类单元测试
 * 
 * @author Lua C++ Project
 * @date 2025-11-14
 */

#include "../framework/test_framework.hpp"
#include "core/function.hpp"
#include "core/upvalue.hpp"
#include "vm/state/lua_state.hpp"

using namespace Lua;
using namespace LuaTest;

void testCFunction(TestSuite& suite) {
    auto testCFunc = [](LuaState* L) -> i32 { return 0; };
    Function* cfunc = new Function(testCFunc);
    
    // Test 1: C function creation
    ASSERT_TRUE(suite, cfunc != nullptr, "C function creation");
    
    // Test 2: Is C function
    ASSERT_TRUE(suite, cfunc->isCFunction(), "Is C function");
    
    // Test 3: Get C function pointer
    CFunction ptr = cfunc->getCFunction();
    ASSERT_TRUE(suite, ptr != nullptr, "Get C function pointer");
    
    // Test 4: GC type
    ASSERT_EQ(suite, GCObjectType::Function, cfunc->getType(), "GC type");
    
    delete cfunc;
}

void testProto(TestSuite& suite) {
    Proto* proto = new Proto();
    
    // Test 1: Proto creation
    ASSERT_TRUE(suite, proto != nullptr, "Proto creation");
    
    // Test 2: Set parameters
    proto->setNumParams(2);
    proto->setVararg(false);
    proto->setMaxStackSize(10);
    
    // Test 3: Get parameters
    ASSERT_EQ(suite, (u8)2, proto->getNumParams(), "NumParams");
    ASSERT_FALSE(suite, proto->isVararg(), "Vararg");
    ASSERT_EQ(suite, (u8)10, proto->getMaxStackSize(), "MaxStackSize");
    
    // Test 4: GC type
    ASSERT_EQ(suite, GCObjectType::Proto, proto->getType(), "GC type");
    
    delete proto;
}

void testProtoConstants(TestSuite& suite) {
    Proto* proto = new Proto();
    
    // Test 1: Add constants
    usize idx1 = proto->addConstant(Value(42.0));
    usize idx2 = proto->addConstant(Value(true));
    ASSERT_EQ(suite, (usize)2, proto->getConstantCount(), "Add constants");
    
    // Test 2: Get constants
    Value c1 = proto->getConstant(idx1);
    ASSERT_TRUE(suite, c1.asNumber() == 42.0, "Get constant");
    
    delete proto;
}

void testProtoInstructions(TestSuite& suite) {
    Proto* proto = new Proto();
    
    // Test 1: Add instructions
    Instruction inst1 = 0x12345678;
    Instruction inst2 = 0xABCDEF00;
    proto->addInstruction(inst1);
    proto->addInstruction(inst2);
    ASSERT_EQ(suite, (usize)2, proto->getInstructionCount(), "Add instructions");
    
    // Test 2: Get instruction
    Instruction i1 = proto->getInstruction(0);
    ASSERT_EQ(suite, inst1, i1, "Get instruction");
    
    // Test 3: Set instruction
    proto->setInstruction(0, 0x11111111);
    Instruction i2 = proto->getInstruction(0);
    ASSERT_EQ(suite, (Instruction)0x11111111, i2, "Set instruction");
    
    delete proto;
}

void testLuaFunction(TestSuite& suite) {
    Proto* proto = new Proto();
    proto->setNumParams(1);
    
    Function* lfunc = new Function(proto);
    
    // Test 1: Lua function creation
    ASSERT_TRUE(suite, lfunc != nullptr, "Lua function creation");
    
    // Test 2: Is Lua function
    ASSERT_TRUE(suite, lfunc->isLuaFunction(), "Is Lua function");
    
    // Test 3: Get Proto
    Proto* p = lfunc->getProto();
    ASSERT_TRUE(suite, p == proto, "Get Proto");
    
    delete lfunc;
    delete proto;
}

void testFunctionUpvalues(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    L->pushNumber(42.0);
    L->pushNumber(100.0);
    
    Upvalue* uv1 = L->findOrCreateUpvalue(1);
    Upvalue* uv2 = L->findOrCreateUpvalue(2);
    
    Proto* proto = new Proto();
    Function* func = new Function(proto);
    
    // Test 1: Add upvalues
    func->addUpvalue(uv1);
    func->addUpvalue(uv2);
    ASSERT_EQ(suite, (usize)2, func->getUpvalueCount(), "Add upvalues");
    
    // Test 2: Get upvalue
    ASSERT_TRUE(suite, func->getUpvalue(0) == uv1, "Get upvalue");
    
    delete L;
    delete func;
    delete proto;
}

void testFunctionEnvironment(TestSuite& suite) {
    Proto* proto = new Proto();
    Function* func = new Function(proto);
    Table* env = new Table();
    
    // Test 1: Set environment
    func->setEnv(env);
    
    // Test 2: Get environment
    ASSERT_TRUE(suite, func->getEnv() == env, "Environment set/get");
    
    delete env;
    delete func;
    delete proto;
}

void registerFunctionTests() {
    auto& registry = TestRegistry::getInstance();
    
    registry.registerTest("Function", "C Function", testCFunction);
    registry.registerTest("Function", "Proto", testProto);
    registry.registerTest("Function", "Proto Constants", testProtoConstants);
    registry.registerTest("Function", "Proto Instructions", testProtoInstructions);
    registry.registerTest("Function", "Lua Function", testLuaFunction);
    registry.registerTest("Function", "Upvalues", testFunctionUpvalues);
    registry.registerTest("Function", "Environment", testFunctionEnvironment);
}

