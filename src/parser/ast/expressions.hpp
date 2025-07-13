#pragma once

#include "ast_base.hpp"
#include "../../lexer/lexer.hpp"
#include "../../vm/value.hpp"

namespace Lua {
    // Literal expression (numbers, strings, booleans, nil)
    class LiteralExpr : public Expr {
    private:
        Value value;

    public:
        explicit LiteralExpr(const Value& value, const SourceLocation& location = SourceLocation()) 
            : Expr(location), value(value) {}

        ExprType getType() const override { return ExprType::Literal; }
        const Value& getValue() const { return value; }
    };

    // Variable expression
    class VariableExpr : public Expr {
    private:
        Str name;

    public:
        explicit VariableExpr(const Str& name, const SourceLocation& location = SourceLocation()) 
            : Expr(location), name(name) {}

        ExprType getType() const override { return ExprType::Variable; }
        const Str& getName() const { return name; }
    };

    // Unary expression
    class UnaryExpr : public Expr {
    private:
        TokenType op;
        UPtr<Expr> right;

    public:
        UnaryExpr(TokenType op, UPtr<Expr> right, const SourceLocation& location = SourceLocation())
            : Expr(location), op(op), right(std::move(right)) {}

        ExprType getType() const override { return ExprType::Unary; }
        TokenType getOperator() const { return op; }
        const Expr* getRight() const { return right.get(); }
    };

    // Binary expression
    class BinaryExpr : public Expr {
    private:
        UPtr<Expr> left;
        TokenType op;
        UPtr<Expr> right;

    public:
        BinaryExpr(UPtr<Expr> left, TokenType op, UPtr<Expr> right, const SourceLocation& location = SourceLocation())
            : Expr(location), left(std::move(left)), op(op), right(std::move(right)) {}

        ExprType getType() const override { return ExprType::Binary; }
        const Expr* getLeft() const { return left.get(); }
        TokenType getOperator() const { return op; }
        const Expr* getRight() const { return right.get(); }
    };

    // Function call expression
    class CallExpr : public Expr {
    private:
        UPtr<Expr> callee;
        Vec<UPtr<Expr>> arguments;

    public:
        CallExpr(UPtr<Expr> callee, Vec<UPtr<Expr>> arguments, const SourceLocation& location = SourceLocation())
            : Expr(location), callee(std::move(callee)), arguments(std::move(arguments)) {}

        ExprType getType() const override { return ExprType::Call; }
        const Expr* getCallee() const { return callee.get(); }
        const Vec<UPtr<Expr>>& getArguments() const { return arguments; }
    };

    // Member access expression (obj.field)
    class MemberExpr : public Expr {
    private:
        UPtr<Expr> object;
        Str name;

    public:
        MemberExpr(UPtr<Expr> object, const Str& name, const SourceLocation& location = SourceLocation())
            : Expr(location), object(std::move(object)), name(name) {}

        ExprType getType() const override { return ExprType::Member; }
        const Expr* getObject() const { return object.get(); }
        const Str& getName() const { return name; }
    };

    // Table field for table construction
    struct TableField {
        UPtr<Expr> key;    // nil for array-style fields
        UPtr<Expr> value;

        TableField(UPtr<Expr> key, UPtr<Expr> value)
            : key(std::move(key)), value(std::move(value)) {}
    };

    // Table construction expression {key = value, [expr] = value, value}
    class TableExpr : public Expr {
    private:
        Vec<TableField> fields;

    public:
        explicit TableExpr(Vec<TableField> fields, const SourceLocation& location = SourceLocation())
            : Expr(location), fields(std::move(fields)) {}

        ExprType getType() const override { return ExprType::Table; }
        const Vec<TableField>& getFields() const { return fields; }
    };

    // Index access expression (obj[key])
    class IndexExpr : public Expr {
    private:
        UPtr<Expr> object;
        UPtr<Expr> index;

    public:
        IndexExpr(UPtr<Expr> object, UPtr<Expr> index, const SourceLocation& location = SourceLocation())
            : Expr(location), object(std::move(object)), index(std::move(index)) {}

        ExprType getType() const override { return ExprType::Index; }
        const Expr* getObject() const { return object.get(); }
        const Expr* getIndex() const { return index.get(); }
    };

    // Function expression
    class FunctionExpr : public Expr {
    private:
        Vec<Str> parameters;
        UPtr<Stmt> body;
        bool isVariadic;

    public:
        FunctionExpr(Vec<Str> params, UPtr<Stmt> body, bool variadic = false, const SourceLocation& location = SourceLocation())
            : Expr(location), parameters(std::move(params)), body(std::move(body)), isVariadic(variadic) {
        }

        ExprType getType() const override { return ExprType::Function; }
        const Vec<Str>& getParameters() const { return parameters; }
        const Stmt* getBody() const { return body.get(); }
        bool getIsVariadic() const { return isVariadic; }
    };

    // Vararg expression (...)
    class VarargExpr : public Expr {
    public:
        explicit VarargExpr(const SourceLocation& location = SourceLocation())
            : Expr(location) {}

        ExprType getType() const override { return ExprType::Vararg; }
    };
}