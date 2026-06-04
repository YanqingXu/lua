#include "lib/testlib.hpp"

#include "compiler/opcode.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "lib/lib_registry.hpp"
#include "lib/lib_manager.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm.hpp"
#include "vm/vm_internal.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <format>
#include <sstream>

namespace Lua {

namespace {

struct Command {
    Str op;
    Vec<Str> args;
};

Vec<Str> splitWords(StrView text) {
    Vec<Str> words;
    usize pos = 0;
    while (pos < text.size()) {
        while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos])) != 0) {
            ++pos;
        }
        usize start = pos;
        while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos])) == 0) {
            ++pos;
        }
        if (start < pos) {
            words.emplace_back(text.substr(start, pos - start));
        }
    }
    return words;
}

Vec<Command> parseCommands(StrView program) {
    Vec<Command> commands;
    usize start = 0;
    while (start <= program.size()) {
        usize end = program.find_first_of(";,", start);
        if (end == StrView::npos) {
            end = program.size();
        }
        Vec<Str> words = splitWords(program.substr(start, end - start));
        if (!words.empty()) {
            Command command;
            command.op = words.front();
            for (usize i = 1; i < words.size(); ++i) {
                command.args.push_back(words[i]);
            }
            commands.push_back(std::move(command));
        }
        if (end == program.size()) {
            break;
        }
        start = end + 1;
    }
    return commands;
}

i32 toInt(const Str& text) {
    if (text == ".") {
        return -999999;
    }
    return static_cast<i32>(std::strtol(text.c_str(), nullptr, 10));
}

i32 absStackIndex(LuaState* L, i32 idx) {
    if (idx > 0) {
        return idx;
    }
    return L->getTop() + idx + 1;
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
        L->pushNumber(L->isNil(idx) ? 1.0 : 0.0);
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
    if (L->getTop() < 1 || !L->at(1).isString()) {
        L->error("bad argument #1 to 'testC' (string expected)");
    }

    GCString* program = L->at(1).asString();
    RuntimeServices services(L->getGlobalState());
    Vec<Command> commands = parseCommands(program->view());

    for (const Command& command : commands) {
        const Str& op = command.op;
        if (op == "pushnum") {
            L->pushNumber(static_cast<LuaNumber>(std::strtod(command.args.at(0).c_str(), nullptr)));
        } else if (op == "pushbool") {
            L->pushBoolean(toInt(command.args.at(0)) != 0);
        } else if (op == "pushnil") {
            L->pushNil();
        } else if (op == "pushstring") {
            L->pushString(L->getGlobalState().getStringPool().intern(command.args.at(0)));
        } else if (op == "gettop") {
            L->pushNumber(static_cast<LuaNumber>(L->getTop()));
        } else if (op == "settop") {
            L->setTop(toInt(command.args.at(0)));
        } else if (op == "pushvalue") {
            const Str& arg = command.args.at(0);
            if (arg == "G" || arg == "E") {
                L->pushTable(L->getGlobalTable());
            } else {
                L->pushValue(toInt(arg));
            }
        } else if (op == "remove") {
            removeStackIndex(L, toInt(command.args.at(0)));
        } else if (op == "insert") {
            L->insert(toInt(command.args.at(0)));
        } else if (op == "replace") {
            const Str& arg = command.args.at(0);
            if (arg == "G" || arg == "E") {
                Value value = L->pop();
                if (!value.isTable()) {
                    L->error("testC: environment replacement expects table");
                }
                L->setGlobalTable(value.asTable());
            } else {
                L->replace(toInt(arg));
            }
        } else if (op == "pop") {
            i32 count = toInt(command.args.at(0));
            while (count-- > 0) {
                (void)L->pop();
            }
        } else if (op == "tobool") {
            L->pushNumber(L->toBoolean(toInt(command.args.at(0))) ? 1.0 : 0.0);
        } else if (op == "tostring") {
            i32 idx = toInt(command.args.at(0));
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
        } else if (op == "concat") {
            concatTop(L, toInt(command.args.at(0)));
        } else if (op == "call" || op == "rawcall") {
            i32 nargs = toInt(command.args.at(0));
            i32 nresults = toInt(command.args.at(1));
            VM::call(services, L, nargs, nresults);
        } else if (op == "lessthan") {
            bool result = VM::detail::lessThan(L, L->at(toInt(command.args.at(0))), L->at(toInt(command.args.at(1))));
            L->pushBoolean(result);
        } else if (op == "equal") {
            bool result = VM::detail::equal(L, L->at(toInt(command.args.at(0))), L->at(toInt(command.args.at(1))));
            L->pushBoolean(result);
        } else if (op == "gettable") {
            const Str& arg = command.args.at(0);
            Value tableValue = (arg == "G" || arg == "E") ? Value(L->getGlobalTable()) : L->at(toInt(arg));
            if (arg == "G" || arg == "E") {
                Value key = L->pop();
                Value result;
                VM::detail::gettable(L, tableValue, key, result);
                L->pushValue(result);
            } else {
                Value key = L->pop();
                Value result;
                VM::detail::gettable(L, tableValue, key, result);
                L->pushValue(result);
            }
        } else if (op == "settable") {
            const Str& arg = command.args.at(0);
            Value tableValue = (arg == "G" || arg == "E") ? Value(L->getGlobalTable()) : L->at(toInt(arg));
            Value value = L->pop();
            Value key = L->pop();
            VM::detail::settable(L, tableValue, key, value);
        } else if (op.rfind("is", 0) == 0) {
            pushTypeResult(L, op, toInt(command.args.at(0)));
        } else if (op == "return") {
            const Str& countArg = command.args.empty() ? Str("0") : command.args.front();
            if (countArg == ".") {
                return L->getTop();
            }
            return toInt(countArg);
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

i32 t_totalmem(LuaState* L) {
    LuaNumber bytes = static_cast<LuaNumber>(L->getGlobalState().getGC().getTotalMemory());
    L->pushNumber(bytes);
    L->pushNumber(static_cast<LuaNumber>(L->getGlobalState().getGC().getObjectCount()));
    L->pushNumber(bytes);
    return 3;
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
        .commitToTable(table);
}

void TestLibModule::initialize(LuaState*) {
}

void openTestLib(LuaState* L) {
    TestLibModule module;
    StandardLibrary::openModule(L, module);
}

} // namespace Lua
