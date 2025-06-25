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

        if (match(TokenType::Do)) {
            return doStatement();
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

        // Regular local variable declaration: local name1, name2, ... = value1, value2, ...
        Vec<Str> names;
        
        // Parse variable names
        do {
            Token name = consume(TokenType::Name, "Expect variable name.");
            names.push_back(name.lexeme);
        } while (match(TokenType::Comma));

        // Parse initializers if present
        Vec<UPtr<Expr>> initializers;
        if (match(TokenType::Assign)) {
            do {
                auto expr = expression();
                if (expr) {
                    initializers.push_back(std::move(expr));
                } else {
                    // If expression parsing fails, break to avoid infinite loop
                    error("Failed to parse initializer expression.");
                    break;
                }
            } while (match(TokenType::Comma));
        }

        match(TokenType::Semicolon); // Optional semicolon
        
        // For now, create multiple LocalStmt for each variable
        // In a more complete implementation, you'd want a MultiLocalStmt
        if (names.size() == 1) {
            UPtr<Expr> init = initializers.empty() ? nullptr : std::move(initializers[0]);
            return std::make_unique<LocalStmt>(names[0], std::move(init));
        } else {
            // For multiple variables, create a block with multiple LocalStmt
            Vec<UPtr<Stmt>> statements;
            for (size_t i = 0; i < names.size(); ++i) {
                UPtr<Expr> init = (i < initializers.size()) ? std::move(initializers[i]) : nullptr;
                statements.push_back(std::make_unique<LocalStmt>(names[i], std::move(init)));
            }
            return std::make_unique<BlockStmt>(std::move(statements));
        }
    }

    UPtr<Stmt> Parser::assignmentStatement() {
        auto expr = expression();
        
        // If expression parsing failed, return null to avoid infinite loop
        if (!expr) {
            error("Failed to parse expression in assignment statement.");
            synchronize(); // Try to recover
            return nullptr;
        }

        // Check if this is an assignment
        if (match(TokenType::Assign)) {
            if (!isValidAssignmentTarget(expr.get())) {
                error("Invalid assignment target.");
                return std::make_unique<ExprStmt>(std::move(expr));
            }

            auto value = expression();
            if (!value) {
                error("Failed to parse assignment value.");
                return std::make_unique<ExprStmt>(std::move(expr));
            }
            
            match(TokenType::Semicolon); // Optional semicolon
            return std::make_unique<AssignStmt>(std::move(expr), std::move(value));
        }

        // It's just an expression statement
        match(TokenType::Semicolon); // Optional semicolon
        return std::make_unique<ExprStmt>(std::move(expr));
    }

    UPtr<Stmt> Parser::ifStatement() {
        SourceLocation ifLocation = SourceLocation::fromToken(previous);
        
        UPtr<Expr> condition;
        try {
            condition = expression();
        } catch (...) {
            // If condition parsing fails, try to recover
            error(ErrorType::InvalidExpression, "Invalid condition in if statement");
            synchronize();
            // Create a dummy condition to continue parsing
            condition = std::make_unique<LiteralExpr>(Value(false));
        }
        
        if (consume(TokenType::Then, "Expect 'then' after if condition.").type != TokenType::Then) {
            // Try to recover from missing 'then'
            auto missingThenError = ParseError::missingToken(SourceLocation::fromToken(current), "then");
            missingThenError.addSuggestion(FixType::Insert, SourceLocation::fromToken(current), 
                "Insert 'then' keyword", "then");
            errorReporter_.addError(std::move(missingThenError));
            
            // Try to find the then block anyway
            if (current.type != TokenType::End && current.type != TokenType::Else && 
                current.type != TokenType::Elseif && !isAtEnd()) {
                // Assume the then block starts here
            } else {
                synchronize();
            }
        }

        UPtr<Stmt> thenBranch;
        try {
            thenBranch = blockStatement();
        } catch (...) {
            error(ErrorType::InvalidStatement, "Invalid then block in if statement");
            synchronize();
            thenBranch = std::make_unique<BlockStmt>(Vec<UPtr<Stmt>>());
        }

        UPtr<Stmt> elseBranch = nullptr;
        if (match(TokenType::Else)) {
            try {
                elseBranch = blockStatement();
            } catch (...) {
                error(ErrorType::InvalidStatement, "Invalid else block in if statement");
                synchronize();
                elseBranch = std::make_unique<BlockStmt>(Vec<UPtr<Stmt>>());
            }
        }

        if (consume(TokenType::End, "Expect 'end' after if statement.").type != TokenType::End) {
            // Try to recover from missing 'end'
            auto missingEndError = ParseError::missingToken(SourceLocation::fromToken(current), "end");
            missingEndError.addSuggestion(FixType::Insert, SourceLocation::fromToken(current), 
                "Insert 'end' keyword to close if statement", "end");
            missingEndError.setDetails("If statement started at " + ifLocation.toString());
            errorReporter_.addError(std::move(missingEndError));
        }

        return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
    }

    UPtr<Stmt> Parser::whileStatement() {
        SourceLocation whileLocation = SourceLocation::fromToken(previous);
        
        UPtr<Expr> condition;
        try {
            condition = expression();
        } catch (...) {
            error(ErrorType::InvalidExpression, "Invalid condition in while statement");
            synchronize();
            condition = std::make_unique<LiteralExpr>(Value(false));
        }
        
        if (consume(TokenType::Do, "Expect 'do' after while condition.").type != TokenType::Do) {
            auto missingDoError = ParseError::missingToken(SourceLocation::fromToken(current), "do");
            missingDoError.addSuggestion(FixType::Insert, SourceLocation::fromToken(current), 
                "Insert 'do' keyword", "do");
            errorReporter_.addError(std::move(missingDoError));
        }

        UPtr<Stmt> body;
        try {
            body = blockStatement();
        } catch (...) {
            error(ErrorType::InvalidStatement, "Invalid body in while statement");
            synchronize();
            body = std::make_unique<BlockStmt>(Vec<UPtr<Stmt>>());
        }

        if (consume(TokenType::End, "Expect 'end' after while body.").type != TokenType::End) {
            auto missingEndError = ParseError::missingToken(SourceLocation::fromToken(current), "end");
            missingEndError.addSuggestion(FixType::Insert, SourceLocation::fromToken(current), 
                "Insert 'end' keyword to close while statement", "end");
            missingEndError.setDetails("While statement started at " + whileLocation.toString());
            errorReporter_.addError(std::move(missingEndError));
        }
        
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
        bool isVariadic = false;
        
        if (!check(TokenType::RightParen)) {
            do {
                if (check(TokenType::DotDotDot)) {
                    advance(); // consume '...'
                    isVariadic = true;
                    break; // '...' must be the last parameter
                } else {
                    Token param = consume(TokenType::Name, "Expect parameter name.");
                    parameters.push_back(param.lexeme);
                }
            } while (match(TokenType::Comma));
        }
        
        consume(TokenType::RightParen, "Expect ')' after parameters.");
        
        // Parse function body
        auto body = blockStatement();
        
        // Expect 'end'
        consume(TokenType::End, "Expect 'end' after function body.");
        
        return std::make_unique<FunctionStmt>(name.lexeme, parameters, std::move(body), isVariadic);
    }

    UPtr<Stmt> Parser::doStatement() {
        // Parse the block body
        auto body = blockStatement();
        
        // Expect 'end'
        consume(TokenType::End, "Expect 'end' after do block.");
        
        return std::make_unique<DoStmt>(std::move(body));
    }
}