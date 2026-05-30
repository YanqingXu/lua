/**
 * @file parser_stmt.cpp
 * @brief Lua语句与语句块解析实现
 *
 * 实现语句块、控制流语句、局部声明、返回语句、赋值语句和函数调用语句解析。
 */

#include "parser_impl.hpp"
#include "parser_utils.hpp"

#include <utility>

namespace Lua {

Vec<StmtPtr> Parser::Impl::parseBlock() {
    RecursionGuard guard(*this, MAX_BLOCK_RECURSION_DEPTH);

    Vec<StmtPtr> statements;
    bool mayConsumeSeparator = false;

    while (!check(TokenType::Eos) &&
           !check(TokenType::End) &&
           !check(TokenType::Else) &&
           !check(TokenType::Elseif) &&
           !check(TokenType::Until)) {
        try {
            if (check(static_cast<TokenType>(';'))) {
                if (!mayConsumeSeparator) {
                    errorAt(current(), "unexpected symbol");
                }
                advance();
                mayConsumeSeparator = false;
                continue;
            }

            if (check(TokenType::Return)) {
                StmtPtr stmt = parseReturnStmt();
                if (stmt) {
                    statements.push_back(std::move(stmt));
                }
                break;
            }

            StmtPtr stmt = parseStatement();
            if (stmt) {
                statements.push_back(std::move(stmt));
                mayConsumeSeparator = true;
            }
        } catch (const ParseError& error) {
            if (!canRecoverFrom(error)) {
                throw;
            }
            recoverAfterError();
        }
    }

    return statements;
}

StmtPtr Parser::Impl::parseStatement() {
    switch (current().type) {
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
            return parseExprStmt();
    }
}

StmtPtr Parser::Impl::parseIfStmt() {
    i32 line = current().line;
    i32 column = current().column;

    expect(TokenType::If, "Expected 'if'");

    IfStmt ifStmt;
    ifStmt.line = line;
    ifStmt.column = column;

    IfStmt::Branch ifBranch;
    ifBranch.condition = parseExpression();
    expect(TokenType::Then, "Expected 'then' after if condition");
    ifBranch.body = parseBlock();
    ifStmt.branches.push_back(std::move(ifBranch));

    while (match(TokenType::Elseif)) {
        IfStmt::Branch elseifBranch;
        elseifBranch.condition = parseExpression();
        expect(TokenType::Then, "Expected 'then' after elseif condition");
        elseifBranch.body = parseBlock();
        ifStmt.branches.push_back(std::move(elseifBranch));
    }

    if (match(TokenType::Else)) {
        ifStmt.elseBranch = parseBlock();
    }

    ifStmt.endLine = current().line;
    expect(TokenType::End, "Expected 'end' to close if statement");

    return makeStmt<IfStmt>(std::move(ifStmt));
}

StmtPtr Parser::Impl::parseWhileStmt() {
    i32 line = current().line;
    i32 column = current().column;

    expect(TokenType::While, "Expected 'while'");

    WhileStmt whileStmt;
    whileStmt.line = line;
    whileStmt.column = column;
    whileStmt.condition = parseExpression();

    expect(TokenType::Do, "Expected 'do' after while condition");
    whileStmt.body = parseBlock();
    whileStmt.endLine = current().line;
    expect(TokenType::End, "Expected 'end' to close while loop");

    return makeStmt<WhileStmt>(std::move(whileStmt));
}

StmtPtr Parser::Impl::parseDoStmt() {
    i32 line = current().line;
    i32 column = current().column;

    expect(TokenType::Do, "Expected 'do'");

    DoStmt doStmt;
    doStmt.line = line;
    doStmt.column = column;
    doStmt.body = parseBlock();

    doStmt.endLine = current().line;
    expect(TokenType::End, "Expected 'end' to close do block");

    return makeStmt<DoStmt>(std::move(doStmt));
}

StmtPtr Parser::Impl::parseRepeatStmt() {
    i32 line = current().line;
    i32 column = current().column;

    expect(TokenType::Repeat, "Expected 'repeat'");

    RepeatStmt repeatStmt;
    repeatStmt.line = line;
    repeatStmt.column = column;
    repeatStmt.body = parseBlock();

    repeatStmt.endLine = current().line;
    expect(TokenType::Until, "Expected 'until' to close repeat loop");
    repeatStmt.condition = parseExpression();

    return makeStmt<RepeatStmt>(std::move(repeatStmt));
}

StmtPtr Parser::Impl::parseForStmt() {
    i32 line = current().line;
    i32 column = current().column;

    expect(TokenType::For, "Expected 'for'");

    if (!current().isName()) {
        error("Expected variable name after 'for'");
    }
    Str varName(ParserUtils::tokenString(current()));
    advance();

    if (match(static_cast<TokenType>('='))) {
        ForNumStmt forStmt;
        forStmt.line = line;
        forStmt.column = column;
        forStmt.var = std::move(varName);
        forStmt.init = parseExpression();

        expect(static_cast<TokenType>(','), "Expected ',' after for init value");
        forStmt.limit = parseExpression();

        if (match(static_cast<TokenType>(','))) {
            forStmt.step = parseExpression();
        } else {
            NumberExpr one;
            one.value = 1.0;
            one.line = current().line;
            one.column = current().column;
            forStmt.step = makeExpr<NumberExpr>(std::move(one));
        }

        expect(TokenType::Do, "Expected 'do' after for header");
        forStmt.body = parseBlock();
        forStmt.endLine = current().line;
        expect(TokenType::End, "Expected 'end' to close for loop");

        return makeStmt<ForNumStmt>(std::move(forStmt));
    } else if (match(static_cast<TokenType>(','))) {
        ForInStmt forStmt;
        forStmt.line = line;
        forStmt.column = column;
        forStmt.vars.push_back(std::move(varName));

        do {
            if (!current().isName()) {
                error("Expected variable name in for-in loop");
            }
            forStmt.vars.emplace_back(ParserUtils::tokenString(current()));
            advance();
        } while (match(static_cast<TokenType>(',')));

        expect(TokenType::In, "Expected 'in' in for-in loop");
        forStmt.iterators = parseExprList();

        expect(TokenType::Do, "Expected 'do' after for-in header");
        forStmt.body = parseBlock();
        forStmt.endLine = current().line;
        expect(TokenType::End, "Expected 'end' to close for-in loop");

        return makeStmt<ForInStmt>(std::move(forStmt));
    } else if (check(TokenType::In)) {
        ForInStmt forStmt;
        forStmt.line = line;
        forStmt.column = column;
        forStmt.vars.push_back(std::move(varName));

        expect(TokenType::In, "Expected 'in' in for-in loop");
        forStmt.iterators = parseExprList();

        expect(TokenType::Do, "Expected 'do' after for-in header");
        forStmt.body = parseBlock();
        forStmt.endLine = current().line;
        expect(TokenType::End, "Expected 'end' to close for-in loop");

        return makeStmt<ForInStmt>(std::move(forStmt));
    } else {
        error("Expected '=' or 'in' after for variable");
		return nullptr;
    }
}

StmtPtr Parser::Impl::parseLocalStmt() {
    i32 line = current().line;
    i32 column = current().column;

    expect(TokenType::Local, "Expected 'local'");

    if (check(TokenType::Function)) {
        advance();

        FunctionStmt funcStmt;
        funcStmt.line = line;
        funcStmt.column = column;
        funcStmt.isLocal = true;
        funcStmt.isMethod = false;

        if (!current().isName()) {
            error("Expected function name after 'local function'");
        }
        funcStmt.name = Str(ParserUtils::tokenString(current()));
        declareLocalName(funcStmt.name, current());
        advance();

        expect(static_cast<TokenType>('('), "Expected '(' after function name");
        funcStmt.params = parseParamList();
        expect(static_cast<TokenType>(')'), "Expected ')' after parameters");

        funcStmt.isVararg = false;
        if (!funcStmt.params.empty() && funcStmt.params.back() == "...") {
            funcStmt.isVararg = true;
            funcStmt.params.pop_back();
        }

        enterFunctionSyntaxScope(line, funcStmt.params);
        funcStmt.body = parseBlock();
        leaveFunctionSyntaxScope();
        funcStmt.endLine = current().line;
        expect(TokenType::End, "Expected 'end' to close function");

        return makeStmt<FunctionStmt>(std::move(funcStmt));
    }

    LocalStmt localStmt;
    localStmt.line = line;
    localStmt.column = column;

    do {
        if (!current().isName()) {
            error("Expected variable name in local statement");
        }
        Str name(ParserUtils::tokenString(current()));
        declareLocalName(name, current());
        localStmt.names.push_back(std::move(name));
        advance();
    } while (match(static_cast<TokenType>(',')));

    if (match(static_cast<TokenType>('='))) {
        localStmt.values = parseExprList();
    }

    return makeStmt<LocalStmt>(std::move(localStmt));
}

StmtPtr Parser::Impl::parseReturnStmt() {
    i32 line = current().line;
    i32 column = current().column;

    expect(TokenType::Return, "Expected 'return'");

    ReturnStmt returnStmt;
    returnStmt.line = line;
    returnStmt.column = column;

    if (!check(TokenType::End) &&
        !check(TokenType::Eos) &&
        !check(TokenType::Else) &&
        !check(TokenType::Elseif) &&
        !check(TokenType::Until) &&
        !check(static_cast<TokenType>(';'))) {
        returnStmt.values = parseExprList();
    }

    match(static_cast<TokenType>(';'));

    return makeStmt<ReturnStmt>(std::move(returnStmt));
}

StmtPtr Parser::Impl::parseBreakStmt() {
    i32 line = current().line;
    i32 column = current().column;

    expect(TokenType::Break, "Expected 'break'");

    BreakStmt breakStmt;
    breakStmt.line = line;
    breakStmt.column = column;

    return makeStmt<BreakStmt>(std::move(breakStmt));
}

StmtPtr Parser::Impl::parseExprStmt() {
    Token firstToken = current();

    ExprPtr expr = parseExpression();

    if (check(static_cast<TokenType>(','))) {
        AssignStmt assignStmt;
        assignStmt.line = expr->getLine();
        assignStmt.column = expr->getColumn();
        assignStmt.targets.push_back(std::move(expr));

        while (match(static_cast<TokenType>(','))) {
            assignStmt.targets.push_back(parseExpression());
        }

        expect(static_cast<TokenType>('='), "Expected '=' in assignment");
        assignStmt.values = parseExprList();

        return makeStmt<AssignStmt>(std::move(assignStmt));
    } else if (match(static_cast<TokenType>('='))) {
        AssignStmt assignStmt;
        assignStmt.line = expr->getLine();
        assignStmt.column = expr->getColumn();
        assignStmt.targets.push_back(std::move(expr));
        assignStmt.values = parseExprList();

        return makeStmt<AssignStmt>(std::move(assignStmt));
    } else {
        if (!std::holds_alternative<CallExpr>(expr->variant)) {
            const Token& errorToken = check(TokenType::Eos) ? firstToken : current();
            errorAt(errorToken, "unexpected symbol");
            return nullptr;
        }

        CallStmt callStmt;
        callStmt.line = expr->getLine();
        callStmt.column = expr->getColumn();
        callStmt.call = std::move(expr);

        return makeStmt<CallStmt>(std::move(callStmt));
    }
}

}
