#include "../framework/test_framework.hpp"

#include "compiler/codegen/codegen.hpp"
#include "compiler/parser/parser.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/string_pool.hpp"
#include "core/table.hpp"
#include "core/upvalue.hpp"
#include "lib/debuglib.hpp"
#include "lib/lib_manager.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm.hpp"

#include <algorithm>
#include <string>
#include <vector>

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
        auto parsed = parser.parse();
        if (!parsed) {
            throw parsed.error();
        }
        Chunk chunk = std::move(*parsed);
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
    checkFunction("getlocal", "debug.getlocal exists");
    checkFunction("setlocal", "debug.setlocal exists");
    checkFunction("getmetatable", "debug.getmetatable exists");
    checkFunction("setmetatable", "debug.setmetatable exists");
    checkFunction("getfenv", "debug.getfenv exists");
    checkFunction("setfenv", "debug.setfenv exists");
    checkFunction("traceback", "debug.traceback exists");
    checkFunction("sethook", "debug.sethook exists");
    checkFunction("gethook", "debug.gethook exists");
    checkFunction("debug", "debug.debug exists");
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

std::vector<std::string> g_hookEvents;

static i32 captureHook(LuaState* L) {
    std::string event = L->isString(1) ? L->toString(1) : "";
    if (!L->isNil(2) && L->isNumber(2)) {
        event += ":" + std::to_string(static_cast<i32>(L->toNumber(2)));
    }
    g_hookEvents.push_back(event);
    return 0;
}

bool hasHookEventPrefix(const std::string& prefix) {
    return std::any_of(
        g_hookEvents.begin(),
        g_hookEvents.end(),
        [&](const std::string& event) {
            return event.rfind(prefix, 0) == 0;
        }
    );
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

void testGetInfoNameInference(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    StandardLibrary::openAll(L);

    bool ok = runLuaChunk(
        L,
        "function named_global_target()\n"
        "    local info = debug.getinfo(1, 'n')\n"
        "    g_last_name = info.name\n"
        "    g_last_namewhat = info.namewhat\n"
        "end\n"
        "holder = {}\n"
        "function holder.field_target()\n"
        "    local info = debug.getinfo(1, 'n')\n"
        "    g_field_name = info.name\n"
        "    g_field_namewhat = info.namewhat\n"
        "end\n"
        "function holder:method_target()\n"
        "    local info = debug.getinfo(1, 'n')\n"
        "    g_method_name = info.name\n"
        "    g_method_namewhat = info.namewhat\n"
        "end\n"
        "function make_upvalue_wrapper(target)\n"
        "    local upv = target\n"
        "    return function()\n"
        "        upv()\n"
        "    end\n"
        "end\n"
        "local local_alias = named_global_target\n"
        "local_alias()\n"
        "g_local_name = g_last_name\n"
        "g_local_namewhat = g_last_namewhat\n"
        "named_global_target()\n"
        "g_global_name = g_last_name\n"
        "g_global_namewhat = g_last_namewhat\n"
        "holder.field_target()\n"
        "local wrapper = make_upvalue_wrapper(named_global_target)\n"
        "wrapper()\n"
        "g_upvalue_name = g_last_name\n"
        "g_upvalue_namewhat = g_last_namewhat\n"
        "holder:method_target()\n"
        "g_invalid_ok, g_invalid_err = pcall(function()\n"
        "    return debug.getinfo(1, 'z')\n"
        "end)\n",
        "test_debuglib_name.lua"
    );
    ASSERT_TRUE(suite, ok, "name inference chunk runs");
    if (!ok) {
        delete L;
        return;
    }

    auto assertNamedResult = [&](const char* nameKey,
                                 const char* whatKey,
                                 const char* expectedName,
                                 const char* expectedWhat,
                                 const char* label) {
        Value nameValue = L->getGlobal(nameKey);
        Value whatValue = L->getGlobal(whatKey);

        ASSERT_TRUE(suite, nameValue.isString(), std::string(label).append(" name exported").c_str());
        ASSERT_TRUE(suite, whatValue.isString(), std::string(label).append(" namewhat exported").c_str());

        if (nameValue.isString()) {
            ASSERT_TRUE(
                suite,
                std::string(nameValue.asString()->c_str()) == expectedName,
                std::string(label).append(" name matches").c_str()
            );
        }
        if (whatValue.isString()) {
            ASSERT_TRUE(
                suite,
                std::string(whatValue.asString()->c_str()) == expectedWhat,
                std::string(label).append(" namewhat matches").c_str()
            );
        }
    };

    assertNamedResult("g_local_name", "g_local_namewhat", "local_alias", "local", "local call");
    assertNamedResult("g_global_name", "g_global_namewhat", "named_global_target", "global", "global call");
    assertNamedResult("g_field_name", "g_field_namewhat", "field_target", "field", "field call");
    assertNamedResult("g_method_name", "g_method_namewhat", "method_target", "method", "method call");
    assertNamedResult("g_upvalue_name", "g_upvalue_namewhat", "upv", "upvalue", "upvalue call");

    Value invalidOk = L->getGlobal("g_invalid_ok");
    Value invalidErr = L->getGlobal("g_invalid_err");
    ASSERT_TRUE(suite, invalidOk.isBoolean(), "invalid option status exported");
    ASSERT_TRUE(suite, invalidErr.isString(), "invalid option message exported");
    if (invalidOk.isBoolean()) {
        ASSERT_TRUE(suite, !invalidOk.asBoolean(), "invalid option call fails");
    }
    if (invalidErr.isString()) {
        ASSERT_TRUE(
            suite,
            std::string(invalidErr.asString()->c_str()).find("invalid option") != std::string::npos,
            "invalid option message mentions invalid option"
        );
    }

    Value funcValue = L->getGlobal("named_global_target");
    ASSERT_TRUE(suite, funcValue.isFunction(), "named_global_target is exported");
    if (funcValue.isFunction()) {
        auto& pool = L->getGlobalState().getStringPool();
        i32 ret = invokeDebug(L, "getinfo", [&](LuaState* s) {
            s->pushFunction(funcValue.asFunction());
            s->pushString(pool.intern("n"));
        });
        ASSERT_EQ(suite, 1, ret, "debug.getinfo(function, 'n') returns table");
        ASSERT_TRUE(suite, L->top().isTable(), "function getinfo('n') returns table");
        if (L->top().isTable()) {
            Table* info = L->top().asTable();
            Value name = getField(L, info, "name");
            Value namewhat = getField(L, info, "namewhat");
            ASSERT_TRUE(suite, name.isNil(), "function getinfo('n') leaves name unset");
            ASSERT_TRUE(suite, namewhat.isString(), "function getinfo('n') exports namewhat");
            if (namewhat.isString()) {
                ASSERT_TRUE(
                    suite,
                    std::string(namewhat.asString()->c_str()).empty(),
                    "function getinfo('n') uses empty namewhat"
                );
            }
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
            ASSERT_TRUE(
                suite,
                text.find(": in function <test_debuglib_trace.lua:1>") != std::string::npos,
                "traceback includes Lua function frame"
            );
        }
    }

    delete L;
}

void testGetLocalAndSetLocal(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    StandardLibrary::openAll(L);

    bool ok = runLuaChunk(
        L,
        "function local_target(a, b)\n"
        "    local sum = a + b\n"
        "    g_local_name1, g_local_value1 = debug.getlocal(1, 1)\n"
        "    g_local_name2, g_local_value2 = debug.getlocal(1, 2)\n"
        "    g_local_name3, g_local_value3 = debug.getlocal(1, 3)\n"
        "    g_setlocal_name = debug.setlocal(1, 3, 99)\n"
        "    return sum\n"
        "end\n"
        "g_local_result = local_target(7, 8)\n",
        "test_debuglib_local.lua"
    );
    ASSERT_TRUE(suite, ok, "local get/set chunk runs");
    if (!ok) {
        delete L;
        return;
    }

    Value localName1 = L->getGlobal("g_local_name1");
    Value localValue1 = L->getGlobal("g_local_value1");
    Value localName2 = L->getGlobal("g_local_name2");
    Value localValue2 = L->getGlobal("g_local_value2");
    Value localName3 = L->getGlobal("g_local_name3");
    Value localValue3 = L->getGlobal("g_local_value3");
    Value setlocalName = L->getGlobal("g_setlocal_name");
    Value result = L->getGlobal("g_local_result");

    ASSERT_TRUE(suite, localName1.isString(), "getlocal name #1 exported");
    ASSERT_TRUE(suite, localValue1.isNumber(), "getlocal value #1 exported");
    ASSERT_TRUE(suite, localName2.isString(), "getlocal name #2 exported");
    ASSERT_TRUE(suite, localValue2.isNumber(), "getlocal value #2 exported");
    ASSERT_TRUE(suite, localName3.isString(), "getlocal name #3 exported");
    ASSERT_TRUE(suite, localValue3.isNumber(), "getlocal value #3 exported");
    ASSERT_TRUE(suite, setlocalName.isString(), "setlocal returns name");
    ASSERT_TRUE(suite, result.isNumber(), "setlocal affected return value");

    if (localName1.isString()) {
        ASSERT_TRUE(suite, std::string(localName1.asString()->c_str()) == "a", "local #1 is parameter a");
    }
    if (localValue1.isNumber()) {
        ASSERT_EQ(suite, 7.0, localValue1.asNumber(), "local #1 value matches");
    }
    if (localName2.isString()) {
        ASSERT_TRUE(suite, std::string(localName2.asString()->c_str()) == "b", "local #2 is parameter b");
    }
    if (localValue2.isNumber()) {
        ASSERT_EQ(suite, 8.0, localValue2.asNumber(), "local #2 value matches");
    }
    if (localName3.isString()) {
        ASSERT_TRUE(suite, std::string(localName3.asString()->c_str()) == "sum", "local #3 is local sum");
    }
    if (localValue3.isNumber()) {
        ASSERT_EQ(suite, 15.0, localValue3.asNumber(), "local #3 value matches before mutation");
    }
    if (setlocalName.isString()) {
        ASSERT_TRUE(suite, std::string(setlocalName.asString()->c_str()) == "sum", "setlocal returns mutated local name");
    }
    if (result.isNumber()) {
        ASSERT_EQ(suite, 99.0, result.asNumber(), "setlocal updates the live local slot");
    }

    Value funcValue = L->getGlobal("local_target");
    ASSERT_TRUE(suite, funcValue.isFunction(), "local_target is exported");
    if (funcValue.isFunction()) {
        auto& pool = L->getGlobalState().getStringPool();
        i32 ret = invokeDebug(L, "getlocal", [&](LuaState* s) {
            s->pushFunction(funcValue.asFunction());
            s->pushNumber(1.0);
        });
        ASSERT_EQ(suite, 1, ret, "debug.getlocal(function) returns one value");
        ASSERT_TRUE(suite, L->top().isString(), "debug.getlocal(function) returns a name");
        if (L->top().isString()) {
            ASSERT_TRUE(suite, std::string(L->top().asString()->c_str()) == "a", "function local metadata returns first parameter");
        }

        ret = invokeDebug(L, "getlocal", [&](LuaState* s) {
            s->pushFunction(funcValue.asFunction());
            s->pushNumber(4.0);
        });
        ASSERT_EQ(suite, 1, ret, "debug.getlocal(function, missing) returns one value");
        ASSERT_TRUE(suite, L->top().isNil(), "missing function local returns nil");

        ret = invokeDebug(L, "getinfo", [&](LuaState* s) {
            s->pushFunction(funcValue.asFunction());
            s->pushString(pool.intern("L"));
        });
        ASSERT_EQ(suite, 1, ret, "debug.getinfo(function, 'L') returns table");
        ASSERT_TRUE(suite, L->top().isTable(), "getinfo('L') returns table");
        if (L->top().isTable()) {
            Value activeLines = getField(L, L->top().asTable(), "activelines");
            ASSERT_TRUE(suite, activeLines.isTable(), "activelines table is populated");
        }
    }

    delete L;
}

void testDebugMetatableWrappers(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    StandardLibrary::openAll(L);

    bool ok = runLuaChunk(L, R"(
        local t = {}
        local mt = {__metatable = "locked", marker = 11}
        setmetatable(t, mt)

        local raw = debug.getmetatable(t)
        g_raw_mt = raw == mt and raw.marker == 11

        local newmt = {marker = 22}
        local returned = debug.setmetatable(t, newmt)
        g_set_return = returned == t
        g_new_raw_mt = debug.getmetatable(t) == newmt and getmetatable(t).marker == 22

        debug.setmetatable(t, nil)
        g_cleared_mt = debug.getmetatable(t) == nil

        local unsupported_ok = pcall(function() debug.setmetatable(1, {}) end)
        g_rejects_unsupported = not unsupported_ok
    )", "test_debuglib_metatable.lua");
    ASSERT_TRUE(suite, ok, "debug metatable chunk runs");

    auto assertGlobalTrue = [&](const char* name, const char* message) {
        Value value = L->getGlobal(name);
        ASSERT_TRUE(suite, value.isBoolean() && value.asBoolean(), message);
    };

    assertGlobalTrue("g_raw_mt", "debug.getmetatable bypasses __metatable protection");
    assertGlobalTrue("g_set_return", "debug.setmetatable returns original object");
    assertGlobalTrue("g_new_raw_mt", "debug.setmetatable replaces protected metatable");
    assertGlobalTrue("g_cleared_mt", "debug.setmetatable accepts nil");
    assertGlobalTrue("g_rejects_unsupported", "debug.setmetatable rejects unsupported primitive metatables");

    delete L;
}

void testDebugFenvWrappers(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    StandardLibrary::openAll(L);

    bool ok = runLuaChunk(L, R"(
        local env = {answer = 123}
        local function f()
            return answer
        end

        local returned = debug.setfenv(f, env)
        g_setfenv_return = returned == f
        g_getfenv_matches = debug.getfenv(f) == env
        g_env_value = f()

        local cfunc_ok = pcall(function() debug.setfenv(print, {}) end)
        g_rejects_cfunc = not cfunc_ok

        local co = coroutine.create(function()
            coroutine.yield(getfenv(0))
            return loadstring("return answer")()
        end)
        local threadReturned = debug.setfenv(co, env)
        g_thread_setfenv_return = threadReturned == co
        g_thread_getfenv_matches = debug.getfenv(co) == env
        local ok1, yieldedEnv = coroutine.resume(co)
        local ok2, loadedAnswer = coroutine.resume(co)
        g_thread_env_yield = ok1 and yieldedEnv == env
        g_thread_env_loadstring = ok2 and loadedAnswer == 123
    )", "test_debuglib_fenv.lua");
    ASSERT_TRUE(suite, ok, "debug fenv chunk runs");

    Value setReturn = L->getGlobal("g_setfenv_return");
    Value getEnvMatches = L->getGlobal("g_getfenv_matches");
    Value envValue = L->getGlobal("g_env_value");
    Value rejectsCFunc = L->getGlobal("g_rejects_cfunc");
    Value threadSetReturn = L->getGlobal("g_thread_setfenv_return");
    Value threadGetEnvMatches = L->getGlobal("g_thread_getfenv_matches");
    Value threadEnvYield = L->getGlobal("g_thread_env_yield");
    Value threadEnvLoadstring = L->getGlobal("g_thread_env_loadstring");

    ASSERT_TRUE(suite, setReturn.isBoolean() && setReturn.asBoolean(), "debug.setfenv returns function");
    ASSERT_TRUE(suite, getEnvMatches.isBoolean() && getEnvMatches.asBoolean(), "debug.getfenv returns assigned env");
    ASSERT_TRUE(suite, envValue.isNumber(), "function reads from assigned env");
    if (envValue.isNumber()) {
        ASSERT_EQ(suite, 123.0, envValue.asNumber(), "function env lookup returns expected value");
    }
    ASSERT_TRUE(suite, rejectsCFunc.isBoolean() && rejectsCFunc.asBoolean(), "debug.setfenv rejects C functions");
    ASSERT_TRUE(suite, threadSetReturn.isBoolean() && threadSetReturn.asBoolean(),
                "debug.setfenv returns thread objects");
    ASSERT_TRUE(suite, threadGetEnvMatches.isBoolean() && threadGetEnvMatches.asBoolean(),
                "debug.getfenv reads thread env");
    ASSERT_TRUE(suite, threadEnvYield.isBoolean() && threadEnvYield.asBoolean(),
                "thread getfenv(0) sees debug.setfenv env");
    ASSERT_TRUE(suite, threadEnvLoadstring.isBoolean() && threadEnvLoadstring.asBoolean(),
                "thread loadstring uses debug.setfenv env");

    delete L;
}

void testHookLifecycle(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    StandardLibrary::openAll(L);

    g_hookEvents.clear();
    Function* hookFunc = new Function(captureHook);
    L->getGlobalState().getGC().registerObject(hookFunc);

    auto& pool = L->getGlobalState().getStringPool();
    i32 ret = invokeDebug(L, "sethook", [&](LuaState* s) {
        s->pushFunction(hookFunc);
        s->pushString(pool.intern("crl"));
        s->pushNumber(2.0);
    });
    ASSERT_EQ(suite, 0, ret, "debug.sethook returns no values");

    bool ok = runLuaChunk(
        L,
        "function hook_target()\n"
        "    local x = 1\n"
        "    x = x + 1\n"
        "    return x\n"
        "end\n"
        "g_hook_result = hook_target()\n",
        "test_debuglib_hook.lua"
    );
    ASSERT_TRUE(suite, ok, "hook target chunk runs");
    if (ok) {
        Value hookResult = L->getGlobal("g_hook_result");
        ASSERT_TRUE(suite, hookResult.isNumber(), "hook target returns number");
        if (hookResult.isNumber()) {
            ASSERT_EQ(suite, 2.0, hookResult.asNumber(), "hook target returns expected value");
        }
    }

    ASSERT_TRUE(suite, !g_hookEvents.empty(), "hook captured events");
    ASSERT_TRUE(suite, hasHookEventPrefix("call"), "hook captured a call event");
    ASSERT_TRUE(suite, hasHookEventPrefix("return"), "hook captured a return event");
    ASSERT_TRUE(suite, hasHookEventPrefix("line"), "hook captured a line event");
    ASSERT_TRUE(suite, hasHookEventPrefix("count"), "hook captured a count event");

    ret = invokeDebug(L, "gethook", nullptr);
    ASSERT_EQ(suite, 3, ret, "debug.gethook returns hook triple");
    ASSERT_TRUE(suite, L->at(-3).isFunction(), "gethook returns hook function");
    ASSERT_TRUE(suite, L->at(-2).isString(), "gethook returns hook mask");
    ASSERT_TRUE(suite, L->at(-1).isNumber(), "gethook returns hook count");
    if (L->at(-3).isFunction()) {
        ASSERT_TRUE(suite, L->at(-3).asFunction() == hookFunc, "gethook returns installed function");
    }
    if (L->at(-2).isString()) {
        ASSERT_TRUE(suite, std::string(L->at(-2).asString()->c_str()) == "crl", "gethook returns installed mask");
    }
    if (L->at(-1).isNumber()) {
        ASSERT_EQ(suite, 2.0, L->at(-1).asNumber(), "gethook returns installed count");
    }

    ret = invokeDebug(L, "sethook", [&](LuaState* s) {
        s->pushNil();
    });
    ASSERT_EQ(suite, 0, ret, "debug.sethook(nil) clears hook");

    ret = invokeDebug(L, "gethook", nullptr);
    ASSERT_EQ(suite, 3, ret, "debug.gethook still returns triple after clear");
    ASSERT_TRUE(suite, L->at(-3).isNil(), "cleared hook function is nil");
    ASSERT_TRUE(suite, L->at(-2).isString(), "cleared hook mask returns string");
    ASSERT_TRUE(suite, L->at(-1).isNumber(), "cleared hook count returns number");
    if (L->at(-2).isString()) {
        ASSERT_TRUE(suite, std::string(L->at(-2).asString()->c_str()).empty(), "cleared hook mask is empty");
    }
    if (L->at(-1).isNumber()) {
        ASSERT_EQ(suite, 0.0, L->at(-1).asNumber(), "cleared hook count is zero");
    }

    delete L;
}

void testThreadHookAndTraceback(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    StandardLibrary::openAll(L);

    bool ok = runLuaChunk(
        L,
        "function coroutine_target()\n"
        "    local a = 10\n"
        "    a = a + 1\n"
        "    coroutine.yield(a)\n"
        "    a = a + 1\n"
        "    return a\n"
        "end\n"
        "co = coroutine.create(coroutine_target)\n",
        "test_debuglib_thread.lua"
    );
    ASSERT_TRUE(suite, ok, "coroutine setup chunk runs");
    if (!ok) {
        delete L;
        return;
    }

    Value threadValue = L->getGlobal("co");
    ASSERT_TRUE(suite, threadValue.isThread(), "co global is a coroutine");
    if (!threadValue.isThread()) {
        delete L;
        return;
    }

    g_hookEvents.clear();
    Function* hookFunc = new Function(captureHook);
    L->getGlobalState().getGC().registerObject(hookFunc);

    auto& pool = L->getGlobalState().getStringPool();
    i32 ret = invokeDebug(L, "sethook", [&](LuaState* s) {
        s->pushValue(threadValue);
        s->pushFunction(hookFunc);
        s->pushString(pool.intern("cr"));
        s->pushNumber(1.0);
    });
    ASSERT_EQ(suite, 0, ret, "debug.sethook(thread, ...) returns no values");

    ret = invokeDebug(L, "gethook", [&](LuaState* s) {
        s->pushValue(threadValue);
    });
    ASSERT_EQ(suite, 3, ret, "debug.gethook(thread) returns hook triple");
    ASSERT_TRUE(suite, L->at(-3).isFunction(), "thread gethook returns function");
    ASSERT_TRUE(suite, L->at(-2).isString(), "thread gethook returns mask");
    ASSERT_TRUE(suite, L->at(-1).isNumber(), "thread gethook returns count");
    if (L->at(-2).isString()) {
        ASSERT_TRUE(suite, std::string(L->at(-2).asString()->c_str()) == "cr", "thread gethook mask matches");
    }
    if (L->at(-1).isNumber()) {
        ASSERT_EQ(suite, 1.0, L->at(-1).asNumber(), "thread gethook count matches");
    }

    ok = runLuaChunk(
        L,
        "g_resume_ok1, g_resume_value1 = coroutine.resume(co)\n"
        "g_thread_trace = debug.traceback(co)\n"
        "g_resume_ok2, g_resume_value2 = coroutine.resume(co)\n",
        "test_debuglib_thread_resume.lua"
    );
    ASSERT_TRUE(suite, ok, "coroutine resume chunk runs");
    if (ok) {
        Value resumeOk1 = L->getGlobal("g_resume_ok1");
        Value resumeValue1 = L->getGlobal("g_resume_value1");
        Value resumeOk2 = L->getGlobal("g_resume_ok2");
        Value resumeValue2 = L->getGlobal("g_resume_value2");
        Value threadTrace = L->getGlobal("g_thread_trace");

        ASSERT_TRUE(suite, resumeOk1.isBoolean() && resumeOk1.asBoolean(), "first coroutine resume succeeds");
        ASSERT_TRUE(suite, resumeValue1.isNumber(), "first coroutine resume returns value");
        ASSERT_TRUE(suite, resumeOk2.isBoolean() && resumeOk2.asBoolean(), "second coroutine resume succeeds");
        ASSERT_TRUE(suite, resumeValue2.isNumber(), "second coroutine resume returns value");
        ASSERT_TRUE(suite, threadTrace.isString(), "thread traceback returns string");

        if (resumeValue1.isNumber()) {
            ASSERT_EQ(suite, 11.0, resumeValue1.asNumber(), "first coroutine resume yielded expected value");
        }
        if (resumeValue2.isNumber()) {
            ASSERT_EQ(suite, 12.0, resumeValue2.asNumber(), "second coroutine resume returned expected value");
        }
        if (threadTrace.isString()) {
            std::string text = threadTrace.asString()->c_str();
            ASSERT_TRUE(
                suite,
                text.find("test_debuglib_thread.lua") != std::string::npos,
                "thread traceback includes coroutine chunk name"
            );
        }
    }

    ASSERT_TRUE(suite, hasHookEventPrefix("call"), "thread hook captured call event");
    ASSERT_TRUE(suite, hasHookEventPrefix("return"), "thread hook captured return event");

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
    registry.registerTest(kSuiteName, "getinfo names", testGetInfoNameInference);
    registry.registerTest(kSuiteName, "traceback", testTracebackFromLua);
    registry.registerTest(kSuiteName, "local access", testGetLocalAndSetLocal);
    registry.registerTest(kSuiteName, "metatable wrappers", testDebugMetatableWrappers);
    registry.registerTest(kSuiteName, "fenv wrappers", testDebugFenvWrappers);
    registry.registerTest(kSuiteName, "hook lifecycle", testHookLifecycle);
    registry.registerTest(kSuiteName, "thread hook", testThreadHookAndTraceback);
}
