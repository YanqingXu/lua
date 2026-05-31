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

#include <iostream>
#include <sstream>
#include <string>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Base Library";
constexpr const char* kCompatibilitySuiteName = "Lua 5.1 Compatibility";

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

bool getGlobalBool(LuaState* L, const char* name) {
    Value v = L->getGlobal(name);
    return v.isBoolean() && v.asBoolean();
}

class ScopedCinRedirect {
public:
    explicit ScopedCinRedirect(std::istream& input)
        : old_(std::cin.rdbuf(input.rdbuf())) {}

    ~ScopedCinRedirect() {
        std::cin.rdbuf(old_);
        std::cin.clear();
    }

private:
    std::streambuf* old_;
};

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

    i32 ret = ctx.invoke("tostring", [](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("\0", 1));
    });
    ASSERT_EQ(suite, ret, 1, "tostring binary string returns 1 value");
    Value binary = L->top();
    ASSERT_TRUE(suite, binary.isString() && binary.asString()->getLength() == 1,
                "tostring preserves embedded NUL strings");
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
    checkTonumber([&](LuaState* s) {
        s->pushString(pool.intern(" +1.23E2 "));
    }, [](const Value& v) { return v.isNumber() && v.asNumber() == 123.0; }, "tonumber accepts signed decimal strings with surrounding whitespace");
    checkTonumber([&](LuaState* s) {
        s->pushString(pool.intern("+ 0.01"));
    }, [](const Value& v) { return v.isNil(); }, "tonumber rejects whitespace between sign and digits");

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

void testSetfenvStackLevelWrapper(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local env = {setfenv = setfenv, getfenv = getfenv, _G = _G, marker = 42}
        setfenv(1, env)
        written = marker
        local switched = getfenv(1) == env
        setfenv(1, _G)
        gSetfenvLevel = switched and env.written == 42 and _G.written == nil and getfenv(1) == _G
    )lua");
    ASSERT_TRUE(suite, ok, "setfenv stack level chunk runs");
    Value result = L->getGlobal("gSetfenvLevel");
    ASSERT_TRUE(suite, result.isBoolean() && result.asBoolean(), "setfenv(1, env) switches caller env");
    delete L;
}

void testSetfenvThreadEnvironmentWrapper(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local original = _G
        local env = {loaded_marker = 73}
        setfenv(0, env)
        local chunk = assert(loadstring("return loaded_marker"))
        local chunkEnv = getfenv(chunk)
        local value = chunk()
        setfenv(0, original)
        gSetfenvThread = (chunkEnv == env and value == 73 and getfenv(0) == original)
    )lua");
    ASSERT_TRUE(suite, ok, "setfenv(0, env) chunk runs");
    Value result = L->getGlobal("gSetfenvThread");
    ASSERT_TRUE(suite, result.isBoolean() && result.asBoolean(), "setfenv(0, env) changes loadstring default env");
    delete L;
}

void testClosureKeepsFunctionEnvironmentAfterSetfenvZero(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local function probe(env)
            setfenv(0, env)
            local foundCoroutine = coroutine ~= nil
            local stillOriginal = getfenv(1) == _G
            setfenv(0, _G)
            return foundCoroutine and stillOriginal
        end

        gClosureEnvAfterSetfenvZero = probe({})
    )lua");
    ASSERT_TRUE(suite, ok, "closure env after setfenv(0) chunk runs");
    Value result = L->getGlobal("gClosureEnvAfterSetfenvZero");
    ASSERT_TRUE(suite, result.isBoolean() && result.asBoolean(),
                "closures should keep their function env after thread env changes");
    delete L;
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

void testErrorPreservesLuaObject(TestSuite& suite) {
    LuaState* L = createFullState();

    bool ok = runLua(L, R"(
        local ok_nil, msg_nil = pcall(function() error() end)
        gErrorNilObject = (not ok_nil and msg_nil == nil)

        local payload = { msg = 'x' }
        local ok_table, msg_table = pcall(function() error(payload) end)
        gErrorTableObject = (not ok_table and msg_table == payload)

        local ok_level0, msg_level0 = pcall(function() error('hi', 0) end)
        gErrorLevelZero = (not ok_level0 and msg_level0 == 'hi')
    )");

    ASSERT_TRUE(suite, ok, "error object chunk runs");

    Value nilObject = L->getGlobal("gErrorNilObject");
    Value tableObject = L->getGlobal("gErrorTableObject");
    Value levelZero = L->getGlobal("gErrorLevelZero");
    ASSERT_TRUE(suite, nilObject.isBoolean() && nilObject.asBoolean(), "error() propagates nil object");
    ASSERT_TRUE(suite, tableObject.isBoolean() && tableObject.asBoolean(), "error(table) preserves table object");
    ASSERT_TRUE(suite, levelZero.isBoolean() && levelZero.asBoolean(), "error(message, 0) keeps raw message");
}

void testCallErrorNamesOffendingValue(TestSuite& suite) {
    LuaState* L = createFullState();

    bool ok = runLua(L, R"lua(
        local function capture(src)
            local f = assert(loadstring(src))
            local ok, msg = pcall(f)
            return msg or ''
        end

        local globalMsg = capture("bbbb = 2; bbbb()")
        gCallErrGlobal = string.find(globalMsg, "global 'bbbb'", 1, true) ~= nil

        local localMsg = capture("local bbbb = 2; bbbb()")
        gCallErrLocal = string.find(localMsg, "local 'bbbb'", 1, true) ~= nil

        local fieldMsg = capture("local a = { bbbb = 3 }; a.bbbb()")
        gCallErrField = string.find(fieldMsg, "field 'bbbb'", 1, true) ~= nil

        local methodMsg = capture("local a = { bbbb = 3 }; a:bbbb()")
        gCallErrMethod = string.find(methodMsg, "method 'bbbb'", 1, true) ~= nil

        local indexMsg = capture("local a = {13}; local bbbb = 1; a[bbbb]()")
        gCallErrIndexType = string.find(indexMsg, "number", 1, true) ~= nil
        gCallErrIndexNotLocal = string.find(indexMsg, "'bbbb'", 1, true) == nil

        local globalIndexMsg = capture("aaa = nil; aaa.bbb:ddd()")
        gIndexErrGlobal = string.find(globalIndexMsg, "global 'aaa'", 1, true) ~= nil

        local fieldIndexMsg = capture("local aaa = { bbb = 1 }; aaa.bbb:ddd()")
        gIndexErrField = string.find(fieldIndexMsg, "field 'bbb'", 1, true) ~= nil

        local upvalueArithMsg = capture("local a, b, c; (function () a = b + 1 end)()")
        gArithErrUpvalue = string.find(upvalueArithMsg, "upvalue 'b'", 1, true) ~= nil

        local localArithMsg = capture("b = 1; local aaa = 'a'; x = aaa + b")
        gArithErrLocal = string.find(localArithMsg, "local 'aaa'", 1, true) ~= nil

        local globalArithMsg = capture("aaa = {}; x = 3 / aaa")
        gArithErrGlobal = string.find(globalArithMsg, "global 'aaa'", 1, true) ~= nil

        local globalNilArithMsg = capture("aaa = '2'; b = nil; x = aaa * b")
        gArithErrGlobalNil = string.find(globalNilArithMsg, "global 'b'", 1, true) ~= nil

        local unaryArithMsg = capture("aaa = {}; x = -aaa")
        gUnaryErrGlobal = string.find(unaryArithMsg, "global 'aaa'", 1, true) ~= nil

        local expressionCallMsg = capture("aaa = {}; (aaa or aaa)()")
        gCallErrExpressionNotGlobal = string.find(expressionCallMsg, "'aaa'", 1, true) == nil
        gCallErrExpressionType = string.find(expressionCallMsg, "table", 1, true) ~= nil
    )lua");

    ASSERT_TRUE(suite, ok, "call error naming chunk runs");
    ASSERT_TRUE(suite, L->getGlobal("gCallErrGlobal").asBoolean(), "call error names global");
    ASSERT_TRUE(suite, L->getGlobal("gCallErrLocal").asBoolean(), "call error names local");
    ASSERT_TRUE(suite, L->getGlobal("gCallErrField").asBoolean(), "call error names field");
    ASSERT_TRUE(suite, L->getGlobal("gCallErrMethod").asBoolean(), "call error names method");
    ASSERT_TRUE(suite, L->getGlobal("gCallErrIndexType").asBoolean(), "call error names value type");
    ASSERT_TRUE(suite, L->getGlobal("gCallErrIndexNotLocal").asBoolean(), "indexed call does not name key local");
    ASSERT_TRUE(suite, L->getGlobal("gIndexErrGlobal").asBoolean(), "index error names global");
    ASSERT_TRUE(suite, L->getGlobal("gIndexErrField").asBoolean(), "index error names field");
    ASSERT_TRUE(suite, L->getGlobal("gArithErrUpvalue").asBoolean(), "arithmetic error names upvalue");
    ASSERT_TRUE(suite, L->getGlobal("gArithErrLocal").asBoolean(), "arithmetic error names local");
    ASSERT_TRUE(suite, L->getGlobal("gArithErrGlobal").asBoolean(), "arithmetic error names global");
    ASSERT_TRUE(suite, L->getGlobal("gArithErrGlobalNil").asBoolean(), "arithmetic error names nil global");
    ASSERT_TRUE(suite, L->getGlobal("gUnaryErrGlobal").asBoolean(), "unary arithmetic error names global");
    ASSERT_TRUE(suite, L->getGlobal("gCallErrExpressionNotGlobal").asBoolean(),
                "expression call error does not name folded global");
    ASSERT_TRUE(suite, L->getGlobal("gCallErrExpressionType").asBoolean(),
                "expression call error still names value type");
}

void testRuntimeErrorMessageCarriesLine(TestSuite& suite) {
    LuaState* L = createFullState();

    bool ok = runLua(L, R"lua(
        local f = assert(loadstring("local a\n for i=1,'a' do \n print(i) \n end"))
        local ok, msg = pcall(f)
        gRuntimeErrorLine = (not ok and string.match(msg, ":(%d+):") == "2")

        local g = assert(loadstring("\n\n for k,v in \n 3 \n do \n print(k) \n end"))
        local gok, gmsg = pcall(g)
        gGenericForRuntimeErrorLine = (not gok and string.match(gmsg, ":(%d+):") == "4")

        local p = [[
function g() f() end
function f(x) error('a', X) end
g()
]]
        local function lineerror(s)
            local ok, msg = pcall(assert(loadstring(s)))
            local line = type(msg) == "string" and string.match(msg, ":(%d+):")
            return line and line + 0
        end
        X = 3; gErrorLevel3Line = (lineerror(p) == 3)
        X = 0; gErrorLevel0Raw = (lineerror(p) == nil)
        X = 1; gErrorLevel1Line = (lineerror(p) == 2)
        X = 2; gErrorLevel2Line = (lineerror(p) == 1)
    )lua");

    ASSERT_TRUE(suite, ok, "runtime error line chunk runs");
    Value result = L->getGlobal("gRuntimeErrorLine");
    ASSERT_TRUE(suite, result.isBoolean() && result.asBoolean(), "runtime error includes failing source line");
    Value genericResult = L->getGlobal("gGenericForRuntimeErrorLine");
    ASSERT_TRUE(suite, genericResult.isBoolean() && genericResult.asBoolean(),
                "generic for runtime error includes iterator source line");
    ASSERT_TRUE(suite, L->getGlobal("gErrorLevel3Line").asBoolean(), "error level 3 reports main chunk line");
    ASSERT_TRUE(suite, L->getGlobal("gErrorLevel0Raw").asBoolean(), "error level 0 keeps raw message");
    ASSERT_TRUE(suite, L->getGlobal("gErrorLevel1Line").asBoolean(), "error level 1 reports throwing function line");
    ASSERT_TRUE(suite, L->getGlobal("gErrorLevel2Line").asBoolean(), "error level 2 reports caller line");
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
    ASSERT_TRUE(suite, L->at(-1).isString(), "xpcall error handler result is returned");
    if (L->at(-1).isString()) {
        ASSERT_EQ(suite, std::string("error handled"), std::string(L->at(-1).asString()->c_str()),
                  "xpcall calls error handler");
    }

    LuaState* fullState = createFullState();
    bool ok = runLua(fullState, R"lua(
        local ok1, msg1 = xpcall(function() error("boom") end,
            function(err) return "handled:" .. type(err) end)
        gXpcallStringHandler = (not ok1 and msg1 == "handled:string")

        local ok2, msg2 = xpcall(function() error({msg = "x"}) end,
            function(err) return {msg = err.msg .. "y"} end)
        gXpcallObjectHandler = (not ok2 and type(msg2) == "table" and msg2.msg == "xy")

        local ok3, trace = xpcall(function()
            local function inner() error("trace") end
            inner()
        end, debug.traceback)
        gXpcallTraceback = (not ok3 and type(trace) == "string" and
            string.find(trace, "stack traceback:", 1, true) ~= nil and
            string.find(trace, "trace", 1, true) ~= nil)

        local marker
        local ok4 = xpcall(function() marker = 42; error("boom") end,
            function(err) return err end)
        gXpcallPreservesOuterMutation = (not ok4 and marker == 42)
    )lua");

    ASSERT_TRUE(suite, ok, "xpcall handler chunk runs");
    ASSERT_TRUE(suite, fullState->getGlobal("gXpcallStringHandler").asBoolean(),
                "xpcall transforms string errors");
    ASSERT_TRUE(suite, fullState->getGlobal("gXpcallObjectHandler").asBoolean(),
                "xpcall transforms object errors");
    ASSERT_TRUE(suite, fullState->getGlobal("gXpcallTraceback").asBoolean(),
                "xpcall invokes traceback handler before unwinding");
    ASSERT_TRUE(suite, fullState->getGlobal("gXpcallPreservesOuterMutation").asBoolean(),
                "xpcall preserves outer-frame mutations made before the error");
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

    // 含 NUL 的源码字符串必须按 GCString 长度传给解析器，不能按 C 字符串截断
    const std::string embeddedNullSource("x = 'a\0a'", 9);
    ret = ctx.invoke("loadstring", [&](LuaState* s) {
        s->pushString(pool.intern(embeddedNullSource.data(), embeddedNullSource.size()));
    });

    ASSERT_EQ(suite, ret, 1, "loadstring accepts embedded NUL source");
    ASSERT_TRUE(suite, L->top().isFunction(), "embedded NUL loadstring returns function");

    // 测试语法错误
    ret = ctx.invoke("loadstring", [&](LuaState* s) {
        s->pushString(pool.intern("return return"));
    });

    ASSERT_EQ(suite, ret, 2, "loadstring returns 2 values on error");
    ASSERT_TRUE(suite, L->at(-2).isNil(), "first return is nil on error");
    ASSERT_TRUE(suite, L->at(-1).isString(), "second return is error message");

    ret = ctx.invoke("loadstring", [&](LuaState* s) {
        s->pushString(pool.intern("break label"));
    });

    ASSERT_EQ(suite, ret, 2, "loadstring returns 2 values for official syntax format");
    ASSERT_TRUE(suite, L->at(-2).isNil(), "official syntax first return is nil");
    ASSERT_TRUE(suite, L->at(-1).isString(), "official syntax second return is error message");
    if (L->at(-1).isString()) {
        std::string message = L->at(-1).asString()->c_str();
        ASSERT_TRUE(suite, message.find("[string \"break label\"]:1:") != std::string::npos,
                    "loadstring syntax error includes chunk id and line");
        ASSERT_TRUE(suite, message.find("near 'label'") != std::string::npos,
                    "loadstring syntax error includes near token");
    }

    ret = ctx.invoke("loadstring", [&](LuaState* s) {
        s->pushString(pool.intern("return 4.5."));
    });

    ASSERT_EQ(suite, ret, 2, "loadstring returns 2 values on malformed number");
    ASSERT_TRUE(suite, L->at(-2).isNil(), "malformed number first return is nil");
    ASSERT_TRUE(suite, L->at(-1).isString(), "malformed number second return is error message");
    if (L->at(-1).isString()) {
        const Str message = L->at(-1).asString()->getData();
        ASSERT_TRUE(suite, message.find("'4.5.'") != Str::npos,
                    "malformed number error mentions the full numeric token");
    }

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

void testUnpackNilUpperBoundUsesLength(TestSuite& suite) {
    LuaState* L = createFullState();

    bool ok = runLua(L, R"lua(
        local t = {1, 2}
        local a, b, c = unpack(t, 1, t.n)
        assert(a == 1 and b == 2 and c == nil)

        local d, e, f = table.unpack(t, 1, t.n)
        assert(d == 1 and e == 2 and f == nil)

        gUnpackNilUpperBound = 1
    )lua");
    ASSERT_TRUE(suite, ok, "unpack nil upper bound defaults to table length");
    ASSERT_EQ(suite, 1.0, getGlobalNumber(L, "gUnpackNilUpperBound"),
              "unpack nil upper bound completed");

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

    // Test 5: Lua 5.1 reader treats an empty string as end-of-input
    {
        LuaState* L = createFullState();
        bool ok = runLua(L, R"lua(
            local i = 0
            local f = load(function()
                i = i + 1
                if i == 1 then return "gLoadEmptyStringEOF = 77" end
                if i == 2 then return "" end
                error("reader called after empty string")
            end)
            assert(i == 2)
            f()
        )lua");
        ASSERT_TRUE(suite, ok, "load empty string EOF runs");
        ASSERT_EQ(suite, 77.0, getGlobalNumber(L, "gLoadEmptyStringEOF"),
                  "load stops reading after empty string");
        delete L;
    }

    // Test 6: load can read a binary chunk produced by string.dump
    {
        LuaState* L = createFullState();
        bool ok = runLua(L, R"lua(
            local dumped = string.dump(loadstring("gDumpLoaded = 1; return gDumpLoaded"))
            local i = 0
            local f = assert(load(function()
                i = i + 1
                return string.sub(dumped, i, i)
            end))
            assert(f() == 1 and gDumpLoaded == 1)
        )lua");
        ASSERT_TRUE(suite, ok, "load reads dumped binary chunk");
        ASSERT_EQ(suite, 1.0, getGlobalNumber(L, "gDumpLoaded"),
                  "loaded binary chunk executes");
        delete L;
    }

    // Test 7: loadstring can restore dumped upvalue metadata for debug.setupvalue
    {
        LuaState* L = createFullState();
        bool ok = runLua(L, R"lua(
            local a, b = 20, 30
            local x = assert(loadstring(string.dump(function (arg)
                if arg == "set" then
                    a = 10 + b
                    b = b + 1
                else
                    return a
                end
            end)))
            assert(x() == nil)
            assert(debug.setupvalue(x, 1, "hi") == "a")
            assert(x() == "hi")
            assert(debug.setupvalue(x, 2, 13) == "b")
            assert(not debug.setupvalue(x, 3, 10))
            x("set")
            assert(x() == 23)
        )lua");
        ASSERT_TRUE(suite, ok, "loadstring restores dumped upvalues");
        delete L;
    }

    // Test 8: reader-based load reports definitive syntax errors before EOF
    {
        LuaState* L = createFullState();
        bool ok = runLua(L, R"lua(
            local source = "*a = 123"
            local i = 0
            local f, err = load(function()
                i = i + 1
                return string.sub(source, i, i)
            end)
            assert(not f and type(err) == "string")
            gEarlySyntaxReads = i
        )lua");
        ASSERT_TRUE(suite, ok, "load reader early syntax error runs");
        ASSERT_EQ(suite, 2.0, getGlobalNumber(L, "gEarlySyntaxReads"),
                  "load reader stops after definitive syntax error");
        delete L;
    }

    // Test 9: reader errors are reported as load return values
    {
        LuaState* L = createFullState();
        bool ok = runLua(L, R"lua(
            local f, err = load(function() error("hhi") end)
            assert(not f and string.find(err, "hhi"))
            gLoadReaderError = 1
        )lua");
        ASSERT_TRUE(suite, ok, "load reader error returns nil and message");
        ASSERT_EQ(suite, 1.0, getGlobalNumber(L, "gLoadReaderError"),
                  "load reader error does not escape");
        delete L;
    }
}

void testPairsAllowsDeletingCurrentHashKey(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local t = {
            [{1}] = 1,
            [{2}] = 2,
            [string.rep("x ", 4)] = 3,
            [100.3] = 4,
            [4] = 5,
        }

        gPairsDeleteCount = 0
        for k, v in pairs(t) do
            assert(t[k] == v)
            gPairsDeleteCount = gPairsDeleteCount + 1
            t[k] = nil
            assert(t[k] == nil)
        end
    )lua");

    ASSERT_TRUE(suite, ok, "pairs deleting current key runs");
    ASSERT_EQ(suite, 5.0, getGlobalNumber(L, "gPairsDeleteCount"),
              "pairs continues after deleting the current hash key");
    delete L;
}

void testAutomaticGCReachesWeakValuesDuringAllocation(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local x = {[1] = {}}
        setmetatable(x, {__mode = "kv"})

        local i = 0
        while x[1] and i < 200 do
            local a = i .. i .. i .. i
            i = i + 1
        end

        gAutoWeakValueCleared = (x[1] == nil)
        gAutoWeakValueIterations = i
    )lua");

    ASSERT_TRUE(suite, ok, "automatic GC weak value chunk runs");
    ASSERT_TRUE(suite, L->getGlobal("gAutoWeakValueCleared").asBoolean(),
                "automatic GC clears weak values during allocation");
    ASSERT_TRUE(suite, getGlobalNumber(L, "gAutoWeakValueIterations") < 200.0,
                "automatic GC clears weak value before bounded loop expires");
    delete L;
}

void testCompatibilityNilTableKey(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local okSet, errSet = pcall(function()
            local t = {}
            t[nil] = 1
        end)
        gNilTableKeyRejected = (not okSet) and type(errSet) == "string" and
                               string.find(errSet, "nil", 1, true) ~= nil

        local t = { value = 1 }
        t.value = nil
        gNilAssignmentStillDeletes = next(t) == nil

        local okRaw = pcall(function()
            rawset({}, nil, 1)
        end)
        gRawsetNilKeyRejected = not okRaw
    )lua");

    ASSERT_TRUE(suite, ok, "nil table key compatibility chunk runs");
    ASSERT_TRUE(suite, getGlobalBool(L, "gNilTableKeyRejected"),
                "t[nil] assignment raises an error");
    ASSERT_TRUE(suite, getGlobalBool(L, "gNilAssignmentStillDeletes"),
                "assigning nil to an existing key still deletes it");
    ASSERT_TRUE(suite, getGlobalBool(L, "gRawsetNilKeyRejected"),
                "rawset rejects nil keys");
    delete L;
}

void testCompatibilityNumericStringConversions(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local t = {"a", "b"}
        table.insert(t, "1", "x")
        gTableInsertNumericString = t[1] == "x" and t[2] == "a" and t[3] == "b"

        local removed = table.remove(t, "1")
        gTableRemoveNumericString = removed == "x" and t[1] == "a" and t[2] == "b"

        gTableConcatNumericString = table.concat({"a", "b"}, "", "1", "2") == "ab"

        local u1, u2 = unpack({"p", "q"}, "1", "2")
        gUnpackNumericString = u1 == "p" and u2 == "q"

        local badOk = pcall(function()
            table.remove({}, "not a number")
        end)
        gBadNumericStringRejected = not badOk
    )lua");

    ASSERT_TRUE(suite, ok, "numeric string conversion compatibility chunk runs");
    ASSERT_TRUE(suite, getGlobalBool(L, "gTableInsertNumericString"),
                "table.insert accepts numeric string positions");
    ASSERT_TRUE(suite, getGlobalBool(L, "gTableRemoveNumericString"),
                "table.remove accepts numeric string positions");
    ASSERT_TRUE(suite, getGlobalBool(L, "gTableConcatNumericString"),
                "table.concat accepts numeric string range bounds");
    ASSERT_TRUE(suite, getGlobalBool(L, "gUnpackNumericString"),
                "unpack accepts numeric string range bounds");
    ASSERT_TRUE(suite, getGlobalBool(L, "gBadNumericStringRejected"),
                "non-numeric strings remain invalid numeric arguments");
    delete L;
}

void testCompatibilityDivisionModuloZero(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local z = 0
        local okDivPos = pcall(function() return 1 / z end)
        local okDivNeg = pcall(function() return -1 / z end)
        local okDivNan = pcall(function() return 0 / z end)
        local okMod, modResult = pcall(function() return 1 % z end)

        gDivisionByZeroDoesNotThrow = okDivPos and okDivNeg and okDivNan
        gModuloByZeroDoesNotThrow = okMod
        gModuloByZeroProducesNaN = okMod and modResult ~= modResult
    )lua");

    ASSERT_TRUE(suite, ok, "division/modulo zero compatibility chunk runs");
    ASSERT_TRUE(suite, getGlobalBool(L, "gDivisionByZeroDoesNotThrow"),
                "floating-point division by zero follows Lua 5.1 double behavior");
    ASSERT_TRUE(suite, getGlobalBool(L, "gModuloByZeroDoesNotThrow"),
                "modulo by zero does not raise a VM-only error");
    ASSERT_TRUE(suite, getGlobalBool(L, "gModuloByZeroProducesNaN"),
                "modulo by zero produces NaN under the double-number policy");
    delete L;
}

void testCompatibilityTableLength(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local withLen = setmetatable({1, 2, 3}, {
            __len = function() return 99 end
        })
        gTableLenIgnoresLenMetamethod = #withLen == 3

        local sparse = {}
        sparse[1] = true
        sparse[3] = true
        local sparseLen = #sparse
        gSparseBoundaryLength = sparse[sparseLen] ~= nil and sparse[sparseLen + 1] == nil

        local huge = {}
        huge[1] = true
        huge[2] = true
        huge[1000001] = true
        gHugeIntegerKeyLength = #huge == 2
    )lua");

    ASSERT_TRUE(suite, ok, "table length compatibility chunk runs");
    ASSERT_TRUE(suite, getGlobalBool(L, "gTableLenIgnoresLenMetamethod"),
                "table __len metamethod is ignored in strict Lua 5.1 mode");
    ASSERT_TRUE(suite, getGlobalBool(L, "gSparseBoundaryLength"),
                "sparse table length returns a valid Lua boundary");
    ASSERT_TRUE(suite, getGlobalBool(L, "gHugeIntegerKeyLength"),
                "large positive integer hash keys do not inflate contiguous length");
    delete L;
}

void testCompatibilityLoadfileStdin(TestSuite& suite) {
    LuaState* L = createFullState();

    std::istringstream loadInput("return 321\n");
    {
        ScopedCinRedirect redirect(loadInput);
        i32 ret = luaB_loadfile(L);
        ASSERT_EQ(suite, ret, 1, "loadfile() from stdin returns one value");
        ASSERT_TRUE(suite, L->top().isFunction(), "loadfile() from stdin returns a function");
    }

    L->setTop(0);
    std::istringstream doInput("return 654\n");
    {
        ScopedCinRedirect redirect(doInput);
        i32 ret = luaB_dofile(L);
        ASSERT_EQ(suite, ret, 1, "dofile() from stdin returns chunk results");
        ASSERT_TRUE(suite, L->top().isNumber() && L->top().asNumber() == 654.0,
                    "dofile() executes stdin chunk");
    }

    delete L;
}

void testCompatibilityOsFailureTriples(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local r1, m1, e1 = os.remove("__lua51_compat_missing_dir__/missing.file")
        gRemoveFailureTriple = r1 == nil and type(m1) == "string" and type(e1) == "number"

        local r2, m2, e2 = os.rename("__lua51_compat_missing_dir__/missing.file",
                                     "__lua51_compat_missing_dir__/renamed.file")
        gRenameFailureTriple = r2 == nil and type(m2) == "string" and type(e2) == "number"
    )lua");

    ASSERT_TRUE(suite, ok, "os failure triple compatibility chunk runs");
    ASSERT_TRUE(suite, getGlobalBool(L, "gRemoveFailureTriple"),
                "os.remove failure returns nil, message, errno");
    ASSERT_TRUE(suite, getGlobalBool(L, "gRenameFailureTriple"),
                "os.rename failure returns nil, message, errno");
    delete L;
}

void testCompatibilityIoLinesFormats(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local path = "__lua51_compat_lines.tmp"
        local f = assert(io.open(path, "w"))
        f:write("10\n20\n")
        f:close()

        local sum = 0
        for n in io.lines(path, "*n") do
            sum = sum + n
        end

        local f2 = assert(io.open(path, "r"))
        local iter = f2:lines("*l", "*n")
        local firstLine, secondNumber = iter()
        f2:close()
        os.remove(path)

        gIoLinesFormatArgs = sum == 30
        gFileLinesMultiResults = firstLine == "10" and secondNumber == 20
    )lua");

    ASSERT_TRUE(suite, ok, "io.lines format compatibility chunk runs");
    ASSERT_TRUE(suite, getGlobalBool(L, "gIoLinesFormatArgs"),
                "io.lines(filename, format) accepts read formats");
    ASSERT_TRUE(suite, getGlobalBool(L, "gFileLinesMultiResults"),
                "file:lines(format, ...) returns multiple read results per iteration");
    delete L;
}

void testCompatibilityCFunctionEnvironment(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local old = getfenv(print)
        local env = { marker = 77 }
        local ret = setfenv(print, env)
        gSetfenvCFunctionReturnsFunction = ret == print
        gGetfenvCFunctionUsesAssignedEnv = getfenv(print) == env
        setfenv(print, old)
    )lua");

    ASSERT_TRUE(suite, ok, "C function environment compatibility chunk runs");
    ASSERT_TRUE(suite, getGlobalBool(L, "gSetfenvCFunctionReturnsFunction"),
                "setfenv accepts C functions");
    ASSERT_TRUE(suite, getGlobalBool(L, "gGetfenvCFunctionUsesAssignedEnv"),
                "getfenv returns a C function's assigned environment");
    delete L;
}

void testCompatibilityErrorAndXpcall(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local payload = { x = 1 }
        local okObj, errObj = pcall(function()
            error(payload, 0)
        end)
        gErrorPreservesTableObject = not okObj and errObj == payload

        local function handler(e)
            return { handled = e }
        end
        local okX, result = xpcall(function()
            error("boom", 0)
        end, handler)
        gXpcallHandlerResult = not okX and type(result) == "table" and result.handled == "boom"

        local okFallback, fallback = xpcall(function()
            error("boom", 0)
        end, function()
            error("handler failed", 0)
        end)
        gXpcallHandlerFailureFallback = not okFallback and type(fallback) == "string" and
                                        string.find(fallback, "error in error handling", 1, true) ~= nil
    )lua");

    ASSERT_TRUE(suite, ok, "error/xpcall compatibility chunk runs");
    ASSERT_TRUE(suite, getGlobalBool(L, "gErrorPreservesTableObject"),
                "error(table, 0) preserves the error object");
    ASSERT_TRUE(suite, getGlobalBool(L, "gXpcallHandlerResult"),
                "xpcall returns the handler result");
    ASSERT_TRUE(suite, getGlobalBool(L, "gXpcallHandlerFailureFallback"),
                "xpcall reports handler failures using the fallback message");
    delete L;
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
    registry.registerTest(kSuiteName, "setfenv stack level", testSetfenvStackLevelWrapper);
    registry.registerTest(kSuiteName, "setfenv thread env", testSetfenvThreadEnvironmentWrapper);
    registry.registerTest(kSuiteName, "closure env survives setfenv zero",
                          testClosureKeepsFunctionEnvironmentAfterSetfenvZero);
    registry.registerTest(kSuiteName, "select", testSelectWrapper);
    registry.registerTest(kSuiteName, "pcall", testPcallWrapper);
    registry.registerTest(kSuiteName, "error object", testErrorPreservesLuaObject);
    registry.registerTest(kSuiteName, "call error naming", testCallErrorNamesOffendingValue);
    registry.registerTest(kSuiteName, "runtime error line", testRuntimeErrorMessageCarriesLine);
    registry.registerTest(kSuiteName, "xpcall", testXpcallWrapper);
    registry.registerTest(kSuiteName, "loadstring", testLoadstringWrapper);
    registry.registerTest(kSuiteName, "loadfile", testLoadfileWrapper);
    registry.registerTest(kSuiteName, "dofile", testDofileWrapper);
    registry.registerTest(kSuiteName, "unpack", testUnpackWrapper);
    registry.registerTest(kSuiteName, "unpack lua", testUnpackLua);
    registry.registerTest(kSuiteName, "unpack nil upper bound", testUnpackNilUpperBoundUsesLength);
    registry.registerTest(kSuiteName, "load", testLoadWrapper);
    registry.registerTest(kSuiteName, "pairs deleting current key", testPairsAllowsDeletingCurrentHashKey);
    registry.registerTest(kSuiteName, "automatic GC clears weak values",
                          testAutomaticGCReachesWeakValuesDuringAllocation);

    registry.registerTest(kCompatibilitySuiteName, "nil table key", testCompatibilityNilTableKey);
    registry.registerTest(kCompatibilitySuiteName, "numeric string conversions",
                          testCompatibilityNumericStringConversions);
    registry.registerTest(kCompatibilitySuiteName, "division and modulo by zero",
                          testCompatibilityDivisionModuloZero);
    registry.registerTest(kCompatibilitySuiteName, "table length", testCompatibilityTableLength);
    registry.registerTest(kCompatibilitySuiteName, "loadfile stdin", testCompatibilityLoadfileStdin);
    registry.registerTest(kCompatibilitySuiteName, "os failure triples",
                          testCompatibilityOsFailureTriples);
    registry.registerTest(kCompatibilitySuiteName, "io.lines formats",
                          testCompatibilityIoLinesFormats);
    registry.registerTest(kCompatibilitySuiteName, "C function environment",
                          testCompatibilityCFunctionEnvironment);
    registry.registerTest(kCompatibilitySuiteName, "error and xpcall",
                          testCompatibilityErrorAndXpcall);
}

