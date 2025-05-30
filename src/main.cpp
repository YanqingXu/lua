#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "types.hpp"
#include "vm/state.hpp"
#include "vm/value.hpp"
#include "vm/table.hpp"
#include "vm/function.hpp"
#include "lexer/lexer.hpp"
#include "common/defines.hpp"
#include "lib/base_lib.hpp"

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

// Test lexer
void testLexer(const std::string& source) {
    std::cout << "Lexer Test:" << std::endl;
    std::cout << "Source: " << source << std::endl;
    
    Lexer lexer(source);
    Token token;
    
    do {
        token = lexer.nextToken();
        std::cout << "Token: " << static_cast<int>(token.type) << " Lexeme: " << token.lexeme 
            << " Line: " << token.line << " Column: " << token.column << std::endl;
    } while (token.type != TokenType::Eof && token.type != TokenType::Error);
}

// Test Lua values
void testValues() {
    std::cout << "\nValue Test:" << std::endl;
    
    Value nil;
    Value boolean(true);
    Value number(42.5);
    Value string("Hello, Lua!");
    
    std::cout << "nil: " << nil.toString() << std::endl;
    std::cout << "boolean: " << boolean.toString() << std::endl;
    std::cout << "number: " << number.toString() << std::endl;
    std::cout << "string: " << string.toString() << std::endl;
    
    // Test table
    auto table = make_ptr<Table>();
    table->set(Value(1), Value("one"));
    table->set(Value(2), Value("two"));
    table->set(Value("name"), Value("lua"));
    
    Value tableValue(table);
    std::cout << "table: " << tableValue.toString() << std::endl;
    std::cout << "table[1]: " << table->get(Value(1)).toString() << std::endl;
    std::cout << "table[2]: " << table->get(Value(2)).toString() << std::endl;
    std::cout << "table['name']: " << table->get(Value("name")).toString() << std::endl;
}

// Test state
void testState() {
    std::cout << "\nState Test:" << std::endl;
    
    State state;
    
    // Register base library
    registerBaseLib(&state);
    
    // Test global variables
    state.setGlobal("x", Value(10));
    state.setGlobal("y", Value(20));
    state.setGlobal("z", Value("Lua"));
    
    std::cout << "x: " << state.getGlobal("x").toString() << std::endl;
    std::cout << "y: " << state.getGlobal("y").toString() << std::endl;
    std::cout << "z: " << state.getGlobal("z").toString() << std::endl;
    
    // Test stack operations
    state.push(Value(1));
    state.push(Value(2));
    state.push(Value(3));
    
    std::cout << "Stack size: " << state.getTop() << std::endl;
    std::cout << "Stack[1]: " << state.get(1).toString() << std::endl;
    std::cout << "Stack[2]: " << state.get(2).toString() << std::endl;
    std::cout << "Stack[3]: " << state.get(3).toString() << std::endl;
    
    // Call native function
    Value printFn = state.getGlobal("print");
    if (printFn.isFunction()) {
        Vec<Value> args;
        args.push_back(Value("Hello from native function!"));
        state.call(printFn, args);
    }
}

// Test executing lua code
void testExecute() {
    std::cout << "\nExecute Test:" << std::endl;
    
    State state;
    registerBaseLib(&state);
    
    // Execute simple Lua code
    state.doString("print('Hello from Lua!')");
}

int main(int argc, char** argv) {
    try {
        std::cout << LUA_VERSION << " " << LUA_COPYRIGHT << std::endl;
        
        testLexer("local x = 10 + 20");
    
        testValues();
        
        testState();
        
        testExecute();
        
        return 0;
    } catch (const LuaException& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Standard error: " << e.what() << std::endl;
        return 1;
    }
}
