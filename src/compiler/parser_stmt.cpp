/**
 * @file parser_stmt.cpp
 * @brief Lua Parser statement and block implementation.
 */

#include "parser.hpp"

#include <utility>

namespace Lua {

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

    return makeStmt<IfStmt>(std::move(ifStmt));
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

    return makeStmt<WhileStmt>(std::move(whileStmt));
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

    return makeStmt<DoStmt>(std::move(doStmt));
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

    return makeStmt<RepeatStmt>(std::move(repeatStmt));
}

StmtPtr Parser::parseForStmt() {
    i32 line = current_.line;
    i32 column = current_.column;

    expect(TokenType::For, "Expected 'for'");

    // 解析循环变量
    if (!current_.isName()) {
        error("Expected variable name after 'for'");
    }
    Str varName = tokenString(current_);
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
            forStmt.step = makeExpr<NumberExpr>(std::move(one));
        }

        expect(TokenType::Do, "Expected 'do' after for header");
        forStmt.body = parseBlock();
        expect(TokenType::End, "Expected 'end' to close for loop");

        return makeStmt<ForNumStmt>(std::move(forStmt));
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
            forStmt.vars.push_back(tokenString(current_));
            advance();
        } while (match(static_cast<TokenType>(',')));

        expect(TokenType::In, "Expected 'in' in for-in loop");
        forStmt.iterators = parseExprList();

        expect(TokenType::Do, "Expected 'do' after for-in header");
        forStmt.body = parseBlock();
        expect(TokenType::End, "Expected 'end' to close for-in loop");

        return makeStmt<ForInStmt>(std::move(forStmt));
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

        return makeStmt<ForInStmt>(std::move(forStmt));
    } else {
        error("Expected '=' or 'in' after for variable");
		return nullptr;  // 永远不会到达
    }
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
        funcStmt.name = tokenString(current_);
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

        return makeStmt<FunctionStmt>(std::move(funcStmt));
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
        localStmt.names.push_back(tokenString(current_));
        advance();
    } while (match(static_cast<TokenType>(',')));

    // 解析初始值（可选）
    if (match(static_cast<TokenType>('='))) {
        localStmt.values = parseExprList();
    }

    return makeStmt<LocalStmt>(std::move(localStmt));
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

    return makeStmt<ReturnStmt>(std::move(returnStmt));
}

StmtPtr Parser::parseBreakStmt() {
    i32 line = current_.line;
    i32 column = current_.column;

    expect(TokenType::Break, "Expected 'break'");

    BreakStmt breakStmt;
    breakStmt.line = line;
    breakStmt.column = column;

    return makeStmt<BreakStmt>(std::move(breakStmt));
}

StmtPtr Parser::parseExprStmt() {
    // 保存第一个 token，用于错误报告
    // 参考官方 Lua 5.1.5 的 "unexpected symbol near 'X'" 格式
    Token firstToken = current_;

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

        return makeStmt<AssignStmt>(std::move(assignStmt));
    } else if (match(static_cast<TokenType>('='))) {
        // 单个赋值: var = exp
        AssignStmt assignStmt;
        assignStmt.line = expr->getLine();
        assignStmt.column = expr->getColumn();
        assignStmt.targets.push_back(std::move(expr));
        assignStmt.values = parseExprList();

        return makeStmt<AssignStmt>(std::move(assignStmt));
    } else {
        // 只有函数调用才能作为表达式语句
        // 参考官方 Lua 5.1.5: lparser.c exprstat() 函数
        // 其他表达式（如 1+2）不是有效的语句
        if (!std::holds_alternative<CallExpr>(expr->variant)) {
            // 使用官方 Lua 风格的错误消息：unexpected symbol near 'X'
            Str errorMsg = errorWithNear("unexpected symbol", firstToken);
            throw ParseError(errorMsg, firstToken.line, firstToken.column);
        }

        CallStmt callStmt;
        callStmt.line = expr->getLine();
        callStmt.column = expr->getColumn();
        callStmt.call = std::move(expr);

        return makeStmt<CallStmt>(std::move(callStmt));
    }
}

} // namespace Lua
