/**
 * @file test_vm_core.cpp
 * @brief VM核心类单元测试 (GlobalState, Stack, CallInfo, LuaState)
 * 
 * @author Lua C++ Project
 * @date 2025-11-14
 */

#include "../framework/test_framework.hpp"
#include "vm/global_state.hpp"
#include "vm/stack.hpp"
#include "vm/call_info.hpp"
#include "vm/lua_state.hpp"
#include "core/value.hpp"

using namespace Lua;
using namespace LuaTest;

void testGlobalState(TestSuite& suite) {
    GlobalState& gs = GlobalState::getInstance();
    
    // Test 1: Singleton
    ASSERT_TRUE(suite, &gs == &GlobalState::getInstance(), "GlobalState singleton");
    
    // Test 2: getStringPool
    StringPool& pool = gs.getStringPool();
    ASSERT_TRUE(suite, &pool == &StringPool::getInstance(), "getStringPool");
    
    // Test 3: getGC
    GarbageCollector& gc = gs.getGC();
    ASSERT_TRUE(suite, &gc == &GarbageCollector::getInstance(), "getGC");
    
    // Test 4: getRegistry
    Table* registry = gs.getRegistry();
    ASSERT_TRUE(suite, registry != nullptr, "getRegistry");
}

void testStackOperations(TestSuite& suite) {
    Stack stack;
    
    // Test 1: Stack creation
    ASSERT_TRUE(suite, stack.empty(), "Stack creation");
    
    // Test 2: Push operations
    stack.push(Value(1.0));
    stack.push(Value(2.0));
    stack.push(Value(true));
    ASSERT_EQ(suite, (usize)3, stack.size(), "Push operations");
    
    // Test 3: Top value
    Value topVal = stack.top();
    ASSERT_TRUE(suite, topVal.isBoolean(), "Top value");
    
    // Test 4: Pop operation
    Value popped = stack.pop();
    ASSERT_EQ(suite, (usize)2, stack.size(), "Pop operation");
    
    // Test 5: At operation
    Value val = stack.at(0);
    ASSERT_TRUE(suite, val.asNumber() == 1.0, "At operation");
    
    // Test 6: Clear
    stack.clear();
    ASSERT_TRUE(suite, stack.empty(), "Clear");
}

void testCallInfo(TestSuite& suite) {
    CallInfo ci;
    
    // Test 1: CallInfo creation
    ASSERT_TRUE(suite, ci.func == 0, "CallInfo creation");
    
    // Test 2: Set values
    ci.func = 10;
    ci.base = 11;
    ci.top = 20;
    ci.nresults = 2;
    ASSERT_TRUE(suite, ci.func == 10 && ci.base == 11, "Set values");
    
    // Test 3: Reset
    ci.reset();
    ASSERT_TRUE(suite, ci.func == 0 && ci.base == 0, "Reset");
}

void testLuaStateCreation(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    
    // Test 1: LuaState creation
    ASSERT_TRUE(suite, L != nullptr, "LuaState creation");
    
    // Test 2: Initial status
    ASSERT_EQ(suite, ThreadStatus::OK, L->getStatus(), "Initial status");
    
    // Test 3: Global table
    Table* globalTable = L->getGlobalTable();
    ASSERT_TRUE(suite, globalTable != nullptr, "Global table");
    
    delete L;
}

void testLuaStateStackOperations(TestSuite& suite) {
    LuaState* L = LuaState::newState();

    // Note: LuaState uses 1-based indexing (Lua style)
    // Initial stack has one nil value at stack_[0] (virtual function slot)
    // getTop() returns total stack size including the initial nil

    // Test 1: Get initial top
    i32 initialTop = L->getTop();

    // Test 2: Push operations
    L->pushNumber(42.0);
    L->pushBoolean(true);
    L->pushString(L->getGlobalState().getStringPool().intern("test"));

    // Test 3: Check top after pushes
    i32 top = L->getTop();
    ASSERT_EQ(suite, initialTop + 3, top, "Get top");

    // Test 4: Type checking using negative indices (from top)
    // -1 is the last pushed value, -2 is second to last, etc.
    ASSERT_TRUE(suite, L->isString(-1), "isString at index -1");
    ASSERT_TRUE(suite, L->isBoolean(-2), "isBoolean at index -2");
    ASSERT_TRUE(suite, L->isNumber(-3), "isNumber at index -3");

    // Test 5: Value access using negative indices
    f64 num = L->toNumber(-3);
    ASSERT_EQ(suite, 42.0, num, "toNumber");

    bool b = L->toBoolean(-2);
    ASSERT_TRUE(suite, b, "toBoolean");

    delete L;
}

void testLuaStateGlobalVariables(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    
    // Test 1: Set global
    L->setGlobal("testVar", Value(123.0));
    
    // Test 2: Get global
    Value val = L->getGlobal("testVar");
    ASSERT_TRUE(suite, val.isNumber() && val.asNumber() == 123.0, "Global variable set/get");
    
    delete L;
}

void registerVMCoreTests() {
    auto& registry = TestRegistry::getInstance();
    
    registry.registerTest("VM Core", "GlobalState", testGlobalState);
    registry.registerTest("VM Core", "Stack Operations", testStackOperations);
    registry.registerTest("VM Core", "CallInfo", testCallInfo);
    registry.registerTest("VM Core", "LuaState Creation", testLuaStateCreation);
    registry.registerTest("VM Core", "LuaState Stack", testLuaStateStackOperations);
    registry.registerTest("VM Core", "LuaState Globals", testLuaStateGlobalVariables);
}

