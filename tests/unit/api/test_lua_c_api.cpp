#include "../framework/test_framework.hpp"

#include "common/lua_error.hpp"
#include "compiler/codegen/codegen.hpp"
#include "compiler/parser/parser.hpp"
#include "core/function.hpp"
#include "core/thread.hpp"
#include "core/upvalue.hpp"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "runtime/runtime_services.hpp"
#include "vm/state/lua_state.hpp"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>

using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Lua C API";

static_assert(!noexcept(lua_xmove(nullptr, nullptr, 0)));
static_assert(!noexcept(lua_call(nullptr, 0, 0)));
static_assert(!noexcept(lua_error(nullptr)));
static_assert(!noexcept(luaL_error(nullptr, "%s", "error")));

int gApiFinalizerCalls = 0;
int gApiFinalizerPayload = 0;
int gApiErrorToken = 0;
int gApiHandlerCalls = 0;
void* gApiHandlerErrorObject = nullptr;
struct AllocatorProbe;
AllocatorProbe* gAllocatorFailureProbe = nullptr;
size_t gAllocatorFailureOffset = 0;
size_t gOversizedUserdataSize = 0;
size_t gArmedAllocatorFailureTarget = 0;

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
    size_t failFromAllocation = 0;
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
    if (probe->failFromAllocation != 0 && probe->allocationAttempts >= probe->failFromAllocation) {
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

int yieldOnlyTopApiValue(lua_State* L) {
    lua_pushnumber(L, 111);
    lua_pushnumber(L, 222);
    return lua_yield(L, 1);
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

int disarmAllocatorFailure(lua_State*) {
    if (gAllocatorFailureProbe != nullptr) {
        gAllocatorFailureProbe->failOnAllocation = 0;
        gAllocatorFailureProbe->failFromAllocation = 0;
        gAllocatorFailureProbe->failOnCall = 0;
    }
    return 0;
}

int throwRuntimeErrorDuringResume(lua_State*) {
    if (gAllocatorFailureProbe != nullptr) {
        gArmedAllocatorFailureTarget = gAllocatorFailureProbe->allocationAttempts + 1;
        gAllocatorFailureProbe->failFromAllocation = gArmedAllocatorFailureTarget;
    }
    throw std::runtime_error("injected coroutine runtime failure");
}

int appendDumpChunk(lua_State*, const void* bytes, size_t size, void* userData) {
    auto* output = static_cast<std::string*>(userData);
    output->append(static_cast<const char*>(bytes), size);
    return 0;
}

struct ReaderProbe {
    const char* pieces[3] = {"return ", "6 * ", "7"};
    size_t index = 0;
};

const char* readProbeChunk(lua_State*, void* userData, size_t* size) {
    auto* probe = static_cast<ReaderProbe*>(userData);
    if (probe->index >= 3) {
        *size = 0;
        return nullptr;
    }
    const char* piece = probe->pieces[probe->index++];
    *size = std::strlen(piece);
    return piece;
}

const char* pushGcObjectsThenFailReader(lua_State* L, void*, size_t*) {
    lua_createtable(L, 0, 0);
    lua_createtable(L, 0, 0);
    throw std::bad_alloc();
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

void testApiStackShrinkReleasesGcRoots(TestSuite& suite) {
    lua_State* L = lua_open();
    auto* state = reinterpret_cast<Lua::LuaState*>(L);
    Lua::GarbageCollector& gc = state->getGlobalState().getGC();

    const Lua::usize baseline = gc.getObjectCount();
    lua_createtable(L, 0, 0);
    const Lua::usize withTable = gc.getObjectCount();
    ASSERT_TRUE(suite, withTable > baseline, "C API table creation adds a managed object");

    lua_pop(L, 1);
    ASSERT_EQ(suite, 0, lua_gettop(L), "C API pop shrinks the logical stack");
    (void)gc.collect(state);
    ASSERT_TRUE(suite, gc.getObjectCount() < withTable,
                "C API stack shrink clears stale physical roots before a wide GC scan");

    lua_close(L);
}

void testLoaderMemoryRollbackReleasesGcRoots(TestSuite& suite) {
    lua_State* L = lua_open();
    auto* state = reinterpret_cast<Lua::LuaState*>(L);
    Lua::GarbageCollector& gc = state->getGlobalState().getGC();
    const Lua::usize baseline = gc.getObjectCount();

    ASSERT_EQ(suite, LUA_ERRMEM, lua_load(L, pushGcObjectsThenFailReader, nullptr, "=reader-oom-roots"),
              "lua_load translates a reader allocation failure");
    ASSERT_EQ(suite, 1, lua_gettop(L), "lua_load replaces reader temporaries with one memory error");

    const Lua::usize afterFailure = gc.getObjectCount();
    ASSERT_TRUE(suite, afterFailure >= baseline + 2, "reader failure allocates the temporary GC graph");
    (void)gc.collect(state);
    ASSERT_TRUE(suite, gc.getObjectCount() + 2 <= afterFailure,
                "loader rollback clears every stale temporary stack root before GC");

    lua_close(L);
}

void testResumeBridgeRollbackReleasesGcRoots(TestSuite& suite) {
    lua_State* parent = lua_open();
    auto* parentState = reinterpret_cast<Lua::LuaState*>(parent);
    Lua::GarbageCollector& gc = parentState->getGlobalState().getGC();

    lua_State* coroutine = lua_newthread(parent);
    constexpr const char* source = R"lua(
        local root = {}
        for i = 1, 48 do
            root[i] = { value = i }
        end
        return root
    )lua";
    ASSERT_EQ(suite, LUA_OK, luaL_loadbuffer(parent, source, std::strlen(source), "=resume-bridge-roots"),
              "resume bridge root scenario compiles");
    lua_xmove(parent, coroutine, 1);
    ASSERT_EQ(suite, LUA_OK, lua_resume(coroutine, 0), "resume bridge returns its large result graph");
    ASSERT_EQ(suite, LUA_TTABLE, lua_type(coroutine, 1), "resumed state exposes the result graph");

    const Lua::usize afterResume = gc.getObjectCount();
    lua_settop(coroutine, 0);
    lua_settop(parent, 0);
    (void)gc.collect(parentState);
    ASSERT_TRUE(suite, gc.getObjectCount() + 32 <= afterResume,
                "bridge rollback does not retain a discarded large result graph");

    lua_close(parent);
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

void testCppApiExceptionContract(TestSuite& suite) {
    lua_State* direct = lua_open();
    lua_pushstring(direct, "direct-error");
    bool directErrorCaught = false;
    try {
        (void)lua_error(direct);
    } catch (const Lua::RuntimeError&) {
        directErrorCaught = true;
    }
    ASSERT_TRUE(suite, directErrorCaught, "C++ caller catches lua_error across its C-linkage boundary");
    lua_close(direct);

    lua_State* called = lua_open();
    lua_pushcclosure(called, raiseLightUserdataError, 0);
    bool callErrorCaught = false;
    try {
        lua_call(called, 0, 0);
    } catch (const Lua::RuntimeError&) {
        callErrorCaught = true;
    }
    ASSERT_TRUE(suite, callErrorCaught, "C++ caller catches an unprotected lua_call error");
    lua_close(called);

    lua_State* auxiliary = lua_open();
    bool auxiliaryErrorCaught = false;
    try {
        (void)luaL_error(auxiliary, "%s", "auxiliary-error");
    } catch (const Lua::RuntimeError&) {
        auxiliaryErrorCaught = true;
    }
    ASSERT_TRUE(suite, auxiliaryErrorCaught, "C++ caller catches luaL_error across its C-linkage boundary");
    lua_close(auxiliary);
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

void testPublicThreadCFunctionEntry(TestSuite& suite) {
    lua_State* L = lua_open();

    lua_State* completed = lua_newthread(L);
    lua_pushcclosure(L, doubleApiArgument, 0);
    lua_xmove(L, completed, 1);
    lua_pushnumber(completed, 21);

    ASSERT_EQ(suite, LUA_OK, lua_resume(completed, 1), "lua_resume accepts a C function as coroutine entry");
    ASSERT_EQ(suite, 1, lua_gettop(completed), "completed C entry exposes its return value");
    ASSERT_EQ(suite, 42.0, lua_tonumber(completed, 1), "C entry receives resume arguments");

    lua_State* yielded = lua_newthread(L);
    lua_pushcclosure(L, yieldApiArguments, 0);
    lua_xmove(L, yielded, 1);
    lua_pushnumber(yielded, 55);

    ASSERT_EQ(suite, LUA_YIELD, lua_resume(yielded, 1), "C coroutine entry can yield on its first resume");
    ASSERT_EQ(suite, LUA_YIELD, lua_status(yielded), "yielding C entry reports LUA_YIELD");
    ASSERT_EQ(suite, 1, lua_gettop(yielded), "yielding C entry exposes yielded values");
    ASSERT_EQ(suite, 55.0, lua_tonumber(yielded, 1), "C entry transfers its argument through lua_yield");

    lua_settop(yielded, 0);
    lua_pushnumber(yielded, 9);
    ASSERT_EQ(suite, LUA_OK, lua_resume(yielded, 1), "yielded C entry resumes to completion");
    ASSERT_EQ(suite, 1, lua_gettop(yielded), "resumed C entry exposes its final result");
    ASSERT_EQ(suite, 9.0, lua_tonumber(yielded, 1), "resume value becomes the C yield return value");

    lua_State* topOnly = lua_newthread(L);
    lua_pushcclosure(L, yieldOnlyTopApiValue, 0);
    lua_xmove(L, topOnly, 1);
    lua_pushnumber(topOnly, 55);

    ASSERT_EQ(suite, LUA_YIELD, lua_resume(topOnly, 1), "lua_yield can select only the top result value");
    ASSERT_EQ(suite, 1, lua_gettop(topOnly), "yield hides callback arguments and lower temporaries");
    ASSERT_EQ(suite, 222.0, lua_tonumber(topOnly, 1), "yield exposes the selected top value");

    lua_settop(topOnly, 0);
    lua_pushnumber(topOnly, 9);
    ASSERT_EQ(suite, LUA_OK, lua_resume(topOnly, 1), "top-only C yield resumes to completion");
    ASSERT_EQ(suite, 1, lua_gettop(topOnly), "resumed top-only C entry exposes one final result");
    ASSERT_EQ(suite, 9.0, lua_tonumber(topOnly, 1), "resume value becomes the yielded call result");

    lua_close(L);
}

int allocateOversizedApiUserdata(lua_State* L) {
    lua_newuserdata(L, gOversizedUserdataSize);
    return 1;
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

void testLoadDumpAndAuxiliaryLoaders(TestSuite& suite) {
    lua_State* L = lua_open();

    constexpr const char* source = "return 40 + 2";
    ASSERT_EQ(suite, LUA_OK, luaL_loadbuffer(L, source, std::strlen(source), "=loadbuffer"),
              "luaL_loadbuffer compiles source without opening libraries");
    ASSERT_TRUE(suite, lua_isfunction(L, -1) != 0, "luaL_loadbuffer pushes a Lua function");

    std::string binary;
    ASSERT_EQ(suite, 0, lua_dump(L, appendDumpChunk, &binary), "lua_dump writes the project-local binary chunk");
    ASSERT_EQ(suite, 1, lua_gettop(L), "lua_dump preserves the source function on the stack");
    ASSERT_TRUE(suite, binary.size() > 12, "lua_dump emits a non-empty chunk payload");

    ASSERT_EQ(suite, LUA_OK, lua_pcall(L, 0, 1, 0), "loaded source function executes");
    ASSERT_EQ(suite, 42.0, lua_tonumber(L, -1), "loaded source function returns expected value");
    lua_settop(L, 0);

    ASSERT_EQ(suite, LUA_OK, luaL_loadbuffer(L, binary.data(), binary.size(), "=binary"),
              "luaL_loadbuffer accepts lua_dump output");
    ASSERT_EQ(suite, LUA_OK, lua_pcall(L, 0, 1, 0), "binary chunk executes");
    ASSERT_EQ(suite, 42.0, lua_tonumber(L, -1), "binary chunk preserves return value");
    lua_settop(L, 0);

    ReaderProbe reader;
    ASSERT_EQ(suite, LUA_OK, lua_load(L, readProbeChunk, &reader, "=reader"), "lua_load concatenates reader chunks");
    ASSERT_EQ(suite, LUA_OK, lua_pcall(L, 0, 1, 0), "reader-loaded function executes");
    ASSERT_EQ(suite, 42.0, lua_tonumber(L, -1), "reader-loaded function returns expected value");
    lua_settop(L, 0);

    const std::filesystem::path filePath = std::filesystem::temp_directory_path() / "lua_cpp_c_api_loadfile.lua";
    {
        std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
        file << "return 42";
    }
    ASSERT_EQ(suite, LUA_OK, luaL_loadfile(L, filePath.string().c_str()), "luaL_loadfile compiles a script file");
    ASSERT_EQ(suite, LUA_OK, lua_pcall(L, 0, 1, 0), "file-loaded function executes");
    ASSERT_EQ(suite, 42.0, lua_tonumber(L, -1), "file-loaded function returns expected value");
    std::filesystem::remove(filePath);
    lua_settop(L, 0);

    ASSERT_EQ(suite, LUA_ERRSYNTAX, luaL_loadstring(L, "return +"),
              "luaL_loadstring returns syntax status for invalid source");
    ASSERT_TRUE(suite, lua_isstring(L, -1) != 0, "syntax failure leaves one error object");
    lua_settop(L, 0);

    ASSERT_EQ(suite, LUA_ERRFILE, luaL_loadfile(L, "missing-c-api-load-file.lua"),
              "luaL_loadfile distinguishes file errors");
    ASSERT_TRUE(suite, lua_isstring(L, -1) != 0, "file failure leaves one error object");
    lua_settop(L, 0);

    const std::string directoryPath = std::filesystem::current_path().string();
    ASSERT_EQ(suite, LUA_ERRFILE, luaL_loadfile(L, directoryPath.c_str()),
              "luaL_loadfile rejects an existing directory as a file error");
    const char* directoryError = lua_tostring(L, -1);
    ASSERT_TRUE(suite, directoryError != nullptr && std::strstr(directoryError, "cannot read") != nullptr,
                "luaL_loadfile distinguishes an unreadable directory from a missing path");

    lua_close(L);
}

void testRegistryReferences(TestSuite& suite) {
    lua_State* L = lua_open();

    lua_pushnil(L);
    ASSERT_EQ(suite, LUA_REFNIL, luaL_ref(L, LUA_REGISTRYINDEX), "luaL_ref maps nil to LUA_REFNIL");
    ASSERT_EQ(suite, 0, lua_gettop(L), "luaL_ref consumes a nil value");

    lua_pushstring(L, "first");
    const int first = luaL_ref(L, LUA_REGISTRYINDEX);
    ASSERT_TRUE(suite, first > 0, "luaL_ref allocates a positive registry reference");
    luaL_getref(L, first);
    ASSERT_EQ(suite, std::string("first"), std::string(lua_tostring(L, -1)), "luaL_getref retrieves stored value");
    lua_pop(L, 1);

    luaL_unref(L, LUA_REGISTRYINDEX, first);
    lua_pushstring(L, "second");
    const int second = luaL_ref(L, LUA_REGISTRYINDEX);
    ASSERT_EQ(suite, first, second, "luaL_ref reuses a released registry reference");
    luaL_getref(L, second);
    ASSERT_EQ(suite, std::string("second"), std::string(lua_tostring(L, -1)),
              "reused registry reference stores the replacement value");

    lua_close(L);
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

    for (const size_t requested : {std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max() - 1}) {
        lua_settop(L, 0);
        gOversizedUserdataSize = requested;
        lua_pushcclosure(L, allocateOversizedApiUserdata, 0);
        ASSERT_EQ(suite, LUA_ERRMEM, lua_pcall(L, 0, 1, 0),
                  "oversized userdata is rejected as a memory error before allocation");
        ASSERT_EQ(suite, 1, lua_gettop(L), "oversized userdata leaves one canonical error object");
        ASSERT_EQ(suite, std::string("not enough memory"), std::string(lua_tostring(L, -1)),
                  "oversized userdata uses the fixed memory error object");
    }

    lua_close(L);
    ASSERT_TRUE(suite, ledger.blocks.empty(), "runtime allocation failures leave close path leak free");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.sizeMismatches,
              "failure rollback preserves allocator size contract");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.unknownFrees, "failure rollback frees each allocator block once");
}

void testPublicResumeAllocationRollback(TestSuite& suite) {
    AllocatorLedger ledger;
    AllocatorProbe probe{&ledger};
    lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
    ASSERT_TRUE(suite, L != nullptr, "resume rollback test creates parent state");

    lua_State* co = lua_newthread(L);
    std::string source = "local ";
    for (int i = 1; i <= 96; ++i) {
        if (i > 1) {
            source += ',';
        }
        source += "v" + std::to_string(i);
    }
    source += " = ";
    for (int i = 1; i <= 96; ++i) {
        if (i > 1) {
            source += ',';
        }
        source += std::to_string(i);
    }
    source += "; return v96";

    ASSERT_EQ(suite, LUA_OK, luaL_loadbuffer(L, source.data(), source.size(), "=resume-oom"),
              "resume rollback test compiles a wide frame");
    lua_xmove(L, co, 1);

    probe.failFromAllocation = probe.allocationAttempts + 1;
    ASSERT_EQ(suite, LUA_ERRMEM, lua_resume(co, 0), "resume frame allocation failure becomes LUA_ERRMEM");
    ASSERT_EQ(suite, 1, lua_gettop(co), "failed resume exposes one canonical error object");
    ASSERT_EQ(suite, std::string("not enough memory"), std::string(lua_tostring(co, -1)),
              "failed resume uses the fixed memory error object");

    probe.failFromAllocation = 0;
    ASSERT_EQ(suite, LUA_ERRRUN, lua_resume(co, 0), "allocation-failed coroutine is canonical dead state");
    ASSERT_EQ(suite, 1, lua_gettop(co), "dead retry keeps one stable error object");

    lua_close(L);
    ASSERT_TRUE(suite, ledger.blocks.empty(), "resume rollback and close remain leak free");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.sizeMismatches, "resume rollback preserves allocator old sizes");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.unknownFrees, "resume rollback frees each allocation once");
}

std::string makeLuaLevelResumeOomSource(bool wrapped) {
    std::string source = "local function fail_inside_resume() return cpp_resume_failure() end\n";

    if (wrapped) {
        source += R"lua(
            local generator = coroutine.wrap(fail_inside_resume)
            local first_ok, first_error = pcall(generator)
            disarm_allocator_failure()
            local retry_ok, retry_error = pcall(generator)
            return not first_ok,
                   first_error == "not enough memory",
                   not retry_ok and string.find(tostring(retry_error), "dead") ~= nil,
                   coroutine.running() == nil
        )lua";
    } else {
        source += R"lua(
            local inner = coroutine.create(fail_inside_resume)
            local outer
            outer = coroutine.create(function()
                local first_ok, first_error = coroutine.resume(inner)
                disarm_allocator_failure()
                local running_restored = coroutine.running() == outer
                coroutine.yield(first_ok, first_error, coroutine.status(inner), running_restored)
                return "outer-done"
            end)

            local outer_ok, first_ok, first_error, inner_status, running_restored = coroutine.resume(outer)
            local retry_ok, retry_error = coroutine.resume(inner)
            local outer_retry_ok, outer_result = coroutine.resume(outer)
            return outer_ok,
                   not first_ok,
                   first_error == "not enough memory",
                   inner_status == "dead",
                   running_restored,
                   not retry_ok and string.find(tostring(retry_error), "dead") ~= nil,
                   outer_retry_ok and outer_result == "outer-done",
                   coroutine.status(outer) == "dead",
                   coroutine.running() == nil
        )lua";
    }
    return source;
}

void testLuaLevelCoroutinePersistentAllocationRollback(TestSuite& suite) {
    for (const bool wrapped : {false, true}) {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        ASSERT_TRUE(suite, L != nullptr, "Lua-level resume OOM test creates state");
        luaL_openlibs(L);

        lua_pushcclosure(L, disarmAllocatorFailure, 0);
        lua_setglobal(L, "disarm_allocator_failure");
        lua_pushcclosure(L, throwRuntimeErrorDuringResume, 0);
        lua_setglobal(L, "cpp_resume_failure");
        gAllocatorFailureProbe = &probe;

        const std::string source = makeLuaLevelResumeOomSource(wrapped);
        ASSERT_EQ(suite, LUA_OK, luaL_loadbuffer(L, source.data(), source.size(), "=lua-level-resume-oom"),
                  "Lua-level resume OOM scenario compiles");
        const int expectedResults = wrapped ? 4 : 9;
        const int status = lua_pcall(L, 0, expectedResults, 0);
        const size_t persistentFailureTarget = gArmedAllocatorFailureTarget;
        probe.failFromAllocation = 0;
        probe.failOnAllocation = 0;
        probe.failOnCall = 0;
        gAllocatorFailureProbe = nullptr;
        gArmedAllocatorFailureTarget = 0;

        ASSERT_EQ(suite, LUA_OK, status, "persistent allocator failure does not escape Lua-level resume");
        ASSERT_TRUE(suite, persistentFailureTarget != 0 && probe.allocationAttempts >= persistentFailureTarget,
                    "Lua-level resume reaches the injected persistent allocator failure");
        ASSERT_EQ(suite, expectedResults, lua_gettop(L), "Lua-level OOM scenario returns every invariant");
        for (int result = 1; result <= expectedResults; ++result) {
            ASSERT_TRUE(suite, lua_toboolean(L, result) != 0,
                        "Lua-level OOM leaves coroutine and caller state canonical");
        }

        lua_close(L);
        ASSERT_TRUE(suite, ledger.blocks.empty(), "Lua-level resume OOM rollback closes without leaks");
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.sizeMismatches,
                  "Lua-level resume OOM preserves allocator sizes");
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.unknownFrees,
                  "Lua-level resume OOM frees each allocator block once");
    }
}

void testLoadBufferAllocatorFailures(TestSuite& suite) {
    constexpr const char* source = "local t = {}; for i = 1, 8 do t[i] = i end; return t[8]";

    AllocatorLedger baselineLedger;
    AllocatorProbe baselineProbe{&baselineLedger};
    lua_State* baseline = lua_newstate(trackingLuaAllocator, &baselineProbe);
    ASSERT_TRUE(suite, baseline != nullptr, "loadbuffer failure scan creates baseline state");
    const size_t attemptsBeforeLoad = baselineProbe.allocationAttempts;
    ASSERT_EQ(suite, LUA_OK, luaL_loadbuffer(baseline, source, std::strlen(source), "=oom-loadbuffer"),
              "baseline loadbuffer succeeds");
    const size_t loadAllocationAttempts = baselineProbe.allocationAttempts - attemptsBeforeLoad;
    ASSERT_TRUE(suite, loadAllocationAttempts > 0, "loadbuffer baseline observes allocator traffic");
    lua_close(baseline);
    ASSERT_TRUE(suite, baselineLedger.blocks.empty(), "baseline loadbuffer state closes without leaks");

    for (size_t offset = 1; offset <= loadAllocationAttempts; ++offset) {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        ASSERT_TRUE(suite, L != nullptr, "loadbuffer failure scan creates state");

        probe.failOnAllocation = probe.allocationAttempts + offset;
        const int status = luaL_loadbuffer(L, source, std::strlen(source), "=oom-loadbuffer");
        ASSERT_EQ(suite, LUA_ERRMEM, status, "every injected loader allocation failure remains LUA_ERRMEM");
        ASSERT_EQ(suite, 1, lua_gettop(L), "loader allocation failure leaves one error object");
        ASSERT_EQ(suite, std::string("not enough memory"), std::string(lua_tostring(L, -1)),
                  "loader allocation failure uses the fixed memory error object");

        probe.failOnAllocation = 0;
        lua_settop(L, 0);
        ASSERT_EQ(suite, LUA_OK, luaL_loadbuffer(L, source, std::strlen(source), "=oom-loadbuffer"),
                  "state remains usable after loader allocation failure");
        ASSERT_EQ(suite, LUA_OK, lua_pcall(L, 0, 1, 0), "recovered loader result executes");
        ASSERT_EQ(suite, 8.0, lua_tonumber(L, -1), "recovered loader result is correct");

        lua_close(L);
        ASSERT_TRUE(suite, ledger.blocks.empty(), "loader failure rollback and close remain leak free");
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.sizeMismatches,
                  "loader failure rollback preserves allocator old sizes");
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.unknownFrees, "loader failure rollback frees each block once");
    }
}

void testLoadersPublishMemoryErrorFromFullStack(TestSuite& suite) {
    AllocatorLedger ledger;
    AllocatorProbe probe{&ledger};
    lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
    ASSERT_TRUE(suite, L != nullptr, "full-stack loader failure test creates state");

    auto* state = reinterpret_cast<Lua::LuaState*>(L);
    const Lua::usize frameBase = state->getCurrentCallInfo().base;
    const int base = static_cast<int>(state->getStack().capacity() - frameBase - 1);
    lua_settop(L, base - 1);
    int prefixMarker = 0;
    lua_pushlightuserdata(L, &prefixMarker);
    ASSERT_EQ(suite, base, lua_gettop(L), "loader test fills every ordinary stack slot");

    bool exceptionEscaped = false;
    int status = -1;
    probe.failFromAllocation = probe.allocationAttempts + 1;
    try {
        status = luaL_loadbuffer(L, "return 1", sizeof("return 1") - 1, "=full-stack-oom");
    } catch (...) {
        exceptionEscaped = true;
    }
    ASSERT_TRUE(suite, !exceptionEscaped, "luaL_loadbuffer contains persistent allocator failure");
    ASSERT_EQ(suite, LUA_ERRMEM, status, "luaL_loadbuffer returns LUA_ERRMEM from a full stack");
    ASSERT_EQ(suite, base + 1, lua_gettop(L), "luaL_loadbuffer preserves prefix and appends its error object");
    ASSERT_TRUE(suite, lua_touserdata(L, -2) == &prefixMarker, "luaL_loadbuffer preserves the full stack prefix");
    ASSERT_EQ(suite, std::string("not enough memory"), std::string(lua_tostring(L, -1)),
              "luaL_loadbuffer publishes the fixed memory error without allocating");

    probe.failFromAllocation = 0;
    lua_settop(L, base);
    probe.failFromAllocation = probe.allocationAttempts + 1;
    exceptionEscaped = false;
    status = -1;
    try {
        status = luaL_loadfile(L, "full-stack-oom.lua");
    } catch (...) {
        exceptionEscaped = true;
    }
    ASSERT_TRUE(suite, !exceptionEscaped, "luaL_loadfile contains persistent allocator failure");
    ASSERT_EQ(suite, LUA_ERRMEM, status, "luaL_loadfile returns LUA_ERRMEM from a full stack");
    ASSERT_EQ(suite, base + 1, lua_gettop(L), "luaL_loadfile preserves prefix and appends its error object");
    ASSERT_TRUE(suite, lua_touserdata(L, -2) == &prefixMarker, "luaL_loadfile preserves the full stack prefix");
    ASSERT_EQ(suite, std::string("not enough memory"), std::string(lua_tostring(L, -1)),
              "luaL_loadfile publishes the fixed memory error without allocating");

    probe.failFromAllocation = 0;
    lua_settop(L, base);
    ReaderProbe reader;
    probe.failFromAllocation = probe.allocationAttempts + 1;
    exceptionEscaped = false;
    status = -1;
    try {
        status = lua_load(L, readProbeChunk, &reader, "=full-stack-reader-oom");
    } catch (...) {
        exceptionEscaped = true;
    }
    ASSERT_TRUE(suite, !exceptionEscaped, "lua_load contains persistent allocator failure");
    ASSERT_EQ(suite, LUA_ERRMEM, status, "lua_load returns LUA_ERRMEM from a full stack");
    ASSERT_EQ(suite, base + 1, lua_gettop(L), "lua_load preserves prefix and appends its error object");
    ASSERT_TRUE(suite, lua_touserdata(L, -2) == &prefixMarker, "lua_load preserves the full stack prefix");
    ASSERT_EQ(suite, std::string("not enough memory"), std::string(lua_tostring(L, -1)),
              "lua_load publishes the fixed memory error without allocating");

    probe.failFromAllocation = 0;
    lua_close(L);
    ASSERT_TRUE(suite, ledger.blocks.empty(), "full-stack loader failures remain leak free");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.sizeMismatches,
              "full-stack loader rollback preserves allocator old sizes");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.unknownFrees,
              "full-stack loader rollback frees each allocation once");
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
    registry.registerTest(kSuiteName, "stack shrink releases GC roots", testApiStackShrinkReleasesGcRoots);
    registry.registerTest(kSuiteName, "loader rollback releases GC roots", testLoaderMemoryRollbackReleasesGcRoots);
    registry.registerTest(kSuiteName, "resume bridge releases GC roots", testResumeBridgeRollbackReleasesGcRoots);
    registry.registerTest(kSuiteName, "checkstack and xmove", testCheckStackAndXMove);
    registry.registerTest(kSuiteName, "C++ API exception contract", testCppApiExceptionContract);
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
    registry.registerTest(kSuiteName, "public thread C function entry", testPublicThreadCFunctionEntry);
    registry.registerTest(kSuiteName, "public thread yield and error API", testPublicThreadYieldAndErrorApi);
    registry.registerTest(kSuiteName, "public thread allocator lifecycle", testPublicThreadAllocatorLifecycle);
    registry.registerTest(kSuiteName, "public resume allocation rollback", testPublicResumeAllocationRollback);
    registry.registerTest(kSuiteName, "Lua-level coroutine persistent allocation rollback",
                          testLuaLevelCoroutinePersistentAllocationRollback);
    registry.registerTest(kSuiteName, "nested C to Lua to C protected calls", testNestedCToLuaToCProtectedCalls);
    registry.registerTest(kSuiteName, "custom allocator lifecycle", testCustomAllocatorLifecycle);
    registry.registerTest(kSuiteName, "allocator replacement", testAllocatorCanBeReplaced);
    registry.registerTest(kSuiteName, "allocator failure paths", testAllocatorFailurePaths);
    registry.registerTest(kSuiteName, "loadbuffer allocator failures", testLoadBufferAllocatorFailures);
    registry.registerTest(kSuiteName, "full-stack loader memory errors", testLoadersPublishMemoryErrorFromFullStack);
    registry.registerTest(kSuiteName, "load dump and auxiliary loaders", testLoadDumpAndAuxiliaryLoaders);
    registry.registerTest(kSuiteName, "registry references", testRegistryReferences);
}
