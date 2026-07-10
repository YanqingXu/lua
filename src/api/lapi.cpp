#include "lua.h"
#include "lauxlib.h"

#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/upvalue.hpp"
#include "core/userdata.hpp"
#include "core/value.hpp"
#include "lib/lib_manager.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm.hpp"
#include "vm/vm_internal.hpp"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <new>
#include <optional>
#include <string>

static Lua::LuaState* fromC(lua_State* L) {
    return reinterpret_cast<Lua::LuaState*>(L);
}

static lua_State* toC(Lua::LuaState* L) {
    return reinterpret_cast<lua_State*>(L);
}

namespace {

enum class ApiIndexKind {
    Stack,
    Registry,
    Globals,
    Environment,
    Upvalue,
    Invalid,
};

struct ApiIndex {
    ApiIndexKind kind = ApiIndexKind::Invalid;
    Lua::usize stackIndex = 0;
    Lua::Function* closure = nullptr;
    Lua::usize upvalueIndex = 0;
};

Lua::usize currentFrameBase(const Lua::LuaState* L) {
    return L->getCurrentCallInfo().base;
}

int apiTop(const Lua::LuaState* L) {
    return static_cast<int>(L->getAbsoluteTop() - currentFrameBase(L));
}

void setApiTop(Lua::LuaState* L, int idx) {
    const Lua::usize base = currentFrameBase(L);
    const auto newTop = idx >= 0
                            ? static_cast<Lua::isize>(base) + idx
                            : static_cast<Lua::isize>(L->getAbsoluteTop()) + idx + 1;
    if (newTop < static_cast<Lua::isize>(base)) {
        L->error("invalid stack index");
    }

    while (L->getAbsoluteTop() < static_cast<Lua::usize>(newTop)) {
        L->pushNil();
    }
    L->setAbsoluteTop(static_cast<Lua::usize>(newTop));
}

Lua::Function* currentClosure(Lua::LuaState* L) {
    if (L->getCurrentCI() == 0) {
        return {};
    }

    const Lua::usize funcIndex = L->getCurrentCallInfo().func;
    if (funcIndex >= L->getAbsoluteTop()) {
        return {};
    }

    const Lua::Value& value = L->getStack().at(funcIndex);
    return value.isFunction() ? value.asFunction() : nullptr;
}

ApiIndex resolveStackIndex(Lua::LuaState* L, int idx) {
    const Lua::usize base = currentFrameBase(L);
    const Lua::usize top = L->getAbsoluteTop();

    if (idx > 0) {
        const Lua::usize absolute = base + static_cast<Lua::usize>(idx - 1);
        return absolute < top ? ApiIndex{ApiIndexKind::Stack, absolute}
                              : ApiIndex{};
    }

    if (idx < 0 && idx > LUA_REGISTRYINDEX) {
        const auto absolute = static_cast<Lua::isize>(top) + idx;
        return absolute >= static_cast<Lua::isize>(base)
                   ? ApiIndex{ApiIndexKind::Stack, static_cast<Lua::usize>(absolute)}
                   : ApiIndex{};
    }

    if (idx == LUA_REGISTRYINDEX) {
        return ApiIndex{ApiIndexKind::Registry};
    }
    if (idx == LUA_GLOBALSINDEX) {
        return ApiIndex{ApiIndexKind::Globals};
    }
    if (idx == LUA_ENVIRONINDEX) {
        Lua::Function* closure = currentClosure(L);
        return closure != nullptr
                   ? ApiIndex{ApiIndexKind::Environment, 0, closure}
                   : ApiIndex{};
    }
    if (idx < LUA_GLOBALSINDEX) {
        Lua::Function* closure = currentClosure(L);
        const int number = LUA_GLOBALSINDEX - idx;
        if (closure != nullptr && number > 0 &&
            static_cast<Lua::usize>(number) <= closure->getUpvalueCount()) {
            return ApiIndex{ApiIndexKind::Upvalue, 0, closure,
                            static_cast<Lua::usize>(number - 1)};
        }
    }

    return {};
}

std::optional<Lua::Value> readIndex(Lua::LuaState* L, const ApiIndex& index) {
    switch (index.kind) {
        case ApiIndexKind::Stack:
            return L->getStack().at(index.stackIndex);
        case ApiIndexKind::Registry:
            return Lua::Value(L->getGlobalState().getRegistry());
        case ApiIndexKind::Globals:
            return Lua::Value(L->getGlobalTable());
        case ApiIndexKind::Environment:
            return Lua::Value(index.closure->getEnv() != nullptr
                                  ? index.closure->getEnv()
                                  : L->getGlobalTable());
        case ApiIndexKind::Upvalue: {
            Lua::Upvalue* upvalue = index.closure->getUpvalue(index.upvalueIndex);
            if (upvalue != nullptr) {
                return upvalue->getValue(L->getStack());
            }
            break;
        }
        case ApiIndexKind::Invalid:
            break;
    }
    return std::nullopt;
}

std::optional<Lua::Value> readIndex(Lua::LuaState* L, int idx) {
    return readIndex(L, resolveStackIndex(L, idx));
}

bool writeIndex(Lua::LuaState* L, const ApiIndex& index, const Lua::Value& value) {
    switch (index.kind) {
        case ApiIndexKind::Stack:
            L->getStack().at(index.stackIndex) = value;
            return true;
        case ApiIndexKind::Globals:
            if (value.isTable()) {
                L->setGlobalTable(value.asTable());
                return true;
            }
            return false;
        case ApiIndexKind::Environment:
            if (value.isTable()) {
                index.closure->setEnv(value.asTable());
                return true;
            }
            return false;
        case ApiIndexKind::Upvalue: {
            Lua::Upvalue* upvalue = index.closure->getUpvalue(index.upvalueIndex);
            if (upvalue != nullptr) {
                upvalue->setValue(L->getStack(), value);
                return true;
            }
            return false;
        }
        case ApiIndexKind::Registry:
        case ApiIndexKind::Invalid:
            return false;
    }
    return false;
}

int valueType(const Lua::Value& value) {
    return static_cast<int>(value.getType());
}

const char* upvalueName(const Lua::Function* closure, Lua::usize index) {
    if (closure->isCFunction()) {
        return "";
    }

    Lua::Proto* proto = closure->getProto();
    Lua::GCString* name = proto != nullptr ? proto->getUpvalueName(index) : nullptr;
    return name != nullptr ? name->c_str() : "";
}

Lua::Table* valueMetatable(Lua::LuaState* state, const Lua::Value& value) {
    if (value.isTable()) {
        return value.asTable()->getMetatable();
    }
    if (value.isUserdata()) {
        return value.asUserdata()->getMetatable();
    }
    return state->getGlobalState().getMetatable(value.getType());
}

void setValueMetatable(Lua::LuaState* state, const Lua::Value& value, Lua::Table* metatable) {
    if (value.isTable()) {
        value.asTable()->setMetatable(metatable);
    } else if (value.isUserdata()) {
        value.asUserdata()->setMetatable(metatable);
    } else {
        state->getGlobalState().setMetatable(value.getType(), metatable);
    }
}

}  // namespace

extern "C" {

lua_State* lua_newstate(lua_Alloc, void*) {
    return toC(Lua::LuaState::newIsolatedState());
}

lua_State* lua_open(void) {
    return lua_newstate(nullptr, nullptr);
}

void lua_close(lua_State* L) {
    delete fromC(L);
}

int lua_gettop(lua_State* L) {
    return apiTop(fromC(L));
}

void lua_settop(lua_State* L, int idx) {
    setApiTop(fromC(L), idx);
}

int lua_checkstack(lua_State* L, int extra) {
    Lua::LuaState* state = fromC(L);
    if (extra < 0) {
        return 0;
    }

    const Lua::usize top = state->getAbsoluteTop();
    const Lua::usize additional = static_cast<Lua::usize>(extra);
    if (top > Lua::MAX_STACK_SIZE || additional > Lua::MAX_STACK_SIZE - top) {
        return 0;
    }

    const Lua::usize desired = top + additional;
    try {
        Lua::Stack& stack = state->getStack();
        if (stack.capacity() < desired) {
            stack.ensureSpace(desired - stack.size());
        }
        state->getCurrentCallInfo().top =
            std::max(state->getCurrentCallInfo().top, desired);
        return 1;
    } catch (const Lua::MemoryError&) {
        return 0;
    } catch (const std::bad_alloc&) {
        return 0;
    }
}

void lua_xmove(lua_State* from, lua_State* to, int n) {
    if (from == to) {
        return;
    }

    Lua::LuaState* source = fromC(from);
    Lua::LuaState* destination = fromC(to);
    if (n < 0 || n > apiTop(source)) {
        source->error("invalid value count for lua_xmove");
    }
    if (&source->getGlobalState() != &destination->getGlobalState() ||
        source->getGlobalTable() != destination->getGlobalTable()) {
        source->error("cannot move values between independent states");
    }
    if (lua_checkstack(to, n) == 0) {
        destination->error("stack overflow in lua_xmove");
    }

    Lua::Vec<Lua::Value> values;
    values.reserve(static_cast<Lua::usize>(n));
    for (int i = n; i > 0; --i) {
        values.push_back(source->at(-i));
    }
    setApiTop(source, apiTop(source) - n);
    for (const Lua::Value& value : values) {
        destination->pushValue(value);
    }
}

void lua_pushvalue(lua_State* L, int idx) {
    Lua::LuaState* state = fromC(L);
    const auto value = readIndex(state, idx);
    if (value.has_value()) {
        state->pushValue(*value);
    }
}

void lua_remove(lua_State* L, int idx) {
    Lua::LuaState* state = fromC(L);
    const ApiIndex index = resolveStackIndex(state, idx);
    if (index.kind != ApiIndexKind::Stack) {
        return;
    }

    const Lua::usize top = state->getAbsoluteTop();
    for (Lua::usize i = index.stackIndex; i + 1 < top; ++i) {
        state->getStack().at(i) = state->getStack().at(i + 1);
    }
    state->setAbsoluteTop(top - 1);
}

void lua_insert(lua_State* L, int idx) {
    Lua::LuaState* state = fromC(L);
    const ApiIndex index = resolveStackIndex(state, idx);
    const Lua::usize top = state->getAbsoluteTop();
    if (index.kind != ApiIndexKind::Stack || top <= currentFrameBase(state)) {
        return;
    }

    const Lua::Value value = state->getStack().at(top - 1);
    for (Lua::usize i = top - 1; i > index.stackIndex; --i) {
        state->getStack().at(i) = state->getStack().at(i - 1);
    }
    state->getStack().at(index.stackIndex) = value;
}

void lua_replace(lua_State* L, int idx) {
    Lua::LuaState* state = fromC(L);
    const ApiIndex index = resolveStackIndex(state, idx);
    if (index.kind == ApiIndexKind::Invalid || index.kind == ApiIndexKind::Registry ||
        state->getAbsoluteTop() <= currentFrameBase(state)) {
        return;
    }

    const Lua::Value value = state->pop();
    (void)writeIndex(state, index, value);
}

int lua_type(lua_State* L, int idx) {
    const auto value = readIndex(fromC(L), idx);
    return value.has_value() ? valueType(*value) : LUA_TNONE;
}

const char* lua_typename(lua_State* L, int tp) {
    return fromC(L)->typeName(tp);
}

int lua_isnumber(lua_State* L, int idx) {
    Lua::LuaState* state = fromC(L);
    const auto value = readIndex(state, idx);
    if (!value.has_value()) {
        return 0;
    }
    if (value->isNumber()) {
        return 1;
    }
    if (!value->isString()) {
        return 0;
    }

    state->pushValue(*value);
    const bool result = state->isNumber(-1);
    state->pop();
    return result ? 1 : 0;
}

int lua_isstring(lua_State* L, int idx) {
    const int t = lua_type(L, idx);
    return (t == LUA_TSTRING || t == LUA_TNUMBER) ? 1 : 0;
}

int lua_iscfunction(lua_State* L, int idx) {
    const auto value = readIndex(fromC(L), idx);
    return value.has_value() && value->isFunction() && value->asFunction()->isCFunction() ? 1 : 0;
}

int lua_isuserdata(lua_State* L, int idx) {
    const int t = lua_type(L, idx);
    return (t == LUA_TUSERDATA || t == LUA_TLIGHTUSERDATA) ? 1 : 0;
}

int lua_toboolean(lua_State* L, int idx) {
    const auto value = readIndex(fromC(L), idx);
    return value.has_value() && value->isTrue() ? 1 : 0;
}

lua_Number lua_tonumber(lua_State* L, int idx) {
    Lua::LuaState* state = fromC(L);
    const auto value = readIndex(state, idx);
    if (!value.has_value()) {
        return 0;
    }
    if (value->isNumber()) {
        return value->asNumber();
    }

    state->pushValue(*value);
    const lua_Number result = state->toNumber(-1);
    state->pop();
    return result;
}

const char* lua_tolstring(lua_State* L, int idx, size_t* len) {
    Lua::LuaState* state = fromC(L);
    const ApiIndex index = resolveStackIndex(state, idx);
    const auto value = readIndex(state, index);
    if (!value.has_value()) {
        if (len) {
            *len = 0;
        }
        return {};
    }

    state->pushValue(*value);
    const char* text = state->toString(-1);
    if (text == nullptr) {
        state->pop();
        if (len) {
            *len = 0;
        }
        return nullptr;
    }
    const Lua::Value converted = state->at(-1);
    if (len) {
        *len = converted.isString() ? converted.asString()->getLength() : std::strlen(text);
    }
    (void)writeIndex(state, index, converted);
    state->pop();
    return text;
}

void* lua_touserdata(lua_State* L, int idx) {
    const auto value = readIndex(fromC(L), idx);
    if (!value.has_value()) {
        return {};
    }
    if (value->isLightUserdata()) {
        return value->asLightUserdata();
    }
    return value->isUserdata() ? value->asUserdata()->getData() : nullptr;
}

size_t lua_objlen(lua_State* L, int idx) {
    const auto value = readIndex(fromC(L), idx);
    if (!value.has_value()) {
        return 0;
    }
    if (value->isString()) {
        return value->asString()->getLength();
    }
    if (value->isTable()) {
        return value->asTable()->length();
    }
    if (value->isUserdata()) {
        return value->asUserdata()->getDataSize();
    }
    return 0;
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

void lua_pushcclosure(lua_State* L, lua_CFunction fn, int n) {
    Lua::LuaState* state = fromC(L);
    if (fn == nullptr || n < 0 || n > apiTop(state)) {
        state->error("invalid C closure");
    }

    Lua::Vec<Lua::Value> upvalues;
    upvalues.reserve(static_cast<Lua::usize>(n));
    for (int i = n; i > 0; --i) {
        upvalues.push_back(state->at(-i));
    }

    auto internal = reinterpret_cast<Lua::CFunction>(fn);
    Lua::Function* closure = state->getGlobalState().getGC().create<Lua::Function>(internal);
    closure->setEnv(state->getGlobalTable());
    for (const Lua::Value& value : upvalues) {
        closure->addUpvalue(state->getGlobalState().getGC().create<Lua::Upvalue>(value));
    }
    setApiTop(state, apiTop(state) - n);
    state->pushFunction(closure);
}

void lua_pushlightuserdata(lua_State* L, void* p) {
    fromC(L)->pushValue(Lua::Value(p));
}

const char* lua_getupvalue(lua_State* L, int funcindex, int n) {
    Lua::LuaState* state = fromC(L);
    const auto value = readIndex(state, funcindex);
    if (!value.has_value() || !value->isFunction() || n <= 0) {
        return {};
    }

    Lua::Function* closure = value->asFunction();
    const Lua::usize index = static_cast<Lua::usize>(n - 1);
    Lua::Upvalue* upvalue = closure->getUpvalue(index);
    if (upvalue == nullptr) {
        return {};
    }

    state->pushValue(upvalue->getValue(state->getStack()));
    return upvalueName(closure, index);
}

const char* lua_setupvalue(lua_State* L, int funcindex, int n) {
    Lua::LuaState* state = fromC(L);
    const auto value = readIndex(state, funcindex);
    if (!value.has_value() || !value->isFunction() || n <= 0) {
        return {};
    }

    Lua::Function* closure = value->asFunction();
    const Lua::usize index = static_cast<Lua::usize>(n - 1);
    Lua::Upvalue* upvalue = closure->getUpvalue(index);
    if (upvalue == nullptr) {
        return {};
    }
    if (apiTop(state) == 0) {
        state->error("lua_setupvalue requires a value");
    }

    const Lua::Value replacement = state->pop();
    upvalue->setValue(state->getStack(), replacement);
    return upvalueName(closure, index);
}

void lua_createtable(lua_State* L, int, int) {
    Lua::LuaState* state = fromC(L);
    state->pushTable(state->getGlobalState().getGC().create<Lua::Table>());
}

void lua_gettable(lua_State* L, int idx) {
    Lua::LuaState* state = fromC(L);
    const auto table = readIndex(state, idx);
    Lua::Value key = state->pop();
    Lua::Value result;
    if (!table.has_value()) {
        state->pushNil();
        return;
    }
    Lua::VM::detail::gettable(state, *table, key, result);
    state->pushValue(result);
}

void* lua_newuserdata(lua_State* L, size_t size) {
    Lua::LuaState* state = fromC(L);
    Lua::Userdata* userdata =
        state->getGlobalState().getGC().create<Lua::Userdata>(static_cast<Lua::usize>(size));
    state->pushUserdata(userdata);
    return userdata->getData();
}

void lua_settable(lua_State* L, int idx) {
    Lua::LuaState* state = fromC(L);
    const auto table = readIndex(state, idx);
    Lua::Value value = state->pop();
    Lua::Value key = state->pop();
    if (table.has_value()) {
        Lua::VM::detail::settable(state, *table, key, value);
    }
}

void lua_rawgeti(lua_State* L, int idx, int n) {
    Lua::LuaState* state = fromC(L);
    const auto table = readIndex(state, idx);
    if (!table.has_value() || !table->isTable()) {
        state->pushNil();
        return;
    }
    state->pushValue(table->asTable()->getArray(n));
}

void lua_rawseti(lua_State* L, int idx, int n) {
    Lua::LuaState* state = fromC(L);
    const auto table = readIndex(state, idx);
    Lua::Value value = state->pop();
    if (table.has_value() && table->isTable()) {
        table->asTable()->setArray(n, value);
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

int lua_getmetatable(lua_State* L, int objindex) {
    Lua::LuaState* state = fromC(L);
    const auto value = readIndex(state, objindex);
    if (!value.has_value()) {
        return 0;
    }

    Lua::Table* metatable = valueMetatable(state, *value);
    if (metatable == nullptr) {
        return 0;
    }
    state->pushTable(metatable);
    return 1;
}

int lua_setmetatable(lua_State* L, int objindex) {
    Lua::LuaState* state = fromC(L);
    const auto value = readIndex(state, objindex);
    if (!value.has_value() || apiTop(state) == 0) {
        return 0;
    }

    const Lua::Value candidate = state->at(-1);
    if (!candidate.isNil() && !candidate.isTable()) {
        state->error("table or nil expected for metatable");
    }
    Lua::Table* metatable = candidate.isTable() ? candidate.asTable() : nullptr;
    setValueMetatable(state, *value, metatable);
    state->pop();
    return 1;
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
}

int luaL_argerror(lua_State* L, int, const char* extramsg) {
    fromC(L)->error(extramsg ? extramsg : "bad argument");
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
