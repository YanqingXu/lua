#pragma once

#include "../types.hpp"
#include <variant>
#include <iostream>
#include <functional>   // 用于std::less
#include <cmath>        // 用于浮点数操作

namespace Lua {
    // 前向声明
    class Table;
    class Function;
    
    // Lua值类型
    enum class ValueType {
        Nil,
        Boolean,
        Number,
        String,
        Table,
        Function
    };
    
    // Lua值
    class Value {
    private:
        // 使用std::variant来存储不同类型的值
        using ValueVariant = std::variant<
            std::monostate,      // Nil
            LuaBoolean,          // Boolean
            LuaNumber,           // Number
            Ptr<Str>,      // String
            Ptr<Table>,          // Table
            Ptr<Function>        // Function
        >;
        
        ValueVariant data;
        
    public:
        // 构造函数
        Value() : data(std::monostate{}) {}
        Value(std::nullptr_t) : data(std::monostate{}) {}
        Value(LuaBoolean val) : data(val) {}
        Value(LuaNumber val) : data(val) {}
        Value(i32 val) : data(static_cast<LuaNumber>(val)) {}  // 接受32位整数
        Value(i64 val) : data(static_cast<LuaNumber>(val)) {}  // 接受64位整数
        Value(const Str& val) : data(make_ptr<Str>(val)) {}
        Value(const char* val) : data(make_ptr<Str>(val)) {}
        Value(Ptr<Table> val) : data(val) {}
        Value(Ptr<Function> val) : data(val) {}
        
        // 复制构造函数
        Value(const Value& other) = default;
        
        // 移动构造函数
        Value(Value&& other) noexcept = default;
        
        // 复制赋值运算符
        Value& operator=(const Value& other) = default;
        
        // 移动赋值运算符
        Value& operator=(Value&& other) noexcept = default;
        
        // 类型检查
        ValueType type() const;
        bool isNil() const { return type() == ValueType::Nil; }
        bool isBoolean() const { return type() == ValueType::Boolean; }
        bool isNumber() const { return type() == ValueType::Number; }
        bool isString() const { return type() == ValueType::String; }
        bool isTable() const { return type() == ValueType::Table; }
        bool isFunction() const { return type() == ValueType::Function; }
        
        // 获取值
        LuaBoolean asBoolean() const;
        LuaNumber asNumber() const;
        const Str& asString() const;
        Ptr<Table> asTable() const;
        Ptr<Function> asFunction() const;
        
        // 转换为字符串用于打印
        Str toString() const;
        
        // 相等比较
        bool operator==(const Value& other) const;
        bool operator!=(const Value& other) const { return !(*this == other); }
        
        // 小于比较，用于map排序
        bool operator<(const Value& other) const {
            // 首先按类型排序
            if (type() != other.type()) {
                return static_cast<int>(type()) < static_cast<int>(other.type());
            }
            
            // 同类型比较
            switch (type()) {
                case ValueType::Nil:
                    return false; // nil不小于nil
                case ValueType::Boolean:
                    return asBoolean() < other.asBoolean();
                case ValueType::Number:
                    return asNumber() < other.asNumber();
                case ValueType::String:
                    return asString() < other.asString();
                case ValueType::Table:
                case ValueType::Function:
                    // 比较指针地址
                    // 使用std::less确保指针比较的安全性
                    return std::less<void*>()(asTable().get(), other.asTable().get());
                default:
                    return false;
            }
        }
    };
}
