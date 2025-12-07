/**
 * @file parser.cpp
 * @brief Lua语法分析器实现
 */

#include "parser.hpp"
#include <sstream>

namespace Lua {

// =====================================================================
// 构造函数和基本Token管理
// =====================================================================

Parser::Parser(const Str& source)
    : lexer_(source)
    , current_(lexer_.nextToken()) {
}

const Token& Parser::current() const {
    return current_;
}

void Parser::advance() {
    current_ = lexer_.nextToken();
}

Token Parser::peek() {
    // 使用Lexer的peekToken()方法实现高效的Token预读
    // 支持LL(1)语法分析，避免复制整个Lexer状态
    return lexer_.peekToken();
}

bool Parser::check(TokenType type) const {
    return current_.type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

void Parser::expect(TokenType type, const Str& message) {
    if (!match(type)) {
        error(message);
    }
}

void Parser::error(const Str& message) {
    // 构造错误消息
    std::ostringstream oss;
    oss << "Syntax error at line " << current_.line
        << ", column " << current_.column << ": " << message;

    // 直接抛出异常（简化版错误处理）
    throw ParseError(oss.str(), current_.line, current_.column);
}

void Parser::reportError(const Str& message) {
    // 构造错误消息
    std::ostringstream oss;
    oss << "Syntax error at line " << current_.line
        << ", column " << current_.column << ": " << message;

    // 添加到错误列表（为将来的错误恢复机制预留）
    errors_.emplace_back(oss.str(), current_.line, current_.column);

    // 设置 panic 模式（为将来的错误恢复机制预留）
    panicMode_ = true;
}

void Parser::synchronize() {
    // 重置 panic 模式
    panicMode_ = false;

    // 跳过 token 直到找到语句边界
    while (!check(TokenType::Eos)) {
        // 检查是否到达块结束符
        if (check(TokenType::End) ||
            check(TokenType::Else) ||
            check(TokenType::Elseif) ||
            check(TokenType::Until)) {
            return;  // 不消费这些 token，让调用者处理
        }

        // 检查是否到达语句开始符
        if (check(TokenType::Local) ||
            check(TokenType::Function) ||
            check(TokenType::If) ||
            check(TokenType::While) ||
            check(TokenType::For) ||
            check(TokenType::Repeat) ||
            check(TokenType::Return) ||
            check(TokenType::Break)) {
            return;  // 不消费这些 token，让调用者处理
        }

        // 继续跳过当前 token
        advance();
    }
}

// 辅助函数：安全地获取Token的字符串值
static Str getTokenString(const Token& token) {
    if (std::holds_alternative<Str>(token.value)) {
        return std::get<Str>(token.value);
    }
    return token.lexeme;  // 回退到lexeme
}

// =====================================================================
// 主解析函数
// =====================================================================

Chunk Parser::parse() {
    Chunk chunk;

    // 解析语句块
    chunk.statements = parseBlock();

    // 确保到达文件末尾
    if (!check(TokenType::Eos)) {
        error("Expected end of file");
    }

    return chunk;
}

Vec<StmtPtr> Parser::parseBlock() {
    RecursionGuard guard(*this);  // 递归深度保护

    Vec<StmtPtr> statements;

    // 解析语句直到遇到块结束符
    while (!check(TokenType::Eos) &&
           !check(TokenType::End) &&
           !check(TokenType::Else) &&
           !check(TokenType::Elseif) &&
           !check(TokenType::Until)) {

        // return语句必须是块的最后一条语句
        if (check(TokenType::Return)) {
            statements.push_back(parseReturnStmt());
            break;
        }

        statements.push_back(parseStatement());
    }

    return statements;
}

// =====================================================================
// 语句解析
// =====================================================================

StmtPtr Parser::parseStatement() {
    // 根据当前Token类型分发到相应的解析函数
    switch (current_.type) {
        case TokenType::If:
            return parseIfStmt();
        case TokenType::While:
            return parseWhileStmt();
        case TokenType::Do:
            return parseDoStmt();
        case TokenType::For:
            return parseForStmt();
        case TokenType::Repeat:
            return parseRepeatStmt();
        case TokenType::Function:
            return parseFunctionStmt();
        case TokenType::Local:
            return parseLocalStmt();
        case TokenType::Break:
            return parseBreakStmt();
        default:
            // 赋值语句或函数调用语句
            return parseExprStmt();
    }
}

StmtPtr Parser::parseIfStmt() {
    i32 line = current_.line;
    i32 column = current_.column;
    
    expect(TokenType::If, "Expected 'if'");
    
    IfStmt ifStmt;
    ifStmt.line = line;
    ifStmt.column = column;
    
    // 解析if分支
    IfStmt::Branch ifBranch;
    ifBranch.condition = parseExpression();
    expect(TokenType::Then, "Expected 'then' after if condition");
    ifBranch.body = parseBlock();
    ifStmt.branches.push_back(std::move(ifBranch));
    
    // 解析elseif分支
    while (match(TokenType::Elseif)) {
        IfStmt::Branch elseifBranch;
        elseifBranch.condition = parseExpression();
        expect(TokenType::Then, "Expected 'then' after elseif condition");
        elseifBranch.body = parseBlock();
        ifStmt.branches.push_back(std::move(elseifBranch));
    }
    
    // 解析else分支
    if (match(TokenType::Else)) {
        ifStmt.elseBranch = parseBlock();
    }
    
    expect(TokenType::End, "Expected 'end' to close if statement");

    return std::make_unique<Stmt>(std::move(ifStmt));
}

StmtPtr Parser::parseWhileStmt() {
    i32 line = current_.line;
    i32 column = current_.column;

    expect(TokenType::While, "Expected 'while'");

    WhileStmt whileStmt;
    whileStmt.line = line;
    whileStmt.column = column;
    whileStmt.condition = parseExpression();

    expect(TokenType::Do, "Expected 'do' after while condition");
    whileStmt.body = parseBlock();
    expect(TokenType::End, "Expected 'end' to close while loop");

    return std::make_unique<Stmt>(std::move(whileStmt));
}

StmtPtr Parser::parseDoStmt() {
    i32 line = current_.line;
    i32 column = current_.column;

    expect(TokenType::Do, "Expected 'do'");

    DoStmt doStmt;
    doStmt.line = line;
    doStmt.column = column;
    doStmt.body = parseBlock();

    expect(TokenType::End, "Expected 'end' to close do block");

    return std::make_unique<Stmt>(std::move(doStmt));
}

StmtPtr Parser::parseRepeatStmt() {
    i32 line = current_.line;
    i32 column = current_.column;

    expect(TokenType::Repeat, "Expected 'repeat'");

    RepeatStmt repeatStmt;
    repeatStmt.line = line;
    repeatStmt.column = column;
    repeatStmt.body = parseBlock();

    expect(TokenType::Until, "Expected 'until' to close repeat loop");
    repeatStmt.condition = parseExpression();

    return std::make_unique<Stmt>(std::move(repeatStmt));
}

StmtPtr Parser::parseForStmt() {
    i32 line = current_.line;
    i32 column = current_.column;

    expect(TokenType::For, "Expected 'for'");

    // 解析循环变量
    if (!current_.isName()) {
        error("Expected variable name after 'for'");
    }
    Str varName = getTokenString(current_);
    advance();

    // 判断是数值for还是泛型for
    if (match(static_cast<TokenType>('='))) {
        // 数值for: for i = start, limit, step do ... end
        ForNumStmt forStmt;
        forStmt.line = line;
        forStmt.column = column;
        forStmt.var = varName;
        forStmt.init = parseExpression();

        expect(static_cast<TokenType>(','), "Expected ',' after for init value");
        forStmt.limit = parseExpression();

        // step是可选的
        if (match(static_cast<TokenType>(','))) {
            forStmt.step = parseExpression();
        } else {
            // 默认step为1
            NumberExpr one;
            one.value = 1.0;
            one.line = current_.line;
            one.column = current_.column;
            forStmt.step = std::make_unique<Expr>(std::move(one));
        }

        expect(TokenType::Do, "Expected 'do' after for header");
        forStmt.body = parseBlock();
        expect(TokenType::End, "Expected 'end' to close for loop");

        return std::make_unique<Stmt>(std::move(forStmt));
    } else if (match(static_cast<TokenType>(','))) {
        // 泛型for: for var1, var2, ... in exp1, exp2, ... do ... end
        ForInStmt forStmt;
        forStmt.line = line;
        forStmt.column = column;
        forStmt.vars.push_back(varName);

        // 解析更多变量
        do {
            if (!current_.isName()) {
                error("Expected variable name in for-in loop");
            }
            forStmt.vars.push_back(getTokenString(current_));
            advance();
        } while (match(static_cast<TokenType>(',')));

        expect(TokenType::In, "Expected 'in' in for-in loop");
        forStmt.iterators = parseExprList();

        expect(TokenType::Do, "Expected 'do' after for-in header");
        forStmt.body = parseBlock();
        expect(TokenType::End, "Expected 'end' to close for-in loop");

        return std::make_unique<Stmt>(std::move(forStmt));
    } else if (check(TokenType::In)) {
        // 单变量泛型for
        ForInStmt forStmt;
        forStmt.line = line;
        forStmt.column = column;
        forStmt.vars.push_back(varName);

        expect(TokenType::In, "Expected 'in' in for-in loop");
        forStmt.iterators = parseExprList();

        expect(TokenType::Do, "Expected 'do' after for-in header");
        forStmt.body = parseBlock();
        expect(TokenType::End, "Expected 'end' to close for-in loop");

        return std::make_unique<Stmt>(std::move(forStmt));
    } else {
        error("Expected '=' or 'in' after for variable");
    }
}

StmtPtr Parser::parseFunctionStmt() {
    i32 line = current_.line;
    i32 column = current_.column;

    expect(TokenType::Function, "Expected 'function'");

    FunctionStmt funcStmt;
    funcStmt.line = line;
    funcStmt.column = column;
    funcStmt.isLocal = false;
    funcStmt.isMethod = false;

    // 解析函数名：支持 name, t.a.b.c.name, t:method
    if (!current_.isName()) {
        error("Expected function name");
    }

    // 第一个名字
    funcStmt.name = getTokenString(current_);
    advance();

    // 解析表路径和方法语法
    // function t.a.b.c.foo() 或 function t:method()
    while (check(static_cast<TokenType>('.')) || check(static_cast<TokenType>(':'))) {
        if (match(static_cast<TokenType>('.'))) {
            // 表成员访问
            funcStmt.tablePath.push_back(funcStmt.name);

            if (!current_.isName()) {
                error("Expected field name after '.'");
            }
            funcStmt.name = getTokenString(current_);
            advance();
        } else if (match(static_cast<TokenType>(':'))) {
            // 方法定义语法糖
            funcStmt.tablePath.push_back(funcStmt.name);
            funcStmt.isMethod = true;

            if (!current_.isName()) {
                error("Expected method name after ':'");
            }
            funcStmt.name = getTokenString(current_);
            advance();
            break;  // 冒号后不能再有点或冒号
        }
    }

    // 解析参数列表
    expect(static_cast<TokenType>('('), "Expected '(' after function name");
    funcStmt.params = parseParamList();
    expect(static_cast<TokenType>(')'), "Expected ')' after parameters");

    // 检查是否有可变参数（最后一个参数是 "..."）
    funcStmt.isVararg = false;
    if (!funcStmt.params.empty() && funcStmt.params.back() == "...") {
        funcStmt.isVararg = true;
        funcStmt.params.pop_back();  // 移除 "..." 参数名
    }

    // 如果是方法定义，自动在参数列表开头添加 self
    if (funcStmt.isMethod) {
        funcStmt.params.insert(funcStmt.params.begin(), "self");
    }

    // 解析函数体
    funcStmt.body = parseBlock();
    expect(TokenType::End, "Expected 'end' to close function");

    return std::make_unique<Stmt>(std::move(funcStmt));
}

StmtPtr Parser::parseLocalStmt() {
    i32 line = current_.line;
    i32 column = current_.column;

    expect(TokenType::Local, "Expected 'local'");

    // local function
    if (check(TokenType::Function)) {
        advance();

        FunctionStmt funcStmt;
        funcStmt.line = line;
        funcStmt.column = column;
        funcStmt.isLocal = true;
        funcStmt.isMethod = false;  // 局部函数不支持方法语法

        if (!current_.isName()) {
            error("Expected function name after 'local function'");
        }
        funcStmt.name = getTokenString(current_);
        advance();

        expect(static_cast<TokenType>('('), "Expected '(' after function name");
        funcStmt.params = parseParamList();
        expect(static_cast<TokenType>(')'), "Expected ')' after parameters");

        // 检查是否有可变参数
        funcStmt.isVararg = false;
        if (!funcStmt.params.empty() && funcStmt.params.back() == "...") {
            funcStmt.isVararg = true;
            funcStmt.params.pop_back();  // 移除 "..." 参数名
        }

        funcStmt.body = parseBlock();
        expect(TokenType::End, "Expected 'end' to close function");

        return std::make_unique<Stmt>(std::move(funcStmt));
    }

    // local var1, var2, ... = exp1, exp2, ...
    LocalStmt localStmt;
    localStmt.line = line;
    localStmt.column = column;

    // 解析变量名列表
    do {
        if (!current_.isName()) {
            error("Expected variable name in local statement");
        }
        localStmt.names.push_back(getTokenString(current_));
        advance();
    } while (match(static_cast<TokenType>(',')));

    // 解析初始值（可选）
    if (match(static_cast<TokenType>('='))) {
        localStmt.values = parseExprList();
    }

    return std::make_unique<Stmt>(std::move(localStmt));
}

StmtPtr Parser::parseReturnStmt() {
    i32 line = current_.line;
    i32 column = current_.column;

    expect(TokenType::Return, "Expected 'return'");

    ReturnStmt returnStmt;
    returnStmt.line = line;
    returnStmt.column = column;

    // 解析返回值列表（可选）
    if (!check(TokenType::End) &&
        !check(TokenType::Eos) &&
        !check(TokenType::Else) &&
        !check(TokenType::Elseif) &&
        !check(TokenType::Until)) {
        returnStmt.values = parseExprList();
    }

    return std::make_unique<Stmt>(std::move(returnStmt));
}

StmtPtr Parser::parseBreakStmt() {
    i32 line = current_.line;
    i32 column = current_.column;

    expect(TokenType::Break, "Expected 'break'");

    BreakStmt breakStmt;
    breakStmt.line = line;
    breakStmt.column = column;

    return std::make_unique<Stmt>(std::move(breakStmt));
}

StmtPtr Parser::parseExprStmt() {
    // 解析表达式
    ExprPtr expr = parseExpression();

    // 检查是否为赋值语句
    if (check(static_cast<TokenType>(','))) {
        // 多重赋值: var1, var2, ... = exp1, exp2, ...
        AssignStmt assignStmt;
        assignStmt.line = expr->getLine();
        assignStmt.column = expr->getColumn();
        assignStmt.targets.push_back(std::move(expr));

        // 解析更多左值
        while (match(static_cast<TokenType>(','))) {
            assignStmt.targets.push_back(parseExpression());
        }

        expect(static_cast<TokenType>('='), "Expected '=' in assignment");
        assignStmt.values = parseExprList();

        return std::make_unique<Stmt>(std::move(assignStmt));
    } else if (match(static_cast<TokenType>('='))) {
        // 单个赋值: var = exp
        AssignStmt assignStmt;
        assignStmt.line = expr->getLine();
        assignStmt.column = expr->getColumn();
        assignStmt.targets.push_back(std::move(expr));
        assignStmt.values = parseExprList();

        return std::make_unique<Stmt>(std::move(assignStmt));
    } else {
        // 函数调用语句
        CallStmt callStmt;
        callStmt.line = expr->getLine();
        callStmt.column = expr->getColumn();
        callStmt.call = std::move(expr);

        return std::make_unique<Stmt>(std::move(callStmt));
    }
}

// =====================================================================
// 表达式解析（按优先级从低到高）
// =====================================================================

ExprPtr Parser::parseExpression() {
    RecursionGuard guard(*this);  // 递归深度保护
    return parseOrExpr();
}

ExprPtr Parser::parseOrExpr() {
    ExprPtr left = parseAndExpr();

    while (match(TokenType::Or)) {
        i32 line = current_.line;
        i32 column = current_.column;

        BinaryExpr binExpr;
        binExpr.op = BinaryExpr::Op::Or;
        binExpr.left = std::move(left);
        binExpr.right = parseAndExpr();
        binExpr.line = line;
        binExpr.column = column;

        left = std::make_unique<Expr>(std::move(binExpr));
    }

    return left;
}

ExprPtr Parser::parseAndExpr() {
    ExprPtr left = parseRelationalExpr();

    while (match(TokenType::And)) {
        i32 line = current_.line;
        i32 column = current_.column;

        BinaryExpr binExpr;
        binExpr.op = BinaryExpr::Op::And;
        binExpr.left = std::move(left);
        binExpr.right = parseRelationalExpr();
        binExpr.line = line;
        binExpr.column = column;

        left = std::make_unique<Expr>(std::move(binExpr));
    }

    return left;
}

ExprPtr Parser::parseRelationalExpr() {
    ExprPtr left = parseConcatExpr();

    // 关系运算符: <, >, <=, >=, ==, ~=
    TokenType op = current_.type;
    if (op == static_cast<TokenType>('<') || op == static_cast<TokenType>('>') ||
        op == TokenType::Le || op == TokenType::Ge ||
        op == TokenType::Eq || op == TokenType::Ne) {

        i32 line = current_.line;
        i32 column = current_.column;
        advance();

        BinaryExpr binExpr;
        binExpr.line = line;
        binExpr.column = column;

        if (op == static_cast<TokenType>('<')) {
            binExpr.op = BinaryExpr::Op::Lt;
        } else if (op == static_cast<TokenType>('>')) {
            binExpr.op = BinaryExpr::Op::Gt;
        } else if (op == TokenType::Le) {
            binExpr.op = BinaryExpr::Op::Le;
        } else if (op == TokenType::Ge) {
            binExpr.op = BinaryExpr::Op::Ge;
        } else if (op == TokenType::Eq) {
            binExpr.op = BinaryExpr::Op::Eq;
        } else if (op == TokenType::Ne) {
            binExpr.op = BinaryExpr::Op::Ne;
        }

        binExpr.left = std::move(left);
        binExpr.right = parseConcatExpr();

        left = std::make_unique<Expr>(std::move(binExpr));
    }

    return left;
}

ExprPtr Parser::parseConcatExpr() {
    ExprPtr left = parseAdditiveExpr();

    // 字符串连接是右结合的
    if (match(TokenType::Concat)) {
        i32 line = current_.line;
        i32 column = current_.column;

        BinaryExpr binExpr;
        binExpr.op = BinaryExpr::Op::Concat;
        binExpr.left = std::move(left);
        binExpr.right = parseConcatExpr();  // 右结合
        binExpr.line = line;
        binExpr.column = column;

        left = std::make_unique<Expr>(std::move(binExpr));
    }

    return left;
}

ExprPtr Parser::parseAdditiveExpr() {
    ExprPtr left = parseMultiplicativeExpr();

    while (check(static_cast<TokenType>('+')) || check(static_cast<TokenType>('-'))) {
        i32 line = current_.line;
        i32 column = current_.column;
        TokenType op = current_.type;
        advance();

        BinaryExpr binExpr;
        binExpr.op = (op == static_cast<TokenType>('+')) ? BinaryExpr::Op::Add : BinaryExpr::Op::Sub;
        binExpr.left = std::move(left);
        binExpr.right = parseMultiplicativeExpr();
        binExpr.line = line;
        binExpr.column = column;

        left = std::make_unique<Expr>(std::move(binExpr));
    }

    return left;
}

ExprPtr Parser::parseMultiplicativeExpr() {
    ExprPtr left = parseUnaryExpr();

    while (check(static_cast<TokenType>('*')) ||
           check(static_cast<TokenType>('/')) ||
           check(static_cast<TokenType>('%'))) {

        i32 line = current_.line;
        i32 column = current_.column;
        TokenType op = current_.type;
        advance();

        BinaryExpr binExpr;
        binExpr.line = line;
        binExpr.column = column;

        if (op == static_cast<TokenType>('*')) {
            binExpr.op = BinaryExpr::Op::Mul;
        } else if (op == static_cast<TokenType>('/')) {
            binExpr.op = BinaryExpr::Op::Div;
        } else {
            binExpr.op = BinaryExpr::Op::Mod;
        }

        binExpr.left = std::move(left);
        binExpr.right = parseUnaryExpr();

        left = std::make_unique<Expr>(std::move(binExpr));
    }

    return left;
}

ExprPtr Parser::parseUnaryExpr() {
    // 一元运算符: not, -, #
    if (match(TokenType::Not)) {
        i32 line = current_.line;
        i32 column = current_.column;

        UnaryExpr unExpr;
        unExpr.op = UnaryExpr::Op::Not;
        unExpr.operand = parseUnaryExpr();
        unExpr.line = line;
        unExpr.column = column;

        return std::make_unique<Expr>(std::move(unExpr));
    } else if (match(static_cast<TokenType>('-'))) {
        i32 line = current_.line;
        i32 column = current_.column;

        UnaryExpr unExpr;
        unExpr.op = UnaryExpr::Op::Neg;
        unExpr.operand = parseUnaryExpr();
        unExpr.line = line;
        unExpr.column = column;

        return std::make_unique<Expr>(std::move(unExpr));
    } else if (match(static_cast<TokenType>('#'))) {
        i32 line = current_.line;
        i32 column = current_.column;

        UnaryExpr unExpr;
        unExpr.op = UnaryExpr::Op::Len;
        unExpr.operand = parseUnaryExpr();
        unExpr.line = line;
        unExpr.column = column;

        return std::make_unique<Expr>(std::move(unExpr));
    }

    return parsePowerExpr();
}

ExprPtr Parser::parsePowerExpr() {
    ExprPtr left = parsePrimaryExpr();

    // 幂运算是右结合的
    if (match(static_cast<TokenType>('^'))) {
        i32 line = current_.line;
        i32 column = current_.column;

        BinaryExpr binExpr;
        binExpr.op = BinaryExpr::Op::Pow;
        binExpr.left = std::move(left);
        binExpr.right = parsePowerExpr();  // 右结合
        binExpr.line = line;
        binExpr.column = column;

        left = std::make_unique<Expr>(std::move(binExpr));
    }

    return left;
}

ExprPtr Parser::parsePrimaryExpr() {
    i32 line = current_.line;
    i32 column = current_.column;

    // nil
    if (match(TokenType::Nil)) {
        NilExpr nilExpr;
        nilExpr.line = line;
        nilExpr.column = column;
        return parsePostfixExpr(std::make_unique<Expr>(std::move(nilExpr)));
    }

    // true
    if (match(TokenType::True)) {
        BoolExpr boolExpr;
        boolExpr.value = true;
        boolExpr.line = line;
        boolExpr.column = column;
        return parsePostfixExpr(std::make_unique<Expr>(std::move(boolExpr)));
    }

    // false
    if (match(TokenType::False)) {
        BoolExpr boolExpr;
        boolExpr.value = false;
        boolExpr.line = line;
        boolExpr.column = column;
        return parsePostfixExpr(std::make_unique<Expr>(std::move(boolExpr)));
    }

    // 数字
    if (current_.isNumber()) {
        NumberExpr numExpr;
        numExpr.value = std::get<f64>(current_.value);
        numExpr.line = line;
        numExpr.column = column;
        advance();
        return parsePostfixExpr(std::make_unique<Expr>(std::move(numExpr)));
    }

    // 字符串
    if (current_.isString()) {
        StringExpr strExpr;
        strExpr.value = getTokenString(current_);
        strExpr.line = line;
        strExpr.column = column;
        advance();
        return parsePostfixExpr(std::make_unique<Expr>(std::move(strExpr)));
    }

    // ...（变长参数）
    if (match(TokenType::Dots)) {
        VarargExpr varargExpr;
        varargExpr.line = line;
        varargExpr.column = column;
        return parsePostfixExpr(std::make_unique<Expr>(std::move(varargExpr)));
    }

    // 表构造器
    if (check(static_cast<TokenType>('{'))) {
        return parsePostfixExpr(parseTableConstructor());
    }

    // 函数定义
    if (check(TokenType::Function)) {
        return parsePostfixExpr(parseFunctionExpr());
    }

    // 括号表达式
    if (match(static_cast<TokenType>('('))) {
        ExprPtr expr = parseExpression();
        expect(static_cast<TokenType>(')'), "Expected ')' after expression");
        return parsePostfixExpr(std::move(expr));
    }

    // 标识符
    if (current_.isName()) {
        NameExpr nameExpr;
        nameExpr.name = getTokenString(current_);
        nameExpr.line = line;
        nameExpr.column = column;
        advance();
        return parsePostfixExpr(std::make_unique<Expr>(std::move(nameExpr)));
    }

    error("Unexpected token in expression");
}

ExprPtr Parser::parsePostfixExpr(ExprPtr base) {
    while (true) {
        i32 line = current_.line;
        i32 column = current_.column;

        // 函数调用: func(args)
        if (match(static_cast<TokenType>('('))) {
            CallExpr callExpr;
            callExpr.func = std::move(base);
            callExpr.line = line;
            callExpr.column = column;

            // 解析参数列表
            if (!check(static_cast<TokenType>(')'))) {
                callExpr.args = parseExprList();
            }

            expect(static_cast<TokenType>(')'), "Expected ')' after arguments");
            base = std::make_unique<Expr>(std::move(callExpr));
        }
        // 索引访问: table[key]
        else if (match(static_cast<TokenType>('['))) {
            IndexExpr indexExpr;
            indexExpr.table = std::move(base);
            indexExpr.index = parseExpression();
            indexExpr.line = line;
            indexExpr.column = column;

            expect(static_cast<TokenType>(']'), "Expected ']' after index");
            base = std::make_unique<Expr>(std::move(indexExpr));
        }
        // 成员访问: table.member
        else if (match(static_cast<TokenType>('.'))) {
            if (!current_.isName()) {
                error("Expected member name after '.'");
            }

            MemberExpr memberExpr;
            memberExpr.table = std::move(base);
            memberExpr.member = getTokenString(current_);
            memberExpr.line = line;
            memberExpr.column = column;
            advance();

            base = std::make_unique<Expr>(std::move(memberExpr));
        }
        // 方法调用: obj:method(args)
        else if (match(static_cast<TokenType>(':'))) {
            if (!current_.isName()) {
                error("Expected method name after ':'");
            }

            Str methodName = getTokenString(current_);
            advance();

            // 创建成员访问
            MemberExpr memberExpr;
            memberExpr.table = std::move(base);
            memberExpr.member = methodName;
            memberExpr.line = line;
            memberExpr.column = column;

            ExprPtr method = std::make_unique<Expr>(std::move(memberExpr));

            // 创建函数调用
            expect(static_cast<TokenType>('('), "Expected '(' after method name");

            CallExpr callExpr;
            callExpr.func = std::move(method);
            callExpr.isMethodCall = true;  // 标记为方法调用
            callExpr.line = line;
            callExpr.column = column;

            if (!check(static_cast<TokenType>(')'))) {
                callExpr.args = parseExprList();
            }

            expect(static_cast<TokenType>(')'), "Expected ')' after arguments");
            base = std::make_unique<Expr>(std::move(callExpr));
        }
        // 函数调用语法糖: f"string" 等价于 f("string")
        else if (current_.isString()) {
            CallExpr callExpr;
            callExpr.func = std::move(base);
            callExpr.line = line;
            callExpr.column = column;

            // 创建字符串参数
            StringExpr strExpr;
            strExpr.value = getTokenString(current_);
            strExpr.line = current_.line;
            strExpr.column = current_.column;
            advance();

            callExpr.args.push_back(std::make_unique<Expr>(std::move(strExpr)));
            base = std::make_unique<Expr>(std::move(callExpr));
        }
        // 函数调用语法糖: f{table} 等价于 f({table})
        else if (check(static_cast<TokenType>('{'))) {
            CallExpr callExpr;
            callExpr.func = std::move(base);
            callExpr.line = line;
            callExpr.column = column;

            // 解析表构造器作为参数
            callExpr.args.push_back(parseTableConstructor());
            base = std::make_unique<Expr>(std::move(callExpr));
        }
        else {
            break;
        }
    }

    return base;
}

ExprPtr Parser::parseTableConstructor() {
    RecursionGuard guard(*this);  // 递归深度保护

    i32 line = current_.line;
    i32 column = current_.column;

    expect(static_cast<TokenType>('{'), "Expected '{'");

    TableExpr tableExpr;
    tableExpr.line = line;
    tableExpr.column = column;

    while (!check(static_cast<TokenType>('}'))) {
        TableField field;

        // [key] = value
        if (match(static_cast<TokenType>('['))) {
            field.key = parseExpression();
            expect(static_cast<TokenType>(']'), "Expected ']' after table key");
            expect(static_cast<TokenType>('='), "Expected '=' after table key");
            field.value = parseExpression();
        }
        // name = value 或数组元素
        else if (current_.isName()) {
            // 使用前瞻判断是 name = value 还是数组元素
            Token nextToken = peek();

            if (nextToken.type == static_cast<TokenType>('=')) {
                // name = value 形式
                Str name = getTokenString(current_);
                i32 nameLine = current_.line;
                i32 nameColumn = current_.column;
                advance();  // 消费 name

                StringExpr keyExpr;
                keyExpr.value = name;
                keyExpr.line = nameLine;
                keyExpr.column = nameColumn;
                field.key = std::make_unique<Expr>(std::move(keyExpr));

                advance();  // 消费 '='
                field.value = parseExpression();
            } else {
                // 数组元素，解析完整表达式
                field.key = nullptr;
                field.value = parseExpression();
            }
        }
        // 数组元素
        else {
            field.key = nullptr;
            field.value = parseExpression();
        }

        tableExpr.fields.push_back(std::move(field));

        // 字段分隔符: , 或 ;
        if (!match(static_cast<TokenType>(','))) {
            match(static_cast<TokenType>(';'));
        }

        // 允许尾随分隔符
        if (check(static_cast<TokenType>('}'))) {
            break;
        }
    }

    expect(static_cast<TokenType>('}'), "Expected '}' to close table constructor");

    return std::make_unique<Expr>(std::move(tableExpr));
}

ExprPtr Parser::parseFunctionExpr() {
    RecursionGuard guard(*this);  // 递归深度保护

    i32 line = current_.line;
    i32 column = current_.column;

    expect(TokenType::Function, "Expected 'function'");

    FunctionExpr funcExpr;
    funcExpr.line = line;
    funcExpr.column = column;

    // 解析参数列表
    expect(static_cast<TokenType>('('), "Expected '(' after 'function'");
    funcExpr.params = parseParamList();
    expect(static_cast<TokenType>(')'), "Expected ')' after parameters");

    // 检查是否有可变参数
    funcExpr.isVararg = false;
    if (!funcExpr.params.empty() && funcExpr.params.back() == "...") {
        funcExpr.isVararg = true;
        funcExpr.params.pop_back();  // 移除 "..." 参数名
    }

    // 解析函数体
    funcExpr.body = parseBlock();
    expect(TokenType::End, "Expected 'end' to close function");

    return std::make_unique<Expr>(std::move(funcExpr));
}

// =====================================================================
// 辅助函数
// =====================================================================

Vec<Str> Parser::parseParamList() {
    Vec<Str> params;

    // 空参数列表
    if (check(static_cast<TokenType>(')'))) {
        return params;
    }

    // 变长参数
    if (match(TokenType::Dots)) {
        params.push_back("...");
        return params;
    }

    // 解析参数名
    do {
        if (current_.isName()) {
            params.push_back(getTokenString(current_));
            advance();
        } else if (match(TokenType::Dots)) {
            params.push_back("...");
            break;  // ... 必须是最后一个参数
        } else {
            error("Expected parameter name");
        }
    } while (match(static_cast<TokenType>(',')));

    return params;
}

Vec<ExprPtr> Parser::parseExprList() {
    Vec<ExprPtr> exprs;

    do {
        exprs.push_back(parseExpression());
    } while (match(static_cast<TokenType>(',')));

    return exprs;
}

} // namespace Lua


