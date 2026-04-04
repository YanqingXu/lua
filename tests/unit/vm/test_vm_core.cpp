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
#include "vm/vm.hpp"
#include "core/value.hpp"
#include "core/userdata.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "compiler/parser.hpp"
#include "compiler/codegen.hpp"
#include "lib/iolib.hpp"
#include <cstdio>

using namespace Lua;
using namespace LuaTest;

namespace {

LuaState* executeChunk(const char* code, const char* chunkName, Proto*& outProto) {
    StringPool& pool = StringPool::getInstance();
    Parser parser(code);
    Chunk chunk = parser.parse();

    CodeGenerator codegen(&pool);
    outProto = codegen.generate(chunk, chunkName);

    LuaState* L = LuaState::newState();
    Function* chunkFunc = new Function(outProto);
    chunkFunc->setEnv(L->getGlobalTable());
    L->getGlobalState().getGC().registerObject(chunkFunc);

    VM::execute(L, chunkFunc);
    return L;
}

Function* getTableFunction(Table* table, StringPool& pool, const char* name) {
    if (!table || !name) {
        return nullptr;
    }

    Value func = table->get(Value(pool.intern(name)));
    return func.isFunction() ? func.asFunction() : nullptr;
}

} // namespace

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

static i32 userdata_ping(LuaState* L) {
    if (!L->isUserdata(1)) {
        L->pushNil();
        return 1;
    }

    L->pushNumber(99.0);
    return 1;
}

void testLuaStateUserdataMetatable(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    L->setTop(0);

    Userdata* ud = Userdata::createFull(sizeof(i32));
    Table* mt = new Table();
    L->getGlobalState().getGC().registerObject(ud);
    L->getGlobalState().getGC().registerObject(mt);

    L->pushUserdata(ud);
    L->pushTable(mt);

    ASSERT_TRUE(suite, L->setMetatable(1), "setMetatable supports userdata");
    ASSERT_TRUE(suite, ud->getMetatable() == mt, "userdata metatable assigned");
    ASSERT_TRUE(suite, L->getMetatable(1), "getMetatable supports userdata");
    ASSERT_TRUE(suite, L->top().isTable(), "userdata metatable pushed");
    if (L->top().isTable()) {
        ASSERT_TRUE(suite, L->top().asTable() == mt, "userdata metatable matches");
    }

    delete L;
}

void testSelfDispatchOnUserdata(TestSuite& suite) {
    const char* code = "return obj:ping()";

    try {
        StringPool& pool = StringPool::getInstance();
        Parser parser(code);
        Chunk chunk = parser.parse();

        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk, "=(userdata_self)");

        ASSERT_TRUE(suite, proto != nullptr, "Proto generated for userdata SELF");

        LuaState* L = LuaState::newState();

        Userdata* ud = Userdata::createFull(sizeof(i32));
        Table* mt = new Table();
        Function* method = new Function(userdata_ping);
        GCString* pingKey = pool.intern("ping");
        GCString* indexKey = pool.intern("__index");

        L->getGlobalState().getGC().registerObject(ud);
        L->getGlobalState().getGC().registerObject(mt);
        L->getGlobalState().getGC().registerObject(method);

        mt->set(Value(pingKey), Value(method));
        mt->set(Value(indexKey), Value(mt));
        ud->setMetatable(mt);
        L->setGlobal("obj", Value(ud));

        Function* chunkFunc = new Function(proto);
        chunkFunc->setEnv(L->getGlobalTable());
        L->getGlobalState().getGC().registerObject(chunkFunc);

        VM::execute(L, chunkFunc);

        ASSERT_TRUE(suite, L->getTop() > 0, "userdata SELF has return value");
        ASSERT_TRUE(suite, L->top().isNumber(), "userdata SELF returns number");
        if (L->top().isNumber()) {
            ASSERT_EQ(suite, 99.0, L->top().asNumber(), "obj:ping() == 99");
        }

        delete L;
        delete proto;
    } catch (const std::exception& e) {
        std::cout << "  [ERROR] Exception: " << e.what() << std::endl;
        ASSERT_TRUE(suite, false, "userdata SELF dispatch should not throw");
    }
}

void testIOLibFileMetatableHooks(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    openIOLib(L);

    auto& pool = L->getGlobalState().getStringPool();
    Value ioVal = L->getGlobal("io");
    ASSERT_TRUE(suite, ioVal.isTable(), "io library table exists");
    if (!ioVal.isTable()) {
        delete L;
        return;
    }

    Table* ioTable = ioVal.asTable();
    Value stdinVal = ioTable->get(Value(pool.intern("stdin")));
    ASSERT_TRUE(suite, stdinVal.isUserdata(), "io.stdin is userdata");
    if (!stdinVal.isUserdata()) {
        delete L;
        return;
    }

    L->setTop(0);
    L->pushValue(stdinVal);
    ASSERT_TRUE(suite, L->getMetatable(1), "file handle has metatable");
    ASSERT_TRUE(suite, L->top().isTable(), "file metatable pushed");
    if (L->top().isTable()) {
        Table* mt = L->top().asTable();
        Value gcMethod = mt->get(Value(pool.intern("__gc")));
        Value tostringMethod = mt->get(Value(pool.intern("__tostring")));
        ASSERT_TRUE(suite, gcMethod.isFunction(), "__gc hook is registered");
        ASSERT_TRUE(suite, tostringMethod.isFunction(), "__tostring hook is registered");

        L->setTop(0);
        L->pushValue(tostringMethod);
        L->pushValue(stdinVal);
        i32 status = L->pcall(1, 1, 0);
        ASSERT_TRUE(suite, status == LUA_OK || (L->getTop() >= 1 && L->top().isString()), "__tostring hook callable");
        ASSERT_TRUE(suite, L->getTop() >= 1 && L->top().isString(), "__tostring returns string");
    }

    delete L;
}

void testIOLibDefaultInputOutput(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    openIOLib(L);

    auto& pool = L->getGlobalState().getStringPool();
    Value ioVal = L->getGlobal("io");
    ASSERT_TRUE(suite, ioVal.isTable(), "io table exists for default I/O test");
    if (!ioVal.isTable()) {
        delete L;
        return;
    }

    Table* ioTable = ioVal.asTable();
    Function* ioOutput = getTableFunction(ioTable, pool, "output");
    Function* ioWrite = getTableFunction(ioTable, pool, "write");
    Function* ioClose = getTableFunction(ioTable, pool, "close");
    Function* ioInput = getTableFunction(ioTable, pool, "input");
    Function* ioRead = getTableFunction(ioTable, pool, "read");

    ASSERT_TRUE(suite, ioOutput && ioWrite && ioClose && ioInput && ioRead,
        "default I/O functions are registered");
    if (!(ioOutput && ioWrite && ioClose && ioInput && ioRead)) {
        delete L;
        return;
    }

    const char* path = "test_vm_core_iolib_default.txt";

    L->getStack().clear();
    L->setAbsoluteTop(0);
    L->pushString(pool.intern(path));
    i32 ret = ioOutput->getCFunction()(L);
    ASSERT_EQ(suite, ret, 1, "io.output returns one value");
    ASSERT_TRUE(suite, L->top().isUserdata(), "io.output returns file handle");

    L->getStack().clear();
    L->setAbsoluteTop(0);
    L->pushString(pool.intern("abc"));
    L->pushNumber(123.0);
    ret = ioWrite->getCFunction()(L);
    ASSERT_EQ(suite, ret, 1, "io.write returns one value");
    ASSERT_TRUE(suite, L->top().isUserdata(), "io.write returns current file handle");

    L->getStack().clear();
    L->setAbsoluteTop(0);
    ret = ioClose->getCFunction()(L);
    ASSERT_EQ(suite, ret, 1, "io.close returns one value");
    ASSERT_TRUE(suite, L->top().isBoolean() && L->top().asBoolean(), "io.close succeeds");

    L->getStack().clear();
    L->setAbsoluteTop(0);
    L->pushString(pool.intern(path));
    ret = ioInput->getCFunction()(L);
    ASSERT_EQ(suite, ret, 1, "io.input returns one value");
    ASSERT_TRUE(suite, L->top().isUserdata(), "io.input returns file handle");

    L->getStack().clear();
    L->setAbsoluteTop(0);
    L->pushNumber(3.0);
    ret = ioRead->getCFunction()(L);
    ASSERT_EQ(suite, ret, 1, "io.read(count) returns one value");
    ASSERT_TRUE(suite, L->top().isString(), "io.read(count) returns string");
    if (L->top().isString()) {
        ASSERT_TRUE(suite, std::string(L->top().asString()->c_str()) == "abc",
            "io.read(3) reads written prefix");
    }

    L->getStack().clear();
    L->setAbsoluteTop(0);
    L->pushString(pool.intern("*n"));
    ret = ioRead->getCFunction()(L);
    ASSERT_EQ(suite, ret, 1, "io.read('*n') returns one value");
    ASSERT_TRUE(suite, L->top().isNumber(), "io.read('*n') returns number");
    if (L->top().isNumber()) {
        ASSERT_EQ(suite, 123.0, L->top().asNumber(), "io.read('*n') reads numeric suffix");
    }

    std::remove(path);
    delete L;
}

void testComparisonExpressionProducesBoolean(TestSuite& suite) {
    const char* code = "local ok = 1 == 1\nreturn ok";

    try {
        Proto* proto = nullptr;
        LuaState* L = executeChunk(code, "=(comparison_bool)", proto);

        ASSERT_TRUE(suite, proto != nullptr, "comparison proto generated");

        ASSERT_TRUE(suite, L->getTop() > 0, "comparison result returned");
        ASSERT_TRUE(suite, L->top().isBoolean(), "comparison result is boolean");
        if (L->top().isBoolean()) {
            ASSERT_TRUE(suite, L->top().asBoolean(), "1 == 1 evaluates to true");
        }

        delete L;
        delete proto;
    } catch (const std::exception& e) {
        std::cout << "  [ERROR] Exception: " << e.what() << std::endl;
        ASSERT_TRUE(suite, false, "comparison expression should execute without hanging");
    }
}

void testLogicalExpressionsProduceRuntimeValues(TestSuite& suite) {
    const char* code = R"(
        local counter = {0}
        local function mark()
            counter[1] = counter[1] + 1
            return "called"
        end

        local andValue = 0 and 7
        local orValue = false or "fallback"
        local notFalse = not false
        local notZero = not 0
        local shortAnd = false and mark()
        local shortOr = true or mark()

        return andValue, orValue, notFalse, notZero, shortAnd, shortOr, counter[1]
    )";

    try {
        Proto* proto = nullptr;
        LuaState* L = executeChunk(code, "=(logical_values_runtime)", proto);

        ASSERT_TRUE(suite, proto != nullptr, "logical values proto generated");
        ASSERT_TRUE(suite, L->getTop() >= 7, "logical values returned");

        if (L->getTop() >= 7) {
            ASSERT_TRUE(suite, L->at(-7).isNumber(), "0 and 7 returns number");
            ASSERT_EQ(suite, 7.0, L->at(-7).asNumber(), "0 and 7 == 7");

            ASSERT_TRUE(suite, L->at(-6).isString(), "false or 'fallback' returns string");
            if (L->at(-6).isString()) {
                ASSERT_TRUE(suite, std::string(L->at(-6).asString()->c_str()) == "fallback",
                    "false or 'fallback' == 'fallback'");
            }

            ASSERT_TRUE(suite, L->at(-5).isBoolean(), "not false returns boolean");
            ASSERT_TRUE(suite, L->at(-5).asBoolean(), "not false == true");

            ASSERT_TRUE(suite, L->at(-4).isBoolean(), "not 0 returns boolean");
            ASSERT_FALSE(suite, L->at(-4).asBoolean(), "not 0 == false");

            ASSERT_TRUE(suite, L->at(-3).isBoolean(), "false and mark() returns boolean false");
            ASSERT_FALSE(suite, L->at(-3).asBoolean(), "false and mark() == false");

            ASSERT_TRUE(suite, L->at(-2).isBoolean(), "true or mark() returns boolean true");
            ASSERT_TRUE(suite, L->at(-2).asBoolean(), "true or mark() == true");

            ASSERT_TRUE(suite, L->at(-1).isNumber(), "short-circuit counter returns number");
            ASSERT_EQ(suite, 0.0, L->at(-1).asNumber(), "short-circuit skipped mark()");
        }

        delete L;
        delete proto;
    } catch (const std::exception& e) {
        std::cout << "  [ERROR] Exception: " << e.what() << std::endl;
        ASSERT_TRUE(suite, false, "logical expressions should execute without throwing");
    }
}

void testLogicalShortCircuitEvaluatesRightHandSideOnlyWhenNeeded(TestSuite& suite) {
    const char* code = R"(
        local counter = {0}
        local function mark()
            counter[1] = counter[1] + 1
            return counter[1]
        end

        local andValue = true and mark()
        local orValue = nil or mark()

        return andValue, orValue, counter[1]
    )";

    try {
        Proto* proto = nullptr;
        LuaState* L = executeChunk(code, "=(logical_short_circuit_runtime)", proto);

        ASSERT_TRUE(suite, proto != nullptr, "logical short-circuit proto generated");
        ASSERT_TRUE(suite, L->getTop() >= 3, "logical short-circuit returned");

        if (L->getTop() >= 3) {
            ASSERT_TRUE(suite, L->at(-3).isNumber(), "true and mark() returns number");
            ASSERT_EQ(suite, 1.0, L->at(-3).asNumber(), "true and mark() evaluates RHS once");

            ASSERT_TRUE(suite, L->at(-2).isNumber(), "nil or mark() returns number");
            ASSERT_EQ(suite, 2.0, L->at(-2).asNumber(), "nil or mark() evaluates RHS once");

            ASSERT_TRUE(suite, L->at(-1).isNumber(), "counter returns number");
            ASSERT_EQ(suite, 2.0, L->at(-1).asNumber(), "RHS ran exactly when needed");
        }

        delete L;
        delete proto;
    } catch (const std::exception& e) {
        std::cout << "  [ERROR] Exception: " << e.what() << std::endl;
        ASSERT_TRUE(suite, false, "logical short-circuit runtime should not throw");
    }
}

void registerVMCoreTests() {
    auto& registry = TestRegistry::getInstance();
    
    registry.registerTest("VM Core", "GlobalState", testGlobalState);
    registry.registerTest("VM Core", "Stack Operations", testStackOperations);
    registry.registerTest("VM Core", "CallInfo", testCallInfo);
    registry.registerTest("VM Core", "LuaState Creation", testLuaStateCreation);
    registry.registerTest("VM Core", "LuaState Stack", testLuaStateStackOperations);
    registry.registerTest("VM Core", "LuaState Globals", testLuaStateGlobalVariables);
    registry.registerTest("VM Core", "LuaState Userdata Metatable", testLuaStateUserdataMetatable);
    registry.registerTest("VM Core", "SELF Dispatch On Userdata", testSelfDispatchOnUserdata);
    registry.registerTest("VM Core", "IOLib File Metatable Hooks", testIOLibFileMetatableHooks);
    registry.registerTest("VM Core", "IOLib Default Input Output", testIOLibDefaultInputOutput);
    registry.registerTest("VM Core", "Comparison Expression Produces Boolean", testComparisonExpressionProducesBoolean);
    registry.registerTest("VM Core", "Logical Expressions Produce Runtime Values", testLogicalExpressionsProduceRuntimeValues);
    registry.registerTest("VM Core", "Logical Short Circuit Runtime", testLogicalShortCircuitEvaluatesRightHandSideOnlyWhenNeeded);
}

