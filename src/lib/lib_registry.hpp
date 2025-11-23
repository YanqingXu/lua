#pragma once

#include "lib/lib_module.hpp"

namespace Lua {

class Table;

class LibRegistry {
public:
    static void registerGlobalFunction(LuaState* L, const char* name, LibCFunction func);

    static void registerGlobalFunctions(LuaState* L, const LibFunctionEntry* entries);

    static void registerTableFunction(LuaState* L, Table* table, const char* name, LibCFunction func);

    static void registerTableFunctions(LuaState* L, Table* table, const LibFunctionEntry* entries);

    static Table* createLibTable(LuaState* L, const char* libName);
};

} // namespace Lua
