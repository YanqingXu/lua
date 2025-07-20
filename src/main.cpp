#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "common/types.hpp"
#include "vm/state.hpp"
#include "vm/value.hpp"
#include "vm/table.hpp"
#include "vm/function.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "parser/visitor.hpp"
#include "compiler/symbol_table.hpp"
#include "common/defines.hpp"
#include "lib/core/lib_manager.hpp"
#include "test_framework/core/test_macros.hpp"
#include "tests/test_main.hpp"

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
            std::string arg = argv[1];
            if (arg == "-test") {
                RUN_MAIN_TEST("MainTestSuite", Lua::Tests::MainTestSuite::runAllTests);
                
            } else {
                // Run file
                std::string filename = arg;
                std::string source = readFile(filename);

                State state;

                // Initialize all standard libraries using simplified framework
                StandardLibrary::initializeAll(&state);

                state.doString(source);
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
