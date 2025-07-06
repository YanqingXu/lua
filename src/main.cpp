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
#include "lib/base/base_lib.hpp"
#include "lib/core/lib_module.hpp"

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
            // Register BaseLib using new modular interface
            auto baseLib = createBaseLib();
            Lib::LibContext context;
            baseLib->initialize(&state, context);
            state.doString(source);
        } else {
            // Interactive mode or show usage
            std::cout << "Lua Interpreter (Minimal Version)" << std::endl;
            std::cout << "Usage: " << argv[0] << " <script.lua>" << std::endl;
            std::cout << std::endl;
            std::cout << "This minimal version includes:" << std::endl;
            std::cout << "- Core VM components" << std::endl;
            std::cout << "- Lexer and Parser" << std::endl;
            std::cout << "- Compiler" << std::endl;
            std::cout << "- BaseLib (core functions)" << std::endl;
            std::cout << "- Basic garbage collection" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
