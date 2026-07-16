#include "../framework/test_framework.hpp"

#include "common/lua_error.hpp"
#include "compiler/codegen/codegen.hpp"
#include "compiler/parser/parser.hpp"
#include "core/function.hpp"
#include "core/table.hpp"
#include "core/thread.hpp"
#include "core/upvalue.hpp"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "runtime/runtime_services.hpp"
#include "vm/state/lua_state.hpp"

#include <atomic>
#include <array>
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>

using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Lua C API";

static_assert(!noexcept(lua_xmove(nullptr, nullptr, 0)));
static_assert(!noexcept(lua_call(nullptr, 0, 0)));
static_assert(!noexcept(lua_error(nullptr)));
static_assert(!noexcept(lua_equal(nullptr, 0, 0)));
static_assert(!noexcept(lua_rawequal(nullptr, 0, 0)));
static_assert(!noexcept(lua_lessthan(nullptr, 0, 0)));
static_assert(!noexcept(lua_getfield(nullptr, 0, nullptr)));
static_assert(!noexcept(lua_setfield(nullptr, 0, nullptr)));
static_assert(!noexcept(lua_rawget(nullptr, 0)));
static_assert(!noexcept(lua_rawset(nullptr, 0)));
static_assert(!noexcept(lua_next(nullptr, 0)));
static_assert(!noexcept(lua_concat(nullptr, 0)));
static_assert(!noexcept(lua_tointeger(nullptr, 0)));
static_assert(!noexcept(lua_tocfunction(nullptr, 0)));
static_assert(!noexcept(lua_tothread(nullptr, 0)));
static_assert(!noexcept(lua_topointer(nullptr, 0)));
static_assert(!noexcept(lua_pushthread(nullptr)));
static_assert(!noexcept(lua_gc(nullptr, 0, 0)));
static_assert(!noexcept(luaL_openlib(nullptr, nullptr, nullptr, 0)));
static_assert(!noexcept(luaL_register(nullptr, nullptr, nullptr)));
static_assert(!noexcept(luaL_getmetafield(nullptr, 0, nullptr)));
static_assert(!noexcept(luaL_callmeta(nullptr, 0, nullptr)));
static_assert(!noexcept(luaL_typerror(nullptr, 0, nullptr)));
static_assert(!noexcept(luaL_optlstring(nullptr, 0, nullptr, nullptr)));
static_assert(!noexcept(luaL_optnumber(nullptr, 0, 0)));
static_assert(!noexcept(luaL_checkinteger(nullptr, 0)));
static_assert(!noexcept(luaL_optinteger(nullptr, 0, 0)));
static_assert(!noexcept(luaL_checkstack(nullptr, 0, nullptr)));
static_assert(!noexcept(luaL_checktype(nullptr, 0, 0)));
static_assert(!noexcept(luaL_checkany(nullptr, 0)));
static_assert(!noexcept(luaL_newmetatable(nullptr, nullptr)));
static_assert(!noexcept(luaL_checkudata(nullptr, 0, nullptr)));
static_assert(!noexcept(luaL_where(nullptr, 0)));
static_assert(!noexcept(luaL_checkoption(nullptr, 0, nullptr, nullptr)));
static_assert(!noexcept(luaL_newstate()));
static_assert(!noexcept(luaL_gsub(nullptr, nullptr, nullptr, nullptr)));
static_assert(!noexcept(luaL_findtable(nullptr, 0, nullptr, 0)));
static_assert(!noexcept(luaL_buffinit(nullptr, nullptr)));
static_assert(!noexcept(luaL_prepbuffer(nullptr)));
static_assert(!noexcept(luaL_addlstring(nullptr, nullptr, 0)));
static_assert(!noexcept(luaL_addstring(nullptr, nullptr)));
static_assert(!noexcept(luaL_addvalue(nullptr)));
static_assert(!noexcept(luaL_pushresult(nullptr)));
static_assert(!noexcept(luaL_error(nullptr, "%s", "error")));
static_assert(!noexcept(lua_getstack(nullptr, 0, nullptr)));
static_assert(!noexcept(lua_getinfo(nullptr, nullptr, nullptr)));
static_assert(!noexcept(lua_getlocal(nullptr, nullptr, 0)));
static_assert(!noexcept(lua_setlocal(nullptr, nullptr, 0)));
static_assert(!noexcept(lua_sethook(nullptr, nullptr, 0, 0)));
static_assert(!noexcept(lua_gethook(nullptr)));
static_assert(!noexcept(lua_gethookmask(nullptr)));
static_assert(!noexcept(lua_gethookcount(nullptr)));
static_assert(!noexcept(luaopen_base(nullptr)));
static_assert(!noexcept(luaopen_table(nullptr)));
static_assert(!noexcept(luaopen_io(nullptr)));
static_assert(!noexcept(luaopen_os(nullptr)));
static_assert(!noexcept(luaopen_string(nullptr)));
static_assert(!noexcept(luaopen_math(nullptr)));
static_assert(!noexcept(luaopen_debug(nullptr)));
static_assert(!noexcept(luaopen_package(nullptr)));
static_assert(!noexcept(lua_atpanic(nullptr, nullptr)));
static_assert(!noexcept(lua_pushvfstring(nullptr, nullptr, std::declval<va_list>())));
static_assert(!noexcept(lua_pushfstring(nullptr, nullptr)));
static_assert(!noexcept(lua_getfenv(nullptr, 0)));
static_assert(!noexcept(lua_setfenv(nullptr, 0)));
static_assert(noexcept(lua_cpcall(nullptr, nullptr, nullptr)));
static_assert(!noexcept(lua_setlevel(nullptr, nullptr)));
static_assert(noexcept(lua_close(nullptr)));
static_assert(noexcept(lua_checkstack(nullptr, 0)));
static_assert(!noexcept(lua_checkexecution(nullptr)));
static_assert(noexcept(lua_pcall(nullptr, 0, 0, 0)));
static_assert(!noexcept(lua_newthread(nullptr)));
static_assert(noexcept(lua_trynewthread(nullptr)));
static_assert(noexcept(lua_resume(nullptr, 0)));
static_assert(noexcept(lua_load(nullptr, nullptr, nullptr, nullptr)));
static_assert(noexcept(lua_dump(nullptr, nullptr, nullptr)));
static_assert(noexcept(luaL_loadbuffer(nullptr, nullptr, 0, nullptr)));
static_assert(noexcept(luaL_loadstring(nullptr, nullptr)));
static_assert(noexcept(luaL_loadfile(nullptr, nullptr)));
static_assert(sizeof(((lua_Debug*)nullptr)->short_src) == LUA_IDSIZE);
static_assert(offsetof(lua_Debug, name) > offsetof(lua_Debug, event));
static_assert(offsetof(lua_Debug, currentline) > offsetof(lua_Debug, source));
static_assert(offsetof(lua_Debug, i_ci) > offsetof(lua_Debug, short_src));

constexpr const char* kProtectedApiExceptionMessage = "unhandled C++ exception in protected Lua API";

int gApiFinalizerCalls = 0;
int gApiFinalizerPayload = 0;
int gCloseFinalizerCalls = 0;
int gCloseFinalizerPayload = 0;
int gFailingCloseFinalizerCalls = 0;
int gApiErrorToken = 0;
int gApiHandlerCalls = 0;
void* gApiHandlerErrorObject = nullptr;
struct AllocatorProbe;
AllocatorProbe* gAllocatorFailureProbe = nullptr;
size_t gAllocatorFailureOffset = 0;
size_t gOversizedUserdataSize = 0;
size_t gArmedAllocatorFailureTarget = 0;
int gPublicHookCalls = 0;
int gPublicHookReturns = 0;
int gPublicHookLines = 0;
int gPublicHookCounts = 0;
bool gPublicHookLineInfo = false;
bool gPublicHookTailInfo = false;
bool gPublicDebugCurrentFrame = false;
bool gPublicDebugCallerFrame = false;
bool gPublicDebugCallerFunction = false;
bool gPublicDebugCallerLines = false;
std::string gPublicDebugLocalName;
lua_Number gPublicDebugLocalValue = 0;
bool gPublicCpcallArgument = false;
std::atomic<bool> gNativeExecutionPollEntered{false};
std::atomic<Lua::u64> gNativeExecutionPollIterations{0};
size_t gGcWorklistFailureOffset = 0;
size_t gGcWorklistAllocationStart = 0;
size_t gGcWorklistAllocationAttempts = 0;
size_t gGcWorklistHardLimit = 0;
bool gGcWorklistUseHardLimit = false;
int gGcWorklistFinalizerCalls = 0;
size_t gTableHashFailureOffset = 0;
size_t gTableHashAllocationStart = 0;
size_t gTableHashHardLimit = 0;
bool gTableHashUseHardLimit = false;
int gTableHashLightKey = 0;
size_t gFragmentedReaderFailureOffset = 0;
size_t gFragmentedReaderAllocationStart = 0;
size_t gFragmentedReaderBufferAttempts = 0;
size_t gFragmentedReaderHardLimit = 0;
bool gFragmentedReaderUseHardLimit = false;
bool gCallInfoUseHardLimit = false;
size_t gCallInfoAllocationStart = 0;
size_t gCallInfoHardLimit = 0;
size_t gMetacallFailureOffset = 0;
size_t gMetacallAllocationStart = 0;
size_t gMetacallHardLimit = 0;
bool gMetacallUseHardLimit = false;
size_t gConcatFailureOffset = 0;
size_t gConcatAllocationStart = 0;
size_t gConcatHardLimit = 0;
bool gConcatUseHardLimit = false;
size_t gTableConcatFailureOffset = 0;
size_t gTableConcatAllocationStart = 0;
size_t gTableConcatHardLimit = 0;
bool gTableConcatUseHardLimit = false;
size_t gTableSortFailureOffset = 0;
size_t gTableSortAllocationStart = 0;
size_t gTableSortHardLimit = 0;
bool gTableSortUseHardLimit = false;

struct AllocatorLedger {
    std::unordered_map<std::uintptr_t, size_t> blocks;
    size_t sizeMismatches = 0;
    size_t unknownFrees = 0;
    size_t liveBytes = 0;
    size_t peakBytes = 0;
    size_t hardLimit = std::numeric_limits<size_t>::max();
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
                probe->ledger->liveBytes -= existing->second;
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

    size_t trackedOldSize = 0;
    if (pointer != nullptr) {
        auto existing = probe->ledger->blocks.find(oldKey);
        if (existing != probe->ledger->blocks.end()) {
            trackedOldSize = existing->second;
            if (trackedOldSize != oldSize) {
                ++probe->ledger->sizeMismatches;
            }
        }
    }

    const size_t retainedLiveBytes = probe->ledger->liveBytes - trackedOldSize;
    if (newSize > probe->ledger->hardLimit || retainedLiveBytes > probe->ledger->hardLimit - newSize) {
        return nullptr;
    }

    void* result = std::realloc(pointer, newSize);
    if (result == nullptr) {
        return nullptr;
    }

    if (pointer != nullptr) {
        probe->ledger->blocks.erase(oldKey);
        probe->ledger->liveBytes -= trackedOldSize;
        ++probe->reallocations;
    } else {
        ++probe->allocations;
    }
    probe->ledger->blocks[reinterpret_cast<std::uintptr_t>(result)] = newSize;
    probe->ledger->liveBytes += newSize;
    if (probe->ledger->liveBytes > probe->ledger->peakBytes) {
        probe->ledger->peakBytes = probe->ledger->liveBytes;
    }
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

int pollNativeExecutionOnce(lua_State* L) {
    lua_checkexecution(L);
    return 0;
}

int pollNativeExecutionUntilCancelled(lua_State* L) {
    gNativeExecutionPollEntered.store(true, std::memory_order_release);
    for (;;) {
        gNativeExecutionPollIterations.fetch_add(1, std::memory_order_relaxed);
        lua_checkexecution(L);
    }
}

int countGcWorklistFinalizer(lua_State*) {
    ++gGcWorklistFinalizerCalls;
    return 0;
}

int collectGcWorklists(lua_State* L) {
    AllocatorProbe* probe = gAllocatorFailureProbe;
    gGcWorklistAllocationStart = probe != nullptr ? probe->allocationAttempts : 0;
    gGcWorklistAllocationAttempts = 0;
    if (probe != nullptr) {
        probe->failOnAllocation =
            gGcWorklistFailureOffset == 0 ? 0 : probe->allocationAttempts + gGcWorklistFailureOffset;
        if (gGcWorklistUseHardLimit) {
            probe->ledger->hardLimit = probe->ledger->liveBytes;
            probe->ledger->peakBytes = probe->ledger->liveBytes;
            gGcWorklistHardLimit = probe->ledger->hardLimit;
        }
    }

    (void)lua_gc(L, LUA_GCCOLLECT, 0);
    if (probe != nullptr) {
        gGcWorklistAllocationAttempts = probe->allocationAttempts - gGcWorklistAllocationStart;
    }
    return 0;
}

void prepareGcWorklistFixture(lua_State* L) {
    lua_settop(L, 0);
    (void)lua_gc(L, LUA_GCSTOP, 0);

    // Keep a broad graph alive so the collector must populate both its gray
    // queue and weak-table worklist during the explicit collection.
    lua_newtable(L);
    for (int index = 1; index <= 48; ++index) {
        lua_newtable(L);
        if (index == 1) {
            lua_newtable(L);
            lua_pushstring(L, "v");
            lua_setfield(L, -2, "__mode");
            (void)lua_setmetatable(L, -2);
        }
        lua_rawseti(L, 1, index);
    }

    // Leave one finalizable userdata unreachable. A successful cycle must
    // allocate the pending-finalizer queue and its reentrancy-safe drain copy.
    (void)lua_newuserdata(L, 0);
    lua_newtable(L);
    lua_pushcclosure(L, countGcWorklistFinalizer, 0);
    lua_setfield(L, -2, "__gc");
    (void)lua_setmetatable(L, -2);
    lua_pop(L, 1);
}

bool gcWorklistFixtureRootIsUsable(lua_State* L) {
    if (lua_gettop(L) < 1 || lua_istable(L, 1) == 0) {
        return false;
    }
    lua_rawgeti(L, 1, 1);
    const bool usable = lua_istable(L, -1) != 0;
    lua_pop(L, 1);
    return usable;
}

void armTableHashAllocatorFailure() {
    AllocatorProbe* probe = gAllocatorFailureProbe;
    gTableHashAllocationStart = probe != nullptr ? probe->allocationAttempts : 0;
    if (probe == nullptr) {
        return;
    }

    probe->failOnAllocation = gTableHashFailureOffset == 0 ? 0 : probe->allocationAttempts + gTableHashFailureOffset;
    if (gTableHashUseHardLimit) {
        probe->ledger->hardLimit = probe->ledger->liveBytes;
        probe->ledger->peakBytes = probe->ledger->liveBytes;
        gTableHashHardLimit = probe->ledger->hardLimit;
    }
}

int writeRawTableHash(lua_State* L) {
    armTableHashAllocatorFailure();
    lua_pushlightuserdata(L, &gTableHashLightKey);
    lua_pushnumber(L, 73);
    lua_rawset(L, 1);
    return 0;
}

int armVmTableHashFailure(lua_State*) {
    armTableHashAllocatorFailure();
    return 0;
}

bool rawTableHashValueEquals(lua_State* L, int tableIndex, lua_Number expected) {
    lua_pushlightuserdata(L, &gTableHashLightKey);
    lua_rawget(L, tableIndex);
    const bool matches = lua_isnumber(L, -1) != 0 && lua_tonumber(L, -1) == expected;
    lua_pop(L, 1);
    return matches;
}

bool rawTableHashValueIsNil(lua_State* L, int tableIndex) {
    lua_pushlightuserdata(L, &gTableHashLightKey);
    lua_rawget(L, tableIndex);
    const bool isNil = lua_isnil(L, -1) != 0;
    lua_pop(L, 1);
    return isNil;
}

constexpr const char* kVmTableHashSource =
    "__arm_table_hash(); __allocator_target['__vm_hash_key'] = 91; return __allocator_target['__vm_hash_key']";

void prepareVmTableHashFixture(lua_State* L) {
    lua_settop(L, 0);
    (void)lua_gc(L, LUA_GCSTOP, 0);
    lua_pushcclosure(L, armVmTableHashFailure, 0);
    lua_setglobal(L, "__arm_table_hash");
    lua_newtable(L);
    lua_setglobal(L, "__allocator_target");
    if (luaL_loadstring(L, kVmTableHashSource) != LUA_OK) {
        throw std::runtime_error("failed to compile VM table-hash allocator fixture");
    }
}

bool vmTableHashValueEquals(lua_State* L, lua_Number expected) {
    lua_getglobal(L, "__allocator_target");
    lua_pushstring(L, "__vm_hash_key");
    lua_rawget(L, -2);
    const bool matches = lua_isnumber(L, -1) != 0 && lua_tonumber(L, -1) == expected;
    lua_pop(L, 2);
    return matches;
}

bool vmTableHashValueIsNil(lua_State* L) {
    lua_getglobal(L, "__allocator_target");
    lua_pushstring(L, "__vm_hash_key");
    lua_rawget(L, -2);
    const bool isNil = lua_isnil(L, -1) != 0;
    lua_pop(L, 2);
    return isNil;
}

int firstPublicPanic(lua_State*) {
    return 0;
}

int secondPublicPanic(lua_State*) {
    return 0;
}

const char* pushPublicVFormat(lua_State* L, const char* format, ...) {
    va_list arguments;
    va_start(arguments, format);
    const char* result = lua_pushvfstring(L, format, arguments);
    va_end(arguments);
    return result;
}

int capturePublicCpcallArgument(lua_State* L) {
    gPublicCpcallArgument = lua_gettop(L) == 1 && lua_touserdata(L, 1) == &gApiErrorToken;
    return 0;
}

int failPublicCpcall(lua_State* L) {
    lua_pushlightuserdata(L, &gApiErrorToken);
    return lua_error(L);
}

int callLuaOpenBase(lua_State* L) {
    return luaopen_base(L);
}

int callLuaOpenTable(lua_State* L) {
    return luaopen_table(L);
}

int callLuaOpenIO(lua_State* L) {
    return luaopen_io(L);
}

int callLuaOpenOS(lua_State* L) {
    return luaopen_os(L);
}

int callLuaOpenString(lua_State* L) {
    return luaopen_string(L);
}

int callLuaOpenMath(lua_State* L) {
    return luaopen_math(L);
}

int callLuaOpenDebug(lua_State* L) {
    return luaopen_debug(L);
}

int callLuaOpenPackage(lua_State* L) {
    return luaopen_package(L);
}

void capturePublicDebugHook(lua_State* L, lua_Debug* ar) {
    if (ar == nullptr) {
        return;
    }

    switch (ar->event) {
    case LUA_HOOKCALL:
        ++gPublicHookCalls;
        break;
    case LUA_HOOKRET:
        ++gPublicHookReturns;
        break;
    case LUA_HOOKTAILRET:
        ++gPublicHookReturns;
        gPublicHookTailInfo = lua_getinfo(L, "S", ar) != 0 && std::strcmp(ar->what, "tail") == 0;
        break;
    case LUA_HOOKLINE:
        ++gPublicHookLines;
        gPublicHookLineInfo = lua_getinfo(L, "l", ar) != 0 && ar->currentline > 0;
        break;
    case LUA_HOOKCOUNT:
        ++gPublicHookCounts;
        break;
    default:
        break;
    }
}

int inspectPublicDebugCaller(lua_State* L) {
    lua_Debug current{};
    gPublicDebugCurrentFrame = lua_getstack(L, 0, &current) != 0 && lua_getinfo(L, "Slu", &current) != 0 &&
                               std::strcmp(current.what, "C") == 0 && current.currentline == -1;

    lua_Debug caller{};
    gPublicDebugCallerFrame = lua_getstack(L, 1, &caller) != 0 && lua_getinfo(L, "SlnufL", &caller) != 0 &&
                              std::strcmp(caller.what, "main") == 0 && caller.currentline > 0;
    gPublicDebugCallerFunction = lua_isfunction(L, -2) != 0;
    gPublicDebugCallerLines = lua_istable(L, -1) != 0;
    lua_pop(L, 2);

    const char* localName = lua_getlocal(L, &caller, 1);
    if (localName != nullptr) {
        gPublicDebugLocalName = localName;
        gPublicDebugLocalValue = lua_tonumber(L, -1);
        lua_pop(L, 1);
    }

    lua_pushinteger(L, 19);
    const char* setName = lua_setlocal(L, &caller, 1);
    if (setName == nullptr || gPublicDebugLocalName != setName) {
        gPublicDebugLocalName.clear();
    }
    return 0;
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

int returnFallbackField(lua_State* L) {
    lua_pushstring(L, "fallback");
    return 1;
}

int captureNewField(lua_State* L) {
    lua_pushvalue(L, 2);
    lua_pushvalue(L, 3);
    lua_rawset(L, lua_upvalueindex(1));
    return 0;
}

int compareIdsEqual(lua_State* L) {
    lua_getfield(L, 1, "id");
    lua_getfield(L, 2, "id");
    lua_pushboolean(L, lua_tonumber(L, -2) == lua_tonumber(L, -1));
    return 1;
}

int compareIdsLess(lua_State* L) {
    lua_getfield(L, 1, "id");
    lua_getfield(L, 2, "id");
    lua_pushboolean(L, lua_tonumber(L, -2) < lua_tonumber(L, -1));
    return 1;
}

int returnConcatFallback(lua_State* L) {
    lua_pushstring(L, "joined");
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

int recordCloseUserdataFinalizer(lua_State* L) {
    ++gCloseFinalizerCalls;
    void* payload = lua_touserdata(L, 1);
    if (payload != nullptr && lua_objlen(L, 1) >= sizeof(int)) {
        gCloseFinalizerPayload += *static_cast<int*>(payload);
    }
    return 0;
}

int failCloseUserdataFinalizer(lua_State* L) {
    ++gFailingCloseFinalizerCalls;
    lua_pushstring(L, "intentional close finalizer failure");
    return lua_error(L);
}

void pushCloseFinalizedUserdata(lua_State* L, int payload, lua_CFunction finalizer) {
    void* storage = lua_newuserdata(L, sizeof(payload));
    *static_cast<int*>(storage) = payload;
    lua_newtable(L);
    lua_pushstring(L, "__gc");
    lua_pushcclosure(L, finalizer, 0);
    lua_settable(L, -3);
    (void)lua_setmetatable(L, -2);
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

int raiseAuxiliaryArgumentError(lua_State* L) {
    return luaL_argerror(L, 1, "contract argument failure");
}

int raiseAuxiliaryFormattedError(lua_State* L) {
    return luaL_error(L, "formatted failure %d", 7);
}

int returnAuxiliaryMetaValue(lua_State* L) {
    lua_pushstring(L, "meta-value");
    return 1;
}

int raiseAuxiliaryTypeError(lua_State* L) {
    return luaL_typerror(L, 1, "widget");
}

int requireAuxiliaryValue(lua_State* L) {
    luaL_checkany(L, 1);
    return 0;
}

int requireAuxiliaryOption(lua_State* L) {
    static const char* const options[] = {"alpha", "beta", nullptr};
    lua_pushinteger(L, luaL_checkoption(L, 1, nullptr, options));
    return 1;
}

int requireAuxiliaryUserdata(lua_State* L) {
    (void)luaL_checkudata(L, 1, "aux.widget");
    return 0;
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

int growCallInfoWithArmedAllocator(lua_State* L) {
    auto* state = reinterpret_cast<Lua::LuaState*>(L);
    while (state->getCurrentCI() + 1 < state->getCallStack().size()) {
        (void)state->pushCallInfo();
    }

    AllocatorProbe* probe = gAllocatorFailureProbe;
    if (probe != nullptr) {
        gCallInfoAllocationStart = probe->allocationAttempts;
        if (gCallInfoUseHardLimit) {
            probe->ledger->hardLimit = probe->ledger->liveBytes;
            probe->ledger->peakBytes = probe->ledger->liveBytes;
            gCallInfoHardLimit = probe->ledger->hardLimit;
        } else {
            probe->failOnAllocation = probe->allocationAttempts + 1;
        }
    }

    (void)state->pushCallInfo();
    return 0;
}

int armMetacallAllocatorFailure(lua_State*) {
    AllocatorProbe* probe = gAllocatorFailureProbe;
    gMetacallAllocationStart = probe != nullptr ? probe->allocationAttempts : 0;
    if (probe != nullptr) {
        probe->failOnAllocation = gMetacallFailureOffset == 0 ? 0 : probe->allocationAttempts + gMetacallFailureOffset;
        if (gMetacallUseHardLimit) {
            probe->ledger->hardLimit = probe->ledger->liveBytes;
            probe->ledger->peakBytes = probe->ledger->liveBytes;
            gMetacallHardLimit = probe->ledger->hardLimit;
        }
    }
    return 0;
}

int returnMetacallArgumentCount(lua_State* L) {
    lua_pushinteger(L, lua_gettop(L));
    return 1;
}

int armConcatAllocatorFailure(lua_State*) {
    AllocatorProbe* probe = gAllocatorFailureProbe;
    gConcatAllocationStart = probe != nullptr ? probe->allocationAttempts : 0;
    if (probe != nullptr) {
        probe->failOnAllocation = gConcatFailureOffset == 0 ? 0 : probe->allocationAttempts + gConcatFailureOffset;
        if (gConcatUseHardLimit) {
            probe->ledger->hardLimit = probe->ledger->liveBytes;
            probe->ledger->peakBytes = probe->ledger->liveBytes;
            gConcatHardLimit = probe->ledger->hardLimit;
        }
    }
    return 0;
}

int armTableConcatAllocatorFailure(lua_State*) {
    AllocatorProbe* probe = gAllocatorFailureProbe;
    gTableConcatAllocationStart = probe != nullptr ? probe->allocationAttempts : 0;
    if (probe != nullptr) {
        probe->failOnAllocation =
            gTableConcatFailureOffset == 0 ? 0 : probe->allocationAttempts + gTableConcatFailureOffset;
        if (gTableConcatUseHardLimit) {
            probe->ledger->hardLimit = probe->ledger->liveBytes;
            probe->ledger->peakBytes = probe->ledger->liveBytes;
            gTableConcatHardLimit = probe->ledger->hardLimit;
        }
    }
    return 0;
}

int armTableSortAllocatorFailure(lua_State*) {
    AllocatorProbe* probe = gAllocatorFailureProbe;
    gTableSortAllocationStart = probe != nullptr ? probe->allocationAttempts : 0;
    if (probe != nullptr) {
        probe->failOnAllocation =
            gTableSortFailureOffset == 0 ? 0 : probe->allocationAttempts + gTableSortFailureOffset;
        if (gTableSortUseHardLimit) {
            probe->ledger->hardLimit = probe->ledger->liveBytes;
            probe->ledger->peakBytes = probe->ledger->liveBytes;
            gTableSortHardLimit = probe->ledger->hardLimit;
        }
    }
    return 0;
}

int compareTableSortNumbers(lua_State* L) {
    lua_pushboolean(L, lua_tonumber(L, 1) < lua_tonumber(L, 2));
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

int throwUnknownExceptionWithAllocatorFailure(lua_State*) {
    if (gAllocatorFailureProbe != nullptr) {
        gArmedAllocatorFailureTarget = gAllocatorFailureProbe->allocationAttempts + 1;
        gAllocatorFailureProbe->failFromAllocation = gArmedAllocatorFailureTarget;
    }
    throw 29;
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

constexpr std::string_view kFragmentedAllocatorReaderSource =
    "local values = {}; for i = 1, 32 do values[i] = i * 2 end; return values[32]";

struct FragmentedAllocatorReader {
    size_t position = 0;
    bool armed = false;
};

const char* readFragmentedAllocatorChunk(lua_State*, void* userData, size_t* size) {
    auto* reader = static_cast<FragmentedAllocatorReader*>(userData);
    AllocatorProbe* probe = gAllocatorFailureProbe;
    if (!reader->armed) {
        reader->armed = true;
        gFragmentedReaderAllocationStart = probe != nullptr ? probe->allocationAttempts : 0;
        gFragmentedReaderBufferAttempts = 0;
        if (probe != nullptr) {
            probe->failOnAllocation =
                gFragmentedReaderFailureOffset == 0 ? 0 : probe->allocationAttempts + gFragmentedReaderFailureOffset;
            if (gFragmentedReaderUseHardLimit) {
                probe->ledger->hardLimit = probe->ledger->liveBytes;
                probe->ledger->peakBytes = probe->ledger->liveBytes;
                gFragmentedReaderHardLimit = probe->ledger->hardLimit;
            }
        }
    }

    if (reader->position >= kFragmentedAllocatorReaderSource.size()) {
        if (probe != nullptr) {
            gFragmentedReaderBufferAttempts = probe->allocationAttempts - gFragmentedReaderAllocationStart;
        }
        *size = 0;
        return {};
    }

    constexpr size_t kPieceSize = 7;
    const size_t remaining = kFragmentedAllocatorReaderSource.size() - reader->position;
    *size = remaining < kPieceSize ? remaining : kPieceSize;
    const char* piece = kFragmentedAllocatorReaderSource.data() + reader->position;
    reader->position += *size;
    return piece;
}

const char* pushGcObjectsThenFailReader(lua_State* L, void*, size_t*) {
    lua_createtable(L, 0, 0);
    lua_createtable(L, 0, 0);
    throw std::bad_alloc();
}

const char* throwRuntimeErrorReader(lua_State* L, void*, size_t*) {
    lua_createtable(L, 0, 0);
    throw std::runtime_error("reader callback failed");
}

const char* throwUnknownExceptionReader(lua_State* L, void*, size_t*) {
    lua_createtable(L, 0, 0);
    throw 31;
}

int throwBadAllocWriter(lua_State*, const void*, size_t, void*) {
    throw std::bad_alloc();
}

int throwRuntimeErrorWriter(lua_State*, const void*, size_t, void*) {
    throw std::runtime_error("writer callback failed");
}

int throwUnknownExceptionWriter(lua_State*, const void*, size_t, void*) {
    throw 37;
}

struct ThrowingAllocatorProbe {
    bool armed = false;
};

void* throwingLuaAllocator(void* userData, void* pointer, size_t, size_t newSize) {
    auto* probe = static_cast<ThrowingAllocatorProbe*>(userData);
    if (newSize == 0) {
        std::free(pointer);
        return nullptr;
    }
    if (probe->armed) {
        throw std::runtime_error("allocator callback failed");
    }
    return std::realloc(pointer, newSize);
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

void testNativeCallbackCooperativeExecutionPoll(TestSuite& suite) {
    lua_State* L = lua_open();
    auto* state = reinterpret_cast<Lua::LuaState*>(L);
    Lua::ExecutionPolicy& policy = state->getGlobalState().getExecutionPolicy();

    Lua::ExecutionPolicy::Limits limits;
    limits.instructionBudget = 23;
    policy.configure(limits);
    lua_pushcclosure(L, pollNativeExecutionOnce, 0);
    ASSERT_EQ(suite, LUA_OK, lua_pcall(L, 0, 0, 0),
              "native callback execution poll permits an active execution window");
    ASSERT_EQ(suite, 0, lua_gettop(L), "successful native execution poll does not mutate the caller stack");
    ASSERT_EQ(suite, static_cast<Lua::u64>(23), policy.remainingInstructions(),
              "native execution poll does not consume the Lua instruction budget");
    ASSERT_EQ(suite, static_cast<Lua::u64>(0), policy.consumedInstructions(),
              "native execution poll leaves opcode accounting unchanged");

    limits.deadline = Lua::ExecutionPolicy::Clock::now();
    policy.configure(limits);
    Lua::GCString* deadlineError =
        state->getGlobalState().getExecutionPolicyErrorMessage(Lua::ExecutionStopReason::DeadlineExceeded);
    lua_pushcclosure(L, pollNativeExecutionOnce, 0);
    ASSERT_EQ(suite, LUA_ERRRUN, lua_pcall(L, 0, 0, 0),
              "native callback execution poll reports an expired monotonic deadline");
    ASSERT_TRUE(suite, state->top().isString() && state->top().asString() == deadlineError,
                "deadline poll publishes the fixed preallocated deadline error object");
    ASSERT_EQ(suite, static_cast<Lua::u64>(23), policy.remainingInstructions(),
              "deadline poll does not consume the Lua instruction budget");
    ASSERT_EQ(suite, LUA_OK, lua_status(L), "protected deadline stop restores the public thread status");
    lua_settop(L, 0);

    limits.instructionBudget = 50'000'000;
    limits.deadline = Lua::ExecutionPolicy::Clock::time_point::max();
    policy.configure(limits);
    const Lua::ExecutionCancellationHandle cancellation = policy.cancellationHandle();
    Lua::GCString* cancellationError =
        state->getGlobalState().getExecutionPolicyErrorMessage(Lua::ExecutionStopReason::Cancelled);
    gNativeExecutionPollEntered.store(false, std::memory_order_relaxed);
    gNativeExecutionPollIterations.store(0, std::memory_order_relaxed);

    std::thread canceller([cancellation] {
        while (!gNativeExecutionPollEntered.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        cancellation.requestCancellation();
    });
    lua_pushcclosure(L, pollNativeExecutionUntilCancelled, 0);
    const int status = lua_pcall(L, 0, 0, 0);
    canceller.join();

    ASSERT_EQ(suite, LUA_ERRRUN, status, "foreign atomic cancellation stops a cooperatively polling native callback");
    ASSERT_TRUE(suite, policy.isCancellationRequested(), "owner observes the one-way foreign cancellation request");
    ASSERT_TRUE(suite, gNativeExecutionPollIterations.load(std::memory_order_relaxed) > 0,
                "native callback reaches at least one cooperative execution poll");
    ASSERT_TRUE(suite, state->top().isString() && state->top().asString() == cancellationError,
                "cancellation poll publishes the fixed preallocated cancellation error object");
    ASSERT_TRUE(suite, policy.remainingInstructions() > 0,
                "native cancellation polling does not spend the fallback Lua instruction budget");

    policy.reset();
    lua_close(L);
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

void testLua51KnownValueApiSemantics(TestSuite& suite) {
    lua_State* L = lua_open();

    lua_pushstring(L, nullptr);
    ASSERT_EQ(suite, LUA_TNIL, lua_type(L, -1), "lua_pushstring(nullptr) pushes nil");
    lua_pop(L, 1);

    lua_pushnumber(L, 12345);
    ASSERT_EQ(suite, static_cast<size_t>(5), lua_objlen(L, -1), "lua_objlen converts numbers to strings");
    ASSERT_EQ(suite, LUA_TSTRING, lua_type(L, -1), "numeric lua_objlen preserves Lua 5.1 conversion side effect");
    ASSERT_EQ(suite, std::string("12345"), std::string(lua_tostring(L, -1)),
              "numeric lua_objlen uses the ordinary Lua number representation");

    lua_close(L);
}

void testPreviouslyUnprobedPublicContractSymbols(TestSuite& suite) {
    lua_State* L = lua_open();

    lua_pushinteger(L, 42);
    ASSERT_TRUE(suite, lua_isnumber(L, -1) != 0, "lua_isnumber accepts numeric values");
    ASSERT_EQ(suite, std::string("number"), std::string(lua_typename(L, lua_type(L, -1))),
              "lua_typename exposes the public number type name");
    size_t length = 0;
    ASSERT_EQ(suite, std::string("42"), std::string(lua_tolstring(L, -1, &length)),
              "lua_tolstring converts a public numeric stack slot");
    ASSERT_EQ(suite, static_cast<size_t>(2), length, "lua_tolstring publishes converted byte length");
    lua_pop(L, 1);

    lua_pushboolean(L, 1);
    ASSERT_EQ(suite, LUA_TBOOLEAN, lua_type(L, -1), "lua_pushboolean publishes the boolean Lua type");
    ASSERT_TRUE(suite, lua_toboolean(L, -1) != 0, "lua_pushboolean value round-trips through lua_toboolean");
    lua_pop(L, 1);

    lua_newtable(L);
    lua_pushstring(L, "key");
    lua_pushnumber(L, 17);
    lua_settable(L, -3);
    lua_pushstring(L, "key");
    lua_gettable(L, -2);
    ASSERT_EQ(suite, 17.0, lua_tonumber(L, -1), "lua_gettable reaches the public metamethod-aware table path");
    lua_settop(L, 0);

    lua_pushnumber(L, 12.5);
    ASSERT_EQ(suite, 12.5, luaL_checknumber(L, 1), "luaL_checknumber accepts a public numeric argument");
    lua_pushstring(L, "aux-value");
    length = 0;
    ASSERT_EQ(suite, std::string("aux-value"), std::string(luaL_checklstring(L, 2, &length)),
              "luaL_checklstring returns the public string argument");
    ASSERT_EQ(suite, static_cast<size_t>(9), length, "luaL_checklstring publishes byte length");
    lua_settop(L, 0);

    lua_pushcclosure(L, raiseAuxiliaryArgumentError, 0);
    ASSERT_EQ(suite, LUA_ERRRUN, lua_pcall(L, 0, 1, 0), "luaL_argerror is contained by lua_pcall");
    ASSERT_TRUE(suite, std::string(lua_tostring(L, -1)).find("contract argument failure") != std::string::npos,
                "luaL_argerror preserves its diagnostic");

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

void testCloseFinalizerSemantics(TestSuite& suite) {
    gCloseFinalizerCalls = 0;
    gCloseFinalizerPayload = 0;
    {
        lua_State* L = lua_open();
        pushCloseFinalizedUserdata(L, 101, recordCloseUserdataFinalizer);

        // Keep the userdata reachable.  lua_close finalizes the whole runtime,
        // not only objects that an ordinary collection considers garbage.
        lua_close(L);
    }
    ASSERT_EQ(suite, 1, gCloseFinalizerCalls, "lua_close runs an uncollected reachable userdata finalizer once");
    ASSERT_EQ(suite, 101, gCloseFinalizerPayload, "close-time finalizer receives its userdata payload");

    gCloseFinalizerCalls = 0;
    gCloseFinalizerPayload = 0;
    gFailingCloseFinalizerCalls = 0;
    {
        lua_State* L = lua_open();
        pushCloseFinalizedUserdata(L, 202, recordCloseUserdataFinalizer);
        pushCloseFinalizedUserdata(L, 0, failCloseUserdataFinalizer);
        lua_close(L);
    }
    ASSERT_EQ(suite, 1, gFailingCloseFinalizerCalls, "lua_close invokes a failing finalizer once");
    ASSERT_EQ(suite, 1, gCloseFinalizerCalls, "one close-time finalizer error does not suppress later finalizers");
    ASSERT_EQ(suite, 202, gCloseFinalizerPayload, "later close-time finalizer still receives its payload");

    gCloseFinalizerCalls = 0;
    gCloseFinalizerPayload = 0;
    {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* mainState = lua_newstate(trackingLuaAllocator, &probe);
        ASSERT_TRUE(suite, mainState != nullptr, "coroutine close test creates allocator-backed state");
        pushCloseFinalizedUserdata(mainState, 303, recordCloseUserdataFinalizer);
        lua_State* coroutine = lua_newthread(mainState);
        ASSERT_TRUE(suite, coroutine != nullptr, "coroutine close test creates child state");

        bool closeThrew = false;
        try {
            lua_close(coroutine);
        } catch (...) {
            closeThrew = true;
        }
        ASSERT_TRUE(suite, !closeThrew, "lua_close(coroutine) safely closes the owning runtime");
        ASSERT_EQ(suite, 1, gCloseFinalizerCalls, "coroutine close runs runtime finalizers");
        ASSERT_EQ(suite, 303, gCloseFinalizerPayload, "coroutine close finalizer receives its payload");
        ASSERT_TRUE(suite, ledger.blocks.empty(), "coroutine close releases the main state and every child allocation");
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.liveBytes,
                  "coroutine close returns allocator live bytes to zero");
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.sizeMismatches,
                  "coroutine close preserves allocator old-size contracts");
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.unknownFrees,
                  "coroutine close frees each allocator block once");
    }

    gCloseFinalizerCalls = 0;
    gCloseFinalizerPayload = 0;
    {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        ASSERT_TRUE(suite, L != nullptr, "persistent-OOM close test creates allocator-backed state");
        pushCloseFinalizedUserdata(L, 404, recordCloseUserdataFinalizer);
        probe.failFromAllocation = probe.allocationAttempts + 1;

        bool closeThrew = false;
        try {
            lua_close(L);
        } catch (...) {
            closeThrew = true;
        }
        ASSERT_TRUE(suite, !closeThrew, "persistent allocator OOM cannot escape lua_close");
        ASSERT_EQ(suite, 1, gCloseFinalizerCalls,
                  "already-owned close stack storage runs a C finalizer during persistent OOM");
        ASSERT_EQ(suite, 404, gCloseFinalizerPayload, "persistent-OOM finalizer receives its payload");
        ASSERT_TRUE(suite, ledger.blocks.empty(), "persistent-OOM close releases every allocator-backed block");
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.liveBytes,
                  "persistent-OOM close returns allocator live bytes to zero");
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.sizeMismatches,
                  "persistent-OOM close preserves allocator old-size contracts");
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.unknownFrees,
                  "persistent-OOM close frees each allocator block once");
    }

    gCloseFinalizerCalls = 0;
    gCloseFinalizerPayload = 0;
    {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        ASSERT_TRUE(suite, L != nullptr, "budgeted close test creates allocator-backed state");
        auto* state = reinterpret_cast<Lua::LuaState*>(L);
        Lua::ExecutionPolicy::Limits limits;
        limits.finalizerBudgetPerDrain = 1;
        state->getGlobalState().getExecutionPolicy().configure(limits);

        pushCloseFinalizedUserdata(L, 501, recordCloseUserdataFinalizer);
        pushCloseFinalizedUserdata(L, 502, recordCloseUserdataFinalizer);
        pushCloseFinalizedUserdata(L, 503, recordCloseUserdataFinalizer);

        bool closeThrew = false;
        try {
            lua_close(L);
        } catch (...) {
            closeThrew = true;
        }
        ASSERT_TRUE(suite, !closeThrew, "finite finalizer budget preserves lua_close noexcept cleanup");
        ASSERT_EQ(suite, 1, gCloseFinalizerCalls, "lua_close enters no more than the configured finalizer budget");
        ASSERT_TRUE(suite, ledger.blocks.empty(), "budgeted close still releases every allocator-backed block");
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.liveBytes,
                  "budgeted close returns allocator live bytes to zero");
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.sizeMismatches,
                  "budgeted close preserves allocator old-size contracts");
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.unknownFrees, "budgeted close frees each allocator block once");
    }

    {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        ASSERT_TRUE(suite, L != nullptr, "owner-thread close test creates allocator-backed state");

        bool foreignCloseReturned = false;
        std::thread foreign([&] {
            lua_close(L);
            foreignCloseReturned = true;
        });
        foreign.join();

        ASSERT_TRUE(suite, foreignCloseReturned, "foreign lua_close is rejected without throwing");
        ASSERT_TRUE(suite, !ledger.blocks.empty(), "foreign lua_close does not destroy owner-thread storage");
        ASSERT_EQ(suite, 0, lua_gettop(L), "owner can still access the state after rejected foreign close");

        lua_close(L);
        ASSERT_TRUE(suite, ledger.blocks.empty(), "owner-thread lua_close releases every allocator block");
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.liveBytes,
                  "owner-thread close returns allocator live bytes to zero");
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.sizeMismatches,
                  "owner-thread close preserves allocator old-size contracts");
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.unknownFrees,
                  "owner-thread close frees each allocator block once");
    }
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
    ASSERT_TRUE(suite, lua_trynewthread(nullptr) == nullptr, "lua_trynewthread safely rejects a null parent");

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

    lua_State* safeCo = lua_trynewthread(L);
    ASSERT_TRUE(suite, safeCo != nullptr, "lua_trynewthread creates a child when allocation succeeds");
    ASSERT_TRUE(suite, lua_isthread(L, -1) != 0, "lua_trynewthread publishes its child on the parent stack");

    lua_close(L);
    ASSERT_TRUE(suite, ledger.blocks.empty(), "closing parent releases child state allocation");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.sizeMismatches,
              "child state frees preserve allocator old-size contract");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.unknownFrees, "child state allocations are each released once");

    auto runConstructionFailure = [&](size_t failureOffset, bool safeApi) {
        AllocatorLedger failedLedger;
        AllocatorProbe failedProbe{&failedLedger};
        lua_State* failureParent = lua_newstate(trackingLuaAllocator, &failedProbe);
        ASSERT_TRUE(suite, failureParent != nullptr, "thread failure test creates parent state");
        const int parentTop = lua_gettop(failureParent);
        auto* parentState = reinterpret_cast<Lua::LuaState*>(failureParent);
        const size_t objectCount = parentState->getGlobalState().getGC().getObjectCount();
        const size_t blockCount = failedLedger.blocks.size();
        failedProbe.failOnAllocation = failedProbe.allocationAttempts + failureOffset;

        if (safeApi) {
            bool exceptionEscaped = false;
            lua_State* result = nullptr;
            try {
                result = lua_trynewthread(failureParent);
            } catch (...) {
                exceptionEscaped = true;
            }
            ASSERT_TRUE(suite, !exceptionEscaped, "lua_trynewthread contains each construction failure");
            ASSERT_TRUE(suite, result == nullptr, "lua_trynewthread reports construction failure with null");
        } else {
            bool memoryErrorEscaped = false;
            try {
                (void)lua_newthread(failureParent);
            } catch (const std::bad_alloc&) {
                memoryErrorEscaped = true;
            }
            ASSERT_TRUE(suite, memoryErrorEscaped, "lua_newthread propagates each construction allocation failure");
        }

        ASSERT_EQ(suite, parentTop, lua_gettop(failureParent), "failed thread creation preserves parent stack");
        ASSERT_EQ(suite, objectCount, parentState->getGlobalState().getGC().getObjectCount(),
                  "failed thread creation removes partial GC objects immediately");
        ASSERT_EQ(suite, blockCount, failedLedger.blocks.size(),
                  "failed thread creation releases partial allocator blocks immediately");
        failedProbe.failOnAllocation = 0;
        lua_close(failureParent);
        ASSERT_TRUE(suite, failedLedger.blocks.empty(), "failed child creation and parent close remain leak free");
        ASSERT_EQ(suite, static_cast<size_t>(0), failedLedger.sizeMismatches,
                  "child creation rollback preserves old-size contract");
        ASSERT_EQ(suite, static_cast<size_t>(0), failedLedger.unknownFrees,
                  "child creation rollback frees each block once");
    };

    for (size_t failureOffset = 1; failureOffset <= childAllocationAttempts; ++failureOffset) {
        runConstructionFailure(failureOffset, false);
        runConstructionFailure(failureOffset, true);
    }

    auto runParentPushFailure = [&](bool safeApi) {
        AllocatorLedger failedLedger;
        AllocatorProbe failedProbe{&failedLedger};
        lua_State* failureParent = lua_newstate(trackingLuaAllocator, &failedProbe);
        ASSERT_TRUE(suite, failureParent != nullptr, "parent-push failure test creates parent state");
        auto* parentState = reinterpret_cast<Lua::LuaState*>(failureParent);
        const size_t parentPushGrowthTop = parentState->getStack().capacity() - 1;
        while (parentState->getAbsoluteTop() < parentPushGrowthTop) {
            lua_pushnil(failureParent);
        }
        const int parentTop = lua_gettop(failureParent);
        const size_t objectCount = parentState->getGlobalState().getGC().getObjectCount();
        const size_t blockCount = failedLedger.blocks.size();
        failedProbe.failOnAllocation = failedProbe.allocationAttempts + childAllocationAttempts + 1;

        bool strictErrorEscaped = false;
        lua_State* result = nullptr;
        try {
            result = safeApi ? lua_trynewthread(failureParent) : lua_newthread(failureParent);
        } catch (const std::bad_alloc&) {
            strictErrorEscaped = true;
        }
        ASSERT_TRUE(suite, safeApi ? result == nullptr && !strictErrorEscaped : result == nullptr && strictErrorEscaped,
                    "parent-stack growth failure follows the selected strict or safe contract");
        ASSERT_EQ(suite, parentTop, lua_gettop(failureParent), "parent-stack growth failure restores the stack");
        ASSERT_EQ(suite, objectCount, parentState->getGlobalState().getGC().getObjectCount(),
                  "parent-stack growth failure destroys the completed but unpublished thread");
        ASSERT_EQ(suite, blockCount, failedLedger.blocks.size(),
                  "parent-stack growth failure releases the unpublished thread allocations");

        failedProbe.failOnAllocation = 0;
        lua_close(failureParent);
        ASSERT_TRUE(suite, failedLedger.blocks.empty(), "parent-push failure and close remain leak free");
        ASSERT_EQ(suite, static_cast<size_t>(0), failedLedger.sizeMismatches,
                  "parent-push rollback preserves allocator old-size contracts");
        ASSERT_EQ(suite, static_cast<size_t>(0), failedLedger.unknownFrees,
                  "parent-push rollback frees each allocation once");
    };

    runParentPushFailure(false);
    runParentPushFailure(true);
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
    ASSERT_EQ(suite, std::string(kProtectedApiExceptionMessage), std::string(lua_tostring(L, -1)),
              "non-standard C++ exception uses the fixed protected-API error");

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
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.liveBytes, "lua_close returns allocator live bytes to zero");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.sizeMismatches, "allocator frees receive original object sizes");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.unknownFrees, "allocator close path only frees tracked blocks");
    ASSERT_EQ(suite, probe.allocations + probe.reallocations, probe.frees,
              "custom allocator allocations and frees balance");
}

std::string makeAllocatorContractString() {
    std::string text(4096, 'x');
    for (size_t index = 0; index < text.size(); index += 127) {
        text[index] = static_cast<char>('a' + (index / 127) % 26);
    }
    return text;
}

void testAllocatorBackedStringContentAndHardLimit(TestSuite& suite) {
    const std::string text = makeAllocatorContractString();
    size_t internAllocationAttempts = 0;

    {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        ASSERT_TRUE(suite, L != nullptr, "string allocator baseline creates state");
        auto* state = reinterpret_cast<Lua::LuaState*>(L);
        auto& pool = state->getGlobalState().getStringPool();

        size_t contentSizedBlocksBefore = 0;
        for (const auto& block : ledger.blocks) {
            if (block.second >= text.size() + 1) {
                ++contentSizedBlocksBefore;
            }
        }
        const size_t attemptsBefore = probe.allocationAttempts;
        Lua::GCString* interned = pool.intern(Lua::StrView(text.data(), text.size()));
        internAllocationAttempts = probe.allocationAttempts - attemptsBefore;
        ASSERT_TRUE(suite, interned != nullptr && interned->getData() == text,
                    "long interned string preserves allocator-backed contents");
        ASSERT_TRUE(suite, internAllocationAttempts >= 3,
                    "long string routes object, contents, and pool node through lua_Alloc");

        size_t contentSizedBlocksAfter = 0;
        for (const auto& block : ledger.blocks) {
            if (block.second >= text.size() + 1) {
                ++contentSizedBlocksAfter;
            }
        }
        ASSERT_TRUE(suite, contentSizedBlocksAfter > contentSizedBlocksBefore,
                    "allocator ledger observes a new long-string content block");

        lua_close(L);
        ASSERT_TRUE(suite, ledger.blocks.empty(), "string allocator baseline closes without blocks");
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.liveBytes,
                  "string allocator baseline closes with zero live bytes");
    }

    for (size_t offset = 1; offset <= internAllocationAttempts; ++offset) {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        ASSERT_TRUE(suite, L != nullptr, "string failure scan creates state");
        auto* state = reinterpret_cast<Lua::LuaState*>(L);
        auto& pool = state->getGlobalState().getStringPool();
        const size_t poolSizeBefore = pool.size();
        const size_t liveBefore = ledger.liveBytes;

        probe.failOnAllocation = probe.allocationAttempts + offset;
        bool allocationFailed = false;
        try {
            (void)pool.intern(Lua::StrView(text.data(), text.size()));
        } catch (const std::bad_alloc&) {
            allocationFailed = true;
        }
        ASSERT_TRUE(suite, allocationFailed, "every long-string allocation point propagates bad_alloc");
        ASSERT_EQ(suite, poolSizeBefore, pool.size(), "failed string intern leaves pool size unchanged");
        ASSERT_TRUE(suite, pool.find(Lua::StrView(text.data(), text.size())) == nullptr,
                    "failed string intern leaves no dangling pool entry");
        ASSERT_EQ(suite, liveBefore, ledger.liveBytes, "failed string intern releases object and content allocations");

        probe.failOnAllocation = 0;
        ASSERT_TRUE(suite, pool.intern(Lua::StrView(text.data(), text.size())) != nullptr,
                    "string pool remains usable after injected failure");
        lua_close(L);
        ASSERT_TRUE(suite, ledger.blocks.empty(), "string failure scan closes without blocks");
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.liveBytes, "string failure scan closes with zero live bytes");
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.sizeMismatches,
                  "string failure scan preserves allocator old sizes");
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.unknownFrees, "string failure scan frees each block once");
    }

    AllocatorLedger ledger;
    AllocatorProbe probe{&ledger};
    lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
    ASSERT_TRUE(suite, L != nullptr, "string hard-limit test creates state");
    auto* state = reinterpret_cast<Lua::LuaState*>(L);
    auto& gc = state->getGlobalState().getGC();
    auto& pool = state->getGlobalState().getStringPool();
    const size_t liveBefore = ledger.liveBytes;
    const size_t poolSizeBefore = pool.size();
    const Lua::usize softBudget = gc.getManagedMemoryBudgetBytes();

    ledger.hardLimit = liveBefore + sizeof(Lua::GCString);
    ledger.peakBytes = liveBefore;
    ASSERT_EQ(suite, softBudget, gc.getManagedMemoryBudgetBytes(),
              "host hard limit does not mutate the GC managed-size budget");
    const Lua::usize previousBudget = gc.setManagedMemoryBudgetBytes(0);
    ASSERT_EQ(suite, ledger.hardLimit, liveBefore + sizeof(Lua::GCString),
              "GC managed-size budget does not mutate the host hard limit");
    gc.setManagedMemoryBudgetBytes(previousBudget);

    bool allocationFailed = false;
    try {
        (void)pool.intern(Lua::StrView(text.data(), text.size()));
    } catch (const std::bad_alloc&) {
        allocationFailed = true;
    }
    ASSERT_TRUE(suite, allocationFailed, "hard limit rejects long string contents");
    ASSERT_EQ(suite, ledger.hardLimit, ledger.peakBytes,
              "hard-limit probe admits the GCString object before rejecting its contents");
    ASSERT_TRUE(suite, ledger.peakBytes <= ledger.hardLimit, "allocator never exceeds its live-byte hard limit");
    ASSERT_EQ(suite, liveBefore, ledger.liveBytes, "hard-limit failure restores prior live bytes");
    ASSERT_EQ(suite, poolSizeBefore, pool.size(), "hard-limit failure leaves string pool unchanged");

    ledger.hardLimit = std::numeric_limits<size_t>::max();
    ASSERT_TRUE(suite, pool.intern(Lua::StrView(text.data(), text.size())) != nullptr,
                "string allocation succeeds after lifting hard limit");
    lua_close(L);
    ASSERT_TRUE(suite, ledger.blocks.empty(), "string hard-limit state closes without blocks");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.liveBytes, "string hard-limit state closes with zero live bytes");
}

void testGcWorklistAllocatorTransactions(TestSuite& suite) {
    size_t collectionAllocationAttempts = 0;

    {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        ASSERT_TRUE(suite, L != nullptr, "GC worklist baseline creates state");
        prepareGcWorklistFixture(L);

        gAllocatorFailureProbe = &probe;
        gGcWorklistFailureOffset = 0;
        gGcWorklistUseHardLimit = false;
        gGcWorklistFinalizerCalls = 0;
        lua_pushcclosure(L, collectGcWorklists, 0);
        const int status = lua_pcall(L, 0, 0, 0);
        collectionAllocationAttempts = gGcWorklistAllocationAttempts;
        gAllocatorFailureProbe = nullptr;

        ASSERT_EQ(suite, LUA_OK, status, "GC worklist baseline collection succeeds");
        ASSERT_TRUE(suite, collectionAllocationAttempts >= 4,
                    "GC collection routes gray, weak, pending-finalizer, and drain-copy storage through lua_Alloc");
        ASSERT_EQ(suite, 1, gGcWorklistFinalizerCalls, "GC worklist baseline drains the finalizer once");
        ASSERT_TRUE(suite, gcWorklistFixtureRootIsUsable(L), "GC worklist baseline preserves the rooted graph");

        lua_close(L);
        ASSERT_TRUE(suite, ledger.blocks.empty(), "GC worklist baseline closes without allocator blocks");
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.liveBytes, "GC worklist baseline closes with zero live bytes");
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.sizeMismatches,
                  "GC worklist baseline preserves allocator old sizes");
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.unknownFrees, "GC worklist baseline frees each block once");
    }

    bool allStatusesAreMemoryErrors = true;
    bool allTargetsAreReached = true;
    bool allErrorsAreFixed = true;
    bool allRootsSurvive = true;
    bool allRetriesSucceed = true;
    bool allFinalizersRunOnce = true;
    bool allStatesCloseCleanly = true;
    for (size_t offset = 1; offset <= collectionAllocationAttempts; ++offset) {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        if (L == nullptr) {
            allStatusesAreMemoryErrors = false;
            allStatesCloseCleanly = false;
            continue;
        }
        prepareGcWorklistFixture(L);

        gAllocatorFailureProbe = &probe;
        gGcWorklistFailureOffset = offset;
        gGcWorklistUseHardLimit = false;
        gGcWorklistFinalizerCalls = 0;
        lua_pushcclosure(L, collectGcWorklists, 0);
        const int status = lua_pcall(L, 0, 0, 0);
        const size_t attempts = probe.allocationAttempts - gGcWorklistAllocationStart;
        allStatusesAreMemoryErrors = allStatusesAreMemoryErrors && status == LUA_ERRMEM;
        allTargetsAreReached = allTargetsAreReached && attempts == offset;
        allErrorsAreFixed = allErrorsAreFixed && lua_gettop(L) == 2 && lua_isstring(L, -1) != 0 &&
                            std::string(lua_tostring(L, -1)) == "not enough memory";
        allRootsSurvive = allRootsSurvive && gcWorklistFixtureRootIsUsable(L);

        probe.failOnAllocation = 0;
        ledger.hardLimit = std::numeric_limits<size_t>::max();
        gGcWorklistFailureOffset = 0;
        lua_settop(L, 1);
        lua_pushcclosure(L, collectGcWorklists, 0);
        const int retryStatus = lua_pcall(L, 0, 0, 0);
        allRetriesSucceed = allRetriesSucceed && retryStatus == LUA_OK && gcWorklistFixtureRootIsUsable(L);
        allFinalizersRunOnce = allFinalizersRunOnce && gGcWorklistFinalizerCalls == 1;

        gAllocatorFailureProbe = nullptr;
        lua_close(L);
        allStatesCloseCleanly = allStatesCloseCleanly && ledger.blocks.empty() && ledger.liveBytes == 0 &&
                                ledger.sizeMismatches == 0 && ledger.unknownFrees == 0;
    }

    ASSERT_TRUE(suite, allStatusesAreMemoryErrors,
                "every GC worklist allocation failure becomes a protected LUA_ERRMEM");
    ASSERT_TRUE(suite, allTargetsAreReached, "GC worklist fail-on-N scan reaches every observed allocation");
    ASSERT_TRUE(suite, allErrorsAreFixed, "GC worklist OOM publishes the fixed memory error object");
    ASSERT_TRUE(suite, allRootsSurvive, "GC worklist OOM preserves the rooted object graph");
    ASSERT_TRUE(suite, allRetriesSucceed, "GC collection remains retryable after every worklist allocation failure");
    ASSERT_TRUE(suite, allFinalizersRunOnce, "GC worklist rollback neither loses nor duplicates a finalizer");
    ASSERT_TRUE(suite, allStatesCloseCleanly, "GC worklist failure scan closes without leaks or size mismatches");

    AllocatorLedger ledger;
    AllocatorProbe probe{&ledger};
    lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
    ASSERT_TRUE(suite, L != nullptr, "GC worklist hard-limit test creates state");
    prepareGcWorklistFixture(L);

    gAllocatorFailureProbe = &probe;
    gGcWorklistFailureOffset = 0;
    gGcWorklistUseHardLimit = true;
    gGcWorklistHardLimit = 0;
    gGcWorklistFinalizerCalls = 0;
    lua_pushcclosure(L, collectGcWorklists, 0);
    const int hardLimitStatus = lua_pcall(L, 0, 0, 0);
    ASSERT_EQ(suite, LUA_ERRMEM, hardLimitStatus, "allocator hard limit rejects GC worklist growth");
    ASSERT_TRUE(suite, gGcWorklistHardLimit != 0 && ledger.peakBytes <= gGcWorklistHardLimit,
                "GC worklist allocation never exceeds the host hard limit");
    ASSERT_TRUE(suite, ledger.liveBytes <= gGcWorklistHardLimit,
                "GC worklist hard-limit failure leaves used bytes within the limit");
    ASSERT_EQ(suite, std::string("not enough memory"), std::string(lua_tostring(L, -1)),
              "GC worklist hard-limit failure uses the fixed memory error object");
    ASSERT_TRUE(suite, gcWorklistFixtureRootIsUsable(L), "GC worklist hard-limit failure preserves roots");

    ledger.hardLimit = std::numeric_limits<size_t>::max();
    probe.failOnAllocation = 0;
    gGcWorklistUseHardLimit = false;
    lua_settop(L, 1);
    lua_pushcclosure(L, collectGcWorklists, 0);
    ASSERT_EQ(suite, LUA_OK, lua_pcall(L, 0, 0, 0), "GC collection succeeds after lifting the allocator hard limit");
    ASSERT_EQ(suite, 1, gGcWorklistFinalizerCalls,
              "hard-limit rollback preserves the pending finalizer for a successful retry");

    gAllocatorFailureProbe = nullptr;
    lua_close(L);
    ASSERT_TRUE(suite, ledger.blocks.empty(), "GC worklist hard-limit state closes without blocks");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.liveBytes,
              "GC worklist hard-limit state closes with zero live bytes");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.sizeMismatches,
              "GC worklist hard-limit path preserves allocator old sizes");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.unknownFrees, "GC worklist hard-limit path frees each block once");

    gGcWorklistFailureOffset = 0;
    gGcWorklistUseHardLimit = false;
    gGcWorklistHardLimit = 0;
}

void testTableHashAllocatorTransactions(TestSuite& suite) {
    size_t rawAllocationAttempts = 0;
    {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        ASSERT_TRUE(suite, L != nullptr, "raw hash baseline creates state");
        (void)lua_gc(L, LUA_GCSTOP, 0);
        lua_newtable(L);

        gAllocatorFailureProbe = &probe;
        gTableHashFailureOffset = 0;
        gTableHashUseHardLimit = false;
        lua_pushcclosure(L, writeRawTableHash, 0);
        lua_pushvalue(L, 1);
        const int status = lua_pcall(L, 1, 0, 0);
        rawAllocationAttempts = probe.allocationAttempts - gTableHashAllocationStart;
        gAllocatorFailureProbe = nullptr;

        ASSERT_EQ(suite, LUA_OK, status, "raw hash baseline insertion succeeds");
        ASSERT_TRUE(suite, rawAllocationAttempts > 0, "raw hash insertion routes storage through lua_Alloc");
        ASSERT_TRUE(suite, rawTableHashValueEquals(L, 1, 73), "raw hash baseline commits the target value");
        lua_close(L);
        ASSERT_TRUE(suite, ledger.blocks.empty() && ledger.liveBytes == 0,
                    "raw hash baseline closes with zero allocator ownership");
        ASSERT_TRUE(suite, ledger.sizeMismatches == 0 && ledger.unknownFrees == 0,
                    "raw hash baseline preserves allocator size and ownership contracts");
    }

    bool rawStatusesAreMemoryErrors = true;
    bool rawTargetsAreReached = true;
    bool rawErrorsAreFixed = true;
    bool rawTablesStayUnchanged = true;
    bool rawRetriesSucceed = true;
    bool rawStatesCloseCleanly = true;
    for (size_t offset = 1; offset <= rawAllocationAttempts; ++offset) {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        if (L == nullptr) {
            rawStatusesAreMemoryErrors = false;
            rawStatesCloseCleanly = false;
            continue;
        }
        (void)lua_gc(L, LUA_GCSTOP, 0);
        lua_newtable(L);

        gAllocatorFailureProbe = &probe;
        gTableHashFailureOffset = offset;
        gTableHashUseHardLimit = false;
        lua_pushcclosure(L, writeRawTableHash, 0);
        lua_pushvalue(L, 1);
        const int status = lua_pcall(L, 1, 0, 0);
        const size_t attempts = probe.allocationAttempts - gTableHashAllocationStart;
        rawStatusesAreMemoryErrors = rawStatusesAreMemoryErrors && status == LUA_ERRMEM;
        rawTargetsAreReached = rawTargetsAreReached && attempts == offset;
        rawErrorsAreFixed = rawErrorsAreFixed && lua_gettop(L) == 2 && lua_isstring(L, -1) != 0 &&
                            std::string(lua_tostring(L, -1)) == "not enough memory";
        rawTablesStayUnchanged = rawTablesStayUnchanged && rawTableHashValueIsNil(L, 1);

        probe.failOnAllocation = 0;
        ledger.hardLimit = std::numeric_limits<size_t>::max();
        gTableHashFailureOffset = 0;
        lua_settop(L, 1);
        lua_pushcclosure(L, writeRawTableHash, 0);
        lua_pushvalue(L, 1);
        const int retryStatus = lua_pcall(L, 1, 0, 0);
        rawRetriesSucceed = rawRetriesSucceed && retryStatus == LUA_OK && rawTableHashValueEquals(L, 1, 73);

        gAllocatorFailureProbe = nullptr;
        lua_close(L);
        rawStatesCloseCleanly = rawStatesCloseCleanly && ledger.blocks.empty() && ledger.liveBytes == 0 &&
                                ledger.sizeMismatches == 0 && ledger.unknownFrees == 0;
    }

    ASSERT_TRUE(suite, rawStatusesAreMemoryErrors, "every raw hash allocation failure becomes LUA_ERRMEM");
    ASSERT_TRUE(suite, rawTargetsAreReached, "raw hash fail-on-N scan reaches every observed allocation");
    ASSERT_TRUE(suite, rawErrorsAreFixed, "raw hash OOM publishes the fixed memory error object");
    ASSERT_TRUE(suite, rawTablesStayUnchanged, "raw hash OOM commits no partial table entry");
    ASSERT_TRUE(suite, rawRetriesSucceed, "raw hash insertion remains usable after every allocation failure");
    ASSERT_TRUE(suite, rawStatesCloseCleanly, "raw hash failure scan closes without leaks or size mismatches");

    {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        ASSERT_TRUE(suite, L != nullptr, "raw hash hard-limit test creates state");
        (void)lua_gc(L, LUA_GCSTOP, 0);
        lua_newtable(L);

        gAllocatorFailureProbe = &probe;
        gTableHashFailureOffset = 0;
        gTableHashUseHardLimit = true;
        gTableHashHardLimit = 0;
        lua_pushcclosure(L, writeRawTableHash, 0);
        lua_pushvalue(L, 1);
        const int status = lua_pcall(L, 1, 0, 0);
        ASSERT_EQ(suite, LUA_ERRMEM, status, "hard limit rejects raw hash growth");
        ASSERT_TRUE(suite,
                    gTableHashHardLimit != 0 && ledger.peakBytes <= gTableHashHardLimit &&
                        ledger.liveBytes <= gTableHashHardLimit,
                    "raw hash allocation never exceeds the host hard limit");
        ASSERT_TRUE(suite, rawTableHashValueIsNil(L, 1), "raw hash hard-limit failure commits no entry");

        ledger.hardLimit = std::numeric_limits<size_t>::max();
        probe.failOnAllocation = 0;
        gTableHashUseHardLimit = false;
        lua_settop(L, 1);
        lua_pushcclosure(L, writeRawTableHash, 0);
        lua_pushvalue(L, 1);
        ASSERT_EQ(suite, LUA_OK, lua_pcall(L, 1, 0, 0), "raw hash insertion succeeds after lifting hard limit");
        ASSERT_TRUE(suite, rawTableHashValueEquals(L, 1, 73), "raw hash hard-limit retry commits the value");

        gAllocatorFailureProbe = nullptr;
        lua_close(L);
        ASSERT_TRUE(suite, ledger.blocks.empty() && ledger.liveBytes == 0,
                    "raw hash hard-limit state closes with zero allocator ownership");
        ASSERT_TRUE(suite, ledger.sizeMismatches == 0 && ledger.unknownFrees == 0,
                    "raw hash hard-limit path preserves allocator contracts");
    }

    size_t vmAllocationAttempts = 0;
    {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        ASSERT_TRUE(suite, L != nullptr, "VM SETTABLE baseline creates state");
        prepareVmTableHashFixture(L);

        gAllocatorFailureProbe = &probe;
        gTableHashFailureOffset = 0;
        gTableHashUseHardLimit = false;
        lua_pushvalue(L, 1);
        const int status = lua_pcall(L, 0, 1, 0);
        vmAllocationAttempts = probe.allocationAttempts - gTableHashAllocationStart;
        gAllocatorFailureProbe = nullptr;

        ASSERT_EQ(suite, LUA_OK, status, "VM SETTABLE baseline executes");
        ASSERT_TRUE(suite, vmAllocationAttempts > 0, "VM SETTABLE routes hash storage through lua_Alloc");
        ASSERT_TRUE(suite, lua_isnumber(L, -1) != 0 && lua_tonumber(L, -1) == 91 && vmTableHashValueEquals(L, 91),
                    "VM SETTABLE baseline commits and reads the target value");
        lua_close(L);
        ASSERT_TRUE(suite, ledger.blocks.empty() && ledger.liveBytes == 0,
                    "VM SETTABLE baseline closes with zero allocator ownership");
        ASSERT_TRUE(suite, ledger.sizeMismatches == 0 && ledger.unknownFrees == 0,
                    "VM SETTABLE baseline preserves allocator contracts");
    }

    bool vmStatusesAreMemoryErrors = true;
    bool vmTargetsAreReached = true;
    bool vmErrorsAreFixed = true;
    bool vmTablesStayUnchanged = true;
    bool vmRetriesSucceed = true;
    bool vmStatesCloseCleanly = true;
    for (size_t offset = 1; offset <= vmAllocationAttempts; ++offset) {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        if (L == nullptr) {
            vmStatusesAreMemoryErrors = false;
            vmStatesCloseCleanly = false;
            continue;
        }
        prepareVmTableHashFixture(L);

        gAllocatorFailureProbe = &probe;
        gTableHashFailureOffset = offset;
        gTableHashUseHardLimit = false;
        lua_pushvalue(L, 1);
        const int status = lua_pcall(L, 0, 1, 0);
        const size_t attempts = probe.allocationAttempts - gTableHashAllocationStart;
        vmStatusesAreMemoryErrors = vmStatusesAreMemoryErrors && status == LUA_ERRMEM;
        vmTargetsAreReached = vmTargetsAreReached && attempts == offset;
        vmErrorsAreFixed = vmErrorsAreFixed && lua_gettop(L) == 2 && lua_isstring(L, -1) != 0 &&
                           std::string(lua_tostring(L, -1)) == "not enough memory";
        vmTablesStayUnchanged = vmTablesStayUnchanged && vmTableHashValueIsNil(L);

        probe.failOnAllocation = 0;
        ledger.hardLimit = std::numeric_limits<size_t>::max();
        gTableHashFailureOffset = 0;
        lua_settop(L, 1);
        lua_pushvalue(L, 1);
        const int retryStatus = lua_pcall(L, 0, 1, 0);
        vmRetriesSucceed =
            vmRetriesSucceed && retryStatus == LUA_OK && lua_tonumber(L, -1) == 91 && vmTableHashValueEquals(L, 91);

        gAllocatorFailureProbe = nullptr;
        lua_close(L);
        vmStatesCloseCleanly = vmStatesCloseCleanly && ledger.blocks.empty() && ledger.liveBytes == 0 &&
                               ledger.sizeMismatches == 0 && ledger.unknownFrees == 0;
    }

    ASSERT_TRUE(suite, vmStatusesAreMemoryErrors, "every VM SETTABLE allocation failure becomes LUA_ERRMEM");
    ASSERT_TRUE(suite, vmTargetsAreReached, "VM SETTABLE fail-on-N scan reaches every observed allocation");
    ASSERT_TRUE(suite, vmErrorsAreFixed, "VM SETTABLE OOM publishes the fixed memory error object");
    ASSERT_TRUE(suite, vmTablesStayUnchanged, "VM SETTABLE OOM commits no partial hash entry");
    ASSERT_TRUE(suite, vmRetriesSucceed, "VM SETTABLE remains executable after every allocation failure");
    ASSERT_TRUE(suite, vmStatesCloseCleanly, "VM SETTABLE failure scan closes without allocator leaks");

    {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        ASSERT_TRUE(suite, L != nullptr, "VM SETTABLE hard-limit test creates state");
        prepareVmTableHashFixture(L);

        gAllocatorFailureProbe = &probe;
        gTableHashFailureOffset = 0;
        gTableHashUseHardLimit = true;
        gTableHashHardLimit = 0;
        lua_pushvalue(L, 1);
        const int status = lua_pcall(L, 0, 1, 0);
        ASSERT_EQ(suite, LUA_ERRMEM, status, "hard limit rejects VM SETTABLE hash growth");
        ASSERT_TRUE(suite,
                    gTableHashHardLimit != 0 && ledger.peakBytes <= gTableHashHardLimit &&
                        ledger.liveBytes <= gTableHashHardLimit,
                    "VM SETTABLE never exceeds the host hard limit");
        ASSERT_TRUE(suite, vmTableHashValueIsNil(L), "VM SETTABLE hard-limit failure commits no entry");

        ledger.hardLimit = std::numeric_limits<size_t>::max();
        probe.failOnAllocation = 0;
        gTableHashUseHardLimit = false;
        lua_settop(L, 1);
        lua_pushvalue(L, 1);
        ASSERT_EQ(suite, LUA_OK, lua_pcall(L, 0, 1, 0), "VM SETTABLE succeeds after lifting hard limit");
        ASSERT_TRUE(suite, lua_tonumber(L, -1) == 91 && vmTableHashValueEquals(L, 91),
                    "VM SETTABLE hard-limit retry commits the value");

        gAllocatorFailureProbe = nullptr;
        lua_close(L);
        ASSERT_TRUE(suite, ledger.blocks.empty() && ledger.liveBytes == 0,
                    "VM SETTABLE hard-limit state closes with zero allocator ownership");
        ASSERT_TRUE(suite, ledger.sizeMismatches == 0 && ledger.unknownFrees == 0,
                    "VM SETTABLE hard-limit path preserves allocator contracts");
    }

    gTableHashFailureOffset = 0;
    gTableHashUseHardLimit = false;
    gTableHashHardLimit = 0;
}

void testTableReallocHardLimitTransaction(TestSuite& suite) {
    AllocatorLedger ledger;
    AllocatorProbe probe{&ledger};
    lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
    ASSERT_TRUE(suite, L != nullptr, "table realloc test creates state");
    auto* state = reinterpret_cast<Lua::LuaState*>(L);
    Lua::Table* table = state->getGlobalState().getGC().create<Lua::Table>();

    table->setArray(1, Lua::Value(11.0));
    const size_t liveBeforeGrowth = ledger.liveBytes;
    const size_t reallocationsBeforeGrowth = probe.reallocations;
    ledger.hardLimit = liveBeforeGrowth;
    ledger.peakBytes = liveBeforeGrowth;

    bool allocationFailed = false;
    try {
        table->setArray(64, Lua::Value(64.0));
    } catch (const std::bad_alloc&) {
        allocationFailed = true;
    }
    ASSERT_TRUE(suite, allocationFailed, "table growth is rejected by the allocator hard limit");
    ASSERT_EQ(suite, static_cast<Lua::usize>(1), table->getArraySize(),
              "failed table realloc preserves logical array size");
    ASSERT_EQ(suite, 11.0, table->getArray(1).asNumber(), "failed table realloc retains old block contents");
    ASSERT_TRUE(suite, table->getArray(64).isNil(), "failed table realloc commits no target value");
    ASSERT_EQ(suite, liveBeforeGrowth, ledger.liveBytes, "failed table realloc preserves allocator live bytes");
    ASSERT_EQ(suite, reallocationsBeforeGrowth, probe.reallocations,
              "rejected realloc does not release or replace the old block");
    ASSERT_TRUE(suite, ledger.peakBytes <= ledger.hardLimit, "table failure never exceeds hard limit");

    ledger.hardLimit = std::numeric_limits<size_t>::max();
    table->setArray(64, Lua::Value(64.0));
    ASSERT_TRUE(suite, probe.reallocations > reallocationsBeforeGrowth,
                "successful table growth uses lua_Alloc realloc form");
    ASSERT_EQ(suite, 11.0, table->getArray(1).asNumber(), "successful realloc retains earlier table value");
    ASSERT_EQ(suite, 64.0, table->getArray(64).asNumber(), "successful realloc commits target table value");

    const std::array<Lua::Value, 3> values = {Lua::Value(65.0), Lua::Value(66.0), Lua::Value(67.0)};
    const size_t liveBeforeRange = ledger.liveBytes;
    ledger.hardLimit = liveBeforeRange;
    ledger.peakBytes = liveBeforeRange;
    allocationFailed = false;
    try {
        table->setArrayRange(65, values);
    } catch (const std::bad_alloc&) {
        allocationFailed = true;
    }
    ASSERT_TRUE(suite, allocationFailed, "SETLIST-style range growth is rejected atomically");
    ASSERT_EQ(suite, static_cast<Lua::usize>(64), table->getArraySize(),
              "failed range growth preserves logical table size");
    ASSERT_TRUE(suite, table->getArray(65).isNil() && table->getArray(66).isNil() && table->getArray(67).isNil(),
                "failed range growth commits none of its values");
    ASSERT_EQ(suite, liveBeforeRange, ledger.liveBytes, "failed range growth retains its old allocation");
    ASSERT_TRUE(suite, ledger.peakBytes <= ledger.hardLimit, "range failure never exceeds hard limit");

    ledger.hardLimit = std::numeric_limits<size_t>::max();
    table->setArrayRange(65, values);
    ASSERT_EQ(suite, 65.0, table->getArray(65).asNumber(), "recovered range writes first value");
    ASSERT_EQ(suite, 67.0, table->getArray(67).asNumber(), "recovered range writes last value");

    lua_close(L);
    ASSERT_TRUE(suite, ledger.blocks.empty(), "table realloc state closes without blocks");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.liveBytes, "table realloc state closes with zero live bytes");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.sizeMismatches, "table realloc preserves allocator old sizes");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.unknownFrees, "table realloc frees each block once");
}

void testProtoAllocatorTransactions(TestSuite& suite) {
    size_t constantAllocationAttempts = 0;
    {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        ASSERT_TRUE(suite, L != nullptr, "Proto constant baseline creates state");
        auto* state = reinterpret_cast<Lua::LuaState*>(L);
        Lua::Proto* proto = state->getGlobalState().getGC().create<Lua::Proto>();
        const size_t attemptsBefore = probe.allocationAttempts;
        ASSERT_EQ(suite, static_cast<Lua::usize>(0), proto->addConstant(Lua::Value(42.0)),
                  "Proto baseline adds first constant");
        constantAllocationAttempts = probe.allocationAttempts - attemptsBefore;
        ASSERT_TRUE(suite, constantAllocationAttempts >= 2,
                    "Proto constant insertion routes vector and dedup metadata through lua_Alloc");
        lua_close(L);
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.liveBytes,
                  "Proto constant baseline closes with zero live bytes");
    }

    for (size_t offset = 1; offset <= constantAllocationAttempts; ++offset) {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        ASSERT_TRUE(suite, L != nullptr, "Proto constant failure scan creates state");
        auto* state = reinterpret_cast<Lua::LuaState*>(L);
        Lua::Proto* proto = state->getGlobalState().getGC().create<Lua::Proto>();

        probe.failOnAllocation = probe.allocationAttempts + offset;
        bool allocationFailed = false;
        try {
            (void)proto->addConstant(Lua::Value(42.0));
        } catch (const std::bad_alloc&) {
            allocationFailed = true;
        }
        ASSERT_TRUE(suite, allocationFailed, "every Proto constant allocation point propagates bad_alloc");
        ASSERT_EQ(suite, static_cast<Lua::usize>(0), proto->getConstantCount(),
                  "failed Proto constant insert rolls back logical contents");

        probe.failOnAllocation = 0;
        ASSERT_EQ(suite, static_cast<Lua::usize>(0), proto->addConstant(Lua::Value(42.0)),
                  "Proto remains usable after constant insertion failure");
        ASSERT_EQ(suite, static_cast<Lua::usize>(0), proto->addConstant(Lua::Value(42.0)),
                  "recovered Proto constant deduplication returns original slot");
        ASSERT_EQ(suite, static_cast<Lua::usize>(1), proto->getConstantCount(),
                  "recovered Proto contains one deduplicated constant");

        lua_close(L);
        ASSERT_TRUE(suite, ledger.blocks.empty(), "Proto constant failure scan closes without blocks");
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.liveBytes,
                  "Proto constant failure scan closes with zero live bytes");
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.sizeMismatches,
                  "Proto constant failure scan preserves allocator old sizes");
        ASSERT_EQ(suite, static_cast<size_t>(0), ledger.unknownFrees,
                  "Proto constant failure scan frees each block once");
    }

    AllocatorLedger ledger;
    AllocatorProbe probe{&ledger};
    lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
    ASSERT_TRUE(suite, L != nullptr, "Proto realloc hard-limit test creates state");
    auto* state = reinterpret_cast<Lua::LuaState*>(L);
    Lua::Proto* proto = state->getGlobalState().getGC().create<Lua::Proto>();
    proto->addInstruction(static_cast<Lua::Instruction>(0x11111111U));
    const size_t liveBeforeGrowth = ledger.liveBytes;
    const size_t reallocationsBeforeGrowth = probe.reallocations;
    ledger.hardLimit = liveBeforeGrowth;
    ledger.peakBytes = liveBeforeGrowth;

    bool allocationFailed = false;
    try {
        (void)proto->addInstruction(static_cast<Lua::Instruction>(0x22222222U));
    } catch (const std::bad_alloc&) {
        allocationFailed = true;
    }
    ASSERT_TRUE(suite, allocationFailed, "Proto metadata realloc is rejected by hard limit");
    ASSERT_EQ(suite, static_cast<Lua::usize>(1), proto->getInstructionCount(),
              "failed Proto realloc preserves instruction count");
    ASSERT_EQ(suite, static_cast<Lua::Instruction>(0x11111111U), proto->getInstruction(0),
              "failed Proto realloc retains old instruction block");
    ASSERT_EQ(suite, liveBeforeGrowth, ledger.liveBytes, "failed Proto realloc preserves live bytes");
    ASSERT_EQ(suite, reallocationsBeforeGrowth, probe.reallocations,
              "failed Proto realloc leaves old allocation registered");
    ASSERT_TRUE(suite, ledger.peakBytes <= ledger.hardLimit, "Proto failure never exceeds hard limit");

    ledger.hardLimit = std::numeric_limits<size_t>::max();
    (void)proto->addInstruction(static_cast<Lua::Instruction>(0x22222222U));
    ASSERT_TRUE(suite, probe.reallocations > reallocationsBeforeGrowth,
                "successful Proto metadata growth uses lua_Alloc realloc form");
    ASSERT_EQ(suite, static_cast<Lua::Instruction>(0x11111111U), proto->getInstruction(0),
              "successful Proto realloc retains first instruction");
    ASSERT_EQ(suite, static_cast<Lua::Instruction>(0x22222222U), proto->getInstruction(1),
              "successful Proto realloc appends second instruction");

    lua_close(L);
    ASSERT_TRUE(suite, ledger.blocks.empty(), "Proto realloc state closes without blocks");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.liveBytes, "Proto realloc state closes with zero live bytes");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.sizeMismatches, "Proto realloc preserves allocator old sizes");
    ASSERT_EQ(suite, static_cast<size_t>(0), ledger.unknownFrees, "Proto realloc frees each block once");
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

void testProtectedStatusApiExceptionBoundaries(TestSuite& suite) {
    lua_State* L = lua_open();
    lua_pushnumber(L, 73);

    ASSERT_EQ(suite, LUA_ERRRUN, lua_load(L, throwRuntimeErrorReader, nullptr, "=reader-runtime"),
              "lua_load maps reader std::exception to LUA_ERRRUN");
    ASSERT_EQ(suite, 2, lua_gettop(L), "reader exception preserves prefix and appends one error");
    ASSERT_EQ(suite, 73.0, lua_tonumber(L, 1), "reader exception preserves prefix value");
    ASSERT_EQ(suite, std::string("reader callback failed"), std::string(lua_tostring(L, -1)),
              "reader std::exception preserves its message when allocation succeeds");

    lua_settop(L, 1);
    ASSERT_EQ(suite, LUA_ERRRUN, lua_load(L, throwUnknownExceptionReader, nullptr, "=reader-unknown"),
              "lua_load maps non-standard reader exception to LUA_ERRRUN");
    ASSERT_EQ(suite, 2, lua_gettop(L), "non-standard reader exception has canonical stack shape");
    ASSERT_EQ(suite, std::string(kProtectedApiExceptionMessage), std::string(lua_tostring(L, -1)),
              "non-standard reader exception uses fixed emergency text");

    lua_settop(L, 0);
    constexpr const char* source = "return 42";
    ASSERT_EQ(suite, LUA_OK, luaL_loadbuffer(L, source, std::strlen(source), "=writer-boundary"),
              "writer boundary fixture compiles");
    auto* state = reinterpret_cast<Lua::LuaState*>(L);
    Lua::Function* sourceFunction = state->at(-1).asFunction();

    ASSERT_EQ(suite, 1, lua_dump(L, throwRuntimeErrorWriter, nullptr),
              "lua_dump maps writer std::exception to its fallback status");
    ASSERT_EQ(suite, 1, lua_gettop(L), "writer std::exception preserves source stack shape");
    ASSERT_TRUE(suite, state->at(-1).asFunction() == sourceFunction,
                "writer std::exception preserves source function identity");

    ASSERT_EQ(suite, 1, lua_dump(L, throwUnknownExceptionWriter, nullptr),
              "lua_dump maps non-standard writer exception to its fallback status");
    ASSERT_EQ(suite, 1, lua_gettop(L), "non-standard writer exception preserves source function");
    ASSERT_TRUE(suite, state->at(-1).asFunction() == sourceFunction,
                "non-standard writer exception preserves source function identity");

    ASSERT_EQ(suite, LUA_ERRMEM, lua_dump(L, throwBadAllocWriter, nullptr),
              "lua_dump distinguishes writer allocation failure");
    ASSERT_EQ(suite, 1, lua_gettop(L), "writer allocation failure preserves source function");
    lua_close(L);

    AllocatorLedger ledger;
    AllocatorProbe probe{&ledger};
    L = lua_newstate(trackingLuaAllocator, &probe);
    ASSERT_TRUE(suite, L != nullptr, "persistent exception boundary state is created");

    lua_pushcclosure(L, throwRuntimeErrorDuringResume, 0);
    gAllocatorFailureProbe = &probe;
    ASSERT_EQ(suite, LUA_ERRRUN, lua_pcall(L, 0, 1, 0),
              "pcall keeps std::exception as runtime error under persistent allocator failure");
    const size_t pcallFailureTarget = gArmedAllocatorFailureTarget;
    probe.failFromAllocation = 0;
    gAllocatorFailureProbe = nullptr;
    gArmedAllocatorFailureTarget = 0;
    ASSERT_TRUE(suite, pcallFailureTarget != 0 && probe.allocationAttempts >= pcallFailureTarget,
                "pcall reaches the persistent allocator failure while formatting its error");
    ASSERT_EQ(suite, std::string(kProtectedApiExceptionMessage), std::string(lua_tostring(L, -1)),
              "pcall publishes fixed runtime text without a second allocation");

    lua_settop(L, 0);
    lua_pushcclosure(L, throwUnknownExceptionWithAllocatorFailure, 0);
    gAllocatorFailureProbe = &probe;
    ASSERT_EQ(suite, LUA_ERRRUN, lua_pcall(L, 0, 1, 0),
              "pcall contains non-standard exception under persistent allocator failure");
    probe.failFromAllocation = 0;
    gAllocatorFailureProbe = nullptr;
    gArmedAllocatorFailureTarget = 0;
    ASSERT_EQ(suite, std::string(kProtectedApiExceptionMessage), std::string(lua_tostring(L, -1)),
              "non-standard pcall exception publishes fixed runtime text");

    lua_settop(L, 0);
    lua_State* coroutine = lua_newthread(L);
    ASSERT_TRUE(suite, coroutine != nullptr, "resume exception boundary creates coroutine");
    lua_pushcclosure(L, throwRuntimeErrorDuringResume, 0);
    lua_xmove(L, coroutine, 1);
    gAllocatorFailureProbe = &probe;
    ASSERT_EQ(suite, LUA_ERRRUN, lua_resume(coroutine, 0),
              "resume keeps std::exception as runtime error under persistent allocator failure");
    const size_t resumeFailureTarget = gArmedAllocatorFailureTarget;
    probe.failFromAllocation = 0;
    gAllocatorFailureProbe = nullptr;
    gArmedAllocatorFailureTarget = 0;
    ASSERT_TRUE(suite, resumeFailureTarget != 0 && probe.allocationAttempts >= resumeFailureTarget,
                "resume reaches the persistent allocator failure while formatting its error");
    ASSERT_EQ(suite, 1, lua_gettop(coroutine), "resume exception leaves one canonical error object");
    ASSERT_EQ(suite, std::string(kProtectedApiExceptionMessage), std::string(lua_tostring(coroutine, -1)),
              "resume publishes fixed runtime text without allocating");

    lua_settop(L, 0);
    lua_pushcclosure(L, yieldApiArguments, 0);
    lua_setglobal(L, "api_yield_leaf");
    lua_pushcclosure(L, throwRuntimeErrorDuringResume, 0);
    lua_setglobal(L, "cpp_resume_failure");
    lua_State* resumedAfterYield = lua_newthread(L);
    ASSERT_TRUE(suite, resumedAfterYield != nullptr, "second-resume boundary creates coroutine");
    constexpr const char* resumeAfterYieldSource = "api_yield_leaf(55); return cpp_resume_failure()";
    ASSERT_EQ(suite, LUA_OK, luaL_loadstring(L, resumeAfterYieldSource),
              "second-resume boundary compiles a yield then C++ failure");
    lua_xmove(L, resumedAfterYield, 1);
    ASSERT_EQ(suite, LUA_YIELD, lua_resume(resumedAfterYield, 0),
              "second-resume boundary reaches a suspended Lua frame");
    ASSERT_EQ(suite, 1, lua_gettop(resumedAfterYield), "yielded value is the only public result");
    lua_settop(resumedAfterYield, 0);

    gAllocatorFailureProbe = &probe;
    ASSERT_EQ(suite, LUA_ERRRUN, lua_resume(resumedAfterYield, 0),
              "second resume contains C++ exception under persistent allocator failure");
    const size_t secondResumeFailureTarget = gArmedAllocatorFailureTarget;
    probe.failFromAllocation = 0;
    gAllocatorFailureProbe = nullptr;
    gArmedAllocatorFailureTarget = 0;
    ASSERT_TRUE(suite, secondResumeFailureTarget != 0 && probe.allocationAttempts >= secondResumeFailureTarget,
                "second resume reaches the persistent allocator failure");
    ASSERT_EQ(suite, 1, lua_gettop(resumedAfterYield),
              "strong rollback publishes one error at the post-abort frame base");
    ASSERT_EQ(suite, LUA_TSTRING, lua_type(resumedAfterYield, 1),
              "second-resume error occupies the first public stack slot");
    ASSERT_EQ(suite, std::string(kProtectedApiExceptionMessage), std::string(lua_tostring(resumedAfterYield, 1)),
              "second resume uses the fixed protected-API error text");

    lua_close(L);
    ASSERT_TRUE(suite, ledger.blocks.empty(), "persistent exception boundary cases close without leaks");

    ThrowingAllocatorProbe throwingProbe;
    L = lua_newstate(throwingLuaAllocator, &throwingProbe);
    ASSERT_TRUE(suite, L != nullptr, "throwing allocator boundary state is created");
    lua_pushnumber(L, 91);
    const int topBeforeFailure = lua_gettop(L);
    throwingProbe.armed = true;
    ASSERT_EQ(suite, 0, lua_checkstack(L, 1000), "lua_checkstack contains a throwing allocator callback");
    ASSERT_EQ(suite, topBeforeFailure, lua_gettop(L), "failed lua_checkstack preserves logical stack");
    bool strictThreadFailureEscaped = false;
    try {
        (void)lua_newthread(L);
    } catch (const std::bad_alloc&) {
        strictThreadFailureEscaped = true;
    }
    ASSERT_TRUE(suite, strictThreadFailureEscaped, "lua_newthread propagates a throwing allocator failure");
    ASSERT_EQ(suite, topBeforeFailure, lua_gettop(L), "strict lua_newthread failure preserves parent stack");

    bool safeThreadFailureEscaped = false;
    lua_State* safeThread = nullptr;
    try {
        safeThread = lua_trynewthread(L);
    } catch (...) {
        safeThreadFailureEscaped = true;
    }
    ASSERT_TRUE(suite, !safeThreadFailureEscaped, "lua_trynewthread contains a throwing allocator callback");
    ASSERT_TRUE(suite, safeThread == nullptr, "lua_trynewthread reports throwing allocator failure with null");
    ASSERT_EQ(suite, topBeforeFailure, lua_gettop(L), "safe lua_trynewthread failure preserves parent stack");
    throwingProbe.armed = false;
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

void testStateStackAndCallInfoAllocatorTransactions(TestSuite& suite) {
    AllocatorLedger ledger;
    AllocatorProbe probe{&ledger};
    lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
    ASSERT_TRUE(suite, L != nullptr, "state stack allocator test creates a state");
    auto* state = reinterpret_cast<Lua::LuaState*>(L);

    const int topBeforeStackGrowth = lua_gettop(L);
    const size_t liveBeforeStackGrowth = ledger.liveBytes;
    const size_t blocksBeforeStackGrowth = ledger.blocks.size();
    const size_t stackAllocationStart = probe.allocationAttempts;
    probe.failOnAllocation = stackAllocationStart + 1;
    ASSERT_EQ(suite, 0, lua_checkstack(L, 2048), "lua_checkstack contains an injected stack allocation failure");
    ASSERT_EQ(suite, stackAllocationStart + 1, probe.allocationAttempts,
              "lua_checkstack reaches the selected allocator attempt");
    ASSERT_EQ(suite, topBeforeStackGrowth, lua_gettop(L), "failed stack growth preserves the logical top");
    ASSERT_EQ(suite, liveBeforeStackGrowth, ledger.liveBytes, "failed stack growth preserves allocator live bytes");
    ASSERT_EQ(suite, blocksBeforeStackGrowth, ledger.blocks.size(), "failed stack growth preserves owned blocks");

    probe.failOnAllocation = 0;
    ledger.hardLimit = ledger.liveBytes;
    ledger.peakBytes = ledger.liveBytes;
    const size_t stackHardLimit = ledger.hardLimit;
    ASSERT_EQ(suite, 0, lua_checkstack(L, 2048), "zero-headroom hard limit rejects stack growth");
    ASSERT_TRUE(suite, ledger.liveBytes <= stackHardLimit && ledger.peakBytes <= stackHardLimit,
                "stack growth never exceeds the allocator hard limit");
    ASSERT_EQ(suite, topBeforeStackGrowth, lua_gettop(L), "hard-limit stack failure preserves the logical top");

    ledger.hardLimit = std::numeric_limits<size_t>::max();
    ASSERT_EQ(suite, 1, lua_checkstack(L, 2048), "stack growth succeeds after lifting the allocator hard limit");
    ASSERT_TRUE(suite, state->getStack().capacity() >= static_cast<Lua::usize>(2049),
                "stack retry publishes the requested capacity and emergency slot");

    gAllocatorFailureProbe = &probe;
    gCallInfoUseHardLimit = false;
    lua_pushcclosure(L, growCallInfoWithArmedAllocator, 0);
    ASSERT_EQ(suite, LUA_ERRMEM, lua_pcall(L, 0, 0, 0),
              "protected call maps CallInfo allocation failure to LUA_ERRMEM");
    ASSERT_EQ(suite, gCallInfoAllocationStart + 1, probe.allocationAttempts,
              "CallInfo failure reaches the selected allocator attempt");
    ASSERT_EQ(suite, static_cast<Lua::usize>(1), state->getCallStackSize(),
              "CallInfo allocation failure restores the base call depth");
    ASSERT_TRUE(suite, lua_isstring(L, -1) != 0 && std::string(lua_tostring(L, -1)) == "not enough memory",
                "CallInfo allocation failure publishes the fixed memory error");

    probe.failOnAllocation = 0;
    lua_settop(L, 0);
    gCallInfoUseHardLimit = true;
    lua_pushcclosure(L, growCallInfoWithArmedAllocator, 0);
    ASSERT_EQ(suite, LUA_ERRMEM, lua_pcall(L, 0, 0, 0), "zero-headroom hard limit rejects CallInfo growth");
    ASSERT_TRUE(suite, ledger.liveBytes <= gCallInfoHardLimit && ledger.peakBytes <= gCallInfoHardLimit,
                "CallInfo growth never exceeds the allocator hard limit");
    ASSERT_EQ(suite, static_cast<Lua::usize>(1), state->getCallStackSize(),
              "hard-limit CallInfo failure restores the base call depth");

    ledger.hardLimit = std::numeric_limits<size_t>::max();
    gCallInfoUseHardLimit = false;
    gAllocatorFailureProbe = nullptr;
    lua_settop(L, 0);
    const Lua::usize baseDepth = state->getCallStackSize();
    while (state->getCurrentCI() + 1 < state->getCallStack().size()) {
        (void)state->pushCallInfo();
    }
    (void)state->pushCallInfo();
    ASSERT_TRUE(suite, state->getCallStack().size() > Lua::INITIAL_CI_SIZE,
                "CallInfo growth succeeds after lifting the allocator hard limit");
    while (state->getCallStackSize() > baseDepth) {
        state->popCallInfo();
    }

    lua_pushcclosure(L, doubleApiArgument, 0);
    lua_pushnumber(L, 21);
    ASSERT_EQ(suite, LUA_OK, lua_pcall(L, 1, 1, 0), "state remains callable after CallInfo allocation rollback");
    ASSERT_EQ(suite, 42.0, lua_tonumber(L, -1), "post-rollback call returns the expected result");

    lua_close(L);
    ASSERT_TRUE(suite, ledger.blocks.empty() && ledger.liveBytes == 0,
                "stack and CallInfo transaction state closes with zero allocator ownership");
    ASSERT_TRUE(suite, ledger.sizeMismatches == 0 && ledger.unknownFrees == 0,
                "stack and CallInfo transactions preserve allocator contracts");
}

constexpr const char* kMetacallAllocatorSource = R"lua(
    __arm_metacall_allocator()
    return __allocator_callable(1, 2, 3, 4, 5, 6, 7, 8,
                                9, 10, 11, 12, 13, 14, 15, 16,
                                17, 18, 19, 20, 21, 22, 23, 24,
                                25, 26, 27, 28, 29, 30, 31, 32)
)lua";

void prepareMetacallAllocatorFixture(lua_State* L) {
    lua_settop(L, 0);
    lua_pushcclosure(L, armMetacallAllocatorFailure, 0);
    lua_setglobal(L, "__arm_metacall_allocator");

    lua_newtable(L);
    lua_newtable(L);
    lua_pushcclosure(L, returnMetacallArgumentCount, 0);
    lua_setfield(L, -2, "__call");
    (void)lua_setmetatable(L, -2);
    lua_setglobal(L, "__allocator_callable");

    if (luaL_loadstring(L, kMetacallAllocatorSource) != LUA_OK) {
        throw std::runtime_error("failed to compile __call allocator fixture");
    }
}

void testMetacallArgumentAllocatorTransactions(TestSuite& suite) {
    size_t metacallAllocationAttempts = 0;
    {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        ASSERT_TRUE(suite, L != nullptr, "__call allocator baseline creates a state");
        prepareMetacallAllocatorFixture(L);
        gAllocatorFailureProbe = &probe;
        gMetacallFailureOffset = 0;
        gMetacallUseHardLimit = false;

        ASSERT_EQ(suite, LUA_OK, lua_pcall(L, 0, 1, 0), "__call allocator baseline executes");
        metacallAllocationAttempts = probe.allocationAttempts - gMetacallAllocationStart;
        ASSERT_TRUE(suite, metacallAllocationAttempts > 0,
                    "__call argument staging observes callback-backed allocation attempts");
        ASSERT_EQ(suite, 33.0, lua_tonumber(L, -1), "__call receives the callable object and all explicit arguments");

        gAllocatorFailureProbe = nullptr;
        lua_close(L);
        ASSERT_TRUE(suite, ledger.blocks.empty() && ledger.liveBytes == 0,
                    "__call allocator baseline closes with zero ownership");
    }

    for (size_t offset = 1; offset <= metacallAllocationAttempts; ++offset) {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        ASSERT_TRUE(suite, L != nullptr, "__call fail-on-N test creates a state");
        prepareMetacallAllocatorFixture(L);
        gAllocatorFailureProbe = &probe;
        gMetacallFailureOffset = offset;
        gMetacallUseHardLimit = false;

        ASSERT_EQ(suite, LUA_ERRMEM, lua_pcall(L, 0, 1, 0), "__call argument staging failure becomes LUA_ERRMEM");
        ASSERT_EQ(suite, gMetacallAllocationStart + offset, probe.allocationAttempts,
                  "__call fail-on-N reaches the selected allocator attempt");
        ASSERT_TRUE(suite,
                    lua_gettop(L) == 1 && lua_isstring(L, -1) != 0 &&
                        std::string(lua_tostring(L, -1)) == "not enough memory",
                    "__call argument staging failure publishes one fixed memory error");

        probe.failOnAllocation = 0;
        gMetacallFailureOffset = 0;
        gAllocatorFailureProbe = nullptr;
        lua_settop(L, 0);
        prepareMetacallAllocatorFixture(L);
        ASSERT_EQ(suite, LUA_OK, lua_pcall(L, 0, 1, 0), "__call succeeds after disarming fail-on-N");
        ASSERT_EQ(suite, 33.0, lua_tonumber(L, -1), "__call retry preserves every argument");

        lua_close(L);
        ASSERT_TRUE(suite,
                    ledger.blocks.empty() && ledger.liveBytes == 0 && ledger.sizeMismatches == 0 &&
                        ledger.unknownFrees == 0,
                    "__call fail-on-N rollback closes with valid allocator ownership");
    }

    AllocatorLedger hardLimitLedger;
    AllocatorProbe hardLimitProbe{&hardLimitLedger};
    lua_State* hardLimitState = lua_newstate(trackingLuaAllocator, &hardLimitProbe);
    ASSERT_TRUE(suite, hardLimitState != nullptr, "__call hard-limit test creates a state");
    prepareMetacallAllocatorFixture(hardLimitState);
    gAllocatorFailureProbe = &hardLimitProbe;
    gMetacallFailureOffset = 0;
    gMetacallUseHardLimit = true;

    ASSERT_EQ(suite, LUA_ERRMEM, lua_pcall(hardLimitState, 0, 1, 0),
              "zero-headroom hard limit rejects __call argument staging");
    ASSERT_TRUE(suite,
                hardLimitLedger.liveBytes <= gMetacallHardLimit && hardLimitLedger.peakBytes <= gMetacallHardLimit,
                "__call argument staging never exceeds the allocator hard limit");
    ASSERT_TRUE(suite,
                lua_gettop(hardLimitState) == 1 && lua_isstring(hardLimitState, -1) != 0 &&
                    std::string(lua_tostring(hardLimitState, -1)) == "not enough memory",
                "__call hard-limit failure preserves the protected stack contract");

    hardLimitLedger.hardLimit = std::numeric_limits<size_t>::max();
    gMetacallUseHardLimit = false;
    gAllocatorFailureProbe = nullptr;
    lua_settop(hardLimitState, 0);
    prepareMetacallAllocatorFixture(hardLimitState);
    ASSERT_EQ(suite, LUA_OK, lua_pcall(hardLimitState, 0, 1, 0),
              "__call succeeds after lifting the allocator hard limit");
    ASSERT_EQ(suite, 33.0, lua_tonumber(hardLimitState, -1), "hard-limit retry preserves every __call argument");

    lua_close(hardLimitState);
    ASSERT_TRUE(suite,
                hardLimitLedger.blocks.empty() && hardLimitLedger.liveBytes == 0 &&
                    hardLimitLedger.sizeMismatches == 0 && hardLimitLedger.unknownFrees == 0,
                "__call hard-limit rollback closes with valid allocator ownership");
}

constexpr const char* kConcatAllocatorSource =
    "__arm_concat_allocator(); return __concat_allocator_left .. __concat_allocator_right";

void prepareConcatAllocatorFixture(lua_State* L) {
    lua_settop(L, 0);
    // Collector worklists have their own fail-on-N matrix. Keep this fixture
    // scoped to OP_CONCAT buffers so an automatic cycle cannot add unrelated
    // implementation-specific deallocation traffic after a successful retry.
    (void)lua_gc(L, LUA_GCSTOP, 0);
    lua_pushcclosure(L, armConcatAllocatorFailure, 0);
    lua_setglobal(L, "__arm_concat_allocator");

    const std::string left(96, 'L');
    const std::string right(96, 'R');
    lua_pushlstring(L, left.data(), left.size());
    lua_setglobal(L, "__concat_allocator_left");
    lua_pushlstring(L, right.data(), right.size());
    lua_setglobal(L, "__concat_allocator_right");

    if (luaL_loadstring(L, kConcatAllocatorSource) != LUA_OK) {
        throw std::runtime_error("failed to compile concat allocator fixture");
    }
}

bool concatAllocatorResultMatches(lua_State* L) {
    size_t length = 0;
    const char* text = lua_tolstring(L, -1, &length);
    return text != nullptr && length == 192 && std::string_view(text, 96) == std::string(96, 'L') &&
           std::string_view(text + 96, 96) == std::string(96, 'R');
}

void testConcatAllocatorTransactions(TestSuite& suite) {
    size_t concatAllocationAttempts = 0;
    {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        ASSERT_TRUE(suite, L != nullptr, "concat allocator baseline creates a state");
        prepareConcatAllocatorFixture(L);
        gAllocatorFailureProbe = &probe;
        gConcatFailureOffset = 0;
        gConcatUseHardLimit = false;

        ASSERT_EQ(suite, LUA_OK, lua_pcall(L, 0, 1, 0), "concat allocator baseline executes");
        concatAllocationAttempts = probe.allocationAttempts - gConcatAllocationStart;
        ASSERT_TRUE(suite, concatAllocationAttempts > 0, "concat path observes callback-backed allocation attempts");
        ASSERT_TRUE(suite, concatAllocatorResultMatches(L), "concat baseline preserves both long operands");

        gAllocatorFailureProbe = nullptr;
        lua_close(L);
        ASSERT_TRUE(suite,
                    ledger.blocks.empty() && ledger.liveBytes == 0 && ledger.sizeMismatches == 0 &&
                        ledger.unknownFrees == 0,
                    "concat allocator baseline closes with valid allocator ownership");
    }

    for (size_t offset = 1; offset <= concatAllocationAttempts; ++offset) {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        ASSERT_TRUE(suite, L != nullptr, "concat fail-on-N test creates a state");
        prepareConcatAllocatorFixture(L);
        gAllocatorFailureProbe = &probe;
        gConcatFailureOffset = offset;
        gConcatUseHardLimit = false;

        ASSERT_EQ(suite, LUA_ERRMEM, lua_pcall(L, 0, 1, 0), "concat allocation failure becomes LUA_ERRMEM");
        ASSERT_EQ(suite, gConcatAllocationStart + offset, probe.allocationAttempts,
                  "concat fail-on-N reaches the selected allocator attempt");
        ASSERT_TRUE(suite,
                    lua_gettop(L) == 1 && lua_isstring(L, -1) != 0 &&
                        std::string(lua_tostring(L, -1)) == "not enough memory",
                    "concat allocation failure publishes one fixed memory error");

        probe.failOnAllocation = 0;
        gConcatFailureOffset = 0;
        gAllocatorFailureProbe = nullptr;
        lua_settop(L, 0);
        prepareConcatAllocatorFixture(L);
        ASSERT_EQ(suite, LUA_OK, lua_pcall(L, 0, 1, 0), "concat succeeds after disarming fail-on-N");
        ASSERT_TRUE(suite, concatAllocatorResultMatches(L), "concat retry preserves both long operands");

        lua_close(L);
        ASSERT_TRUE(suite,
                    ledger.blocks.empty() && ledger.liveBytes == 0 && ledger.sizeMismatches == 0 &&
                        ledger.unknownFrees == 0,
                    "concat fail-on-N rollback closes with valid allocator ownership");
    }

    AllocatorLedger hardLimitLedger;
    AllocatorProbe hardLimitProbe{&hardLimitLedger};
    lua_State* hardLimitState = lua_newstate(trackingLuaAllocator, &hardLimitProbe);
    ASSERT_TRUE(suite, hardLimitState != nullptr, "concat hard-limit test creates a state");
    prepareConcatAllocatorFixture(hardLimitState);
    gAllocatorFailureProbe = &hardLimitProbe;
    gConcatFailureOffset = 0;
    gConcatUseHardLimit = true;

    ASSERT_EQ(suite, LUA_ERRMEM, lua_pcall(hardLimitState, 0, 1, 0), "zero-headroom hard limit rejects concat growth");
    ASSERT_TRUE(suite, hardLimitLedger.liveBytes <= gConcatHardLimit && hardLimitLedger.peakBytes <= gConcatHardLimit,
                "concat growth never exceeds the allocator hard limit");
    ASSERT_TRUE(suite,
                lua_gettop(hardLimitState) == 1 && lua_isstring(hardLimitState, -1) != 0 &&
                    std::string(lua_tostring(hardLimitState, -1)) == "not enough memory",
                "concat hard-limit failure preserves the protected stack contract");

    hardLimitLedger.hardLimit = std::numeric_limits<size_t>::max();
    gConcatUseHardLimit = false;
    gAllocatorFailureProbe = nullptr;
    lua_settop(hardLimitState, 0);
    prepareConcatAllocatorFixture(hardLimitState);
    ASSERT_EQ(suite, LUA_OK, lua_pcall(hardLimitState, 0, 1, 0),
              "concat succeeds after lifting the allocator hard limit");
    ASSERT_TRUE(suite, concatAllocatorResultMatches(hardLimitState),
                "hard-limit retry preserves both long concat operands");

    lua_close(hardLimitState);
    ASSERT_TRUE(suite,
                hardLimitLedger.blocks.empty() && hardLimitLedger.liveBytes == 0 &&
                    hardLimitLedger.sizeMismatches == 0 && hardLimitLedger.unknownFrees == 0,
                "concat hard-limit rollback closes with valid allocator ownership");
}

constexpr const char* kTableConcatAllocatorSource =
    "__arm_table_concat_allocator(); return table.concat(__table_concat_allocator_values, "
    "__table_concat_allocator_separator)";

void prepareTableConcatAllocatorFixture(lua_State* L) {
    lua_settop(L, 0);
    (void)luaopen_table(L);
    lua_pop(L, 1);

    lua_pushcclosure(L, armTableConcatAllocatorFailure, 0);
    lua_setglobal(L, "__arm_table_concat_allocator");

    const std::string left(96, 'L');
    const std::string right(96, 'R');
    lua_newtable(L);
    lua_pushlstring(L, left.data(), left.size());
    lua_rawseti(L, -2, 1);
    lua_pushnumber(L, 42.5);
    lua_rawseti(L, -2, 2);
    lua_pushlstring(L, right.data(), right.size());
    lua_rawseti(L, -2, 3);
    lua_setglobal(L, "__table_concat_allocator_values");
    lua_pushlstring(L, "::", 2);
    lua_setglobal(L, "__table_concat_allocator_separator");

    if (luaL_loadstring(L, kTableConcatAllocatorSource) != LUA_OK) {
        throw std::runtime_error("failed to compile table.concat allocator fixture");
    }
}

bool tableConcatAllocatorResultMatches(lua_State* L) {
    size_t length = 0;
    const char* text = lua_tolstring(L, -1, &length);
    const std::string expected = std::string(96, 'L') + "::42.5::" + std::string(96, 'R');
    return text != nullptr && length == expected.size() && std::string_view(text, length) == expected;
}

void testTableConcatAllocatorTransactions(TestSuite& suite) {
    size_t allocationAttempts = 0;
    {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        ASSERT_TRUE(suite, L != nullptr, "table.concat allocator baseline creates a state");
        prepareTableConcatAllocatorFixture(L);
        gAllocatorFailureProbe = &probe;
        gTableConcatFailureOffset = 0;
        gTableConcatUseHardLimit = false;

        ASSERT_EQ(suite, LUA_OK, lua_pcall(L, 0, 1, 0), "table.concat allocator baseline executes");
        allocationAttempts = probe.allocationAttempts - gTableConcatAllocationStart;
        ASSERT_TRUE(suite, allocationAttempts > 0, "table.concat observes callback-backed allocation attempts");
        ASSERT_TRUE(suite, tableConcatAllocatorResultMatches(L), "table.concat preserves strings and numbers");

        gAllocatorFailureProbe = nullptr;
        lua_close(L);
        ASSERT_TRUE(suite, ledger.blocks.empty() && ledger.liveBytes == 0,
                    "table.concat allocator baseline closes with zero ownership");
    }

    for (size_t offset = 1; offset <= allocationAttempts; ++offset) {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        ASSERT_TRUE(suite, L != nullptr, "table.concat fail-on-N test creates a state");
        prepareTableConcatAllocatorFixture(L);
        gAllocatorFailureProbe = &probe;
        gTableConcatFailureOffset = offset;
        gTableConcatUseHardLimit = false;

        ASSERT_EQ(suite, LUA_ERRMEM, lua_pcall(L, 0, 1, 0), "table.concat failure becomes LUA_ERRMEM");
        ASSERT_EQ(suite, gTableConcatAllocationStart + offset, probe.allocationAttempts,
                  "table.concat fail-on-N reaches the selected allocator attempt");
        ASSERT_TRUE(suite,
                    lua_gettop(L) == 1 && lua_isstring(L, -1) != 0 &&
                        std::string(lua_tostring(L, -1)) == "not enough memory",
                    "table.concat allocation failure publishes one fixed memory error");

        probe.failOnAllocation = 0;
        gTableConcatFailureOffset = 0;
        gAllocatorFailureProbe = nullptr;
        lua_settop(L, 0);
        prepareTableConcatAllocatorFixture(L);
        ASSERT_EQ(suite, LUA_OK, lua_pcall(L, 0, 1, 0), "table.concat succeeds after disarming fail-on-N");
        ASSERT_TRUE(suite, tableConcatAllocatorResultMatches(L), "table.concat retry preserves exact content");

        lua_close(L);
        ASSERT_TRUE(suite,
                    ledger.blocks.empty() && ledger.liveBytes == 0 && ledger.sizeMismatches == 0 &&
                        ledger.unknownFrees == 0,
                    "table.concat fail-on-N rollback closes with valid allocator ownership");
    }

    AllocatorLedger hardLimitLedger;
    AllocatorProbe hardLimitProbe{&hardLimitLedger};
    lua_State* hardLimitState = lua_newstate(trackingLuaAllocator, &hardLimitProbe);
    ASSERT_TRUE(suite, hardLimitState != nullptr, "table.concat hard-limit test creates a state");
    prepareTableConcatAllocatorFixture(hardLimitState);
    gAllocatorFailureProbe = &hardLimitProbe;
    gTableConcatFailureOffset = 0;
    gTableConcatUseHardLimit = true;

    ASSERT_EQ(suite, LUA_ERRMEM, lua_pcall(hardLimitState, 0, 1, 0),
              "zero-headroom hard limit rejects table.concat growth");
    ASSERT_TRUE(
        suite, hardLimitLedger.liveBytes <= gTableConcatHardLimit && hardLimitLedger.peakBytes <= gTableConcatHardLimit,
        "table.concat growth never exceeds the allocator hard limit");
    ASSERT_TRUE(suite,
                lua_gettop(hardLimitState) == 1 && lua_isstring(hardLimitState, -1) != 0 &&
                    std::string(lua_tostring(hardLimitState, -1)) == "not enough memory",
                "table.concat hard-limit failure preserves the protected stack contract");

    hardLimitLedger.hardLimit = std::numeric_limits<size_t>::max();
    gTableConcatUseHardLimit = false;
    gAllocatorFailureProbe = nullptr;
    lua_settop(hardLimitState, 0);
    prepareTableConcatAllocatorFixture(hardLimitState);
    ASSERT_EQ(suite, LUA_OK, lua_pcall(hardLimitState, 0, 1, 0),
              "table.concat succeeds after lifting the allocator hard limit");
    ASSERT_TRUE(suite, tableConcatAllocatorResultMatches(hardLimitState),
                "table.concat hard-limit retry preserves exact content");

    lua_close(hardLimitState);
    ASSERT_TRUE(suite,
                hardLimitLedger.blocks.empty() && hardLimitLedger.liveBytes == 0 &&
                    hardLimitLedger.sizeMismatches == 0 && hardLimitLedger.unknownFrees == 0,
                "table.concat hard-limit rollback closes with valid allocator ownership");
}

constexpr const char* kTableSortAllocatorSource =
    "__arm_table_sort_allocator(); table.sort(__table_sort_allocator_values, __table_sort_allocator_less); "
    "return __table_sort_allocator_values[1], __table_sort_allocator_values[16]";

void prepareTableSortAllocatorFixture(lua_State* L) {
    lua_settop(L, 0);
    (void)luaopen_table(L);
    lua_pop(L, 1);

    lua_pushcclosure(L, armTableSortAllocatorFailure, 0);
    lua_setglobal(L, "__arm_table_sort_allocator");
    lua_pushcclosure(L, compareTableSortNumbers, 0);
    lua_setglobal(L, "__table_sort_allocator_less");

    lua_newtable(L);
    for (int index = 1; index <= 16; ++index) {
        lua_pushinteger(L, 17 - index);
        lua_rawseti(L, -2, index);
    }
    lua_setglobal(L, "__table_sort_allocator_values");

    if (luaL_loadstring(L, kTableSortAllocatorSource) != LUA_OK) {
        throw std::runtime_error("failed to compile table.sort allocator fixture");
    }
}

bool tableSortAllocatorValuesMatch(lua_State* L, bool ascending) {
    lua_getglobal(L, "__table_sort_allocator_values");
    bool matches = lua_istable(L, -1) != 0;
    for (int index = 1; matches && index <= 16; ++index) {
        lua_rawgeti(L, -1, index);
        const lua_Number expected = ascending ? index : 17 - index;
        matches = lua_isnumber(L, -1) != 0 && lua_tonumber(L, -1) == expected;
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    return matches;
}

void testTableSortAllocatorTransactions(TestSuite& suite) {
    size_t allocationAttempts = 0;
    {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        ASSERT_TRUE(suite, L != nullptr, "table.sort allocator baseline creates a state");
        prepareTableSortAllocatorFixture(L);
        gAllocatorFailureProbe = &probe;
        gTableSortFailureOffset = 0;
        gTableSortUseHardLimit = false;

        ASSERT_EQ(suite, LUA_OK, lua_pcall(L, 0, 2, 0), "table.sort allocator baseline executes");
        allocationAttempts = probe.allocationAttempts - gTableSortAllocationStart;
        ASSERT_TRUE(suite, allocationAttempts > 1,
                    "table.sort observes result-copy and comparator-snapshot allocations");
        ASSERT_TRUE(suite, tableSortAllocatorValuesMatch(L, true), "table.sort baseline publishes sorted values");

        gAllocatorFailureProbe = nullptr;
        lua_close(L);
        ASSERT_TRUE(suite, ledger.blocks.empty() && ledger.liveBytes == 0,
                    "table.sort allocator baseline closes with zero ownership");
    }

    bool statesCreated = true;
    bool statusesAreMemoryErrors = true;
    bool targetsAreReached = true;
    bool errorsAreFixed = true;
    bool tablesRemainUnchanged = true;
    bool retriesSucceed = true;
    bool retryResultsAreSorted = true;
    bool statesCloseCleanly = true;
    for (size_t offset = 1; offset <= allocationAttempts; ++offset) {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        if (L == nullptr) {
            statesCreated = false;
            statesCloseCleanly = false;
            continue;
        }
        prepareTableSortAllocatorFixture(L);
        gAllocatorFailureProbe = &probe;
        gTableSortFailureOffset = offset;
        gTableSortUseHardLimit = false;

        const int status = lua_pcall(L, 0, 2, 0);
        statusesAreMemoryErrors = statusesAreMemoryErrors && status == LUA_ERRMEM;
        targetsAreReached = targetsAreReached && probe.allocationAttempts == gTableSortAllocationStart + offset;
        errorsAreFixed = errorsAreFixed && lua_gettop(L) == 1 && lua_isstring(L, -1) != 0 &&
                         std::string(lua_tostring(L, -1)) == "not enough memory";
        tablesRemainUnchanged = tablesRemainUnchanged && tableSortAllocatorValuesMatch(L, false);

        probe.failOnAllocation = 0;
        gTableSortFailureOffset = 0;
        gAllocatorFailureProbe = nullptr;
        lua_settop(L, 0);
        prepareTableSortAllocatorFixture(L);
        const int retryStatus = lua_pcall(L, 0, 2, 0);
        retriesSucceed = retriesSucceed && retryStatus == LUA_OK;
        retryResultsAreSorted =
            retryResultsAreSorted && retryStatus == LUA_OK && tableSortAllocatorValuesMatch(L, true);

        lua_close(L);
        statesCloseCleanly = statesCloseCleanly && ledger.blocks.empty() && ledger.liveBytes == 0 &&
                             ledger.sizeMismatches == 0 && ledger.unknownFrees == 0;
    }

    ASSERT_TRUE(suite, statesCreated, "table.sort fail-on-N scan creates every state");
    ASSERT_TRUE(suite, statusesAreMemoryErrors, "every table.sort allocation failure becomes LUA_ERRMEM");
    ASSERT_TRUE(suite, targetsAreReached, "table.sort scan reaches every observed allocator attempt");
    ASSERT_TRUE(suite, errorsAreFixed, "table.sort allocation failures publish one fixed memory error");
    ASSERT_TRUE(suite, tablesRemainUnchanged, "table.sort failures do not partially publish working copies");
    ASSERT_TRUE(suite, retriesSucceed, "table.sort succeeds after every injected allocation failure");
    ASSERT_TRUE(suite, retryResultsAreSorted, "every table.sort retry publishes sorted values");
    ASSERT_TRUE(suite, statesCloseCleanly, "table.sort fail-on-N scan closes with valid allocator ownership");

    AllocatorLedger hardLimitLedger;
    AllocatorProbe hardLimitProbe{&hardLimitLedger};
    lua_State* hardLimitState = lua_newstate(trackingLuaAllocator, &hardLimitProbe);
    ASSERT_TRUE(suite, hardLimitState != nullptr, "table.sort hard-limit test creates a state");
    prepareTableSortAllocatorFixture(hardLimitState);
    gAllocatorFailureProbe = &hardLimitProbe;
    gTableSortFailureOffset = 0;
    gTableSortUseHardLimit = true;

    ASSERT_EQ(suite, LUA_ERRMEM, lua_pcall(hardLimitState, 0, 2, 0),
              "zero-headroom hard limit rejects table.sort growth");
    ASSERT_TRUE(suite,
                hardLimitLedger.liveBytes <= gTableSortHardLimit && hardLimitLedger.peakBytes <= gTableSortHardLimit,
                "table.sort growth never exceeds the allocator hard limit");
    ASSERT_TRUE(suite,
                lua_gettop(hardLimitState) == 1 && lua_isstring(hardLimitState, -1) != 0 &&
                    std::string(lua_tostring(hardLimitState, -1)) == "not enough memory",
                "table.sort hard-limit failure preserves the protected stack contract");
    ASSERT_TRUE(suite, tableSortAllocatorValuesMatch(hardLimitState, false),
                "table.sort hard-limit failure leaves the target table unchanged");

    hardLimitLedger.hardLimit = std::numeric_limits<size_t>::max();
    gTableSortUseHardLimit = false;
    gAllocatorFailureProbe = nullptr;
    lua_settop(hardLimitState, 0);
    prepareTableSortAllocatorFixture(hardLimitState);
    ASSERT_EQ(suite, LUA_OK, lua_pcall(hardLimitState, 0, 2, 0),
              "table.sort succeeds after lifting the allocator hard limit");
    ASSERT_TRUE(suite, tableSortAllocatorValuesMatch(hardLimitState, true),
                "table.sort hard-limit retry publishes sorted values");

    lua_close(hardLimitState);
    ASSERT_TRUE(suite,
                hardLimitLedger.blocks.empty() && hardLimitLedger.liveBytes == 0 &&
                    hardLimitLedger.sizeMismatches == 0 && hardLimitLedger.unknownFrees == 0,
                "table.sort hard-limit rollback closes with valid allocator ownership");
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
                   first_error == "unhandled C++ exception in protected Lua API",
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
                   first_error == "unhandled C++ exception in protected Lua API",
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

void testFragmentedReaderAllocatorTransactions(TestSuite& suite) {
    size_t bufferAllocationAttempts = 0;
    {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        ASSERT_TRUE(suite, L != nullptr, "fragmented reader baseline creates state");
        int prefixMarker = 0;
        lua_pushlightuserdata(L, &prefixMarker);

        FragmentedAllocatorReader reader;
        gAllocatorFailureProbe = &probe;
        gFragmentedReaderFailureOffset = 0;
        gFragmentedReaderUseHardLimit = false;
        const int status = lua_load(L, readFragmentedAllocatorChunk, &reader, "=fragmented-reader");
        bufferAllocationAttempts = gFragmentedReaderBufferAttempts;
        gAllocatorFailureProbe = nullptr;

        ASSERT_EQ(suite, LUA_OK, status, "fragmented reader baseline compiles");
        ASSERT_TRUE(suite, bufferAllocationAttempts > 0,
                    "fragmented reader source buffer routes growth through lua_Alloc");
        ASSERT_TRUE(suite, lua_gettop(L) == 2 && lua_touserdata(L, 1) == &prefixMarker,
                    "fragmented reader baseline preserves its stack prefix");
        ASSERT_EQ(suite, LUA_OK, lua_pcall(L, 0, 1, 0), "fragmented reader baseline executes");
        ASSERT_EQ(suite, 64.0, lua_tonumber(L, -1), "fragmented reader baseline returns the expected value");

        lua_close(L);
        ASSERT_TRUE(suite, ledger.blocks.empty() && ledger.liveBytes == 0,
                    "fragmented reader baseline closes with zero allocator ownership");
        ASSERT_TRUE(suite, ledger.sizeMismatches == 0 && ledger.unknownFrees == 0,
                    "fragmented reader baseline preserves allocator contracts");
    }

    bool statusesAreMemoryErrors = true;
    bool targetsAreReached = true;
    bool errorsAreFixed = true;
    bool prefixesArePreserved = true;
    bool retriesSucceed = true;
    bool statesCloseCleanly = true;
    for (size_t offset = 1; offset <= bufferAllocationAttempts; ++offset) {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        if (L == nullptr) {
            statusesAreMemoryErrors = false;
            statesCloseCleanly = false;
            continue;
        }
        int prefixMarker = 0;
        lua_pushlightuserdata(L, &prefixMarker);

        FragmentedAllocatorReader reader;
        gAllocatorFailureProbe = &probe;
        gFragmentedReaderFailureOffset = offset;
        gFragmentedReaderUseHardLimit = false;
        const int status = lua_load(L, readFragmentedAllocatorChunk, &reader, "=fragmented-reader-oom");
        const size_t attempts = probe.allocationAttempts - gFragmentedReaderAllocationStart;
        statusesAreMemoryErrors = statusesAreMemoryErrors && status == LUA_ERRMEM;
        targetsAreReached = targetsAreReached && attempts == offset;
        errorsAreFixed = errorsAreFixed && lua_gettop(L) == 2 && lua_isstring(L, -1) != 0 &&
                         std::string(lua_tostring(L, -1)) == "not enough memory";
        prefixesArePreserved = prefixesArePreserved && lua_gettop(L) == 2 && lua_touserdata(L, 1) == &prefixMarker;

        probe.failOnAllocation = 0;
        ledger.hardLimit = std::numeric_limits<size_t>::max();
        gFragmentedReaderFailureOffset = 0;
        lua_settop(L, 1);
        FragmentedAllocatorReader retryReader;
        const int retryLoadStatus = lua_load(L, readFragmentedAllocatorChunk, &retryReader, "=fragmented-reader-retry");
        const int retryCallStatus = retryLoadStatus == LUA_OK ? lua_pcall(L, 0, 1, 0) : retryLoadStatus;
        retriesSucceed = retriesSucceed && retryCallStatus == LUA_OK && lua_tonumber(L, -1) == 64;

        gAllocatorFailureProbe = nullptr;
        lua_close(L);
        statesCloseCleanly = statesCloseCleanly && ledger.blocks.empty() && ledger.liveBytes == 0 &&
                             ledger.sizeMismatches == 0 && ledger.unknownFrees == 0;
    }

    ASSERT_TRUE(suite, statusesAreMemoryErrors, "every fragmented reader buffer failure becomes LUA_ERRMEM");
    ASSERT_TRUE(suite, targetsAreReached, "fragmented reader scan reaches every observed buffer allocation");
    ASSERT_TRUE(suite, errorsAreFixed, "fragmented reader OOM publishes the fixed memory error object");
    ASSERT_TRUE(suite, prefixesArePreserved, "fragmented reader OOM preserves the caller stack prefix");
    ASSERT_TRUE(suite, retriesSucceed, "fragmented reader remains usable after every buffer allocation failure");
    ASSERT_TRUE(suite, statesCloseCleanly, "fragmented reader failure scan closes without allocator leaks");

    {
        AllocatorLedger ledger;
        AllocatorProbe probe{&ledger};
        lua_State* L = lua_newstate(trackingLuaAllocator, &probe);
        ASSERT_TRUE(suite, L != nullptr, "fragmented reader hard-limit test creates state");
        int prefixMarker = 0;
        lua_pushlightuserdata(L, &prefixMarker);

        FragmentedAllocatorReader reader;
        gAllocatorFailureProbe = &probe;
        gFragmentedReaderFailureOffset = 0;
        gFragmentedReaderUseHardLimit = true;
        gFragmentedReaderHardLimit = 0;
        const int status = lua_load(L, readFragmentedAllocatorChunk, &reader, "=fragmented-reader-limit");
        ASSERT_EQ(suite, LUA_ERRMEM, status, "zero-headroom hard limit rejects fragmented reader growth");
        ASSERT_TRUE(suite,
                    gFragmentedReaderHardLimit != 0 && ledger.peakBytes <= gFragmentedReaderHardLimit &&
                        ledger.liveBytes <= gFragmentedReaderHardLimit,
                    "fragmented reader never exceeds the host hard limit");
        ASSERT_TRUE(suite, lua_gettop(L) == 2 && lua_touserdata(L, 1) == &prefixMarker,
                    "fragmented reader hard-limit failure preserves its stack prefix");

        ledger.hardLimit = std::numeric_limits<size_t>::max();
        probe.failOnAllocation = 0;
        gFragmentedReaderUseHardLimit = false;
        lua_settop(L, 1);
        FragmentedAllocatorReader retryReader;
        ASSERT_EQ(suite, LUA_OK, lua_load(L, readFragmentedAllocatorChunk, &retryReader, "=fragmented-reader-limit"),
                  "fragmented reader compiles after lifting the hard limit");
        ASSERT_EQ(suite, LUA_OK, lua_pcall(L, 0, 1, 0),
                  "fragmented reader result executes after lifting the hard limit");
        ASSERT_EQ(suite, 64.0, lua_tonumber(L, -1), "fragmented reader hard-limit retry returns the expected value");

        gAllocatorFailureProbe = nullptr;
        lua_close(L);
        ASSERT_TRUE(suite, ledger.blocks.empty() && ledger.liveBytes == 0,
                    "fragmented reader hard-limit state closes with zero allocator ownership");
        ASSERT_TRUE(suite, ledger.sizeMismatches == 0 && ledger.unknownFrees == 0,
                    "fragmented reader hard-limit path preserves allocator contracts");
    }

    gFragmentedReaderFailureOffset = 0;
    gFragmentedReaderBufferAttempts = 0;
    gFragmentedReaderHardLimit = 0;
    gFragmentedReaderUseHardLimit = false;
}

void testLoadBufferAllocatorFailures(TestSuite& suite) {
    constexpr const char* source =
        "local allocator_backed_identifier_name = {}; for i = 1, 8 do "
        "allocator_backed_identifier_name[i] = i end; return allocator_backed_identifier_name[8]";

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

    AllocatorLedger hardLimitLedger;
    AllocatorProbe hardLimitProbe{&hardLimitLedger};
    lua_State* hardLimitState = lua_newstate(trackingLuaAllocator, &hardLimitProbe);
    ASSERT_TRUE(suite, hardLimitState != nullptr, "loadbuffer hard-limit test creates state");
    hardLimitLedger.hardLimit = hardLimitLedger.liveBytes;
    hardLimitLedger.peakBytes = hardLimitLedger.liveBytes;
    const size_t hardLimit = hardLimitLedger.hardLimit;
    ASSERT_EQ(suite, LUA_ERRMEM, luaL_loadbuffer(hardLimitState, source, std::strlen(source), "=hard-limit-loadbuffer"),
              "zero-headroom hard limit rejects loadbuffer growth");
    ASSERT_TRUE(suite, hardLimitLedger.peakBytes <= hardLimit && hardLimitLedger.liveBytes <= hardLimit,
                "loadbuffer never exceeds the host hard limit");
    ASSERT_TRUE(suite,
                lua_gettop(hardLimitState) == 1 && lua_isstring(hardLimitState, -1) != 0 &&
                    std::string(lua_tostring(hardLimitState, -1)) == "not enough memory",
                "loadbuffer hard-limit failure publishes the fixed memory error");

    hardLimitLedger.hardLimit = std::numeric_limits<size_t>::max();
    hardLimitProbe.failOnAllocation = 0;
    lua_settop(hardLimitState, 0);
    ASSERT_EQ(suite, LUA_OK, luaL_loadbuffer(hardLimitState, source, std::strlen(source), "=hard-limit-loadbuffer"),
              "loadbuffer compiles after lifting the hard limit");
    ASSERT_EQ(suite, LUA_OK, lua_pcall(hardLimitState, 0, 1, 0), "loadbuffer hard-limit retry executes");
    ASSERT_EQ(suite, 8.0, lua_tonumber(hardLimitState, -1), "loadbuffer hard-limit retry returns the expected value");

    lua_close(hardLimitState);
    ASSERT_TRUE(suite, hardLimitLedger.blocks.empty() && hardLimitLedger.liveBytes == 0,
                "loadbuffer hard-limit state closes with zero allocator ownership");
    ASSERT_TRUE(suite, hardLimitLedger.sizeMismatches == 0 && hardLimitLedger.unknownFrees == 0,
                "loadbuffer hard-limit path preserves allocator contracts");
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

void testPublicTableTraversalApi(TestSuite& suite) {
    lua_State* L = lua_open();
    lua_newtable(L);

    lua_pushnumber(L, 7);
    lua_setfield(L, 1, "answer");
    ASSERT_EQ(suite, 1, lua_gettop(L), "setfield consumes exactly its value");
    lua_getfield(L, 1, "answer");
    ASSERT_EQ(suite, 7.0, lua_tonumber(L, -1), "getfield reads an existing field");
    lua_pop(L, 1);

    lua_pushstring(L, "raw");
    lua_pushnumber(L, 8);
    lua_rawset(L, 1);
    ASSERT_EQ(suite, 1, lua_gettop(L), "rawset consumes its key and value");
    lua_pushstring(L, "raw");
    lua_rawget(L, 1);
    ASSERT_EQ(suite, 8.0, lua_tonumber(L, -1), "rawget replaces its key with the raw value");
    lua_pop(L, 1);

    lua_pushnumber(L, 9);
    lua_setfield(L, -2, "negative");
    lua_getfield(L, -1, "negative");
    ASSERT_EQ(suite, 9.0, lua_tonumber(L, -1), "field APIs resolve negative table indexes before stack changes");
    lua_pop(L, 1);

    lua_newtable(L);
    lua_pushstring(L, "__index");
    lua_pushcclosure(L, returnFallbackField, 0);
    lua_rawset(L, -3);
    ASSERT_EQ(suite, 1, lua_setmetatable(L, 1), "table accepts an index metatable");

    lua_getfield(L, 1, "missing");
    ASSERT_EQ(suite, std::string("fallback"), std::string(lua_tostring(L, -1)),
              "getfield invokes the __index metamethod");
    lua_pop(L, 1);
    lua_pushstring(L, "missing");
    lua_rawget(L, 1);
    ASSERT_EQ(suite, LUA_TNIL, lua_type(L, -1), "rawget bypasses the __index metamethod");
    lua_pop(L, 1);

    lua_newtable(L);
    ASSERT_EQ(suite, 1, lua_getmetatable(L, 1), "table retains its index metatable");
    lua_pushstring(L, "__newindex");
    lua_pushvalue(L, 2);
    lua_pushcclosure(L, captureNewField, 1);
    lua_rawset(L, -3);
    lua_pop(L, 1);

    lua_pushnumber(L, 12);
    lua_setfield(L, 1, "captured");
    ASSERT_EQ(suite, 2, lua_gettop(L), "setfield consumes its value after __newindex returns");
    lua_getfield(L, 2, "captured");
    ASSERT_EQ(suite, 12.0, lua_tonumber(L, -1), "setfield routes absent fields through __newindex");
    lua_pop(L, 1);
    lua_getfield(L, 1, "captured");
    ASSERT_EQ(suite, std::string("fallback"), std::string(lua_tostring(L, -1)),
              "__newindex capture leaves the original table field absent");
    lua_pop(L, 1);

    lua_pushstring(L, "direct");
    lua_pushnumber(L, 13);
    lua_rawset(L, 1);
    lua_getfield(L, 1, "direct");
    ASSERT_EQ(suite, 13.0, lua_tonumber(L, -1), "rawset bypasses the __newindex metamethod");
    lua_pop(L, 1);
    lua_getfield(L, 2, "direct");
    ASSERT_EQ(suite, LUA_TNIL, lua_type(L, -1), "rawset does not write through the __newindex sink");
    lua_pop(L, 1);

    lua_pushnumber(L, 21);
    lua_setfield(L, LUA_REGISTRYINDEX, "field-contract");
    lua_getfield(L, LUA_REGISTRYINDEX, "field-contract");
    ASSERT_EQ(suite, 21.0, lua_tonumber(L, -1), "field APIs support the registry pseudo-index");
    lua_pop(L, 1);
    lua_pushstring(L, "raw-contract");
    lua_pushnumber(L, 22);
    lua_rawset(L, LUA_GLOBALSINDEX);
    lua_pushstring(L, "raw-contract");
    lua_rawget(L, LUA_GLOBALSINDEX);
    ASSERT_EQ(suite, 22.0, lua_tonumber(L, -1), "raw APIs support the globals pseudo-index");
    lua_pop(L, 1);

    lua_settop(L, 0);
    lua_newtable(L);
    lua_pushnumber(L, 22);
    lua_rawseti(L, 1, 2);
    lua_pushnumber(L, 11);
    lua_setfield(L, 1, "alpha");
    lua_pushnil(L);

    bool sawArrayValue = false;
    bool sawHashValue = false;
    int entries = 0;
    while (lua_next(L, 1) != 0) {
        ++entries;
        if (lua_type(L, -2) == LUA_TNUMBER && lua_tonumber(L, -2) == 2) {
            sawArrayValue = lua_tonumber(L, -1) == 22;
        }
        if (lua_type(L, -2) == LUA_TSTRING && std::string(lua_tostring(L, -2)) == "alpha") {
            sawHashValue = lua_tonumber(L, -1) == 11;
        }
        lua_pop(L, 1);
        ASSERT_EQ(suite, 2, lua_gettop(L), "lua_next keeps only the table and current key between iterations");
    }
    ASSERT_EQ(suite, 2, entries, "lua_next visits every non-nil table entry exactly once");
    ASSERT_TRUE(suite, sawArrayValue && sawHashValue, "lua_next returns both array and hash key/value pairs");
    ASSERT_EQ(suite, 1, lua_gettop(L), "lua_next pops the final key at end of traversal");

    lua_newtable(L);
    lua_pushnil(L);
    ASSERT_EQ(suite, 0, lua_next(L, 2), "lua_next reports the end of an empty table");
    ASSERT_EQ(suite, 2, lua_gettop(L), "empty-table traversal pops its initial nil key");

    lua_close(L);
}

void testPublicComparisonAndConcatApi(TestSuite& suite) {
    lua_State* L = lua_open();

    ASSERT_EQ(suite, 0, lua_equal(L, 1, 2), "lua_equal rejects missing indexes");
    ASSERT_EQ(suite, 0, lua_rawequal(L, 1, 2), "lua_rawequal rejects missing indexes");
    ASSERT_EQ(suite, 0, lua_lessthan(L, 1, 2), "lua_lessthan rejects missing indexes");

    lua_pushnumber(L, 4);
    lua_pushnumber(L, 4);
    lua_pushnumber(L, 5);
    ASSERT_EQ(suite, 1, lua_equal(L, 1, 2), "lua_equal compares primitive numbers");
    ASSERT_EQ(suite, 1, lua_rawequal(L, 1, 2), "lua_rawequal compares primitive numbers");
    ASSERT_EQ(suite, 1, lua_lessthan(L, 2, 3), "lua_lessthan compares primitive numbers");
    lua_settop(L, 0);

    lua_newtable(L);
    lua_pushnumber(L, 1);
    lua_setfield(L, 1, "id");
    lua_newtable(L);
    lua_pushnumber(L, 1);
    lua_setfield(L, 2, "id");
    lua_newtable(L);
    lua_pushstring(L, "__eq");
    lua_pushcclosure(L, compareIdsEqual, 0);
    lua_rawset(L, 3);
    lua_pushstring(L, "__lt");
    lua_pushcclosure(L, compareIdsLess, 0);
    lua_rawset(L, 3);
    lua_pushvalue(L, 3);
    ASSERT_EQ(suite, 1, lua_setmetatable(L, 1), "first comparison table accepts the shared metatable");
    lua_pushvalue(L, 3);
    ASSERT_EQ(suite, 1, lua_setmetatable(L, 2), "second comparison table accepts the shared metatable");

    ASSERT_EQ(suite, 1, lua_equal(L, 1, 2), "lua_equal invokes the shared __eq metamethod");
    ASSERT_EQ(suite, 0, lua_rawequal(L, 1, 2), "lua_rawequal bypasses the __eq metamethod");
    ASSERT_EQ(suite, 1, lua_rawequal(L, 1, 1), "lua_rawequal recognizes identical table objects");
    lua_pushnumber(L, 2);
    lua_setfield(L, 2, "id");
    ASSERT_EQ(suite, 1, lua_lessthan(L, 1, 2), "lua_lessthan invokes the shared __lt metamethod");

    lua_settop(L, 0);
    lua_concat(L, 0);
    size_t length = 1;
    const char* empty = lua_tolstring(L, -1, &length);
    ASSERT_TRUE(suite, empty != nullptr && length == 0, "lua_concat with zero operands pushes an empty string");
    lua_pop(L, 1);

    lua_pushstring(L, "identity");
    lua_concat(L, 1);
    ASSERT_EQ(suite, 1, lua_gettop(L), "lua_concat with one operand leaves the stack unchanged");
    ASSERT_EQ(suite, std::string("identity"), std::string(lua_tostring(L, -1)),
              "lua_concat with one operand preserves its value");
    lua_settop(L, 0);

    lua_pushstring(L, "value=");
    lua_pushnumber(L, 12);
    lua_concat(L, 2);
    ASSERT_EQ(suite, std::string("value=12"), std::string(lua_tostring(L, -1)),
              "lua_concat converts numbers and reduces its operands to one value");
    lua_settop(L, 0);

    constexpr char left[] = {'a', '\0', 'b'};
    constexpr char right[] = {'c', '\0', 'd'};
    lua_pushlstring(L, left, sizeof(left));
    lua_pushlstring(L, right, sizeof(right));
    lua_concat(L, 2);
    const char* joined = lua_tolstring(L, -1, &length);
    ASSERT_EQ(suite, static_cast<size_t>(6), length, "lua_concat preserves embedded NUL byte lengths");
    ASSERT_TRUE(suite, joined != nullptr && std::memcmp(joined, "a\0bc\0d", 6) == 0,
                "lua_concat preserves embedded NUL byte contents");
    lua_settop(L, 0);

    lua_newtable(L);
    lua_newtable(L);
    lua_newtable(L);
    lua_pushstring(L, "__concat");
    lua_pushcclosure(L, returnConcatFallback, 0);
    lua_rawset(L, 3);
    lua_pushvalue(L, 3);
    ASSERT_EQ(suite, 1, lua_setmetatable(L, 1), "left concat table accepts a metatable");
    lua_pushvalue(L, 3);
    ASSERT_EQ(suite, 1, lua_setmetatable(L, 2), "right concat table accepts a metatable");
    lua_pushvalue(L, 1);
    lua_setfield(L, LUA_REGISTRYINDEX, "concat-left");
    lua_pushvalue(L, 2);
    lua_setfield(L, LUA_REGISTRYINDEX, "concat-right");
    lua_remove(L, 3);
    lua_concat(L, 2);
    ASSERT_EQ(suite, std::string("joined"), std::string(lua_tostring(L, -1)),
              "lua_concat invokes the __concat metamethod");

    lua_settop(L, 0);
    auto* state = reinterpret_cast<Lua::LuaState*>(L);
    const Lua::usize frameBase = state->getCurrentCallInfo().base;
    const int prefixSize = static_cast<int>(state->getStack().capacity() - frameBase - 2);
    lua_settop(L, prefixSize);
    lua_getfield(L, LUA_REGISTRYINDEX, "concat-left");
    lua_getfield(L, LUA_REGISTRYINDEX, "concat-right");
    lua_concat(L, 2);
    ASSERT_EQ(suite, prefixSize + 1, lua_gettop(L),
              "metamethod concat survives stack storage growth and still reduces two operands");
    ASSERT_EQ(suite, std::string("joined"), std::string(lua_tostring(L, -1)),
              "metamethod concat refreshes operand storage after stack growth");

    lua_settop(L, 0);
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, "concat-left");
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, "concat-right");
    lua_pushboolean(L, 1);
    lua_pushboolean(L, 0);
    bool rejected = false;
    try {
        lua_concat(L, 2);
    } catch (const Lua::RuntimeError&) {
        rejected = true;
    }
    ASSERT_TRUE(suite, rejected, "lua_concat reports incompatible operands to an unprotected C++ caller");
    ASSERT_EQ(suite, 2, lua_gettop(L), "failed lua_concat retains its operand stack");

    lua_close(L);
}

void testPublicTypeThreadAndGcApi(TestSuite& suite) {
    lua_State* L = lua_open();

    lua_pushnumber(L, 42.0);
    lua_pushstring(L, "-17");
    lua_pushboolean(L, 1);
    ASSERT_EQ(suite, static_cast<lua_Integer>(42), lua_tointeger(L, 1),
              "lua_tointeger converts integral numeric values");
    ASSERT_EQ(suite, static_cast<lua_Integer>(-17), lua_tointeger(L, 2), "lua_tointeger accepts numeric strings");
    ASSERT_EQ(suite, static_cast<lua_Integer>(0), lua_tointeger(L, 3),
              "lua_tointeger returns zero for non-numeric values");
    lua_settop(L, 0);

    lua_pushcclosure(L, returnFallbackField, 0);
    ASSERT_TRUE(suite, lua_tocfunction(L, -1) == returnFallbackField,
                "lua_tocfunction preserves a public C callback identity");
    ASSERT_TRUE(suite, lua_topointer(L, -1) != nullptr, "lua_topointer exposes stable function identity");
    const void* functionIdentity = lua_topointer(L, -1);
    ASSERT_TRUE(suite, functionIdentity == lua_topointer(L, -1), "function pointer identity is stable");
    lua_pop(L, 1);
    luaL_openlibs(L);
    lua_getglobal(L, "print");
    ASSERT_TRUE(suite, lua_iscfunction(L, -1) != 0 && lua_tocfunction(L, -1) != nullptr,
                "lua_tocfunction exposes registered standard-library C functions");
    lua_pop(L, 1);

    lua_newtable(L);
    const void* tableIdentity = lua_topointer(L, -1);
    ASSERT_TRUE(suite, tableIdentity != nullptr && tableIdentity == lua_topointer(L, -1),
                "lua_topointer exposes stable table identity");
    lua_pushvalue(L, -1);
    ASSERT_TRUE(suite, tableIdentity == lua_topointer(L, -1), "copied table values retain pointer identity");
    lua_pop(L, 2);
    int light = 0;
    lua_pushlightuserdata(L, &light);
    ASSERT_TRUE(suite, lua_topointer(L, -1) == &light, "lua_topointer preserves light userdata pointers");
    lua_pop(L, 1);
    void* userdata = lua_newuserdata(L, sizeof(int));
    ASSERT_TRUE(suite, lua_topointer(L, -1) == userdata, "lua_topointer returns full userdata payload identity");
    lua_pop(L, 1);
    lua_pushnumber(L, 1);
    ASSERT_TRUE(suite, lua_topointer(L, -1) == nullptr, "lua_topointer rejects primitive values");
    lua_pop(L, 1);

    ASSERT_EQ(suite, 1, lua_pushthread(L), "lua_pushthread identifies the main state");
    ASSERT_EQ(suite, LUA_TTHREAD, lua_type(L, -1), "lua_pushthread pushes a thread value for the main state");
    ASSERT_TRUE(suite, lua_tothread(L, -1) == L, "lua_tothread round-trips the main state");
    ASSERT_TRUE(suite, lua_topointer(L, -1) == L, "main thread pointer identity matches its state");
    lua_pop(L, 1);

    lua_State* child = lua_newthread(L);
    ASSERT_TRUE(suite, child != nullptr && lua_tothread(L, -1) == child,
                "lua_tothread returns the state published by lua_newthread");
    ASSERT_TRUE(suite, lua_topointer(L, -1) == child, "coroutine pointer identity matches its state");
    ASSERT_EQ(suite, 0, lua_pushthread(child), "lua_pushthread distinguishes a coroutine from the main state");
    ASSERT_TRUE(suite, lua_tothread(child, -1) == child, "a coroutine pushes and round-trips itself");
    lua_pop(child, 1);
    lua_pop(L, 1);
    ASSERT_TRUE(suite, lua_tothread(L, 1) == nullptr, "lua_tothread rejects missing indexes");

    auto* state = reinterpret_cast<Lua::LuaState*>(L);
    Lua::GarbageCollector& gc = state->getGlobalState().getGC();
    const int countKilobytes = lua_gc(L, LUA_GCCOUNT, 0);
    const int countRemainder = lua_gc(L, LUA_GCCOUNTB, 0);
    ASSERT_EQ(suite, gc.getTotalMemory(),
              static_cast<Lua::usize>(countKilobytes) * 1024U + static_cast<Lua::usize>(countRemainder),
              "lua_gc count and countb reconstruct managed memory bytes");
    ASSERT_EQ(suite, 200, lua_gc(L, LUA_GCSETPAUSE, 250), "lua_gc setpause returns the previous value");
    ASSERT_EQ(suite, 250, gc.getPause(), "lua_gc setpause stores the requested value");
    ASSERT_EQ(suite, 200, lua_gc(L, LUA_GCSETSTEPMUL, 350), "lua_gc setstepmul returns the previous value");
    ASSERT_EQ(suite, 350, gc.getStepMultiplier(), "lua_gc setstepmul stores the requested value");
    ASSERT_EQ(suite, 0, lua_gc(L, LUA_GCSTOP, 0), "lua_gc stop reports success");
    ASSERT_TRUE(suite, gc.isAutomaticStopped(), "lua_gc stop disables automatic collection");
    ASSERT_EQ(suite, 0, lua_gc(L, LUA_GCRESTART, 0), "lua_gc restart reports success");
    ASSERT_TRUE(suite, !gc.isAutomaticStopped(), "lua_gc restart enables automatic collection");
    const int stepResult = lua_gc(L, LUA_GCSTEP, 1);
    ASSERT_TRUE(suite, stepResult == 0 || stepResult == 1, "lua_gc step reports an incomplete or completed cycle");
    ASSERT_EQ(suite, 0, lua_gc(L, LUA_GCCOLLECT, 0), "lua_gc full collection reports success");
    ASSERT_TRUE(suite, !gc.isAutomaticStopped(), "lua_gc full collection leaves automatic collection running");
    ASSERT_EQ(suite, -1, lua_gc(L, 999, 0), "lua_gc rejects unknown operations");

    (void)lua_gc(L, LUA_GCSETPAUSE, 200);
    (void)lua_gc(L, LUA_GCSETSTEPMUL, 200);
    lua_close(L);
}

void testPublicAuxiliaryChecksAndMetatables(TestSuite& suite) {
    lua_State* freshState = luaL_newstate();
    ASSERT_TRUE(suite, freshState != nullptr, "luaL_newstate creates an independent state");
    lua_close(freshState);

    lua_State* L = lua_open();
    lua_pushnumber(L, 18.0);
    lua_pushstring(L, "beta");
    lua_pushnil(L);

    size_t length = 0;
    ASSERT_EQ(suite, static_cast<lua_Integer>(18), luaL_checkinteger(L, 1),
              "luaL_checkinteger converts numeric arguments");
    ASSERT_EQ(suite, static_cast<lua_Integer>(41), luaL_optinteger(L, 3, 41),
              "luaL_optinteger uses its default for nil");
    ASSERT_EQ(suite, 12.5, luaL_optnumber(L, 4, 12.5), "luaL_optnumber uses its default for missing arguments");
    ASSERT_EQ(suite, std::string("fallback"), std::string(luaL_optlstring(L, 3, "fallback", &length)),
              "luaL_optlstring uses its default for nil");
    ASSERT_EQ(suite, static_cast<size_t>(8), length, "luaL_optlstring publishes the default byte length");
    luaL_checktype(L, 2, LUA_TSTRING);
    luaL_checkany(L, 3);
    luaL_checkstack(L, 8, "auxiliary check");
    ASSERT_TRUE(suite, true, "luaL_checktype, luaL_checkany, and luaL_checkstack accept valid inputs");

    static const char* const options[] = {"alpha", "beta", nullptr};
    ASSERT_EQ(suite, 1, luaL_checkoption(L, 2, nullptr, options), "luaL_checkoption returns the matching index");
    ASSERT_EQ(suite, 0, luaL_checkoption(L, 4, "alpha", options),
              "luaL_checkoption accepts a default for missing arguments");

    luaL_where(L, 0);
    ASSERT_EQ(suite, std::string(""), std::string(lua_tostring(L, -1)),
              "luaL_where publishes an empty prefix when no source frame exists");
    lua_pop(L, 1);

    lua_pushcclosure(L, raiseAuxiliaryTypeError, 0);
    ASSERT_EQ(suite, LUA_ERRRUN, lua_pcall(L, 0, 0, 0), "luaL_typerror raises a protected runtime error");
    ASSERT_TRUE(suite, std::string(lua_tostring(L, -1)).find("widget expected") != std::string::npos,
                "luaL_typerror describes the expected type");
    lua_pop(L, 1);

    lua_settop(L, 0);
    lua_pushcclosure(L, raiseAuxiliaryArgumentError, 0);
    lua_setglobal(L, "auxcheck");
    pushLuaChunk(L, "return auxcheck()");
    ASSERT_EQ(suite, LUA_ERRRUN, lua_pcall(L, 0, 0, 0), "luaL_argerror propagates through a Lua caller");
    ASSERT_TRUE(suite, std::string(lua_tostring(L, -1)).find("to 'auxcheck'") != std::string::npos,
                "luaL_argerror resolves the public function name from the caller");
    lua_pop(L, 1);
    lua_pushcclosure(L, raiseAuxiliaryFormattedError, 0);
    lua_setglobal(L, "auxfail");
    pushLuaChunk(L, "return auxfail()");
    ASSERT_EQ(suite, LUA_ERRRUN, lua_pcall(L, 0, 0, 0), "luaL_error raises a formatted protected error");
    const std::string formattedError = lua_tostring(L, -1);
    ASSERT_TRUE(suite, formattedError.find(":1: formatted failure 7") != std::string::npos,
                "luaL_error prefixes its message with the Lua caller source location");
    lua_pop(L, 1);

    lua_pushcclosure(L, requireAuxiliaryValue, 0);
    ASSERT_EQ(suite, LUA_ERRRUN, lua_pcall(L, 0, 0, 0), "luaL_checkany rejects a missing argument");
    lua_pop(L, 1);
    lua_pushcclosure(L, requireAuxiliaryOption, 0);
    lua_pushstring(L, "beta");
    ASSERT_EQ(suite, LUA_OK, lua_pcall(L, 1, 1, 0), "luaL_checkoption accepts a listed option in a C callback");
    ASSERT_EQ(suite, 1.0, lua_tonumber(L, -1), "luaL_checkoption callback returns the matching option index");
    lua_pop(L, 1);
    lua_pushcclosure(L, requireAuxiliaryOption, 0);
    lua_pushstring(L, "gamma");
    ASSERT_EQ(suite, LUA_ERRRUN, lua_pcall(L, 1, 0, 0), "luaL_checkoption rejects an unknown option");
    lua_pop(L, 1);

    ASSERT_EQ(suite, 1, luaL_newmetatable(L, "aux.widget"), "luaL_newmetatable creates a named metatable once");
    lua_pushcclosure(L, returnAuxiliaryMetaValue, 0);
    lua_setfield(L, -2, "__probe");
    const void* metatableIdentity = lua_topointer(L, -1);
    lua_pop(L, 1);
    ASSERT_EQ(suite, 0, luaL_newmetatable(L, "aux.widget"), "luaL_newmetatable returns the existing named metatable");
    ASSERT_TRUE(suite, lua_topointer(L, -1) == metatableIdentity,
                "luaL_newmetatable leaves the registered metatable on the stack");
    lua_pop(L, 1);

    void* payload = lua_newuserdata(L, sizeof(int));
    luaL_getmetatable(L, "aux.widget");
    ASSERT_EQ(suite, 1, lua_setmetatable(L, -2), "luaL_getmetatable retrieves a metatable for userdata");
    ASSERT_TRUE(suite, luaL_checkudata(L, -1, "aux.widget") == payload,
                "luaL_checkudata returns a payload with the registered metatable");
    ASSERT_EQ(suite, 1, luaL_getmetafield(L, -1, "__probe"), "luaL_getmetafield finds a raw metafield");
    ASSERT_TRUE(suite, lua_tocfunction(L, -1) == returnAuxiliaryMetaValue,
                "luaL_getmetafield leaves the matching function on the stack");
    lua_pop(L, 1);
    ASSERT_EQ(suite, 0, luaL_getmetafield(L, -1, "__missing"), "luaL_getmetafield removes missing lookup temporaries");
    ASSERT_EQ(suite, 1, luaL_callmeta(L, -1, "__probe"), "luaL_callmeta invokes a matching metafield");
    ASSERT_EQ(suite, std::string("meta-value"), std::string(lua_tostring(L, -1)), "luaL_callmeta leaves one result");
    lua_pop(L, 2);

    lua_pushcclosure(L, requireAuxiliaryUserdata, 0);
    lua_pushnumber(L, 1);
    ASSERT_EQ(suite, LUA_ERRRUN, lua_pcall(L, 1, 0, 0), "luaL_checkudata rejects mismatched values");
    lua_pop(L, 1);

    lua_close(L);
}

void testPublicAuxiliaryRegistrationAndBuffer(TestSuite& suite) {
    lua_State* L = lua_open();
    static const luaL_Reg capturedFunctions[] = {{"value", returnFirstUpvalue}, {nullptr, nullptr}};
    lua_pushinteger(L, 73);
    luaL_openlib(L, "aux.probe", capturedFunctions, 1);
    ASSERT_EQ(suite, LUA_TTABLE, lua_type(L, 1), "luaL_openlib leaves the module table on the stack");
    lua_getfield(L, 1, "value");
    lua_call(L, 0, 1);
    ASSERT_EQ(suite, 73.0, lua_tonumber(L, -1), "luaL_openlib registers closures with copied upvalues");
    lua_pop(L, 1);

    lua_getglobal(L, "aux");
    lua_getfield(L, -1, "probe");
    ASSERT_TRUE(suite, lua_rawequal(L, 1, -1) != 0, "luaL_openlib publishes a dotted global module name");
    lua_pop(L, 2);
    lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
    lua_getfield(L, -1, "aux.probe");
    ASSERT_TRUE(suite, lua_rawequal(L, 1, -1) != 0, "luaL_openlib records the same module in registry _LOADED");
    lua_settop(L, 0);

    static const luaL_Reg plainFunctions[] = {{"probe", returnAuxiliaryMetaValue}, {nullptr, nullptr}};
    lua_newtable(L);
    luaL_register(L, nullptr, plainFunctions);
    lua_getfield(L, -1, "probe");
    lua_call(L, 0, 1);
    ASSERT_EQ(suite, std::string("meta-value"), std::string(lua_tostring(L, -1)),
              "luaL_register fills an existing table when the library name is null");
    lua_settop(L, 0);

    lua_newtable(L);
    ASSERT_TRUE(suite, luaL_findtable(L, -1, "one.two", 2) == nullptr,
                "luaL_findtable creates every missing dotted component");
    ASSERT_EQ(suite, LUA_TTABLE, lua_type(L, -1), "luaL_findtable leaves the final table on the stack");
    lua_pop(L, 1);
    lua_pushnumber(L, 5);
    lua_setfield(L, -2, "blocked");
    const char* conflict = luaL_findtable(L, -1, "blocked.child", 1);
    ASSERT_EQ(suite, std::string("blocked.child"), std::string(conflict),
              "luaL_findtable identifies the conflicting path component");
    lua_settop(L, 0);

    const char* substituted = luaL_gsub(L, "a-b-a", "a", "xy");
    ASSERT_EQ(suite, std::string("xy-b-xy"), std::string(substituted),
              "luaL_gsub replaces every literal occurrence and pushes its result");
    lua_pop(L, 1);

    luaL_Buffer buffer;
    luaL_buffinit(L, &buffer);
    luaL_addstring(&buffer, "prefix");
    const char embedded[] = {'\0', 'x'};
    luaL_addlstring(&buffer, embedded, sizeof(embedded));
    char* prepared = luaL_prepbuffer(&buffer);
    ASSERT_TRUE(suite, prepared == buffer.buffer, "luaL_prepbuffer flushes and returns the inline buffer");
    prepared[0] = 'A';
    luaL_addsize(&buffer, 1);
    lua_pushstring(L, "tail");
    luaL_addvalue(&buffer);
    luaL_addchar(&buffer, '!');
    luaL_pushresult(&buffer);
    size_t resultLength = 0;
    const char* result = lua_tolstring(L, -1, &resultLength);
    const std::string expected("prefix\0xAtail!", 14);
    ASSERT_EQ(suite, expected, std::string(result, resultLength),
              "luaL buffer APIs preserve flushed chunks, embedded NUL bytes, values, and characters");
    ASSERT_EQ(suite, 1, buffer.lvl, "luaL_pushresult normalizes the buffer stack level");

    lua_close(L);
}

void assertNamedLibraryOpen(TestSuite& suite, lua_State* L, lua_CFunction opener, const char* globalName) {
    const int initialTop = lua_gettop(L);
    lua_pushcclosure(L, opener, 0);
    lua_pushstring(L, globalName);
    lua_call(L, 1, LUA_MULTRET);
    ASSERT_EQ(suite, initialTop + 1, lua_gettop(L), "named luaopen entry pushes exactly one result");
    ASSERT_TRUE(suite, lua_istable(L, -1), "named luaopen entry returns a table");
    lua_getglobal(L, globalName);
    ASSERT_TRUE(suite, lua_rawequal(L, -1, -2), "named luaopen result matches the registered global table");
    lua_settop(L, initialTop);
}

void testPublicStandardLibraryOpeners(TestSuite& suite) {
    lua_State* L = lua_open();
    ASSERT_TRUE(suite, L != nullptr, "lua_open creates a state for public standard-library openers");
    if (L == nullptr) {
        return;
    }

    lua_pushinteger(L, 73);
    lua_pushcclosure(L, callLuaOpenBase, 0);
    lua_pushstring(L, "");
    lua_call(L, 1, LUA_MULTRET);
    ASSERT_EQ(suite, 3, lua_gettop(L), "luaopen_base preserves arguments and pushes two results");
    ASSERT_TRUE(suite, lua_istable(L, 2), "luaopen_base first result is the global table");
    ASSERT_TRUE(suite, lua_istable(L, 3), "luaopen_base second result is the coroutine table");
    lua_getglobal(L, "_G");
    ASSERT_TRUE(suite, lua_rawequal(L, 2, -1), "luaopen_base returns the registered global table first");
    lua_pop(L, 1);
    lua_getglobal(L, LUA_COLIBNAME);
    ASSERT_TRUE(suite, lua_rawequal(L, 3, -1), "luaopen_base returns the registered coroutine table second");
    lua_settop(L, 0);

    assertNamedLibraryOpen(suite, L, callLuaOpenTable, LUA_TABLIBNAME);
    assertNamedLibraryOpen(suite, L, callLuaOpenIO, LUA_IOLIBNAME);
    assertNamedLibraryOpen(suite, L, callLuaOpenOS, LUA_OSLIBNAME);
    assertNamedLibraryOpen(suite, L, callLuaOpenString, LUA_STRLIBNAME);
    assertNamedLibraryOpen(suite, L, callLuaOpenMath, LUA_MATHLIBNAME);
    assertNamedLibraryOpen(suite, L, callLuaOpenDebug, LUA_DBLIBNAME);
    assertNamedLibraryOpen(suite, L, callLuaOpenPackage, LUA_LOADLIBNAME);

    lua_pushinteger(L, 41);
    luaL_openlibs(L);
    ASSERT_EQ(suite, 1, lua_gettop(L), "luaL_openlibs preserves the caller stack");
    ASSERT_EQ(suite, 41.0, lua_tonumber(L, 1), "luaL_openlibs preserves existing stack values");
    for (const char* name : {LUA_COLIBNAME, LUA_TABLIBNAME, LUA_IOLIBNAME, LUA_OSLIBNAME, LUA_STRLIBNAME,
                             LUA_MATHLIBNAME, LUA_DBLIBNAME, LUA_LOADLIBNAME}) {
        lua_getglobal(L, name);
        ASSERT_TRUE(suite, lua_istable(L, -1), "luaL_openlibs registers every official standard-library table");
        lua_pop(L, 1);
    }

    lua_close(L);
}

void testPublicPanicFormatEnvironmentAndCpcall(TestSuite& suite) {
    lua_State* L = lua_open();
    ASSERT_TRUE(suite, L != nullptr, "lua_open creates a state for the final core API batch");
    if (L == nullptr) {
        return;
    }

    ASSERT_TRUE(suite, lua_atpanic(L, firstPublicPanic) == nullptr, "lua_atpanic returns the initial null callback");
    ASSERT_TRUE(suite, lua_atpanic(L, secondPublicPanic) == firstPublicPanic,
                "lua_atpanic replaces and returns the previous callback");
    lua_State* child = lua_newthread(L);
    ASSERT_TRUE(suite, child != nullptr, "lua_newthread creates a state for shared panic and level checks");
    ASSERT_TRUE(suite, lua_atpanic(child, firstPublicPanic) == secondPublicPanic,
                "lua_atpanic is shared by every thread in one runtime");

    auto* sourceState = reinterpret_cast<Lua::LuaState*>(L);
    auto* targetState = reinterpret_cast<Lua::LuaState*>(child);
    sourceState->setHostCallDepth(7);
    targetState->setHostCallDepth(1);
    lua_setlevel(L, child);
    ASSERT_EQ(suite, 7, targetState->getHostCallDepth(),
              "lua_setlevel copies the nested host-call level between threads");
    sourceState->setHostCallDepth(0);
    targetState->setHostCallDepth(0);
    lua_settop(L, 0);

    char pointerText[4 * sizeof(void*) + 8]{};
    std::snprintf(pointerText, sizeof(pointerText), "%p", static_cast<void*>(&gApiErrorToken));
    const std::string expected = std::string("text|Z|17|1.25|") + pointerText + "|%|%q|(null)";
    const char* formatted = lua_pushfstring(L, "%s|%c|%d|%f|%p|%%|%q|%s", "text", 'Z', 17, 1.25,
                                            static_cast<void*>(&gApiErrorToken), static_cast<const char*>(nullptr));
    ASSERT_EQ(suite, expected, std::string(formatted), "lua_pushfstring implements the Lua 5.1 formatting vocabulary");
    ASSERT_TRUE(suite, formatted == lua_tostring(L, -1), "lua_pushfstring returns the pushed string pointer");
    const char* vformatted = pushPublicVFormat(L, "v=%d/%s", 29, "ok");
    ASSERT_EQ(suite, std::string("v=29/ok"), std::string(vformatted),
              "lua_pushvfstring consumes a caller-owned va_list");
    lua_settop(L, 0);

    pushLuaChunk(L, "return environment_value");
    lua_newtable(L);
    lua_pushinteger(L, 73);
    lua_setfield(L, -2, "environment_value");
    ASSERT_EQ(suite, 1, lua_setfenv(L, 1), "lua_setfenv assigns a Lua closure environment");
    ASSERT_EQ(suite, 1, lua_gettop(L), "lua_setfenv consumes the environment table");
    lua_getfenv(L, 1);
    ASSERT_TRUE(suite, lua_istable(L, -1), "lua_getfenv pushes a function environment");
    lua_getfield(L, -1, "environment_value");
    ASSERT_EQ(suite, 73.0, lua_tonumber(L, -1), "lua_getfenv exposes the assigned function environment");
    lua_pop(L, 2);
    ASSERT_EQ(suite, LUA_OK, lua_pcall(L, 0, 1, 0), "a Lua closure executes with its assigned environment");
    ASSERT_EQ(suite, 73.0, lua_tonumber(L, -1), "GETGLOBAL observes the lua_setfenv table");
    lua_settop(L, 0);

    (void)lua_newuserdata(L, sizeof(int));
    lua_newtable(L);
    lua_pushstring(L, "userdata-env");
    lua_setfield(L, -2, "kind");
    ASSERT_EQ(suite, 1, lua_setfenv(L, 1), "lua_setfenv assigns a full-userdata environment");
    lua_getfenv(L, 1);
    lua_getfield(L, -1, "kind");
    ASSERT_EQ(suite, std::string("userdata-env"), std::string(lua_tostring(L, -1)),
              "lua_getfenv returns the full-userdata environment");
    lua_settop(L, 0);

    child = lua_newthread(L);
    lua_newtable(L);
    lua_pushinteger(L, 91);
    lua_setfield(L, -2, "thread_value");
    ASSERT_EQ(suite, 1, lua_setfenv(L, 1), "lua_setfenv assigns a thread global table");
    lua_getfenv(L, 1);
    lua_getfield(L, -1, "thread_value");
    ASSERT_EQ(suite, 91.0, lua_tonumber(L, -1), "lua_getfenv returns a thread global table");
    lua_settop(L, 0);

    lua_pushinteger(L, 5);
    lua_newtable(L);
    ASSERT_EQ(suite, 0, lua_setfenv(L, 1), "lua_setfenv rejects unsupported value kinds");
    ASSERT_EQ(suite, 1, lua_gettop(L), "failed lua_setfenv still consumes the table");
    lua_getfenv(L, 1);
    ASSERT_TRUE(suite, lua_isnil(L, -1), "lua_getfenv pushes nil for unsupported value kinds");
    lua_settop(L, 0);

    lua_pushstring(L, "prefix");
    gPublicCpcallArgument = false;
    ASSERT_EQ(suite, LUA_OK, lua_cpcall(L, capturePublicCpcallArgument, &gApiErrorToken),
              "lua_cpcall protects a successful C callback");
    ASSERT_TRUE(suite, gPublicCpcallArgument, "lua_cpcall passes user data as one light-userdata argument");
    ASSERT_EQ(suite, 1, lua_gettop(L), "successful lua_cpcall preserves the caller prefix and discards results");
    ASSERT_EQ(suite, std::string("prefix"), std::string(lua_tostring(L, 1)),
              "successful lua_cpcall preserves existing values");
    ASSERT_EQ(suite, LUA_ERRRUN, lua_cpcall(L, failPublicCpcall, nullptr),
              "lua_cpcall translates callback failure into a status");
    ASSERT_EQ(suite, 2, lua_gettop(L), "failed lua_cpcall preserves the prefix and pushes one error object");
    ASSERT_TRUE(suite, lua_touserdata(L, -1) == &gApiErrorToken,
                "failed lua_cpcall preserves a non-string Lua error object");

    lua_close(L);
}

void testPublicDebugStackInfoAndLocals(TestSuite& suite) {
    lua_State* L = lua_open();

    lua_Debug functionInfo{};
    lua_pushcclosure(L, inspectPublicDebugCaller, 0);
    const void* functionIdentity = lua_topointer(L, -1);
    ASSERT_EQ(suite, 1, lua_getinfo(L, ">Suf", &functionInfo), "lua_getinfo accepts a function from the stack");
    ASSERT_EQ(suite, std::string("C"), std::string(functionInfo.what),
              "lua_getinfo reports C function source metadata");
    ASSERT_EQ(suite, std::string("=[C]"), std::string(functionInfo.source),
              "lua_getinfo reports the official C source marker");
    ASSERT_EQ(suite, std::string("[C]"), std::string(functionInfo.short_src),
              "lua_getinfo formats the official short C source marker");
    ASSERT_EQ(suite, 0, functionInfo.nups, "lua_getinfo reports C closure upvalues");
    ASSERT_TRUE(suite, lua_topointer(L, -1) == functionIdentity,
                "lua_getinfo >f pops and then returns the queried function");
    lua_pop(L, 1);

    gPublicDebugCurrentFrame = false;
    gPublicDebugCallerFrame = false;
    gPublicDebugCallerFunction = false;
    gPublicDebugCallerLines = false;
    gPublicDebugLocalName.clear();
    gPublicDebugLocalValue = 0;
    lua_pushcclosure(L, inspectPublicDebugCaller, 0);
    lua_setglobal(L, "inspect_debug_caller");
    pushLuaChunk(L, "local value = 7\ninspect_debug_caller()\nreturn value");
    ASSERT_EQ(suite, LUA_OK, lua_pcall(L, 0, 1, 0), "debug stack/local test chunk executes");
    ASSERT_EQ(suite, 19.0, lua_tonumber(L, -1), "lua_setlocal mutates the live caller slot");
    ASSERT_TRUE(suite, gPublicDebugCurrentFrame, "lua_getstack resolves the current C frame");
    ASSERT_TRUE(suite, gPublicDebugCallerFrame, "lua_getstack and lua_getinfo resolve the Lua caller");
    ASSERT_TRUE(suite, gPublicDebugCallerFunction, "lua_getinfo f pushes the active function");
    ASSERT_TRUE(suite, gPublicDebugCallerLines, "lua_getinfo L pushes the active-line table");
    ASSERT_EQ(suite, std::string("value"), gPublicDebugLocalName,
              "lua_getlocal and lua_setlocal return the active local name");
    ASSERT_EQ(suite, 7.0, gPublicDebugLocalValue, "lua_getlocal pushes the active local value");
    lua_pop(L, 1);

    lua_Debug missing{};
    ASSERT_EQ(suite, 1, lua_getstack(L, -1, &missing), "lua_getstack represents a negative level as a lost tail call");
    ASSERT_EQ(suite, 1, lua_getinfo(L, "Slu", &missing), "lost tail-call records remain queryable");
    ASSERT_EQ(suite, std::string("tail"), std::string(missing.what),
              "lost tail-call records use the official tail marker");
    ASSERT_EQ(suite, -1, missing.currentline, "lost tail-call records have no current line");
    ASSERT_EQ(suite, 0, lua_getstack(L, 99, &missing), "lua_getstack rejects missing levels");

    lua_close(L);
}

void testPublicDebugHooks(TestSuite& suite) {
    lua_State* L = lua_open();
    gPublicHookCalls = 0;
    gPublicHookReturns = 0;
    gPublicHookLines = 0;
    gPublicHookCounts = 0;
    gPublicHookLineInfo = false;
    gPublicHookTailInfo = false;

    const int mask = LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE | LUA_MASKCOUNT;
    ASSERT_EQ(suite, 1, lua_sethook(L, capturePublicDebugHook, mask, 2), "lua_sethook installs a C hook");
    ASSERT_TRUE(suite, lua_gethook(L) == capturePublicDebugHook, "lua_gethook returns the installed callback");
    ASSERT_EQ(suite, mask, lua_gethookmask(L), "lua_gethookmask preserves every requested event bit");
    ASSERT_EQ(suite, 2, lua_gethookcount(L), "lua_gethookcount returns the count interval");

    pushLuaChunk(L, "local total = 0\nfor i = 1, 3 do\n  total = total + i\nend\nreturn total");
    ASSERT_EQ(suite, LUA_OK, lua_pcall(L, 0, 1, 0), "hooked Lua chunk executes");
    ASSERT_EQ(suite, 6.0, lua_tonumber(L, -1), "hook callbacks preserve execution state");
    ASSERT_TRUE(suite, gPublicHookCalls > 0, "C hook receives call events");
    ASSERT_TRUE(suite, gPublicHookReturns > 0, "C hook receives return events");
    ASSERT_TRUE(suite, gPublicHookLines > 0 && gPublicHookLineInfo,
                "C hook receives line events with queryable activation records");
    ASSERT_TRUE(suite, gPublicHookCounts > 0, "C hook receives count events");
    lua_pop(L, 1);

    pushLuaChunk(L, "local function tail(n)\n  if n == 0 then return 1 end\n  return tail(n - 1)\nend\nreturn tail(2)");
    ASSERT_EQ(suite, LUA_OK, lua_pcall(L, 0, 1, 0), "tail-hook Lua chunk executes");
    ASSERT_TRUE(suite, gPublicHookTailInfo, "tail-return hook records expose the official tail activation marker");
    lua_pop(L, 1);

    ASSERT_EQ(suite, 1, lua_sethook(L, capturePublicDebugHook, 0, 9), "a zero mask disables the callback");
    ASSERT_TRUE(suite, lua_gethook(L) == nullptr, "zero-mask hook installation clears the callback");
    ASSERT_EQ(suite, 0, lua_gethookmask(L), "zero-mask hook installation clears the mask");
    ASSERT_EQ(suite, 9, lua_gethookcount(L), "disabled hooks preserve the official base count field");
    ASSERT_EQ(suite, 1, lua_sethook(L, nullptr, 0, 0), "lua_sethook clears the hook");
    ASSERT_EQ(suite, 0, lua_gethookcount(L), "clearing the hook resets the requested count");

    lua_close(L);
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
    registry.registerTest(kSuiteName, "native callback cooperative execution poll",
                          testNativeCallbackCooperativeExecutionPoll);
    registry.registerTest(kSuiteName, "C++ API exception contract", testCppApiExceptionContract);
    registry.registerTest(kSuiteName, "C closure upvalue introspection", testCClosureUpvalueIntrospection);
    registry.registerTest(kSuiteName, "Lua closure upvalue introspection", testLuaClosureUpvalueIntrospection);
    registry.registerTest(kSuiteName, "light and full userdata", testLightAndFullUserdata);
    registry.registerTest(kSuiteName, "Lua 5.1 known value API semantics", testLua51KnownValueApiSemantics);
    registry.registerTest(kSuiteName, "previously unprobed public contract symbols",
                          testPreviouslyUnprobedPublicContractSymbols);
    registry.registerTest(kSuiteName, "userdata finalizer through C API", testUserdataFinalizerThroughCApi);
    registry.registerTest(kSuiteName, "lua_close finalizer and coroutine semantics", testCloseFinalizerSemantics);
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
    registry.registerTest(kSuiteName, "allocator-backed string content and hard limit",
                          testAllocatorBackedStringContentAndHardLimit);
    registry.registerTest(kSuiteName, "GC worklist allocator transactions", testGcWorklistAllocatorTransactions);
    registry.registerTest(kSuiteName, "table hash allocator transactions", testTableHashAllocatorTransactions);
    registry.registerTest(kSuiteName, "table realloc hard-limit transaction", testTableReallocHardLimitTransaction);
    registry.registerTest(kSuiteName, "Proto allocator transactions", testProtoAllocatorTransactions);
    registry.registerTest(kSuiteName, "allocator replacement", testAllocatorCanBeReplaced);
    registry.registerTest(kSuiteName, "allocator failure paths", testAllocatorFailurePaths);
    registry.registerTest(kSuiteName, "state stack and CallInfo allocator transactions",
                          testStateStackAndCallInfoAllocatorTransactions);
    registry.registerTest(kSuiteName, "__call argument allocator transactions",
                          testMetacallArgumentAllocatorTransactions);
    registry.registerTest(kSuiteName, "concat allocator transactions", testConcatAllocatorTransactions);
    registry.registerTest(kSuiteName, "table.concat allocator transactions", testTableConcatAllocatorTransactions);
    registry.registerTest(kSuiteName, "table.sort allocator transactions", testTableSortAllocatorTransactions);
    registry.registerTest(kSuiteName, "fragmented reader allocator transactions",
                          testFragmentedReaderAllocatorTransactions);
    registry.registerTest(kSuiteName, "loadbuffer allocator failures", testLoadBufferAllocatorFailures);
    registry.registerTest(kSuiteName, "full-stack loader memory errors", testLoadersPublishMemoryErrorFromFullStack);
    registry.registerTest(kSuiteName, "load dump and auxiliary loaders", testLoadDumpAndAuxiliaryLoaders);
    registry.registerTest(kSuiteName, "protected status API exception boundaries",
                          testProtectedStatusApiExceptionBoundaries);
    registry.registerTest(kSuiteName, "registry references", testRegistryReferences);
    registry.registerTest(kSuiteName, "public table field raw and traversal APIs", testPublicTableTraversalApi);
    registry.registerTest(kSuiteName, "public comparison and concat APIs", testPublicComparisonAndConcatApi);
    registry.registerTest(kSuiteName, "public type thread and GC APIs", testPublicTypeThreadAndGcApi);
    registry.registerTest(kSuiteName, "public auxiliary checks and metatables", testPublicAuxiliaryChecksAndMetatables);
    registry.registerTest(kSuiteName, "public auxiliary registration and buffer",
                          testPublicAuxiliaryRegistrationAndBuffer);
    registry.registerTest(kSuiteName, "public standard library openers", testPublicStandardLibraryOpeners);
    registry.registerTest(kSuiteName, "public panic format environment cpcall and setlevel",
                          testPublicPanicFormatEnvironmentAndCpcall);
    registry.registerTest(kSuiteName, "public debug stack info and locals", testPublicDebugStackInfoAndLocals);
    registry.registerTest(kSuiteName, "public debug hooks", testPublicDebugHooks);
}
