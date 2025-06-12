#include "value_test.hpp"

namespace Lua {
namespace Tests {

void ValueTest::runAllTests() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Running Value Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    testValues();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Value Tests Completed" << std::endl;
    std::cout << "========================================" << std::endl;
}

void ValueTest::testValues() {
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
    auto table = make_gc_table();
    table->set(Value(1), Value("one"));
    table->set(Value(2), Value("two"));
    table->set(Value("name"), Value("lua"));

    Value tableValue(table);
    std::cout << "table: " << tableValue.toString() << std::endl;
    std::cout << "table[1]: " << table->get(Value(1)).toString() << std::endl;
    std::cout << "table[2]: " << table->get(Value(2)).toString() << std::endl;
    std::cout << "table['name']: " << table->get(Value("name")).toString() << std::endl;
}

} // namespace Tests
} // namespace Lua