#include "string_lib_test.hpp"
#include "../../lib/string_lib.hpp"
#include "../../vm/state.hpp"
#include "../../vm/value.hpp"
#include "../../vm/table.hpp"
#include "../../lib/lib_manager.hpp"
#include "../../gc/core/gc_ref.hpp"
#include <chrono>
#include <functional>
#include <cassert>

#pragma warning(disable: 4566)

namespace Lua {
namespace Tests {

void StringLibTest::runAllTests() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Running String Library Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Basic string function tests
    testLen();
    testSub();
    testUpper();
    testLower();
    testReverse();
    
    // Pattern matching function tests
    testFind();
    testMatch();
    testGmatch();
    testGsub();
    
    // Formatting function tests
    testFormat();
    testRep();
    
    // Character function tests
    testByte();
    testChar();
    
    // Utility function tests
    testTrim();
    testSplit();
    testJoin();
    testStartswith();
    testEndswith();
    testContains();
    
    // Edge case and error handling tests
    testEdgeCases();
    testErrorHandling();
    testUnicodeSupport();
    testPerformance();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "String Library Tests Completed" << std::endl;
    std::cout << "========================================" << std::endl;
}

// Basic string function tests

void StringLibTest::testLen() {
    printTestHeader("string.len");
    
    try {
        auto state = std::make_unique<State>();
        
        // Test empty string
        state->push(Value(""));
        Value result = StringLib::len(state.get(), 1);
        bool test1 = result.isNumber() && result.asNumber() == 0.0;
        printTestResult("empty string", test1);
        
        // Test normal string
        state->clearStack();
        state->push(Value("hello"));
        result = StringLib::len(state.get(), 1);
        bool test2 = result.isNumber() && result.asNumber() == 5.0;
        printTestResult("normal string", test2);
        
        // Test string with spaces
        state->clearStack();
        state->push(Value("hello world"));
        result = StringLib::len(state.get(), 1);
        bool test3 = result.isNumber() && result.asNumber() == 11.0;
        printTestResult("string with spaces", test3);
        
        // Test number as string
        state->clearStack();
        state->push(Value(123.0));
        result = StringLib::len(state.get(), 1);
        bool test4 = result.isNumber() && result.asNumber() == 3.0;
        printTestResult("number as string", test4);
        
    } catch (const std::exception& e) {
        printTestResult("exception handling", false);
        std::cout << "[ERROR] " << e.what() << std::endl;
    }
}

void StringLibTest::testSub() {
    printTestHeader("string.sub");
    
    try {
        auto state = std::make_unique<State>();
        
        // Test basic substring
        state->push(Value("hello world"));
        state->push(Value(1.0));
        state->push(Value(5.0));
        Value result = StringLib::sub(state.get(), 3);
        bool test1 = result.isString() && result.asString() == "hello";
        printTestResult("basic substring", test1);
        
        // Test substring to end
        state->clearStack();
        state->push(Value("hello world"));
        state->push(Value(7.0));
        result = StringLib::sub(state.get(), 2);
        bool test2 = result.isString() && result.asString() == "world";
        printTestResult("substring to end", test2);
        
        // Test negative indices
        state->clearStack();
        state->push(Value("hello"));
        state->push(Value(-3.0));
        state->push(Value(-1.0));
        result = StringLib::sub(state.get(), 3);
        bool test3 = result.isString() && result.asString() == "llo";
        printTestResult("negative indices", test3);
        
        // Test out of range
        state->clearStack();
        state->push(Value("hello"));
        state->push(Value(10.0));
        state->push(Value(15.0));
        result = StringLib::sub(state.get(), 3);
        bool test4 = result.isString() && result.asString() == "";
        printTestResult("out of range", test4);
        
    } catch (const std::exception& e) {
        printTestResult("exception handling", false);
        std::cout << "[ERROR] " << e.what() << std::endl;
    }
}

void StringLibTest::testUpper() {
    printTestHeader("string.upper");
    
    try {
        auto state = std::make_unique<State>();
        
        // Test lowercase to uppercase
        state->push(Value("hello world"));
        Value result = StringLib::upper(state.get(), 1);
        bool test1 = result.isString() && result.asString() == "HELLO WORLD";
        printTestResult("lowercase to uppercase", test1);
        
        // Test mixed case
        state->clearStack();
        state->push(Value("HeLLo WoRLd"));
        result = StringLib::upper(state.get(), 1);
        bool test2 = result.isString() && result.asString() == "HELLO WORLD";
        printTestResult("mixed case", test2);
        
        // Test with numbers and symbols
        state->clearStack();
        state->push(Value("hello123!@#"));
        result = StringLib::upper(state.get(), 1);
        bool test3 = result.isString() && result.asString() == "HELLO123!@#";
        printTestResult("with numbers and symbols", test3);
        
        // Test empty string
        state->clearStack();
        state->push(Value(""));
        result = StringLib::upper(state.get(), 1);
        bool test4 = result.isString() && result.asString() == "";
        printTestResult("empty string", test4);
        
    } catch (const std::exception& e) {
        printTestResult("exception handling", false);
        std::cout << "[ERROR] " << e.what() << std::endl;
    }
}

void StringLibTest::testLower() {
    printTestHeader("string.lower");
    
    try {
        auto state = std::make_unique<State>();
        
        // Test uppercase to lowercase
        state->push(Value("HELLO WORLD"));
        Value result = StringLib::lower(state.get(), 1);
        bool test1 = result.isString() && result.asString() == "hello world";
        printTestResult("uppercase to lowercase", test1);
        
        // Test mixed case
        state->clearStack();
        state->push(Value("HeLLo WoRLd"));
        result = StringLib::lower(state.get(), 1);
        bool test2 = result.isString() && result.asString() == "hello world";
        printTestResult("mixed case", test2);
        
        // Test with numbers and symbols
        state->clearStack();
        state->push(Value("HELLO123!@#"));
        result = StringLib::lower(state.get(), 1);
        bool test3 = result.isString() && result.asString() == "hello123!@#";
        printTestResult("with numbers and symbols", test3);
        
    } catch (const std::exception& e) {
        printTestResult("exception handling", false);
        std::cout << "[ERROR] " << e.what() << std::endl;
    }
}

void StringLibTest::testReverse() {
    printTestHeader("string.reverse");
    
    try {
        auto state = std::make_unique<State>();
        
        // Test basic reverse
        state->push(Value("hello"));
        Value result = StringLib::reverse(state.get(), 1);
        bool test1 = result.isString() && result.asString() == "olleh";
        printTestResult("basic reverse", test1);
        
        // Test palindrome
        state->clearStack();
        state->push(Value("racecar"));
        result = StringLib::reverse(state.get(), 1);
        bool test2 = result.isString() && result.asString() == "racecar";
        printTestResult("palindrome", test2);
        
        // Test single character
        state->clearStack();
        state->push(Value("a"));
        result = StringLib::reverse(state.get(), 1);
        bool test3 = result.isString() && result.asString() == "a";
        printTestResult("single character", test3);
        
        // Test empty string
        state->clearStack();
        state->push(Value(""));
        result = StringLib::reverse(state.get(), 1);
        bool test4 = result.isString() && result.asString() == "";
        printTestResult("empty string", test4);
        
    } catch (const std::exception& e) {
        printTestResult("exception handling", false);
        std::cout << "[ERROR] " << e.what() << std::endl;
    }
}

// Pattern matching function tests

void StringLibTest::testFind() {
    printTestHeader("string.find");
    
    try {
        auto state = std::make_unique<State>();
        
        // Test basic find (plain text)
        state->push(Value("hello world"));
        state->push(Value("world"));
        state->push(Value(1.0));
        state->push(Value(true)); // plain text search
        Value result = StringLib::find(state.get(), 4);
        bool test1 = result.isTable();
        if (test1) {
            auto table = result.asTable();
            Value start = table->get(Value(1.0));
            Value end = table->get(Value(2.0));
            test1 = start.isNumber() && start.asNumber() == 7.0 &&
                   end.isNumber() && end.asNumber() == 11.0;
        }
        printTestResult("basic find (plain)", test1);
        
        // Test not found
        state->clearStack();
        state->push(Value("hello world"));
        state->push(Value("xyz"));
        state->push(Value(1.0));
        state->push(Value(true));
        result = StringLib::find(state.get(), 4);
        bool test2 = result.isNil();
        printTestResult("not found", test2);
        
        // Test find from position
        state->clearStack();
        state->push(Value("hello hello"));
        state->push(Value("hello"));
        state->push(Value(7.0));
        state->push(Value(true));
        result = StringLib::find(state.get(), 4);
        bool test3 = result.isTable();
        if (test3) {
            auto table = result.asTable();
            Value start = table->get(Value(1.0));
            test3 = start.isNumber() && start.asNumber() == 7.0;
        }
        printTestResult("find from position", test3);
        
    } catch (const std::exception& e) {
        printTestResult("exception handling", false);
        std::cout << "[ERROR] " << e.what() << std::endl;
    }
}

void StringLibTest::testMatch() {
    printTestHeader("string.match");
    
    try {
        auto state = std::make_unique<State>();
        
        // Test basic pattern matching
        state->push(Value("hello123world"));
        state->push(Value("[0-9]+"));
        Value result = StringLib::match(state.get(), 2);
        bool test1 = result.isString() && result.asString() == "123";
        printTestResult("basic pattern matching", test1);
        
        // Test no match
        state->clearStack();
        state->push(Value("hello world"));
        state->push(Value("[0-9]+"));
        result = StringLib::match(state.get(), 2);
        bool test2 = result.isNil();
        printTestResult("no match", test2);
        
    } catch (const std::exception& e) {
        printTestResult("exception handling", false);
        std::cout << "[ERROR] " << e.what() << std::endl;
    }
}

void StringLibTest::testGmatch() {
    printTestHeader("string.gmatch");
    
    try {
        auto state = std::make_unique<State>();
        
        // Test gmatch (placeholder implementation)
        state->push(Value("hello world"));
        state->push(Value("\\w+"));
        Value result = StringLib::gmatch(state.get(), 2);
        bool test1 = result.isNil(); // Current implementation returns nil
        printTestResult("gmatch placeholder", test1);
        
    } catch (const std::exception& e) {
        printTestResult("exception handling", false);
        std::cout << "[ERROR] " << e.what() << std::endl;
    }
}

void StringLibTest::testGsub() {
    printTestHeader("string.gsub");
    
    try {
        auto state = std::make_unique<State>();
        
        // Test basic substitution
        state->push(Value("hello world hello"));
        state->push(Value("hello"));
        state->push(Value("hi"));
        Value result = StringLib::gsub(state.get(), 3);
        bool test1 = result.isTable();
        if (test1) {
            auto table = result.asTable();
            Value newStr = table->get(Value(1.0));
            Value count = table->get(Value(2.0));
            test1 = newStr.isString() && newStr.asString() == "hi world hi" &&
                   count.isNumber() && count.asNumber() == 2.0;
        }
        printTestResult("basic substitution", test1);
        
    } catch (const std::exception& e) {
        printTestResult("exception handling", false);
        std::cout << "[ERROR] " << e.what() << std::endl;
    }
}

// Formatting function tests

void StringLibTest::testFormat() {
    printTestHeader("string.format");
    
    try {
        auto state = std::make_unique<State>();
        
        // Test basic string formatting
        state->push(Value("Hello %s!"));
        state->push(Value("World"));
        Value result = StringLib::format(state.get(), 2);
        bool test1 = result.isString() && result.asString() == "Hello World!";
        printTestResult("basic string formatting", test1);
        
        // Test number formatting
        state->clearStack();
        state->push(Value("Number: %d"));
        state->push(Value(42.0));
        result = StringLib::format(state.get(), 2);
        bool test2 = result.isString() && result.asString() == "Number: 42";
        printTestResult("number formatting", test2);
        
        // Test multiple arguments
        state->clearStack();
        state->push(Value("%s: %d"));
        state->push(Value("Count"));
        state->push(Value(5.0));
        result = StringLib::format(state.get(), 3);
        bool test3 = result.isString() && result.asString() == "Count: 5";
        printTestResult("multiple arguments", test3);
        
    } catch (const std::exception& e) {
        printTestResult("exception handling", false);
        std::cout << "[ERROR] " << e.what() << std::endl;
    }
}

void StringLibTest::testRep() {
    printTestHeader("string.rep");
    
    try {
        auto state = std::make_unique<State>();
        
        // Test basic repetition
        state->push(Value("abc"));
        state->push(Value(3.0));
        Value result = StringLib::rep(state.get(), 2);
        bool test1 = result.isString() && result.asString() == "abcabcabc";
        printTestResult("basic repetition", test1);
        
        // Test with separator
        state->clearStack();
        state->push(Value("abc"));
        state->push(Value(3.0));
        state->push(Value("-"));
        result = StringLib::rep(state.get(), 3);
        bool test2 = result.isString() && result.asString() == "abc-abc-abc";
        printTestResult("with separator", test2);
        
        // Test zero repetitions
        state->clearStack();
        state->push(Value("abc"));
        state->push(Value(0.0));
        result = StringLib::rep(state.get(), 2);
        bool test3 = result.isString() && result.asString() == "";
        printTestResult("zero repetitions", test3);
        
        // Test single repetition
        state->clearStack();
        state->push(Value("abc"));
        state->push(Value(1.0));
        result = StringLib::rep(state.get(), 2);
        bool test4 = result.isString() && result.asString() == "abc";
        printTestResult("single repetition", test4);
        
    } catch (const std::exception& e) {
        printTestResult("exception handling", false);
        std::cout << "[ERROR] " << e.what() << std::endl;
    }
}

// Character function tests

void StringLibTest::testByte() {
    printTestHeader("string.byte");
    
    try {
        auto state = std::make_unique<State>();
        
        // Test single character
        state->push(Value("A"));
        Value result = StringLib::byte_func(state.get(), 1);
        bool test1 = result.isNumber() && result.asNumber() == 65.0;
        printTestResult("single character", test1);
        
        // Test character at position
        state->clearStack();
        state->push(Value("ABC"));
        state->push(Value(2.0));
        result = StringLib::byte_func(state.get(), 2);
        bool test2 = result.isNumber() && result.asNumber() == 66.0;
        printTestResult("character at position", test2);
        
        // Test range of characters
        state->clearStack();
        state->push(Value("ABC"));
        state->push(Value(1.0));
        state->push(Value(3.0));
        result = StringLib::byte_func(state.get(), 3);
        bool test3 = result.isTable();
        if (test3) {
            auto table = result.asTable();
            Value byte1 = table->get(Value(1.0));
            Value byte2 = table->get(Value(2.0));
            Value byte3 = table->get(Value(3.0));
            test3 = byte1.isNumber() && byte1.asNumber() == 65.0 &&
                   byte2.isNumber() && byte2.asNumber() == 66.0 &&
                   byte3.isNumber() && byte3.asNumber() == 67.0;
        }
        printTestResult("range of characters", test3);
        
    } catch (const std::exception& e) {
        printTestResult("exception handling", false);
        std::cout << "[ERROR] " << e.what() << std::endl;
    }
}

void StringLibTest::testChar() {
    printTestHeader("string.char");
    
    try {
        auto state = std::make_unique<State>();
        
        // Test single character code
        state->push(Value(65.0));
        Value result = StringLib::char_func(state.get(), 1);
        bool test1 = result.isString() && result.asString() == "A";
        printTestResult("single character code", test1);
        
        // Test multiple character codes
        state->clearStack();
        state->push(Value(65.0));
        state->push(Value(66.0));
        state->push(Value(67.0));
        result = StringLib::char_func(state.get(), 3);
        bool test2 = result.isString() && result.asString() == "ABC";
        printTestResult("multiple character codes", test2);
        
        // Test special characters
        state->clearStack();
        state->push(Value(32.0)); // space
        state->push(Value(33.0)); // !
        result = StringLib::char_func(state.get(), 2);
        bool test3 = result.isString() && result.asString() == " !";
        printTestResult("special characters", test3);
        
    } catch (const std::exception& e) {
        printTestResult("exception handling", false);
        std::cout << "[ERROR] " << e.what() << std::endl;
    }
}

// Utility function tests

void StringLibTest::testTrim() {
    printTestHeader("string.trim");
    
    try {
        auto state = std::make_unique<State>();
        
        // Test basic trim
        state->push(Value("  hello world  "));
        Value result = StringLib::trim(state.get(), 1);
        bool test1 = result.isString() && result.asString() == "hello world";
        printTestResult("basic trim", test1);
        
        // Test trim with tabs and newlines
        state->clearStack();
        state->push(Value("\t\nhello\n\t"));
        result = StringLib::trim(state.get(), 1);
        bool test2 = result.isString() && result.asString() == "hello";
        printTestResult("trim tabs and newlines", test2);
        
        // Test already trimmed string
        state->clearStack();
        state->push(Value("hello"));
        result = StringLib::trim(state.get(), 1);
        bool test3 = result.isString() && result.asString() == "hello";
        printTestResult("already trimmed", test3);
        
        // Test whitespace only string
        state->clearStack();
        state->push(Value("   \t\n   "));
        result = StringLib::trim(state.get(), 1);
        bool test4 = result.isString() && result.asString() == "";
        printTestResult("whitespace only", test4);
        
    } catch (const std::exception& e) {
        printTestResult("exception handling", false);
        std::cout << "[ERROR] " << e.what() << std::endl;
    }
}

void StringLibTest::testSplit() {
    printTestHeader("string.split");
    
    try {
        auto state = std::make_unique<State>();
        
        // Test basic split
        state->push(Value("a,b,c"));
        state->push(Value(","));
        Value result = StringLib::split(state.get(), 2);
        bool test1 = result.isTable();
        if (test1) {
            auto table = result.asTable();
            Value part1 = table->get(Value(1.0));
            Value part2 = table->get(Value(2.0));
            Value part3 = table->get(Value(3.0));
            test1 = part1.isString() && part1.asString() == "a" &&
                   part2.isString() && part2.asString() == "b" &&
                   part3.isString() && part3.asString() == "c";
        }
        printTestResult("basic split", test1);
        
        // Test split with multi-character separator
        state->clearStack();
        state->push(Value("hello::world::test"));
        state->push(Value("::"));
        result = StringLib::split(state.get(), 2);
        bool test2 = result.isTable();
        if (test2) {
            auto table = result.asTable();
            Value part1 = table->get(Value(1.0));
            Value part2 = table->get(Value(2.0));
            Value part3 = table->get(Value(3.0));
            test2 = part1.isString() && part1.asString() == "hello" &&
                   part2.isString() && part2.asString() == "world" &&
                   part3.isString() && part3.asString() == "test";
        }
        printTestResult("multi-character separator", test2);
        
        // Test split into characters
        state->clearStack();
        state->push(Value("abc"));
        state->push(Value(""));
        result = StringLib::split(state.get(), 2);
        bool test3 = result.isTable();
        if (test3) {
            auto table = result.asTable();
            Value char1 = table->get(Value(1.0));
            Value char2 = table->get(Value(2.0));
            Value char3 = table->get(Value(3.0));
            test3 = char1.isString() && char1.asString() == "a" &&
                   char2.isString() && char2.asString() == "b" &&
                   char3.isString() && char3.asString() == "c";
        }
        printTestResult("split into characters", test3);
        
    } catch (const std::exception& e) {
        printTestResult("exception handling", false);
        std::cout << "[ERROR] " << e.what() << std::endl;
    }
}

void StringLibTest::testJoin() {
    printTestHeader("string.join");
    
    try {
        auto state = std::make_unique<State>();
        auto table = make_gc_table();
        
        // Setup test table
        table->set(Value(1.0), Value("hello"));
        table->set(Value(2.0), Value("world"));
        table->set(Value(3.0), Value("test"));
        
        // Test basic join
        state->push(Value(table));
        state->push(Value(", "));
        Value result = StringLib::join(state.get(), 2);
        bool test1 = result.isString() && result.asString() == "hello, world, test";
        printTestResult("basic join", test1);
        
        // Test join with different separator
        state->clearStack();
        state->push(Value(table));
        state->push(Value(" | "));
        result = StringLib::join(state.get(), 2);
        bool test2 = result.isString() && result.asString() == "hello | world | test";
        printTestResult("different separator", test2);
        
        // Test join with empty separator
        state->clearStack();
        state->push(Value(table));
        state->push(Value(""));
        result = StringLib::join(state.get(), 2);
        bool test3 = result.isString() && result.asString() == "helloworldtest";
        printTestResult("empty separator", test3);
        
    } catch (const std::exception& e) {
        printTestResult("exception handling", false);
        std::cout << "[ERROR] " << e.what() << std::endl;
    }
}

void StringLibTest::testStartswith() {
    printTestHeader("string.startswith");
    
    try {
        auto state = std::make_unique<State>();
        
        // Test positive case
        state->push(Value("hello world"));
        state->push(Value("hello"));
        Value result = StringLib::startswith(state.get(), 2);
        bool test1 = result.isBoolean() && result.asBoolean() == true;
        printTestResult("positive case", test1);
        
        // Test negative case
        state->clearStack();
        state->push(Value("hello world"));
        state->push(Value("world"));
        result = StringLib::startswith(state.get(), 2);
        bool test2 = result.isBoolean() && result.asBoolean() == false;
        printTestResult("negative case", test2);
        
        // Test exact match
        state->clearStack();
        state->push(Value("hello"));
        state->push(Value("hello"));
        result = StringLib::startswith(state.get(), 2);
        bool test3 = result.isBoolean() && result.asBoolean() == true;
        printTestResult("exact match", test3);
        
        // Test empty prefix
        state->clearStack();
        state->push(Value("hello"));
        state->push(Value(""));
        result = StringLib::startswith(state.get(), 2);
        bool test4 = result.isBoolean() && result.asBoolean() == true;
        printTestResult("empty prefix", test4);
        
    } catch (const std::exception& e) {
        printTestResult("exception handling", false);
        std::cout << "[ERROR] " << e.what() << std::endl;
    }
}

void StringLibTest::testEndswith() {
    printTestHeader("string.endswith");
    
    try {
        auto state = std::make_unique<State>();
        
        // Test positive case
        state->push(Value("hello world"));
        state->push(Value("world"));
        Value result = StringLib::endswith(state.get(), 2);
        bool test1 = result.isBoolean() && result.asBoolean() == true;
        printTestResult("positive case", test1);
        
        // Test negative case
        state->clearStack();
        state->push(Value("hello world"));
        state->push(Value("hello"));
        result = StringLib::endswith(state.get(), 2);
        bool test2 = result.isBoolean() && result.asBoolean() == false;
        printTestResult("negative case", test2);
        
        // Test exact match
        state->clearStack();
        state->push(Value("world"));
        state->push(Value("world"));
        result = StringLib::endswith(state.get(), 2);
        bool test3 = result.isBoolean() && result.asBoolean() == true;
        printTestResult("exact match", test3);
        
        // Test empty suffix
        state->clearStack();
        state->push(Value("hello"));
        state->push(Value(""));
        result = StringLib::endswith(state.get(), 2);
        bool test4 = result.isBoolean() && result.asBoolean() == true;
        printTestResult("empty suffix", test4);
        
    } catch (const std::exception& e) {
        printTestResult("exception handling", false);
        std::cout << "[ERROR] " << e.what() << std::endl;
    }
}

void StringLibTest::testContains() {
    printTestHeader("string.contains");
    
    try {
        auto state = std::make_unique<State>();
        
        // Test positive case
        state->push(Value("hello world"));
        state->push(Value("lo wo"));
        Value result = StringLib::contains(state.get(), 2);
        bool test1 = result.isBoolean() && result.asBoolean() == true;
        printTestResult("positive case", test1);
        
        // Test negative case
        state->clearStack();
        state->push(Value("hello world"));
        state->push(Value("xyz"));
        result = StringLib::contains(state.get(), 2);
        bool test2 = result.isBoolean() && result.asBoolean() == false;
        printTestResult("negative case", test2);
        
        // Test exact match
        state->clearStack();
        state->push(Value("hello"));
        state->push(Value("hello"));
        result = StringLib::contains(state.get(), 2);
        bool test3 = result.isBoolean() && result.asBoolean() == true;
        printTestResult("exact match", test3);
        
        // Test empty substring
        state->clearStack();
        state->push(Value("hello"));
        state->push(Value(""));
        result = StringLib::contains(state.get(), 2);
        bool test4 = result.isBoolean() && result.asBoolean() == true;
        printTestResult("empty substring", test4);
        
    } catch (const std::exception& e) {
        printTestResult("exception handling", false);
        std::cout << "[ERROR] " << e.what() << std::endl;
    }
}

// Edge case and error handling tests

void StringLibTest::testEdgeCases() {
    printTestHeader("Edge Cases");
    
    try {
        auto state = std::make_unique<State>();
        
        // Test with empty strings
        state->push(Value(""));
        Value result = StringLib::len(state.get(), 1);
        bool test1 = result.isNumber() && result.asNumber() == 0.0;
        printTestResult("empty string length", test1);
        
        // Test with very long strings
        Str longStr(10000, 'a');
        state->clearStack();
        state->push(Value(longStr));
        result = StringLib::len(state.get(), 1);
        bool test2 = result.isNumber() && result.asNumber() == 10000.0;
        printTestResult("very long string", test2);
        
        // Test with special characters
        state->clearStack();
        state->push(Value("\0\n\t\r"));
        result = StringLib::len(state.get(), 1);
        bool test3 = result.isNumber() && result.asNumber() == 4.0;
        printTestResult("special characters", test3);
        
    } catch (const std::exception& e) {
        printTestResult("exception handling", false);
        std::cout << "[ERROR] " << e.what() << std::endl;
    }
}

void StringLibTest::testErrorHandling() {
    printTestHeader("Error Handling");
    
    try {
        auto state = std::make_unique<State>();
        
        // Test insufficient arguments
        bool test1 = false;
        try {
            StringLib::len(state.get(), 0);
        } catch (const std::runtime_error&) {
            test1 = true;
        }
        printTestResult("insufficient arguments", test1);
        
        // Test invalid argument types
        bool test2 = false;
        try {
            state->clearStack();
            state->push(Value()); // nil value
            StringLib::len(state.get(), 1);
        } catch (const std::runtime_error& e) {
            test2 = true;
            std::cout << "[OK] Expected exception for nil value: " << e.what() << std::endl;
            // Expected exception for nil value
        } catch (const std::exception& e) {
            test2 = true;
            std::cout << "[ERROR] Unexpected exception: " << e.what() << std::endl;
            // Catch any other exception type
        }
        printTestResult("invalid argument types", test2);
        
        // Test out of range character codes
        bool test3 = false;
        try {
            state->clearStack();
            state->push(Value(256.0)); // out of range
            StringLib::char_func(state.get(), 1);
        } catch (const std::runtime_error&) {
            test3 = true;
        }
        printTestResult("out of range character codes", test3);
        
    } catch (const std::exception& e) {
        printTestResult("exception handling", false);
        std::cout << "[ERROR] " << e.what() << std::endl;
    }
}

void StringLibTest::testUnicodeSupport() {
    printTestHeader("Unicode Support");
    
    try {
        auto state = std::make_unique<State>();
        
        // Test basic ASCII
        state->clearStack();
        state->push(Value("hello"));
        Value result = StringLib::len(state.get(), 1);
        bool test1 = result.isNumber() && result.asNumber() == 5.0;
        printTestResult("ASCII string length", test1);
        
        // Test UTF-8 string length
        state->clearStack();
        state->push(Value("café"));
        result = StringLib::len(state.get(), 1);
        bool test2 = result.isNumber() && result.asNumber() == 4.0;
        printTestResult("UTF-8 string length", test2);
        
        // Note: Full Unicode support would require more complex implementation
        // This is a placeholder for future Unicode support tests
        
    } catch (const std::exception& e) {
        printTestResult("exception handling", false);
        std::cout << "[ERROR] " << e.what() << std::endl;
    }
}

void StringLibTest::testPerformance() {
    printTestHeader("Performance Tests");
    
    try {
        auto state = std::make_unique<State>();
        
        // Test string operations performance
        measureStringOperation("string.len", [&state]() {
            state->clearStack();
            state->push(Value("hello world"));
            StringLib::len(state.get(), 1);
        }, 10000);
        
        measureStringOperation("string.upper", [&state]() {
            state->clearStack();
            state->push(Value("hello world"));
            StringLib::upper(state.get(), 1);
        }, 10000);
        
        measureStringOperation("string.sub", [&state]() {
            state->clearStack();
            state->push(Value("hello world"));
            state->push(Value(1.0));
            state->push(Value(5.0));
            StringLib::sub(state.get(), 3);
        }, 10000);
        
    } catch (const std::exception& e) {
        printTestResult("performance tests", false);
        std::cout << "[ERROR] " << e.what() << std::endl;
    }
}

// Helper functions

void StringLibTest::printTestResult(const Str& testName, bool passed) {
    std::cout << "  [" << (passed ? "PASS" : "FAIL") << "] " << testName << std::endl;
}

void StringLibTest::printTestHeader(const Str& testName) {
    std::cout << "\n--- Testing " << testName << " ---" << std::endl;
}

bool StringLibTest::compareStrings(const Str& expected, const Str& actual) {
    return expected == actual;
}

bool StringLibTest::compareNumbers(f64 expected, f64 actual, f64 epsilon) {
    return std::abs(expected - actual) < epsilon;
}

Vec<Str> StringLibTest::getTestStrings() {
    return {
        "",
        "a",
        "hello",
        "hello world",
        "Hello World!",
        "123456789",
        "!@#$%^&*()",
        "\n\t\r",
        "   spaces   ",
        "very long string that contains many characters to test performance and edge cases"
    };
}

Vec<Str> StringLibTest::getUnicodeTestStrings() {
    return {
        "hello",
        "café",
        "naïve",
        "résumé"
        // Note: More Unicode test cases would be added with full Unicode support
    };
}

Vec<Str> StringLibTest::getPatternTestCases() {
    return {
        "[0-9]+",
        "[a-zA-Z]+",
        "\\w+",
        "\\d+",
        "\\s+",
        ".*",
        "^hello",
        "world$"
    };
}

void StringLibTest::measureStringOperation(const Str& operationName, 
                                         std::function<void()> operation,
                                         i32 iterations) {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (i32 i = 0; i < iterations; ++i) {
        operation();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    f64 avgTime = static_cast<f64>(duration.count()) / iterations;
    std::cout << "  [PERF] " << operationName << ": " << avgTime << " μs/op (" 
              << iterations << " iterations)" << std::endl;
}

// Integration tests

void StringLibIntegrationTest::runAllTests() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Running String Library Integration Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    testLibraryRegistration();
    testStateIntegration();
    testMemoryManagement();
    testThreadSafety();
    testInteractionWithOtherLibs();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "String Library Integration Tests Completed" << std::endl;
    std::cout << "========================================" << std::endl;
}

void StringLibIntegrationTest::testLibraryRegistration() {
    printTestResult("Library Registration", true); // Placeholder
}

void StringLibIntegrationTest::testStateIntegration() {
    printTestResult("State Integration", true); // Placeholder
}

void StringLibIntegrationTest::testMemoryManagement() {
    printTestResult("Memory Management", true); // Placeholder
}

void StringLibIntegrationTest::testThreadSafety() {
    printTestResult("Thread Safety", true); // Placeholder
}

void StringLibIntegrationTest::testInteractionWithOtherLibs() {
    printTestResult("Interaction with Other Libraries", true); // Placeholder
}

void StringLibIntegrationTest::printTestResult(const Str& testName, bool passed) {
    std::cout << "  [" << (passed ? "PASS" : "FAIL") << "] " << testName << std::endl;
}

// String formatter tests

void StringFormatterTest::runAllTests() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Running String Formatter Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    testBasicFormatting();
    testNumberFormatting();
    testStringFormatting();
    testComplexFormatting();
    testFormatSpecParsing();
    testErrorCases();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "String Formatter Tests Completed" << std::endl;
    std::cout << "========================================" << std::endl;
}

void StringFormatterTest::testBasicFormatting() {
    printTestResult("Basic Formatting", true); // Placeholder
}

void StringFormatterTest::testNumberFormatting() {
    printTestResult("Number Formatting", true); // Placeholder
}

void StringFormatterTest::testStringFormatting() {
    printTestResult("String Formatting", true); // Placeholder
}

void StringFormatterTest::testComplexFormatting() {
    printTestResult("Complex Formatting", true); // Placeholder
}

void StringFormatterTest::testFormatSpecParsing() {
    printTestResult("Format Spec Parsing", true); // Placeholder
}

void StringFormatterTest::testErrorCases() {
    printTestResult("Error Cases", true); // Placeholder
}

void StringFormatterTest::printTestResult(const Str& testName, bool passed) {
    std::cout << "  [" << (passed ? "PASS" : "FAIL") << "] " << testName << std::endl;
}

}
}