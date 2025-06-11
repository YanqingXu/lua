#include "string_lib.hpp"
#include "lib_common.hpp"
#include "lib_utils.hpp"
#include "../vm/state.hpp"
#include "../vm/table.hpp"
#include <algorithm>
#include <regex>
#include <sstream>
#include <iomanip>

namespace Lua {

    void StringLib::registerModule(State* state) {
        // Create string table
        auto stringTable = GCRef<Table>(new Table());
        
        // Register string functions
        registerFunction(state, Value(stringTable), "len", len);
        registerFunction(state, Value(stringTable), "sub", sub);
        registerFunction(state, Value(stringTable), "upper", upper);
        registerFunction(state, Value(stringTable), "lower", lower);
        registerFunction(state, Value(stringTable), "find", find);
        registerFunction(state, Value(stringTable), "gsub", gsub);
        registerFunction(state, Value(stringTable), "match", match);
        registerFunction(state, Value(stringTable), "gmatch", gmatch);
        registerFunction(state, Value(stringTable), "rep", rep);
        registerFunction(state, Value(stringTable), "reverse", reverse);
        registerFunction(state, Value(stringTable), "format", format);
        registerFunction(state, Value(stringTable), "char", char_func);
        registerFunction(state, Value(stringTable), "byte", byte_func);
        
        // Set string table as global
        state->setGlobal("string", Value(stringTable));
        
        // Mark as loaded
        setLoaded(true);
    }
    
    Value StringLib::len(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(1)) {
            return Value();
        }
        
        auto val = checker.getValue();
        if (!val || !val->isString()) {
            LibUtils::Error::throwTypeError(state, 1, "string", 
                val ? LibUtils::Convert::typeToString(val->type()) : "no value");
            return Value();
        }
        
        return Value(static_cast<double>(val->asString().length()));
    }
    
    Value StringLib::sub(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(2)) {
            return Value();
        }
        
        auto str = checker.getValue();
        if (!str || !str->isString()) {
            LibUtils::Error::throwTypeError(state, 1, "string", 
                str ? LibUtils::Convert::typeToString(str->type()) : "no value");
            return Value();
        }
        
        auto startVal = checker.getValue();
        if (!startVal || !startVal->isNumber()) {
            LibUtils::Error::throwTypeError(state, 2, "number", 
                startVal ? LibUtils::Convert::typeToString(startVal->type()) : "no value");
            return Value();
        }
        
        Str s = str->asString();
        int start = static_cast<int>(startVal->asNumber());
        int end = static_cast<int>(s.length());
        
        if (nargs >= 3) {
            Value endVal = state->get(3);
            if (endVal.isNumber()) {
                end = static_cast<int>(endVal.asNumber());
            }
        }
        
        // Handle negative indices (Lua-style)
        if (start < 0) start = static_cast<int>(s.length()) + start + 1;
        if (end < 0) end = static_cast<int>(s.length()) + end + 1;
        
        // Convert to 0-based indexing
        start = std::max(1, start) - 1;
        end = std::min(static_cast<int>(s.length()), end);
        
        if (start >= end || start >= static_cast<int>(s.length())) {
            return Value("");
        }
        
        return Value(s.substr(start, end - start));
    }
    
    Value StringLib::upper(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(1)) {
            return Value();
        }
        
        auto val = checker.getValue();
        if (!val || !val->isString()) {
            LibUtils::Error::throwTypeError(state, 1, "string", 
                val ? LibUtils::Convert::typeToString(val->type()) : "no value");
            return Value();
        }
        
        return Value(LibUtils::String::toUpper(val->asString()));
    }
    
    Value StringLib::lower(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(1)) {
            return Value();
        }
        
        auto val = checker.getValue();
        if (!val || !val->isString()) {
            LibUtils::Error::throwTypeError(state, 1, "string", 
                val ? LibUtils::Convert::typeToString(val->type()) : "no value");
            return Value();
        }
        
        return Value(LibUtils::String::toLower(val->asString()));
    }
    
    Value StringLib::find(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(2)) {
            return Value(nullptr);
        }
        
        auto str = checker.getValue();
        if (!str || !str->isString()) {
            LibUtils::Error::throwTypeError(state, 1, "string", 
                str ? LibUtils::Convert::typeToString(str->type()) : "no value");
            return Value(nullptr);
        }
        
        auto pattern = checker.getValue();
        if (!pattern || !pattern->isString()) {
            LibUtils::Error::throwTypeError(state, 2, "string", 
                pattern ? LibUtils::Convert::typeToString(pattern->type()) : "no value");
            return Value(nullptr);
        }
        
        Str s = str->asString();
        Str p = pattern->asString();
        
        int init = 1;
        bool plain = false;
        
        if (nargs >= 3) {
            Value initVal = state->get(3);
            if (initVal.isNumber()) {
                init = static_cast<int>(initVal.asNumber());
            }
        }
        
        if (nargs >= 4) {
            Value plainVal = state->get(4);
            if (plainVal.isBoolean()) {
                plain = plainVal.asBoolean();
            }
        }
        
        // Convert to 0-based indexing
        init = std::max(1, init) - 1;
        
        if (init >= static_cast<int>(s.length())) {
            return Value(nullptr);
        }
        
        size_t pos;
        if (plain) {
            // Plain text search
            pos = s.find(p, init);
        } else {
            // Pattern matching (simplified)
            size_t start, end;
            if (matchPattern(s.substr(init), p, start, end)) {
                pos = init + start;
            } else {
                pos = Str::npos;
            }
        }
        
        if (pos == Str::npos) {
            return Value(nullptr);
        }
        
        // Return 1-based indices
        return Value(static_cast<double>(pos + 1));
    }
    
    Value StringLib::gsub(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(3)) {
            return Value(nullptr);
        }
        
        auto str = checker.getValue();
        if (!str || !str->isString()) {
            LibUtils::Error::throwTypeError(state, 1, "string", 
                str ? LibUtils::Convert::typeToString(str->type()) : "no value");
            return Value(nullptr);
        }
        
        auto pattern = checker.getValue();
        if (!pattern || !pattern->isString()) {
            LibUtils::Error::throwTypeError(state, 2, "string", 
                pattern ? LibUtils::Convert::typeToString(pattern->type()) : "no value");
            return Value(nullptr);
        }
        
        auto replacement = checker.getValue();
        if (!replacement || !replacement->isString()) {
            LibUtils::Error::throwTypeError(state, 3, "string", 
                replacement ? LibUtils::Convert::typeToString(replacement->type()) : "no value");
            return Value(nullptr);
        }
        
        Str s = str->asString();
        Str p = pattern->asString();
        Str r = replacement->asString();
        
        int n = -1; // Replace all by default
        if (nargs >= 4) {
            Value nVal = state->get(4);
            if (nVal.isNumber()) {
                n = static_cast<int>(nVal.asNumber());
            }
        }
        
        Str result = replacePattern(s, p, r);
        return Value(result);
    }
    
    Value StringLib::match(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(2)) {
            return Value(nullptr);
        }
        
        auto str = checker.getValue();
        if (!str || !str->isString()) {
            LibUtils::Error::throwTypeError(state, 1, "string", 
                str ? LibUtils::Convert::typeToString(str->type()) : "no value");
            return Value(nullptr);
        }
        
        auto pattern = checker.getValue();
        if (!pattern || !pattern->isString()) {
            LibUtils::Error::throwTypeError(state, 2, "string", 
                pattern ? LibUtils::Convert::typeToString(pattern->type()) : "no value");
            return Value(nullptr);
        }
        
        Str s = str->asString();
        Str p = pattern->asString();
        
        int init = 1;
        if (nargs >= 3) {
            Value initVal = state->get(3);
            if (initVal.isNumber()) {
                init = static_cast<int>(initVal.asNumber());
            }
        }
        
        // Convert to 0-based indexing
        init = std::max(1, init) - 1;
        
        if (init >= static_cast<int>(s.length())) {
            return Value(nullptr);
        }
        
        size_t start, end;
        if (matchPattern(s.substr(init), p, start, end)) {
            return Value(s.substr(init + start, end - start));
        }
        
        return Value(nullptr);
    }
    
    Value StringLib::gmatch(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(2)) {
            return Value(nullptr);
        }
        
        auto str = checker.getValue();
        if (!str || !str->isString()) {
            LibUtils::Error::throwTypeError(state, 1, "string", 
                str ? LibUtils::Convert::typeToString(str->type()) : "no value");
            return Value(nullptr);
        }
        
        auto pattern = checker.getValue();
        if (!pattern || !pattern->isString()) {
            LibUtils::Error::throwTypeError(state, 2, "string", 
                pattern ? LibUtils::Convert::typeToString(pattern->type()) : "no value");
            return Value(nullptr);
        }
        
        // This would return an iterator function in a real implementation
        // For now, return nil as a placeholder
        return Value(nullptr);
    }
    
    Value StringLib::rep(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(2)) {
            return Value(nullptr);
        }
        
        auto str = checker.getValue();
        if (!str || !str->isString()) {
            LibUtils::Error::throwTypeError(state, 1, "string", 
                str ? LibUtils::Convert::typeToString(str->type()) : "no value");
            return Value(nullptr);
        }
        
        auto count = checker.getValue();
        if (!count || !count->isNumber()) {
            LibUtils::Error::throwTypeError(state, 2, "number", 
                count ? LibUtils::Convert::typeToString(count->type()) : "no value");
            return Value(nullptr);
        }
        
        Str s = str->asString();
        int n = static_cast<int>(count->asNumber());
        
        if (n <= 0) {
            return Value("");
        }
        
        Str sep = "";
        if (nargs >= 3) {
            Value sepVal = state->get(3);
            if (sepVal.isString()) {
                sep = sepVal.asString();
            }
        }
        
        std::ostringstream oss;
        for (int i = 0; i < n; ++i) {
            if (i > 0 && !sep.empty()) {
                oss << sep;
            }
            oss << s;
        }
        
        return Value(oss.str());
    }
    
    Value StringLib::reverse(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(1)) {
            return Value();
        }
        
        auto val = checker.getValue();
        if (!val || !val->isString()) {
            LibUtils::Error::throwTypeError(state, 1, "string", 
                val ? LibUtils::Convert::typeToString(val->type()) : "no value");
            return Value();
        }
        
        Str s = val->asString();
        std::reverse(s.begin(), s.end());
        return Value(s);
    }
    
    Value StringLib::format(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(1)) {
            return Value(nullptr);
        }
        
        auto format = checker.getValue();
        if (!format || !format->isString()) {
            LibUtils::Error::throwTypeError(state, 1, "string", 
                format ? LibUtils::Convert::typeToString(format->type()) : "no value");
            return Value(nullptr);
        }
        
        Vec<Value> args;
        for (int i = 2; i <= nargs; ++i) {
            args.push_back(state->get(i));
        }
        
        Str result = formatString(format->asString(), args);
        return Value(result);
    }
    
    Value StringLib::char_func(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        std::ostringstream oss;
        for (int i = 1; i <= nargs; ++i) {
            Value val = state->get(i);
            if (!val.isNumber()) {
                LibUtils::Error::throwTypeError(state, i, "number", 
                    LibUtils::Convert::typeToString(val.type()));
                return Value();
            }
            
            int code = static_cast<int>(val.asNumber());
            if (code < 0 || code > 255) {
                LibUtils::Error::throwArgError(state, i, "value out of range");
                return Value();
            }
            
            oss << static_cast<char>(code);
        }
        
        return Value(oss.str());
    }
    
    Value StringLib::byte_func(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(1)) {
            return Value(nullptr);
        }
        
        auto str = checker.getValue();
        if (!str || !str->isString()) {
            LibUtils::Error::throwTypeError(state, 1, "string", 
                str ? LibUtils::Convert::typeToString(str->type()) : "no value");
            return Value(nullptr);
        }
        
        Str s = str->asString();
        int start = 1;
        int end = 1;
        
        if (nargs >= 2) {
            Value startVal = state->get(2);
            if (startVal.isNumber()) {
                start = static_cast<int>(startVal.asNumber());
            }
        }
        
        if (nargs >= 3) {
            Value endVal = state->get(3);
            if (endVal.isNumber()) {
                end = static_cast<int>(endVal.asNumber());
            }
        } else {
            end = start;
        }
        
        // Handle negative indices
        if (start < 0) start = static_cast<int>(s.length()) + start + 1;
        if (end < 0) end = static_cast<int>(s.length()) + end + 1;
        
        // Convert to 0-based indexing
        start = std::max(1, start) - 1;
        end = std::min(static_cast<int>(s.length()), end) - 1;
        
        if (start > end || start >= static_cast<int>(s.length())) {
            return Value(nullptr);
        }
        
        if (start == end) {
            // Return single byte value
            return Value(static_cast<double>(static_cast<unsigned char>(s[start])));
        } else {
            // Return multiple byte values (would need multiple return values)
            // For now, return the first byte
            return Value(static_cast<double>(static_cast<unsigned char>(s[start])));
        }
    }
    
    // Helper function implementations
    bool StringLib::matchPattern(const Str& str, const Str& pattern, size_t& start, size_t& end) {
        // This is a very simplified pattern matching implementation
        // A real implementation would handle Lua patterns properly
        try {
            std::regex regex(pattern);
            std::smatch match;
            if (std::regex_search(str, match, regex)) {
                start = match.position();
                end = start + match.length();
                return true;
            }
        } catch (...) {
            // Fallback to plain text search
            size_t pos = str.find(pattern);
            if (pos != Str::npos) {
                start = pos;
                end = pos + pattern.length();
                return true;
            }
        }
        return false;
    }
    
    Str StringLib::replacePattern(const Str& str, const Str& pattern, const Str& replacement) {
        // This is a simplified implementation
        try {
            std::regex regex(pattern);
            return std::regex_replace(str, regex, replacement);
        } catch (...) {
            // Fallback to simple string replacement
            Str result = str;
            size_t pos = 0;
            while ((pos = result.find(pattern, pos)) != Str::npos) {
                result.replace(pos, pattern.length(), replacement);
                pos += replacement.length();
            }
            return result;
        }
    }
    
    Vec<Str> StringLib::findAllMatches(const Str& str, const Str& pattern) {
        Vec<Str> matches;
        // This is a simplified implementation
        try {
            std::regex regex(pattern);
            std::sregex_iterator iter(str.begin(), str.end(), regex);
            std::sregex_iterator end;
            
            for (; iter != end; ++iter) {
                matches.push_back(iter->str());
            }
        } catch (...) {
            // Fallback implementation
        }
        return matches;
    }
    
    Str StringLib::formatString(const Str& format, const Vec<Value>& args) {
        // This is a very simplified printf-style formatting implementation
        std::ostringstream oss;
        size_t argIndex = 0;
        
        for (size_t i = 0; i < format.length(); ++i) {
            if (format[i] == '%' && i + 1 < format.length()) {
                char specifier = format[i + 1];
                if (specifier == '%') {
                    oss << '%';
                } else if (argIndex < args.size()) {
                    const Value& arg = args[argIndex++];
                    switch (specifier) {
                        case 'd':
                        case 'i':
                            if (arg.isNumber()) {
                                oss << static_cast<long long>(arg.asNumber());
                            } else {
                                oss << "0";
                            }
                            break;
                        case 'f':
                            if (arg.isNumber()) {
                                oss << std::fixed << std::setprecision(6) << arg.asNumber();
                            } else {
                                oss << "0.000000";
                            }
                            break;
                        case 's':
                            oss << LibUtils::Convert::toString(arg);
                            break;
                        default:
                            oss << '%' << specifier;
                            break;
                    }
                } else {
                    oss << '%' << specifier;
                }
                ++i; // Skip the specifier
            } else {
                oss << format[i];
            }
        }
        
        return oss.str();
    }
    
    Str StringLib::escapePattern(const Str& pattern) {
        Str escaped;
        for (char c : pattern) {
            if (c == '.' || c == '^' || c == '$' || c == '*' || c == '+' || 
                c == '?' || c == '(' || c == ')' || c == '[' || c == ']' || 
                c == '{' || c == '}' || c == '\\' || c == '|') {
                escaped += '\\';
            }
            escaped += c;
        }
        return escaped;
    }
    
    // Legacy registration function
    void registerStringLib(State* state) {
        StringLib stringLib;
        stringLib.registerModule(state);
    }

} // namespace Lua
