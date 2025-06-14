#pragma once


#include "../../parser/ast/parse_error.hpp"
#include "../../parser/ast/source_location.hpp"

namespace Lua::Tests {
    /**
     * @brief Parse Error Test Suite
     * 
     * This class provides comprehensive tests for the ParseError class and related
     * error handling functionality. It tests error creation, formatting, suggestions,
     * error collection, and severity handling.
     */
    class ParseErrorTest {
    public:
        /**
         * @brief Run all parse error tests
         * 
         * Executes all parse error related test cases including basic error creation,
         * error formatting, suggestions, error collection, and severity handling.
         */
        static void runAllTests();
        
    private:
        static void testBasicErrorCreation();
        static void testErrorWithSuggestions();
        static void testStaticFactoryMethods();
        static void testErrorFormatting();
        static void testErrorCollector();
        static void testErrorSeverity();
        static void testRelatedErrors();
    };
}
