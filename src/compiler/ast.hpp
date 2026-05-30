#pragma once

/**
 * @file ast.hpp
 * @brief Lua抽象语法树（AST）节点定义
 * 
 * 实现Lua 5.1的抽象语法树节点类型，用于表示解析后的程序结构。
 * 
 * 核心功能：
 * - 定义所有语句类型（赋值、控制结构、函数定义等）
 * - 定义所有表达式类型（字面量、运算符、函数调用等）
 * - 使用智能指针管理节点内存
 * - 支持访问者模式进行AST遍历
 * 
 * 设计原则：
 * - 使用std::variant实现类型安全的多态
 * - 使用std::unique_ptr管理节点生命周期
 * - 清晰的节点层次结构
 * - 完整的位置信息（行号、列号）
 */

#include "common/types.hpp"

namespace Lua {

// 前向声明
struct Expr;
struct Stmt;

// 智能指针类型别名
using ExprPtr = UPtr<Expr>;
using StmtPtr = UPtr<Stmt>;

// =====================================================================
// 源代码位置信息基类
// =====================================================================

/**
 * @brief 源代码位置信息
 *
 * 所有AST节点的基类，包含源代码的行号和列号信息。
 * 用于错误报告、调试和代码生成时的位置追踪。
 */
struct SourceLocation {
    i32 line;    ///< 行号（从1开始）
    i32 column;  ///< 列号（从1开始）

    /**
     * @brief 默认构造函数
     */
    SourceLocation() : line(0), column(0) {}

    /**
     * @brief 带参数的构造函数
     * @param l 行号
     * @param c 列号
     */
    SourceLocation(i32 l, i32 c) : line(l), column(c) {}
};

// =====================================================================
// 表达式节点（Expressions）
// =====================================================================

/**
 * @brief nil字面量
 */
struct NilExpr : SourceLocation {
};

/**
 * @brief 布尔字面量
 */
struct BoolExpr : SourceLocation {
    bool value;
};

/**
 * @brief 数字字面量
 */
struct NumberExpr : SourceLocation {
    f64 value;
};

/**
 * @brief 字符串字面量
 */
struct StringExpr : SourceLocation {
    Str value;
};

/**
 * @brief 变长参数 ...
 */
struct VarargExpr : SourceLocation {
};

/**
 * @brief 标识符（变量名）
 */
struct NameExpr : SourceLocation {
    Str name;
};

/**
 * @brief 二元运算表达式
 */
struct BinaryExpr : SourceLocation {
    enum class Op {
        // 算术运算
        Add, Sub, Mul, Div, Mod, Pow,
        // 比较运算
        Eq, Ne, Lt, Le, Gt, Ge,
        // 逻辑运算
        And, Or,
        // 字符串连接
        Concat
    };

    Op op;
    ExprPtr left;
    ExprPtr right;
};

/**
 * @brief 一元运算表达式
 */
struct UnaryExpr : SourceLocation {
    enum class Op {
        Not,    // not
        Neg,    // -
        Len     // #
    };

    Op op;
    ExprPtr operand;
};

/**
 * @brief 表构造器
 */
struct TableField {
    ExprPtr key;    // nil表示数组部分
    ExprPtr value;
};

struct TableExpr : SourceLocation {
    Vec<TableField> fields;
};

/**
 * @brief 函数调用
 *
 * 支持两种调用方式：
 * - 普通调用：func(args)
 * - 方法调用：obj:method(args) - 等价于 obj.method(obj, args)
 */
struct CallExpr : SourceLocation {
    ExprPtr func;
    Vec<ExprPtr> args;
    bool isMethodCall = false;  // 是否为方法调用（使用冒号语法）
};

/**
 * @brief 表索引访问 table[key]
 */
struct IndexExpr : SourceLocation {
    ExprPtr table;
    ExprPtr index;
};

/**
 * @brief 成员访问 table.member
 */
struct MemberExpr : SourceLocation {
    ExprPtr table;
    Str member;
};

/**
 * @brief 函数定义表达式
 */
struct FunctionExpr : SourceLocation {
    Vec<Str> params;
    bool isVararg;
    Vec<StmtPtr> body;
    i32 endLine = 0;
};

/**
 * @brief 括号表达式
 *
 * 需要保留括号语义以对齐 Lua 5.1：
 * (exp) 会将函数调用/vararg 的多返回值收敛为单值。
 */
struct ParenExpr : SourceLocation {
    ExprPtr expression;
};

/**
 * @brief 表达式的variant类型
 */
using ExprVariant = std::variant<
    NilExpr,
    BoolExpr,
    NumberExpr,
    StringExpr,
    VarargExpr,
    NameExpr,
    BinaryExpr,
    UnaryExpr,
    TableExpr,
    CallExpr,
    IndexExpr,
    MemberExpr,
    FunctionExpr,
    ParenExpr
>;

/**
 * @brief 表达式基类
 */
struct Expr {
    ExprVariant variant;

    template<typename T>
    explicit Expr(T&& v) : variant(std::forward<T>(v)) {}

    i32 getLine() const;
    i32 getColumn() const;
};

// =====================================================================
// 语句节点（Statements）
// =====================================================================

/**
 * @brief 空语句
 */
struct EmptyStmt : SourceLocation {
};

/**
 * @brief 赋值语句
 */
struct AssignStmt : SourceLocation {
    Vec<ExprPtr> targets;  // 左值列表
    Vec<ExprPtr> values;   // 右值列表
};

/**
 * @brief 局部变量声明
 */
struct LocalStmt : SourceLocation {
    Vec<Str> names;
    Vec<ExprPtr> values;
};

/**
 * @brief 函数调用语句
 */
struct CallStmt : SourceLocation {
    ExprPtr call;
};

/**
 * @brief if语句
 */
struct IfStmt : SourceLocation {
    struct Branch {
        ExprPtr condition;
        Vec<StmtPtr> body;
    };

    Vec<Branch> branches;  // if和elseif分支
    Vec<StmtPtr> elseBranch;  // else分支
    i32 endLine = 0;
};

/**
 * @brief while循环
 */
struct WhileStmt : SourceLocation {
    ExprPtr condition;
    Vec<StmtPtr> body;
    i32 endLine = 0;
};

/**
 * @brief repeat-until循环
 */
struct RepeatStmt : SourceLocation {
    Vec<StmtPtr> body;
    ExprPtr condition;
    i32 endLine = 0;
};

/**
 * @brief 数值for循环
 */
struct ForNumStmt : SourceLocation {
    Str var;
    ExprPtr init;
    ExprPtr limit;
    ExprPtr step;  // 可选，默认为1
    Vec<StmtPtr> body;
    i32 endLine = 0;
};

/**
 * @brief 泛型for循环
 */
struct ForInStmt : SourceLocation {
    Vec<Str> vars;
    Vec<ExprPtr> iterators;
    Vec<StmtPtr> body;
    i32 endLine = 0;
};

/**
 * @brief 函数定义语句
 *
 * 支持以下形式：
 * - function foo() end                  -- 简单函数
 * - function t.a.b.c.foo() end         -- 表成员函数
 * - function t:method() end             -- 方法定义（自动添加self参数）
 */
struct FunctionStmt : SourceLocation {
    Str name;                    // 基础函数名
    Vec<Str> tablePath;          // 表路径，例如 t.a.b.c 中的 ["t", "a", "b", "c"]
    bool isMethod;               // 是否为方法定义（使用冒号语法）
    Vec<Str> params;
    bool isVararg;
    Vec<StmtPtr> body;
    bool isLocal;
    i32 endLine = 0;
};

/**
 * @brief return语句
 */
struct ReturnStmt : SourceLocation {
    Vec<ExprPtr> values;
};

/**
 * @brief break语句
 */
struct BreakStmt : SourceLocation {
};

/**
 * @brief do-end块
 */
struct DoStmt : SourceLocation {
    Vec<StmtPtr> body;
    i32 endLine = 0;
};

/**
 * @brief 语句的variant类型
 */
using StmtVariant = std::variant<
    EmptyStmt,
    AssignStmt,
    LocalStmt,
    CallStmt,
    IfStmt,
    WhileStmt,
    RepeatStmt,
    ForNumStmt,
    ForInStmt,
    FunctionStmt,
    ReturnStmt,
    BreakStmt,
    DoStmt
>;

/**
 * @brief 语句基类
 */
struct Stmt {
    StmtVariant variant;

    template<typename T>
    explicit Stmt(T&& v) : variant(std::forward<T>(v)) {}

    i32 getLine() const;
    i32 getColumn() const;
    i32 getEndLine() const;
};

/**
 * @brief 程序块（Chunk）
 */
struct Chunk {
    Vec<StmtPtr> statements;
};

} // namespace Lua

