#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "common/types.hpp"
#include "vm/lua_state.hpp"
#include "vm/global_state.hpp"
#include "vm/global_state.hpp"
#include "vm/value.hpp"
#include "vm/table.hpp"
#include "vm/function.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "parser/visitor.hpp"
#include "compiler/symbol_table.hpp"
#include "common/defines.hpp"
#include "lib/core/lib_manager.hpp"

using namespace Lua;

// Forward declaration for the REPL function from repl.cpp
void run_repl();

// Read file content
std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw LuaException("Could not open file: " + path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main(int argc, char** argv) {
   try {
       if (argc > 1) {
            // Run file
            std::string filename = argv[1];
            std::string source = readFile(filename);

            // Create Lua state using official Lua 5.1 architecture
            std::cerr << "[Lua5.1] Using official Lua 5.1 architecture (LuaState + GlobalState)" << std::endl;

            // Create GlobalState and LuaState following official Lua 5.1 pattern
            GlobalState* globalState = new GlobalState();
            LuaState* luaState = new LuaState(globalState);

            if (!luaState) {
                std::cerr << "Error: Failed to create LuaState" << std::endl;
                delete globalState;
                return 1;
            }

            // Initialize all standard libraries
            StandardLibrary::initializeAll(luaState);

            // Execute code
            bool success = luaState->doString(source);

            // Show architecture information
            std::cerr << "[Lua5.1] Architecture: Official Lua 5.1 (LuaState + GlobalState + VMExecutor)" << std::endl;

            // Cleanup
            delete luaState;
            delete globalState;

            if (!success) {
                return 1;
            }
       } else {
           run_repl();
       }
   } catch (const std::exception& e) {
       std::cerr << "Error: " << e.what() << std::endl;
       return 1;
   }

   return 0;
}
