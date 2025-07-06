#pragma once

#include "../core/lib_define.hpp"
#include "error_handling.hpp"
#include <any>
#include <optional>
#include <limits>
#include <type_traits>

namespace Lua {
    namespace Lib {
        // ArgUtils和ErrorUtils命名空间已移除
        // 相关功能已整合到其他模块中

        /**
         * 类型转换工具类
         * 使用types.hpp中的简化类型系统
         */
        // 前向声明（模板中需要）
        Str getTypeName(const Value& value);
        
        /**
         * 安全的数值转换
         */
        template<typename T>
        struct NumericConverter {
            static_assert(std::is_arithmetic_v<T>, "T must be arithmetic type");
            
            static T convert(const Value& value, StrView context = "") {
                if (!value.isNumber()) {
                    throw LibException(
                        LibErrorCode::TypeMismatch,
                        Str(context) + ": expected number, got " + Str(getTypeName(value))
                    );
                }
                
                auto num = value.asNumber();
                
                // 检查范围
                if constexpr (std::is_integral_v<T>) {
                    if (num < std::numeric_limits<T>::min() || num > std::numeric_limits<T>::max()) {
                        throw LibException(
                            LibErrorCode::OutOfRange,
                            Str(context) + ": number " + std::to_string(num) + 
                            " out of range for type " + typeid(T).name()
                        );
                    }
                    return static_cast<T>(num);
                } else {
                    return static_cast<T>(num);
                }
            }
        };
        
        /**
         * 字符串转换
         */
        inline Str toString(const Value& value, StrView context = "");
        
        /**
         * 布尔转换
         */
        inline bool toBool(const Value& value, StrView context = "");
        
        // getTypeName函数已移到下面的参数检查和错误处理函数部分
        
        /**
         * 类型别名转换器
         */
        inline i8 toI8(const Value& value, StrView context = "");
        inline i16 toI16(const Value& value, StrView context = "");
        inline i32 toI32(const Value& value, StrView context = "");
        inline i64 toI64(const Value& value, StrView context = "");
        inline u8 toU8(const Value& value, StrView context = "");
        inline u16 toU16(const Value& value, StrView context = "");
        inline u32 toU32(const Value& value, StrView context = "");
        inline u64 toU64(const Value& value, StrView context = "");
        inline f32 toF32(const Value& value, StrView context = "");
        inline f64 toF64(const Value& value, StrView context = "");
        inline usize toUsize(const Value& value, StrView context = "");
        
        /**
         * Lua特定类型转换
         */
        inline LuaInteger toLuaInteger(const Value& value, StrView context = "");
        inline LuaNumber toLuaNumber(const Value& value, StrView context = "");
        inline LuaBoolean toLuaBoolean(const Value& value, StrView context = "");
        
        /**
         * 数组转换
         */
        template<typename T>
        Vec<T> toVector(State* state, i32 tableIndex, StrView context = "");
        
        /**
         * 哈希表转换
         */
        
        // 参数检查和错误处理函数
        void checkArgCount(State* state, i32 expected, StrView funcName);
        void checkArgCount(State* state, i32 min, i32 max, StrView funcName);
        Value checkNumber(State* state, i32 index, StrView funcName);
        Value checkString(State* state, i32 index, StrView funcName);
        Value checkTable(State* state, i32 index, StrView funcName);
        Value checkFunction(State* state, i32 index, StrView funcName);
        std::optional<Value> optNumber(State* state, i32 index, f64 defaultValue);
        std::optional<Value> optString(State* state, i32 index, StrView defaultValue);

        void typeError(State* state, i32 index, StrView expected, StrView funcName);
        
        [[noreturn]] void error(State* state, StrView message);
        [[noreturn]] void argError(State* state, i32 index, StrView message);
        [[noreturn]] void typeError(State* state, i32 index, StrView expected);
        template<typename V>
        HashMap<Str, V> toHashMap(State* state, i32 tableIndex, StrView context = "");

        // SafeFunctionCall类已移动到error_handling.hpp中
        
        // makeSafeCall函数已移动到error_handling.hpp中
        
        // REGISTER_SAFE_FUNCTION宏已移动到error_handling.hpp中

        /**
         * 参数提取器模板类
         * 用于安全地从Lua栈中提取参数
         */
        template<typename... Args>
        class ArgumentExtractor {
        public:
            ArgumentExtractor(State* state, i32 nargs, StrView functionName)
                : state_(state), nargs_(nargs), functionName_(functionName) {
                if (!state_) {
                    throw std::invalid_argument("state cannot be null");
                }
            }
            
            std::tuple<Args...> extract() {
                if (nargs_ != static_cast<i32>(sizeof...(Args))) {
                    throw std::invalid_argument(
                        Str(functionName_) + ": expected " + std::to_string(sizeof...(Args)) + 
                        " arguments, got " + std::to_string(nargs_)
                    );
                }
                return extractImpl(std::index_sequence_for<Args...>{});
            }
            
        private:
            template<std::size_t... I>
            std::tuple<Args...> extractImpl(std::index_sequence<I...>) {
                return std::make_tuple(extractArg<Args>(I + 1)...);
            }
            
            template<typename T>
            T extractArg(i32 index) {
                auto value = state_->get(index);
                if constexpr (std::is_same_v<T, Value>) {
                    return value;
                } else if constexpr (std::is_arithmetic_v<T>) {
                    return NumericConverter<T>::convert(value, functionName_);
                } else if constexpr (std::is_same_v<T, Str>) {
                    return toString(value, functionName_);
                } else if constexpr (std::is_same_v<T, bool>) {
                    return toBool(value, functionName_);
                } else {
                    static_assert(sizeof(T) == 0, "Unsupported argument type");
                }
            }
            
            State* state_;
            i32 nargs_;
            StrView functionName_;
        };
        
        /**
         * 便利的参数提取宏
         */
        #define EXTRACT_ARGS(state, nargs, func_name, ...) \
            ArgumentExtractor<__VA_ARGS__>(state, nargs, func_name).extract()
            
    } // namespace Lib
} // namespace Lua