#pragma once

#include "lib_framework.hpp"
#include "../common/types.hpp"
#include "../vm/state.hpp"
#include "../vm/value.hpp"
#include <string>
#include <regex>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>

namespace Lua {
    // Forward declarations
    namespace Tests {
        class StringLibTest;
    }
    
    /**
     * @brief String library implementation
     * 
     * Provides string manipulation functions including:
     * - Basic operations: len, sub, upper, lower, reverse
     * - Pattern matching: find, match, gmatch, gsub
     * - Formatting: format, rep
     * - Character operations: byte, char
     * - Utility functions: trim, split, join
     */
    class StringLib : public LibModule {
        friend class Tests::StringLibTest;
        
    public:
        StringLib() = default;
        
        StrView getName() const noexcept override;
        void registerFunctions(FunctionRegistry& registry, const LibraryContext& context) override;
        
    private:
        // Basic string functions
        static Value len(State* state, i32 nargs);
        static Value sub(State* state, i32 nargs);
        static Value upper(State* state, i32 nargs);
        static Value lower(State* state, i32 nargs);
        static Value reverse(State* state, i32 nargs);
        
        // Pattern matching functions
        static Value find(State* state, i32 nargs);
        static Value match(State* state, i32 nargs);
        static Value gmatch(State* state, i32 nargs);
        static Value gsub(State* state, i32 nargs);
        
        // Formatting functions
        static Value format(State* state, i32 nargs);
        static Value rep(State* state, i32 nargs);
        
        // Character functions
        static Value byte_func(State* state, i32 nargs);
        static Value char_func(State* state, i32 nargs);
        
        // Utility functions
        static Value trim(State* state, i32 nargs);
        static Value split(State* state, i32 nargs);
        static Value join(State* state, i32 nargs);
        static Value startswith(State* state, i32 nargs);
        static Value endswith(State* state, i32 nargs);
        static Value contains(State* state, i32 nargs);
        
        // Helper functions
        static Str escapePattern(const Str& pattern);
        static bool isValidUtf8(const Str& str);
        static usize utf8Length(const Str& str);
        static Str utf8Substring(const Str& str, usize start, usize length);
        
        // Pattern matching helpers
        static bool matchPattern(const Str& text, const Str& pattern, usize& start, usize& end);
        static Str replacePattern(const Str& text, const Str& pattern, const Str& replacement);
        
        // Validation helpers
        static void validateStringArg(const Value& val, const Str& funcName, i32 argIndex);
        static void validateNumberArg(const Value& val, const Str& funcName, i32 argIndex);
        static void validateRange(i32 start, i32 end, usize strLen, const Str& funcName);
    };
    
    /**
     * @brief String formatting utilities
     * 
     * Helper class for string.format implementation
     */
    class StringFormatter {
    public:
        static Str format(const Str& formatStr, const Vec<Value>& args);
        
    private:
        struct FormatSpec {
            char type = 's';        // Format type (s, d, f, x, etc.)
            i32 width = 0;          // Field width
            i32 precision = -1;     // Precision for floating point
            bool leftAlign = false; // Left alignment flag
            bool showSign = false;  // Show sign for numbers
            bool padZero = false;   // Pad with zeros
            char fill = ' ';        // Fill character
        };
        
        static FormatSpec parseFormatSpec(const Str& spec);
        static Str formatValue(const Value& value, const FormatSpec& spec);
        static Str formatString(const Str& str, const FormatSpec& spec);
        static Str formatNumber(f64 num, const FormatSpec& spec);
        static Str formatInteger(i64 num, const FormatSpec& spec);
    };
}