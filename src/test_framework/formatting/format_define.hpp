#pragma once


namespace Lua {
    namespace Tests {
    namespace TestFormatting {
    
    // Color type enumeration for different message types
    enum class ColorType {
        RESET,
        SUCCESS,
        ERROR_COLOR,
        WARNING,
        INFO,
        HEADER,
        SUBHEADER,
        EMPHASIS,
        DIM
    };

    // Color enumeration for different message types
    enum class Color {
        RESET, BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE,
        BRIGHT_BLACK, BRIGHT_RED, BRIGHT_GREEN, BRIGHT_YELLOW,
        BRIGHT_BLUE, BRIGHT_MAGENTA, BRIGHT_CYAN, BRIGHT_WHITE
    };

    // Test level enumeration for different test types
    enum class TestLevel {
        MAIN,      // main test suite (top level)
        MODULE,    // module level (e.g., parser, lexer, vm)
        SUITE,     // test suite (e.g., ExprTestSuite, StmtTestSuite)
        GROUP,     // test group (e.g., BinaryExprTest, UnaryExprTest)
        INDIVIDUAL // individual test case
    };
}}}