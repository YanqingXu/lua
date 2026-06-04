#include "lua.h"
#include "lauxlib.h"

#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/value.hpp"
#include "lib/lib_manager.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm.hpp"
#include "vm/vm_internal.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

static Lua::LuaState* fromC(lua_State* L) {
    return reinterpret_cast<Lua::LuaState*>(L);
}

static lua_State* toC(Lua::LuaState* L) {
    return reinterpret_cast<lua_State*>(L);
}

static int absIndex(Lua::LuaState* L, int idx) {
    if (idx > 0) {
        return idx;
    }
    return L->getTop() + idx + 1;
}

extern "C" {

lua_State* lua_newstate(lua_Alloc, void*) {
    return toC(Lua::LuaState::newState());
}

lua_State* lua_open(void) {
    return lua_newstate(nullptr, nullptr);
}

void lua_close(lua_State* L) {
    delete fromC(L);
}

int lua_gettop(lua_State* L) {
    return fromC(L)->getTop();
}

void lua_settop(lua_State* L, int idx) {
    fromC(L)->setTop(idx);
}

void lua_pushvalue(lua_State* L, int idx) {
    fromC(L)->pushValue(idx);
}

void lua_remove(lua_State* L, int idx) {
    Lua::LuaState* state = fromC(L);
    const int top = state->getTop();
    const int pos = absIndex(state, idx);
    if (pos < 1 || pos > top) {
        return;
    }

    Lua::Vec<Lua::Value> values;
    values.reserve(static_cast<Lua::usize>(top - 1));
    for (int i = 1; i <= top; ++i) {
        if (i != pos) {
            values.push_back(state->at(i));
        }
    }
    state->setTop(0);
    for (const Lua::Value& value : values) {
        state->pushValue(value);
    }
}

void lua_insert(lua_State* L, int idx) {
    fromC(L)->insert(idx);
}

void lua_replace(lua_State* L, int idx) {
    fromC(L)->replace(idx);
}

int lua_type(lua_State* L, int idx) {
    return fromC(L)->type(idx);
}

const char* lua_typename(lua_State* L, int tp) {
    return fromC(L)->typeName(tp);
}

int lua_isnumber(lua_State* L, int idx) {
    return fromC(L)->isNumber(idx) ? 1 : 0;
}

int lua_isstring(lua_State* L, int idx) {
    Lua::LuaState* state = fromC(L);
    const int t = state->type(idx);
    return (t == LUA_TSTRING || t == LUA_TNUMBER) ? 1 : 0;
}

int lua_iscfunction(lua_State* L, int idx) {
    try {
        const Lua::Value& value = fromC(L)->at(idx);
        return value.isFunction() && value.asFunction()->isCFunction() ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

int lua_isuserdata(lua_State* L, int idx) {
    Lua::LuaState* state = fromC(L);
    const int t = state->type(idx);
    return (t == LUA_TUSERDATA || t == LUA_TLIGHTUSERDATA) ? 1 : 0;
}

int lua_toboolean(lua_State* L, int idx) {
    return fromC(L)->toBoolean(idx) ? 1 : 0;
}

lua_Number lua_tonumber(lua_State* L, int idx) {
    return fromC(L)->toNumber(idx);
}

const char* lua_tolstring(lua_State* L, int idx, size_t* len) {
    Lua::LuaState* state = fromC(L);
    const char* text = state->toString(idx);
    if (text == nullptr) {
        if (len) {
            *len = 0;
        }
        return nullptr;
    }
    if (len) {
        const Lua::Value& value = state->at(idx);
        *len = value.isString() ? value.asString()->getLength() : std::strlen(text);
    }
    return text;
}

void lua_pushnil(lua_State* L) {
    fromC(L)->pushNil();
}

void lua_pushnumber(lua_State* L, lua_Number n) {
    fromC(L)->pushNumber(n);
}

void lua_pushinteger(lua_State* L, lua_Integer n) {
    fromC(L)->pushNumber(static_cast<Lua::LuaNumber>(n));
}

void lua_pushboolean(lua_State* L, int b) {
    fromC(L)->pushBoolean(b != 0);
}

void lua_pushlstring(lua_State* L, const char* s, size_t len) {
    Lua::LuaState* state = fromC(L);
    state->pushString(state->getGlobalState().getStringPool().intern(s ? s : "", len));
}

void lua_pushstring(lua_State* L, const char* s) {
    lua_pushlstring(L, s ? s : "", s ? std::strlen(s) : 0);
}

void lua_pushcclosure(lua_State* L, lua_CFunction fn, int) {
    Lua::LuaState* state = fromC(L);
    auto internal = reinterpret_cast<Lua::CFunction>(fn);
    Lua::Function* closure = state->getGlobalState().getGC().create<Lua::Function>(internal);
    closure->setEnv(state->getGlobalTable());
    state->pushFunction(closure);
}

void lua_createtable(lua_State* L, int, int) {
    Lua::LuaState* state = fromC(L);
    state->pushTable(state->getGlobalState().getGC().create<Lua::Table>());
}

void lua_gettable(lua_State* L, int idx) {
    Lua::LuaState* state = fromC(L);
    const int tableIdx = absIndex(state, idx);
    Lua::Value key = state->pop();
    Lua::Value result;
    Lua::VM::detail::gettable(state, state->at(tableIdx), key, result);
    state->pushValue(result);
}

void lua_settable(lua_State* L, int idx) {
    Lua::LuaState* state = fromC(L);
    const int tableIdx = absIndex(state, idx);
    Lua::Value value = state->pop();
    Lua::Value key = state->pop();
    Lua::VM::detail::settable(state, state->at(tableIdx), key, value);
}

void lua_rawgeti(lua_State* L, int idx, int n) {
    Lua::LuaState* state = fromC(L);
    const int tableIdx = absIndex(state, idx);
    Lua::Value tableValue = state->at(tableIdx);
    if (!tableValue.isTable()) {
        state->pushNil();
        return;
    }
    state->pushValue(tableValue.asTable()->getArray(n));
}

void lua_rawseti(lua_State* L, int idx, int n) {
    Lua::LuaState* state = fromC(L);
    const int tableIdx = absIndex(state, idx);
    Lua::Value value = state->pop();
    Lua::Value tableValue = state->at(tableIdx);
    if (tableValue.isTable()) {
        tableValue.asTable()->setArray(n, value);
    }
}

void lua_getglobal(lua_State* L, const char* name) {
    fromC(L)->pushValue(fromC(L)->getGlobal(name ? name : ""));
}

void lua_setglobal(lua_State* L, const char* name) {
    Lua::LuaState* state = fromC(L);
    Lua::Value value = state->pop();
    state->setGlobal(name ? name : "", value);
}

void lua_call(lua_State* L, int nargs, int nresults) {
    Lua::LuaState* state = fromC(L);
    Lua::RuntimeServices services(state->getGlobalState());
    Lua::VM::call(services, state, nargs, nresults);
}

int lua_pcall(lua_State* L, int nargs, int nresults, int errfunc) {
    return fromC(L)->pcall(nargs, nresults, errfunc);
}

void luaL_openlibs(lua_State* L) {
    Lua::StandardLibrary::openAll(fromC(L));
}

int luaL_error(lua_State* L, const char* fmt, ...) {
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buffer, sizeof(buffer), fmt ? fmt : "", args);
    va_end(args);
    fromC(L)->error(buffer);
    return 0;
}

int luaL_argerror(lua_State* L, int, const char* extramsg) {
    fromC(L)->error(extramsg ? extramsg : "bad argument");
    return 0;
}

void luaL_argcheck(lua_State* L, int cond, int narg, const char* extramsg) {
    if (!cond) {
        luaL_argerror(L, narg, extramsg);
    }
}

lua_Number luaL_checknumber(lua_State* L, int narg) {
    Lua::LuaState* state = fromC(L);
    if (!state->isNumber(narg)) {
        state->error("number expected");
    }
    return state->toNumber(narg);
}

int luaL_checkint(lua_State* L, int narg) {
    return static_cast<int>(luaL_checknumber(L, narg));
}

const char* luaL_checklstring(lua_State* L, int narg, size_t* len) {
    const char* text = lua_tolstring(L, narg, len);
    if (text == nullptr) {
        fromC(L)->error("string expected");
    }
    return text;
}

const char* luaL_checkstring(lua_State* L, int narg) {
    return luaL_checklstring(L, narg, nullptr);
}

}
