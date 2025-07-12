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

           State state;

           // Initialize all standard libraries using simplified framework
           StandardLibrary::initializeAll(&state);

           state.doString(source);
       } else {
           RUN_MAIN_TEST("MainTestSuite", Lua::Tests::MainTestSuite::runAllTests);

           // Interactive mode or show usage  
           /*std::cout << "Lua Interpreter (Simplified Standard Library)" << std::endl;  
           std::cout << "Usage: " << argv[0] << " <script.lua>" << std::endl;  
           std::cout << std::endl;  
           std::cout << "This version includes:" << std::endl;  
           std::cout << "- Core VM components" << std::endl;  
           std::cout << "- Lexer and Parser" << std::endl;  
           std::cout << "- Compiler" << std::endl;  
           std::cout << "- Simplified BaseLib (core functions)" << std::endl;  
           std::cout << "- Simplified StringLib (string functions)" << std::endl;  
           std::cout << "- Simplified MathLib (math functions)" << std::endl;  
           std::cout << "- Garbage collection" << std::endl;*/  
       }  
   } catch (const std::exception& e) {  
       std::cerr << "Error: " << e.what() << std::endl;  
       return 1;  
   }  

   return 0;  
}
