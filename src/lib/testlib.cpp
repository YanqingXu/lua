#include "lib/testlib.hpp"

#include "lauxlib.h"
#include "lualib.h"

#include "compiler/opcode.hpp"
#include "common/number_conversion.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/upvalue.hpp"
#include "core/userdata.hpp"
#include "lib/lib_registry.hpp"
#include "lib/lib_manager.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm.hpp"
#include "vm/vm_internal.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <format>
#include <limits>
#include <memory>
#include <sstream>
#include <unordered_map>

namespace Lua {

namespace {

struct Command {
    Str op;
    Vec<Str> args;
};

struct RemoteStateAllocatorContext {
    lua_Alloc upstream = nullptr;
    void* upstreamUserData = nullptr;
    GarbageCollector* parentGC = nullptr;
    usize liveBytes = 0;
};

std::unordered_map<lua_State*, std::unique_ptr<RemoteStateAllocatorContext>> gRemoteStateAllocators;

void* testDefaultAllocator(void*, void* pointer, size_t, size_t newSize) {
    if (newSize == 0) {
        std::free(pointer);
        return nullptr;
    }
    return std::realloc(pointer, newSize);
}

void* remoteStateAllocator(void* userData, void* pointer, size_t oldSize, size_t newSize) {
    auto* context = static_cast<RemoteStateAllocatorContext*>(userData);
    if (context == nullptr || context->upstream == nullptr) {
        return nullptr;
    }

    if (newSize != 0 && newSize > oldSize && context->parentGC != nullptr) {
        const usize limit = context->parentGC->getMemoryLimitBytes();
        if (limit != std::numeric_limits<usize>::max()) {
            usize remoteBytes = context->liveBytes;
            for (const auto& [state, other] : gRemoteStateAllocators) {
                (void)state;
                if (other.get() != context && other->parentGC == context->parentGC) {
                    remoteBytes = other->liveBytes > std::numeric_limits<usize>::max() - remoteBytes
                                      ? std::numeric_limits<usize>::max()
                                      : remoteBytes + other->liveBytes;
                }
            }

            const usize parentBytes = context->parentGC->getTotalMemory();
            const usize growth = static_cast<usize>(newSize - oldSize);
            if (parentBytes > limit || remoteBytes > limit - parentBytes ||
                growth > limit - parentBytes - remoteBytes) {
                return nullptr;
            }
        }
    }

    void* result = context->upstream(context->upstreamUserData, pointer, oldSize, newSize);
    if (result == nullptr && newSize != 0) {
        return nullptr;
    }

    if (oldSize <= context->liveBytes) {
        context->liveBytes -= oldSize;
    } else {
        context->liveBytes = 0;
    }
    if (newSize > std::numeric_limits<usize>::max() - context->liveBytes) {
        context->liveBytes = std::numeric_limits<usize>::max();
    } else {
        context->liveBytes += newSize;
    }
    return result;
}

Vec<Str> splitWords(StrView text) {
    Vec<Str> words;
    usize pos = 0;
    const auto isDelimiter = [](char ch) {
        return std::isspace(static_cast<unsigned char>(ch)) != 0 || ch == ',' || ch == ';';
    };
    while (pos < text.size()) {
        while (pos < text.size() && isDelimiter(text[pos])) {
            ++pos;
        }
        usize start = pos;
        while (pos < text.size() && !isDelimiter(text[pos])) {
            ++pos;
        }
        if (start < pos) {
            words.emplace_back(text.substr(start, pos - start));
        }
    }
    return words;
}

usize commandArity(const Str& op) {
    if (op == "pushnil" || op == "gettop" || op == "next") {
        return 0;
    }
    if (op == "call" || op == "rawcall" || op == "lessthan" || op == "equal") {
        return 2;
    }
    if (op == "pushnum" || op == "pushbool" || op == "pushstring" || op == "settop" || op == "pushvalue" ||
        op == "remove" || op == "insert" || op == "replace" || op == "pop" || op == "tobool" || op == "tostring" ||
        op == "tonumber" || op == "objsize" || op == "tocfunction" || op == "concat" || op == "gettable" ||
        op == "settable" || op == "loadstring" || op == "loadfile" || op == "pushcclosure" || op == "newuserdata" ||
        op == "return" || op.rfind("is", 0) == 0) {
        return 1;
    }
    return 0;
}

Vec<Command> parseCommands(StrView program) {
    Vec<Command> commands;
    Vec<Str> words = splitWords(program);
    usize cursor = 0;
    while (cursor < words.size()) {
        Command command;
        command.op = words[cursor++];
        const usize arity = commandArity(command.op);
        for (usize i = 0; i < arity && cursor < words.size(); ++i) {
            command.args.push_back(words[cursor++]);
        }
        commands.push_back(std::move(command));
    }
    return commands;
}

i32 readNumber(LuaState* L, const Str& text) {
    if (text == ".") {
        const i32 value = static_cast<i32>(L->toNumber(-1));
        (void)L->pop();
        return value;
    }
    return static_cast<i32>(std::strtol(text.c_str(), nullptr, 10));
}

i32 absStackIndex(LuaState* L, i32 idx) {
    if (idx > 0) {
        return idx;
    }
    return L->getTop() + idx + 1;
}

Function* currentClosure(LuaState* L) {
    if (L->getCurrentCI() == 0) {
        return nullptr;
    }
    const usize functionIndex = L->getCurrentCallInfo().func;
    if (functionIndex >= L->getStack().size() || !L->getStack().at(functionIndex).isFunction()) {
        return nullptr;
    }
    return L->getStack().at(functionIndex).asFunction();
}

Table* currentEnvironment(LuaState* L) {
    Function* closure = currentClosure(L);
    return closure != nullptr && closure->getEnv() != nullptr ? closure->getEnv() : L->getGlobalTable();
}

Table* pseudoTable(LuaState* L, const Str& index) {
    if (index == "R") {
        return L->getGlobalState().getRegistry();
    }
    if (index == "E") {
        return currentEnvironment(L);
    }
    if (index == "G") {
        return L->getGlobalTable();
    }
    return nullptr;
}

i32 pseudoUpvalueNumber(const Str& index) {
    if (index.size() < 2 || index.front() != 'U') {
        return -1;
    }
    return static_cast<i32>(std::strtol(index.c_str() + 1, nullptr, 10));
}

void pushIndexedValue(LuaState* L, const Str& index) {
    if (index == "G" || index == "U0") {
        L->pushTable(L->getGlobalTable());
        return;
    }
    if (index == "E") {
        L->pushTable(currentEnvironment(L));
        return;
    }

    const i32 upvalueNumber = pseudoUpvalueNumber(index);
    if (upvalueNumber > 0) {
        Function* closure = currentClosure(L);
        Upvalue* upvalue = closure != nullptr ? closure->getUpvalue(static_cast<usize>(upvalueNumber - 1)) : nullptr;
        if (upvalue != nullptr) {
            L->pushValue(upvalue->getValue(L->getStack()));
        } else {
            L->pushNil();
        }
        return;
    }

    L->pushValue(readNumber(L, index));
}

void replaceIndexedValue(LuaState* L, const Str& index) {
    Value value = L->pop();
    if (index == "G" || index == "U0") {
        if (!value.isTable()) {
            L->error("testC: global replacement expects a table");
        }
        L->setGlobalTable(value.asTable());
        return;
    }
    if (index == "E") {
        if (!value.isTable()) {
            L->error("testC: environment replacement expects a table");
        }
        Function* closure = currentClosure(L);
        if (closure == nullptr) {
            L->error("testC: no current closure environment");
        }
        closure->setEnv(value.asTable());
        return;
    }

    const i32 upvalueNumber = pseudoUpvalueNumber(index);
    if (upvalueNumber > 0) {
        Function* closure = currentClosure(L);
        Upvalue* upvalue = closure != nullptr ? closure->getUpvalue(static_cast<usize>(upvalueNumber - 1)) : nullptr;
        if (upvalue != nullptr) {
            upvalue->setValue(L->getStack(), value);
        }
        return;
    }

    L->pushValue(value);
    L->replace(readNumber(L, index));
}

void removeStackIndex(LuaState* L, i32 idx) {
    const i32 top = L->getTop();
    const i32 pos = absStackIndex(L, idx);
    if (pos < 1 || pos > top) {
        L->error("testC: invalid remove index");
    }

    Vec<Value> values;
    values.reserve(static_cast<usize>(top - 1));
    for (i32 i = 1; i <= top; ++i) {
        if (i != pos) {
            values.push_back(L->at(i));
        }
    }
    L->setTop(0);
    for (const Value& value : values) {
        L->pushValue(value);
    }
}

void pushTypeResult(LuaState* L, const Str& op, i32 idx) {
    if (op == "isnumber") {
        L->pushNumber(L->isNumber(idx) ? 1.0 : 0.0);
    } else if (op == "isstring") {
        const i32 t = L->type(idx);
        L->pushNumber((t == 3 || t == 4) ? 1.0 : 0.0);
    } else if (op == "isfunction") {
        L->pushNumber(L->isFunction(idx) ? 1.0 : 0.0);
    } else if (op == "iscfunction") {
        try {
            const Value& value = L->at(idx);
            L->pushNumber(value.isFunction() && value.asFunction()->isCFunction() ? 1.0 : 0.0);
        } catch (...) {
            L->pushNumber(0.0);
        }
    } else if (op == "istable") {
        L->pushNumber(L->isTable(idx) ? 1.0 : 0.0);
    } else if (op == "isuserdata") {
        L->pushNumber(L->isUserdata(idx) ? 1.0 : 0.0);
    } else if (op == "isnil") {
        L->pushNumber(L->type(idx) == 0 ? 1.0 : 0.0);
    } else if (op == "isnull") {
        L->pushNumber(L->type(idx) == -1 ? 1.0 : 0.0);
    }
}

void concatTop(LuaState* L, i32 count) {
    if (count <= 0) {
        L->pushString(L->getGlobalState().getStringPool().intern(""));
        return;
    }
    if (count > L->getTop()) {
        L->error("testC: concat underflow");
    }

    const i32 first = L->getTop() - count + 1;
    Str out;
    for (i32 i = first; i <= L->getTop(); ++i) {
        const char* text = L->toString(i);
        if (text == nullptr) {
            L->error("testC: concat expects string or number");
        }
        const Value& value = L->at(i);
        if (value.isString()) {
            out.append(value.asString()->c_str(), value.asString()->getLength());
        } else {
            out.append(text);
        }
    }
    L->setTop(first - 1);
    L->pushString(L->getGlobalState().getStringPool().intern(out));
}

i32 t_listcode(LuaState* L) {
    if (L->getTop() < 1 || !L->at(1).isFunction() || L->at(1).asFunction()->isCFunction()) {
        L->error("bad argument #1 to 'listcode' (Lua function expected)");
    }

    Proto* proto = L->at(1).asFunction()->getProto();
    Table* table = L->getGlobalState().getGC().create<Table>();
    auto& pool = L->getGlobalState().getStringPool();

    table->set(Value(pool.intern("maxstack")), Value(static_cast<LuaNumber>(proto->getMaxStackSize())));
    table->set(Value(pool.intern("numparams")), Value(static_cast<LuaNumber>(proto->getNumParams())));

    for (usize pc = 0; pc < proto->getInstructionCount(); ++pc) {
        Instruction inst = proto->getInstruction(pc);
        Str line = std::format("{} - {} {}", pc + 1, getOpName(GET_OPCODE(inst)), pc + 1);
        table->setArray(static_cast<i32>(pc + 1), Value(pool.intern(line)));
    }

    L->pushTable(table);
    return 1;
}

i32 t_testC(LuaState* L) {
    LuaState* caller = L;
    Str programText;
    if (caller->getTop() >= 2 && caller->at(1).isNumber() && caller->at(2).isString()) {
        const auto address = static_cast<std::uintptr_t>(caller->toNumber(1));
        L = reinterpret_cast<LuaState*>(address);
        programText.assign(caller->at(2).asString()->c_str(), caller->at(2).asString()->getLength());
    } else if (caller->getTop() >= 1 && caller->at(1).isString()) {
        programText.assign(caller->at(1).asString()->c_str(), caller->at(1).asString()->getLength());
    } else {
        L->error("bad argument #1 to 'testC' (string expected)");
    }

    RuntimeServices services(L->getGlobalState());
    Vec<Command> commands = parseCommands(programText);

    for (const Command& command : commands) {
        const Str& op = command.op;
        if (op == "pushnum") {
            L->pushNumber(static_cast<LuaNumber>(std::strtod(command.args.at(0).c_str(), nullptr)));
        } else if (op == "pushbool") {
            L->pushBoolean(readNumber(L, command.args.at(0)) != 0);
        } else if (op == "pushnil") {
            L->pushNil();
        } else if (op == "pushstring") {
            L->pushString(L->getGlobalState().getStringPool().intern(command.args.at(0)));
        } else if (op == "gettop") {
            L->pushNumber(static_cast<LuaNumber>(L->getTop()));
        } else if (op == "settop") {
            L->setTop(readNumber(L, command.args.at(0)));
        } else if (op == "pushvalue") {
            pushIndexedValue(L, command.args.at(0));
        } else if (op == "remove") {
            removeStackIndex(L, readNumber(L, command.args.at(0)));
        } else if (op == "insert") {
            L->insert(readNumber(L, command.args.at(0)));
        } else if (op == "replace") {
            replaceIndexedValue(L, command.args.at(0));
        } else if (op == "pop") {
            i32 count = readNumber(L, command.args.at(0));
            while (count-- > 0) {
                (void)L->pop();
            }
        } else if (op == "tobool") {
            L->pushNumber(L->toBoolean(readNumber(L, command.args.at(0))) ? 1.0 : 0.0);
        } else if (op == "tostring") {
            i32 idx = readNumber(L, command.args.at(0));
            const char* text = L->toString(idx);
            if (text == nullptr) {
                L->pushNil();
            } else {
                const Value& value = L->at(idx);
                if (value.isString()) {
                    L->pushString(value.asString());
                } else {
                    L->pushString(L->getGlobalState().getStringPool().intern(text));
                }
            }
        } else if (op == "tonumber") {
            L->pushNumber(L->toNumber(readNumber(L, command.args.at(0))));
        } else if (op == "objsize") {
            const i32 idx = readNumber(L, command.args.at(0));
            const i32 type = L->type(idx);
            usize size = 0;
            if (type == 3 || type == 4) {
                (void)L->toString(idx);
                if (L->type(idx) == 4) {
                    size = L->at(idx).asString()->getLength();
                }
            } else if (type == 5) {
                size = L->at(idx).asTable()->length();
            } else if (type == 7) {
                size = L->at(idx).asUserdata()->getDataSize();
            }
            L->pushNumber(static_cast<LuaNumber>(size));
        } else if (op == "tocfunction") {
            const i32 idx = readNumber(L, command.args.at(0));
            if (L->type(idx) == 6 && L->at(idx).asFunction()->isCFunction()) {
                Function* source = L->at(idx).asFunction();
                Function* copy = source->isApiCFunction()
                                     ? L->getGlobalState().getGC().create<Function>(source->getApiCFunction())
                                     : L->getGlobalState().getGC().create<Function>(source->getCFunction());
                copy->setEnv(source->getEnv());
                L->pushFunction(copy);
            } else {
                L->pushNil();
            }
        } else if (op == "concat") {
            concatTop(L, readNumber(L, command.args.at(0)));
        } else if (op == "call" || op == "rawcall") {
            i32 nargs = readNumber(L, command.args.at(0));
            i32 nresults = readNumber(L, command.args.at(1));
            if (op == "rawcall") {
                VM::call(services, L, nargs, nresults);
            } else {
                (void)L->pcall(nargs, nresults, 0);
            }
        } else if (op == "loadstring") {
            const i32 idx = readNumber(L, command.args.at(0));
            if (!L->at(idx).isString()) {
                L->error("testC: loadstring expects a string");
            }
            GCString* source = L->at(idx).asString();
            (void)luaL_loadbuffer(reinterpret_cast<lua_State*>(L), source->c_str(), source->getLength(),
                                  source->c_str());
        } else if (op == "loadfile") {
            const i32 idx = readNumber(L, command.args.at(0));
            if (!L->at(idx).isString()) {
                L->error("testC: loadfile expects a string");
            }
            (void)luaL_loadfile(reinterpret_cast<lua_State*>(L), L->at(idx).asString()->c_str());
        } else if (op == "pushcclosure") {
            const i32 upvalueCount = readNumber(L, command.args.at(0));
            if (upvalueCount < 0 || upvalueCount > L->getTop()) {
                L->error("testC: invalid C closure upvalue count");
            }
            Vec<Value> values;
            values.reserve(static_cast<usize>(upvalueCount));
            for (i32 i = upvalueCount; i > 0; --i) {
                values.push_back(L->at(-i));
            }
            Function* closure = L->getGlobalState().getGC().create<Function>(t_testC);
            closure->setEnv(currentEnvironment(L));
            for (const Value& value : values) {
                closure->addUpvalue(L->getGlobalState().getGC().create<Upvalue>(value));
            }
            L->setTop(L->getTop() - upvalueCount);
            L->pushFunction(closure);
        } else if (op == "newuserdata") {
            const i32 size = readNumber(L, command.args.at(0));
            if (size < 0) {
                L->error("testC: invalid userdata size");
            }
            Userdata* userdata = L->getGlobalState().getGC().create<Userdata>(static_cast<usize>(size));
            userdata->setEnvironment(currentEnvironment(L));
            L->pushUserdata(userdata);
        } else if (op == "lessthan") {
            const i32 lhs = readNumber(L, command.args.at(0));
            const i32 rhs = readNumber(L, command.args.at(1));
            const bool result =
                L->type(lhs) != -1 && L->type(rhs) != -1 && VM::detail::lessThan(L, L->at(lhs), L->at(rhs));
            L->pushBoolean(result);
        } else if (op == "equal") {
            const i32 lhs = readNumber(L, command.args.at(0));
            const i32 rhs = readNumber(L, command.args.at(1));
            const bool result =
                L->type(lhs) != -1 && L->type(rhs) != -1 && VM::detail::equal(L, L->at(lhs), L->at(rhs));
            L->pushBoolean(result);
        } else if (op == "gettable") {
            const Str& arg = command.args.at(0);
            Table* specialTable = pseudoTable(L, arg);
            Value tableValue = specialTable != nullptr ? Value(specialTable) : L->at(readNumber(L, arg));
            Value key = L->pop();
            Value result;
            VM::detail::gettable(L, tableValue, key, result);
            L->pushValue(result);
        } else if (op == "settable") {
            const Str& arg = command.args.at(0);
            Table* specialTable = pseudoTable(L, arg);
            Value tableValue = specialTable != nullptr ? Value(specialTable) : L->at(readNumber(L, arg));
            Value value = L->pop();
            Value key = L->pop();
            VM::detail::settable(L, tableValue, key, value);
        } else if (op == "next") {
            if (L->getTop() < 2 || !L->at(-2).isTable()) {
                L->error("testC: next expects a table and key");
            }
            Table* table = L->at(-2).asTable();
            Value key = L->pop();
            Value nextKey;
            Value nextValue;
            if (table->next(key, nextKey, nextValue)) {
                L->pushValue(nextKey);
                L->pushValue(nextValue);
            }
        } else if (op.rfind("is", 0) == 0) {
            pushTypeResult(L, op, readNumber(L, command.args.at(0)));
        } else if (op == "return") {
            const Str& countArg = command.args.empty() ? Str("0") : command.args.front();
            if (countArg == ".") {
                return readNumber(L, countArg);
            }
            return readNumber(L, countArg);
        } else {
            L->error((Str("testC: unsupported command '") + op + "'").c_str());
        }
    }

    return 0;
}

i32 t_d2s(LuaState* L) {
    LuaNumber number = L->toNumber(1);
    u64 bits = 0;
    std::memcpy(&bits, &number, sizeof(bits));
    Str out(sizeof(bits), '\0');
    std::memcpy(out.data(), &bits, sizeof(bits));
    L->pushString(L->getGlobalState().getStringPool().intern(out.data(), out.size()));
    return 1;
}

i32 t_s2d(LuaState* L) {
    if (L->getTop() < 1 || !L->at(1).isString() || L->at(1).asString()->getLength() != sizeof(LuaNumber)) {
        L->error("bad argument #1 to 's2d' (8-byte string expected)");
    }
    LuaNumber number = 0.0;
    std::memcpy(&number, L->at(1).asString()->c_str(), sizeof(number));
    L->pushNumber(number);
    return 1;
}

i32 t_checkmemory(LuaState*) {
    return 0;
}

LuaNumber checkTestLibNumber(LuaState* L, i32 index, const char* functionName) {
    if (L->getTop() < index) {
        L->error(std::format("bad argument #{} to '{}' (number expected)", index, functionName).c_str());
    }

    const Value value = L->at(index);
    LuaNumber number = 0.0;
    if (value.isNumber()) {
        return value.asNumber();
    }
    if (value.isString() && luaStringToNumber(value.asString()->view(), number)) {
        return number;
    }
    L->error(std::format("bad argument #{} to '{}' (number expected)", index, functionName).c_str());
}

LuaNumber usizeExclusiveUpperBound() noexcept {
    return std::ldexp(LuaNumber{1}, std::numeric_limits<usize>::digits);
}

i32 t_totalmem(LuaState* L) {
    GarbageCollector& gc = L->getGlobalState().getGC();
    if (L->getTop() >= 1) {
        const LuaNumber requested = checkTestLibNumber(L, 1, "totalmem");
        if (std::isnan(requested)) {
            L->error("bad argument #1 to 'totalmem' (finite number expected)");
        }

        usize limit = 0;
        if (requested > 0) {
            limit = !std::isfinite(requested) || requested >= usizeExclusiveUpperBound()
                        ? std::numeric_limits<usize>::max()
                        : static_cast<usize>(requested);
        }
        gc.setMemoryLimitBytes(limit);
        return 0;
    }

    LuaNumber bytes = static_cast<LuaNumber>(gc.getTotalMemory());
    L->pushNumber(bytes);
    L->pushNumber(static_cast<LuaNumber>(gc.getObjectCount()));
    L->pushNumber(static_cast<LuaNumber>(gc.getMemoryLimitBytes()));
    return 3;
}

i32 t_newuserdata(LuaState* L) {
    const LuaNumber requested = checkTestLibNumber(L, 1, "newuserdata");
    if (!std::isfinite(requested)) {
        L->error("bad argument #1 to 'newuserdata' (finite size expected)");
    }
    if (requested < 0) {
        L->error("bad argument #1 to 'newuserdata' (size must be non-negative)");
    }
    if (requested >= usizeExclusiveUpperBound()) {
        L->error("bad argument #1 to 'newuserdata' (size out of range)");
    }
    Userdata* userdata = L->getGlobalState().getGC().create<Userdata>(static_cast<usize>(requested));
    userdata->setEnvironment(currentEnvironment(L));
    L->pushUserdata(userdata);
    return 1;
}

i32 t_upvalue(LuaState* L) {
    if (L->getTop() < 2 || !L->at(1).isFunction()) {
        L->error("bad arguments to 'upvalue'");
    }
    const i32 index = static_cast<i32>(L->toNumber(2));
    lua_State* state = reinterpret_cast<lua_State*>(L);
    if (L->getTop() < 3) {
        const char* name = lua_getupvalue(state, 1, index);
        if (name == nullptr) {
            return 0;
        }
        lua_pushstring(state, name);
        return 2;
    }

    const char* name = lua_setupvalue(state, 1, index);
    lua_pushstring(state, name != nullptr ? name : "");
    return 1;
}

i32 t_ref(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("bad argument #1 to 'ref' (value expected)");
    }
    L->pushValue(1);
    const int reference = luaL_ref(reinterpret_cast<lua_State*>(L), LUA_REGISTRYINDEX);
    L->pushNumber(static_cast<LuaNumber>(reference));
    return 1;
}

i32 t_getref(LuaState* L) {
    const int reference = static_cast<int>(L->toNumber(1));
    luaL_getref(reinterpret_cast<lua_State*>(L), reference);
    return 1;
}

i32 t_unref(LuaState* L) {
    const int reference = static_cast<int>(L->toNumber(1));
    luaL_unref(reinterpret_cast<lua_State*>(L), LUA_REGISTRYINDEX, reference);
    return 0;
}

i32 t_udataval(LuaState* L) {
    std::uintptr_t address = 0;
    if (L->at(1).isUserdata()) {
        address = reinterpret_cast<std::uintptr_t>(L->at(1).asUserdata()->getData());
    } else if (L->at(1).isLightUserdata()) {
        address = reinterpret_cast<std::uintptr_t>(L->at(1).asLightUserdata());
    } else {
        L->error("bad argument #1 to 'udataval' (userdata expected)");
    }
    L->pushNumber(static_cast<LuaNumber>(address));
    return 1;
}

i32 t_pushuserdata(LuaState* L) {
    const auto address = static_cast<std::uintptr_t>(L->toNumber(1));
    L->pushValue(Value(reinterpret_cast<void*>(address)));
    return 1;
}

i32 t_newstate(LuaState* L) {
    void* upstreamUserData = nullptr;
    lua_Alloc upstream = lua_getallocf(reinterpret_cast<lua_State*>(L), &upstreamUserData);
    auto context = std::make_unique<RemoteStateAllocatorContext>();
    context->upstream = upstream != nullptr ? upstream : testDefaultAllocator;
    context->upstreamUserData = upstreamUserData;
    context->parentGC = &L->getGlobalState().getGC();

    lua_State* remote = lua_newstate(remoteStateAllocator, context.get());
    if (remote == nullptr) {
        L->pushNil();
    } else {
        try {
            gRemoteStateAllocators.emplace(remote, std::move(context));
        } catch (...) {
            lua_close(remote);
            throw;
        }
        L->pushNumber(static_cast<LuaNumber>(reinterpret_cast<std::uintptr_t>(remote)));
    }
    return 1;
}

lua_State* remoteStateArgument(LuaState* L) {
    if (L->getTop() < 1 || !L->at(1).isNumber()) {
        L->error("remote state handle expected");
    }
    const auto address = static_cast<std::uintptr_t>(L->toNumber(1));
    return reinterpret_cast<lua_State*>(address);
}

i32 t_closestate(LuaState* L) {
    lua_State* remote = remoteStateArgument(L);
    auto allocation = gRemoteStateAllocators.find(remote);
    lua_close(remote);
    if (allocation != gRemoteStateAllocators.end()) {
        gRemoteStateAllocators.erase(allocation);
    }
    return 0;
}

i32 t_doremote(LuaState* L) {
    lua_State* remote = remoteStateArgument(L);
    if (L->getTop() < 2 || !L->at(2).isString()) {
        L->error("remote source string expected");
    }
    GCString* source = L->at(2).asString();
    lua_settop(remote, 0);
    int status = luaL_loadbuffer(remote, source->c_str(), source->getLength(), source->c_str());
    if (status == LUA_OK) {
        status = lua_pcall(remote, 0, LUA_MULTRET, 0);
    }
    if (status != LUA_OK) {
        L->pushNil();
        L->pushNumber(static_cast<LuaNumber>(status));
        const char* message = lua_tostring(remote, -1);
        if (message != nullptr) {
            L->pushString(L->getGlobalState().getStringPool().intern(message));
        } else {
            L->pushNil();
        }
        lua_settop(remote, 0);
        return 3;
    }

    const int resultCount = lua_gettop(remote);
    for (int i = 1; i <= resultCount; ++i) {
        const char* result = lua_tostring(remote, i);
        if (result != nullptr) {
            L->pushString(L->getGlobalState().getStringPool().intern(result));
        } else {
            L->pushNil();
        }
    }
    lua_settop(remote, 0);
    return resultCount;
}

i32 t_loadlib(LuaState* L) {
    lua_State* remote = remoteStateArgument(L);
    luaL_openlibs(remote);
    constexpr char openers[] = "function baselibopen() return _G end "
                               "function dblibopen() return debug end "
                               "function iolibopen() return io end "
                               "function mathlibopen() return math end "
                               "function strlibopen() return string end "
                               "function tablibopen() return table end "
                               "function packageopen() return package end";
    int status = luaL_loadbuffer(remote, openers, sizeof(openers) - 1, "=testC-loadlib");
    if (status == LUA_OK) {
        status = lua_pcall(remote, 0, 0, 0);
    }
    if (status != LUA_OK) {
        const char* message = lua_tostring(remote, -1);
        L->error(message != nullptr ? message : "could not initialize remote libraries");
    }
    return 0;
}

i32 t_doonnewstack(LuaState* L) {
    if (L->getTop() < 1 || !L->at(1).isString()) {
        L->error("bad argument #1 to 'doonnewstack' (string expected)");
    }
    Str source(L->at(1).asString()->c_str(), L->at(1).asString()->getLength());
    lua_State* parent = reinterpret_cast<lua_State*>(L);
    lua_State* thread = lua_newthread(parent);
    if (thread == nullptr) {
        L->pushNumber(static_cast<LuaNumber>(LUA_ERRMEM));
        return 1;
    }

    int status = luaL_loadbuffer(thread, source.data(), source.size(), source.c_str());
    if (status == LUA_OK) {
        status = lua_pcall(thread, 0, 0, 0);
    }
    L->pushNumber(static_cast<LuaNumber>(status));
    return 1;
}

i32 t_gsub(LuaState* L) {
    if (L->getTop() < 3 || !L->at(1).isString() || !L->at(2).isString() || !L->at(3).isString()) {
        L->error("bad arguments to 'gsub' (three strings expected)");
    }
    const Str source(L->at(1).asString()->c_str(), L->at(1).asString()->getLength());
    const Str pattern(L->at(2).asString()->c_str(), L->at(2).asString()->getLength());
    const Str replacement(L->at(3).asString()->c_str(), L->at(3).asString()->getLength());
    if (pattern.empty()) {
        L->pushString(L->getGlobalState().getStringPool().intern(source));
        return 1;
    }

    Str result;
    usize cursor = 0;
    while (cursor < source.size()) {
        const usize match = source.find(pattern, cursor);
        if (match == Str::npos) {
            result.append(source, cursor, Str::npos);
            break;
        }
        result.append(source, cursor, match - cursor);
        result.append(replacement);
        cursor = match + pattern.size();
    }
    L->pushString(L->getGlobalState().getStringPool().intern(result));
    return 1;
}

} // namespace

void TestLibModule::registerFunctions(LuaState* L) {
    if (!L) {
        return;
    }

    Table* table = FunctionRegistrar::createLibTable(L, "T");
    FunctionRegistrar(L)
        .addGlobal("listcode", t_listcode)
        .addGlobal("testC", t_testC)
        .addGlobal("d2s", t_d2s)
        .addGlobal("s2d", t_s2d)
        .addGlobal("checkmemory", t_checkmemory)
        .addGlobal("totalmem", t_totalmem)
        .addGlobal("newuserdata", t_newuserdata)
        .addGlobal("upvalue", t_upvalue)
        .addGlobal("ref", t_ref)
        .addGlobal("getref", t_getref)
        .addGlobal("unref", t_unref)
        .addGlobal("udataval", t_udataval)
        .addGlobal("pushuserdata", t_pushuserdata)
        .addGlobal("newstate", t_newstate)
        .addGlobal("closestate", t_closestate)
        .addGlobal("doremote", t_doremote)
        .addGlobal("loadlib", t_loadlib)
        .addGlobal("doonnewstack", t_doonnewstack)
        .addGlobal("gsub", t_gsub)
        .commitToTable(table);
}

void TestLibModule::initialize(LuaState*) {}

void openTestLib(LuaState* L) {
    TestLibModule module;
    StandardLibrary::openModule(L, module);
}

} // namespace Lua
