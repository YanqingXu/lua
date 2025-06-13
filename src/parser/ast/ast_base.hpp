#pragma once

#include "../../common/types.hpp"
#include "../../vm/value.hpp"
#include "source_location.hpp"

namespace Lua {
    // Expression types
    enum class ExprType {
        Binary,     // Binary expression
        Unary,      // Unary expression
        Literal,    // Literal
        Variable,   // Variable
        Call,       // Function call
        Table,      // Table construction
        Member,     // Member access (obj.field)
        Index,      // Index access (obj[key])
        Function    // Function expression
    };

    // Statement types
    enum class StmtType {
        Expression, // Expression statement
        Block,      // Block statement
        Local,      // Local variable declaration
        Assign,     // Assignment statement
        If,         // If statement
        While,      // While loop
        For,        // For loop (numeric)
        ForIn,      // For-in loop (iterator)
        RepeatUntil,// Repeat-until loop
        Return,     // Return statement
        Break,      // Break statement
        Function    // Function definition statement
    };

    // Expression base class
    class Expr {
    protected:
        SourceLocation location_;  // 源码位置信息
        
    public:
        virtual ~Expr() = default;
        virtual ExprType getType() const = 0;
        
        // 位置信息相关方法
        const SourceLocation& getLocation() const { return location_; }
        void setLocation(const SourceLocation& location) { location_ = location; }
        
        // 便利构造函数
        explicit Expr(const SourceLocation& location = SourceLocation()) 
            : location_(location) {}
    };

    // Statement base class
    class Stmt {
    protected:
        SourceLocation location_;  // 源码位置信息
        
    public:
        virtual ~Stmt() = default;
        virtual StmtType getType() const = 0;
        
        // 位置信息相关方法
        const SourceLocation& getLocation() const { return location_; }
        void setLocation(const SourceLocation& location) { location_ = location; }
        
        // 便利构造函数
        explicit Stmt(const SourceLocation& location = SourceLocation()) 
            : location_(location) {}
    };
}