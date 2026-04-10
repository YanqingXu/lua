/**
 * @file debuglib.cpp
 * @brief Lua debug library implementation
 *
 * Uses the modern C++ fluent registration API (option 2).
 *
 * @author Lua C++ Project
 * @date 2026-04-10
 */

#include "lib/debuglib.hpp"

#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/upvalue.hpp"
#include "lib/lib_manager.hpp"
#include "lib/lib_registry.hpp"
#include "vm/call_info.hpp"

#include <algorithm>
#include <sstream>

namespace Lua {

namespace {

// =====================================================================
// Internal Helpers
// =====================================================================

constexpr usize SHORT_SRC_LIMIT = 60;

GCString* internString(LuaState* L, StrView value) {
    return L->getGlobalState().getStringPool().intern(value);
}

GCString* internString(LuaState* L, const char* value) {
    return internString(L, value ? StrView(value) : StrView(""));
}

GCString* internEmptyString(LuaState* L) {
    return internString(L, "");
}

void setField(Table* table, LuaState* L, const char* key, const Value& value) {
    table->set(Value(internString(L, key)), value);
}

void setStringField(Table* table, LuaState* L, const char* key, StrView value) {
    setField(table, L, key, Value(internString(L, value)));
}

void setNumberField(Table* table, LuaState* L, const char* key, i32 value) {
    setField(table, L, key, Value(static_cast<LuaNumber>(value)));
}

Str makeShortSource(StrView source) {
    if (source.empty()) {
        return "?";
    }

    if (source.size() <= SHORT_SRC_LIMIT) {
        return Str(source);
    }

    constexpr usize prefix = 3;
    return "..." + Str(source.substr(source.size() - (SHORT_SRC_LIMIT - prefix)));
}

Function* functionFromCallInfo(LuaState* L, const CallInfo& ci) {
    Stack& stack = L->getStack();
    if (ci.func >= stack.size()) {
        return nullptr;
    }

    Value& funcValue = stack[ci.func];
    if (!funcValue.isFunction()) {
        return nullptr;
    }

    return funcValue.asFunction();
}

struct DebugFrameRef {
    Function* func = nullptr;
    const CallInfo* ci = nullptr;
};

bool resolveStackLevel(LuaState* L, i32 level, DebugFrameRef& outFrame) {
    if (level < 0) {
        return false;
    }

    usize currentIndex = L->getCurrentCI();
    if (static_cast<usize>(level) > currentIndex) {
        return false;
    }

    usize targetIndex = currentIndex - static_cast<usize>(level);
    const CallInfo& ci = L->getCallStack()[targetIndex];
    Function* func = functionFromCallInfo(L, ci);
    if (!func) {
        return false;
    }

    outFrame.func = func;
    outFrame.ci = &ci;
    return true;
}

i32 currentLineForFrame(Function* func, const CallInfo* ci) {
    if (!func || !ci || func->isCFunction()) {
        return -1;
    }

    Proto* proto = func->getProto();
    if (!proto || !ci->savedpc) {
        return -1;
    }

    const Vec<Instruction>& code = proto->getCode();
    if (code.empty()) {
        return -1;
    }

    i32 pc = static_cast<i32>(ci->savedpc - code.data()) - 1;
    if (pc < 0) {
        pc = 0;
    }
    if (pc >= static_cast<i32>(code.size())) {
        pc = static_cast<i32>(code.size()) - 1;
    }

    return proto->getLine(static_cast<usize>(pc));
}

GCString* upvalueNameOrEmpty(LuaState* L, Function* func, usize index) {
    if (!func) {
        return nullptr;
    }

    if (func->isLuaFunction()) {
        Proto* proto = func->getProto();
        if (proto != nullptr) {
            if (GCString* name = proto->getUpvalueName(index)) {
                return name;
            }
        }
    }

    return internEmptyString(L);
}

Table* createInfoTable(LuaState* L) {
    Table* table = new Table();
    L->getGlobalState().getGC().registerObject(table);
    return table;
}

void populateInfoS(Table* info, LuaState* L, Function* func, const CallInfo* ci) {
    if (!func) {
        return;
    }

    if (func->isCFunction()) {
        setStringField(info, L, "source", "=[C]");
        setStringField(info, L, "short_src", "[C]");
        setStringField(info, L, "what", "C");
        setNumberField(info, L, "linedefined", -1);
        setNumberField(info, L, "lastlinedefined", -1);
        setNumberField(info, L, "currentline", -1);
        return;
    }

    Proto* proto = func->getProto();
    StrView source = proto && proto->getSource() ? proto->getSource()->view() : StrView("=?");

    setStringField(info, L, "source", source);
    setStringField(info, L, "short_src", makeShortSource(source));
    setStringField(
        info,
        L,
        "what",
        (proto && proto->getLineDefined() == 0) ? StrView("main") : StrView("Lua")
    );
    setNumberField(info, L, "linedefined", proto ? proto->getLineDefined() : -1);
    setNumberField(info, L, "lastlinedefined", proto ? proto->getLastLineDefined() : -1);
    setNumberField(info, L, "currentline", currentLineForFrame(func, ci));
}

void populateInfoU(Table* info, LuaState* L, Function* func) {
    if (!func) {
        return;
    }

    setNumberField(info, L, "nups", static_cast<i32>(func->getNumUpvalues()));
}

Str describeFunction(Function* func) {
    if (!func) {
        return "?";
    }

    if (func->isCFunction()) {
        return "C function";
    }

    Proto* proto = func->getProto();
    if (!proto) {
        return "Lua function";
    }

    Str source = proto->getSource() ? proto->getSource()->c_str() : "?";
    if (proto->getLineDefined() <= 0) {
        return "main chunk";
    }

    std::ostringstream oss;
    oss << "function <" << source << ":" << proto->getLineDefined() << ">";
    return oss.str();
}

Str formatFrameLine(Function* func, const CallInfo* ci) {
    if (!func) {
        return "\t?";
    }

    if (func->isCFunction()) {
        return "\t[C]: in " + describeFunction(func);
    }

    Proto* proto = func->getProto();
    Str source = proto && proto->getSource() ? proto->getSource()->c_str() : "?";
    i32 line = currentLineForFrame(func, ci);
    if (line < 0 && proto != nullptr) {
        line = proto->getLineDefined();
    }

    std::ostringstream oss;
    if (line > 0) {
        oss << "\t" << source << ":" << line << ": in " << describeFunction(func);
    } else {
        oss << "\t" << source << ": in " << describeFunction(func);
    }
    return oss.str();
}

Function* checkFunctionArg(LuaState* L, i32 idx, const char* message) {
    if (!L->isFunction(idx)) {
        L->error(message);
    }
    return L->at(idx).asFunction();
}

i32 checkPositiveIndex(LuaState* L, i32 idx, const char* message) {
    if (!L->isNumber(idx)) {
        L->error(message);
    }

    i32 value = static_cast<i32>(L->toNumber(idx));
    if (value <= 0) {
        L->error(message);
    }
    return value;
}

} // namespace

// =====================================================================
// debug.getregistry() - Get the registry table
// =====================================================================

i32 luaDebug_getregistry(LuaState* L) {
    L->pushTable(L->getGlobalState().getRegistry());
    return 1;
}

// =====================================================================
// debug.getupvalue(func, up) - Get a function upvalue
// =====================================================================

i32 luaDebug_getupvalue(LuaState* L) {
    Function* func = checkFunctionArg(
        L,
        1,
        "bad argument #1 to 'getupvalue' (function expected)"
    );
    i32 upIndex = checkPositiveIndex(
        L,
        2,
        "bad argument #2 to 'getupvalue' (positive index expected)"
    );

    Upvalue* upvalue = func->getUpvalue(static_cast<usize>(upIndex - 1));
    if (!upvalue) {
        L->pushNil();
        return 1;
    }

    GCString* name = upvalueNameOrEmpty(L, func, static_cast<usize>(upIndex - 1));
    L->pushString(name);
    L->pushValue(upvalue->getValue(L->getStack()));
    return 2;
}

// =====================================================================
// debug.setupvalue(func, up, value) - Set a function upvalue
// =====================================================================

i32 luaDebug_setupvalue(LuaState* L) {
    Function* func = checkFunctionArg(
        L,
        1,
        "bad argument #1 to 'setupvalue' (function expected)"
    );
    i32 upIndex = checkPositiveIndex(
        L,
        2,
        "bad argument #2 to 'setupvalue' (positive index expected)"
    );

    Upvalue* upvalue = func->getUpvalue(static_cast<usize>(upIndex - 1));
    if (!upvalue) {
        L->pushNil();
        return 1;
    }

    upvalue->setValue(L->getStack(), L->at(3));
    L->pushString(upvalueNameOrEmpty(L, func, static_cast<usize>(upIndex - 1)));
    return 1;
}

// =====================================================================
// debug.getinfo(thread|func|level [, what]) - Get debug information
// =====================================================================

i32 luaDebug_getinfo(LuaState* L) {
    i32 nargs = L->getTop();
    if (nargs < 1) {
        L->error("bad argument #1 to 'getinfo' (function or level expected)");
    }

    StrView options = "flnSu";
    if (nargs >= 2) {
        if (!L->isString(2)) {
            L->error("bad argument #2 to 'getinfo' (string expected)");
        }
        options = L->toString(2);
    }

    DebugFrameRef frame;
    Function* func = nullptr;

    if (L->isFunction(1)) {
        func = L->at(1).asFunction();
    } else if (L->isNumber(1)) {
        i32 level = static_cast<i32>(L->toNumber(1));
        if (!resolveStackLevel(L, level, frame)) {
            L->pushNil();
            return 1;
        }
        func = frame.func;
    } else {
        L->error("bad argument #1 to 'getinfo' (function or level expected)");
    }

    Table* info = createInfoTable(L);
    for (char option : options) {
        switch (option) {
        case 'S':
            populateInfoS(info, L, func, frame.ci);
            break;
        case 'u':
            populateInfoU(info, L, func);
            break;
        case 'f':
            setField(info, L, "func", Value(func));
            break;
        case 'l':
            setNumberField(info, L, "currentline", currentLineForFrame(func, frame.ci));
            break;
        case 'n':
            setStringField(info, L, "namewhat", "");
            break;
        default:
            break;
        }
    }

    L->pushTable(info);
    return 1;
}

// =====================================================================
// debug.traceback([message [, level]]) - Build a traceback string
// =====================================================================

i32 luaDebug_traceback(LuaState* L) {
    const char* message = nullptr;
    i32 level = 1;

    if (L->getTop() >= 1 && !L->isNil(1)) {
        if (!L->isString(1)) {
            L->error("bad argument #1 to 'traceback' (string expected)");
        }
        message = L->toString(1);
    }

    if (L->getTop() >= 2) {
        if (!L->isNumber(2)) {
            L->error("bad argument #2 to 'traceback' (number expected)");
        }
        level = std::max(0, static_cast<i32>(L->toNumber(2)));
    }

    std::ostringstream oss;
    if (message && *message != '\0') {
        oss << message << "\n";
    }
    oss << "stack traceback:";

    i32 startIndex = static_cast<i32>(L->getCurrentCI()) - level;
    for (i32 index = startIndex; index >= 0; --index) {
        const CallInfo& ci = L->getCallStack()[static_cast<usize>(index)];
        Function* func = functionFromCallInfo(L, ci);
        if (!func) {
            continue;
        }

        oss << "\n" << formatFrameLine(func, &ci);
    }

    L->pushString(internString(L, oss.str()));
    return 1;
}

// =====================================================================
// Debug library registration entry
// =====================================================================

void DebugLibModule::registerFunctions(LuaState* L) {
    if (!L) {
        return;
    }

    Table* debugTable = FunctionRegistrar::createLibTable(L, "debug");
    if (!debugTable) {
        L->error("Failed to create debug library table");
        return;
    }

    FunctionRegistrar(L)
        .addGlobal("getregistry", luaDebug_getregistry)
        .addGlobal("getupvalue", luaDebug_getupvalue)
        .addGlobal("setupvalue", luaDebug_setupvalue)
        .addGlobal("getinfo", luaDebug_getinfo)
        .addGlobal("traceback", luaDebug_traceback)
        .commitToTable(debugTable);
}

void openDebugLib(LuaState* L) {
    if (!L) {
        return;
    }

    DebugLibModule module;
    StandardLibrary::openModule(L, module);
}

} // namespace Lua
