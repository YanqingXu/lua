#pragma once

#include "../../../common/types.hpp"

namespace Lua {
    class ReturnStmtTest {
    public:
        static void runAllTests();
        
    private:
        static void testEmptyReturn();
        static void testSingleReturn();
        static void testMultipleReturn();
        static void testReturnWithExpressions();
        static void testReturnSyntaxErrors();
        
        static void testReturnParsing(const Str& code, size_t expectedValueCount, const Str& description);
        static void testReturnParsingError(const Str& code, const Str& description);
    };
}