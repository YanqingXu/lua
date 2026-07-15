#include "lua.h"
#include "lauxlib.h"

#include "common/lua_error.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/thread.hpp"
#include "core/upvalue.hpp"
#include "core/userdata.hpp"
#include "core/value.hpp"
#include "lib/lib_manager.hpp"
#include "lib/baselib.hpp"
#include "lib/stringlib.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm.hpp"
#include "vm/vm_internal.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <new>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

static Lua::LuaState* fromC(lua_State* L) {
    return reinterpret_cast<Lua::LuaState*>(L);
}

static lua_State* toC(Lua::LuaState* L) {
    return reinterpret_cast<lua_State*>(L);
}

namespace {

void* defaultLuaAllocator(void*, void* pointer, size_t, size_t newSize) {
    if (newSize == 0) {
        std::free(pointer);
        return {};
    }
    return std::realloc(pointer, newSize);
}

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
    const Lua::usize oldTop = L->getAbsoluteTop();
    const auto newTop = idx >= 0 ? static_cast<Lua::isize>(base) + idx : static_cast<Lua::isize>(oldTop) + idx + 1;
    if (newTop < static_cast<Lua::isize>(base)) {
        L->error("invalid stack index");
    }

    while (L->getAbsoluteTop() < static_cast<Lua::usize>(newTop)) {
        L->pushNil();
    }
    const Lua::usize resolvedTop = static_cast<Lua::usize>(newTop);
    for (Lua::usize slot = resolvedTop; slot < oldTop; ++slot) {
        L->getStack().at(slot) = Lua::Value();
    }
    L->setAbsoluteTop(resolvedTop);
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
        return absolute < top ? ApiIndex{ApiIndexKind::Stack, absolute} : ApiIndex{};
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
        return closure != nullptr ? ApiIndex{ApiIndexKind::Environment, 0, closure} : ApiIndex{};
    }
    if (idx < LUA_GLOBALSINDEX) {
        Lua::Function* closure = currentClosure(L);
        const int number = LUA_GLOBALSINDEX - idx;
        if (closure != nullptr && number > 0 && static_cast<Lua::usize>(number) <= closure->getUpvalueCount()) {
            return ApiIndex{ApiIndexKind::Upvalue, 0, closure, static_cast<Lua::usize>(number - 1)};
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
        return Lua::Value(index.closure->getEnv() != nullptr ? index.closure->getEnv() : L->getGlobalTable());
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

void pushInternalCFunction(Lua::LuaState* state, Lua::CFunction callback) {
    Lua::Function* function = state->getGlobalState().getGC().create<Lua::Function>(callback);
    function->setEnv(state->getGlobalTable());
    state->pushFunction(function);
}

int normalizeLoadResult(lua_State* L, int base, int callStatus, int failureStatus) {
    if (callStatus != LUA_OK) {
        return callStatus;
    }

    const int top = lua_gettop(L);
    if (top == base + 1 && lua_isfunction(L, -1)) {
        return LUA_OK;
    }
    if (top >= base + 2 && lua_isnil(L, base + 1)) {
        lua_remove(L, base + 1);
        lua_settop(L, base + 1);
        return failureStatus;
    }

    lua_settop(L, base);
    lua_pushstring(L, "loader returned an invalid result shape");
    return LUA_ERRRUN;
}

void restoreAbsoluteTopClearingSlots(Lua::LuaState* state, Lua::usize restoredTop) noexcept {
    const Lua::usize oldTop = state->getAbsoluteTop();
    if (restoredTop < oldTop) {
        Lua::Stack& stack = state->getStack();
        const Lua::usize clearEnd = std::min(oldTop, stack.capacity());
        for (Lua::usize slot = restoredTop; slot < clearEnd; ++slot) {
            stack[slot] = Lua::Value();
        }
    }
    state->setAbsoluteTop(restoredTop);
}

int publishApiStatusError(Lua::LuaState* state, Lua::usize savedTop, const Lua::Value& errorValue,
                          int status) noexcept {
    const Lua::usize frameBase = currentFrameBase(state);
    const Lua::usize restoredTop = std::max(savedTop, frameBase);
    restoreAbsoluteTopClearingSlots(state, restoredTop);
    if (state->tryPushValueNoAlloc(errorValue)) {
        return status;
    }

    const Lua::Value memoryError(state->getGlobalState().getMemoryErrorMessage());
    if (state->tryPushValueNoAlloc(memoryError)) {
        return LUA_ERRMEM;
    }

    // Ordinary API pushes reserve one physical stack slot. If legacy/internal
    // code violated that invariant, preserve the caller's prefix rather than
    // overwriting its final value merely to publish an error object.
    return LUA_ERRMEM;
}

template <typename Failure>
int publishApiRuntimeException(Lua::LuaState* state, const char* message, Failure& failure) noexcept {
    try {
        Lua::GCString* text = state->getGlobalState().getStringPool().intern(message != nullptr ? message : "");
        return failure(Lua::Value(text), LUA_ERRRUN);
    } catch (...) {
        return failure(Lua::Value(state->getGlobalState().getApiExceptionMessage()), LUA_ERRRUN);
    }
}

template <typename Action, typename Failure>
int apiStatusBoundary(Lua::LuaState* state, Action&& action, Failure&& failure) noexcept {
    static_assert(std::is_nothrow_invocable_r_v<int, Failure&, const Lua::Value&, int>);
    try {
        return std::forward<Action>(action)();
    } catch (const Lua::MemoryError&) {
        return failure(Lua::Value(state->getGlobalState().getMemoryErrorMessage()), LUA_ERRMEM);
    } catch (const std::bad_alloc&) {
        return failure(Lua::Value(state->getGlobalState().getMemoryErrorMessage()), LUA_ERRMEM);
    } catch (const Lua::LuaError& error) {
        if (error.hasErrorObject()) {
            return failure(error.getErrorObject(), LUA_ERRRUN);
        }
        return publishApiRuntimeException(state, error.what(), failure);
    } catch (const std::exception& error) {
        return publishApiRuntimeException(state, error.what(), failure);
    } catch (...) {
        return failure(Lua::Value(state->getGlobalState().getApiExceptionMessage()), LUA_ERRRUN);
    }
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

int prepareCFunctionCoroutineEntry(lua_State* L, int nargs) {
    Lua::LuaState* state = fromC(L);
    if (state->getCurrentCI() != 0 || apiTop(state) != nargs + 1) {
        return LUA_OK;
    }

    const std::optional<Lua::Value> entry = readIndex(state, 1);
    if (!entry.has_value() || !entry->isFunction() || !entry->asFunction()->isCFunction()) {
        return LUA_OK;
    }

    // Thread::resume executes a Lua frame. A tiny vararg trampoline keeps the
    // C callback inside that resumable frame, so both ordinary returns and a
    // lua_yield from a C entry follow the same continuation path as a C call
    // made by Lua code. Its private environment binds `f` without changing the
    // entry closure's own environment or imposing a limit on argument count.
    static constexpr char source[] = "return f(...)";
    const int status = luaL_loadbuffer(L, source, sizeof(source) - 1, "=(C coroutine entry)");
    if (status == LUA_OK) {
        Lua::Function* trampoline = readIndex(state, -1)->asFunction();
        Lua::GCString* name = state->getGlobalState().getStringPool().intern("f");
        Lua::Table* environment = state->getGlobalState().getGC().create<Lua::Table>();
        trampoline->setEnv(environment);
        environment->set(Lua::Value(name), *entry);
        lua_insert(L, 1);
        lua_remove(L, 2);
    }
    return status;
}

} // namespace

extern "C" {

lua_State* lua_newstate(lua_Alloc allocator, void* userData) LUA_CXX_MAY_THROW {
    lua_Alloc effectiveAllocator = allocator != nullptr ? allocator : defaultLuaAllocator;
    return toC(Lua::LuaState::newAllocatedState(effectiveAllocator, userData));
}

lua_State* lua_open(void) LUA_CXX_MAY_THROW {
    return lua_newstate(nullptr, nullptr);
}

void lua_close(lua_State* L) LUA_CXX_MAY_THROW {
    Lua::LuaState::destroyState(fromC(L));
}

lua_Alloc lua_getallocf(lua_State* L, void** userData) LUA_CXX_MAY_THROW {
    Lua::LuaAllocator* allocator = fromC(L)->getGlobalState().getAllocator();
    if (userData != nullptr) {
        *userData = allocator != nullptr ? allocator->getUserData() : nullptr;
    }
    return allocator != nullptr ? allocator->getFunction() : nullptr;
}

void lua_setallocf(lua_State* L, lua_Alloc allocatorFunction, void* userData) LUA_CXX_MAY_THROW {
    Lua::LuaAllocator* allocator = fromC(L)->getGlobalState().getAllocator();
    if (allocator != nullptr && allocatorFunction != nullptr) {
        allocator->set(allocatorFunction, userData);
    }
}

int lua_gettop(lua_State* L) LUA_CXX_MAY_THROW {
    return apiTop(fromC(L));
}

void lua_settop(lua_State* L, int idx) LUA_CXX_MAY_THROW {
    setApiTop(fromC(L), idx);
}

int lua_checkstack(lua_State* L, int extra) LUA_CXX_NOEXCEPT {
    Lua::LuaState* state = fromC(L);
    if (extra < 0) {
        return 0;
    }

    const Lua::usize top = state->getAbsoluteTop();
    const Lua::usize additional = static_cast<Lua::usize>(extra);
    // One physical slot is reserved for allocation-free error publication.
    // Consequently the largest ordinary logical top is MAX_STACK_SIZE - 1.
    if (top >= Lua::MAX_STACK_SIZE || additional >= Lua::MAX_STACK_SIZE - top) {
        return 0;
    }

    const Lua::usize desired = top + additional;
    try {
        Lua::Stack& stack = state->getStack();
        const Lua::usize requiredCapacity = desired + 1;
        if (stack.capacity() < requiredCapacity) {
            stack.ensureSpace(requiredCapacity - stack.size());
        }
        state->getCurrentCallInfo().top = std::max(state->getCurrentCallInfo().top, desired);
        return 1;
    } catch (const Lua::MemoryError&) {
        return 0;
    } catch (const std::bad_alloc&) {
        return 0;
    } catch (...) {
        return 0;
    }
}

void lua_xmove(lua_State* from, lua_State* to, int n) LUA_CXX_MAY_THROW {
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

void lua_pushvalue(lua_State* L, int idx) LUA_CXX_MAY_THROW {
    Lua::LuaState* state = fromC(L);
    const auto value = readIndex(state, idx);
    if (value.has_value()) {
        state->pushValue(*value);
    }
}

void lua_remove(lua_State* L, int idx) LUA_CXX_MAY_THROW {
    Lua::LuaState* state = fromC(L);
    const ApiIndex index = resolveStackIndex(state, idx);
    if (index.kind != ApiIndexKind::Stack) {
        return;
    }

    const Lua::usize top = state->getAbsoluteTop();
    for (Lua::usize i = index.stackIndex; i + 1 < top; ++i) {
        state->getStack().at(i) = state->getStack().at(i + 1);
    }
    state->getStack().at(top - 1) = Lua::Value();
    state->setAbsoluteTop(top - 1);
}

void lua_insert(lua_State* L, int idx) LUA_CXX_MAY_THROW {
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

void lua_replace(lua_State* L, int idx) LUA_CXX_MAY_THROW {
    Lua::LuaState* state = fromC(L);
    const ApiIndex index = resolveStackIndex(state, idx);
    if (index.kind == ApiIndexKind::Invalid || index.kind == ApiIndexKind::Registry ||
        state->getAbsoluteTop() <= currentFrameBase(state)) {
        return;
    }

    const Lua::Value value = state->pop();
    (void)writeIndex(state, index, value);
}

int lua_type(lua_State* L, int idx) LUA_CXX_MAY_THROW {
    const auto value = readIndex(fromC(L), idx);
    return value.has_value() ? valueType(*value) : LUA_TNONE;
}

const char* lua_typename(lua_State* L, int tp) LUA_CXX_MAY_THROW {
    return fromC(L)->typeName(tp);
}

int lua_isnumber(lua_State* L, int idx) LUA_CXX_MAY_THROW {
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

int lua_isstring(lua_State* L, int idx) LUA_CXX_MAY_THROW {
    const int t = lua_type(L, idx);
    return (t == LUA_TSTRING || t == LUA_TNUMBER) ? 1 : 0;
}

int lua_iscfunction(lua_State* L, int idx) LUA_CXX_MAY_THROW {
    const auto value = readIndex(fromC(L), idx);
    return value.has_value() && value->isFunction() && value->asFunction()->isCFunction() ? 1 : 0;
}

int lua_isuserdata(lua_State* L, int idx) LUA_CXX_MAY_THROW {
    const int t = lua_type(L, idx);
    return (t == LUA_TUSERDATA || t == LUA_TLIGHTUSERDATA) ? 1 : 0;
}

int lua_toboolean(lua_State* L, int idx) LUA_CXX_MAY_THROW {
    const auto value = readIndex(fromC(L), idx);
    return value.has_value() && value->isTrue() ? 1 : 0;
}

lua_Number lua_tonumber(lua_State* L, int idx) LUA_CXX_MAY_THROW {
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

const char* lua_tolstring(lua_State* L, int idx, size_t* len) LUA_CXX_MAY_THROW {
    Lua::LuaState* state = fromC(L);
    const ApiIndex index = resolveStackIndex(state, idx);
    const auto value = readIndex(state, index);
    if (!value.has_value()) {
        if (len) {
            *len = 0;
        }
        return {};
    }

    // Reading an existing string must remain allocation-free. In particular,
    // protected loader failures may occupy the reserved emergency stack slot
    // while the host allocator is still rejecting every request.
    if (value->isString()) {
        if (len != nullptr) {
            *len = value->asString()->getLength();
        }
        return value->asString()->c_str();
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

void* lua_touserdata(lua_State* L, int idx) LUA_CXX_MAY_THROW {
    const auto value = readIndex(fromC(L), idx);
    if (!value.has_value()) {
        return {};
    }
    if (value->isLightUserdata()) {
        return value->asLightUserdata();
    }
    return value->isUserdata() ? value->asUserdata()->getData() : nullptr;
}

size_t lua_objlen(lua_State* L, int idx) LUA_CXX_MAY_THROW {
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
    if (value->isNumber()) {
        size_t length = 0;
        (void)lua_tolstring(L, idx, &length);
        return length;
    }
    return 0;
}

void lua_pushnil(lua_State* L) LUA_CXX_MAY_THROW {
    fromC(L)->pushNil();
}

void lua_pushnumber(lua_State* L, lua_Number n) LUA_CXX_MAY_THROW {
    fromC(L)->pushNumber(n);
}

void lua_pushinteger(lua_State* L, lua_Integer n) LUA_CXX_MAY_THROW {
    fromC(L)->pushNumber(static_cast<Lua::LuaNumber>(n));
}

void lua_pushboolean(lua_State* L, int b) LUA_CXX_MAY_THROW {
    fromC(L)->pushBoolean(b != 0);
}

void lua_pushlstring(lua_State* L, const char* s, size_t len) LUA_CXX_MAY_THROW {
    Lua::LuaState* state = fromC(L);
    state->pushString(state->getGlobalState().getStringPool().intern(s ? s : "", len));
}

void lua_pushstring(lua_State* L, const char* s) LUA_CXX_MAY_THROW {
    if (s == nullptr) {
        lua_pushnil(L);
        return;
    }
    lua_pushlstring(L, s, std::strlen(s));
}

void lua_pushcclosure(lua_State* L, lua_CFunction fn, int n) LUA_CXX_MAY_THROW {
    Lua::LuaState* state = fromC(L);
    if (fn == nullptr || n < 0 || n > apiTop(state)) {
        state->error("invalid C closure");
    }

    Lua::Vec<Lua::Value> upvalues;
    upvalues.reserve(static_cast<Lua::usize>(n));
    for (int i = n; i > 0; --i) {
        upvalues.push_back(state->at(-i));
    }

    Lua::Function* closure = state->getGlobalState().getGC().create<Lua::Function>(fn);
    closure->setEnv(state->getGlobalTable());
    for (const Lua::Value& value : upvalues) {
        closure->addUpvalue(state->getGlobalState().getGC().create<Lua::Upvalue>(value));
    }
    setApiTop(state, apiTop(state) - n);
    state->pushFunction(closure);
}

void lua_pushlightuserdata(lua_State* L, void* p) LUA_CXX_MAY_THROW {
    fromC(L)->pushValue(Lua::Value(p));
}

const char* lua_getupvalue(lua_State* L, int funcindex, int n) LUA_CXX_MAY_THROW {
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

const char* lua_setupvalue(lua_State* L, int funcindex, int n) LUA_CXX_MAY_THROW {
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

void lua_createtable(lua_State* L, int, int) LUA_CXX_MAY_THROW {
    Lua::LuaState* state = fromC(L);
    state->pushTable(state->getGlobalState().getGC().create<Lua::Table>());
}

void lua_gettable(lua_State* L, int idx) LUA_CXX_MAY_THROW {
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

void* lua_newuserdata(lua_State* L, size_t size) LUA_CXX_MAY_THROW {
    Lua::LuaState* state = fromC(L);
    Lua::Userdata* userdata = state->getGlobalState().getGC().create<Lua::Userdata>(static_cast<Lua::usize>(size));
    Lua::Function* closure = currentClosure(state);
    userdata->setEnvironment(closure != nullptr && closure->getEnv() != nullptr ? closure->getEnv()
                                                                                : state->getGlobalTable());
    state->pushUserdata(userdata);
    return userdata->getData();
}

void lua_settable(lua_State* L, int idx) LUA_CXX_MAY_THROW {
    Lua::LuaState* state = fromC(L);
    const auto table = readIndex(state, idx);
    Lua::Value value = state->pop();
    Lua::Value key = state->pop();
    if (table.has_value()) {
        Lua::VM::detail::settable(state, *table, key, value);
    }
}

void lua_rawgeti(lua_State* L, int idx, int n) LUA_CXX_MAY_THROW {
    Lua::LuaState* state = fromC(L);
    const auto table = readIndex(state, idx);
    if (!table.has_value() || !table->isTable()) {
        state->pushNil();
        return;
    }
    state->pushValue(table->asTable()->getArray(n));
}

void lua_rawseti(lua_State* L, int idx, int n) LUA_CXX_MAY_THROW {
    Lua::LuaState* state = fromC(L);
    const auto table = readIndex(state, idx);
    Lua::Value value = state->pop();
    if (table.has_value() && table->isTable()) {
        table->asTable()->setArray(n, value);
    }
}

void lua_getglobal(lua_State* L, const char* name) LUA_CXX_MAY_THROW {
    fromC(L)->pushValue(fromC(L)->getGlobal(name ? name : ""));
}

void lua_setglobal(lua_State* L, const char* name) LUA_CXX_MAY_THROW {
    Lua::LuaState* state = fromC(L);
    Lua::Value value = state->pop();
    state->setGlobal(name ? name : "", value);
}

int lua_getmetatable(lua_State* L, int objindex) LUA_CXX_MAY_THROW {
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

int lua_setmetatable(lua_State* L, int objindex) LUA_CXX_MAY_THROW {
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

void lua_call(lua_State* L, int nargs, int nresults) LUA_CXX_MAY_THROW {
    Lua::LuaState* state = fromC(L);
    Lua::RuntimeServices services(state->getGlobalState());
    Lua::VM::call(services, state, nargs, nresults);
}

int lua_pcall(lua_State* L, int nargs, int nresults, int errfunc) LUA_CXX_NOEXCEPT {
    Lua::LuaState* state = fromC(L);
    const Lua::usize frameBase = currentFrameBase(state);
    const Lua::usize absoluteTop = state->getAbsoluteTop();
    const Lua::usize argumentCount = nargs >= 0 ? static_cast<Lua::usize>(nargs) : 0;
    const Lua::usize savedPrefixTop =
        nargs >= 0 && absoluteTop >= frameBase + argumentCount + 1 ? absoluteTop - argumentCount - 1 : frameBase;

    auto failure = [state, savedPrefixTop](const Lua::Value& errorValue, int status) noexcept {
        const int result = publishApiStatusError(state, savedPrefixTop, errorValue, status);
        state->setStatus(Lua::ThreadStatus::OK);
        return result;
    };
    return apiStatusBoundary(
        state,
        [&]() {
            int internalErrorFunction = errfunc;
            if (errfunc > 0 && state->getCurrentCI() == 0) {
                internalErrorFunction += static_cast<int>(currentFrameBase(state));
            }
            return state->pcall(nargs, nresults, internalErrorFunction);
        },
        failure);
}

lua_State* lua_newthread(lua_State* L) LUA_CXX_NOEXCEPT {
    Lua::LuaState* parent = fromC(L);
    const Lua::usize savedTop = parent->getAbsoluteTop();
    try {
        Lua::Thread* thread = Lua::Thread::create(parent);
        parent->pushValue(Lua::Value(thread));
        return toC(thread->getLuaState());
    } catch (...) {
        restoreAbsoluteTopClearingSlots(parent, savedTop);
        return nullptr;
    }
}

int lua_resume(lua_State* L, int nargs) LUA_CXX_NOEXCEPT {
    Lua::LuaState* state = fromC(L);
    Lua::Thread* thread = state->getThread();

    auto fail = [&](const Lua::Value& errorValue, int status) noexcept {
        // A strong rollback can remove the suspended frame. Compute the base
        // after that rollback instead of publishing above a stale frame base.
        const int result = publishApiStatusError(state, currentFrameBase(state), errorValue, status);
        state->setStatus(static_cast<Lua::ThreadStatus>(result));
        return result;
    };
    auto runtimeError = [&](const char* message) noexcept { return publishApiRuntimeException(state, message, fail); };

    if (thread == nullptr) {
        return runtimeError("cannot resume main state");
    }
    if (nargs < 0 || nargs > apiTop(state)) {
        return runtimeError("invalid resume argument count");
    }

    Lua::GlobalState& globalState = state->getGlobalState();
    Lua::LuaState* bridge = globalState.getMainThread();
    Lua::Thread* running = globalState.getRunningThread();
    if (running != nullptr && running != thread) {
        bridge = running->getLuaState();
    }
    if (bridge == nullptr || bridge == state) {
        return runtimeError("cannot find resume caller state");
    }

    const Lua::usize bridgeTop = bridge->getAbsoluteTop();
    auto failure = [&](const Lua::Value& errorValue, int status) noexcept {
        restoreAbsoluteTopClearingSlots(bridge, bridgeTop);
        thread->abortResume(status == LUA_ERRMEM ? Lua::ThreadStatus::ErrMem : Lua::ThreadStatus::ErrRun);
        return fail(errorValue, status);
    };
    return apiStatusBoundary(
        state,
        [&]() {
            const int preparationStatus = prepareCFunctionCoroutineEntry(L, nargs);
            if (preparationStatus != LUA_OK) {
                const Lua::Value errorValue = state->at(-1);
                thread->abortResume(preparationStatus == LUA_ERRMEM ? Lua::ThreadStatus::ErrMem
                                                                    : Lua::ThreadStatus::ErrRun);
                return fail(errorValue, preparationStatus);
            }
            const int stateTop = apiTop(state);
            // Thread::resume owns the VM transition and expects resume arguments
            // on its caller stack. Keep the bridge's existing prefix untouched.
            for (int i = nargs; i > 0; --i) {
                bridge->pushValue(state->at(-i));
            }
            setApiTop(state, stateTop - nargs);

            const bool resumed = thread->resume(bridge, nargs);
            const Lua::usize outputTop = bridge->getAbsoluteTop();
            const Lua::usize outputCount = outputTop >= bridgeTop ? outputTop - bridgeTop : 0;
            if (!resumed) {
                Lua::Value errorValue;
                if (outputCount >= 2) {
                    errorValue = bridge->getStack().at(bridgeTop + 1);
                }
                restoreAbsoluteTopClearingSlots(bridge, bridgeTop);

                int status = static_cast<int>(state->getStatus());
                if (status == LUA_OK || status == LUA_YIELD) {
                    status = LUA_ERRRUN;
                }
                if (outputCount < 2) {
                    auto& pool = globalState.getStringPool();
                    errorValue = Lua::Value(pool.intern("coroutine resume failed"));
                }
                return fail(errorValue, status);
            }

            Lua::LuaVector<Lua::Value> outputValues(Lua::LuaStdAllocator<Lua::Value>(globalState.getAllocator()));
            if (outputCount > 1) {
                outputValues.reserve(outputCount - 1);
                for (Lua::usize i = 1; i < outputCount; ++i) {
                    outputValues.push_back(bridge->getStack().at(bridgeTop + i));
                }
            }
            restoreAbsoluteTopClearingSlots(bridge, bridgeTop);

            if (thread->isSuspended()) {
                setApiTop(state, 0);
                for (const Lua::Value& value : outputValues) {
                    state->pushValue(value);
                }
                return static_cast<int>(LUA_YIELD);
            }

            // The internal coroutine runner copies results to its caller. A public
            // lua_resume must additionally leave those results on the resumed
            // state's own API stack.
            setApiTop(state, 0);
            for (const Lua::Value& value : outputValues) {
                state->pushValue(value);
            }
            return static_cast<int>(LUA_OK);
        },
        failure);
}

int lua_error(lua_State* L) LUA_CXX_MAY_THROW {
    Lua::LuaState* state = fromC(L);
    if (apiTop(state) == 0) {
        state->error("lua_error requires an error object");
    }
    return state->error();
}

int lua_yield(lua_State* L, int nresults) LUA_CXX_MAY_THROW {
    Lua::LuaState* state = fromC(L);
    if (nresults < 0 || nresults > apiTop(state)) {
        state->error("invalid yield result count");
    }
    if (!state->canYield()) {
        state->error("cannot yield across non-resumable call boundaries");
    }

    state->setStatus(Lua::ThreadStatus::Yield);
    state->setYieldResults(nresults);
    return 0;
}

int lua_status(lua_State* L) LUA_CXX_MAY_THROW {
    return static_cast<int>(fromC(L)->getStatus());
}

int luaL_loadbuffer(lua_State* L, const char* buffer, size_t size, const char* name) LUA_CXX_NOEXCEPT {
    if (L == nullptr || (buffer == nullptr && size != 0)) {
        return LUA_ERRSYNTAX;
    }

    const int base = lua_gettop(L);
    Lua::LuaState* state = fromC(L);
    const Lua::usize savedTop = state->getAbsoluteTop();
    auto failure = [state, savedTop](const Lua::Value& errorValue, int status) noexcept {
        return publishApiStatusError(state, savedTop, errorValue, status);
    };
    return apiStatusBoundary(
        state,
        [&]() {
            pushInternalCFunction(state, Lua::luaB_loadstring);
            lua_pushlstring(L, buffer != nullptr ? buffer : "", size);
            lua_pushstring(L, name != nullptr ? name : "=(loadbuffer)");
            return normalizeLoadResult(L, base, lua_pcall(L, 2, LUA_MULTRET, 0), LUA_ERRSYNTAX);
        },
        failure);
}

int luaL_loadstring(lua_State* L, const char* source) LUA_CXX_NOEXCEPT {
    return luaL_loadbuffer(L, source != nullptr ? source : "", source != nullptr ? std::strlen(source) : 0,
                           source != nullptr ? source : "");
}

int luaL_loadfile(lua_State* L, const char* filename) LUA_CXX_NOEXCEPT {
    if (L == nullptr) {
        return LUA_ERRFILE;
    }

    const int base = lua_gettop(L);
    Lua::LuaState* state = fromC(L);
    const Lua::usize savedTop = state->getAbsoluteTop();
    auto failure = [state, savedTop](const Lua::Value& errorValue, int status) noexcept {
        return publishApiStatusError(state, savedTop, errorValue, status);
    };
    return apiStatusBoundary(
        state,
        [&]() {
            pushInternalCFunction(state, Lua::luaB_loadfile);
            if (filename == nullptr) {
                lua_pushnil(L);
            } else {
                lua_pushstring(L, filename);
            }
            const int callStatus = lua_pcall(L, 1, LUA_MULTRET, 0);
            int failureStatus = LUA_ERRSYNTAX;
            if (callStatus == LUA_OK && lua_gettop(L) >= base + 2 && lua_isnil(L, base + 1)) {
                const char* message = lua_tostring(L, base + 2);
                if (message != nullptr && (std::strstr(message, "cannot open") != nullptr ||
                                           std::strstr(message, "cannot read") != nullptr)) {
                    failureStatus = LUA_ERRFILE;
                }
            }
            return normalizeLoadResult(L, base, callStatus, failureStatus);
        },
        failure);
}

int lua_load(lua_State* L, lua_Reader reader, void* data, const char* chunkname) LUA_CXX_NOEXCEPT {
    if (L == nullptr || reader == nullptr) {
        return LUA_ERRSYNTAX;
    }

    Lua::LuaState* state = fromC(L);
    const Lua::usize savedTop = state->getAbsoluteTop();
    auto failure = [state, savedTop](const Lua::Value& errorValue, int status) noexcept {
        return publishApiStatusError(state, savedTop, errorValue, status);
    };
    return apiStatusBoundary(
        state,
        [&]() {
            std::string source;
            for (;;) {
                size_t size = 0;
                const char* piece = reader(L, data, &size);
                if (piece == nullptr || size == 0) {
                    break;
                }
                source.append(piece, size);
            }
            return luaL_loadbuffer(L, source.data(), source.size(), chunkname != nullptr ? chunkname : "=(load)");
        },
        failure);
}

int lua_dump(lua_State* L, lua_Writer writer, void* data) LUA_CXX_NOEXCEPT {
    if (L == nullptr || writer == nullptr || !lua_isfunction(L, -1) || lua_iscfunction(L, -1)) {
        return 1;
    }

    const int base = lua_gettop(L);
    Lua::LuaState* state = fromC(L);
    const Lua::usize savedTop = state->getAbsoluteTop();
    auto failure = [state, savedTop](const Lua::Value&, int status) noexcept {
        restoreAbsoluteTopClearingSlots(state, savedTop);
        return status == LUA_ERRMEM ? LUA_ERRMEM : 1;
    };
    return apiStatusBoundary(
        state,
        [&]() {
            pushInternalCFunction(state, Lua::str_dump);
            lua_pushvalue(L, base);
            const int status = lua_pcall(L, 1, 1, 0);
            if (status != LUA_OK) {
                lua_settop(L, base);
                return status;
            }

            size_t size = 0;
            const char* bytes = lua_tolstring(L, -1, &size);
            const int writerStatus = bytes != nullptr ? writer(L, bytes, size, data) : 1;
            lua_settop(L, base);
            return writerStatus;
        },
        failure);
}

int luaL_ref(lua_State* L, int tableIndex) LUA_CXX_MAY_THROW {
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return LUA_REFNIL;
    }

    const int absoluteTable =
        tableIndex < 0 && tableIndex > LUA_REGISTRYINDEX ? lua_gettop(L) + tableIndex + 1 : tableIndex;
    lua_rawgeti(L, absoluteTable, 0);
    int reference = static_cast<int>(lua_tonumber(L, -1));
    lua_pop(L, 1);

    if (reference != 0) {
        lua_rawgeti(L, absoluteTable, reference);
        lua_rawseti(L, absoluteTable, 0);
    } else {
        reference = static_cast<int>(lua_objlen(L, absoluteTable)) + 1;
    }
    lua_rawseti(L, absoluteTable, reference);
    return reference;
}

void luaL_unref(lua_State* L, int tableIndex, int reference) LUA_CXX_MAY_THROW {
    if (reference < 0) {
        return;
    }

    const int absoluteTable =
        tableIndex < 0 && tableIndex > LUA_REGISTRYINDEX ? lua_gettop(L) + tableIndex + 1 : tableIndex;
    lua_rawgeti(L, absoluteTable, 0);
    lua_rawseti(L, absoluteTable, reference);
    lua_pushinteger(L, reference);
    lua_rawseti(L, absoluteTable, 0);
}

void luaL_openlibs(lua_State* L) LUA_CXX_MAY_THROW {
    Lua::StandardLibrary::openAll(fromC(L));
}

int luaL_error(lua_State* L, const char* fmt, ...) LUA_CXX_MAY_THROW {
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buffer, sizeof(buffer), fmt ? fmt : "", args);
    va_end(args);
    fromC(L)->error(buffer);
}

int luaL_argerror(lua_State* L, int, const char* extramsg) LUA_CXX_MAY_THROW {
    fromC(L)->error(extramsg ? extramsg : "bad argument");
}

void luaL_argcheck(lua_State* L, int cond, int narg, const char* extramsg) LUA_CXX_MAY_THROW {
    if (!cond) {
        luaL_argerror(L, narg, extramsg);
    }
}

lua_Number luaL_checknumber(lua_State* L, int narg) LUA_CXX_MAY_THROW {
    if (lua_isnumber(L, narg) == 0) {
        luaL_argerror(L, narg, "number expected");
    }
    return lua_tonumber(L, narg);
}

int luaL_checkint(lua_State* L, int narg) LUA_CXX_MAY_THROW {
    return static_cast<int>(luaL_checknumber(L, narg));
}

const char* luaL_checklstring(lua_State* L, int narg, size_t* len) LUA_CXX_MAY_THROW {
    const char* text = lua_tolstring(L, narg, len);
    if (text == nullptr) {
        fromC(L)->error("string expected");
    }
    return text;
}

const char* luaL_checkstring(lua_State* L, int narg) LUA_CXX_MAY_THROW {
    return luaL_checklstring(L, narg, nullptr);
}
}
