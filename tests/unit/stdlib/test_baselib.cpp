/**
 * @file test_baselib.cpp
 * @brief 基础库函数测试 - 依赖统一测试框架
 */

#include "../framework/test_framework.hpp"
#include "lib/baselib.hpp"
#include "vm/lua_state.hpp"
#include "core/string_pool.hpp"
#include "core/function.hpp"
#include "core/table.hpp"

#include <string>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Base Library";

} // namespace

void testPrintWrapper(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseLib);
    if (!ctx.ensureGlobalFunction("print", suite, "print function exists")) {
        return;
    }

    i32 ret = ctx.invoke("print", [](LuaState* L) {
        L->pushString(L->getGlobalState().getStringPool().intern("Test output"));
    });
    ASSERT_EQ(suite, ret, 0, "print returns 0");
}

void testTypeWrapper(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseLib);
    if (!ctx.ensureGlobalFunction("type", suite, "type function exists")) {
        return;
    }

    LuaState* L = ctx.getState();

    auto checkType = [&](auto pushValue, const std::string& expected, const std::string& msg) {
        i32 ret = ctx.invoke("type", [&](LuaState* s) { pushValue(s); });
        ASSERT_EQ(suite, ret, 1, "type returns 1 value");
        Value val = L->top();
        bool isStr = val.isString();
        ASSERT_TRUE(suite, isStr, "type returns string");
        if (isStr) {
            std::string s = val.asString()->c_str();
            ASSERT_TRUE(suite, s == expected, msg);
        }
    };

    checkType([](LuaState* s) { s->pushNumber(42.0); }, "number", "type(42) == 'number'");
    checkType([](LuaState* s) { s->pushString(s->getGlobalState().getStringPool().intern("hello")); }, "string", "type('hello') == 'string'");
    checkType([](LuaState* s) { s->pushNil(); }, "nil", "type(nil) == 'nil'");
}

void testTostringWrapper(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseLib);
    if (!ctx.ensureGlobalFunction("tostring", suite, "tostring function exists")) {
        return;
    }

    LuaState* L = ctx.getState();

    auto checkTostring = [&](auto pushValue, const std::string& expected, const std::string& msg) {
        i32 ret = ctx.invoke("tostring", [&](LuaState* s) { pushValue(s); });
        ASSERT_EQ(suite, ret, 1, "tostring returns 1 value");
        Value val = L->top();
        bool isStr = val.isString();
        ASSERT_TRUE(suite, isStr, "tostring returns string");
        if (isStr) {
            std::string s = val.asString()->c_str();
            ASSERT_TRUE(suite, s == expected, msg);
        }
    };

    checkTostring([](LuaState* s) { s->pushNumber(123.0); }, "123", "tostring(123) == '123'");
    checkTostring([](LuaState* s) { s->pushBoolean(true); }, "true", "tostring(true) == 'true'");
    checkTostring([](LuaState* s) { s->pushBoolean(false); }, "false", "tostring(false) == 'false'");
}

void testTonumberWrapper(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseLib);
    if (!ctx.ensureGlobalFunction("tonumber", suite, "tonumber function exists")) {
        return;
    }

    LuaState* L = ctx.getState();
    StringPool& pool = L->getGlobalState().getStringPool();

    auto checkTonumber = [&](auto pushArgs, std::function<bool(const Value&)> validator, const std::string& msg) {
        i32 ret = ctx.invoke("tonumber", [&](LuaState* s) { pushArgs(s); });
        ASSERT_EQ(suite, ret, 1, "tonumber returns 1 value");
        Value val = L->top();
        ASSERT_TRUE(suite, validator(val), msg);
    };

    checkTonumber([](LuaState* s) { s->pushNumber(456.0); }, [](const Value& v) { return v.isNumber() && v.asNumber() == 456.0; }, "tonumber(456) == 456.0");
    checkTonumber([&](LuaState* s) { s->pushString(pool.intern("123")); }, [](const Value& v) { return v.isNumber() && v.asNumber() == 123.0; }, "tonumber('123') == 123.0");
    checkTonumber([&](LuaState* s) {
        s->pushString(pool.intern("1A"));
        s->pushNumber(16.0);
    }, [](const Value& v) { return v.isNumber() && v.asNumber() == 26.0; }, "tonumber('1A', 16) == 26.0");

    i32 ret = ctx.invoke("tonumber", [&](LuaState* s) {
        s->pushString(pool.intern("xyz"));
    });
    ASSERT_EQ(suite, ret, 1, "tonumber invalid returns 1 value");
    Value invalidResult = L->top();
    ASSERT_TRUE(suite, invalidResult.isNil(), "tonumber('xyz') returns nil");
}

void testAssertWrapper(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseLib);
    if (!ctx.ensureGlobalFunction("assert", suite, "assert function exists")) {
        return;
    }

    StringPool& pool = ctx.getState()->getGlobalState().getStringPool();

    auto expectReturn = [&](auto pushArgs, i32 expected, const std::string& msg) {
        i32 ret = ctx.invoke("assert", [&](LuaState* s) { pushArgs(s); });
        ASSERT_EQ(suite, ret, expected, msg);
    };

    expectReturn([](LuaState* s) { s->pushBoolean(true); }, 1, "assert(true) returns 1 value");
    expectReturn([](LuaState* s) { s->pushNumber(1.0); }, 1, "assert(1) returns 1 value");
    expectReturn([&](LuaState* s) {
        s->pushBoolean(true);
        s->pushString(pool.intern("test message"));
    }, 2, "assert(true, msg) returns all arguments");
}

void testMetatableWrapper(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseLib);
    bool setExists = ctx.ensureGlobalFunction("setmetatable", suite, "setmetatable exists");
    bool getExists = ctx.ensureGlobalFunction("getmetatable", suite, "getmetatable exists");
    if (!setExists || !getExists) {
        return;
    }

    LuaState* L = ctx.getState();
    Table* t = new Table();
    Table* mt = new Table();
    L->getGlobalState().getGC().registerObject(t);
    L->getGlobalState().getGC().registerObject(mt);

    i32 ret = ctx.invoke("setmetatable", [t, mt](LuaState* s) {
        s->pushTable(t);
        s->pushTable(mt);
    });
    ASSERT_EQ(suite, ret, 1, "setmetatable returns 1 value");

    ret = ctx.invoke("getmetatable", [t](LuaState* s) {
        s->pushTable(t);
    });
    ASSERT_EQ(suite, ret, 1, "getmetatable returns 1 value");
    Value mtResult = L->top();
    ASSERT_TRUE(suite, mtResult.isTable(), "getmetatable returns table");
    if (mtResult.isTable()) {
        ASSERT_TRUE(suite, mtResult.asTable() == mt, "metatable matches");
    }
}

void testRawgetWrapper(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseLib);
    if (!ctx.ensureGlobalFunction("rawget", suite, "rawget exists")) {
        return;
    }

    LuaState* L = ctx.getState();
    auto& pool = L->getGlobalState().getStringPool();

    // 创建表并设置值
    Table* t = new Table();
    L->getGlobalState().getGC().registerObject(t);
    t->set(Value(pool.intern("key")), Value(42.0));
    t->set(Value(1.0), Value(100.0));

    // 测试字符串键
    i32 ret = ctx.invoke("rawget", [t, &pool](LuaState* s) {
        s->pushTable(t);
        s->pushString(pool.intern("key"));
    });
    ASSERT_EQ(suite, ret, 1, "rawget returns 1 value");
    ASSERT_EQ(suite, 42.0, L->top().asNumber(), "rawget(t, 'key') == 42");

    // 测试数字键
    ret = ctx.invoke("rawget", [t](LuaState* s) {
        s->pushTable(t);
        s->pushNumber(1.0);
    });
    ASSERT_EQ(suite, ret, 1, "rawget returns 1 value");
    ASSERT_EQ(suite, 100.0, L->top().asNumber(), "rawget(t, 1) == 100");

    // 测试不存在的键
    ret = ctx.invoke("rawget", [t, &pool](LuaState* s) {
        s->pushTable(t);
        s->pushString(pool.intern("nonexistent"));
    });
    ASSERT_EQ(suite, ret, 1, "rawget returns 1 value");
    ASSERT_TRUE(suite, L->top().isNil(), "rawget(t, 'nonexistent') == nil");
}

void testRawsetWrapper(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseLib);
    if (!ctx.ensureGlobalFunction("rawset", suite, "rawset exists")) {
        return;
    }

    LuaState* L = ctx.getState();
    auto& pool = L->getGlobalState().getStringPool();

    // 创建空表
    Table* t = new Table();
    L->getGlobalState().getGC().registerObject(t);

    // 测试设置字符串键
    i32 ret = ctx.invoke("rawset", [t, &pool](LuaState* s) {
        s->pushTable(t);
        s->pushString(pool.intern("name"));
        s->pushString(pool.intern("Lua"));
    });
    ASSERT_EQ(suite, ret, 1, "rawset returns 1 value");
    ASSERT_TRUE(suite, L->top().isTable(), "rawset returns table");
    Value v1 = t->get(Value(pool.intern("name")));
    ASSERT_TRUE(suite, v1.isString(), "value is string");
    ASSERT_TRUE(suite, std::string(v1.asString()->c_str()) == "Lua", "t['name'] == 'Lua'");

    // 测试设置数字键
    ret = ctx.invoke("rawset", [t](LuaState* s) {
        s->pushTable(t);
        s->pushNumber(1.0);
        s->pushNumber(999.0);
    });
    Value v2 = t->get(Value(1.0));
    ASSERT_EQ(suite, 999.0, v2.asNumber(), "t[1] == 999");
}

void testRawequalWrapper(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseLib);
    if (!ctx.ensureGlobalFunction("rawequal", suite, "rawequal exists")) {
        return;
    }

    LuaState* L = ctx.getState();
    auto& pool = L->getGlobalState().getStringPool();

    // 测试相同的数字
    i32 ret = ctx.invoke("rawequal", [](LuaState* s) {
        s->pushNumber(42.0);
        s->pushNumber(42.0);
    });
    ASSERT_EQ(suite, ret, 1, "rawequal returns 1 value");
    ASSERT_TRUE(suite, L->top().asBoolean(), "rawequal(42, 42) == true");

    // 测试不同的数字
    ret = ctx.invoke("rawequal", [](LuaState* s) {
        s->pushNumber(42.0);
        s->pushNumber(43.0);
    });
    ASSERT_FALSE(suite, L->top().asBoolean(), "rawequal(42, 43) == false");

    // 测试相同的字符串
    GCString* str = pool.intern("test");
    ret = ctx.invoke("rawequal", [str](LuaState* s) {
        s->pushString(str);
        s->pushString(str);
    });
    ASSERT_TRUE(suite, L->top().asBoolean(), "rawequal('test', 'test') == true");

    // 测试不同类型
    ret = ctx.invoke("rawequal", [&pool](LuaState* s) {
        s->pushNumber(42.0);
        s->pushString(pool.intern("42"));
    });
    ASSERT_FALSE(suite, L->top().asBoolean(), "rawequal(42, '42') == false");
}

void testSelectWrapper(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseLib);
    if (!ctx.ensureGlobalFunction("select", suite, "select exists")) {
        return;
    }

    LuaState* L = ctx.getState();
    auto& pool = L->getGlobalState().getStringPool();

    // 测试 select("#", ...)
    i32 ret = ctx.invoke("select", [&pool](LuaState* s) {
        s->pushString(pool.intern("#"));
        s->pushNumber(1.0);
        s->pushNumber(2.0);
        s->pushNumber(3.0);
    });
    ASSERT_EQ(suite, ret, 1, "select('#', ...) returns 1 value");
    ASSERT_EQ(suite, 3.0, L->top().asNumber(), "select('#', 1, 2, 3) == 3");

    // 测试 select(2, ...)
    ret = ctx.invoke("select", [](LuaState* s) {
        s->pushNumber(2.0);
        s->pushNumber(10.0);
        s->pushNumber(20.0);
        s->pushNumber(30.0);
        s->pushNumber(40.0);
    });
    ASSERT_EQ(suite, ret, 3, "select(2, ...) returns 3 values");
    ASSERT_EQ(suite, 40.0, L->at(-1).asNumber(), "last value is 40");
    ASSERT_EQ(suite, 30.0, L->at(-2).asNumber(), "second value is 30");
    ASSERT_EQ(suite, 20.0, L->at(-3).asNumber(), "first value is 20");

    // 测试 select(-1, ...)
    ret = ctx.invoke("select", [](LuaState* s) {
        s->pushNumber(-1.0);
        s->pushNumber(100.0);
        s->pushNumber(200.0);
        s->pushNumber(300.0);
    });
    ASSERT_EQ(suite, ret, 1, "select(-1, ...) returns 1 value");
    ASSERT_EQ(suite, 300.0, L->top().asNumber(), "select(-1, 100, 200, 300) == 300");
}

void registerBaselibTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "print", testPrintWrapper);
    registry.registerTest(kSuiteName, "type", testTypeWrapper);
    registry.registerTest(kSuiteName, "tostring", testTostringWrapper);
    registry.registerTest(kSuiteName, "tonumber", testTonumberWrapper);
    registry.registerTest(kSuiteName, "assert", testAssertWrapper);
    registry.registerTest(kSuiteName, "metatable", testMetatableWrapper);
    registry.registerTest(kSuiteName, "rawget", testRawgetWrapper);
    registry.registerTest(kSuiteName, "rawset", testRawsetWrapper);
    registry.registerTest(kSuiteName, "rawequal", testRawequalWrapper);
    registry.registerTest(kSuiteName, "select", testSelectWrapper);
}

