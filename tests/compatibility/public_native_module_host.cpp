#include "lauxlib.h"
#include "lualib.h"
#include "runtime/runtime_services.hpp"
#include "vm/state/lua_state.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

struct FixtureResult {
    int stateCalls;
    int moduleCalls;
};

lua_State* apiState(Lua::LuaState* state) {
    return reinterpret_cast<lua_State*>(state);
}

std::runtime_error apiFailure(lua_State* state, const char* operation) {
    const char* detail = lua_tostring(state, -1);
    std::string message = operation;
    message += ": ";
    message += detail != nullptr ? detail : "unknown Lua error";
    lua_settop(state, 0);
    return std::runtime_error(message);
}

int tableInteger(lua_State* state, const char* field) {
    lua_pushstring(state, field);
    lua_gettable(state, -2);
    if (!lua_isnumber(state, -1)) {
        throw std::runtime_error(std::string("native module returned a non-numeric ") + field);
    }
    const int result = static_cast<int>(lua_tonumber(state, -1));
    lua_pop(state, 1);
    return result;
}

FixtureResult callFixture(lua_State* state, const char* modulePath) {
    lua_settop(state, 0);
    lua_getglobal(state, "package");
    lua_pushstring(state, "loadlib");
    lua_gettable(state, -2);
    lua_remove(state, -2);
    lua_pushstring(state, modulePath);
    lua_pushstring(state, "luaopen_publicfixture");
    if (lua_pcall(state, 2, 1, 0) != LUA_OK) {
        throw apiFailure(state, "package.loadlib");
    }
    if (!lua_isfunction(state, -1)) {
        throw std::runtime_error("package.loadlib did not return a function");
    }

    lua_pushstring(state, "publicfixture");
    if (lua_pcall(state, 1, 1, 0) != LUA_OK) {
        throw apiFailure(state, "luaopen_publicfixture");
    }
    if (!lua_istable(state, -1)) {
        throw std::runtime_error("luaopen_publicfixture did not return a table");
    }

    lua_pushstring(state, "source");
    lua_gettable(state, -2);
    const char* source = lua_tostring(state, -1);
    if (source == nullptr || std::string(source) != "public-lua-h-only") {
        throw std::runtime_error("native module returned the wrong source marker");
    }
    lua_pop(state, 1);

    const int stateCalls = tableInteger(state, "state_calls");
    const int moduleCalls = tableInteger(state, "module_calls");
    lua_settop(state, 0);
    return {stateCalls, moduleCalls};
}

std::string moduleSearchName(const char* modulePath) {
    const std::string filename = std::filesystem::path(modulePath).filename().string();
#ifdef __APPLE__
    return "@executable_path/" + filename;
#else
    return filename;
#endif
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: lua_public_module_host <module-path>\n";
        return 2;
    }

    try {
        const std::string searchName = moduleSearchName(argv[1]);
        {
            Lua::EngineContext survivorContext;
            Lua::UPtr<Lua::LuaState> survivor = Lua::LuaState::create(survivorContext);
            luaL_openlibs(apiState(survivor.get()));

            {
                Lua::EngineContext firstContext;
                Lua::UPtr<Lua::LuaState> first = Lua::LuaState::create(firstContext);
                luaL_openlibs(apiState(first.get()));

                const FixtureResult firstCall = callFixture(apiState(first.get()), searchName.c_str());
                const FixtureResult firstAgain = callFixture(apiState(first.get()), argv[1]);
                if (firstCall.stateCalls != 1 || firstCall.moduleCalls != 1 || firstAgain.stateCalls != 2 ||
                    firstAgain.moduleCalls != 2) {
                    throw std::runtime_error("first EngineContext did not keep isolated module state");
                }

                const FixtureResult survivorCall = callFixture(apiState(survivor.get()), argv[1]);
                if (survivorCall.stateCalls != 1 || survivorCall.moduleCalls != 3) {
                    throw std::runtime_error("second EngineContext did not start with isolated Lua state");
                }
                if (firstContext.nativeModules().loadedCount() != 1 ||
                    !firstContext.nativeModules().contains(searchName) ||
                    !firstContext.nativeModules().contains(argv[1]) ||
                    survivorContext.nativeModules().loadedCount() != 1) {
                    throw std::runtime_error("module search-name alias created a second context lease");
                }
            }

            const FixtureResult afterFirstClose = callFixture(apiState(survivor.get()), argv[1]);
            if (afterFirstClose.stateCalls != 2 || afterFirstClose.moduleCalls != 4 ||
                survivorContext.nativeModules().loadedCount() != 1) {
                throw std::runtime_error("surviving EngineContext lost its module lease or state");
            }
        }

        Lua::EngineContext reloadedContext;
        Lua::UPtr<Lua::LuaState> reloaded = Lua::LuaState::create(reloadedContext);
        luaL_openlibs(apiState(reloaded.get()));
        const FixtureResult afterLastClose = callFixture(apiState(reloaded.get()), argv[1]);
        if (afterLastClose.stateCalls != 1 || afterLastClose.moduleCalls != 1 ||
            reloadedContext.nativeModules().loadedCount() != 1) {
            throw std::runtime_error("last EngineContext close did not unload and reset the native module");
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    return 0;
}
