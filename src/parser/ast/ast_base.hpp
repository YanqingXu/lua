#pragma once

#include "../../types.hpp"
#include "../../vm/value.hpp"

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
    public:
        virtual ~Expr() = default;
        virtual ExprType getType() const = 0;
    };

    // Statement base class
    class Stmt {
    public:
        virtual ~Stmt() = default;
        virtual StmtType getType() const = 0;
    };
}