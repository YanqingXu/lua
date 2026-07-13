#include "../framework/test_framework.hpp"

#include "common/lua_error.hpp"
#include "compiler/codegen/codegen.hpp"
#include "compiler/parser/parser.hpp"
#include "core/function.hpp"
#include "core/thread.hpp"
#include "core/upvalue.hpp"
#include "lua.h"
#include "lualib.h"
#include "runtime/runtime_services.hpp"
#include "vm/state/lua_state.hpp"

#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <unordered_map>

using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Lua C API";

int gApiFinalizerCalls = 0;
int gApiFinalizerPayload = 0;
int gApiErrorToken = 0;
int gApiHandlerCalls = 0;
void* gApiHandlerErrorObject = nullptr;
struct AllocatorProbe;
AllocatorProbe* gAllocatorFailureProbe = nullptr;
size_t gAllocatorFailureOffset = 0;

struct AllocatorLedger {
    std::unordered_map<std::uintptr_t, size_t> blocks;
    size_t sizeMismatches = 0;
    size_t unknownFrees = 0;
};

struct AllocatorProbe {
    AllocatorLedger* ledger = nullptr;
    size_t calls = 0;
    size_t allocations = 0;
    size_t reallocations = 0;
    size_t frees = 0;
    size_t failOnCall = 0;
    size_t allocationAttempts = 0;
    size_t failOnAllocation = 0;
};

void* trackingLuaAllocator(void* userData, void* pointer, size_t oldSize, size_t newSize) {
    auto* probe = static_cast<AllocatorProbe*>(userData);
    ++probe->calls;

    const std::uintptr_t oldKey = reinterpret_cast<std::uintptr_t>(pointer);
    if (newSize == 0) {
        if (pointer != nullptr) {
            auto existing = probe->ledger->blocks.find(oldKey);
            if (existing == probe->ledger->blocks.end()) {
                ++probe->ledger->unknownFrees;
            } else {
                if (existing->second != oldSize) {
                    ++probe->ledger->sizeMismatches;
                }
                probe->ledger->blocks.erase(existing);
            }
            std::free(pointer);
            ++probe->frees;
        }
        return nullptr;
    }

    ++probe->allocationAttempts;
    if (probe->failOnAllocation != 0 && probe->allocationAttempts == probe->failOnAllocation) {
        return nullptr;
    }

    if (probe->failOnCall != 0 && probe->calls == probe->failOnCall) {
        return nullptr;
    }

    if (pointer != nullptr) {
        auto existing = probe->ledger->blocks.find(oldKey);
        if (existing != probe->ledger->blocks.end() && existing->second != oldSize) {
            ++probe->ledger->sizeMismatches;
        }
    }

    void* result = std::realloc(pointer, newSize);
    if (result == nullptr) {
        return nullptr;
    }

    if (pointer != nullptr) {
        probe->ledger->blocks.erase(oldKey);
        ++probe->reallocations;
    } else {
        ++probe->allocations;
    }
    probe->ledger->blocks[reinterpret_cast<std::uintptr_t>(result)] = newSize;
    return result;
}

void pushLuaChunk(lua_State* L, const char* source) {
    auto* state = reinterpret_cast<Lua::LuaState*>(L);
    Lua::RuntimeServices services(state->getGlobalState());
    Lua::Parser parser(source, services);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }

    Lua::Chunk chunk = std::move(*parsed);
    Lua::CodeGenerator codegen(services);
    Lua::Proto* proto = codegen.generate(chunk, "api_test");
    if (proto == nullptr) {
        throw std::runtime_error("API test chunk code generation failed");
    }

    Lua::Function* closure = state->getGlobalState().getGC().create<Lua::Function>(proto);
    closure->setEnv(state->getGlobalTable());
    state->pushFunction(closure);
}

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

int raiseLightUserdataError(lua_State* L) {
    lua_pushlightuserdata(L, &gApiErrorToken);
    return lua_error(L);
}

int callFailingCFunction(lua_State* L) {
    lua_pushcclosure(L, raiseLightUserdataError, 0);
    lua_call(L, 0, 0);
    return 0;
}

int transformApiError(lua_State* L) {
    ++gApiHandlerCalls;
    gApiHandlerErrorObject = lua_touserdata(L, 1);
    lua_pushstring(L, "handled-error");
    return 1;
}

int failWhileHandlingApiError(lua_State* L) {
    lua_pushstring(L, "handler-failure");
    return lua_error(L);
}

int throwMemoryErrorFromC(lua_State*) {
    throw Lua::MemoryError("simulated allocation failure");
}

int throwUnknownCppException(lua_State*) {
    throw 17;
}

int attemptYieldFromMainState(lua_State* L) {
    return lua_yield(L, 0);
}

int yieldApiArguments(lua_State* L) {
    return lua_yield(L, lua_gettop(L));
}

int doubleApiArgument(lua_State* L) {
    lua_pushnumber(L, lua_tonumber(L, 1) * 2);
    return 1;
}

int allocateApiUserdata(lua_State* L) {
    if (gAllocatorFailureProbe != nullptr && gAllocatorFailureOffset != 0) {
        gAllocatorFailureProbe->failOnCall = gAllocatorFailureProbe->calls + gAllocatorFailureOffset;
    }
    lua_newuserdata(L, 48);
    return 1;
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

    ASSERT_EQ(suite, LUA_TTABLE, lua_type(L, LUA_REGISTRYINDEX), "registry pseudo-index exposes table");
    ASSERT_EQ(suite, LUA_TTABLE, lua_type(L, LUA_GLOBALSINDEX), "globals pseudo-index exposes table");

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
    ASSERT_EQ(suite, 73.0, lua_tonumber(L, -1), "globals pseudo-index shares the state global table");

    lua_close(L);
}

void testIndependentStateIsolation(TestSuite& suite) {
    lua_State* first = lua_open();
    lua_State* second = lua_open();

    lua_pushnumber(first, 17);
    lua_setglobal(first, "isolated_global");
    lua_getglobal(second, "isolated_global");
    ASSERT_EQ(suite, LUA_TNIL, lua_type(second, -1), "independent states do not share globals");
    lua_pop(second, 1);

    lua_pushstring(first, "first-registry");
    lua_rawseti(first, LUA_REGISTRYINDEX, 91);
    lua_rawgeti(second, LUA_REGISTRYINDEX, 91);
    ASSERT_EQ(suite, LUA_TNIL, lua_type(second, -1), "independent states do not share registries");

    auto* firstState = reinterpret_cast<Lua::LuaState*>(first);
    auto* secondState = reinterpret_cast<Lua::LuaState*>(second);
    ASSERT_TRUE(suite, &firstState->getGlobalState() != &secondState->getGlobalState(),
                "independent states own distinct global states");
    ASSERT_TRUE(suite, &firstState->getGlobalState().getStringPool() != &secondState->getGlobalState().getStringPool(),
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
    ASSERT_EQ(suite, 0, lua_checkstack(parent, 1000000), "checkstack rejects growth beyond VM maximum");

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
    Lua::LuaState::destroyState(childState);
    lua_close(parent);
}

void testCClosureUpvalueIntrospection(TestSuite& suite) {
    lua_State* L = lua_open();

    lua_pushnumber(L, 41);
    lua_pushcclosure(L, returnFirstUpvalue, 1);

    const char* getName = lua_getupvalue(L, 1, 1);
    ASSERT_TRUE(suite, getName != nullptr, "getupvalue finds C closure upvalue");
    ASSERT_EQ(suite, std::string(""), std::string(getName), "C closure upvalue name is empty");
    ASSERT_EQ(suite, 41.0, lua_tonumber(L, -1), "getupvalue pushes captured value");
    lua_pop(L, 1);

    const int beforeInvalidGet = lua_gettop(L);
    ASSERT_TRUE(suite, lua_getupvalue(L, 1, 2) == nullptr, "getupvalue rejects invalid ordinal");
    ASSERT_EQ(suite, beforeInvalidGet, lua_gettop(L), "invalid getupvalue leaves stack unchanged");

    lua_pushnumber(L, 99);
    const char* setName = lua_setupvalue(L, 1, 1);
    ASSERT_TRUE(suite, setName != nullptr, "setupvalue finds C closure upvalue");
    ASSERT_EQ(suite, std::string(""), std::string(setName), "setupvalue reports empty C closure name");
    ASSERT_EQ(suite, 1, lua_gettop(L), "setupvalue consumes replacement value");

    lua_pushnumber(L, 123);
    ASSERT_TRUE(suite, lua_setupvalue(L, 1, 2) == nullptr, "setupvalue rejects invalid ordinal");
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
    ASSERT_EQ(suite, std::string("captured"), std::string(getName), "getupvalue returns Lua closure debug name");
    ASSERT_EQ(suite, 7.0, lua_tonumber(L, -1), "getupvalue pushes Lua closure value");
    lua_pop(L, 1);

    lua_pushnumber(L, 8);
    const char* setName = lua_setupvalue(L, 1, 1);
    ASSERT_TRUE(suite, setName != nullptr, "setupvalue finds Lua closure upvalue");
    ASSERT_EQ(suite, std::string("captured"), std::string(setName), "setupvalue returns Lua closure debug name");
    ASSERT_EQ(suite, 1, lua_gettop(L), "Lua setupvalue consumes replacement value");

    lua_getupvalue(L, 1, 1);
    ASSERT_EQ(suite, 8.0, lua_tonumber(L, -1), "Lua setupvalue persists replacement");

    lua_close(L);
}

void testLightAndFullUserdata(TestSuite& suite) {
    lua_State* L = lua_open();

    int lightPayload = 17;
    lua_pushlightuserdata(L, &lightPayload);
    ASSERT_EQ(suite, LUA_TLIGHTUSERDATA, lua_type(L, -1), "pushlightuserdata preserves Lua type");
    ASSERT_TRUE(suite, lua_isuserdata(L, -1) != 0, "isuserdata accepts light userdata");
    ASSERT_TRUE(suite, lua_touserdata(L, -1) == &lightPayload, "touserdata returns light pointer");
    ASSERT_EQ(suite, static_cast<size_t>(0), lua_objlen(L, -1), "light userdata has zero object length");
    lua_pop(L, 1);

    void* empty = lua_newuserdata(L, 0);
    ASSERT_TRUE(suite, empty != nullptr, "zero-size full userdata has stable address");
    ASSERT_EQ(suite, LUA_TUSERDATA, lua_type(L, -1), "newuserdata pushes full userdata");
    ASSERT_EQ(suite, static_cast<size_t>(0), lua_objlen(L, -1), "zero-size userdata reports requested size");
    void* secondEmpty = lua_newuserdata(L, 0);
    ASSERT_TRUE(suite, secondEmpty != empty, "zero-size userdata addresses stay distinct");
    lua_pop(L, 2);

    void* full = lua_newuserdata(L, sizeof(int));
    ASSERT_TRUE(suite, full != nullptr, "full userdata exposes payload buffer");
    ASSERT_TRUE(suite, reinterpret_cast<std::uintptr_t>(full) % 8 == 0, "full userdata payload is eight-byte aligned");
    ASSERT_TRUE(suite, lua_touserdata(L, -1) == full, "touserdata returns full payload pointer");
    ASSERT_EQ(suite, sizeof(int), lua_objlen(L, -1), "full userdata reports payload size");
    ASSERT_EQ(suite, 0, *static_cast<int*>(full), "full userdata starts zero initialized");
    *static_cast<int*>(full) = 73;
    ASSERT_EQ(suite, 73, *static_cast<int*>(lua_touserdata(L, -1)), "full userdata payload mutation persists");

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
    ASSERT_EQ(suite, sizeof(embedded), lua_objlen(L, -1), "objlen preserves embedded string bytes");
    lua_pop(L, 1);

    lua_newtable(L);
    lua_pushnumber(L, 1);
    lua_rawseti(L, -2, 1);
    lua_pushnumber(L, 2);
    lua_rawseti(L, -2, 2);
    ASSERT_EQ(suite, static_cast<size_t>(2), lua_objlen(L, -1), "objlen reports table sequence length");

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
    ASSERT_EQ(suite, 1, lua_setmetatable(L, -2), "C API assigns userdata finalizer metatable");
    lua_pop(L, 1);

    lua_getglobal(L, "collectgarbage");
    lua_pushstring(L, "collect");
    lua_call(L, 1, 0);
    ASSERT_EQ(suite, 1, gApiFinalizerCalls, "C API userdata finalizer runs once");
    ASSERT_EQ(suite, 2468, gApiFinalizerPayload, "userdata finalizer receives payload pointer");

    lua_getglobal(L, "collectgarbage");
    lua_pushstring(L, "collect");
    lua_call(L, 1, 0);
    ASSERT_EQ(suite, 1, gApiFinalizerCalls, "userdata finalizer does not run twice");

    lua_close(L);
}

void testProtectedCallRestoresStackAndPreservesErrorObject(TestSuite& suite) {
    lua_State* L = lua_open();

    lua_pushnumber(L, 17);
    lua_pushcclosure(L, raiseLightUserdataError, 0);
    lua_pushstring(L, "discarded-argument");
    const int directStatus = lua_pcall(L, 1, LUA_MULTRET, 0);
    ASSERT_EQ(suite, LUA_ERRRUN, directStatus, "pcall reports Lua error status");
    ASSERT_EQ(suite, 2, lua_gettop(L), "pcall replaces function and arguments with one error");
    ASSERT_EQ(suite, 17.0, lua_tonumber(L, 1), "pcall preserves stack prefix");
    ASSERT_TRUE(suite, lua_touserdata(L, 2) == &gApiErrorToken, "pcall preserves non-string Lua error object identity");
    lua_pop(L, 1);

    lua_pushcclosure(L, callFailingCFunction, 0);
    const int nestedCallStatus = lua_pcall(L, 0, 0, 0);
    ASSERT_EQ(suite, LUA_ERRRUN, nestedCallStatus, "lua_call error propagates to enclosing protected call");
    ASSERT_EQ(suite, 2, lua_gettop(L), "enclosing pcall restores stack after nested lua_call failure");
    ASSERT_TRUE(suite, lua_touserdata(L, -1) == &gApiErrorToken, "nested lua_call preserves Lua error object identity");

    lua_close(L);
}

void testProtectedCallErrorHandlers(TestSuite& suite) {
    lua_State* L = lua_open();
    gApiHandlerCalls = 0;
    gApiHandlerErrorObject = nullptr;

    lua_pushcclosure(L, transformApiError, 0);
    lua_pushcclosure(L, raiseLightUserdataError, 0);
    const int handledStatus = lua_pcall(L, 0, 1, 1);
    ASSERT_EQ(suite, LUA_ERRRUN, handledStatus, "successful error handler preserves original error status");
    ASSERT_EQ(suite, 2, lua_gettop(L), "pcall leaves handler below transformed error");
    ASSERT_EQ(suite, 1, gApiHandlerCalls, "pcall invokes error handler exactly once");
    ASSERT_TRUE(suite, gApiHandlerErrorObject == &gApiErrorToken, "error handler receives original Lua error object");
    ASSERT_EQ(suite, std::string("handled-error"), std::string(lua_tostring(L, -1)),
              "pcall returns transformed error handler result");

    lua_settop(L, 0);
    lua_pushcclosure(L, transformApiError, 0);
    lua_pushcclosure(L, raiseLightUserdataError, 0);
    ASSERT_EQ(suite, LUA_ERRRUN, lua_pcall(L, 0, 1, -2), "pcall accepts negative error-handler index");
    ASSERT_EQ(suite, std::string("handled-error"), std::string(lua_tostring(L, -1)),
              "negative error-handler index transforms error");

    lua_settop(L, 0);
    pushLuaChunk(L, "return function(err) return err end");
    lua_call(L, 0, 1);
    lua_pushcclosure(L, raiseLightUserdataError, 0);
    const int luaHandlerStatus = lua_pcall(L, 0, 1, 1);
    ASSERT_EQ(suite, LUA_ERRRUN, luaHandlerStatus, "Lua closure can serve as pcall error handler");
    ASSERT_TRUE(suite, lua_touserdata(L, -1) == &gApiErrorToken,
                "Lua error handler returns original object without coercion");

    lua_settop(L, 0);
    lua_pushcclosure(L, failWhileHandlingApiError, 0);
    lua_pushcclosure(L, raiseLightUserdataError, 0);
    const int failedHandlerStatus = lua_pcall(L, 0, 1, 1);
    ASSERT_EQ(suite, LUA_ERRERR, failedHandlerStatus, "pcall distinguishes error-handler failure");
    ASSERT_EQ(suite, 2, lua_gettop(L), "failed handler still leaves one canonical error result");
    ASSERT_EQ(suite, std::string("error in error handling"), std::string(lua_tostring(L, -1)),
              "failed handler returns canonical error text");

    lua_settop(L, 0);
    lua_pushnumber(L, 99);
    lua_pushcclosure(L, raiseLightUserdataError, 0);
    ASSERT_EQ(suite, LUA_ERRERR, lua_pcall(L, 0, 1, 1), "non-function error handler reports LUA_ERRERR");

    lua_close(L);
}

void testCApiYieldInsideCoroutine(TestSuite& suite) {
    lua_State* L = lua_open();
    auto* parent = reinterpret_cast<Lua::LuaState*>(L);

    lua_pushcclosure(L, yieldApiArguments, 0);
    lua_setglobal(L, "api_yield_leaf");
    pushLuaChunk(L, "return api_yield_leaf(55)");
    Lua::Function* body = parent->at(-1).asFunction();
    lua_pop(L, 1);

    Lua::Thread* thread = Lua::Thread::create(parent, body);
    parent->pushValue(Lua::Value(thread));
    const bool resumed = thread->resume(parent, 0);

    ASSERT_TRUE(suite, resumed, "lua_yield succeeds inside resumable coroutine");
    ASSERT_TRUE(suite, thread->isSuspended(), "C API yield suspends coroutine");
    ASSERT_EQ(suite, LUA_YIELD, lua_status(reinterpret_cast<lua_State*>(thread->getLuaState())),
              "yielded coroutine exposes LUA_YIELD status");
    ASSERT_EQ(suite, 3, lua_gettop(L), "resume returns thread, success flag, and yield value");
    ASSERT_TRUE(suite, lua_toboolean(L, 2) != 0, "resume reports successful yield");
    ASSERT_EQ(suite, 55.0, lua_tonumber(L, 3), "lua_yield transfers C callback argument");

    lua_close(L);
}

void testPublicThreadResumeApi(TestSuite& suite) {
    lua_State* L = lua_open();
    lua_pushnumber(L, 7);
    lua_State* co = lua_newthread(L);

    ASSERT_TRUE(suite, co != nullptr, "lua_newthread returns a child state");
    ASSERT_EQ(suite, 2, lua_gettop(L), "lua_newthread pushes thread without disturbing prefix");
    ASSERT_TRUE(suite, lua_isthread(L, -1) != 0, "lua_newthread pushes a thread value");
    ASSERT_EQ(suite, 0, lua_gettop(co), "new thread has an empty API-visible stack");

    pushLuaChunk(L, "return 21, 'done'");
    lua_xmove(L, co, 1);
    const int resumeStatus = lua_resume(co, 0);
    ASSERT_EQ(suite, LUA_OK, resumeStatus, "lua_resume completes a Lua entry function");
    ASSERT_EQ(suite, LUA_OK, lua_status(co), "completed child reports LUA_OK");
    ASSERT_EQ(suite, 2, lua_gettop(co), "completed child exposes return values");
    ASSERT_EQ(suite, 21.0, lua_tonumber(co, 1), "lua_resume preserves numeric result");
    ASSERT_EQ(suite, std::string("done"), std::string(lua_tostring(co, 2)), "lua_resume preserves string result");
    ASSERT_EQ(suite, 2, lua_gettop(L), "lua_resume preserves caller stack prefix");

    ASSERT_EQ(suite, LUA_ERRRUN, lua_resume(co, 0), "resuming a dead child returns LUA_ERRRUN");
    ASSERT_EQ(suite, 1, lua_gettop(co), "dead resume leaves one canonical error object");
    ASSERT_TRUE(suite, std::string(lua_tostring(co, -1)).find("dead") != std::string::npos,
                "dead resume reports stable error text");

    lua_close(L);
}

void testPublicThreadYieldAndErrorApi(TestSuite& suite) {
    lua_State* L = lua_open();
    lua_pushcclosure(L, yieldApiArguments, 0);
    lua_setglobal(L, "api_yield_leaf");

    lua_State* yielded = lua_newthread(L);
    pushLuaChunk(L, "local value = api_yield_leaf(55); return value + 1");
    lua_xmove(L, yielded, 1);

    ASSERT_EQ(suite, LUA_YIELD, lua_resume(yielded, 0), "public lua_resume exposes coroutine yield status");
    ASSERT_EQ(suite, LUA_YIELD, lua_status(yielded), "yielded child reports LUA_YIELD");
    ASSERT_EQ(suite, 1, lua_gettop(yielded), "yielded values remain on child stack");
    ASSERT_EQ(suite, 55.0, lua_tonumber(yielded, 1), "yielded value is API-visible");

    lua_settop(yielded, 0);
    lua_pushnumber(yielded, 9);
    ASSERT_EQ(suite, LUA_OK, lua_resume(yielded, 1), "public lua_resume accepts values after yield");
    ASSERT_EQ(suite, 1, lua_gettop(yielded), "finished child exposes final return value");
    ASSERT_EQ(suite, 10.0, lua_tonumber(yielded, 1), "resume argument becomes yield return value");

    lua_pushcclosure(L, raiseLightUserdataError, 0);
    lua_setglobal(L, "api_resume_error");
    lua_State* failed = lua_newthread(L);
    pushLuaChunk(L, "return api_resume_error()");
    lua_xmove(L, failed, 1);
    ASSERT_EQ(suite, LUA_ERRRUN, lua_resume(failed, 0), "public lua_resume returns runtime error status");
    ASSERT_EQ(suite, 1, lua_gettop(failed), "failed child exposes one error object");
    ASSERT_TRUE(suite, lua_touserdata(failed, -1) == &gApiErrorToken,
                "public lua_resume preserves non-string error object identity");

    lua_close(L);
}

void testPublicThreadAllocatorLifecycle(TestSuite& suite) {
    AllocatorLedger ledger;
    AllocatorProbe probe{&ledger};
    lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
    ASSERT_TRUE(suite, L != nullptr, "thread allocator test creates parent state");

    const size_t blocksBeforeThread = ledger.blocks.size();
    const size_t attemptsBeforeThread = probe.allocationAttempts;
    lua_State* co = lua_newthread(L);
    const size_t childAllocationAttempts = probe.allocationAttempts - attemptsBeforeThread;
    ASSERT_TRUE(suite, co != nullptr, "custom allocator creates child state");
    ASSERT_TRUE(suite, ledger.blocks.size() >= blocksBeforeThread + 4,
                "thread object, child state, stack, and CallInfo use allocator");

    void* childAllocatorData = nullptr;
    ASSERT_TRUE(suite, lua_getallocf(co, &childAllocatorData) == trackingLuaAllocator,
                "child state shares current allocator callback");
    ASSERT_TRUE(suite, childAllocatorData == &probe, "child state shares allocator userdata");

    lua_close(L);
    ASSERT_TRUE(suite, ledger.blocks.empty(), "closing parent releases child state allocation");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.sizeMismatches,
              "child state frees preserve allocator old-size contract");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.unknownFrees, "child state allocations are each released once");

    for (size_t failureOffset = 1; failureOffset <= childAllocationAttempts; ++failureOffset) {
        AllocatorLedger failedLedger;
        AllocatorProbe failedProbe{&failedLedger};
        lua_State* failureParent = lua_newstate(trackingLuaAllocator, &failedProbe);
        ASSERT_TRUE(suite, failureParent != nullptr, "thread failure test creates parent state");
        const int parentTop = lua_gettop(failureParent);
        failedProbe.failOnAllocation = failedProbe.allocationAttempts + failureOffset;
        ASSERT_TRUE(suite, lua_newthread(failureParent) == nullptr,
                    "each child construction allocation failure returns null");
        ASSERT_EQ(suite, parentTop, lua_gettop(failureParent), "failed lua_newthread preserves parent stack");
        failedProbe.failOnAllocation = 0;
        lua_close(failureParent);
        ASSERT_TRUE(suite, failedLedger.blocks.empty(), "failed child creation and parent close remain leak free");
        ASSERT_EQ(suite, static_cast<size_t>(0), failedLedger.sizeMismatches,
                  "child creation rollback preserves old-size contract");
        ASSERT_EQ(suite, static_cast<size_t>(0), failedLedger.unknownFrees,
                  "child creation rollback frees each block once");
    }
}

void testProtectedCallNormalizesCppExceptionsAndYield(TestSuite& suite) {
    lua_State* L = lua_open();

    lua_pushcclosure(L, throwMemoryErrorFromC, 0);
    ASSERT_EQ(suite, LUA_ERRMEM, lua_pcall(L, 0, 1, 0), "pcall maps MemoryError to LUA_ERRMEM");
    ASSERT_EQ(suite, std::string("not enough memory"), std::string(lua_tostring(L, -1)),
              "memory failure uses fixed Lua memory error object");
    ASSERT_EQ(suite, LUA_OK, lua_status(L), "pcall restores state status after memory error");

    lua_settop(L, 0);
    lua_pushcclosure(L, throwUnknownCppException, 0);
    ASSERT_EQ(suite, LUA_ERRRUN, lua_pcall(L, 0, 1, 0), "pcall contains non-standard C++ exceptions");
    ASSERT_EQ(suite, std::string("unknown C++ exception"), std::string(lua_tostring(L, -1)),
              "unknown C++ exception is converted to Lua error text");

    lua_settop(L, 0);
    lua_pushcclosure(L, attemptYieldFromMainState, 0);
    ASSERT_EQ(suite, LUA_ERRRUN, lua_pcall(L, 0, 1, 0), "pcall rejects yield across main-state host boundary");
    ASSERT_TRUE(suite, std::string(lua_tostring(L, -1)).find("cannot yield") != std::string::npos,
                "yield boundary reports a stable Lua runtime error");
    ASSERT_EQ(suite, LUA_OK, lua_status(L), "failed yield leaves protected state usable");

    lua_close(L);
}

void testNestedCToLuaToCProtectedCalls(TestSuite& suite) {
    lua_State* L = lua_open();

    lua_pushcclosure(L, doubleApiArgument, 0);
    lua_setglobal(L, "api_nested_leaf");
    pushLuaChunk(L, "return api_nested_leaf(21)");
    ASSERT_EQ(suite, LUA_OK, lua_pcall(L, 0, 1, 0), "pcall executes nested C to Lua to C success path");
    ASSERT_EQ(suite, 42.0, lua_tonumber(L, -1), "nested C to Lua to C call returns result");

    lua_settop(L, 0);
    lua_pushcclosure(L, raiseLightUserdataError, 0);
    lua_setglobal(L, "api_nested_error");
    lua_pushnumber(L, 73);
    pushLuaChunk(L, "return api_nested_error()");
    ASSERT_EQ(suite, LUA_ERRRUN, lua_pcall(L, 0, 1, 0), "pcall catches nested C to Lua to C error path");
    ASSERT_EQ(suite, 2, lua_gettop(L), "nested error restores host stack prefix");
    ASSERT_EQ(suite, 73.0, lua_tonumber(L, 1), "nested error preserves prefix value");
    ASSERT_TRUE(suite, lua_touserdata(L, -1) == &gApiErrorToken, "nested error preserves original object identity");

    lua_close(L);
}

void testCustomAllocatorLifecycle(TestSuite& suite) {
    AllocatorLedger ledger;
    AllocatorProbe probe{&ledger};
    lua_State* L = lua_newstate(trackingLuaAllocator, &probe);

    ASSERT_TRUE(suite, L != nullptr, "lua_newstate creates state through custom allocator");
    ASSERT_TRUE(suite, probe.allocations > 20, "state initialization routes fixed runtime objects through allocator");

    void* observedUserData = nullptr;
    ASSERT_TRUE(suite, lua_getallocf(L, &observedUserData) == trackingLuaAllocator,
                "getallocf returns configured callback");
    ASSERT_TRUE(suite, observedUserData == &probe, "getallocf returns configured allocator userdata");

    const size_t blocksBeforeObjects = ledger.blocks.size();
    lua_newuserdata(L, 64);
    lua_newtable(L);
    lua_pushstring(L, "allocator-owned-string");
    ASSERT_TRUE(suite, ledger.blocks.size() >= blocksBeforeObjects + 4,
                "userdata payload and GC object blocks use allocator");

    lua_close(L);
    ASSERT_TRUE(suite, ledger.blocks.empty(), "lua_close releases state, context, fixed roots, objects, and payloads");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.sizeMismatches, "allocator frees receive original object sizes");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.unknownFrees, "allocator close path only frees tracked blocks");
    ASSERT_EQ(suite, probe.allocations + probe.reallocations, probe.frees,
              "custom allocator allocations and frees balance");
}

void testAllocatorCanBeReplaced(TestSuite& suite) {
    AllocatorLedger ledger;
    AllocatorProbe first{&ledger};
    AllocatorProbe second{&ledger};
    lua_State* L = lua_newstate(trackingLuaAllocator, &first);
    ASSERT_TRUE(suite, L != nullptr, "allocator replacement test creates state");

    lua_setallocf(L, trackingLuaAllocator, &second);
    void* observedUserData = nullptr;
    ASSERT_TRUE(suite, lua_getallocf(L, &observedUserData) == trackingLuaAllocator,
                "setallocf preserves replacement callback");
    ASSERT_TRUE(suite, observedUserData == &second, "setallocf replaces allocator userdata");

    const size_t secondAllocationsBefore = second.allocations;
    lua_newuserdata(L, 32);
    ASSERT_TRUE(suite, second.allocations >= secondAllocationsBefore + 2,
                "future object and payload allocations use replacement allocator");

    lua_close(L);
    ASSERT_TRUE(suite, second.frees > 0, "existing runtime blocks are released through current allocator");
    ASSERT_TRUE(suite, ledger.blocks.empty(), "allocator replacement keeps close path leak free");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.sizeMismatches,
              "allocator replacement preserves old-size contract");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.unknownFrees,
              "allocator replacement frees shared tracked blocks exactly once");
}

void testAllocatorFailurePaths(TestSuite& suite) {
    for (size_t failOnCall :
         {static_cast<size_t>(1), static_cast<size_t>(2), static_cast<size_t>(5), static_cast<size_t>(20)}) {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        probe.failOnAllocation = failOnCall;
        lua_State* failed = lua_newstate(trackingLuaAllocator, &probe);
        ASSERT_TRUE(suite, failed == nullptr, "lua_newstate returns null when allocator rejects construction");
        ASSERT_TRUE(suite, ledger.blocks.empty(), "failed state construction releases partial allocator blocks");
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.unknownFrees, "failed state construction does not double free");
    }

    AllocatorLedger ledger;
    AllocatorProbe probe{&ledger};
    lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
    ASSERT_TRUE(suite, L != nullptr, "runtime allocation failure test creates state");

    lua_pushcclosure(L, allocateApiUserdata, 0);
    probe.failOnCall = probe.calls + 1;
    ASSERT_EQ(suite, LUA_ERRMEM, lua_pcall(L, 0, 1, 0), "pcall protection setup failure becomes LUA_ERRMEM");

    lua_settop(L, 0);
    probe.failOnCall = 0;
    lua_pushcclosure(L, allocateApiUserdata, 0);
    gAllocatorFailureProbe = &probe;
    gAllocatorFailureOffset = 1;
    ASSERT_EQ(suite, LUA_ERRMEM, lua_pcall(L, 0, 1, 0), "GC object allocation failure becomes LUA_ERRMEM");

    lua_settop(L, 0);
    probe.failOnCall = 0;
    lua_pushcclosure(L, allocateApiUserdata, 0);
    gAllocatorFailureOffset = 2;
    ASSERT_EQ(suite, LUA_ERRMEM, lua_pcall(L, 0, 1, 0), "userdata payload allocation failure becomes LUA_ERRMEM");
    gAllocatorFailureProbe = nullptr;
    gAllocatorFailureOffset = 0;
    probe.failOnCall = 0;

    lua_close(L);
    ASSERT_TRUE(suite, ledger.blocks.empty(), "runtime allocation failures leave close path leak free");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.sizeMismatches,
              "failure rollback preserves allocator size contract");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.unknownFrees, "failure rollback frees each allocator block once");
}

} // namespace

void registerLuaCApiTests() {
    auto& registry = TestRegistry::getInstance();
    registry.registerTest(kSuiteName, "stack and invalid indexes", testStackAndInvalidIndexes);
    registry.registerTest(kSuiteName, "registry and globals pseudo-indexes", testRegistryAndGlobalsPseudoIndexes);
    registry.registerTest(kSuiteName, "independent state isolation", testIndependentStateIsolation);
    registry.registerTest(kSuiteName, "C closure captures upvalues", testCClosureCapturesUpvalues);
    registry.registerTest(kSuiteName, "C closure upvalue mutation", testCClosureUpvalueMutationPersists);
    registry.registerTest(kSuiteName, "stack removal inside C frame", testStackRemovalInsideCFrame);
    registry.registerTest(kSuiteName, "checkstack and xmove", testCheckStackAndXMove);
    registry.registerTest(kSuiteName, "C closure upvalue introspection", testCClosureUpvalueIntrospection);
    registry.registerTest(kSuiteName, "Lua closure upvalue introspection", testLuaClosureUpvalueIntrospection);
    registry.registerTest(kSuiteName, "light and full userdata", testLightAndFullUserdata);
    registry.registerTest(kSuiteName, "userdata finalizer through C API", testUserdataFinalizerThroughCApi);
    registry.registerTest(kSuiteName, "protected call stack and error object",
                          testProtectedCallRestoresStackAndPreservesErrorObject);
    registry.registerTest(kSuiteName, "protected call error handlers", testProtectedCallErrorHandlers);
    registry.registerTest(kSuiteName, "protected call C++ and yield boundaries",
                          testProtectedCallNormalizesCppExceptionsAndYield);
    registry.registerTest(kSuiteName, "C API yield inside coroutine", testCApiYieldInsideCoroutine);
    registry.registerTest(kSuiteName, "public thread resume API", testPublicThreadResumeApi);
    registry.registerTest(kSuiteName, "public thread yield and error API", testPublicThreadYieldAndErrorApi);
    registry.registerTest(kSuiteName, "public thread allocator lifecycle", testPublicThreadAllocatorLifecycle);
    registry.registerTest(kSuiteName, "nested C to Lua to C protected calls", testNestedCToLuaToCProtectedCalls);
    registry.registerTest(kSuiteName, "custom allocator lifecycle", testCustomAllocatorLifecycle);
    registry.registerTest(kSuiteName, "allocator replacement", testAllocatorCanBeReplaced);
    registry.registerTest(kSuiteName, "allocator failure paths", testAllocatorFailurePaths);
}
