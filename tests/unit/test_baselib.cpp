/**
 * @file test_baselib.cpp
 * @brief 测试基础库函数
 */

#include "test_framework.hpp"
#include "lib/baselib.hpp"
#include "vm/lua_state.hpp"
#include "core/string_pool.hpp"
#include "core/function.hpp"
#include <iostream>
#include <cassert>

using namespace Lua;
using namespace LuaTest;

void testPrint() {
    std::cout << "[TEST 1] Testing print function..." << std::endl;
    
    LuaState* L = LuaState::newState();
    openBaseLib(L);
    
    // 测试print函数
    Value printFunc = L->getGlobal("print");
    assert(printFunc.isFunction());
    
    // 调用print("Hello, Lua!")
    L->pushFunction(printFunc.asFunction());
    L->pushString(L->getGlobalState().getStringPool().intern("Hello, Lua!"));
    
    std::cout << "  Output: ";
    printFunc.asFunction()->getCFunction()(L);
    
    delete L;
    std::cout << "  PASS" << std::endl;
}

void testType() {
    std::cout << "[TEST 2] Testing type function..." << std::endl;
    
    LuaState* L = LuaState::newState();
    openBaseLib(L);
    
    Value typeFunc = L->getGlobal("type");
    assert(typeFunc.isFunction());
    
    // 测试type(42)
    L->pushFunction(typeFunc.asFunction());
    L->pushNumber(42.0);
    typeFunc.asFunction()->getCFunction()(L);
    
    Value result = L->top();
    assert(result.isString());
    std::cout << "  type(42) = " << result.asString()->c_str() << std::endl;
    assert(std::string(result.asString()->c_str()) == "number");
    
    delete L;
    std::cout << "  PASS" << std::endl;
}

void testTostring() {
    std::cout << "[TEST 3] Testing tostring function..." << std::endl;
    
    LuaState* L = LuaState::newState();
    openBaseLib(L);
    
    Value tostringFunc = L->getGlobal("tostring");
    assert(tostringFunc.isFunction());
    
    // 测试tostring(123)
    L->pushFunction(tostringFunc.asFunction());
    L->pushNumber(123.0);
    tostringFunc.asFunction()->getCFunction()(L);
    
    Value result = L->top();
    assert(result.isString());
    std::cout << "  tostring(123) = " << result.asString()->c_str() << std::endl;
    
    delete L;
    std::cout << "  PASS" << std::endl;
}

void testTonumber() {
    std::cout << "[TEST 4] Testing tonumber function..." << std::endl;
    
    LuaState* L = LuaState::newState();
    openBaseLib(L);
    
    Value tonumberFunc = L->getGlobal("tonumber");
    assert(tonumberFunc.isFunction());
    
    // 测试tonumber(456)
    L->pushFunction(tonumberFunc.asFunction());
    L->pushNumber(456.0);
    tonumberFunc.asFunction()->getCFunction()(L);
    
    Value result = L->top();
    assert(result.isNumber());
    std::cout << "  tonumber(456) = " << result.asNumber() << std::endl;
    assert(result.asNumber() == 456.0);
    
    delete L;
    std::cout << "  PASS" << std::endl;
}

void testAssert() {
    std::cout << "[TEST 5] Testing assert function..." << std::endl;
    
    LuaState* L = LuaState::newState();
    openBaseLib(L);
    
    Value assertFunc = L->getGlobal("assert");
    assert(assertFunc.isFunction());
    
    // 测试assert(true)
    L->pushFunction(assertFunc.asFunction());
    L->pushBoolean(true);
    i32 nresults = assertFunc.asFunction()->getCFunction()(L);
    
    std::cout << "  assert(true) returned " << nresults << " values" << std::endl;
    assert(nresults == 1);
    
    delete L;
    std::cout << "  PASS" << std::endl;
}

void testMetatable() {
    std::cout << "[TEST 6] Testing setmetatable/getmetatable..." << std::endl;
    
    LuaState* L = LuaState::newState();
    openBaseLib(L);
    
    // 创建表和元表
    Table* t = new Table();
    Table* mt = new Table();
    L->getGlobalState().getGC().registerObject(t);
    L->getGlobalState().getGC().registerObject(mt);
    
    // 测试setmetatable
    Value setmetatableFunc = L->getGlobal("setmetatable");
    assert(setmetatableFunc.isFunction());
    
    L->pushFunction(setmetatableFunc.asFunction());
    L->pushTable(t);
    L->pushTable(mt);
    setmetatableFunc.asFunction()->getCFunction()(L);
    
    // 测试getmetatable
    Value getmetatableFunc = L->getGlobal("getmetatable");
    assert(getmetatableFunc.isFunction());
    
    L->pushFunction(getmetatableFunc.asFunction());
    L->pushTable(t);
    getmetatableFunc.asFunction()->getCFunction()(L);
    
    Value result = L->top();
    assert(result.isTable());
    assert(result.asTable() == mt);
    
    std::cout << "  Metatable set and retrieved successfully" << std::endl;
    
    delete L;
    std::cout << "  PASS" << std::endl;
}

// 包装函数以适配测试框架
void testPrintWrapper(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    openBaseLib(L);
    
    // 测试print函数存在
    Value printFunc = L->getGlobal("print");
    ASSERT_TRUE(suite, printFunc.isFunction(), "print function exists");
    
    // 测试print函数能够正常调用
    L->pushFunction(printFunc.asFunction());
    L->pushString(L->getGlobalState().getStringPool().intern("Test output"));
    i32 ret = printFunc.asFunction()->getCFunction()(L);
    ASSERT_EQ(suite, ret, 0, "print returns 0");
    
    delete L;
}

void testTypeWrapper(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    openBaseLib(L);
    
    Value typeFunc = L->getGlobal("type");
    ASSERT_TRUE(suite, typeFunc.isFunction(), "type function exists");
    
    // 测试type(42)
    L->getStack().clear();
    L->pushNumber(42.0);
    i32 ret = typeFunc.asFunction()->getCFunction()(L);
    ASSERT_EQ(suite, ret, 1, "type returns 1 value");
    Value val = L->top();
    bool isStr = val.isString();
    ASSERT_TRUE(suite, isStr, "type returns string");
    if (isStr) {
        std::string s = val.asString()->c_str();
        bool match = (s == "number");
        ASSERT_TRUE(suite, match, "type(42) == 'number'");
    }
    
    // 测试type("hello")
    L->getStack().clear();
    L->pushString(L->getGlobalState().getStringPool().intern("hello"));
    ret = typeFunc.asFunction()->getCFunction()(L);
    val = L->top();
    if (val.isString()) {
        std::string s = val.asString()->c_str();
        bool match = (s == "string");
        ASSERT_TRUE(suite, match, "type('hello') == 'string'");
    }
    
    // 测试type(nil)
    L->getStack().clear();
    L->pushNil();
    ret = typeFunc.asFunction()->getCFunction()(L);
    val = L->top();
    if (val.isString()) {
        std::string s = val.asString()->c_str();
        bool match = (s == "nil");
        ASSERT_TRUE(suite, match, "type(nil) == 'nil'");
    }
    
    delete L;
}

void testTostringWrapper(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    openBaseLib(L);
    
    Value tostringFunc = L->getGlobal("tostring");
    ASSERT_TRUE(suite, tostringFunc.isFunction(), "tostring function exists");
    
    // 测试tostring(123)
    L->getStack().clear();
    L->pushNumber(123.0);
    i32 ret = tostringFunc.asFunction()->getCFunction()(L);
    ASSERT_EQ(suite, ret, 1, "tostring returns 1 value");
    Value val = L->top();
    bool isStr = val.isString();
    ASSERT_TRUE(suite, isStr, "tostring returns string");
    
    // 测试tostring(true)
    L->getStack().clear();
    L->pushBoolean(true);
    ret = tostringFunc.asFunction()->getCFunction()(L);
    val = L->top();
    if (val.isString()) {
        std::string s = val.asString()->c_str();
        bool match = (s == "true");
        ASSERT_TRUE(suite, match, "tostring(true) == 'true'");
    }
    
    // 测试tostring(false)
    L->getStack().clear();
    L->pushBoolean(false);
    ret = tostringFunc.asFunction()->getCFunction()(L);
    val = L->top();
    if (val.isString()) {
        std::string s = val.asString()->c_str();
        bool match = (s == "false");
        ASSERT_TRUE(suite, match, "tostring(false) == 'false'");
    }
    
    delete L;
}

void testTonumberWrapper(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    openBaseLib(L);
    
    Value tonumberFunc = L->getGlobal("tonumber");
    ASSERT_TRUE(suite, tonumberFunc.isFunction(), "tonumber function exists");
    
    // 测试tonumber(456)
    L->getStack().clear();
    L->pushNumber(456.0);
    i32 ret = tonumberFunc.asFunction()->getCFunction()(L);
    ASSERT_EQ(suite, ret, 1, "tonumber returns 1 value");
    Value val = L->top();
    bool isNum = val.isNumber();
    ASSERT_TRUE(suite, isNum, "tonumber returns number");
    bool match1 = (val.asNumber() == 456.0);
    ASSERT_TRUE(suite, match1, "tonumber(456) == 456.0");
    
    // 测试tonumber("123")
    L->getStack().clear();
    L->pushString(L->getGlobalState().getStringPool().intern("123"));
    ret = tonumberFunc.asFunction()->getCFunction()(L);
    val = L->top();
    isNum = val.isNumber();
    ASSERT_TRUE(suite, isNum, "tonumber('123') returns number");
    bool match2 = (val.asNumber() == 123.0);
    ASSERT_TRUE(suite, match2, "tonumber('123') == 123.0");
    
    // 测试tonumber("1A", 16)
    L->getStack().clear();
    L->pushString(L->getGlobalState().getStringPool().intern("1A"));
    L->pushNumber(16.0);
    ret = tonumberFunc.asFunction()->getCFunction()(L);
    val = L->top();
    isNum = val.isNumber();
    ASSERT_TRUE(suite, isNum, "tonumber('1A', 16) returns number");
    bool match3 = (val.asNumber() == 26.0);
    ASSERT_TRUE(suite, match3, "tonumber('1A', 16) == 26.0");
    
    // 测试tonumber("xyz") -> nil
    L->getStack().clear();
    L->pushString(L->getGlobalState().getStringPool().intern("xyz"));
    ret = tonumberFunc.asFunction()->getCFunction()(L);
    val = L->top();
    bool isNil = val.isNil();
    ASSERT_TRUE(suite, isNil, "tonumber('xyz') returns nil");
    
    delete L;
}

void testAssertWrapper(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    openBaseLib(L);
    
    Value assertFunc = L->getGlobal("assert");
    ASSERT_TRUE(suite, assertFunc.isFunction(), "assert function exists");
    
    // 测试assert(true)
    L->getStack().clear();
    L->pushBoolean(true);
    i32 ret = assertFunc.asFunction()->getCFunction()(L);
    ASSERT_EQ(suite, ret, 1, "assert(true) returns 1 value");
    
    // 测试assert(1)
    L->getStack().clear();
    L->pushNumber(1.0);
    ret = assertFunc.asFunction()->getCFunction()(L);
    ASSERT_EQ(suite, ret, 1, "assert(1) returns 1 value");
    
    // 测试assert(true, "message")
    L->getStack().clear();
    L->pushBoolean(true);
    L->pushString(L->getGlobalState().getStringPool().intern("test message"));
    ret = assertFunc.asFunction()->getCFunction()(L);
    ASSERT_EQ(suite, ret, 2, "assert(true, msg) returns all arguments");
    
    delete L;
}

void testMetatableWrapper(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    openBaseLib(L);
    
    // 创建表和元表
    Table* t = new Table();
    Table* mt = new Table();
    L->getGlobalState().getGC().registerObject(t);
    L->getGlobalState().getGC().registerObject(mt);
    
    Value setmetatableFunc = L->getGlobal("setmetatable");
    Value getmetatableFunc = L->getGlobal("getmetatable");
    ASSERT_TRUE(suite, setmetatableFunc.isFunction(), "setmetatable exists");
    ASSERT_TRUE(suite, getmetatableFunc.isFunction(), "getmetatable exists");
    
    // 测试setmetatable
    L->getStack().clear();
    L->pushTable(t);
    L->pushTable(mt);
    i32 ret = setmetatableFunc.asFunction()->getCFunction()(L);
    ASSERT_EQ(suite, ret, 1, "setmetatable returns 1 value");
    
    // 测试getmetatable
    L->getStack().clear();
    L->pushTable(t);
    ret = getmetatableFunc.asFunction()->getCFunction()(L);
    ASSERT_EQ(suite, ret, 1, "getmetatable returns 1 value");
    Value val = L->top();
    bool isTable = val.isTable();
    ASSERT_TRUE(suite, isTable, "getmetatable returns table");
    bool matchesMt = (val.asTable() == mt);
    ASSERT_TRUE(suite, matchesMt, "metatable matches");
    
    delete L;
}

void registerBaselibTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest("Base Library", "print", testPrintWrapper);
    registry.registerTest("Base Library", "type", testTypeWrapper);
    registry.registerTest("Base Library", "tostring", testTostringWrapper);
    registry.registerTest("Base Library", "tonumber", testTonumberWrapper);
    registry.registerTest("Base Library", "assert", testAssertWrapper);
    registry.registerTest("Base Library", "metatable", testMetatableWrapper);
}

