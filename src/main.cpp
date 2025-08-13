#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "common/types.hpp"
#include "vm/state.hpp"
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

            // Determine which architecture to use (Phase 1 refactoring)
            State* state = nullptr;
            GlobalState* globalState = nullptr;

            if (filename.find("globalstate") != std::string::npos) {
                // Use explicit GlobalState with State (Phase 1 refactoring)
                std::cerr << "[Phase1] Using explicit GlobalState architecture" << std::endl;
                globalState = new GlobalState();
                state = new State(globalState);
            } else {
                // Use State with internal Lua 5.1 architecture (Phase 1 migration)
                std::cerr << "[Phase1] Using State with internal Lua 5.1 architecture" << std::endl;
                state = new State();
            }

            if (!state) {
                std::cerr << "Error: Failed to create State" << std::endl;
                return 1;
            }

            // Initialize all standard libraries using simplified framework
            StandardLibrary::initializeAll(state);

            // Execute code
            bool success = state->doString(source);

            // Show architecture information
            if (state->isUsingGlobalState()) {
                if (state->getLuaState()) {
                    std::cerr << "[Phase1] Architecture: State with Lua 5.1 (LuaState + GlobalState)" << std::endl;
                } else {
                    std::cerr << "[Phase1] Architecture: State with GlobalState integration" << std::endl;
                }
            } else {
                std::cerr << "[Phase1] Architecture: Traditional State (legacy fallback)" << std::endl;
            }

            // Cleanup
            delete state;
            if (globalState) {
                delete globalState;
            }

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
