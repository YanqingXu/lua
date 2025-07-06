#pragma once

#include "../core/lib_define.hpp"
#include "lib_utils.hpp"
#include "error_handling.hpp"
#include "lib_utils.hpp"
#include <limits>
#include <type_traits>

namespace Lua {
    namespace Lib {
        // 类型转换工具已迁移到 lib_utils.hpp
        // 为了向后兼容，这里提供别名
        // TypeConverter命名空间已移除，功能已整合到Lib命名空间中
        
        // 保留原有的命名空间定义以防有遗留代码
        namespace TypeConverterLegacy {
        // Forward declaration
        inline Str getTypeName(const Value& value);
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
                        Str(context) + ": expected number, got " + getTypeName(value)
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
        inline Str toString(const Value& value, StrView context = "") {
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
        
        /**
         * 布尔转换
         */
        inline bool toBool(const Value& value, StrView context = "") {
            if (value.isBoolean()) {
                return value.asBoolean();
            } else if (value.isNil()) {
                return false;
            } else {
                // Lua中，除了nil和false，其他都是true
                return true;
            }
        }
        
        /**
         * 获取类型名称
         */
        inline Str getTypeName(const Value& value) {
            if (value.isNumber()) return "number";
            if (value.isString()) return "string";
            if (value.isBoolean()) return "boolean";
            if (value.isNil()) return "nil";
            if (value.isTable()) return "table";
            if (value.isFunction()) return "function";
            return "unknown";
        }
        
        /**
         * 类型别名转换器
         */
        inline i8 toI8(const Value& value, StrView context = "") {
            return NumericConverter<i8>::convert(value, context);
        }
        
        inline i16 toI16(const Value& value, StrView context = "") {
            return NumericConverter<i16>::convert(value, context);
        }
        
        inline i32 toI32(const Value& value, StrView context = "") {
            return NumericConverter<i32>::convert(value, context);
        }
        
        inline i64 toI64(const Value& value, StrView context = "") {
            return NumericConverter<i64>::convert(value, context);
        }
        
        inline u8 toU8(const Value& value, StrView context = "") {
            return NumericConverter<u8>::convert(value, context);
        }
        
        inline u16 toU16(const Value& value, StrView context = "") {
            return NumericConverter<u16>::convert(value, context);
        }
        
        inline u32 toU32(const Value& value, StrView context = "") {
            return NumericConverter<u32>::convert(value, context);
        }
        
        inline u64 toU64(const Value& value, StrView context = "") {
            return NumericConverter<u64>::convert(value, context);
        }
        
        inline f32 toF32(const Value& value, StrView context = "") {
            return NumericConverter<f32>::convert(value, context);
        }
        
        inline f64 toF64(const Value& value, StrView context = "") {
            return NumericConverter<f64>::convert(value, context);
        }
        
        inline usize toUsize(const Value& value, StrView context = "") {
            return NumericConverter<usize>::convert(value, context);
        }
        
        /**
         * Lua特定类型转换
         */
        inline LuaInteger toLuaInteger(const Value& value, StrView context = "") {
            return NumericConverter<LuaInteger>::convert(value, context);
        }
        
        inline LuaNumber toLuaNumber(const Value& value, StrView context = "") {
            return NumericConverter<LuaNumber>::convert(value, context);
        }
        
        inline LuaBoolean toLuaBoolean(const Value& value, StrView context = "") {
            return toBool(value, context);
        }
        
        /**
         * 数组转换
         */
        template<typename T>
        Vec<T> toVector(State* state, i32 tableIndex, StrView context = "") {
            auto table = state->get(tableIndex);
            if (!table.isTable()) {
                throw LibException(
                    LibErrorCode::TypeMismatch,
                    Str(context) + ": expected table, got " + getTypeName(table)
                );
            }
            
            Vec<T> result;
            // 简化的表遍历逻辑
            // 在实际实现中，这里会遍历Lua表
            return result;
        }
        
        /**
         * 哈希表转换
         */
        template<typename V>
        HashMap<Str, V> toHashMap(State* state, i32 tableIndex, StrView context = "") {
            auto table = state->get(tableIndex);
            if (!table.isTable()) {
                throw LibException(
                    LibErrorCode::TypeMismatch,
                    Str(context) + ": expected table, got " + getTypeName(table)
                );
            }
            
            HashMap<Str, V> result;
            // 简化的表遍历逻辑
            // 在实际实现中，这里会遍历Lua表的键值对
            return result;
        }
    }
    
    /**
     * 类型转换库模块
     */
    class TypeConversionLib : public Lib::LibModule {
    public:
        StrView getName() const noexcept override {
            return "typeconv";
        }
        
        void registerFunctions(Lib::LibFuncRegistry& registry, const Lib::LibContext& context) override {
            // 基础类型转换
            REGISTER_SAFE_FUNCTION(registry, toint8, toInt8);
            REGISTER_SAFE_FUNCTION(registry, toint16, toInt16);
            REGISTER_SAFE_FUNCTION(registry, toint32, toInt32);
            REGISTER_SAFE_FUNCTION(registry, toint64, toInt64);
            
            REGISTER_SAFE_FUNCTION(registry, touint8, toUint8);
            REGISTER_SAFE_FUNCTION(registry, touint16, toUint16);
            REGISTER_SAFE_FUNCTION(registry, touint32, toUint32);
            REGISTER_SAFE_FUNCTION(registry, touint64, toUint64);
            
            REGISTER_SAFE_FUNCTION(registry, tofloat32, toFloat32);
            REGISTER_SAFE_FUNCTION(registry, tofloat64, toFloat64);
            
            // 字符串和布尔转换
            REGISTER_SAFE_FUNCTION(registry, tostring, toStringFunc);
            REGISTER_SAFE_FUNCTION(registry, tobool, toBoolFunc);
            
            // 类型检查
            REGISTER_SAFE_FUNCTION(registry, typename, getTypeNameFunc);
            REGISTER_SAFE_FUNCTION(registry, istype, isTypeFunc);
            
            // 数组和表转换
            REGISTER_SAFE_FUNCTION(registry, toarray, toArrayFunc);
            REGISTER_SAFE_FUNCTION(registry, totable, toTableFunc);
            
            // 范围检查
            REGISTER_SAFE_FUNCTION(registry, checkrange, checkRangeFunc);
            REGISTER_SAFE_FUNCTION(registry, clamp, clampFunc);
        }
        
    private:
        static Value toInt8(State* state, i32 nargs) {
            ErrorUtils::checkArgCount(nargs, 1, "toint8");
            auto result = toI8(state->get(1), "toint8");
            return Value(static_cast<LuaInteger>(result));
        }
        
        static Value toInt16(State* state, i32 nargs) {
            ErrorUtils::checkArgCount(nargs, 1, "toint16");
            auto result = toI16(state->get(1), "toint16");
            return Value(static_cast<LuaInteger>(result));
        }
        
        static Value toInt32(State* state, i32 nargs) {
            ErrorUtils::checkArgCount(nargs, 1, "toint32");
            auto result = toI32(state->get(1), "toint32");
            return Value(static_cast<LuaInteger>(result));
        }
        
        static Value toInt64(State* state, i32 nargs) {
            ErrorUtils::checkArgCount(nargs, 1, "toint64");
            auto result = toI64(state->get(1), "toint64");
            return Value(static_cast<LuaInteger>(result));
        }
        
        static Value toUint8(State* state, i32 nargs) {
            ErrorUtils::checkArgCount(nargs, 1, "touint8");
            auto result = toU8(state->get(1), "touint8");
            return Value(static_cast<LuaInteger>(result));
        }
        
        static Value toUint16(State* state, i32 nargs) {
            ErrorUtils::checkArgCount(nargs, 1, "touint16");
            auto result = toU16(state->get(1), "touint16");
            return Value(static_cast<LuaInteger>(result));
        }
        
        static Value toUint32(State* state, i32 nargs) {
            ErrorUtils::checkArgCount(nargs, 1, "touint32");
            auto result = toU32(state->get(1), "touint32");
            return Value(static_cast<LuaInteger>(result));
        }
        
        static Value toUint64(State* state, i32 nargs) {
            ErrorUtils::checkArgCount(nargs, 1, "touint64");
            auto result = toU64(state->get(1), "touint64");
            return Value(static_cast<LuaInteger>(result));
        }
        
        static Value toFloat32(State* state, i32 nargs) {
            ErrorUtils::checkArgCount(nargs, 1, "tofloat32");
            auto result = toF32(state->get(1), "tofloat32");
            return Value(static_cast<LuaNumber>(result));
        }
        
        static Value toFloat64(State* state, i32 nargs) {
            ErrorUtils::checkArgCount(nargs, 1, "tofloat64");
            auto result = toF64(state->get(1), "tofloat64");
            return Value(static_cast<LuaNumber>(result));
        }
        
        static Value toStringFunc(State* state, i32 nargs) {
            ErrorUtils::checkArgCount(nargs, 1, "tostring");
            auto result = toString(state->get(1), "tostring");
            return Value(result);
        }
        
        static Value toBoolFunc(State* state, i32 nargs) {
            ErrorUtils::checkArgCount(nargs, 1, "tobool");
            auto result = toBool(state->get(1), "tobool");
            return Value(result);
        }
        
        static Value getTypeNameFunc(State* state, i32 nargs) {
            ErrorUtils::checkArgCount(nargs, 1, "typename");
            auto result = getTypeName(state->get(1));
            return Value(result);
        }
        
        static Value isTypeFunc(State* state, i32 nargs) {
            ErrorUtils::checkArgCount(nargs, 2, "istype");
            auto value = state->get(1);
            auto expectedType = toString(state->get(2), "istype");
            auto actualType = getTypeName(value);
            return Value(actualType == expectedType);
        }
        
        static Value toArrayFunc(State* state, i32 nargs) {
            ErrorUtils::checkArgCount(nargs, 1, "toarray");
            
            // 简化的数组转换实现
            auto table = state->get(1);
            if (!table.isTable()) {
                throw LibException(
                    LibErrorCode::TypeMismatch,
                    "toarray: expected table, got " + getTypeName(table)
                );
            }
            
            // 在实际实现中，这里会创建一个新的数组表
            return table;
        }
        
        static Value toTableFunc(State* state, i32 nargs) {
            ErrorUtils::checkArgCount(nargs, 1, "totable");
            
            // 简化的表转换实现
            auto value = state->get(1);
            
            // 在实际实现中，这里会根据值的类型创建相应的表
            if (value.isTable()) {
                return value;
            } else {
                // 创建包含单个元素的表
                return value; // 简化返回
            }
        }
        
        static Value checkRangeFunc(State* state, i32 nargs) {
            ErrorUtils::checkArgCount(nargs, 3, "checkrange");
            
            auto value = toF64(state->get(1), "checkrange");
            auto min = toF64(state->get(2), "checkrange");
            auto max = toF64(state->get(3), "checkrange");
            
            if (value < min || value > max) {
                throw LibException(
                    LibErrorCode::OutOfRange,
                    "checkrange: value " + std::to_string(value) + 
                    " out of range [" + std::to_string(min) + ", " + std::to_string(max) + "]"
                );
            }
            
            return Value(true);
        }
        
        static Value clampFunc(State* state, i32 nargs) {
            ErrorUtils::checkArgCount(nargs, 3, "clamp");
            
            auto value = toF64(state->get(1), "clamp");
            auto min = toF64(state->get(2), "clamp");
            auto max = toF64(state->get(3), "clamp");
            
            if (value < min) value = min;
            else if (value > max) value = max;
            
            return Value(value);
        }
    };
    
    // ArgumentExtractor 已移至 lib_utils.hpp 中
    // 这里保留注释以说明迁移情况
    
    /**
     * 便利的参数提取宏
     */
    #define EXTRACT_ARGS(state, nargs, func_name, ...) \
        ArgumentExtractor<__VA_ARGS__>(state, nargs, func_name).extract()
    
    } // namespace TypeConverterLegacy
    } // namespace Lib
} // namespace Lua