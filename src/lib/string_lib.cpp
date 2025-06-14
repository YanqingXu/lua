#include "string_lib.hpp"
#include "../vm/table.hpp"
#include "../gc/core/gc_ref.hpp"
#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>
#include <iomanip>
#include <locale>
#include <codecvt>

namespace Lua {
    
    StrView StringLib::getName() const noexcept {
        return "string";
    }
    
    void StringLib::registerFunctions(FunctionRegistry& registry) {
        // Basic string functions
        REGISTER_FUNCTION(registry, len, len);
        REGISTER_FUNCTION(registry, sub, sub);
        REGISTER_FUNCTION(registry, upper, upper);
        REGISTER_FUNCTION(registry, lower, lower);
        REGISTER_FUNCTION(registry, reverse, reverse);
        
        // Pattern matching functions
        REGISTER_FUNCTION(registry, find, find);
        REGISTER_FUNCTION(registry, match, match);
        REGISTER_FUNCTION(registry, gmatch, gmatch);
        REGISTER_FUNCTION(registry, gsub, gsub);
        
        // Formatting functions
        REGISTER_FUNCTION(registry, format, format);
        REGISTER_FUNCTION(registry, rep, rep);
        
        // Character functions
        REGISTER_FUNCTION(registry, byte, byte_func);
        REGISTER_FUNCTION(registry, char, char_func);
        
        // Utility functions
        REGISTER_FUNCTION(registry, trim, trim);
        REGISTER_FUNCTION(registry, split, split);
        REGISTER_FUNCTION(registry, join, join);
        REGISTER_FUNCTION(registry, startswith, startswith);
        REGISTER_FUNCTION(registry, endswith, endswith);
        REGISTER_FUNCTION(registry, contains, contains);
    }
    
    // Basic string functions implementation
    
    Value StringLib::len(State* state, i32 nargs) {
        if (nargs < 1) {
            throw std::runtime_error("string.len: expected at least 1 argument");
        }
        
        Value strVal = state->get(1);  // Use 1-based indexing
        validateStringArg(strVal, "len", 1);
        
        Str str = TypeConverter::toString(strVal);
        return Value(static_cast<f64>(utf8Length(str)));
    }
    
    Value StringLib::sub(State* state, i32 nargs) {
        if (nargs < 2) {
            throw std::runtime_error("string.sub: expected at least 2 arguments");
        }
        
        Value strVal = state->get(1);   // Use 1-based indexing
        Value startVal = state->get(2); // Use 1-based indexing
        validateStringArg(strVal, "sub", 1);
        validateNumberArg(startVal, "sub", 2);
        
        Str str = TypeConverter::toString(strVal);
        i32 start = static_cast<i32>(TypeConverter::toLuaNumber(startVal));
        i32 end = static_cast<i32>(str.length());
        
        if (nargs >= 3) {
            Value endVal = state->get(3);  // Use 1-based indexing
            validateNumberArg(endVal, "sub", 3);
            end = static_cast<i32>(TypeConverter::toLuaNumber(endVal));
        }
        
        // Handle negative indices (Lua-style)
        usize strLen = str.length();
        if (start < 0) start += static_cast<i32>(strLen) + 1;
        if (end < 0) end += static_cast<i32>(strLen) + 1;
        
        // Clamp to valid range
        start = std::max(1, start);
        end = std::min(static_cast<i32>(strLen), end);
        
        if (start > end) {
            return Value("");
        }
        
        // Convert to 0-based indexing
        start--;
        
        Str result = str.substr(start, end - start);
        return Value(result);
    }
    
    Value StringLib::upper(State* state, i32 nargs) {
        if (nargs < 1) {
            throw std::runtime_error("string.upper: expected 1 argument");
        }
        
        Value strVal = state->get(1);
        validateStringArg(strVal, "upper", 1);
        
        Str str = TypeConverter::toString(strVal);
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        
        return Value(str);
    }
    
    Value StringLib::lower(State* state, i32 nargs) {
        if (nargs < 1) {
            throw std::runtime_error("string.lower: expected 1 argument");
        }
        
        Value strVal = state->get(1);
        validateStringArg(strVal, "lower", 1);
        
        Str str = TypeConverter::toString(strVal);
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        
        return Value(str);
    }
    
    Value StringLib::reverse(State* state, i32 nargs) {
        if (nargs < 1) {
            throw std::runtime_error("string.reverse: expected 1 argument");
        }
        
        Value strVal = state->get(1);
        validateStringArg(strVal, "reverse", 1);
        
        Str str = TypeConverter::toString(strVal);
        std::reverse(str.begin(), str.end());
        
        return Value(str);
    }
    
    // Pattern matching functions implementation
    
    Value StringLib::find(State* state, i32 nargs) {
        if (nargs < 2) {
            throw std::runtime_error("string.find: expected at least 2 arguments");
        }
        
        Value strVal = state->get(1);
        Value patternVal = state->get(2);
        validateStringArg(strVal, "find", 1);
        validateStringArg(patternVal, "find", 2);
        
        Str str = TypeConverter::toString(strVal);
        Str pattern = TypeConverter::toString(patternVal);
        
        i32 init = 1;
        if (nargs >= 3) {
            Value initVal = state->get(3);
            validateNumberArg(initVal, "find", 3);
            init = static_cast<i32>(TypeConverter::toLuaNumber(initVal));
        }
        
        bool plain = false;
        if (nargs >= 4) {
            Value plainVal = state->get(4);
            plain = TypeConverter::toBool(plainVal);
        }
        
        // Handle negative init
        if (init < 0) init += static_cast<i32>(str.length()) + 1;
        init = std::max(1, init);
        
        // Convert to 0-based indexing
        usize startPos = static_cast<usize>(init - 1);
        if (startPos >= str.length()) {
            return Value(); // nil
        }
        
        usize pos;
        if (plain) {
            // Plain text search
            pos = str.find(pattern, startPos);
            if (pos == Str::npos) {
                return Value(); // nil
            }
            
            // Return 1-based indices
            auto table = GCRef<Table>(new Table());
            table->set(Value(1), Value(static_cast<f64>(pos + 1)));
            table->set(Value(2), Value(static_cast<f64>(pos + pattern.length())));
            return Value(table);
        } else {
            // Pattern matching (simplified regex)
            try {
                std::regex regex(pattern);
                std::smatch match;
                Str searchStr = str.substr(startPos);
                
                if (std::regex_search(searchStr, match, regex)) {
                    usize matchPos = startPos + match.position();
                    auto table = GCRef<Table>(new Table());
                    table->set(Value(1), Value(static_cast<f64>(matchPos + 1)));
                    table->set(Value(2), Value(static_cast<f64>(matchPos + match.length())));
                    
                    // Add captured groups
                    for (usize i = 1; i < match.size(); ++i) {
                        table->set(Value(static_cast<f64>(i + 2)), Value(match[i].str()));
                    }
                    
                    return Value(table);
                }
            } catch (const std::regex_error&) {
                throw std::runtime_error("string.find: invalid pattern");
            }
        }
        
        return Value(); // nil
    }
    
    Value StringLib::match(State* state, i32 nargs) {
        if (nargs < 2) {
            throw std::runtime_error("string.match: expected at least 2 arguments");
        }
        
        Value strVal = state->get(1);
        Value patternVal = state->get(2);
        validateStringArg(strVal, "match", 1);
        validateStringArg(patternVal, "match", 2);
        
        Str str = TypeConverter::toString(strVal);
        Str pattern = TypeConverter::toString(patternVal);
        
        i32 init = 1;
        if (nargs >= 3) {
            Value initVal = state->get(3);
            validateNumberArg(initVal, "match", 3);
            init = static_cast<i32>(TypeConverter::toLuaNumber(initVal));
        }
        
        // Handle negative init
        if (init < 0) init += static_cast<i32>(str.length()) + 1;
        init = std::max(1, init);
        
        usize startPos = static_cast<usize>(init - 1);
        if (startPos >= str.length()) {
            return Value(); // nil
        }
        
        try {
            std::regex regex(pattern);
            std::smatch match;
            Str searchStr = str.substr(startPos);
            
            if (std::regex_search(searchStr, match, regex)) {
                if (match.size() == 1) {
                    // No captures, return the whole match
                    return Value(match[0].str());
                } else {
                    // Return captures
                    auto table = GCRef<Table>(new Table());
                    for (usize i = 1; i < match.size(); ++i) {
                        table->set(Value(static_cast<f64>(i)), Value(match[i].str()));
                    }
                    return Value(table);
                }
            }
        } catch (const std::regex_error&) {
            throw std::runtime_error("string.match: invalid pattern");
        }
        
        return Value(); // nil
    }
    
    Value StringLib::gmatch(State* state, i32 nargs) {
        if (nargs < 2) {
            throw std::runtime_error("string.gmatch: expected 2 arguments");
        }
        
        // For now, return a simple implementation
        // In a full implementation, this would return an iterator function
        return Value(); // nil - placeholder
    }
    
    Value StringLib::gsub(State* state, i32 nargs) {
        if (nargs < 3) {
            throw std::runtime_error("string.gsub: expected at least 3 arguments");
        }
        
        Value strVal = state->get(1);
        Value patternVal = state->get(2);
        Value replVal = state->get(3);
        
        validateStringArg(strVal, "gsub", 1);
        validateStringArg(patternVal, "gsub", 2);
        validateStringArg(replVal, "gsub", 3);
        
        Str str = TypeConverter::toString(strVal);
        Str pattern = TypeConverter::toString(patternVal);
        Str replacement = TypeConverter::toString(replVal);
        
        i32 maxRepl = -1;
        if (nargs >= 4) {
            Value maxVal = state->get(4);
            validateNumberArg(maxVal, "gsub", 4);
            maxRepl = static_cast<i32>(TypeConverter::toLuaNumber(maxVal));
        }
        
        try {
            std::regex regex(pattern);
            Str result;
            i32 count = 0;
            
            if (maxRepl < 0) {
                result = std::regex_replace(str, regex, replacement);
                // Count matches for return value
                std::sregex_iterator iter(str.begin(), str.end(), regex);
                std::sregex_iterator end;
                count = static_cast<i32>(std::distance(iter, end));
            } else {
                // Limited replacements
                result = str;
                std::sregex_iterator iter(str.begin(), str.end(), regex);
                std::sregex_iterator end;
                
                for (; iter != end && count < maxRepl; ++iter, ++count) {
                    // This is a simplified implementation
                    // A full implementation would handle this more carefully
                }
                result = std::regex_replace(str, regex, replacement);
            }
            
            auto table = GCRef<Table>(new Table());
            table->set(Value(1), Value(result));
            table->set(Value(2), Value(static_cast<f64>(count)));
            return Value(table);
            
        } catch (const std::regex_error&) {
            throw std::runtime_error("string.gsub: invalid pattern");
        }
    }
    
    // Formatting functions implementation
    
    Value StringLib::format(State* state, i32 nargs) {
        if (nargs < 1) {
            throw std::runtime_error("string.format: expected at least 1 argument");
        }
        
        Value formatVal = state->get(1);
        validateStringArg(formatVal, "format", 1);
        
        Str formatStr = TypeConverter::toString(formatVal);
        Vec<Value> args;
        
        for (i32 i = 2; i <= nargs; ++i) {
            args.push_back(state->get(i));
        }
        
        Str result = StringFormatter::format(formatStr, args);
        return Value(result);
    }
    
    Value StringLib::rep(State* state, i32 nargs) {
        if (nargs < 2) {
            throw std::runtime_error("string.rep: expected 2 arguments");
        }
        
        Value strVal = state->get(1);
        Value countVal = state->get(2);
        validateStringArg(strVal, "rep", 1);
        validateNumberArg(countVal, "rep", 2);
        
        Str str = TypeConverter::toString(strVal);
        i32 count = static_cast<i32>(TypeConverter::toLuaNumber(countVal));
        
        if (count < 0) {
            throw std::runtime_error("string.rep: count must be non-negative");
        }
        
        Str sep = "";
        if (nargs >= 3) {
            Value sepVal = state->get(3);
            validateStringArg(sepVal, "rep", 3);
            sep = TypeConverter::toString(sepVal);
        }
        
        if (count == 0) {
            return Value("");
        }
        
        std::ostringstream oss;
        for (i32 i = 0; i < count; ++i) {
            if (i > 0 && !sep.empty()) {
                oss << sep;
            }
            oss << str;
        }
        
        return Value(oss.str());
    }
    
    // Character functions implementation
    
    Value StringLib::byte_func(State* state, i32 nargs) {
        if (nargs < 1) {
            throw std::runtime_error("string.byte: expected at least 1 argument");
        }
        
        Value strVal = state->get(1);
        validateStringArg(strVal, "byte", 1);
        
        Str str = TypeConverter::toString(strVal);
        if (str.empty()) {
            return Value(); // nil
        }
        
        i32 start = 1;
        if (nargs >= 2) {
            Value startVal = state->get(2);
            validateNumberArg(startVal, "byte", 2);
            start = static_cast<i32>(TypeConverter::toLuaNumber(startVal));
        }
        
        i32 end = start;
        if (nargs >= 3) {
            Value endVal = state->get(3);
            validateNumberArg(endVal, "byte", 3);
            end = static_cast<i32>(TypeConverter::toLuaNumber(endVal));
        }
        
        // Handle negative indices
        usize strLen = str.length();
        if (start < 0) start += static_cast<i32>(strLen) + 1;
        if (end < 0) end += static_cast<i32>(strLen) + 1;
        
        // Clamp to valid range
        start = std::max(1, start);
        end = std::min(static_cast<i32>(strLen), end);
        
        if (start > end || start > static_cast<i32>(strLen)) {
            return Value(); // nil
        }
        
        if (start == end) {
            // Single character
            unsigned char ch = static_cast<unsigned char>(str[start - 1]);
            return Value(static_cast<f64>(ch));
        } else {
            // Multiple characters - return table
            auto table = GCRef<Table>(new Table());
            for (i32 i = start; i <= end && i <= static_cast<i32>(strLen); ++i) {
                unsigned char ch = static_cast<unsigned char>(str[i - 1]);
                table->set(Value(static_cast<f64>(i - start + 1)), Value(static_cast<f64>(ch)));
            }
            return Value(table);
        }
    }
    
    Value StringLib::char_func(State* state, i32 nargs) {
        if (nargs < 1) {
            throw std::runtime_error("string.char: expected at least 1 argument");
        }
        
        std::ostringstream oss;
        
        for (i32 i = 1; i <= nargs; ++i) {
            Value val = state->get(i);
            validateNumberArg(val, "char", i);
            
            i32 code = static_cast<i32>(TypeConverter::toLuaNumber(val));
            if (code < 0 || code > 255) {
                throw std::runtime_error("string.char: character code out of range");
            }
            
            oss << static_cast<char>(code);
        }
        
        return Value(oss.str());
    }
    
    // Utility functions implementation
    
    Value StringLib::trim(State* state, i32 nargs) {
        if (nargs < 1) {
            throw std::runtime_error("string.trim: expected 1 argument");
        }
        
        Value strVal = state->get(1);
        validateStringArg(strVal, "trim", 1);
        
        Str str = TypeConverter::toString(strVal);
        
        // Trim whitespace from both ends
        auto start = str.find_first_not_of(" \t\n\r\f\v");
        if (start == Str::npos) {
            return Value("");
        }
        
        auto end = str.find_last_not_of(" \t\n\r\f\v");
        return Value(str.substr(start, end - start + 1));
    }
    
    Value StringLib::split(State* state, i32 nargs) {
        if (nargs < 2) {
            throw std::runtime_error("string.split: expected 2 arguments");
        }
        
        Value strVal = state->get(1);
        Value sepVal = state->get(2);
        validateStringArg(strVal, "split", 1);
        validateStringArg(sepVal, "split", 2);
        
        Str str = TypeConverter::toString(strVal);
        Str separator = TypeConverter::toString(sepVal);
        
        auto table = GCRef<Table>(new Table());
        
        if (separator.empty()) {
            // Split into individual characters
            for (usize i = 0; i < str.length(); ++i) {
                table->set(Value(static_cast<f64>(i + 1)), Value(Str(1, str[i])));
            }
        } else {
            usize start = 0;
            usize pos = 0;
            i32 index = 1;
            
            while ((pos = str.find(separator, start)) != Str::npos) {
                table->set(Value(static_cast<f64>(index++)), Value(str.substr(start, pos - start)));
                start = pos + separator.length();
            }
            
            // Add the last part
            table->set(Value(static_cast<f64>(index)), Value(str.substr(start)));
        }
        
        return Value(table);
    }
    
    Value StringLib::join(State* state, i32 nargs) {
        if (nargs < 2) {
            throw std::runtime_error("string.join: expected 2 arguments");
        }
        
        Value tableVal = state->get(1);
        Value sepVal = state->get(2);
        
        if (!tableVal.isTable()) {
            throw std::runtime_error("string.join: first argument must be a table");
        }
        validateStringArg(sepVal, "join", 2);
        
        auto table = tableVal.asTable();
        Str separator = TypeConverter::toString(sepVal);
        
        std::ostringstream oss;
        bool first = true;
        
        // Iterate through table (assuming array-like structure)
        for (i32 i = 1; ; ++i) {
            Value key = Value(static_cast<f64>(i));
            Value val = table->get(key);
            
            if (val.isNil()) {
                break;
            }
            
            if (!first) {
                oss << separator;
            }
            first = false;
            
            oss << TypeConverter::toString(val);
        }
        
        return Value(oss.str());
    }
    
    Value StringLib::startswith(State* state, i32 nargs) {
        if (nargs < 2) {
            throw std::runtime_error("string.startswith: expected 2 arguments");
        }
        
        Value strVal = state->get(1);
        Value prefixVal = state->get(2);
        validateStringArg(strVal, "startswith", 1);
        validateStringArg(prefixVal, "startswith", 2);
        
        Str str = TypeConverter::toString(strVal);
        Str prefix = TypeConverter::toString(prefixVal);
        
        bool result = str.length() >= prefix.length() && 
                     str.substr(0, prefix.length()) == prefix;
        
        return Value(result);
    }
    
    Value StringLib::endswith(State* state, i32 nargs) {
        if (nargs < 2) {
            throw std::runtime_error("string.endswith: expected 2 arguments");
        }
        
        Value strVal = state->get(1);
        Value suffixVal = state->get(2);
        validateStringArg(strVal, "endswith", 1);
        validateStringArg(suffixVal, "endswith", 2);
        
        Str str = TypeConverter::toString(strVal);
        Str suffix = TypeConverter::toString(suffixVal);
        
        bool result = str.length() >= suffix.length() && 
                     str.substr(str.length() - suffix.length()) == suffix;
        
        return Value(result);
    }
    
    Value StringLib::contains(State* state, i32 nargs) {
        if (nargs < 2) {
            throw std::runtime_error("string.contains: expected 2 arguments");
        }
        
        Value strVal = state->get(1);
        Value substrVal = state->get(2);
        validateStringArg(strVal, "contains", 1);
        validateStringArg(substrVal, "contains", 2);
        
        Str str = TypeConverter::toString(strVal);
        Str substr = TypeConverter::toString(substrVal);
        
        bool result = str.find(substr) != Str::npos;
        return Value(result);
    }
    
    // Helper functions implementation
    
    void StringLib::validateStringArg(const Value& val, const Str& funcName, i32 argIndex) {
        if (!val.isString() && !val.isNumber()) {
            throw std::runtime_error("string." + funcName + ": argument " + 
                                   std::to_string(argIndex) + " must be a string or number");
        }
    }
    
    void StringLib::validateNumberArg(const Value& val, const Str& funcName, i32 argIndex) {
        if (!val.isNumber()) {
            throw std::runtime_error("string." + funcName + ": argument " + 
                                   std::to_string(argIndex) + " must be a number");
        }
    }
    
    void StringLib::validateRange(i32 start, i32 end, usize strLen, const Str& funcName) {
        if (start < 1 || end < 1 || start > static_cast<i32>(strLen) || end > static_cast<i32>(strLen)) {
            throw std::runtime_error("string." + funcName + ": index out of range");
        }
    }
    
    bool StringLib::isValidUtf8(const Str& str) {
        // Simplified UTF-8 validation
        for (usize i = 0; i < str.length(); ) {
            unsigned char c = str[i];
            if (c < 0x80) {
                i++;
            } else if ((c >> 5) == 0x06) {
                if (i + 1 >= str.length() || (str[i + 1] & 0xC0) != 0x80) return false;
                i += 2;
            } else if ((c >> 4) == 0x0E) {
                if (i + 2 >= str.length() || (str[i + 1] & 0xC0) != 0x80 || (str[i + 2] & 0xC0) != 0x80) return false;
                i += 3;
            } else if ((c >> 3) == 0x1E) {
                if (i + 3 >= str.length() || (str[i + 1] & 0xC0) != 0x80 || (str[i + 2] & 0xC0) != 0x80 || (str[i + 3] & 0xC0) != 0x80) return false;
                i += 4;
            } else {
                return false;
            }
        }
        return true;
    }
    
    usize StringLib::utf8Length(const Str& str) {
        usize length = 0;
        for (usize i = 0; i < str.length(); ) {
            unsigned char c = str[i];
            if (c < 0x80) {
                i++;
            } else if ((c >> 5) == 0x06) {
                i += 2;
            } else if ((c >> 4) == 0x0E) {
                i += 3;
            } else if ((c >> 3) == 0x1E) {
                i += 4;
            } else {
                i++; // Invalid UTF-8, skip byte
            }
            length++;
        }
        return length;
    }
    
    Str StringLib::utf8Substring(const Str& str, usize start, usize length) {
        if (start == 0 || length == 0) {
            return "";
        }
        
        usize byteStart = 0;
        usize charCount = 0;
        
        // Find the byte position for the start character
        for (usize i = 0; i < str.length() && charCount < start - 1; ) {
            unsigned char c = str[i];
            if (c < 0x80) {
                i++;
            } else if ((c >> 5) == 0x06) {
                i += 2;
            } else if ((c >> 4) == 0x0E) {
                i += 3;
            } else if ((c >> 3) == 0x1E) {
                i += 4;
            } else {
                i++; // Invalid UTF-8, skip byte
            }
            charCount++;
            byteStart = i;
        }
        
        if (charCount < start - 1) {
            return ""; // Start position beyond string length
        }
        
        usize byteEnd = byteStart;
        charCount = 0;
        
        // Find the byte position for the end character
        for (usize i = byteStart; i < str.length() && charCount < length; ) {
            unsigned char c = str[i];
            if (c < 0x80) {
                i++;
            } else if ((c >> 5) == 0x06) {
                i += 2;
            } else if ((c >> 4) == 0x0E) {
                i += 3;
            } else if ((c >> 3) == 0x1E) {
                i += 4;
            } else {
                i++; // Invalid UTF-8, skip byte
            }
            charCount++;
            byteEnd = i;
        }
        
        return str.substr(byteStart, byteEnd - byteStart);
    }
    
    Str StringLib::escapePattern(const Str& pattern) {
        Str result;
        result.reserve(pattern.length() * 2); // Reserve space for potential escaping
        
        for (char c : pattern) {
            switch (c) {
                case '^':
                case '$':
                case '(':
                case ')':
                case '%':
                case '.':
                case '[':
                case ']':
                case '*':
                case '+':
                case '-':
                case '?':
                    result += '%';
                    result += c;
                    break;
                default:
                    result += c;
                    break;
            }
        }
        
        return result;
    }
    
    bool StringLib::matchPattern(const Str& text, const Str& pattern, usize& start, usize& end) {
        // Simple pattern matching implementation
        // This is a basic implementation - Lua patterns are more complex
        try {
            std::regex regex(pattern);
            std::smatch match;
            
            if (std::regex_search(text, match, regex)) {
                start = match.position();
                end = start + match.length() - 1;
                return true;
            }
        } catch (const std::regex_error&) {
            // If regex fails, try simple string search
            usize pos = text.find(pattern);
            if (pos != Str::npos) {
                start = pos;
                end = pos + pattern.length() - 1;
                return true;
            }
        }
        
        return false;
    }
    
    Str StringLib::replacePattern(const Str& text, const Str& pattern, const Str& replacement) {
        // Simple pattern replacement implementation
        try {
            std::regex regex(pattern);
            return std::regex_replace(text, regex, replacement);
        } catch (const std::regex_error&) {
            // If regex fails, try simple string replacement
            Str result = text;
            usize pos = 0;
            while ((pos = result.find(pattern, pos)) != Str::npos) {
                result.replace(pos, pattern.length(), replacement);
                pos += replacement.length();
            }
            return result;
        }
    }
    
    // StringFormatter implementation
    
    Str StringFormatter::format(const Str& formatStr, const Vec<Value>& args) {
        std::ostringstream result;
        usize argIndex = 0;
        
        for (usize i = 0; i < formatStr.length(); ++i) {
            if (formatStr[i] == '%') {
                if (i + 1 < formatStr.length() && formatStr[i + 1] == '%') {
                    result << '%';
                    i++; // Skip the second %
                    continue;
                }
                
                // Parse format specifier
                usize specStart = i + 1;
                usize specEnd = specStart;
                
                // Find end of format specifier
                while (specEnd < formatStr.length() && 
                       formatStr[specEnd] != 's' && formatStr[specEnd] != 'd' && 
                       formatStr[specEnd] != 'f' && formatStr[specEnd] != 'x' && 
                       formatStr[specEnd] != 'X' && formatStr[specEnd] != 'o' && 
                       formatStr[specEnd] != 'c' && formatStr[specEnd] != 'e' && 
                       formatStr[specEnd] != 'E' && formatStr[specEnd] != 'g' && 
                       formatStr[specEnd] != 'G') {
                    specEnd++;
                }
                
                if (specEnd >= formatStr.length()) {
                    throw std::runtime_error("string.format: incomplete format specifier");
                }
                
                Str spec = formatStr.substr(specStart, specEnd - specStart + 1);
                FormatSpec formatSpec = parseFormatSpec(spec);
                
                if (argIndex >= args.size()) {
                    throw std::runtime_error("string.format: not enough arguments");
                }
                
                result << formatValue(args[argIndex], formatSpec);
                argIndex++;
                i = specEnd;
            } else {
                result << formatStr[i];
            }
        }
        
        return result.str();
    }
    
    StringFormatter::FormatSpec StringFormatter::parseFormatSpec(const Str& spec) {
        FormatSpec result;
        
        if (spec.empty()) {
            return result;
        }
        
        // Get the format type (last character)
        result.type = spec.back();
        
        // Parse flags, width, and precision (simplified)
        // This is a basic implementation - a full implementation would be more complex
        
        return result;
    }
    
    Str StringFormatter::formatValue(const Value& value, const FormatSpec& spec) {
        switch (spec.type) {
            case 's':
                return formatString(TypeConverter::toString(value), spec);
            case 'd':
                return formatInteger(static_cast<i64>(TypeConverter::toLuaNumber(value)), spec);
            case 'f':
            case 'e':
            case 'E':
            case 'g':
            case 'G':
                return formatNumber(TypeConverter::toLuaNumber(value), spec);
            case 'x':
            case 'X':
            case 'o':
                return formatInteger(static_cast<i64>(TypeConverter::toLuaNumber(value)), spec);
            case 'c': {
                i32 code = static_cast<i32>(TypeConverter::toLuaNumber(value));
                if (code < 0 || code > 255) {
                    throw std::runtime_error("string.format: character code out of range");
                }
                return Str(1, static_cast<char>(code));
            }
            default:
                return TypeConverter::toString(value);
        }
    }
    
    Str StringFormatter::formatString(const Str& str, const FormatSpec& spec) {
        // Basic string formatting
        if (spec.width <= 0) {
            return str;
        }
        
        if (static_cast<i32>(str.length()) >= spec.width) {
            return str;
        }
        
        i32 padding = spec.width - static_cast<i32>(str.length());
        if (spec.leftAlign) {
            return str + Str(padding, spec.fill);
        } else {
            return Str(padding, spec.fill) + str;
        }
    }
    
    Str StringFormatter::formatNumber(f64 num, const FormatSpec& spec) {
        std::ostringstream oss;
        
        if (spec.precision >= 0) {
            oss << std::fixed << std::setprecision(spec.precision);
        }
        
        if (spec.width > 0) {
            oss << std::setw(spec.width);
        }
        
        if (spec.leftAlign) {
            oss << std::left;
        }
        
        if (spec.padZero) {
            oss << std::setfill('0');
        }
        
        switch (spec.type) {
            case 'e':
                oss << std::scientific << std::nouppercase;
                break;
            case 'E':
                oss << std::scientific << std::uppercase;
                break;
            case 'g':
                oss << std::defaultfloat << std::nouppercase;
                break;
            case 'G':
                oss << std::defaultfloat << std::uppercase;
                break;
            default:
                oss << std::fixed;
                break;
        }
        
        oss << num;
        return oss.str();
    }
    
    Str StringFormatter::formatInteger(i64 num, const FormatSpec& spec) {
        std::ostringstream oss;
        
        if (spec.width > 0) {
            oss << std::setw(spec.width);
        }
        
        if (spec.leftAlign) {
            oss << std::left;
        }
        
        if (spec.padZero) {
            oss << std::setfill('0');
        }
        
        switch (spec.type) {
            case 'x':
                oss << std::hex << std::nouppercase;
                break;
            case 'X':
                oss << std::hex << std::uppercase;
                break;
            case 'o':
                oss << std::oct;
                break;
            default:
                oss << std::dec;
                break;
        }
        
        oss << num;
        return oss.str();
    }
}