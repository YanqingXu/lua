/**
 * @file parser_expr.cpp
 * @brief Lua Parser expression precedence implementation.
 */

#include "parser_impl.hpp"

#include <utility>

namespace Lua {

ExprPtr Parser::Impl::parseExpression() {
    RecursionGuard guard(*this);  // 递归深度保护
    return parseOrExpr();
}

ExprPtr Parser::Impl::parseOrExpr() {
    ExprPtr left = parseAndExpr();

    while (check(TokenType::Or)) {
        Token opToken = current();
        advance();

        ExprPtr right = parseAndExpr();
        left = makeBinaryExpr(BinaryExpr::Op::Or, opToken, std::move(left), std::move(right));
    }

    return left;
}

ExprPtr Parser::Impl::parseAndExpr() {
    ExprPtr left = parseRelationalExpr();

    while (check(TokenType::And)) {
        Token opToken = current();
        advance();

        ExprPtr right = parseRelationalExpr();
        left = makeBinaryExpr(BinaryExpr::Op::And, opToken, std::move(left), std::move(right));
    }

    return left;
}

ExprPtr Parser::Impl::parseRelationalExpr() {
    ExprPtr left = parseConcatExpr();

    // 关系运算符: <, >, <=, >=, ==, ~=
    TokenType op = current().type;
    if (op == static_cast<TokenType>('<') || op == static_cast<TokenType>('>') ||
        op == TokenType::Le || op == TokenType::Ge ||
        op == TokenType::Eq || op == TokenType::Ne) {

        Token opToken = current();
        advance();

        BinaryExpr::Op binaryOp = BinaryExpr::Op::Eq;

        if (op == static_cast<TokenType>('<')) {
            binaryOp = BinaryExpr::Op::Lt;
        } else if (op == static_cast<TokenType>('>')) {
            binaryOp = BinaryExpr::Op::Gt;
        } else if (op == TokenType::Le) {
            binaryOp = BinaryExpr::Op::Le;
        } else if (op == TokenType::Ge) {
            binaryOp = BinaryExpr::Op::Ge;
        } else if (op == TokenType::Eq) {
            binaryOp = BinaryExpr::Op::Eq;
        } else if (op == TokenType::Ne) {
            binaryOp = BinaryExpr::Op::Ne;
        }

        ExprPtr right = parseConcatExpr();
        left = makeBinaryExpr(binaryOp, opToken, std::move(left), std::move(right));
    }

    return left;
}

ExprPtr Parser::Impl::parseConcatExpr() {
    ExprPtr left = parseAdditiveExpr();

    // 字符串连接是右结合的
    if (check(TokenType::Concat)) {
        Token opToken = current();
        advance();

        ExprPtr right = parseConcatExpr();  // 右结合
        left = makeBinaryExpr(BinaryExpr::Op::Concat, opToken, std::move(left), std::move(right));
    }

    return left;
}

ExprPtr Parser::Impl::parseAdditiveExpr() {
    ExprPtr left = parseMultiplicativeExpr();

    while (check(static_cast<TokenType>('+')) || check(static_cast<TokenType>('-'))) {
        Token opToken = current();
        TokenType op = opToken.type;
        advance();

        BinaryExpr::Op binaryOp = (op == static_cast<TokenType>('+')) ? BinaryExpr::Op::Add : BinaryExpr::Op::Sub;
        ExprPtr right = parseMultiplicativeExpr();
        left = makeBinaryExpr(binaryOp, opToken, std::move(left), std::move(right));
    }

    return left;
}

ExprPtr Parser::Impl::parseMultiplicativeExpr() {
    ExprPtr left = parseUnaryExpr();

    while (check(static_cast<TokenType>('*')) ||
           check(static_cast<TokenType>('/')) ||
           check(static_cast<TokenType>('%'))) {

        Token opToken = current();
        TokenType op = opToken.type;
        advance();

        BinaryExpr::Op binaryOp = BinaryExpr::Op::Mod;

        if (op == static_cast<TokenType>('*')) {
            binaryOp = BinaryExpr::Op::Mul;
        } else if (op == static_cast<TokenType>('/')) {
            binaryOp = BinaryExpr::Op::Div;
        }

        ExprPtr right = parseUnaryExpr();
        left = makeBinaryExpr(binaryOp, opToken, std::move(left), std::move(right));
    }

    return left;
}

ExprPtr Parser::Impl::parseUnaryExpr() {
    // 一元运算符: not, -, #
    if (check(TokenType::Not)) {
        Token opToken = current();
        advance();

        ExprPtr operand = parseUnaryExpr();
        return makeUnaryExpr(UnaryExpr::Op::Not, opToken, std::move(operand));
    } else if (check(static_cast<TokenType>('-'))) {
        Token opToken = current();
        advance();

        ExprPtr operand = parseUnaryExpr();
        return makeUnaryExpr(UnaryExpr::Op::Neg, opToken, std::move(operand));
    } else if (check(static_cast<TokenType>('#'))) {
        Token opToken = current();
        advance();

        ExprPtr operand = parseUnaryExpr();
        return makeUnaryExpr(UnaryExpr::Op::Len, opToken, std::move(operand));
    }

    return parsePowerExpr();
}

ExprPtr Parser::Impl::parsePowerExpr() {
    ExprPtr left = parsePrimaryExpr();

    // 幂运算是右结合的
    if (check(static_cast<TokenType>('^'))) {
        Token opToken = current();
        advance();

        ExprPtr right = parsePowerExpr();  // 右结合
        left = makeBinaryExpr(BinaryExpr::Op::Pow, opToken, std::move(left), std::move(right));
    }

    return left;
}

Vec<ExprPtr> Parser::Impl::parseExprList() {
    Vec<ExprPtr> exprs;

    do {
        exprs.push_back(parseExpression());
    } while (match(static_cast<TokenType>(',')));

    return exprs;
}

} // namespace Lua
