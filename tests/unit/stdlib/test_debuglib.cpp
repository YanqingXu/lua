#include "../framework/test_framework.hpp"

#include "compiler/codegen.hpp"
#include "compiler/parser.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/string_pool.hpp"
#include "core/table.hpp"
#include "core/upvalue.hpp"
#include "lib/debuglib.hpp"
#include "lib/lib_manager.hpp"
#include "vm/lua_state.hpp"
#include "vm/vm.hpp"

#include <string>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Debug Library";

int invokeDebug(LuaState* L, const char* name, const std::function<void(LuaState*)>& pushArgs) {
    Value debugValue = L->getGlobal("debug");
    if (!debugValue.isTable()) {
        return -1;
    }

    Table* debugTable = debugValue.asTable();
    GCString* key = L->getGlobalState().getStringPool().intern(name);
    Value funcValue = debugTable->get(Value(key));
    if (!funcValue.isFunction()) {
        return -1;
    }

    L->getStack().clear();
    L->setAbsoluteTop(0);
    if (pushArgs) {
        pushArgs(L);
    }

    return funcValue.asFunction()->getCFunction()(L);
}

bool runLuaChunk(LuaState* L, const char* source, const char* chunkName) {
    try {
        Parser parser(source);
        Chunk chunk = parser.parse();
        StringPool& pool = StringPool::getInstance();
        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk, chunkName);
        if (!proto) {
            return false;
        }

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

std::string getStringField(LuaState* L, Table* table, const char* key) {
    GCString* fieldKey = L->getGlobalState().getStringPool().intern(key);
    Value field = table->get(Value(fieldKey));
    return field.isString() ? std::string(field.asString()->c_str()) : "";
}

double getNumberField(LuaState* L, Table* table, const char* key) {
    GCString* fieldKey = L->getGlobalState().getStringPool().intern(key);
    Value field = table->get(Value(fieldKey));
    return field.isNumber() ? field.asNumber() : -9999.0;
}

Value getField(LuaState* L, Table* table, const char* key) {
    GCString* fieldKey = L->getGlobalState().getStringPool().intern(key);
    return table->get(Value(fieldKey));
}

void testDebugTableRegistration(TestSuite& suite) {
    LuaStdLibTestContext ctx(openDebugLib);
    LuaState* L = ctx.getState();

    Value debugValue = L->getGlobal("debug");
    ASSERT_TRUE(suite, debugValue.isTable(), "debug table exists");
    if (!debugValue.isTable()) {
        return;
    }

    Table* debugTable = debugValue.asTable();
    auto checkFunction = [&](const char* name, const char* message) {
        Value field = debugTable->get(Value(L->getGlobalState().getStringPool().intern(name)));
        ASSERT_TRUE(suite, field.isFunction(), message);
    };

    checkFunction("getregistry", "debug.getregistry exists");
    checkFunction("getupvalue", "debug.getupvalue exists");
    checkFunction("setupvalue", "debug.setupvalue exists");
    checkFunction("getinfo", "debug.getinfo exists");
    checkFunction("traceback", "debug.traceback exists");
}

void testGetRegistry(TestSuite& suite) {
    LuaStdLibTestContext ctx(openDebugLib);
    LuaState* L = ctx.getState();

    i32 ret = invokeDebug(L, "getregistry", nullptr);
    ASSERT_EQ(suite, 1, ret, "debug.getregistry returns one value");
    ASSERT_TRUE(suite, L->top().isTable(), "debug.getregistry returns table");
    if (L->top().isTable()) {
        ASSERT_TRUE(
            suite,
            L->top().asTable() == L->getGlobalState().getRegistry(),
            "debug.getregistry returns the registry table"
        );
    }
}

static i32 dummyClosure(LuaState* L) {
    (void)L;
    return 0;
}

void testGetAndSetUpvalue(TestSuite& suite) {
    LuaStdLibTestContext ctx(openDebugLib);
    LuaState* L = ctx.getState();
    auto& gc = L->getGlobalState().getGC();
    auto& pool = L->getGlobalState().getStringPool();

    Function* closure = new Function(dummyClosure);
    gc.registerObject(closure);

    Upvalue* upvalue = Upvalue::createClosed(Value(41.0));
    gc.registerObject(upvalue);
    closure->addUpvalue(upvalue);

    i32 ret = invokeDebug(L, "getupvalue", [&](LuaState* s) {
        s->pushFunction(closure);
        s->pushNumber(1.0);
    });
    ASSERT_EQ(suite, 2, ret, "debug.getupvalue returns name and value");
    ASSERT_TRUE(suite, L->at(-2).isString(), "debug.getupvalue returns a name string");
    ASSERT_TRUE(suite, L->at(-1).isNumber(), "debug.getupvalue returns the upvalue");
    if (L->at(-1).isNumber()) {
        ASSERT_EQ(suite, 41.0, L->at(-1).asNumber(), "upvalue value matches");
    }

    ret = invokeDebug(L, "setupvalue", [&](LuaState* s) {
        s->pushFunction(closure);
        s->pushNumber(1.0);
        s->pushString(pool.intern("patched"));
    });
    ASSERT_EQ(suite, 1, ret, "debug.setupvalue returns one value");
    ASSERT_TRUE(suite, L->top().isString(), "debug.setupvalue returns name string");

    Value updated = upvalue->getValue(L->getStack());
    ASSERT_TRUE(suite, updated.isString(), "upvalue updated to string");
    if (updated.isString()) {
        ASSERT_TRUE(suite, std::string(updated.asString()->c_str()) == "patched", "setupvalue writes new value");
    }
}

void testGetInfoWithFunctionArg(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    StandardLibrary::openAll(L);

    bool ok = runLuaChunk(
        L,
        "function inspect_target(x)\n"
        "    local y = x + 1\n"
        "    return y\n"
        "end\n",
        "test_debuglib_info.lua"
    );
    ASSERT_TRUE(suite, ok, "test chunk for getinfo runs");
    if (!ok) {
        delete L;
        return;
    }

    Value funcValue = L->getGlobal("inspect_target");
    ASSERT_TRUE(suite, funcValue.isFunction(), "inspect_target exported as function");
    if (!funcValue.isFunction()) {
        delete L;
        return;
    }

    auto& pool = L->getGlobalState().getStringPool();
    i32 ret = invokeDebug(L, "getinfo", [&](LuaState* s) {
        s->pushFunction(funcValue.asFunction());
        s->pushString(pool.intern("Suf"));
    });
    ASSERT_EQ(suite, 1, ret, "debug.getinfo(function) returns table");
    ASSERT_TRUE(suite, L->top().isTable(), "debug.getinfo(function) returns table");
    if (L->top().isTable()) {
        Table* info = L->top().asTable();
        ASSERT_TRUE(
            suite,
            getStringField(L, info, "source") == "test_debuglib_info.lua",
            "getinfo source matches chunk name"
        );
        ASSERT_TRUE(suite, getStringField(L, info, "what") == "Lua", "getinfo what is Lua");
        ASSERT_EQ(suite, 0.0, getNumberField(L, info, "nups"), "getinfo reports nups");
        Value returnedFunc = getField(L, info, "func");
        ASSERT_TRUE(
            suite,
            returnedFunc.isFunction() && returnedFunc.asFunction() == funcValue.asFunction(),
            "getinfo returns original function"
        );
    }

    delete L;
}

void testGetInfoFromLuaStack(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    StandardLibrary::openAll(L);

    bool ok = runLuaChunk(
        L,
        "function capture_info()\n"
        "    local info = debug.getinfo(1, 'Slu')\n"
        "    g_source = info.source\n"
        "    g_what = info.what\n"
        "    g_currentline = info.currentline\n"
        "    g_nups = info.nups\n"
        "end\n"
        "capture_info()\n",
        "test_debuglib_stack.lua"
    );
    ASSERT_TRUE(suite, ok, "stack-level getinfo chunk runs");
    if (ok) {
        Value source = L->getGlobal("g_source");
        Value what = L->getGlobal("g_what");
        Value currentLine = L->getGlobal("g_currentline");
        Value nups = L->getGlobal("g_nups");

        ASSERT_TRUE(suite, source.isString(), "stack getinfo source exported");
        ASSERT_TRUE(suite, what.isString(), "stack getinfo what exported");
        ASSERT_TRUE(suite, currentLine.isNumber(), "stack getinfo currentline exported");
        ASSERT_TRUE(suite, nups.isNumber(), "stack getinfo nups exported");

        if (source.isString()) {
            ASSERT_TRUE(
                suite,
                std::string(source.asString()->c_str()) == "test_debuglib_stack.lua",
                "stack getinfo source matches chunk name"
            );
        }
        if (what.isString()) {
            ASSERT_TRUE(suite, std::string(what.asString()->c_str()) == "Lua", "stack getinfo what is Lua");
        }
        if (currentLine.isNumber()) {
            ASSERT_TRUE(suite, currentLine.asNumber() > 0.0, "stack getinfo currentline is populated");
        }
        if (nups.isNumber()) {
            ASSERT_EQ(suite, 0.0, nups.asNumber(), "stack getinfo nups is zero");
        }
    }

    delete L;
}

void testTracebackFromLua(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    StandardLibrary::openAll(L);

    bool ok = runLuaChunk(
        L,
        "function level2()\n"
        "    g_trace = debug.traceback('trace message')\n"
        "end\n"
        "function level1()\n"
        "    level2()\n"
        "end\n"
        "level1()\n",
        "test_debuglib_trace.lua"
    );
    ASSERT_TRUE(suite, ok, "traceback chunk runs");
    if (ok) {
        Value trace = L->getGlobal("g_trace");
        ASSERT_TRUE(suite, trace.isString(), "traceback returns string");
        if (trace.isString()) {
            std::string text = trace.asString()->c_str();
            ASSERT_TRUE(suite, text.find("trace message") != std::string::npos, "traceback keeps message");
            ASSERT_TRUE(suite, text.find("stack traceback:") != std::string::npos, "traceback includes header");
            ASSERT_TRUE(
                suite,
                text.find("test_debuglib_trace.lua") != std::string::npos,
                "traceback includes chunk name"
            );
        }
    }

    delete L;
}

} // namespace

void registerDebugLibTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "debug table", testDebugTableRegistration);
    registry.registerTest(kSuiteName, "getregistry", testGetRegistry);
    registry.registerTest(kSuiteName, "upvalue access", testGetAndSetUpvalue);
    registry.registerTest(kSuiteName, "getinfo function", testGetInfoWithFunctionArg);
    registry.registerTest(kSuiteName, "getinfo stack", testGetInfoFromLuaStack);
    registry.registerTest(kSuiteName, "traceback", testTracebackFromLua);
}
