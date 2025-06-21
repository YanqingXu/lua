#include "state_global_test.hpp"

namespace Lua {
namespace Tests {

void StateGlobalTest::testBasicGlobalOperations() {
    State state;
    
    // Test setting and getting basic types
    state.setGlobal("number", Value(42));
    state.setGlobal("string", Value("hello"));
    state.setGlobal("boolean", Value(true));
    state.setGlobal("nil_value", Value(nullptr));
    
    // Verify retrieval
    assert(state.getGlobal("number").asNumber() == 42);
    assert(state.getGlobal("string").toString() == "hello");
    assert(state.getGlobal("boolean").asBoolean() == true);
    assert(state.getGlobal("nil_value").isNil());
    
    // Test type checking
    assert(state.getGlobal("number").isNumber());
    assert(state.getGlobal("string").isString());
    assert(state.getGlobal("boolean").isBoolean());
    assert(state.getGlobal("nil_value").isNil());
}

void StateGlobalTest::testGlobalTypes() {
    State state;
    
    // Test all supported types as globals
    
    // Numbers
    state.setGlobal("int", Value(123));
    state.setGlobal("float", Value(3.14159));
    state.setGlobal("negative", Value(-456));
    state.setGlobal("zero", Value(0));
    
    assert(state.getGlobal("int").asNumber() == 123);
    assert(state.getGlobal("float").asNumber() == 3.14159);
    assert(state.getGlobal("negative").asNumber() == -456);
    assert(state.getGlobal("zero").asNumber() == 0);
    
    // Strings
    state.setGlobal("normal_string", Value("hello world"));
    state.setGlobal("empty_string", Value(""));
    state.setGlobal("special_chars", Value("!@#$%^&*()_+{}|:<>?[]\\"));
    state.setGlobal("unicode", Value("测试中文"));
    
    assert(state.getGlobal("normal_string").toString() == "hello world");
    assert(state.getGlobal("empty_string").toString() == "");
    assert(state.getGlobal("special_chars").toString() == "!@#$%^&*()_+{}|:<>?[]\\");
    assert(state.getGlobal("unicode").toString() == "测试中文");
    
    // Booleans
    state.setGlobal("true_val", Value(true));
    state.setGlobal("false_val", Value(false));
    
    assert(state.getGlobal("true_val").asBoolean() == true);
    assert(state.getGlobal("false_val").asBoolean() == false);
    
    // Nil
    state.setGlobal("nil_explicit", Value(nullptr));
    assert(state.getGlobal("nil_explicit").isNil());
}

void StateGlobalTest::testGlobalOverwrite() {
    State state;
    
    // Set initial value
    state.setGlobal("variable", Value(100));
    assert(state.getGlobal("variable").asNumber() == 100);
    
    // Overwrite with same type
    state.setGlobal("variable", Value(200));
    assert(state.getGlobal("variable").asNumber() == 200);
    
    // Overwrite with different type
    state.setGlobal("variable", Value("string"));
    assert(state.getGlobal("variable").toString() == "string");
    assert(state.getGlobal("variable").isString());
    assert(!state.getGlobal("variable").isNumber());
    
    // Overwrite with boolean
    state.setGlobal("variable", Value(true));
    assert(state.getGlobal("variable").asBoolean() == true);
    assert(state.getGlobal("variable").isBoolean());
    
    // Overwrite with nil
    state.setGlobal("variable", Value(nullptr));
    assert(state.getGlobal("variable").isNil());
}

void StateGlobalTest::testNonExistentGlobals() {
    State state;
    
    // Test getting non-existent globals
    assert(state.getGlobal("nonexistent").isNil());
    assert(state.getGlobal("another_nonexistent").isNil());
    assert(state.getGlobal("").isNil());
    
    // Test common Lua global names that don't exist yet
    assert(state.getGlobal("print").isNil());
    assert(state.getGlobal("_G").isNil());
    assert(state.getGlobal("table").isNil());
    assert(state.getGlobal("string").isNil());
    assert(state.getGlobal("math").isNil());
    assert(state.getGlobal("io").isNil());
    assert(state.getGlobal("os").isNil());
    
    // Set one global and verify others are still nil
    state.setGlobal("exists", Value(42));
    assert(state.getGlobal("exists").asNumber() == 42);
    assert(state.getGlobal("still_nonexistent").isNil());
}

void StateGlobalTest::testGlobalPersistence() {
    State state;
    
    // Set multiple globals
    state.setGlobal("persistent1", Value(123));
    state.setGlobal("persistent2", Value("test"));
    state.setGlobal("persistent3", Value(true));
    
    // Perform other operations that might affect globals
    state.push(Value(1));
    state.push(Value(2));
    state.push(Value(3));
    
    Value popped = state.pop();
    assert(popped.asNumber() == 3);
    
    state.setTop(0);  // Clear stack
    
    // Globals should still exist
    assert(state.getGlobal("persistent1").asNumber() == 123);
    assert(state.getGlobal("persistent2").toString() == "test");
    assert(state.getGlobal("persistent3").asBoolean() == true);
    
    // Add more stack operations
    for (int i = 0; i < 100; ++i) {
        state.push(Value(i));
    }
    
    state.clearStack();
    
    // Globals should still persist
    assert(state.getGlobal("persistent1").asNumber() == 123);
    assert(state.getGlobal("persistent2").toString() == "test");
    assert(state.getGlobal("persistent3").asBoolean() == true);
}

void StateGlobalTest::testGlobalMemoryManagement() {
    State state;
    
    // Get initial memory size
    usize initialSize = state.getSize() + state.getAdditionalSize();
    
    // Add globals and check memory growth
    state.setGlobal("mem_test1", Value(42));
    usize size1 = state.getSize() + state.getAdditionalSize();
    assert(size1 >= initialSize);
    
    state.setGlobal("mem_test2", Value("string value"));
    usize size2 = state.getSize() + state.getAdditionalSize();
    assert(size2 >= size1);
    
    state.setGlobal("mem_test3", Value(true));
    usize size3 = state.getSize() + state.getAdditionalSize();
    assert(size3 >= size2);
    
    // Overwrite with nil (should not necessarily reduce size immediately)
    state.setGlobal("mem_test1", Value(nullptr));
    state.setGlobal("mem_test2", Value(nullptr));
    state.setGlobal("mem_test3", Value(nullptr));
    
    // Values should be nil
    assert(state.getGlobal("mem_test1").isNil());
    assert(state.getGlobal("mem_test2").isNil());
    assert(state.getGlobal("mem_test3").isNil());
    
    // Test GC marking
    state.markReferences(nullptr);
    
    // Should still be able to set new globals
    state.setGlobal("after_gc", Value(999));
    assert(state.getGlobal("after_gc").asNumber() == 999);
}

void StateGlobalTest::testSpecialGlobalNames() {
    State state;
    
    // Test empty string as global name
    state.setGlobal("", Value(42));
    assert(state.getGlobal("").asNumber() == 42);
    
    // Test single character names
    state.setGlobal("a", Value(1));
    state.setGlobal("z", Value(26));
    state.setGlobal("_", Value(100));
    
    assert(state.getGlobal("a").asNumber() == 1);
    assert(state.getGlobal("z").asNumber() == 26);
    assert(state.getGlobal("_").asNumber() == 100);
    
    // Test names with special characters
    state.setGlobal("var_with_underscore", Value("underscore"));
    state.setGlobal("var123", Value("numbers"));
    state.setGlobal("_private", Value("private"));
    
    assert(state.getGlobal("var_with_underscore").toString() == "underscore");
    assert(state.getGlobal("var123").toString() == "numbers");
    assert(state.getGlobal("_private").toString() == "private");
    
    // Test very long names
    std::string longName(1000, 'x');
    state.setGlobal(longName, Value("long"));
    assert(state.getGlobal(longName).toString() == "long");
    
    // Test Unicode names
    state.setGlobal("变量", Value("chinese"));
    state.setGlobal("переменная", Value("russian"));
    state.setGlobal("変数", Value("japanese"));
    
    assert(state.getGlobal("变量").toString() == "chinese");
    assert(state.getGlobal("переменная").toString() == "russian");
    assert(state.getGlobal("变数").toString() == "japanese");
}

void StateGlobalTest::testManyGlobals() {
    State state;
    
    const int numGlobals = 1000;
    
    // Set many globals
    for (int i = 0; i < numGlobals; ++i) {
        std::string name = "global_" + std::to_string(i);
        state.setGlobal(name, Value(i));
    }
    
    // Verify all globals
    for (int i = 0; i < numGlobals; ++i) {
        std::string name = "global_" + std::to_string(i);
        Value val = state.getGlobal(name);
        assert(val.isNumber());
        assert(val.asNumber() == i);
    }
    
    // Modify some globals
    for (int i = 0; i < numGlobals; i += 2) {
        std::string name = "global_" + std::to_string(i);
        state.setGlobal(name, Value("modified_" + std::to_string(i)));
    }
    
    // Verify modifications
    for (int i = 0; i < numGlobals; ++i) {
        std::string name = "global_" + std::to_string(i);
        Value val = state.getGlobal(name);
        
        if (i % 2 == 0) {
            // Even indices were modified to strings
            assert(val.isString());
            assert(val.toString() == "modified_" + std::to_string(i));
        } else {
            // Odd indices should still be numbers
            assert(val.isNumber());
            assert(val.asNumber() == i);
        }
    }
}

void StateGlobalTest::testGlobalInteractionWithStack() {
    State state;
    
    // Set some globals
    state.setGlobal("stack_test", Value(42));
    state.setGlobal("another", Value("string"));
    
    // Push values to stack
    state.push(Value(1));
    state.push(Value(2));
    state.push(Value(3));
    
    // Globals should be unaffected by stack operations
    assert(state.getGlobal("stack_test").asNumber() == 42);
    assert(state.getGlobal("another").toString() == "string");
    
    // Pop values
    state.pop();
    state.pop();
    
    // Globals should still be there
    assert(state.getGlobal("stack_test").asNumber() == 42);
    assert(state.getGlobal("another").toString() == "string");
    
    // Clear stack
    state.clearStack();
    
    // Globals should persist
    assert(state.getGlobal("stack_test").asNumber() == 42);
    assert(state.getGlobal("another").toString() == "string");
    
    // Set globals while stack has values
    state.push(Value(100));
    state.push(Value(200));
    
    state.setGlobal("with_stack", Value("set with stack"));
    
    // Both stack and globals should coexist
    assert(state.getTop() == 2);
    assert(state.get(1).asNumber() == 100);
    assert(state.get(2).asNumber() == 200);
    assert(state.getGlobal("with_stack").toString() == "set with stack");
    assert(state.getGlobal("stack_test").asNumber() == 42);
}

void StateGlobalTest::testGlobalCaseSensitivity() {
    State state;
    
    // Test case sensitivity
    state.setGlobal("Variable", Value(1));
    state.setGlobal("variable", Value(2));
    state.setGlobal("VARIABLE", Value(3));
    state.setGlobal("VaRiAbLe", Value(4));
    
    // All should be different
    assert(state.getGlobal("Variable").asNumber() == 1);
    assert(state.getGlobal("variable").asNumber() == 2);
    assert(state.getGlobal("VARIABLE").asNumber() == 3);
    assert(state.getGlobal("VaRiAbLe").asNumber() == 4);
    
    // Non-existent case variations should be nil
    assert(state.getGlobal("variablE").isNil());
    assert(state.getGlobal("Variable ").isNil());  // with space
    assert(state.getGlobal(" Variable").isNil());  // with leading space
}

void StateGlobalTest::testGlobalValueTypes() {
    State state;
    
    // Test setting and getting different value types
    
    // Integer numbers
    state.setGlobal("int_pos", Value(42));
    state.setGlobal("int_neg", Value(-17));
    state.setGlobal("int_zero", Value(0));
    
    // Floating point numbers
    state.setGlobal("float_pos", Value(3.14159));
    state.setGlobal("float_neg", Value(-2.71828));
    state.setGlobal("float_small", Value(0.000001));
    state.setGlobal("float_large", Value(1000000.0));
    
    // Strings
    state.setGlobal("str_normal", Value("hello"));
    state.setGlobal("str_empty", Value(""));
    state.setGlobal("str_space", Value(" "));
    state.setGlobal("str_newline", Value("line1\nline2"));
    state.setGlobal("str_tab", Value("col1\tcol2"));
    
    // Booleans
    state.setGlobal("bool_true", Value(true));
    state.setGlobal("bool_false", Value(false));
    
    // Nil
    state.setGlobal("nil_val", Value(nullptr));
    
    // Verify all values and types
    assert(state.getGlobal("int_pos").asNumber() == 42);
    assert(state.getGlobal("int_neg").asNumber() == -17);
    assert(state.getGlobal("int_zero").asNumber() == 0);
    
    assert(state.getGlobal("float_pos").asNumber() == 3.14159);
    assert(state.getGlobal("float_neg").asNumber() == -2.71828);
    assert(state.getGlobal("float_small").asNumber() == 0.000001);
    assert(state.getGlobal("float_large").asNumber() == 1000000.0);
    
    assert(state.getGlobal("str_normal").toString() == "hello");
    assert(state.getGlobal("str_empty").toString() == "");
    assert(state.getGlobal("str_space").toString() == " ");
    assert(state.getGlobal("str_newline").toString() == "line1\nline2");
    assert(state.getGlobal("str_tab").toString() == "col1\tcol2");
    
    assert(state.getGlobal("bool_true").asBoolean() == true);
    assert(state.getGlobal("bool_false").asBoolean() == false);
    
    assert(state.getGlobal("nil_val").isNil());
    
    // Verify types
    assert(state.getGlobal("int_pos").isNumber());
    assert(state.getGlobal("float_pos").isNumber());
    assert(state.getGlobal("str_normal").isString());
    assert(state.getGlobal("bool_true").isBoolean());
    assert(state.getGlobal("nil_val").isNil());
}

} // namespace Tests
} // namespace Lua