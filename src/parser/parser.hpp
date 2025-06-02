#pragma once

#include "../types.hpp"
#include "../lexer/lexer.hpp"
#include "../compiler/compiler.hpp"
#include "../vm/value.hpp"

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

    // Expression base class
    class Expr {
    public:
        virtual ~Expr() = default;
        virtual ExprType getType() const = 0;
    };

    // Literal expression (numbers, strings, booleans, nil)
    class LiteralExpr : public Expr {
    private:
        Value value;

    public:
        explicit LiteralExpr(const Value& value) : value(value) {}

        ExprType getType() const override { return ExprType::Literal; }
        const Value& getValue() const { return value; }
    };

    // Variable expression
    class VariableExpr : public Expr {
    private:
        Str name;

    public:
        explicit VariableExpr(const Str& name) : name(name) {}

        ExprType getType() const override { return ExprType::Variable; }
        const Str& getName() const { return name; }
    };

    // Unary expression
    class UnaryExpr : public Expr {
    private:
        TokenType op;
        UPtr<Expr> right;

    public:
        UnaryExpr(TokenType op, UPtr<Expr> right)
            : op(op), right(std::move(right)) {}

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
        BinaryExpr(UPtr<Expr> left, TokenType op, UPtr<Expr> right)
            : left(std::move(left)), op(op), right(std::move(right)) {}

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
        CallExpr(UPtr<Expr> callee, Vec<UPtr<Expr>> arguments)
            : callee(std::move(callee)), arguments(std::move(arguments)) {}

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
        MemberExpr(UPtr<Expr> object, const Str& name)
            : object(std::move(object)), name(name) {}

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
        explicit TableExpr(Vec<TableField> fields)
            : fields(std::move(fields)) {}

        ExprType getType() const override { return ExprType::Table; }
        const Vec<TableField>& getFields() const { return fields; }
    };

    // Index access expression (obj[key])
    class IndexExpr : public Expr {
    private:
        UPtr<Expr> object;
        UPtr<Expr> index;

    public:
        IndexExpr(UPtr<Expr> object, UPtr<Expr> index)
            : object(std::move(object)), index(std::move(index)) {}

        ExprType getType() const override { return ExprType::Index; }
        const Expr* getObject() const { return object.get(); }
        const Expr* getIndex() const { return index.get(); }
    };

    // Statement types
    enum class StmtType {
        Expression,  // Expression statement
        Block,       // Block statement
        If,          // if statement
        While,       // while loop
        Function,    // Function definition
        Return,      // return statement
        Local,       // Local variable declaration
        Assign       // Assignment statement
    };

    // Statement base class
    class Stmt {
    public:
        virtual ~Stmt() = default;
        virtual StmtType getType() const = 0;
    };

    // Expression statement
    class ExprStmt : public Stmt {
    private:
        UPtr<Expr> expression;

    public:
        explicit ExprStmt(UPtr<Expr> expression)
            : expression(std::move(expression)) {}

        StmtType getType() const override { return StmtType::Expression; }
        const Expr* getExpression() const { return expression.get(); }
    };

    // Block statement (block composed of multiple statements)
    class BlockStmt : public Stmt {
    private:
        Vec<UPtr<Stmt>> statements;

    public:
        explicit BlockStmt(Vec<UPtr<Stmt>> statements)
            : statements(std::move(statements)) {}

        StmtType getType() const override { return StmtType::Block; }
        const Vec<UPtr<Stmt>>& getStatements() const { return statements; }
    };

    // Local variable declaration statement
    class LocalStmt : public Stmt {
    private:
        Str name;
        UPtr<Expr> initializer;

    public:
        LocalStmt(const Str& name, UPtr<Expr> initializer)
            : name(name), initializer(std::move(initializer)) {}

        StmtType getType() const override { return StmtType::Local; }
        const Str& getName() const { return name; }
        const Expr* getInitializer() const { return initializer.get(); }
    };

    // Assignment statement (var = expr, obj.field = expr, obj[key] = expr)
    class AssignStmt : public Stmt {
    private:
        UPtr<Expr> target;
        UPtr<Expr> value;

    public:
        AssignStmt(UPtr<Expr> target, UPtr<Expr> value)
            : target(std::move(target)), value(std::move(value)) {}

        StmtType getType() const override { return StmtType::Assign; }
        const Expr* getTarget() const { return target.get(); }
        const Expr* getValue() const { return value.get(); }
    };

    // If statement (if condition then body [else elseBody] end)
    class IfStmt : public Stmt {
    private:
        UPtr<Expr> condition;
        UPtr<Stmt> thenBranch;
        UPtr<Stmt> elseBranch;

    public:
        IfStmt(UPtr<Expr> condition, UPtr<Stmt> thenBranch, UPtr<Stmt> elseBranch = nullptr)
            : condition(std::move(condition)), thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}

        StmtType getType() const override { return StmtType::If; }
        const Expr* getCondition() const { return condition.get(); }
        const Stmt* getThenBranch() const { return thenBranch.get(); }
        const Stmt* getElseBranch() const { return elseBranch.get(); }
    };

    // Parser class
    class Parser {
    private:
        Lexer lexer;
        Token current;
        Token previous;
        bool hadError;

        // Helper methods
        void advance();
        bool check(TokenType type) const;
        bool match(TokenType type);
        bool match(std::initializer_list<TokenType> types);
        Token consume(TokenType type, const Str& message);
        void error(const Str& message);
        void synchronize();

        // Parse expressions
        UPtr<Expr> expression();
        UPtr<Expr> logicalOr();
        UPtr<Expr> logicalAnd();
        UPtr<Expr> equality();
        UPtr<Expr> comparison();
        UPtr<Expr> concatenation();
        UPtr<Expr> simpleExpression();
        UPtr<Expr> term();
        UPtr<Expr> unary();
        UPtr<Expr> power();
        UPtr<Expr> primary();
        UPtr<Expr> finishCall(UPtr<Expr> callee);
        UPtr<Expr> tableConstructor();
        UPtr<Expr> functionExpression();

        // Parse statements
        UPtr<Stmt> statement();
        UPtr<Stmt> expressionStatement();
        UPtr<Stmt> localDeclaration();
        UPtr<Stmt> assignmentStatement();
        UPtr<Stmt> ifStatement();
        UPtr<Stmt> blockStatement();

        // Helper for assignment target validation
        bool isValidAssignmentTarget(const Expr* expr) const;

    public:
        explicit Parser(const Str& source);

        // Parse entire program
        Vec<UPtr<Stmt>> parse();

        // Check if there are errors
        bool hasError() const { return hadError; }
    };

    // Function expression
    class FunctionExpr : public Expr {
    private:
        Vec<Str> parameters;
        UPtr<Stmt> body;

    public:
        FunctionExpr(Vec<Str> params, UPtr<Stmt> body)
            : parameters(std::move(params)), body(std::move(body)) {
        }

        ExprType getType() const override { return ExprType::Function; }
        const Vec<Str>& getParameters() const { return parameters; }
        const Stmt* getBody() const { return body.get(); }
    };
}