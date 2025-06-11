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
#include "tests/gc_integration_test.hpp"
#include "tests/string_pool_demo_test.hpp"

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

// Forward declarations
void runStringPoolDemo();
void runGCIntegrationDemo();
        


void runStringPoolDemo() {
    try {
        Lua::Tests::runStringPoolDemoTests();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        throw;
    }
}

void runGCIntegrationDemo() {
    try {
        Lua::Tests::runGCIntegrationTests();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        throw;
    }
}

int main(int argc, char** argv) {
    try {
        if (argc > 1) {
            std::string arg = argv[1];
            
            if (arg == "--gc-demo" || arg == "-g") {
                // Run GC integration demo
                std::cout << "Running GC Integration Demo...\n\n";
                runGCIntegrationDemo();
                std::cout << "\nGC Integration Demo completed!\n";
            } else if (arg == "--string-pool" || arg == "-s") {
                // Run String Pool demo
                std::cout << "Running String Pool Demo...\n\n";
                runStringPoolDemo();
                std::cout << "\nString Pool Demo completed!\n";
            } else if (arg == "--test" || arg == "-t") {
                 // Run tests
                 std::cout << "Test functionality not available in this build.\n";
                 std::cout << "Use --gc-demo to run GC integration demo instead.\n";
            } else {
                // Run file
                std::string filename = arg;
                std::string source = readFile(filename);
                
                State state;
                registerBaseLib(&state);
                state.doString(source);
            }
        } else {
            // Show usage
            std::cout << "Usage:\n";
            std::cout << "  " << argv[0] << " <filename>        - Run Lua file\n";
            std::cout << "  " << argv[0] << " --test|-t        - Run all tests\n";
            std::cout << "  " << argv[0] << " --gc-demo|-g     - Run GC integration demo\n";
            std::cout << "  " << argv[0] << " --string-pool|-s - Run String Pool demo\n";
            std::cout << "  " << argv[0] << "                   - Show this help\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
