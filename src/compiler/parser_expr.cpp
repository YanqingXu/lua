/**
 * @file parser_expr.cpp
 * @brief Lua Parser expression precedence implementation.
 */

#include "parser.hpp"

#include <utility>

namespace Lua {

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

        left = makeExpr<BinaryExpr>(std::move(binExpr));
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

        left = makeExpr<BinaryExpr>(std::move(binExpr));
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

        left = makeExpr<BinaryExpr>(std::move(binExpr));
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

        left = makeExpr<BinaryExpr>(std::move(binExpr));
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

        left = makeExpr<BinaryExpr>(std::move(binExpr));
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

        left = makeExpr<BinaryExpr>(std::move(binExpr));
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

        return makeExpr<UnaryExpr>(std::move(unExpr));
    } else if (match(static_cast<TokenType>('-'))) {
        i32 line = current_.line;
        i32 column = current_.column;

        UnaryExpr unExpr;
        unExpr.op = UnaryExpr::Op::Neg;
        unExpr.operand = parseUnaryExpr();
        unExpr.line = line;
        unExpr.column = column;

        return makeExpr<UnaryExpr>(std::move(unExpr));
    } else if (match(static_cast<TokenType>('#'))) {
        i32 line = current_.line;
        i32 column = current_.column;

        UnaryExpr unExpr;
        unExpr.op = UnaryExpr::Op::Len;
        unExpr.operand = parseUnaryExpr();
        unExpr.line = line;
        unExpr.column = column;

        return makeExpr<UnaryExpr>(std::move(unExpr));
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

        left = makeExpr<BinaryExpr>(std::move(binExpr));
    }

    return left;
}

Vec<ExprPtr> Parser::parseExprList() {
    Vec<ExprPtr> exprs;

    do {
        exprs.push_back(parseExpression());
    } while (match(static_cast<TokenType>(',')));

    return exprs;
}

} // namespace Lua
