#include "lauxlib.h"

#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm_handlers/vm_diagnostics.hpp"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

Lua::LuaState* fromAuxState(lua_State* state) noexcept {
    return reinterpret_cast<Lua::LuaState*>(state);
}

int absoluteIndex(lua_State* state, int index) {
    return index > 0 || index <= LUA_REGISTRYINDEX ? index : lua_gettop(state) + index + 1;
}

std::string chunkId(Lua::StrView source) {
    if (!source.empty() && source.front() == '=') {
        return std::string(source.substr(1));
    }
    if (!source.empty() && source.front() == '@') {
        return std::string(source.substr(1));
    }

    constexpr std::size_t kSnippetLimit = 60;
    std::string snippet;
    bool truncated = false;
    for (char ch : source) {
        if (ch == '\n' || ch == '\r' || snippet.size() >= kSnippetLimit) {
            truncated = true;
            break;
        }
        if (ch == '"' || ch == '\\') {
            snippet.push_back('\\');
        }
        snippet.push_back(ch);
    }
    if (truncated) {
        snippet += "...";
    }
    return "[string \"" + snippet + "\"]";
}

std::string formatMessage(const char* format, va_list arguments) {
    const char* effectiveFormat = format != nullptr ? format : "";
    va_list countArguments;
    va_copy(countArguments, arguments);
    const int required = std::vsnprintf(nullptr, 0, effectiveFormat, countArguments);
    va_end(countArguments);
    if (required <= 0) {
        return required == 0 ? std::string() : std::string(effectiveFormat);
    }

    std::vector<char> buffer(static_cast<std::size_t>(required) + 1U);
    (void)std::vsnprintf(buffer.data(), buffer.size(), effectiveFormat, arguments);
    return std::string(buffer.data(), static_cast<std::size_t>(required));
}

struct AuxiliaryFunctionInfo {
    std::string name = "?";
    bool method = false;
};

AuxiliaryFunctionInfo currentAuxiliaryFunction(lua_State* stateHandle) {
    Lua::LuaState* state = fromAuxState(stateHandle);
    if (state == nullptr || state->getCurrentCI() == 0) {
        return {};
    }

    const Lua::CallInfo& current = state->getCallStack()[state->getCurrentCI()];
    const Lua::CallInfo& caller = state->getCallStack()[state->getCurrentCI() - 1];
    const Lua::Stack& stack = state->getStack();
    if (caller.func >= stack.size() || !stack[caller.func].isFunction()) {
        return {};
    }

    Lua::Function* callerFunction = stack[caller.func].asFunction();
    Lua::Proto* proto = callerFunction != nullptr ? callerFunction->getProto() : nullptr;
    if (proto == nullptr || caller.savedpc == nullptr || current.func < caller.base) {
        return {};
    }

    const auto code = proto->getInstructionSpan();
    const Lua::usize nextPc = static_cast<Lua::usize>(caller.savedpc - code.data());
    const Lua::usize instructionPc = nextPc > 0 ? nextPc - 1 : 0;
    const Lua::i32 functionRegister = static_cast<Lua::i32>(current.func - caller.base);
    const Lua::Opt<Lua::Str> description =
        Lua::VM::handlers::diagnostics::describeRegister(proto, functionRegister, instructionPc);
    if (!description.has_value()) {
        return {};
    }

    AuxiliaryFunctionInfo info;
    info.method = description->starts_with("method '");
    const std::size_t firstQuote = description->find('\'');
    const std::size_t lastQuote = description->rfind('\'');
    if (firstQuote != std::string::npos && lastQuote > firstQuote) {
        info.name.assign(description->data() + firstQuote + 1, lastQuote - firstQuote - 1);
    }
    return info;
}

std::size_t bufferLength(const luaL_Buffer* buffer) {
    return static_cast<std::size_t>(buffer->p - buffer->buffer);
}

std::size_t bufferFree(const luaL_Buffer* buffer) {
    return static_cast<std::size_t>(LUAL_BUFFERSIZE) - bufferLength(buffer);
}

int emptyBuffer(luaL_Buffer* buffer) {
    const std::size_t length = bufferLength(buffer);
    if (length == 0) {
        return 0;
    }
    lua_pushlstring(buffer->L, buffer->buffer, length);
    buffer->p = buffer->buffer;
    ++buffer->lvl;
    return 1;
}

void adjustStack(luaL_Buffer* buffer) {
    constexpr int kLimit = LUA_MINSTACK / 2;
    if (buffer->lvl <= 1) {
        return;
    }

    lua_State* state = buffer->L;
    int toGet = 1;
    std::size_t topLength = lua_objlen(state, -1);
    do {
        const std::size_t length = lua_objlen(state, -(toGet + 1));
        if (buffer->lvl - toGet + 1 >= kLimit || topLength > length) {
            topLength += length;
            ++toGet;
        } else {
            break;
        }
    } while (toGet < buffer->lvl);

    lua_concat(state, toGet);
    buffer->lvl = buffer->lvl - toGet + 1;
}

} // namespace

extern "C" {

void luaL_where(lua_State* L, int level) LUA_CXX_MAY_THROW {
    Lua::LuaState* state = fromAuxState(L);
    if (state != nullptr && level >= 0 && static_cast<Lua::usize>(level) <= state->getCurrentCI()) {
        const Lua::usize frameIndex = state->getCurrentCI() - static_cast<Lua::usize>(level);
        const Lua::CallInfo& call = state->getCallStack()[frameIndex];
        const Lua::Stack& stack = state->getStack();
        if (call.func < stack.size() && stack[call.func].isFunction()) {
            Lua::Function* function = stack[call.func].asFunction();
            Lua::Proto* proto = function != nullptr ? function->getProto() : nullptr;
            if (proto != nullptr && call.savedpc != nullptr) {
                const auto code = proto->getInstructionSpan();
                const Lua::usize pc = static_cast<Lua::usize>(call.savedpc - code.data());
                const Lua::i32 line = proto->getLine(pc > 0 ? pc - 1 : 0);
                const Lua::StrView source =
                    proto->getSource() != nullptr ? proto->getSource()->view() : Lua::StrView("=?");
                const std::string location = chunkId(source) + ":" + std::to_string(line) + ": ";
                lua_pushlstring(L, location.data(), location.size());
                return;
            }
        }
    }
    lua_pushstring(L, "");
}

int luaL_error(lua_State* L, const char* format, ...) LUA_CXX_MAY_THROW {
    va_list arguments;
    va_start(arguments, format);
    const std::string message = formatMessage(format, arguments);
    va_end(arguments);

    luaL_where(L, 1);
    lua_pushlstring(L, message.data(), message.size());
    lua_concat(L, 2);
    return lua_error(L);
}

int luaL_argerror(lua_State* L, int narg, const char* extraMessage) LUA_CXX_MAY_THROW {
    const char* detail = extraMessage != nullptr ? extraMessage : "bad argument";
    if (fromAuxState(L)->getCurrentCI() == 0) {
        return luaL_error(L, "bad argument #%d (%s)", narg, detail);
    }

    AuxiliaryFunctionInfo function = currentAuxiliaryFunction(L);
    if (function.method) {
        --narg;
        if (narg == 0) {
            return luaL_error(L, "calling '%s' on bad self (%s)", function.name.c_str(), detail);
        }
    }
    return luaL_error(L, "bad argument #%d to '%s' (%s)", narg, function.name.c_str(), detail);
}

int luaL_typerror(lua_State* L, int narg, const char* typeName) LUA_CXX_MAY_THROW {
    const char* expected = typeName != nullptr ? typeName : "value";
    const char* actual = lua_typename(L, lua_type(L, narg));
    const std::string message = std::string(expected) + " expected, got " + (actual != nullptr ? actual : "no value");
    return luaL_argerror(L, narg, message.c_str());
}

void luaL_argcheck(lua_State* L, int condition, int narg, const char* extraMessage) LUA_CXX_MAY_THROW {
    if (condition == 0) {
        (void)luaL_argerror(L, narg, extraMessage);
    }
}

const char* luaL_checklstring(lua_State* L, int narg, size_t* length) LUA_CXX_MAY_THROW {
    const char* text = lua_tolstring(L, narg, length);
    if (text == nullptr) {
        (void)luaL_typerror(L, narg, "string");
    }
    return text;
}

const char* luaL_optlstring(lua_State* L, int narg, const char* defaultValue, size_t* length) LUA_CXX_MAY_THROW {
    if (lua_isnoneornil(L, narg)) {
        if (length != nullptr) {
            *length = defaultValue != nullptr ? std::strlen(defaultValue) : 0;
        }
        return defaultValue;
    }
    return luaL_checklstring(L, narg, length);
}

const char* luaL_checkstring(lua_State* L, int narg) LUA_CXX_MAY_THROW {
    return luaL_checklstring(L, narg, nullptr);
}

lua_Number luaL_checknumber(lua_State* L, int narg) LUA_CXX_MAY_THROW {
    const lua_Number number = lua_tonumber(L, narg);
    if (number == 0 && lua_isnumber(L, narg) == 0) {
        (void)luaL_typerror(L, narg, "number");
    }
    return number;
}

lua_Number luaL_optnumber(lua_State* L, int narg, lua_Number defaultValue) LUA_CXX_MAY_THROW {
    return lua_isnoneornil(L, narg) ? defaultValue : luaL_checknumber(L, narg);
}

lua_Integer luaL_checkinteger(lua_State* L, int narg) LUA_CXX_MAY_THROW {
    const lua_Integer integer = lua_tointeger(L, narg);
    if (integer == 0 && lua_isnumber(L, narg) == 0) {
        (void)luaL_typerror(L, narg, "number");
    }
    return integer;
}

lua_Integer luaL_optinteger(lua_State* L, int narg, lua_Integer defaultValue) LUA_CXX_MAY_THROW {
    return lua_isnoneornil(L, narg) ? defaultValue : luaL_checkinteger(L, narg);
}

int luaL_checkint(lua_State* L, int narg) LUA_CXX_MAY_THROW {
    return static_cast<int>(luaL_checkinteger(L, narg));
}

void luaL_checkstack(lua_State* L, int size, const char* message) LUA_CXX_MAY_THROW {
    if (lua_checkstack(L, size) == 0) {
        (void)luaL_error(L, "stack overflow (%s)", message != nullptr ? message : "");
    }
}

void luaL_checktype(lua_State* L, int narg, int type) LUA_CXX_MAY_THROW {
    if (lua_type(L, narg) != type) {
        (void)luaL_typerror(L, narg, lua_typename(L, type));
    }
}

void luaL_checkany(lua_State* L, int narg) LUA_CXX_MAY_THROW {
    if (lua_type(L, narg) == LUA_TNONE) {
        (void)luaL_argerror(L, narg, "value expected");
    }
}

int luaL_newmetatable(lua_State* L, const char* typeName) LUA_CXX_MAY_THROW {
    lua_getfield(L, LUA_REGISTRYINDEX, typeName);
    if (!lua_isnil(L, -1)) {
        return 0;
    }
    lua_pop(L, 1);
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_setfield(L, LUA_REGISTRYINDEX, typeName);
    return 1;
}

void* luaL_checkudata(lua_State* L, int narg, const char* typeName) LUA_CXX_MAY_THROW {
    void* payload = lua_touserdata(L, narg);
    if (payload != nullptr && lua_getmetatable(L, narg) != 0) {
        lua_getfield(L, LUA_REGISTRYINDEX, typeName);
        if (lua_rawequal(L, -1, -2) != 0) {
            lua_pop(L, 2);
            return payload;
        }
    }
    (void)luaL_typerror(L, narg, typeName);
    return nullptr;
}

int luaL_checkoption(lua_State* L, int narg, const char* defaultValue, const char* const options[]) LUA_CXX_MAY_THROW {
    const char* selected = defaultValue != nullptr ? luaL_optstring(L, narg, defaultValue) : luaL_checkstring(L, narg);
    if (options != nullptr) {
        for (int index = 0; options[index] != nullptr; ++index) {
            if (std::strcmp(options[index], selected) == 0) {
                return index;
            }
        }
    }
    return luaL_argerror(L, narg, (std::string("invalid option '") + selected + "'").c_str());
}

int luaL_getmetafield(lua_State* L, int objectIndex, const char* event) LUA_CXX_MAY_THROW {
    if (lua_getmetatable(L, objectIndex) == 0) {
        return 0;
    }
    lua_pushstring(L, event);
    lua_rawget(L, -2);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 2);
        return 0;
    }
    lua_remove(L, -2);
    return 1;
}

int luaL_callmeta(lua_State* L, int objectIndex, const char* event) LUA_CXX_MAY_THROW {
    const int object = absoluteIndex(L, objectIndex);
    if (luaL_getmetafield(L, object, event) == 0) {
        return 0;
    }
    lua_pushvalue(L, object);
    lua_call(L, 1, 1);
    return 1;
}

const char* luaL_findtable(lua_State* L, int tableIndex, const char* fieldName, int sizeHint) LUA_CXX_MAY_THROW {
    const char* current = fieldName != nullptr ? fieldName : "";
    lua_pushvalue(L, tableIndex);
    for (;;) {
        const char* separator = std::strchr(current, '.');
        const char* end = separator != nullptr ? separator : current + std::strlen(current);
        lua_pushlstring(L, current, static_cast<size_t>(end - current));
        lua_rawget(L, -2);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            lua_createtable(L, 0, separator != nullptr ? 1 : sizeHint);
            lua_pushlstring(L, current, static_cast<size_t>(end - current));
            lua_pushvalue(L, -2);
            lua_settable(L, -4);
        } else if (!lua_istable(L, -1)) {
            lua_pop(L, 2);
            return current;
        }
        lua_remove(L, -2);
        if (separator == nullptr) {
            return nullptr;
        }
        current = separator + 1;
    }
}

void luaL_openlib(lua_State* L, const char* libraryName, const luaL_Reg* functions,
                  int upvalueCount) LUA_CXX_MAY_THROW {
    if (libraryName != nullptr) {
        int functionCount = 0;
        if (functions != nullptr) {
            for (const luaL_Reg* function = functions; function->name != nullptr; ++function) {
                ++functionCount;
            }
        }

        (void)luaL_findtable(L, LUA_REGISTRYINDEX, "_LOADED", 1);
        lua_getfield(L, -1, libraryName);
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            const char* conflict = luaL_findtable(L, LUA_GLOBALSINDEX, libraryName, functionCount);
            if (conflict != nullptr) {
                (void)luaL_error(L, "name conflict for module '%s'", libraryName);
            }
            lua_pushvalue(L, -1);
            lua_setfield(L, -3, libraryName);
        }
        lua_remove(L, -2);
        lua_insert(L, -(upvalueCount + 1));
    }

    if (functions != nullptr) {
        for (const luaL_Reg* function = functions; function->name != nullptr; ++function) {
            for (int index = 0; index < upvalueCount; ++index) {
                lua_pushvalue(L, -upvalueCount);
            }
            lua_pushcclosure(L, function->func, upvalueCount);
            lua_setfield(L, -(upvalueCount + 2), function->name);
        }
    }
    lua_pop(L, upvalueCount);
}

void luaL_register(lua_State* L, const char* libraryName, const luaL_Reg* functions) LUA_CXX_MAY_THROW {
    luaL_openlib(L, libraryName, functions, 0);
}

lua_State* luaL_newstate(void) LUA_CXX_MAY_THROW {
    return lua_open();
}

const char* luaL_gsub(lua_State* L, const char* source, const char* pattern,
                      const char* replacement) LUA_CXX_MAY_THROW {
    const char* current = source != nullptr ? source : "";
    const char* needle = pattern != nullptr ? pattern : "";
    const char* substitute = replacement != nullptr ? replacement : "";
    if (*needle == '\0') {
        lua_pushstring(L, current);
        return lua_tostring(L, -1);
    }

    luaL_Buffer buffer;
    luaL_buffinit(L, &buffer);
    const std::size_t needleLength = std::strlen(needle);
    const char* match = nullptr;
    while ((match = std::strstr(current, needle)) != nullptr) {
        luaL_addlstring(&buffer, current, static_cast<size_t>(match - current));
        luaL_addstring(&buffer, substitute);
        current = match + needleLength;
    }
    luaL_addstring(&buffer, current);
    luaL_pushresult(&buffer);
    return lua_tostring(L, -1);
}

void luaL_buffinit(lua_State* L, luaL_Buffer* buffer) LUA_CXX_MAY_THROW {
    buffer->L = L;
    buffer->p = buffer->buffer;
    buffer->lvl = 0;
}

char* luaL_prepbuffer(luaL_Buffer* buffer) LUA_CXX_MAY_THROW {
    if (emptyBuffer(buffer) != 0) {
        adjustStack(buffer);
    }
    return buffer->buffer;
}

void luaL_addlstring(luaL_Buffer* buffer, const char* text, size_t length) LUA_CXX_MAY_THROW {
    const char* current = text;
    std::size_t remaining = length;
    while (remaining != 0) {
        if (bufferFree(buffer) == 0) {
            (void)luaL_prepbuffer(buffer);
        }
        const std::size_t count = std::min(remaining, bufferFree(buffer));
        std::memcpy(buffer->p, current, count);
        buffer->p += count;
        current += count;
        remaining -= count;
    }
}

void luaL_addstring(luaL_Buffer* buffer, const char* text) LUA_CXX_MAY_THROW {
    const char* effectiveText = text != nullptr ? text : "";
    luaL_addlstring(buffer, effectiveText, std::strlen(effectiveText));
}

void luaL_addvalue(luaL_Buffer* buffer) LUA_CXX_MAY_THROW {
    size_t valueLength = 0;
    const char* value = lua_tolstring(buffer->L, -1, &valueLength);
    if (value == nullptr) {
        (void)luaL_error(buffer->L, "string expected");
    }
    if (valueLength <= bufferFree(buffer)) {
        std::memcpy(buffer->p, value, valueLength);
        buffer->p += valueLength;
        lua_pop(buffer->L, 1);
        return;
    }

    if (emptyBuffer(buffer) != 0) {
        lua_insert(buffer->L, -2);
    }
    ++buffer->lvl;
    adjustStack(buffer);
}

void luaL_pushresult(luaL_Buffer* buffer) LUA_CXX_MAY_THROW {
    (void)emptyBuffer(buffer);
    lua_concat(buffer->L, buffer->lvl);
    buffer->lvl = 1;
}

} // extern "C"
