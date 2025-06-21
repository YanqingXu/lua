#include "parser.hpp"

namespace Lua {
    UPtr<Stmt> Parser::statement() {
        if (match(TokenType::Local)) {
            return localDeclaration();
        }
    
        if (match(TokenType::If)) {
            return ifStatement();
        }
    
        if (match(TokenType::While)) {
            return whileStatement();
        }
    
        if (match(TokenType::For)) {
            return forStatement();
        }
    
        if (match(TokenType::Repeat)) {
            return repeatUntilStatement();
        }
    
        if (match(TokenType::Return)) {
            return returnStatement();
        }
    
        if (match(TokenType::Break)) {
            return breakStatement();
        }

        if (match(TokenType::Function)) {
            return functionStatement();
        }

        return assignmentStatement();
    }

    UPtr<Stmt> Parser::expressionStatement() {
        auto expr = expression();
        match(TokenType::Semicolon); // Optional semicolon
        return std::make_unique<ExprStmt>(std::move(expr));
    }

    UPtr<Stmt> Parser::localDeclaration() {
        // Check if it's a local function definition: local function name ...
        if (match(TokenType::Function)) {
            Token name = consume(TokenType::Name, "Expect function name after 'local function'.");
            auto funcExpr = functionExpression(); // This will parse parameters and body
            // The functionExpression already returns a FunctionExpr, which is an Expr.
            // We need to wrap this in a LocalStmt.
            return std::make_unique<LocalStmt>(name.lexeme, std::move(funcExpr));
        }

        // Regular local variable declaration: local name = value
        Token name = consume(TokenType::Name, "Expect variable name.");

        UPtr<Expr> initializer = nullptr;
        if (match(TokenType::Assign)) {
            initializer = expression();
        }

        match(TokenType::Semicolon); // Optional semicolon
        return std::make_unique<LocalStmt>(name.lexeme, std::move(initializer));
    }

    UPtr<Stmt> Parser::assignmentStatement() {
        auto expr = expression();

        // Check if this is an assignment
        if (match(TokenType::Assign)) {
            if (!isValidAssignmentTarget(expr.get())) {
                error("Invalid assignment target.");
                return std::make_unique<ExprStmt>(std::move(expr));
            }

            auto value = expression();
            match(TokenType::Semicolon); // Optional semicolon
            return std::make_unique<AssignStmt>(std::move(expr), std::move(value));
        }

        // It's just an expression statement
        match(TokenType::Semicolon); // Optional semicolon
        return std::make_unique<ExprStmt>(std::move(expr));
    }

    UPtr<Stmt> Parser::ifStatement() {
        auto condition = expression();
        consume(TokenType::Then, "Expect 'then' after if condition.");

        auto thenBranch = blockStatement();

        UPtr<Stmt> elseBranch = nullptr;
        if (match(TokenType::Else)) {
            elseBranch = blockStatement();
        }

        consume(TokenType::End, "Expect 'end' after if statement.");

        return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
    }

    UPtr<Stmt> Parser::whileStatement() {
        // Parse condition expression
        auto condition = expression();
        
        // Expect 'do' keyword
        consume(TokenType::Do, "Expect 'do' after while condition.");
        
        // Parse body as a block statement
        auto body = blockStatement();
        
        // Expect 'end' keyword
        consume(TokenType::End, "Expect 'end' after while body.");
        
        return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
    }

    UPtr<Stmt> Parser::blockStatement() {
        Vec<UPtr<Stmt>> statements;

        while (!check(TokenType::End) && !check(TokenType::Else) && !check(TokenType::Eof)) {
            statements.push_back(statement());
        }

        return std::make_unique<BlockStmt>(std::move(statements));
    }

    UPtr<Stmt> Parser::returnStatement() {
        Vec<UPtr<Expr>> values;

        // check if there are values to return
        if (!check(TokenType::End) && !check(TokenType::Else) &&
            !check(TokenType::Semicolon) && !isAtEnd()) {
            // Parse first expression
            values.push_back(expression());
            
            // Parse additional expressions separated by commas
            while (match(TokenType::Comma)) {
                values.push_back(expression());
            }
        }

        match(TokenType::Semicolon);
        
        // Use appropriate constructor based on number of values
        if (values.empty()) {
            return std::make_unique<ReturnStmt>();
        } else if (values.size() == 1) {
            // Use single-value constructor for backward compatibility
            return std::make_unique<ReturnStmt>(std::move(values[0]));
        } else {
            // Use multi-value constructor
            return std::make_unique<ReturnStmt>(std::move(values));
        }
    }

    UPtr<Stmt> Parser::breakStatement() {
        // Optional semicolon after break
        match(TokenType::Semicolon);
        
        return std::make_unique<BreakStmt>();
    }

    UPtr<Stmt> Parser::forStatement() {
        // Parse first variable name
        Token firstVar = consume(TokenType::Name, "Expect variable name after 'for'.");
        
        // Check if this is a numeric for loop (=) or for-in loop (,/in)
        if (check(TokenType::Assign)) {
            // Numeric for loop: for var = start, end [, step] do body end
            advance(); // consume '='
            
            // Parse start expression
            auto start = expression();
            
            // Expect ','
            consume(TokenType::Comma, "Expect ',' after for start value.");
            
            // Parse end expression
            auto end = expression();
            
            // Parse optional step expression
            UPtr<Expr> step = nullptr;
            if (match(TokenType::Comma)) {
                step = expression();
            }
            
            // Expect 'do'
            consume(TokenType::Do, "Expect 'do' after for range.");
            
            // Parse body as a block statement
            auto body = blockStatement();
            
            // Expect 'end'
            consume(TokenType::End, "Expect 'end' after for body.");
            
            return std::make_unique<ForStmt>(firstVar.lexeme, std::move(start), std::move(end), std::move(step), std::move(body));
        } else {
            // For-in loop: for var1, var2, ... in expr do body end
            Vec<Str> variables;
            variables.push_back(firstVar.lexeme);
            
            // Parse additional variables
            while (match(TokenType::Comma)) {
                Token var = consume(TokenType::Name, "Expect variable name after ','.");
                variables.push_back(var.lexeme);
            }
            
            // Expect 'in'
            consume(TokenType::In, "Expect 'in' after for variables.");
            
            // Parse iterator expression list
            Vec<UPtr<Expr>> iterators;
            do {
                iterators.push_back(expression());
            } while (match(TokenType::Comma));
            
            // Expect 'do'
            consume(TokenType::Do, "Expect 'do' after for iterator.");
            
            // Parse body as a block statement
            auto body = blockStatement();
            
            // Expect 'end'
            consume(TokenType::End, "Expect 'end' after for body.");
            
            return std::make_unique<ForInStmt>(variables, std::move(iterators), std::move(body));
        }
    }

    UPtr<Stmt> Parser::repeatUntilStatement() {
        // Parse body statements until 'until' keyword
        Vec<UPtr<Stmt>> statements;
        
        while (!check(TokenType::Until) && !isAtEnd()) {
            statements.push_back(statement());
        }
        
        // Expect 'until'
        consume(TokenType::Until, "Expect 'until' after repeat body.");
        
        // Parse condition expression
        auto condition = expression();
        
        // Create block statement for body
        auto body = std::make_unique<BlockStmt>(std::move(statements));
        
        return std::make_unique<RepeatUntilStmt>(std::move(body), std::move(condition));
    }

    UPtr<Stmt> Parser::functionStatement() {
        // Parse function name
        Token name = consume(TokenType::Name, "Expect function name.");
        
        // Parse parameter list
        consume(TokenType::LeftParen, "Expect '(' after function name.");
        
        Vec<Str> parameters;
        if (!check(TokenType::RightParen)) {
            do {
                Token param = consume(TokenType::Name, "Expect parameter name.");
                parameters.push_back(param.lexeme);
            } while (match(TokenType::Comma));
        }
        
        consume(TokenType::RightParen, "Expect ')' after parameters.");
        
        // Parse function body
        auto body = blockStatement();
        
        // Expect 'end'
        consume(TokenType::End, "Expect 'end' after function body.");
        
        return std::make_unique<FunctionStmt>(name.lexeme, parameters, std::move(body));
    }
}