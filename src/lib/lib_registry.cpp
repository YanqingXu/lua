#include "lib/lib_registry.hpp"

#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/value.hpp"
#include "vm/global_state.hpp"
#include "vm/lua_state.hpp"

namespace Lua {

namespace {
Function* createClosure(LuaState* L, LibCFunction func) {
    if (!L || !func) {
        return nullptr;
    }
    Function* closure = new Function(func);
    L->getGlobalState().getGC().registerObject(closure);
    return closure;
}
}

void LibRegistry::registerGlobalFunction(LuaState* L, const char* name, LibCFunction func) {
    if (!L || !name || !func) {
        return;
    }

    Function* closure = createClosure(L, func);
    if (!closure) {
        return;
    }

    L->setGlobal(name, Value(closure));
}

void LibRegistry::registerGlobalFunctions(LuaState* L, const LibFunctionEntry* entries) {
    if (!L || !entries) {
        return;
    }

    for (const LibFunctionEntry* entry = entries; entry && entry->name; ++entry) {
        registerGlobalFunction(L, entry->name, entry->func);
    }
}

void LibRegistry::registerTableFunction(LuaState* L, Table* table, const char* name, LibCFunction func) {
    if (!L || !table || !name || !func) {
        return;
    }

    Function* closure = createClosure(L, func);
    if (!closure) {
        return;
    }

    GCString* key = L->getGlobalState().getStringPool().intern(name);
    table->set(Value(key), Value(closure));
}

void LibRegistry::registerTableFunctions(LuaState* L, Table* table, const LibFunctionEntry* entries) {
    if (!L || !table || !entries) {
        return;
    }

    for (const LibFunctionEntry* entry = entries; entry && entry->name; ++entry) {
        registerTableFunction(L, table, entry->name, entry->func);
    }
}

Table* LibRegistry::createLibTable(LuaState* L, const char* libName) {
    if (!L || !libName) {
        return nullptr;
    }

    Table* table = new Table();
    L->getGlobalState().getGC().registerObject(table);
    L->setGlobal(libName, Value(table));
    return table;
}

} // namespace Lua
