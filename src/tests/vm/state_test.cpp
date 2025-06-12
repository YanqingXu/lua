#include "state_test.hpp"

namespace Lua {
namespace Tests {

void StateTest::runAllTests() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Running State Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    testState();
    testExecute();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "State Tests Completed" << std::endl;
    std::cout << "========================================" << std::endl;
}

void StateTest::testState() {
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
    
    // Test ipairs and pairs functions
    Value ipairsFn = state.getGlobal("ipairs");
    Value pairsFn = state.getGlobal("pairs");
    
    if (ipairsFn.isFunction()) {
        std::cout << "ipairs function registered successfully" << std::endl;
    } else {
        std::cout << "ERROR: ipairs function not found" << std::endl;
    }
    
    if (pairsFn.isFunction()) {
        std::cout << "pairs function registered successfully" << std::endl;
    } else {
        std::cout << "ERROR: pairs function not found" << std::endl;
    }
}

void StateTest::testExecute() {
    std::cout << "\nExecute Test:" << std::endl;

    State state;
    registerBaseLib(&state);

    // Execute simple Lua code
    state.doString("print('Hello from Lua!')");
}

} // namespace Tests
} // namespace Lua