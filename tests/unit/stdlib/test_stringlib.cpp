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
#include "lib/lib_manager.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm.hpp"
#include "core/string_pool.hpp"
#include "core/table.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "compiler/parser/parser.hpp"
#include "compiler/codegen/codegen.hpp"

#include <string>
#include <functional>
#include <limits>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "String Library";

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
        if (!proto)
            return false;

        Function* func = new Function(proto);
        L->getGlobalState().getGC().registerObject(func);
        func->setEnv(L->getGlobalTable());
        VM::execute(L, func);
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

/// Helper: get global string including embedded NUL bytes
std::string getGlobalBytes(LuaState* L, const char* name) {
    Value v = L->getGlobal(name);
    if (!v.isString()) {
        return "";
    }
    GCString* str = v.asString();
    return std::string(str->c_str(), str->getLength());
}

/// Helper: create state with all standard libraries
LuaState* createFullState() {
    LuaState* L = LuaState::newState();
    StandardLibrary::openAll(L);
    return L;
}

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
    i32 ret = callStringFunc(L, "len",
                             [&](LuaState* s) { s->pushString(s->getGlobalState().getStringPool().intern("hello")); });
    ASSERT_EQ(suite, ret, 1, "string.len returns 1 value");
    ASSERT_EQ(suite, 5.0, L->top().asNumber(), "len('hello') == 5");

    // Test 2: Empty string
    ret = callStringFunc(L, "len", [&](LuaState* s) { s->pushString(s->getGlobalState().getStringPool().intern("")); });
    ASSERT_EQ(suite, 0.0, L->top().asNumber(), "len('') == 0");

    // Test 3: Long string
    ret = callStringFunc(
        L, "len", [&](LuaState* s) { s->pushString(s->getGlobalState().getStringPool().intern("Hello, World!")); });
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

    // Test 4: Lua 5.1 boundary normalization
    ret = callStringFunc(L, "sub", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("123456789"));
        s->pushNumber(0.0);
        s->pushNumber(0.0);
    });
    result = L->top();
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str.empty(), "sub('123456789', 0, 0) == ''");
    }

    ret = callStringFunc(L, "sub", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("123456789"));
        s->pushNumber(-10.0);
        s->pushNumber(10.0);
    });
    result = L->top();
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "123456789", "sub('123456789', -10, 10) clamps to full string");
    }
}

// =====================================================================
// string.upper and string.lower Tests
// =====================================================================

void testStringCase(TestSuite& suite) {
    LuaStdLibTestContext ctx(openStringLib);
    LuaState* L = ctx.getState();

    // Test 1: upper
    i32 ret = callStringFunc(
        L, "upper", [&](LuaState* s) { s->pushString(s->getGlobalState().getStringPool().intern("Hello World")); });
    Value result = L->top();
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "HELLO WORLD", "upper('Hello World') == 'HELLO WORLD'");
    }

    // Test 2: lower
    ret = callStringFunc(
        L, "lower", [&](LuaState* s) { s->pushString(s->getGlobalState().getStringPool().intern("Hello World")); });
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
    i32 ret = callStringFunc(L, "reverse",
                             [&](LuaState* s) { s->pushString(s->getGlobalState().getStringPool().intern("hello")); });
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
    i32 ret =
        callStringFunc(L, "byte", [&](LuaState* s) { s->pushString(s->getGlobalState().getStringPool().intern("A")); });
    ASSERT_EQ(suite, ret, 1, "byte returns 1 value");
    ASSERT_EQ(suite, 65.0, L->top().asNumber(), "byte('A') == 65");

    // Test 2: byte range
    ret = callStringFunc(L, "byte", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("ABC"));
        s->pushNumber(1.0);
        s->pushNumber(3.0);
    });
    ASSERT_EQ(suite, ret, 3, "byte('ABC', 1, 3) returns 3 values");

    // Test 3: byte boundary normalization
    ret = callStringFunc(L, "byte", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("hi"));
        s->pushNumber(-3.0);
    });
    ASSERT_EQ(suite, ret, 0, "byte('hi', -3) returns no values");

    ret = callStringFunc(L, "byte", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("hi"));
        s->pushNumber(3.0);
    });
    ASSERT_EQ(suite, ret, 0, "byte('hi', 3) returns no values");

    // Test 4: char
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
    Value result = L->at(-2); // Result string
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "hell0 w0rld", "gsub replaces all occurrences");
    }
    Value count = L->top(); // Count
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

    // Test 3: Width and precision
    ret = callStringFunc(L, "format", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("[%8.2f]"));
        s->pushNumber(3.14159);
    });
    result = L->top();
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "[    3.14]", "format width+precision for %f");
    }

    // Test 4: Zero padding and sign flag
    ret = callStringFunc(L, "format", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("%+05d"));
        s->pushNumber(42.0);
    });
    result = L->top();
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "+0042", "format sign+zero padding for %d");
    }

    // Test 5: Scientific notation
    ret = callStringFunc(L, "format", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("%.2e"));
        s->pushNumber(1234.0);
    });
    result = L->top();
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "1.23e+03", "format('%.2e', 1234) == '1.23e+03'");
    }

    // Test 6: General floating-point format
    ret = callStringFunc(L, "format", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("%.4g"));
        s->pushNumber(1234.5678);
    });
    result = L->top();
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "1235", "format('%.4g', 1234.5678) == '1235'");
    }

    // Test 7: Hex and octal formatting
    ret = callStringFunc(L, "format", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("%#x %#o"));
        s->pushNumber(255.0);
        s->pushNumber(9.0);
    });
    result = L->top();
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "0xff 011", "format supports %#x and %#o");
    }

    // Test 8: Character formatting
    ret = callStringFunc(L, "format", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("%c"));
        s->pushNumber(65.0);
    });
    result = L->top();
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "A", "format('%c', 65) == 'A'");
    }

    // Test 9: Quoted strings
    ret = callStringFunc(L, "format", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("%q"));
        s->pushString(s->getGlobalState().getStringPool().intern("line\n\"quoted\""));
    });
    result = L->top();
    if (result.isString()) {
        std::string str(result.asString()->getData());
        std::string expected = "\"line\\";
        expected.push_back('\n');
        expected += "\\\"quoted\\\"\"";
        ASSERT_TRUE(suite, str == expected, "format('%q', s) quotes and escapes");
    }

    // Test 10: Lua 5.1 %q preserves high-bit bytes and appends raw %s data
    ret = callStringFunc(L, "format", [&](LuaState* s) {
        std::string raw;
        raw.push_back('"');
        raw.push_back(static_cast<char>(0xED));
        raw += "lo\"\n\\";

        s->pushString(s->getGlobalState().getStringPool().intern("%q%s"));
        s->pushString(s->getGlobalState().getStringPool().intern(raw.data(), raw.size()));
        s->pushString(s->getGlobalState().getStringPool().intern(raw.data(), raw.size()));
    });
    result = L->top();
    if (result.isString()) {
        std::string raw;
        raw.push_back('"');
        raw.push_back(static_cast<char>(0xED));
        raw += "lo\"\n\\";

        std::string quoted;
        quoted.push_back('"');
        quoted += "\\\"";
        quoted.push_back(static_cast<char>(0xED));
        quoted += "lo\\\"";
        quoted.push_back('\\');
        quoted.push_back('\n');
        quoted += "\\\\";
        quoted.push_back('"');

        std::string str(result.asString()->getData());
        ASSERT_TRUE(suite, str == quoted + raw, "format('%q%s', binary, binary) follows Lua 5.1 quoting");
    }

    // Test 11: Escaped percent
    ret = callStringFunc(L, "format",
                         [&](LuaState* s) { s->pushString(s->getGlobalState().getStringPool().intern("100%% done")); });
    result = L->top();
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "100% done", "format handles %% escape");
    }

    // Test 12: %s accepts numbers via tostring semantics
    ret = callStringFunc(L, "format", [&](LuaState* s) {
        s->pushString(s->getGlobalState().getStringPool().intern("value=%s"));
        s->pushNumber(12.5);
    });
    result = L->top();
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "value=12.5", "%s accepts number arguments");
    }

    // Test 13: Not enough arguments throws
    ret = callStringFunc(L, "format", [&](LuaState* s) {
        auto& pool = s->getGlobalState().getStringPool();
        s->pushString(pool.intern("%02d/%d"));
        s->pushString(pool.intern("7"));
        s->pushString(pool.intern("2026"));
    });
    result = L->top();
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "07/2026", "format numeric specifiers accept numeric strings");
    }

    // Test 14: Not enough arguments throws
    bool notEnoughArgs = false;
    try {
        callStringFunc(L, "format", [&](LuaState* s) {
            s->pushString(s->getGlobalState().getStringPool().intern("%d %d"));
            s->pushNumber(1.0);
        });
    } catch (const std::runtime_error& e) {
        notEnoughArgs = std::string(e.what()) == "string.format: not enough arguments";
    }
    ASSERT_TRUE(suite, notEnoughArgs, "format throws when arguments are missing");

    // Test 15: Invalid format option throws
    bool invalidOption = false;
    try {
        callStringFunc(L, "format", [&](LuaState* s) {
            s->pushString(s->getGlobalState().getStringPool().intern("%p"));
            s->pushNumber(1.0);
        });
    } catch (const std::runtime_error& e) {
        invalidOption = std::string(e.what()) == "invalid option '%p' to 'format'";
    }
    ASSERT_TRUE(suite, invalidOption, "format throws on unsupported specifier");
}

// =====================================================================
// string.gmatch Tests (via Lua execution — requires for-in loop)
// =====================================================================

void testStringGmatchBasic(TestSuite& suite) {
    LuaState* L = createFullState();

    // Test 1: Collect all words with %a+
    bool ok = runLua(L, R"(
        gAlias = (string.gfind == string.gmatch)
        local result = ""
        local count = 0
        for w in string.gmatch("hello world foo", "%a+") do
            result = result .. w .. ","
            count = count + 1
        end
        gResult = result
        gCount = count
    )");
    ASSERT_TRUE(suite, ok, "gmatch basic word iteration runs");
    ASSERT_TRUE(suite, L->getGlobal("gAlias").asBoolean(), "gfind aliases gmatch");
    ASSERT_EQ(suite, std::string("hello,world,foo,"), getGlobalStr(L, "gResult"), "gmatch(%a+) collects all words");
    ASSERT_EQ(suite, 3.0, getGlobalNumber(L, "gCount"), "gmatch(%a+) finds 3 words");

    delete L;
}

void testStringGmatchDigits(TestSuite& suite) {
    LuaState* L = createFullState();

    // Test: Collect all numbers with %d+
    bool ok = runLua(L, R"(
        local result = ""
        local count = 0
        for n in string.gmatch("abc 123 def 456 ghi 789", "%d+") do
            result = result .. n .. ","
            count = count + 1
        end
        gResult = result
        gCount = count
    )");
    ASSERT_TRUE(suite, ok, "gmatch digit iteration runs");
    ASSERT_EQ(suite, std::string("123,456,789,"), getGlobalStr(L, "gResult"), "gmatch(%d+) collects all numbers");
    ASSERT_EQ(suite, 3.0, getGlobalNumber(L, "gCount"), "gmatch(%d+) finds 3 numbers");

    delete L;
}

void testStringGmatchCaptures(TestSuite& suite) {
    LuaState* L = createFullState();

    // Test: Capture key=value pairs
    bool ok = runLua(L, R"lua(
        local keys = ""
        local vals = ""
        local count = 0
        for k, v in string.gmatch("name=John age=30 city=NYC", "(%w+)=(%w+)") do
            keys = keys .. k .. ","
            vals = vals .. v .. ","
            count = count + 1
        end
        gKeys = keys
        gVals = vals
        gCount = count
    )lua");
    ASSERT_TRUE(suite, ok, "gmatch capture pairs runs");
    ASSERT_EQ(suite, std::string("name,age,city,"), getGlobalStr(L, "gKeys"), "gmatch captures keys correctly");
    ASSERT_EQ(suite, std::string("John,30,NYC,"), getGlobalStr(L, "gVals"), "gmatch captures values correctly");
    ASSERT_EQ(suite, 3.0, getGlobalNumber(L, "gCount"), "gmatch finds 3 pairs");

    delete L;
}

void testStringGmatchSingleChar(TestSuite& suite) {
    LuaState* L = createFullState();

    // Test: Match each character with "."
    bool ok = runLua(L, R"(
        local result = ""
        local count = 0
        for c in string.gmatch("abc", ".") do
            result = result .. c
            count = count + 1
        end
        gResult = result
        gCount = count
    )");
    ASSERT_TRUE(suite, ok, "gmatch single char runs");
    ASSERT_EQ(suite, std::string("abc"), getGlobalStr(L, "gResult"), "gmatch(.) matches each character");
    ASSERT_EQ(suite, 3.0, getGlobalNumber(L, "gCount"), "gmatch(.) finds 3 characters");

    delete L;
}

void testStringGmatchEmptyString(TestSuite& suite) {
    LuaState* L = createFullState();

    // Test: No matches on empty string
    bool ok = runLua(L, R"(
        local count = 0
        for w in string.gmatch("", "%a+") do
            count = count + 1
        end
        gCount = count
    )");
    ASSERT_TRUE(suite, ok, "gmatch on empty string runs");
    ASSERT_EQ(suite, 0.0, getGlobalNumber(L, "gCount"), "gmatch on empty string yields nothing");

    delete L;
}

void testStringGmatchNoMatch(TestSuite& suite) {
    LuaState* L = createFullState();

    // Test: Pattern doesn't match anything
    bool ok = runLua(L, R"(
        local count = 0
        for w in string.gmatch("hello world", "%d+") do
            count = count + 1
        end
        gCount = count
    )");
    ASSERT_TRUE(suite, ok, "gmatch no match runs");
    ASSERT_EQ(suite, 0.0, getGlobalNumber(L, "gCount"), "gmatch returns nothing when no matches");

    delete L;
}

// =====================================================================
// Pattern matching tests for string.find (pattern mode)
// =====================================================================

void testStringFindPattern(TestSuite& suite) {
    LuaState* L = createFullState();

    // Test 1: find with character class pattern
    bool ok = runLua(L, R"(
        local s, e = string.find("hello123world", "%d+")
        gStart = s
        gEnd = e
    )");
    ASSERT_TRUE(suite, ok, "find pattern %d+ runs");
    ASSERT_EQ(suite, 6.0, getGlobalNumber(L, "gStart"), "find(%d+) start=6");
    ASSERT_EQ(suite, 8.0, getGlobalNumber(L, "gEnd"), "find(%d+) end=8");

    // Test 2: find with anchor
    ok = runLua(L, R"(
        local s, e = string.find("hello", "^hel")
        gStart2 = s
        gEnd2 = e
    )");
    ASSERT_TRUE(suite, ok, "find anchor pattern runs");
    ASSERT_EQ(suite, 1.0, getGlobalNumber(L, "gStart2"), "find(^hel) start=1");
    ASSERT_EQ(suite, 3.0, getGlobalNumber(L, "gEnd2"), "find(^hel) end=3");

    // Test 3: find with capture returns captures too
    ok = runLua(L, R"lua(
        local s, e, cap = string.find("key=val", "(%a+)=")
        gStart3 = s
        gEnd3 = e
        gCap3 = cap
    )lua");
    ASSERT_TRUE(suite, ok, "find with capture runs");
    ASSERT_EQ(suite, 1.0, getGlobalNumber(L, "gStart3"), "find capture start=1");
    ASSERT_EQ(suite, 4.0, getGlobalNumber(L, "gEnd3"), "find capture end=4");
    ASSERT_EQ(suite, std::string("key"), getGlobalStr(L, "gCap3"), "find capture='key'");

    delete L;
}

// =====================================================================
// Pattern matching tests for string.match
// =====================================================================

void testStringMatchPattern(TestSuite& suite) {
    LuaState* L = createFullState();

    // Test 1: match returns first capture
    bool ok = runLua(L, R"(
        gResult = string.match("hello123", "%d+")
    )");
    ASSERT_TRUE(suite, ok, "match %d+ runs");
    ASSERT_EQ(suite, std::string("123"), getGlobalStr(L, "gResult"), "match(%d+) returns '123'");

    // Test 2: match with captures
    ok = runLua(L, R"lua(
        local k, v = string.match("name=John", "(%a+)=(%a+)")
        gKey = k
        gVal = v
    )lua");
    ASSERT_TRUE(suite, ok, "match captures runs");
    ASSERT_EQ(suite, std::string("name"), getGlobalStr(L, "gKey"), "match key='name'");
    ASSERT_EQ(suite, std::string("John"), getGlobalStr(L, "gVal"), "match val='John'");

    // Test 3: match returns nil on no match
    ok = runLua(L, R"(
        local r = string.match("hello", "%d+")
        if r == nil then gIsNil = 1 else gIsNil = 0 end
    )");
    ASSERT_TRUE(suite, ok, "match nil runs");
    ASSERT_EQ(suite, 1.0, getGlobalNumber(L, "gIsNil"), "match returns nil on no match");

    delete L;
}

// =====================================================================
// Pattern matching tests for string.gsub (pattern mode)
// =====================================================================

void testStringGsubPattern(TestSuite& suite) {
    LuaState* L = createFullState();

    // Test 1: gsub with character class pattern
    bool ok = runLua(L, R"(
        local r, n = string.gsub("abc 123 def 456", "%d+", "NUM")
        gResult = r
        gCount = n
    )");
    ASSERT_TRUE(suite, ok, "gsub pattern %d+ runs");
    ASSERT_EQ(suite, std::string("abc NUM def NUM"), getGlobalStr(L, "gResult"), "gsub(%d+, NUM) replaces numbers");
    ASSERT_EQ(suite, 2.0, getGlobalNumber(L, "gCount"), "gsub replaced 2 times");

    // Test 2: gsub with capture substitution
    ok = runLua(L, R"lua(
        local r, n = string.gsub("hello world", "(%a+)", "[%1]")
        gResult2 = r
        gCount2 = n
    )lua");
    ASSERT_TRUE(suite, ok, "gsub capture substitution runs");
    ASSERT_EQ(suite, std::string("[hello] [world]"), getGlobalStr(L, "gResult2"), "gsub with capture substitution");
    ASSERT_EQ(suite, 2.0, getGlobalNumber(L, "gCount2"), "gsub replaced 2 words");

    // Test 3: gsub with max replacements
    ok = runLua(L, R"(
        local r, n = string.gsub("aaa", "a", "b", 2)
        gResult3 = r
        gCount3 = n
    )");
    ASSERT_TRUE(suite, ok, "gsub max replacements runs");
    ASSERT_EQ(suite, std::string("bba"), getGlobalStr(L, "gResult3"), "gsub with n=2 replaces first 2 only");
    ASSERT_EQ(suite, 2.0, getGlobalNumber(L, "gCount3"), "gsub count=2");

    delete L;
}

void testStringGsubTableReplacement(TestSuite& suite) {
    LuaState* L = createFullState();

    bool ok = runLua(L, R"lua(
        local r, n = string.gsub("a b c d", "(%a)", { a = "A", b = false, c = nil, d = "D" })
        gResult = r
        gCount = n
    )lua");
    ASSERT_TRUE(suite, ok, "gsub table replacement runs");
    ASSERT_EQ(suite, std::string("A b c D"), getGlobalStr(L, "gResult"),
              "gsub table uses first capture as lookup key and preserves false/nil matches");
    ASSERT_EQ(suite, 4.0, getGlobalNumber(L, "gCount"), "gsub table replacement counts all matches");

    ok = runLua(L, R"lua(
        local r = string.gsub("foo bar", "%a+", { foo = "FOO" })
        gResult2 = r
    )lua");
    ASSERT_TRUE(suite, ok, "gsub table replacement without captures runs");
    ASSERT_EQ(suite, std::string("FOO bar"), getGlobalStr(L, "gResult2"),
              "gsub table uses whole match when there are no captures");

    ok = runLua(L, R"lua(
        local r = string.gsub("ab", "(.)", { a = "%1", b = 7 })
        gResult3 = r
    )lua");
    ASSERT_TRUE(suite, ok, "gsub table raw replacement values run");
    ASSERT_EQ(suite, std::string("%17"), getGlobalStr(L, "gResult3"),
              "gsub table replacement values are raw and numbers stringify");

    ok = runLua(L, R"lua(
        local repl = setmetatable({}, {__index = function(_, key) return string.upper(key) end})
        local r = string.gsub("a alo b hi", "%w%w+", repl)
        gResult4 = r
    )lua");
    ASSERT_TRUE(suite, ok, "gsub table replacement __index runs");
    ASSERT_EQ(suite, std::string("a ALO b HI"), getGlobalStr(L, "gResult4"),
              "gsub table replacement uses __index metamethod");

    ok = runLua(L, R"lua(
        gBadCloseCapture = not pcall(string.gsub, "alo", ".)", {})
        gBadBackrefZero = not pcall(string.gsub, "alo", "(%0)", "a")
        gBadBackrefOne = not pcall(string.gsub, "alo", "(%1)", "a")
    )lua");
    ASSERT_TRUE(suite, ok, "gsub malformed pattern checks run");
    ASSERT_TRUE(suite, L->getGlobal("gBadCloseCapture").asBoolean(), "gsub rejects unmatched close capture");
    ASSERT_TRUE(suite, L->getGlobal("gBadBackrefZero").asBoolean(), "gsub rejects invalid %0 pattern capture");
    ASSERT_TRUE(suite, L->getGlobal("gBadBackrefOne").asBoolean(), "gsub rejects invalid %1 pattern capture");

    delete L;
}

void testStringGsubFunctionReplacement(TestSuite& suite) {
    LuaState* L = createFullState();

    bool ok = runLua(L, R"lua(
        local calls = ""
        local r, n = string.gsub("x=1 y=2 z=3", "(%a)=(%d)", function(k, v)
            calls = calls .. k .. v
            if k == "y" then return false end
            return k .. ":" .. v
        end)
        gResult = r
        gCalls = calls
        gCount = n
    )lua");
    ASSERT_TRUE(suite, ok, "gsub function replacement runs");
    ASSERT_EQ(suite, std::string("x:1 y=2 z:3"), getGlobalStr(L, "gResult"),
              "gsub function uses captures and preserves false return matches");
    ASSERT_EQ(suite, std::string("x1y2z3"), getGlobalStr(L, "gCalls"), "gsub function receives all captures");
    ASSERT_EQ(suite, 3.0, getGlobalNumber(L, "gCount"), "gsub function replacement counts all matches");

    ok = runLua(L, R"lua(
        local r = string.gsub("abc", ".", function(ch) return "[" .. ch .. "]" end)
        gResult2 = r
    )lua");
    ASSERT_TRUE(suite, ok, "gsub function replacement without captures runs");
    ASSERT_EQ(suite, std::string("[a][b][c]"), getGlobalStr(L, "gResult2"),
              "gsub function receives whole match when there are no captures");

    ok = runLua(L, R"lua(
        local r = string.gsub("ab", "(.)", function() return "%1" end)
        gResult3 = r
    )lua");
    ASSERT_TRUE(suite, ok, "gsub function raw replacement values run");
    ASSERT_EQ(suite, std::string("%1%1"), getGlobalStr(L, "gResult3"),
              "gsub function replacement strings are not capture-expanded again");

    ok = runLua(L, R"lua(
        local r = string.gsub("abc", "%w", "%1%0")
        gResult4 = r
    )lua");
    ASSERT_TRUE(suite, ok, "gsub replacement whole match capture without explicit captures runs");
    ASSERT_EQ(suite, std::string("aabbcc"), getGlobalStr(L, "gResult4"),
              "gsub replacement %1 falls back to whole match when there are no explicit captures");

    ok = runLua(L, R"lua(
        local function rev(s)
            return string.gsub(s, "(.)(.+)", function(c, s1) return rev(s1) .. c end)
        end

        local x = string.rep("012345", 10)
        gDeepResult = (rev(rev(x)) == x)
    )lua");
    ASSERT_TRUE(suite, ok, "recursive gsub function replacement runs");
    ASSERT_TRUE(suite, L->getGlobal("gDeepResult").asBoolean(), "recursive gsub function replacement preserves result");

    ok = runLua(L, R"lua(
        local function rev(s)
            return string.gsub(s, "(.)(.+)", function(c, s1) return rev(s1) .. c end)
        end

        local ok, msg = pcall(rev, string.rep("012345", 40))
        gDeepErrorBounded =
            (not ok and string.find(tostring(msg), "stack overflow", 1, true) ~= nil)
    )lua");
    ASSERT_TRUE(suite, ok, "over-deep recursive gsub replacement chunk runs");
    ASSERT_TRUE(suite, L->getGlobal("gDeepErrorBounded").asBoolean(),
                "over-deep recursive gsub replacement is caught by VM call-depth guard");

    delete L;
}

void testStringBinarySafety(TestSuite& suite) {
    LuaState* L = createFullState();

    bool ok = runLua(L, R"lua(
        local s = string.char(65, 0, 66, 0, 67)
        gLen = string.len(s)
        gSub = string.sub(s, 1, 5)
        local r, n = string.gsub(s, "%z", "Z")
        gGsub = r
        gCount = n
        local fs, fe = string.find(s, string.char(0, 66), 1, true)
        gFindStart = fs
        gFindEnd = fe
        local ps, pe = string.find("a\0o a\0o a\0o", "a\0o", 2)
        gPatternFindStart = ps
        gPatternFindEnd = pe
    )lua");
    ASSERT_TRUE(suite, ok, "binary-safe string operations run");
    ASSERT_EQ(suite, 5.0, getGlobalNumber(L, "gLen"), "string.len counts embedded NUL bytes");
    ASSERT_EQ(suite, std::string("A\0B\0C", 5), getGlobalBytes(L, "gSub"), "string.sub preserves embedded NUL bytes");
    ASSERT_EQ(suite, std::string("AZBZC"), getGlobalBytes(L, "gGsub"), "string.gsub can replace embedded NUL bytes");
    ASSERT_EQ(suite, 2.0, getGlobalNumber(L, "gCount"), "string.gsub counts NUL replacements");
    ASSERT_EQ(suite, 2.0, getGlobalNumber(L, "gFindStart"), "plain string.find can locate embedded NUL sequence start");
    ASSERT_EQ(suite, 3.0, getGlobalNumber(L, "gFindEnd"), "plain string.find can locate embedded NUL sequence end");
    ASSERT_EQ(suite, 5.0, getGlobalNumber(L, "gPatternFindStart"),
              "pattern string.find can locate embedded NUL sequence start");
    ASSERT_EQ(suite, 7.0, getGlobalNumber(L, "gPatternFindEnd"),
              "pattern string.find can locate embedded NUL sequence end");

    delete L;
}

void testStringDump(TestSuite& suite) {
    LuaState* L = createFullState();

    bool ok = runLua(L, R"lua(
        local dumped = string.dump(function(a) return a + 1 end)
        gDumpType = type(dumped)
        gDumpLen = string.len(dumped)
        gDumpPrefix = string.sub(dumped, 1, 4)
        gDumpVersion = string.byte(dumped, 5)
        gDumpFormat = string.byte(dumped, 6)
        gDumpIntSize = string.byte(dumped, 8)
        gDumpSizeTSize = string.byte(dumped, 9)
        gDumpInstructionSize = string.byte(dumped, 10)
        gDumpNumberSize = string.byte(dumped, 11)
        gDumpNumberIntegral = string.byte(dumped, 12)
        gDumpPrivateMarker = string.sub(dumped, 13, 16)
    )lua");
    ASSERT_TRUE(suite, ok, "string.dump returns for Lua function");
    ASSERT_EQ(suite, std::string("string"), getGlobalStr(L, "gDumpType"), "string.dump returns a string");
    ASSERT_TRUE(suite, getGlobalNumber(L, "gDumpLen") > 12.0, "string.dump returns a non-trivial binary chunk");
    ASSERT_EQ(suite, std::string("\x1bLua", 4), getGlobalBytes(L, "gDumpPrefix"),
              "string.dump chunk starts with Lua signature");
    ASSERT_EQ(suite, 81.0, getGlobalNumber(L, "gDumpVersion"), "string.dump writes Lua 5.1 chunk version");
    ASSERT_EQ(suite, 0.0, getGlobalNumber(L, "gDumpFormat"), "string.dump writes official format 0");
    ASSERT_EQ(suite, 4.0, getGlobalNumber(L, "gDumpIntSize"), "string.dump writes 4-byte int size");
    ASSERT_TRUE(suite, getGlobalNumber(L, "gDumpSizeTSize") == 4.0 || getGlobalNumber(L, "gDumpSizeTSize") == 8.0,
                "string.dump writes a platform size_t size");
    ASSERT_EQ(suite, 4.0, getGlobalNumber(L, "gDumpInstructionSize"), "string.dump writes 4-byte instruction size");
    ASSERT_EQ(suite, 8.0, getGlobalNumber(L, "gDumpNumberSize"), "string.dump writes 8-byte lua_Number size");
    ASSERT_EQ(suite, 0.0, getGlobalNumber(L, "gDumpNumberIntegral"),
              "string.dump writes floating-point lua_Number flag");
    ASSERT_EQ(suite, std::string("LC++", 4), getGlobalBytes(L, "gDumpPrivateMarker"),
              "string.dump writes the project-local LC++ marker for locvar register metadata");

    ok = runLua(L, R"lua(
        local ok = pcall(function() return string.dump(print) end)
        gDumpCFunctionFailed = ok and 0 or 1
    )lua");
    ASSERT_TRUE(suite, ok, "string.dump C function error check runs");
    ASSERT_EQ(suite, 1.0, getGlobalNumber(L, "gDumpCFunctionFailed"), "string.dump rejects C functions");

    delete L;
}

void testStringResourceAndIntegerBoundaries(TestSuite& suite) {
    {
        LuaStdLibTestContext ctx(openStringLib);
        LuaState* L = ctx.getState();
        GCString* input = L->getGlobalState().getStringPool().intern("ab");
        const usize oldOutputLimit = L->getGlobalState().getResourcePolicy().maxOutputBytes;
        L->getGlobalState().getResourcePolicy().maxOutputBytes = 5;
        bool rejected = false;
        try {
            (void)callStringFunc(L, "rep", [&](LuaState* state) {
                state->pushString(input);
                state->pushNumber(3);
            });
        } catch (const std::exception& error) {
            rejected = std::string(error.what()).find("result exceeds resource limit") != std::string::npos;
        }
        L->getGlobalState().getResourcePolicy().maxOutputBytes = oldOutputLimit;
        ASSERT_TRUE(suite, rejected, "string.rep checks multiplication and output limit before allocation");
    }

    {
        LuaStdLibTestContext ctx(openStringLib);
        LuaState* L = ctx.getState();
        StringPool& pool = L->getGlobalState().getStringPool();
        const usize oldPatternLimit = L->getGlobalState().getResourcePolicy().maxPatternSteps;
        L->getGlobalState().getResourcePolicy().maxPatternSteps = 4;
        bool rejected = false;
        try {
            (void)callStringFunc(L, "find", [&](LuaState* state) {
                state->pushString(pool.intern("aaaaaaaaaaaaaaaa"));
                state->pushString(pool.intern("a*a*a*a*b"));
            });
        } catch (const RuntimeError& error) {
            rejected = std::string(error.what()).find("pattern step limit exceeded") != std::string::npos;
        }
        L->getGlobalState().getResourcePolicy().maxPatternSteps = oldPatternLimit;
        ASSERT_TRUE(suite, rejected, "pattern matching stops at the per-context step limit");
    }

    {
        LuaStdLibTestContext ctx(openStringLib);
        LuaState* L = ctx.getState();
        ExecutionPolicy::Limits limits;
        limits.nativeWorkBudget = 4;
        L->getGlobalState().getExecutionPolicy().configure(limits);
        bool stopped = false;
        try {
            (void)callStringFunc(L, "upper", [&](LuaState* state) {
                state->pushString(state->getGlobalState().getStringPool().intern("native-work"));
            });
        } catch (const RuntimeError& error) {
            stopped = std::string(error.what()) == "execution native work budget exceeded";
        }
        L->getGlobalState().getExecutionPolicy().reset();
        ASSERT_TRUE(suite, stopped, "string transforms consume the independent native-work budget");
    }

    {
        LuaStdLibTestContext ctx(openStringLib);
        LuaState* L = ctx.getState();
        StringPool& pool = L->getGlobalState().getStringPool();
        const usize oldOutputLimit = L->getGlobalState().getResourcePolicy().maxOutputBytes;
        L->getGlobalState().getResourcePolicy().maxOutputBytes = 5;

        std::string gsubError;
        try {
            (void)callStringFunc(L, "gsub", [&](LuaState* state) {
                state->pushString(pool.intern("aaaa"));
                state->pushString(pool.intern("a"));
                state->pushString(pool.intern("bb"));
            });
        } catch (const RuntimeError& error) {
            gsubError = error.what();
        }

        bool formatRejected = false;
        try {
            (void)callStringFunc(L, "format", [&](LuaState* state) {
                state->pushString(pool.intern("%s%s"));
                state->pushString(pool.intern("abc"));
                state->pushString(pool.intern("abc"));
            });
        } catch (const RuntimeError& error) {
            formatRejected = std::string(error.what()).find("result exceeds resource limit") != std::string::npos;
        }

        L->getGlobalState().getResourcePolicy().maxOutputBytes = oldOutputLimit;

        ASSERT_TRUE(suite, !gsubError.empty(), "string.gsub rejects output growth beyond the configured limit");
        ASSERT_TRUE(suite, gsubError.find("result exceeds resource limit") != std::string::npos,
                    std::string("string.gsub reports its stable resource error; actual: ") + gsubError);
        ASSERT_TRUE(suite, formatRejected, "string.format checks the output limit before append growth");
    }

    const auto rejects = [](const char* functionName, const std::function<void(LuaState*)>& args) {
        LuaStdLibTestContext ctx(openStringLib);
        try {
            (void)callStringFunc(ctx.getState(), functionName, args);
        } catch (const std::exception&) {
            return true;
        }
        return false;
    };

    ASSERT_TRUE(suite,
                rejects("sub", [](LuaState* state) {
                    state->pushString(state->getGlobalState().getStringPool().intern("abc"));
                    state->pushNumber(std::numeric_limits<LuaNumber>::quiet_NaN());
                }),
                "string.sub rejects NaN indices before conversion");
    ASSERT_TRUE(suite,
                rejects("rep", [](LuaState* state) {
                    state->pushString(state->getGlobalState().getStringPool().intern("a"));
                    state->pushNumber(std::numeric_limits<LuaNumber>::infinity());
                }),
                "string.rep rejects infinite counts before conversion");
    ASSERT_TRUE(suite,
                rejects("byte", [](LuaState* state) {
                    state->pushString(state->getGlobalState().getStringPool().intern("a"));
                    state->pushNumber(std::numeric_limits<LuaNumber>::max());
                }),
                "string.byte rejects out-of-range indices before conversion");
    ASSERT_TRUE(suite,
                rejects("char", [](LuaState* state) {
                    state->pushNumber(-std::numeric_limits<LuaNumber>::infinity());
                }),
                "string.char rejects infinite values before conversion");
    ASSERT_TRUE(suite,
                rejects("format", [](LuaState* state) {
                    state->pushString(state->getGlobalState().getStringPool().intern("%d"));
                    state->pushNumber(std::numeric_limits<LuaNumber>::quiet_NaN());
                }),
                "string.format integer specifiers reject NaN before conversion");
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
    registry.registerTest(kSuiteName, "string.gmatch basic", testStringGmatchBasic);
    registry.registerTest(kSuiteName, "string.gmatch digits", testStringGmatchDigits);
    registry.registerTest(kSuiteName, "string.gmatch captures", testStringGmatchCaptures);
    registry.registerTest(kSuiteName, "string.gmatch single char", testStringGmatchSingleChar);
    registry.registerTest(kSuiteName, "string.gmatch empty string", testStringGmatchEmptyString);
    registry.registerTest(kSuiteName, "string.gmatch no match", testStringGmatchNoMatch);
    registry.registerTest(kSuiteName, "string.find pattern", testStringFindPattern);
    registry.registerTest(kSuiteName, "string.match pattern", testStringMatchPattern);
    registry.registerTest(kSuiteName, "string.gsub pattern", testStringGsubPattern);
    registry.registerTest(kSuiteName, "string.gsub table replacement", testStringGsubTableReplacement);
    registry.registerTest(kSuiteName, "string.gsub function replacement", testStringGsubFunctionReplacement);
    registry.registerTest(kSuiteName, "string binary safety", testStringBinarySafety);
    registry.registerTest(kSuiteName, "string.dump", testStringDump);
    registry.registerTest(kSuiteName, "resource and integer boundaries", testStringResourceAndIntegerBoundaries);
}
