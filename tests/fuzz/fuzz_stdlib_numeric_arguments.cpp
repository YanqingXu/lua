#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace {

void pushLibraryFunction(lua_State* state, const char* library, const char* function) {
    lua_getglobal(state, library);
    lua_getfield(state, -1, function);
    lua_remove(state, -2);
}

void pushNumericTable(lua_State* state) {
    lua_createtable(state, 3, 0);
    for (int index = 1; index <= 3; ++index) {
        lua_pushnumber(state, static_cast<lua_Number>(index));
        lua_rawseti(state, -2, index);
    }
}

void invoke(lua_State* state, std::uint8_t operation, lua_Number first, lua_Number second) {
    int argumentCount = 0;
    switch (operation % 16U) {
        case 0:
            pushLibraryFunction(state, "string", "sub");
            lua_pushliteral(state, "abcdef");
            lua_pushnumber(state, first);
            lua_pushnumber(state, second);
            argumentCount = 3;
            break;
        case 1:
            pushLibraryFunction(state, "string", "rep");
            lua_pushliteral(state, "x");
            lua_pushnumber(state, first);
            argumentCount = 2;
            break;
        case 2:
            pushLibraryFunction(state, "string", "byte");
            lua_pushliteral(state, "abcdef");
            lua_pushnumber(state, first);
            lua_pushnumber(state, second);
            argumentCount = 3;
            break;
        case 3:
            pushLibraryFunction(state, "string", "char");
            lua_pushnumber(state, first);
            lua_pushnumber(state, second);
            argumentCount = 2;
            break;
        case 4:
            pushLibraryFunction(state, "string", "find");
            lua_pushliteral(state, "abcdef");
            lua_pushliteral(state, "b");
            lua_pushnumber(state, first);
            lua_pushboolean(state, 1);
            argumentCount = 4;
            break;
        case 5:
            pushLibraryFunction(state, "string", "format");
            lua_pushliteral(state, "%*.*f");
            lua_pushnumber(state, first);
            lua_pushnumber(state, second);
            lua_pushnumber(state, 1.0);
            argumentCount = 4;
            break;
        case 6:
            pushLibraryFunction(state, "table", "insert");
            pushNumericTable(state);
            lua_pushnumber(state, first);
            lua_pushnumber(state, second);
            argumentCount = 3;
            break;
        case 7:
            pushLibraryFunction(state, "table", "remove");
            pushNumericTable(state);
            lua_pushnumber(state, first);
            argumentCount = 2;
            break;
        case 8:
            pushLibraryFunction(state, "table", "concat");
            pushNumericTable(state);
            lua_pushliteral(state, ",");
            lua_pushnumber(state, first);
            lua_pushnumber(state, second);
            argumentCount = 4;
            break;
        case 9:
            pushLibraryFunction(state, "table", "unpack");
            pushNumericTable(state);
            lua_pushnumber(state, first);
            lua_pushnumber(state, second);
            argumentCount = 3;
            break;
        case 10:
            pushLibraryFunction(state, "math", "random");
            lua_pushnumber(state, first);
            lua_pushnumber(state, second);
            argumentCount = 2;
            break;
        case 11:
            pushLibraryFunction(state, "math", "randomseed");
            lua_pushnumber(state, first);
            argumentCount = 1;
            break;
        case 12:
            lua_getglobal(state, "select");
            lua_pushnumber(state, first);
            lua_pushnumber(state, 1.0);
            lua_pushnumber(state, 2.0);
            argumentCount = 3;
            break;
        case 13:
            lua_getglobal(state, "collectgarbage");
            lua_pushliteral(state, "step");
            lua_pushnumber(state, first);
            argumentCount = 2;
            break;
        case 14:
            pushLibraryFunction(state, "debug", "getlocal");
            lua_pushnumber(state, first);
            lua_pushnumber(state, second);
            argumentCount = 2;
            break;
        default:
            pushLibraryFunction(state, "math", "ldexp");
            lua_pushnumber(state, first);
            lua_pushnumber(state, second);
            argumentCount = 2;
            break;
    }
    (void)lua_pcall(state, argumentCount, LUA_MULTRET, 0);
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    if (data == nullptr || size < 1) {
        return 0;
    }

    lua_Number first = 0.0;
    lua_Number second = 0.0;
    if (size >= 1 + sizeof(first)) {
        std::memcpy(&first, data + 1, sizeof(first));
    }
    if (size >= 1 + sizeof(first) + sizeof(second)) {
        std::memcpy(&second, data + 1 + sizeof(first), sizeof(second));
    }

    lua_State* state = nullptr;
    try {
        state = lua_newstate(nullptr, nullptr);
        if (state == nullptr) {
            return 0;
        }
        luaL_openlibs(state);
        invoke(state, data[0], first, second);
        lua_settop(state, 0);
    } catch (...) {
        // Public callbacks may throw before the protected call is established;
        // fuzzer teardown must remain safe in that case too.
    }
    lua_close(state);
    return 0;
}
