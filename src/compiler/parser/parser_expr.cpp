/**
 * @file parser_expr.cpp
 * @brief Lua表达式优先级链解析实现
 *
 * 实现逻辑、关系、连接、算术、一元和幂运算等表达式优先级规则。
 */

#include "parser_impl.hpp"

#include <utility>

namespace Lua {

ExprPtr Parser::Impl::parseExpression() {
    RecursionGuard guard(*this);
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

    if (check(TokenType::Concat)) {
        Token opToken = current();
        advance();

        ExprPtr right = parseConcatExpr();
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

    if (check(static_cast<TokenType>('^'))) {
        Token opToken = current();
        advance();

        ExprPtr right = parsePowerExpr();
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

}
