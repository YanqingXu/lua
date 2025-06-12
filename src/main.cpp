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
#include "lib/base_lib.hpp"
#include "examples/gc_demo_functions.cpp"
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

// All test functions are now in tests/test_main.cpp

int main(int argc, char** argv) {
    try {
        if (argc > 1) {
            std::string arg = argv[1];
            
            if (arg == "--test" || arg == "-t") {
                 // Run all tests
                 std::cout << "Running All Tests...\n\n";
                 Lua::Tests::runAllTests();
                 std::cout << "\nAll Tests completed!\n";
            } else {
                // Run file
                std::string filename = arg;
                std::string source = readFile(filename);
                
                State state;
                registerBaseLib(&state);
                state.doString(source);
            }
        } else {
            // // Show usage
            // std::cout << "Usage:\n";
            // std::cout << "  " << argv[0] << " <filename>        - Run Lua file\n";
            // std::cout << "  " << argv[0] << " --test|-t        - Run all tests\n";
            // std::cout << "  " << argv[0] << " --gc-demo|-g     - Run GC integration demo\n";
            // std::cout << "  " << argv[0] << " --string-pool|-s - Run String Pool demo\n";
            // std::cout << "  " << argv[0] << "                   - Show this help\n";

            // Run all tests
            std::cout << "Running All Tests...\n\n";
            Lua::Tests::runAllTests();
            std::cout << "\nAll Tests completed!\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
