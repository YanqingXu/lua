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
#include "tests/test_main.hpp"
#include "test_framework/core/test_macros.hpp"

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
            registerBaseLib(&state);
            state.doString(source);
        } else {
            // Run all tests by default
            RUN_MAIN_TEST("Lua Interpreter Tests", Lua::Tests::runAllTests);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
