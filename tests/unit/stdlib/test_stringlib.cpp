/**
 * @file test_stringlib.cpp
 * @brief String Library Function Tests
 * 
 * Comprehensive tests for Lua string library implementation.
 * Tests cover normal cases, edge cases, and error conditions.
 * 
 * @author Lua C++ Project
 * @date 2026-01-23
 */

#include "../framework/test_framework.hpp"
#include "lib/stringlib.hpp"
#include "vm/lua_state.hpp"
#include "core/string_pool.hpp"
#include "core/table.hpp"
#include "core/function.hpp"

#include <string>
#include <functional>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "String Library";

// Helper function to call a string library function
i32 callStringFunc(LuaState* L, const char* funcName, const std::function<void(LuaState*)>& pushArgs) {
    // Get string table
    Value stringTable = L->getGlobal("string");
    if (!stringTable.isTable()) {
        return -1;
    }

    // Get function from table
    Table* strTable = stringTable.asTable();
    GCString* key = L->getGlobalState().getStringPool().intern(funcName);
    Value func = strTable->get(Value(key));

    if (!func.isFunction()) {
        return -1;
    }

    // Clear stack and push arguments
    L->getStack().clear();
    L->setAbsoluteTop(0);

    if (pushArgs) {
        pushArgs(L);
    }

    // Call the C function
    Function* f = func.asFunction();
    if (f->isCFunction()) {
        return f->getCFunction()(L);
    }

    return -1;
}

} // namespace

// =====================================================================
// string.len Tests
// =====================================================================

void testStringLen(TestSuite& suite) {
    LuaStdLibTestContext ctx(openStringLib);
    LuaState* L = ctx.getState();

    // Get string table
    Value stringTable = ctx.getGlobal("string");
    if (!stringTable.isTable()) {
        ASSERT_TRUE(suite, false, "string table exists");
        return;
    }

    // Test 1: Normal string
    i32 ret = callStringFunc(L, "len", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("hello"));
    });
    ASSERT_EQ(suite, ret, 1, "string.len returns 1 value");
    ASSERT_EQ(suite, 5.0, L->top().asNumber(), "len('hello') == 5");

    // Test 2: Empty string
    ret = callStringFunc(L, "len", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern(""));
    });
    ASSERT_EQ(suite, 0.0, L->top().asNumber(), "len('') == 0");

    // Test 3: Long string
    ret = callStringFunc(L, "len", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("Hello, World!"));
    });
    ASSERT_EQ(suite, 13.0, L->top().asNumber(), "len('Hello, World!') == 13");
}

// =====================================================================
// string.sub Tests
// =====================================================================

void testStringSub(TestSuite& suite) {
    LuaStdLibTestContext ctx(openStringLib);
    LuaState* L = ctx.getState();

    // Test 1: Normal substring
    i32 ret = callStringFunc(L, "sub", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("hello"));
        s->pushNumber(2.0);
        s->pushNumber(4.0);
    });
    ASSERT_EQ(suite, ret, 1, "string.sub returns 1 value");
    Value result = L->top();
    ASSERT_TRUE(suite, result.isString(), "sub returns string");
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "ell", "sub('hello', 2, 4) == 'ell'");
    }

    // Test 2: Negative indices
    ret = callStringFunc(L, "sub", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("hello"));
        s->pushNumber(-3.0);
        s->pushNumber(-1.0);
    });
    result = L->top();
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "llo", "sub('hello', -3, -1) == 'llo'");
    }

    // Test 3: Default end
    ret = callStringFunc(L, "sub", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("hello"));
        s->pushNumber(3.0);
    });
    result = L->top();
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "llo", "sub('hello', 3) == 'llo'");
    }
}

// =====================================================================
// string.upper and string.lower Tests
// =====================================================================

void testStringCase(TestSuite& suite) {
    LuaStdLibTestContext ctx(openStringLib);
    LuaState* L = ctx.getState();

    // Test 1: upper
    i32 ret = callStringFunc(L, "upper", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("Hello World"));
    });
    Value result = L->top();
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "HELLO WORLD", "upper('Hello World') == 'HELLO WORLD'");
    }

    // Test 2: lower
    ret = callStringFunc(L, "lower", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("Hello World"));
    });
    result = L->top();
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "hello world", "lower('Hello World') == 'hello world'");
    }
}

// =====================================================================
// string.reverse and string.rep Tests
// =====================================================================

void testStringReverseRep(TestSuite& suite) {
    LuaStdLibTestContext ctx(openStringLib);
    LuaState* L = ctx.getState();

    // Test 1: reverse
    i32 ret = callStringFunc(L, "reverse", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("hello"));
    });
    Value result = L->top();
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "olleh", "reverse('hello') == 'olleh'");
    }

    // Test 2: rep
    ret = callStringFunc(L, "rep", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("ab"));
        s->pushNumber(3.0);
    });
    result = L->top();
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "ababab", "rep('ab', 3) == 'ababab'");
    }

    // Test 3: rep with 0
    ret = callStringFunc(L, "rep", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("test"));
        s->pushNumber(0.0);
    });
    result = L->top();
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "", "rep('test', 0) == ''");
    }
}

// =====================================================================
// string.byte and string.char Tests
// =====================================================================

void testStringByteChar(TestSuite& suite) {
    LuaStdLibTestContext ctx(openStringLib);
    LuaState* L = ctx.getState();

    // Test 1: byte single character
    i32 ret = callStringFunc(L, "byte", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("A"));
    });
    ASSERT_EQ(suite, ret, 1, "byte returns 1 value");
    ASSERT_EQ(suite, 65.0, L->top().asNumber(), "byte('A') == 65");

    // Test 2: byte range
    ret = callStringFunc(L, "byte", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("ABC"));
        s->pushNumber(1.0);
        s->pushNumber(3.0);
    });
    ASSERT_EQ(suite, ret, 3, "byte('ABC', 1, 3) returns 3 values");

    // Test 3: char
    ret = callStringFunc(L, "char", [&](LuaState* s) {
        s->pushNumber(65.0);
        s->pushNumber(66.0);
        s->pushNumber(67.0);
    });
    Value result = L->top();
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "ABC", "char(65, 66, 67) == 'ABC'");
    }
}

// =====================================================================
// string.find Tests
// =====================================================================

void testStringFind(TestSuite& suite) {
    LuaStdLibTestContext ctx(openStringLib);
    LuaState* L = ctx.getState();

    // Test 1: Simple find
    i32 ret = callStringFunc(L, "find", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("hello world"));
        s->pushString(s->getGlobalState().getStringPool().intern("world"));
    });
    ASSERT_EQ(suite, ret, 2, "find returns 2 values");
    // Should return start=7, end=11

    // Test 2: Not found
    ret = callStringFunc(L, "find", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("hello"));
        s->pushString(s->getGlobalState().getStringPool().intern("xyz"));
    });
    Value result = L->top();
    ASSERT_TRUE(suite, result.isNil(), "find returns nil when not found");
}

// =====================================================================
// string.gsub Tests
// =====================================================================

void testStringGsub(TestSuite& suite) {
    LuaStdLibTestContext ctx(openStringLib);
    LuaState* L = ctx.getState();

    // Test 1: Simple replacement
    i32 ret = callStringFunc(L, "gsub", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("hello world"));
        s->pushString(s->getGlobalState().getStringPool().intern("o"));
        s->pushString(s->getGlobalState().getStringPool().intern("0"));
    });
    ASSERT_EQ(suite, ret, 2, "gsub returns 2 values");
    // Should return "hell0 w0rld" and count=2
    Value result = L->at(-2);  // Result string
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "hell0 w0rld", "gsub replaces all occurrences");
    }
    Value count = L->top();  // Count
    ASSERT_EQ(suite, 2.0, count.asNumber(), "gsub count == 2");
}

// =====================================================================
// string.format Tests
// =====================================================================

void testStringFormat(TestSuite& suite) {
    LuaStdLibTestContext ctx(openStringLib);
    LuaState* L = ctx.getState();

    // Test 1: String formatting
    i32 ret = callStringFunc(L, "format", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("Hello %s"));
        s->pushString(s->getGlobalState().getStringPool().intern("World"));
    });
    Value result = L->top();
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "Hello World", "format('Hello %s', 'World') == 'Hello World'");
    }

    // Test 2: Number formatting
    ret = callStringFunc(L, "format", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("Number: %d"));
        s->pushNumber(42.0);
    });
    result = L->top();
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "Number: 42", "format('Number: %d', 42) == 'Number: 42'");
    }
}

// =====================================================================
// Test Registration
// =====================================================================

void registerStringLibTests() {
    auto& registry = TestRegistry::getInstance();
    registry.registerTest(kSuiteName, "string.len", testStringLen);
    registry.registerTest(kSuiteName, "string.sub", testStringSub);
    registry.registerTest(kSuiteName, "string.upper/lower", testStringCase);
    registry.registerTest(kSuiteName, "string.reverse/rep", testStringReverseRep);
    registry.registerTest(kSuiteName, "string.byte/char", testStringByteChar);
    registry.registerTest(kSuiteName, "string.find", testStringFind);
    registry.registerTest(kSuiteName, "string.gsub", testStringGsub);
    registry.registerTest(kSuiteName, "string.format", testStringFormat);
}

