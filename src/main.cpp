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
   std::cout << "[DEBUG] Main: Starting Lua interpreter" << std::endl;
   std::cout << "[DEBUG] Main: argc=" << argc << std::endl;

   try {
       if (argc > 1) {
           // Run file
           std::string filename = argv[1];
           std::cout << "[DEBUG] Main: Reading file: " << filename << std::endl;
           std::string source = readFile(filename);
           std::cout << "[DEBUG] Main: File content length: " << source.length() << std::endl;

           std::cout << "[DEBUG] Main: Creating State" << std::endl;
           State state;

           std::cout << "[DEBUG] Main: Initializing standard libraries" << std::endl;
           // Initialize all standard libraries using simplified framework
           StandardLibrary::initializeAll(&state);

           std::cout << "[DEBUG] Main: Executing Lua code" << std::endl;
           state.doString(source);
           std::cout << "[DEBUG] Main: Execution completed" << std::endl;
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
