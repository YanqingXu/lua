#include "vm/lua_state.hpp"
#include "core/string_pool.hpp"
#include <iostream>

using namespace Lua;

int main() {
    LuaState* L = LuaState::newState();
    
    std::cout << "Initial top: " << L->getTop() << std::endl;
    
    // Push some values
    L->pushNumber(1.0);
    L->pushNumber(2.0);
    L->pushNumber(3.0);
    
    std::cout << "After pushing 3 numbers, top: " << L->getTop() << std::endl;
    
    // Test setTop(0)
    L->setTop(0);
    std::cout << "After setTop(0), top: " << L->getTop() << std::endl;
    
    // Push new values
    L->pushBoolean(true);
    L->pushNumber(42.0);
    
    std::cout << "After pushing boolean and number, top: " << L->getTop() << std::endl;
    
    // Test accessing values
    try {
        std::cout << "at(-2) is boolean: " << L->at(-2).isBoolean() << std::endl;
        std::cout << "at(-2) value: " << L->at(-2).asBoolean() << std::endl;
        std::cout << "at(-1) is number: " << L->at(-1).isNumber() << std::endl;
        std::cout << "at(-1) value: " << L->at(-1).asNumber() << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
    
    delete L;
    return 0;
}

