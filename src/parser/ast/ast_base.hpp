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
        Function,   // Function expression
        Vararg      // Vararg expression (...)
    };

    // Statement types
    enum class StmtType {
        Expression, // Expression statement
        Block,      // Block statement
        Local,      // Local variable declaration
        MultiLocal, // Multi-local variable declaration (local a, b, c = ...)
        Assign,     // Assignment statement
        If,         // If statement
        While,      // While loop
        For,        // For loop (numeric)
        ForIn,      // For-in loop (iterator)
        RepeatUntil,// Repeat-until loop
        Return,     // Return statement
        Break,      // Break statement
        Function,   // Function definition statement
        Do          // Do block statement
    };

    // Expression base class
    class Expr {
    protected:
        SourceLocation location_;  // Source location information
        
    public:
        virtual ~Expr() = default;
        virtual ExprType getType() const = 0;
        
        // Location information related methods
        const SourceLocation& getLocation() const { return location_; }
        void setLocation(const SourceLocation& location) { location_ = location; }
        
        // Convenience constructor
        explicit Expr(const SourceLocation& location = SourceLocation()) 
            : location_(location) {}
    };

    // Statement base class
    class Stmt {
    protected:
        SourceLocation location_;  // Source location information
        
    public:
        virtual ~Stmt() = default;
        virtual StmtType getType() const = 0;
        
        // Location information related methods
        const SourceLocation& getLocation() const { return location_; }
        void setLocation(const SourceLocation& location) { location_ = location; }
        
        // Convenience constructor
        explicit Stmt(const SourceLocation& location = SourceLocation()) 
            : location_(location) {}
    };
}