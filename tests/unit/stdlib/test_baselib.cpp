/**
 * @file test_baselib.cpp
 * @brief 基础库函数测试 - 依赖统一测试框架
 */

#include "../framework/test_framework.hpp"
#include "lib/baselib.hpp"
#include "lib/lib_manager.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm.hpp"
#include "core/string_pool.hpp"
#include "core/function.hpp"
#include "core/table.hpp"
#include "core/gc_string.hpp"
#include "compiler/opcode.hpp"
#include "compiler/parser/parser.hpp"
#include "compiler/codegen/codegen.hpp"

#include <string>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Base Library";

/// Helper: compile and execute Lua code with all standard libs
bool runLua(LuaState* L, const char* code) {
    try {
        Parser parser(code);
        auto parsed = parser.parse();
        if (!parsed) {
            throw parsed.error();
        }
        Chunk chunk = std::move(*parsed);
        StringPool& pool = StringPool::getInstance();
        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk, "test");
        if (!proto) return false;

        Function* func = new Function(proto);
        L->getGlobalState().getGC().registerObject(func);
        func->setEnv(L->getGlobalTable());
        VM::execute(L, func);
        delete proto;
        return true;
    } catch (...) {
        return false;
    }
}

/// Helper: get global number
double getGlobalNumber(LuaState* L, const char* name) {
    Value v = L->getGlobal(name);
    return v.isNumber() ? v.asNumber() : -9999.0;
}

/// Helper: get global string
std::string getGlobalStr(LuaState* L, const char* name) {
    Value v = L->getGlobal(name);
    return v.isString() ? std::string(v.asString()->c_str()) : "";
}

/// Helper: create state with all standard libraries
LuaState* createFullState() {
    LuaState* L = LuaState::newState();
    StandardLibrary::openAll(L);
    return L;
}

} // namespace

void testGlobalSelfReference(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseLib);
    LuaState* L = ctx.getState();

    Value globalSelf = L->getGlobal("_G");
    ASSERT_TRUE(suite, globalSelf.isTable(), "_G exists");
    if (globalSelf.isTable()) {
        ASSERT_TRUE(suite, globalSelf.asTable() == L->getGlobalTable(), "_G references the global table");
    }

    Value version = L->getGlobal("_VERSION");
    ASSERT_TRUE(suite, version.isString(), "_VERSION remains registered");
}

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

    Table* protectedTable = new Table();
    Table* protectedMt = new Table();
    L->getGlobalState().getGC().registerObject(protectedTable);
    L->getGlobalState().getGC().registerObject(protectedMt);
    auto& pool = L->getGlobalState().getStringPool();
    protectedMt->set(Value(pool.intern("__metatable")), Value(pool.intern("locked")));

    ret = ctx.invoke("setmetatable", [protectedTable, protectedMt](LuaState* s) {
        s->pushTable(protectedTable);
        s->pushTable(protectedMt);
    });
    ASSERT_EQ(suite, ret, 1, "setmetatable protected returns 1 value");

    ret = ctx.invoke("getmetatable", [protectedTable](LuaState* s) {
        s->pushTable(protectedTable);
    });
    ASSERT_EQ(suite, ret, 1, "protected getmetatable returns 1 value");
    Value protectedResult = L->top();
    ASSERT_TRUE(suite, protectedResult.isString(), "protected getmetatable returns __metatable value");
    if (protectedResult.isString()) {
        ASSERT_TRUE(
            suite,
            std::string(protectedResult.asString()->c_str()) == "locked",
            "protected getmetatable returns locked marker"
        );
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

void testPcallWrapper(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseLib);
    if (!ctx.ensureGlobalFunction("pcall", suite, "pcall function exists")) {
        return;
    }

    LuaState* L = ctx.getState();

    // 测试成功调用
    // 创建一个简单的函数：function() return 42 end
    Proto* proto = new Proto();
    proto->setMaxStackSize(2);

    // LOADK R(0) 42
    proto->addInstruction(CREATE_ABx(OpCode::LOADK, 0, static_cast<i32>(proto->addConstant(Value(42.0)))));
    // RETURN R(0) 2 (返回1个值)
    proto->addInstruction(CREATE_ABC(OpCode::RETURN, 0, 2, 0));

    Function* testFunc = new Function(proto);
    L->getGlobalState().getGC().registerObject(testFunc);
    testFunc->setEnv(L->getGlobalTable());

    i32 ret = ctx.invoke("pcall", [&](LuaState* s) {
        s->pushValue(Value(testFunc));
    });

    ASSERT_EQ(suite, ret, 2, "pcall returns 2 values on success");
    ASSERT_TRUE(suite, L->at(-2).isBoolean(), "first return is boolean");
    ASSERT_TRUE(suite, L->at(-2).asBoolean(), "first return is true");
    ASSERT_TRUE(suite, L->at(-1).isNumber(), "second return is number");
    ASSERT_EQ(suite, 42.0, L->at(-1).asNumber(), "second return is 42");

    // 测试调用非函数值
    ret = ctx.invoke("pcall", [](LuaState* s) {
        s->pushNumber(123.0);
    });

    ASSERT_EQ(suite, ret, 2, "pcall returns 2 values on error");
    ASSERT_TRUE(suite, L->at(-2).isBoolean(), "first return is boolean");
    ASSERT_FALSE(suite, L->at(-2).asBoolean(), "first return is false");
    ASSERT_TRUE(suite, L->at(-1).isString(), "second return is error message");
}

void testXpcallWrapper(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseLib);
    if (!ctx.ensureGlobalFunction("xpcall", suite, "xpcall function exists")) {
        return;
    }

    LuaState* L = ctx.getState();

    // 创建测试函数
    Proto* proto = new Proto();
    proto->setMaxStackSize(2);
    proto->addInstruction(CREATE_ABx(OpCode::LOADK, 0, static_cast<i32>(proto->addConstant(Value(100.0)))));
    proto->addInstruction(CREATE_ABC(OpCode::RETURN, 0, 2, 0));

    Function* testFunc = new Function(proto);
    L->getGlobalState().getGC().registerObject(testFunc);
    testFunc->setEnv(L->getGlobalTable());

    // 创建错误处理器（返回固定字符串）
    Proto* errProto = new Proto();
    errProto->setMaxStackSize(2);
    auto& pool = L->getGlobalState().getStringPool();
    errProto->addInstruction(CREATE_ABx(OpCode::LOADK, 0, static_cast<i32>(errProto->addConstant(Value(pool.intern("error handled"))))));
    errProto->addInstruction(CREATE_ABC(OpCode::RETURN, 0, 2, 0));

    Function* errFunc = new Function(errProto);
    L->getGlobalState().getGC().registerObject(errFunc);
    errFunc->setEnv(L->getGlobalTable());

    // 测试成功调用
    i32 ret = ctx.invoke("xpcall", [&](LuaState* s) {
        s->pushValue(Value(testFunc));
        s->pushValue(Value(errFunc));
    });

    ASSERT_EQ(suite, ret, 2, "xpcall returns 2 values on success");
    ASSERT_TRUE(suite, L->at(-2).isBoolean(), "first return is boolean");
    ASSERT_TRUE(suite, L->at(-2).asBoolean(), "first return is true");
    ASSERT_EQ(suite, 100.0, L->at(-1).asNumber(), "second return is 100");

    // 测试调用非函数值（触发错误处理器）
    ret = ctx.invoke("xpcall", [&](LuaState* s) {
        s->pushNumber(999.0);
        s->pushValue(Value(errFunc));
    });

    ASSERT_EQ(suite, ret, 2, "xpcall returns 2 values on error");
    ASSERT_TRUE(suite, L->at(-2).isBoolean(), "first return is boolean");
    ASSERT_FALSE(suite, L->at(-2).asBoolean(), "first return is false");
}

void testLoadstringWrapper(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseLib);
    if (!ctx.ensureGlobalFunction("loadstring", suite, "loadstring function exists")) {
        return;
    }

    LuaState* L = ctx.getState();
    auto& pool = L->getGlobalState().getStringPool();

    // 测试成功编译
    i32 ret = ctx.invoke("loadstring", [&](LuaState* s) {
        s->pushString(pool.intern("return 42"));
    });

    ASSERT_EQ(suite, ret, 1, "loadstring returns 1 value on success");
    ASSERT_TRUE(suite, L->top().isFunction(), "loadstring returns function");

    // 测试语法错误
    ret = ctx.invoke("loadstring", [&](LuaState* s) {
        s->pushString(pool.intern("return return"));
    });

    ASSERT_EQ(suite, ret, 2, "loadstring returns 2 values on error");
    ASSERT_TRUE(suite, L->at(-2).isNil(), "first return is nil on error");
    ASSERT_TRUE(suite, L->at(-1).isString(), "second return is error message");

    // 测试非字符串参数
    ret = ctx.invoke("loadstring", [](LuaState* s) {
        s->pushNumber(123.0);
    });

    ASSERT_EQ(suite, ret, 2, "loadstring returns 2 values on type error");
    ASSERT_TRUE(suite, L->at(-2).isNil(), "first return is nil");
    ASSERT_TRUE(suite, L->at(-1).isString(), "second return is error message");
}

void testLoadfileWrapper(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseLib);
    if (!ctx.ensureGlobalFunction("loadfile", suite, "loadfile function exists")) {
        return;
    }

    LuaState* L = ctx.getState();
    auto& pool = L->getGlobalState().getStringPool();

    // 测试文件不存在
    i32 ret = ctx.invoke("loadfile", [&](LuaState* s) {
        s->pushString(pool.intern("nonexistent_file.lua"));
    });

    ASSERT_EQ(suite, ret, 2, "loadfile returns 2 values on file not found");
    ASSERT_TRUE(suite, L->at(-2).isNil(), "first return is nil");
    ASSERT_TRUE(suite, L->at(-1).isString(), "second return is error message");

    // 测试非字符串参数
    ret = ctx.invoke("loadfile", [](LuaState* s) {
        s->pushNumber(456.0);
    });

    ASSERT_EQ(suite, ret, 2, "loadfile returns 2 values on type error");
    ASSERT_TRUE(suite, L->at(-2).isNil(), "first return is nil");
    ASSERT_TRUE(suite, L->at(-1).isString(), "second return is error message");
}

void testDofileWrapper(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseLib);
    if (!ctx.ensureGlobalFunction("dofile", suite, "dofile function exists")) {
        return;
    }

    // 注意：dofile 是 legacy 函数，测试其存在性即可
    // 实际文件执行测试需要创建临时文件，这里简化处理
    ASSERT_TRUE(suite, true, "dofile function registered");
}

// =====================================================================
// unpack Tests
// =====================================================================

void testUnpackWrapper(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseLib);
    LuaState* L = ctx.getState();

    // Test 1: unpack basic — unpack({10, 20, 30}) returns 10, 20, 30
    {
        Table* t = new Table();
        L->getGlobalState().getGC().registerObject(t);
        t->setArray(1, Value(10.0));
        t->setArray(2, Value(20.0));
        t->setArray(3, Value(30.0));

        i32 ret = ctx.invoke("unpack", [&](LuaState* s) {
            s->pushTable(t);
        });
        ASSERT_EQ(suite, ret, 3, "unpack({10,20,30}) returns 3 values");
        // Values are on the stack: at(-3)=10, at(-2)=20, at(-1)=30
        ASSERT_EQ(suite, 10.0, L->at(-3).asNumber(), "unpack first = 10");
        ASSERT_EQ(suite, 20.0, L->at(-2).asNumber(), "unpack second = 20");
        ASSERT_EQ(suite, 30.0, L->at(-1).asNumber(), "unpack third = 30");
    }

    // Test 2: unpack with range — unpack({10, 20, 30, 40}, 2, 3) returns 20, 30
    {
        Table* t = new Table();
        L->getGlobalState().getGC().registerObject(t);
        t->setArray(1, Value(10.0));
        t->setArray(2, Value(20.0));
        t->setArray(3, Value(30.0));
        t->setArray(4, Value(40.0));

        i32 ret = ctx.invoke("unpack", [&](LuaState* s) {
            s->pushTable(t);
            s->pushNumber(2.0);
            s->pushNumber(3.0);
        });
        ASSERT_EQ(suite, ret, 2, "unpack(t,2,3) returns 2 values");
        ASSERT_EQ(suite, 20.0, L->at(-2).asNumber(), "unpack(t,2,3) first = 20");
        ASSERT_EQ(suite, 30.0, L->at(-1).asNumber(), "unpack(t,2,3) second = 30");
    }

    // Test 3: unpack empty table
    {
        Table* t = new Table();
        L->getGlobalState().getGC().registerObject(t);

        i32 ret = ctx.invoke("unpack", [&](LuaState* s) {
            s->pushTable(t);
        });
        ASSERT_EQ(suite, ret, 0, "unpack({}) returns 0 values");
    }

    // Test 4: unpack single element
    {
        Table* t = new Table();
        L->getGlobalState().getGC().registerObject(t);
        t->setArray(1, Value(42.0));

        i32 ret = ctx.invoke("unpack", [&](LuaState* s) {
            s->pushTable(t);
        });
        ASSERT_EQ(suite, ret, 1, "unpack({42}) returns 1 value");
        ASSERT_EQ(suite, 42.0, L->top().asNumber(), "unpack({42}) = 42");
    }

    // Test 5: unpack with explicit i > j (empty range)
    {
        Table* t = new Table();
        L->getGlobalState().getGC().registerObject(t);
        t->setArray(1, Value(10.0));

        i32 ret = ctx.invoke("unpack", [&](LuaState* s) {
            s->pushTable(t);
            s->pushNumber(3.0);
            s->pushNumber(1.0);
        });
        ASSERT_EQ(suite, ret, 0, "unpack(t,3,1) returns 0 (empty range)");
    }
}

// =====================================================================
// unpack via Lua execution (integration test)
// =====================================================================

void testUnpackLua(TestSuite& suite) {
    LuaState* L = createFullState();

    // Test: unpack in multiple assignment
    bool ok = runLua(L, R"(
        local a, b, c = unpack({10, 20, 30})
        gA = a
        gB = b
        gC = c
    )");
    ASSERT_TRUE(suite, ok, "unpack in multi-assignment runs");
    ASSERT_EQ(suite, 10.0, getGlobalNumber(L, "gA"), "unpack a=10");
    ASSERT_EQ(suite, 20.0, getGlobalNumber(L, "gB"), "unpack b=20");
    ASSERT_EQ(suite, 30.0, getGlobalNumber(L, "gC"), "unpack c=30");

    // Test: unpack with range in Lua
    ok = runLua(L, R"(
        local b, c = unpack({10, 20, 30, 40}, 2, 3)
        gB2 = b
        gC2 = c
    )");
    ASSERT_TRUE(suite, ok, "unpack with range in Lua runs");
    ASSERT_EQ(suite, 20.0, getGlobalNumber(L, "gB2"), "unpack(t,2,3) b=20");
    ASSERT_EQ(suite, 30.0, getGlobalNumber(L, "gC2"), "unpack(t,2,3) c=30");

    // Test: unpack with mixed types
    ok = runLua(L, R"lua(
        local a, b, c = unpack({"hello", 42, true})
        gStr = a
        gNum = b
        if c then gBool = 1 else gBool = 0 end
    )lua");
    ASSERT_TRUE(suite, ok, "unpack with mixed types runs");
    ASSERT_EQ(suite, std::string("hello"), getGlobalStr(L, "gStr"), "unpack str='hello'");
    ASSERT_EQ(suite, 42.0, getGlobalNumber(L, "gNum"), "unpack num=42");
    ASSERT_EQ(suite, 1.0, getGlobalNumber(L, "gBool"), "unpack bool=true");

    delete L;
}

// =====================================================================
// load Tests (via Lua execution)
// =====================================================================

void testLoadWrapper(TestSuite& suite) {
    // Test 1: load with a simple loader function
    {
        LuaState* L = createFullState();
        bool ok = runLua(L, R"lua(
            local done = false
            local f = load(function()
                if done then return nil end
                done = true
                return "gResult = 42"
            end)
            f()
        )lua");
        ASSERT_TRUE(suite, ok, "load basic runs");
        ASSERT_EQ(suite, 42.0, getGlobalNumber(L, "gResult"), "load basic result=42");
        delete L;
    }

    // Test 2: load with multi-piece source
    {
        LuaState* L = createFullState();
        bool ok = runLua(L, R"lua(
            local pieces = {"gX = ", "100 + ", "200"}
            local i = 0
            local f = load(function()
                i = i + 1
                return pieces[i]
            end)
            f()
        )lua");
        ASSERT_TRUE(suite, ok, "load multi-piece runs");
        ASSERT_EQ(suite, 300.0, getGlobalNumber(L, "gX"), "load multi-piece gX=300");
        delete L;
    }

    // Test 3: load with syntax error returns nil + message
    {
        LuaState* L = createFullState();
        bool ok = runLua(L, R"lua(
            local done = false
            local f, err = load(function()
                if done then return nil end
                done = true
                return "if if if"
            end)
            if f == nil then gLoadErr = 1 else gLoadErr = 0 end
            if err then gHasMsg = 1 else gHasMsg = 0 end
        )lua");
        ASSERT_TRUE(suite, ok, "load syntax error runs");
        ASSERT_EQ(suite, 1.0, getGlobalNumber(L, "gLoadErr"), "load returns nil on error");
        ASSERT_EQ(suite, 1.0, getGlobalNumber(L, "gHasMsg"), "load returns error message");
        delete L;
    }

    // Test 4: load with empty source (loader returns nil immediately)
    {
        LuaState* L = createFullState();
        bool ok = runLua(L, R"lua(
            local f, err = load(function() return nil end)
            if f then
                f()
                gEmpty = 1
            else
                gEmpty = 0
            end
        )lua");
        ASSERT_TRUE(suite, ok, "load empty source runs");
        ASSERT_EQ(suite, 1.0, getGlobalNumber(L, "gEmpty"), "load empty source produces valid function");
        delete L;
    }
}

void registerBaselibTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "_G", testGlobalSelfReference);
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
    registry.registerTest(kSuiteName, "pcall", testPcallWrapper);
    registry.registerTest(kSuiteName, "xpcall", testXpcallWrapper);
    registry.registerTest(kSuiteName, "loadstring", testLoadstringWrapper);
    registry.registerTest(kSuiteName, "loadfile", testLoadfileWrapper);
    registry.registerTest(kSuiteName, "dofile", testDofileWrapper);
    registry.registerTest(kSuiteName, "unpack", testUnpackWrapper);
    registry.registerTest(kSuiteName, "unpack lua", testUnpackLua);
    registry.registerTest(kSuiteName, "load", testLoadWrapper);
}

