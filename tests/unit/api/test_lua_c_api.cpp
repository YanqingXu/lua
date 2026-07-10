#include "../framework/test_framework.hpp"

#include "core/function.hpp"
#include "core/upvalue.hpp"
#include "lua.h"
#include "lualib.h"
#include "vm/state/lua_state.hpp"

#include <cstdint>

using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Lua C API";

int gApiFinalizerCalls = 0;
int gApiFinalizerPayload = 0;

int returnCapturedUpvalues(lua_State* L) {
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_pushvalue(L, lua_upvalueindex(2));
    return 2;
}

int incrementCapturedUpvalue(lua_State* L) {
    const lua_Number next = lua_tonumber(L, lua_upvalueindex(1)) + 1;
    lua_pushnumber(L, next);
    lua_replace(L, lua_upvalueindex(1));
    lua_pushvalue(L, lua_upvalueindex(1));
    return 1;
}

int removeMiddleArgument(lua_State* L) {
    lua_remove(L, 2);
    return lua_gettop(L);
}

int returnFirstUpvalue(lua_State* L) {
    lua_pushvalue(L, lua_upvalueindex(1));
    return 1;
}

int recordUserdataFinalizer(lua_State* L) {
    ++gApiFinalizerCalls;
    void* payload = lua_touserdata(L, 1);
    if (payload != nullptr && lua_objlen(L, 1) >= sizeof(int)) {
        gApiFinalizerPayload = *static_cast<int*>(payload);
    }
    return 0;
}

void testStackAndInvalidIndexes(TestSuite& suite) {
    lua_State* L = lua_open();

    lua_pushnumber(L, 10);
    lua_pushnumber(L, 20);

    ASSERT_EQ(suite, 2, lua_gettop(L), "stack top counts pushed values");
    ASSERT_EQ(suite, LUA_TNUMBER, lua_type(L, 1), "positive index resolves from frame base");
    ASSERT_EQ(suite, LUA_TNUMBER, lua_type(L, -1), "negative index resolves from stack top");
    ASSERT_EQ(suite, LUA_TNONE, lua_type(L, 3), "positive invalid index reports none");
    ASSERT_EQ(suite, LUA_TNONE, lua_type(L, -3), "negative invalid index reports none");

    lua_pushvalue(L, -2);
    ASSERT_EQ(suite, 10.0, lua_tonumber(L, -1), "pushvalue accepts negative index");
    lua_remove(L, 2);
    ASSERT_EQ(suite, 2, lua_gettop(L), "remove shrinks logical stack");
    ASSERT_EQ(suite, 10.0, lua_tonumber(L, 1), "remove preserves lower value");
    ASSERT_EQ(suite, 10.0, lua_tonumber(L, 2), "remove shifts upper value");

    lua_settop(L, 0);
    lua_pushnumber(L, 1);
    lua_pushnumber(L, 2);
    lua_pushnumber(L, 3);
    lua_insert(L, 1);
    ASSERT_EQ(suite, 3.0, lua_tonumber(L, 1), "insert moves top value to positive index");
    ASSERT_EQ(suite, 1.0, lua_tonumber(L, 2), "insert shifts existing values upward");

    lua_pushnumber(L, 9);
    lua_replace(L, -2);
    ASSERT_EQ(suite, 3, lua_gettop(L), "replace pops its source value");
    ASSERT_EQ(suite, 9.0, lua_tonumber(L, -1), "replace writes a negative stack index");

    lua_settop(L, 5);
    ASSERT_EQ(suite, 5, lua_gettop(L), "settop grows the API-visible stack");
    ASSERT_EQ(suite, LUA_TNIL, lua_type(L, 5), "settop fills new slots with nil");
    lua_settop(L, -3);
    ASSERT_EQ(suite, 3, lua_gettop(L), "negative settop shrinks relative to current top");

    lua_close(L);
}

void testRegistryAndGlobalsPseudoIndexes(TestSuite& suite) {
    lua_State* L = lua_open();

    ASSERT_EQ(suite, LUA_TTABLE, lua_type(L, LUA_REGISTRYINDEX),
              "registry pseudo-index exposes table");
    ASSERT_EQ(suite, LUA_TTABLE, lua_type(L, LUA_GLOBALSINDEX),
              "globals pseudo-index exposes table");

    lua_pushstring(L, "registry-value");
    lua_rawseti(L, LUA_REGISTRYINDEX, 37);
    lua_rawgeti(L, LUA_REGISTRYINDEX, 37);
    ASSERT_EQ(suite, std::string("registry-value"), std::string(lua_tostring(L, -1)),
              "registry supports raw integer access");
    lua_pop(L, 1);

    lua_pushstring(L, "api_pseudo_global");
    lua_pushnumber(L, 73);
    lua_settable(L, LUA_GLOBALSINDEX);
    lua_getglobal(L, "api_pseudo_global");
    ASSERT_EQ(suite, 73.0, lua_tonumber(L, -1),
              "globals pseudo-index shares the state global table");

    lua_close(L);
}

void testIndependentStateIsolation(TestSuite& suite) {
    lua_State* first = lua_open();
    lua_State* second = lua_open();

    lua_pushnumber(first, 17);
    lua_setglobal(first, "isolated_global");
    lua_getglobal(second, "isolated_global");
    ASSERT_EQ(suite, LUA_TNIL, lua_type(second, -1),
              "independent states do not share globals");
    lua_pop(second, 1);

    lua_pushstring(first, "first-registry");
    lua_rawseti(first, LUA_REGISTRYINDEX, 91);
    lua_rawgeti(second, LUA_REGISTRYINDEX, 91);
    ASSERT_EQ(suite, LUA_TNIL, lua_type(second, -1),
              "independent states do not share registries");

    auto* firstState = reinterpret_cast<Lua::LuaState*>(first);
    auto* secondState = reinterpret_cast<Lua::LuaState*>(second);
    ASSERT_TRUE(suite, &firstState->getGlobalState() != &secondState->getGlobalState(),
                "independent states own distinct global states");
    ASSERT_TRUE(suite, &firstState->getGlobalState().getStringPool() !=
                           &secondState->getGlobalState().getStringPool(),
                "independent states own distinct string pools");

    lua_close(second);
    lua_close(first);
}

void testCClosureCapturesUpvalues(TestSuite& suite) {
    lua_State* L = lua_open();

    lua_pushnumber(L, 11);
    lua_pushnumber(L, 22);
    lua_pushcclosure(L, returnCapturedUpvalues, 2);

    ASSERT_EQ(suite, 1, lua_gettop(L), "pushcclosure consumes captured values");
    ASSERT_TRUE(suite, lua_iscfunction(L, -1) != 0, "pushcclosure pushes a C function");

    lua_call(L, 0, 2);
    ASSERT_EQ(suite, 2, lua_gettop(L), "C closure returns requested results");
    ASSERT_EQ(suite, 11.0, lua_tonumber(L, 1), "first C closure upvalue keeps capture order");
    ASSERT_EQ(suite, 22.0, lua_tonumber(L, 2), "second C closure upvalue keeps capture order");

    lua_close(L);
}

void testCClosureUpvalueMutationPersists(TestSuite& suite) {
    lua_State* L = lua_open();

    lua_pushnumber(L, 5);
    lua_pushcclosure(L, incrementCapturedUpvalue, 1);
    lua_pushvalue(L, -1);
    lua_call(L, 0, 1);
    ASSERT_EQ(suite, 6.0, lua_tonumber(L, -1), "replace writes C closure pseudo-index");

    lua_pop(L, 1);
    lua_call(L, 0, 1);
    ASSERT_EQ(suite, 7.0, lua_tonumber(L, -1), "C closure upvalue mutation persists");

    lua_close(L);
}

void testStackRemovalInsideCFrame(TestSuite& suite) {
    lua_State* L = lua_open();

    lua_pushcclosure(L, removeMiddleArgument, 0);
    lua_pushnumber(L, 1);
    lua_pushnumber(L, 2);
    lua_pushnumber(L, 3);
    lua_call(L, 3, 2);

    ASSERT_EQ(suite, 2, lua_gettop(L), "C frame remove returns two values");
    ASSERT_EQ(suite, 1.0, lua_tonumber(L, 1), "C frame remove keeps first argument");
    ASSERT_EQ(suite, 3.0, lua_tonumber(L, 2), "C frame remove shifts third argument");

    lua_close(L);
}

void testCheckStackAndXMove(TestSuite& suite) {
    lua_State* parent = lua_open();
    auto* parentState = reinterpret_cast<Lua::LuaState*>(parent);
    Lua::LuaState* childState = Lua::LuaState::newThread(parentState);
    auto* child = reinterpret_cast<lua_State*>(childState);

    ASSERT_EQ(suite, 1, lua_checkstack(parent, 128), "checkstack accepts bounded growth");
    ASSERT_EQ(suite, 0, lua_checkstack(parent, -1), "checkstack rejects negative growth");
    ASSERT_EQ(suite, 0, lua_checkstack(parent, 1000000),
              "checkstack rejects growth beyond VM maximum");

    lua_pushnumber(parent, 10);
    lua_pushnumber(parent, 20);
    lua_xmove(parent, child, 2);
    ASSERT_EQ(suite, 0, lua_gettop(parent), "xmove removes values from source");
    ASSERT_EQ(suite, 2, lua_gettop(child), "xmove appends values to destination");
    ASSERT_EQ(suite, 10.0, lua_tonumber(child, 1), "xmove preserves first value order");
    ASSERT_EQ(suite, 20.0, lua_tonumber(child, 2), "xmove preserves second value order");

    lua_pushnumber(child, 30);
    lua_xmove(child, parent, 1);
    ASSERT_EQ(suite, 30.0, lua_tonumber(parent, 1), "xmove supports reverse movement");

    lua_State* independent = lua_open();
    bool rejected = false;
    try {
        lua_xmove(child, independent, 1);
    } catch (...) {
        rejected = true;
    }
    ASSERT_TRUE(suite, rejected, "xmove rejects independent states");
    ASSERT_EQ(suite, 2, lua_gettop(child), "rejected xmove preserves source stack");

    lua_close(independent);
    delete childState;
    lua_close(parent);
}

void testCClosureUpvalueIntrospection(TestSuite& suite) {
    lua_State* L = lua_open();

    lua_pushnumber(L, 41);
    lua_pushcclosure(L, returnFirstUpvalue, 1);

    const char* getName = lua_getupvalue(L, 1, 1);
    ASSERT_TRUE(suite, getName != nullptr, "getupvalue finds C closure upvalue");
    ASSERT_EQ(suite, std::string(""), std::string(getName),
              "C closure upvalue name is empty");
    ASSERT_EQ(suite, 41.0, lua_tonumber(L, -1), "getupvalue pushes captured value");
    lua_pop(L, 1);

    const int beforeInvalidGet = lua_gettop(L);
    ASSERT_TRUE(suite, lua_getupvalue(L, 1, 2) == nullptr,
                "getupvalue rejects invalid ordinal");
    ASSERT_EQ(suite, beforeInvalidGet, lua_gettop(L),
              "invalid getupvalue leaves stack unchanged");

    lua_pushnumber(L, 99);
    const char* setName = lua_setupvalue(L, 1, 1);
    ASSERT_TRUE(suite, setName != nullptr, "setupvalue finds C closure upvalue");
    ASSERT_EQ(suite, std::string(""), std::string(setName),
              "setupvalue reports empty C closure name");
    ASSERT_EQ(suite, 1, lua_gettop(L), "setupvalue consumes replacement value");

    lua_pushnumber(L, 123);
    ASSERT_TRUE(suite, lua_setupvalue(L, 1, 2) == nullptr,
                "setupvalue rejects invalid ordinal");
    ASSERT_EQ(suite, 2, lua_gettop(L), "invalid setupvalue keeps replacement value");
    lua_pop(L, 1);

    lua_call(L, 0, 1);
    ASSERT_EQ(suite, 99.0, lua_tonumber(L, 1), "setupvalue mutation reaches callback");

    lua_close(L);
}

void testLuaClosureUpvalueIntrospection(TestSuite& suite) {
    lua_State* L = lua_open();
    auto* state = reinterpret_cast<Lua::LuaState*>(L);
    auto& global = state->getGlobalState();

    Lua::Proto* proto = global.getGC().create<Lua::Proto>();
    proto->addUpvalueName(global.getStringPool().intern("captured"));
    Lua::Function* closure = global.getGC().create<Lua::Function>(proto);
    closure->setEnv(state->getGlobalTable());
    closure->addUpvalue(global.getGC().create<Lua::Upvalue>(Lua::Value(7.0)));
    state->pushFunction(closure);

    const char* getName = lua_getupvalue(L, -1, 1);
    ASSERT_TRUE(suite, getName != nullptr, "getupvalue finds Lua closure upvalue");
    ASSERT_EQ(suite, std::string("captured"), std::string(getName),
              "getupvalue returns Lua closure debug name");
    ASSERT_EQ(suite, 7.0, lua_tonumber(L, -1), "getupvalue pushes Lua closure value");
    lua_pop(L, 1);

    lua_pushnumber(L, 8);
    const char* setName = lua_setupvalue(L, 1, 1);
    ASSERT_TRUE(suite, setName != nullptr, "setupvalue finds Lua closure upvalue");
    ASSERT_EQ(suite, std::string("captured"), std::string(setName),
              "setupvalue returns Lua closure debug name");
    ASSERT_EQ(suite, 1, lua_gettop(L), "Lua setupvalue consumes replacement value");

    lua_getupvalue(L, 1, 1);
    ASSERT_EQ(suite, 8.0, lua_tonumber(L, -1), "Lua setupvalue persists replacement");

    lua_close(L);
}

void testLightAndFullUserdata(TestSuite& suite) {
    lua_State* L = lua_open();

    int lightPayload = 17;
    lua_pushlightuserdata(L, &lightPayload);
    ASSERT_EQ(suite, LUA_TLIGHTUSERDATA, lua_type(L, -1),
              "pushlightuserdata preserves Lua type");
    ASSERT_TRUE(suite, lua_isuserdata(L, -1) != 0,
                "isuserdata accepts light userdata");
    ASSERT_TRUE(suite, lua_touserdata(L, -1) == &lightPayload,
                "touserdata returns light pointer");
    ASSERT_EQ(suite, static_cast<size_t>(0), lua_objlen(L, -1),
              "light userdata has zero object length");
    lua_pop(L, 1);

    void* empty = lua_newuserdata(L, 0);
    ASSERT_TRUE(suite, empty != nullptr, "zero-size full userdata has stable address");
    ASSERT_EQ(suite, LUA_TUSERDATA, lua_type(L, -1),
              "newuserdata pushes full userdata");
    ASSERT_EQ(suite, static_cast<size_t>(0), lua_objlen(L, -1),
              "zero-size userdata reports requested size");
    void* secondEmpty = lua_newuserdata(L, 0);
    ASSERT_TRUE(suite, secondEmpty != empty, "zero-size userdata addresses stay distinct");
    lua_pop(L, 2);

    void* full = lua_newuserdata(L, sizeof(int));
    ASSERT_TRUE(suite, full != nullptr, "full userdata exposes payload buffer");
    ASSERT_TRUE(suite, reinterpret_cast<std::uintptr_t>(full) % 8 == 0,
                "full userdata payload is eight-byte aligned");
    ASSERT_TRUE(suite, lua_touserdata(L, -1) == full,
                "touserdata returns full payload pointer");
    ASSERT_EQ(suite, sizeof(int), lua_objlen(L, -1),
              "full userdata reports payload size");
    ASSERT_EQ(suite, 0, *static_cast<int*>(full), "full userdata starts zero initialized");
    *static_cast<int*>(full) = 73;
    ASSERT_EQ(suite, 73, *static_cast<int*>(lua_touserdata(L, -1)),
              "full userdata payload mutation persists");

    lua_newtable(L);
    lua_pushstring(L, "metatable-marker");
    lua_rawseti(L, -2, 1);
    ASSERT_EQ(suite, 1, lua_setmetatable(L, -2), "setmetatable accepts userdata");
    ASSERT_EQ(suite, 1, lua_getmetatable(L, -1), "getmetatable finds userdata table");
    lua_rawgeti(L, -1, 1);
    ASSERT_EQ(suite, std::string("metatable-marker"), std::string(lua_tostring(L, -1)),
              "userdata metatable round-trips through C API");
    lua_pop(L, 2);

    lua_pushnil(L);
    ASSERT_EQ(suite, 1, lua_setmetatable(L, -2), "nil removes userdata metatable");
    ASSERT_EQ(suite, 0, lua_getmetatable(L, -1), "removed userdata metatable stays absent");
    lua_pop(L, 1);

    const char embedded[] = {'a', '\0', 'b'};
    lua_pushlstring(L, embedded, sizeof(embedded));
    ASSERT_EQ(suite, sizeof(embedded), lua_objlen(L, -1),
              "objlen preserves embedded string bytes");
    lua_pop(L, 1);

    lua_newtable(L);
    lua_pushnumber(L, 1);
    lua_rawseti(L, -2, 1);
    lua_pushnumber(L, 2);
    lua_rawseti(L, -2, 2);
    ASSERT_EQ(suite, static_cast<size_t>(2), lua_objlen(L, -1),
              "objlen reports table sequence length");

    lua_close(L);
}

void testUserdataFinalizerThroughCApi(TestSuite& suite) {
    lua_State* L = lua_open();
    luaL_openlibs(L);
    gApiFinalizerCalls = 0;
    gApiFinalizerPayload = 0;

    void* payload = lua_newuserdata(L, sizeof(int));
    *static_cast<int*>(payload) = 2468;
    lua_newtable(L);
    lua_pushstring(L, "__gc");
    lua_pushcclosure(L, recordUserdataFinalizer, 0);
    lua_settable(L, -3);
    ASSERT_EQ(suite, 1, lua_setmetatable(L, -2),
              "C API assigns userdata finalizer metatable");
    lua_pop(L, 1);

    lua_getglobal(L, "collectgarbage");
    lua_pushstring(L, "collect");
    lua_call(L, 1, 0);
    ASSERT_EQ(suite, 1, gApiFinalizerCalls, "C API userdata finalizer runs once");
    ASSERT_EQ(suite, 2468, gApiFinalizerPayload,
              "userdata finalizer receives payload pointer");

    lua_getglobal(L, "collectgarbage");
    lua_pushstring(L, "collect");
    lua_call(L, 1, 0);
    ASSERT_EQ(suite, 1, gApiFinalizerCalls, "userdata finalizer does not run twice");

    lua_close(L);
}

}  // namespace

void registerLuaCApiTests() {
    auto& registry = TestRegistry::getInstance();
    registry.registerTest(kSuiteName, "stack and invalid indexes", testStackAndInvalidIndexes);
    registry.registerTest(kSuiteName, "registry and globals pseudo-indexes",
                          testRegistryAndGlobalsPseudoIndexes);
    registry.registerTest(kSuiteName, "independent state isolation",
                          testIndependentStateIsolation);
    registry.registerTest(kSuiteName, "C closure captures upvalues", testCClosureCapturesUpvalues);
    registry.registerTest(kSuiteName, "C closure upvalue mutation",
                          testCClosureUpvalueMutationPersists);
    registry.registerTest(kSuiteName, "stack removal inside C frame", testStackRemovalInsideCFrame);
    registry.registerTest(kSuiteName, "checkstack and xmove", testCheckStackAndXMove);
    registry.registerTest(kSuiteName, "C closure upvalue introspection",
                          testCClosureUpvalueIntrospection);
    registry.registerTest(kSuiteName, "Lua closure upvalue introspection",
                          testLuaClosureUpvalueIntrospection);
    registry.registerTest(kSuiteName, "light and full userdata", testLightAndFullUserdata);
    registry.registerTest(kSuiteName, "userdata finalizer through C API",
                          testUserdataFinalizerThroughCApi);
}
