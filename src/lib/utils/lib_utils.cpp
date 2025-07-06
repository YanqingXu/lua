#include "lib_utils.hpp"
#include "error_handling.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <optional>

namespace Lua {
    namespace Lib {
        
        // ArgUtils和ErrorUtils实现已移除
        // 相关功能已整合到其他模块中

        
        // 类型转换工具实现
        
        Str toString(const Value& value, StrView context) {
            if (value.isString()) {
                return value.asString();
            } else if (value.isNumber()) {
                return std::to_string(value.asNumber());
            } else if (value.isBoolean()) {
                return value.asBoolean() ? "true" : "false";
            } else if (value.isNil()) {
                return "nil";
            } else {
                throw LibException(
                    LibErrorCode::TypeMismatch,
                    Str(context) + ": cannot convert " + getTypeName(value) + " to string"
                );
            }
        }
        
        bool toBool(const Value& value, StrView context) {
            if (value.isBoolean()) {
                return value.asBoolean();
            } else if (value.isNil()) {
                return false;
            } else {
                // Lua中，除了nil和false，其他都是true
                return true;
            }
        }
        
        Str getTypeName(const Value& value) {
            if (value.isNumber()) return "number";
            if (value.isString()) return "string";
            if (value.isBoolean()) return "boolean";
            if (value.isNil()) return "nil";
            if (value.isTable()) return "table";
            if (value.isFunction()) return "function";
            return "unknown";
        }
        
        // 参数检查函数实现
        void checkArgCount(State* state, i32 expected, StrView funcName) {
            i32 nargs = state->getTop();
            if (nargs != expected) {
                std::ostringstream oss;
                oss << funcName << ": expected " << expected << " arguments, got " << nargs;
                error(state, oss.str());
            }
        }

        void checkArgCount(State* state, i32 min, i32 max, StrView funcName) {
            i32 nargs = state->getTop();
            if (nargs < min || (max != -1 && nargs > max)) {
                std::ostringstream oss;
                if (max == -1) {
                    oss << funcName << ": expected at least " << min << " arguments, got " << nargs;
                } else {
                    oss << funcName << ": expected " << min << "-" << max << " arguments, got " << nargs;
                }
                error(state, oss.str());
            }
        }

        Value checkNumber(State* state, i32 index, StrView funcName) {
            if (index > state->getTop()) {
                std::ostringstream oss;
                oss << funcName << ": argument " << index << " missing";
                error(state, oss.str());
            }
            
            Value val = state->get(index);
            if (!val.isNumber()) {
                typeError(state, index, "number", funcName);
            }
            return val;
        }

        Value checkString(State* state, i32 index, StrView funcName) {
            if (index > state->getTop()) {
                std::ostringstream oss;
                oss << funcName << ": argument " << index << " missing";
                error(state, oss.str());
            }
            
            Value val = state->get(index);
            if (!val.isString()) {
                typeError(state, index, "string", funcName);
            }
            return val;
        }

        Value checkTable(State* state, i32 index, StrView funcName) {
            if (index > state->getTop()) {
                std::ostringstream oss;
                oss << funcName << ": argument " << index << " missing";
                error(state, oss.str());
            }
            
            Value val = state->get(index);
            if (!val.isTable()) {
                typeError(state, index, "table", funcName);
            }
            return val;
        }

        Value checkFunction(State* state, i32 index, StrView funcName) {
            if (index > state->getTop()) {
                std::ostringstream oss;
                oss << funcName << ": argument " << index << " missing";
                error(state, oss.str());
            }
            
            Value val = state->get(index);
            if (!val.isFunction()) {
                typeError(state, index, "function", funcName);
            }
            return val;
        }

        std::optional<Value> optNumber(State* state, i32 index, f64 defaultValue) {
            if (index > state->getTop()) {
                return Value(defaultValue);
            }
            
            Value val = state->get(index);
            if (val.isNil()) {
                return Value(defaultValue);
            }
            
            if (!val.isNumber()) {
                return std::nullopt; // Type error
            }
            
            return val;
        }

        std::optional<Value> optString(State* state, i32 index, StrView defaultValue) {
            if (index > state->getTop()) {
                return Value(Str(defaultValue));
            }
            
            Value val = state->get(index);
            if (val.isNil()) {
                return Value(Str(defaultValue));
            }
            
            if (!val.isString()) {
                return std::nullopt; // Type error
            }
            
            return val;
        }



        void typeError(State* state, i32 index, StrView expected, StrView funcName) {
            Value val = state->get(index);
            std::ostringstream oss;
            oss << funcName << ": argument " << index << " expected " << expected 
                << ", got " << getTypeName(val);
            error(state, oss.str());
        }

        // 错误处理函数实现
        [[noreturn]] void error(State* state, StrView message) {
            // In a real implementation, this would use Lua's error mechanism
            throw std::runtime_error(Str(message));
        }

        [[noreturn]] void argError(State* state, i32 index, StrView message) {
            std::ostringstream oss;
            oss << "bad argument #" << index << " (" << message << ")";
            error(state, oss.str());
        }

        [[noreturn]] void typeError(State* state, i32 index, StrView expected) {
            Value val = state->get(index);
            std::ostringstream oss;
            oss << "bad argument #" << index << " (" << expected 
                << " expected, got " << getTypeName(val) << ")";
            error(state, oss.str());
        }
         
         // 类型别名转换器实现
        i8 toI8(const Value& value, StrView context) {
            return NumericConverter<i8>::convert(value, context);
        }
        
        i16 toI16(const Value& value, StrView context) {
            return NumericConverter<i16>::convert(value, context);
        }
        
        i32 toI32(const Value& value, StrView context) {
            return NumericConverter<i32>::convert(value, context);
        }
        
        i64 toI64(const Value& value, StrView context) {
            return NumericConverter<i64>::convert(value, context);
        }
        
        u8 toU8(const Value& value, StrView context) {
            return NumericConverter<u8>::convert(value, context);
        }
        
        u16 toU16(const Value& value, StrView context) {
            return NumericConverter<u16>::convert(value, context);
        }
        
        u32 toU32(const Value& value, StrView context) {
            return NumericConverter<u32>::convert(value, context);
        }
        
        u64 toU64(const Value& value, StrView context) {
            return NumericConverter<u64>::convert(value, context);
        }
        
        f32 toF32(const Value& value, StrView context) {
            return NumericConverter<f32>::convert(value, context);
        }
        
        f64 toF64(const Value& value, StrView context) {
            return NumericConverter<f64>::convert(value, context);
        }
        
        usize toUsize(const Value& value, StrView context) {
            return NumericConverter<usize>::convert(value, context);
        }
        
        // Lua特定类型转换实现
        LuaInteger toLuaInteger(const Value& value, StrView context) {
            return NumericConverter<LuaInteger>::convert(value, context);
        }
        
        LuaNumber toLuaNumber(const Value& value, StrView context) {
            return NumericConverter<LuaNumber>::convert(value, context);
        }
        
        LuaBoolean toLuaBoolean(const Value& value, StrView context) {
            return toBool(value, context);
        }
        
        // 模板特化实现
        template<>
        Vec<i32> toVector<i32>(State* state, i32 tableIndex, StrView context) {
            auto table = state->get(tableIndex);
            if (!table.isTable()) {
                throw LibException(
                    LibErrorCode::TypeMismatch,
                    Str(context) + ": expected table, got " + Str(getTypeName(table))
                );
            }
            
            Vec<i32> result;
            // TODO: 实现表遍历逻辑
            // 在实际实现中，这里会遍历Lua表
            return result;
        }
        
        template<>
        HashMap<Str, Str> toHashMap<Str>(State* state, i32 tableIndex, StrView context) {
            auto table = state->get(tableIndex);
            if (!table.isTable()) {
                throw LibException(
                    LibErrorCode::TypeMismatch,
                    Str(context) + ": expected table, got " + Str(getTypeName(table))
                );
            }
            
            HashMap<Str, Str> result;
            // TODO: 实现表遍历逻辑
            // 在实际实现中，这里会遍历Lua表的键值对
            return result;
        }
        
    } // namespace Lib
} // namespace Lua