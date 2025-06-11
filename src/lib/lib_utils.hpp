#pragma once

#include "lib_common.hpp"
#include "../vm/state.hpp"
#include "../vm/value.hpp"
#include "../vm/table.hpp"
#include <string>
#include <optional>
#include <vector>
#include <algorithm>
#include <cctype>

namespace Lua {
    namespace LibUtils {
        
        // Argument checking utilities
        class ArgChecker {
        public:
            ArgChecker(State* state, int nargs) : state_(state), nargs_(nargs), current_arg_(1) {}
            
            // Check minimum number of arguments
            bool checkMinArgs(int min_args) {
                if (nargs_ < min_args) {
                    throw std::runtime_error(LibErrors::TOO_FEW_ARGS);
                    return false;
                }
                return true;
            }
            
            // Check exact number of arguments
            bool checkExactArgs(int exact_args) {
                if (nargs_ != exact_args) {
                    throw std::runtime_error(nargs_ < exact_args ? LibErrors::TOO_FEW_ARGS : LibErrors::TOO_MANY_ARGS);
                    return false;
                }
                return true;
            }
            
            // Get next argument as number
            std::optional<LuaNumber> getNumber() {
                if (current_arg_ > nargs_) return std::nullopt;
                
                Value val = state_->get(current_arg_++);
                if (val.isNumber()) {
                    return val.asNumber();
                }
                
                throw std::runtime_error(LibErrors::WRONG_TYPE);
                return std::nullopt;
            }
            
            // Get next argument as string
            std::optional<std::string> getString() {
                if (current_arg_ > nargs_) return std::nullopt;
                
                Value val = state_->get(current_arg_++);
                if (val.isString()) {
                    return val.asString();
                }
                
                throw std::runtime_error(LibErrors::WRONG_TYPE);
                return std::nullopt;
            }
            
            // Get next argument as table
            std::optional<Table*> getTable() {
                if (current_arg_ > nargs_) return std::nullopt;
                
                Value val = state_->get(current_arg_++);
                if (val.isTable()) {
                    return val.asTable().get();
                }
                
                throw std::runtime_error(LibErrors::WRONG_TYPE);
                return std::nullopt;
            }
            
            // Get next argument as function
            std::optional<Function*> getFunction() {
                if (current_arg_ > nargs_) return std::nullopt;
                
                Value val = state_->get(current_arg_++);
                if (val.isFunction()) {
                    return val.asFunction().get();
                }
                
                throw std::runtime_error(LibErrors::WRONG_TYPE);
                return std::nullopt;
            }
            
            // Get next argument as any value
            std::optional<Value> getValue() {
                if (current_arg_ > nargs_) return std::nullopt;
                return state_->get(current_arg_++);
            }
            
            // Check if more arguments available
            bool hasMore() const {
                return current_arg_ <= nargs_;
            }
            
            // Get current argument index
            int getCurrentIndex() const {
                return current_arg_;
            }
            
            // Reset to specific argument
            void resetTo(int index) {
                current_arg_ = index;
            }
            
        private:
            State* state_;
            int nargs_;
            int current_arg_;
        };
        
        // Type conversion utilities
        namespace Convert {
            
            // Convert ValueType to string
            inline std::string typeToString(ValueType type) {
                switch (type) {
                    case ValueType::Nil: return "nil";
                    case ValueType::Boolean: return "boolean";
                    case ValueType::Number: return "number";
                    case ValueType::String: return "string";
                    case ValueType::Table: return "table";
                    case ValueType::Function: return "function";
                    default: return "unknown";
                }
            }
            
            // Convert value to number with optional default
            inline LuaNumber toNumber(const Value& val, LuaNumber default_val = 0.0) {
                if (val.isNumber()) {
                    return val.asNumber();
                } else if (val.isString()) {
                    try {
                        return std::stod(val.asString());
                    } catch (...) {
                        return default_val;
                    }
                }
                return default_val;
            }
            
            // Convert value to integer
            inline LuaInteger toInteger(const Value& val, LuaInteger default_val = 0) {
                LuaNumber num = toNumber(val, static_cast<LuaNumber>(default_val));
                return static_cast<LuaInteger>(num);
            }
            
            // Convert value to string
            inline std::string toString(const Value& val) {
                if (val.isString()) {
                    return val.asString();
                } else if (val.isNumber()) {
                    LuaNumber num = val.asNumber();
                    // Check if it's an integer
                    if (num == static_cast<long long>(num)) {
                        return std::to_string(static_cast<long long>(num));
                    } else {
                        return std::to_string(num);
                    }
                } else if (val.isBoolean()) {
                    return val.asBoolean() ? "true" : "false";
                } else if (val.isNil()) {
                    return "nil";
                }
                return "<" + Convert::typeToString(val.type()) + ">";
            }
            
            // Raw equality comparison without metamethods
            inline bool rawEqual(const Value& val1, const Value& val2) {
                if (val1.type() != val2.type()) {
                    return false;
                }
                
                switch (val1.type()) {
                    case ValueType::Nil:
                        return true;
                    case ValueType::Boolean:
                        return val1.asBoolean() == val2.asBoolean();
                    case ValueType::Number:
                        return val1.asNumber() == val2.asNumber();
                    case ValueType::String:
                        return val1.asString() == val2.asString();
                    case ValueType::Table:
                        return val1.asTable() == val2.asTable();
                    case ValueType::Function:
                        return val1.asFunction() == val2.asFunction();
                    default:
                        return false;
                }
            }
            
            // Convert value to boolean
            inline bool toBoolean(const Value& val) {
                if (val.isBoolean()) {
                    return val.asBoolean();
                } else if (val.isNil()) {
                    return false;
                }
                return true; // Everything else is truthy in Lua
            }
        }
        
        // String utilities
        namespace String {
            
            // Check if string represents a valid number
            inline bool isNumeric(const std::string& str) {
                try {
                    static_cast<void>(std::stod(str));
                    return true;
                } catch (...) {
                    return false;
                }
            }
            
            // Trim whitespace from string
            inline std::string trim(const std::string& str) {
                size_t start = str.find_first_not_of(" \t\n\r");
                if (start == std::string::npos) return "";
                
                size_t end = str.find_last_not_of(" \t\n\r");
                return str.substr(start, end - start + 1);
            }
            
            // Split string by delimiter
            inline std::vector<std::string> split(const std::string& str, char delimiter) {
                std::vector<std::string> result;
                std::string current;
                
                for (char c : str) {
                    if (c == delimiter) {
                        result.push_back(current);
                        current.clear();
                    } else {
                        current += c;
                    }
                }
                
                if (!current.empty()) {
                    result.push_back(current);
                }
                
                return result;
            }
            
            // Convert string to uppercase
            inline std::string toUpper(const std::string& str) {
                std::string result = str;
                std::transform(result.begin(), result.end(), result.begin(), ::toupper);
                return result;
            }
            
            // Convert string to lowercase
            inline std::string toLower(const std::string& str) {
                std::string result = str;
                std::transform(result.begin(), result.end(), result.begin(), ::tolower);
                return result;
            }
        }
        
        // Table utilities
        namespace TableUtils {
            
            // Get table length (array part)
            inline size_t getArrayLength(Table* table) {
                if (!table) return 0;
                
                size_t length = 0;
                for (size_t i = 1; ; ++i) {
                    Value val = table->get(Value(static_cast<LuaNumber>(i)));
                    if (val.isNil()) break;
                    length = i;
                }
                return length;
            }
            
            // Check if table is an array (only numeric keys starting from 1)
            inline bool isArray(Table* table) {
                if (!table) return false;
                
                size_t length = getArrayLength(table);
                if (length == 0) return true;
                
                // Check if all keys from 1 to length exist
                for (size_t i = 1; i <= length; ++i) {
                    Value val = table->get(Value(static_cast<LuaNumber>(i)));
                    if (val.isNil()) return false;
                }
                
                return true;
            }
            
            // Convert table to vector (array part only)
            inline std::vector<Value> toVector(Table* table) {
                std::vector<Value> result;
                if (!table) return result;
                
                size_t length = getArrayLength(table);
                result.reserve(length);
                
                for (size_t i = 1; i <= length; ++i) {
                    result.push_back(table->get(Value(static_cast<LuaNumber>(i))));
                }
                
                return result;
            }
        }
        
        // Error handling utilities
        namespace Error {
            
            // Create formatted error message
            inline std::string format(const std::string& message, const std::string& detail = "") {
                if (detail.empty()) {
                    return message;
                }
                return message + ": " + detail;
            }
            
            // Throw argument error
            inline void argumentError(State* state, int arg_index, const std::string& expected_type) {
                std::string msg = "bad argument #" + std::to_string(arg_index) + " (" + expected_type + " expected)";
                throw std::runtime_error(msg);
            }
            
            // Throw range error
            inline void rangeError(State* state, const std::string& message) {
                throw std::runtime_error(format(LibErrors::OUT_OF_RANGE, message));
            }
            
            // Function declarations for lib_utils.cpp implementations
            void throwError(State* state, const std::string& message, int level = 1);
            void throwTypeError(State* state, int argIndex, const std::string& expectedType, const std::string& actualType);
            void throwArgError(State* state, int argIndex, const std::string& message);
            std::string formatError(const std::string& functionName, const std::string& message);
        }
    }
}