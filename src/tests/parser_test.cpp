#include "parser_test.hpp"

namespace Lua {
namespace Tests {

void testParser() {
    std::cout << "\nParser Test:" << std::endl;

    // Test cases for different expression types
    std::vector<std::string> testCases = {
        // Basic arithmetic
        "1 + 2",
        "3 * 4 + 5",
        "10 - 2 * 3",
        "(1 + 2) * 3",

        // Comparison operators
        "x == y",
        "a < b",
        "c >= d",
        "e ~= f",

        // Logical operators
        "true and false",
        "x > 0 or y < 10",
        "not (a and b)",

        // String concatenation
        "\"hello\" .. \" world\"",
        "a .. b .. c",

        // Power operator
        "2 ^ 3",
        "2 ^ 3 ^ 2",

        // Complex expressions
        "a + b * c == d and e or f",
        "x ^ 2 + y ^ 2 < r ^ 2",
        "not a or b and c > d",

        // Function calls
        "print(\"hello\")",
        "math.max(a, b)",
        "func(1, 2, 3)",

        // Unary operators
        "-x",
        "#table",
        "not flag",

        // Mixed expressions
        "a + b * c ^ d - e / f % g",
        "(a + b) * (c - d) / (e + f)",

        // Expression statements
        "x",
        "42",
        "print(\"hello\")",
        "a + b",
        "func(1, 2, 3);",
        "math.max(10, 20);",

        // Local declarations
        "local x",
        "local y = 10",
        "local name = \"John\"",
        "local result = a + b * c",
        "local flag = true",
        "local table = {1, 2, 3}",
        "local func = function() end",
        "local pi = 3.14159;",

        // While loops
        "while x > 0 do x = x - 1 end",
        "while true do break end",

        // For loops
        "for i = 1, 10 do print(i) end",
        "for i = 1, 10, 2 do print(i) end",
        "for j = 10, 1, -1 do print(j) end",
        "for k = start, finish do k = k + 1 end",

        // For-in loops
        "for k, v in pairs(table) do print(k, v) end",
        "for i, v in ipairs(array) do print(i, v) end",
        "for key in next, table do print(key) end",
        "for a, b, c in iterator() do print(a, b, c) end",

        // Mixed statements
        "local x = 5; x = x + 1"
    };

    for (const auto& testCase : testCases) {
        std::cout << "\nTesting: " << testCase << std::endl;

        try {
            Parser parser(testCase);
            auto statements = parser.parse();

            if (parser.hasError()) {
                std::cout << "  Parse Error!" << std::endl;
            } else {
                std::cout << "  Parsed successfully! (" << statements.size() << " statements)" << std::endl;

                // Print basic info about the parsed expression
                if (!statements.empty() && statements[0]->getType() == StmtType::Expression) {
                    auto exprStmt = static_cast<const ExprStmt*>(statements[0].get());
                    auto expr = exprStmt->getExpression();

                    std::cout << "  Expression type: ";
                    switch (expr->getType()) {
                        case ExprType::Binary:
                            std::cout << "Binary";
                            break;
                        case ExprType::Unary:
                            std::cout << "Unary";
                            break;
                        case ExprType::Literal:
                            std::cout << "Literal";
                            break;
                        case ExprType::Variable:
                            std::cout << "Variable";
                            break;
                        case ExprType::Call:
                            std::cout << "Call";
                            break;
                        case ExprType::Table:
                            std::cout << "Table";
                            break;
                        case ExprType::Member:
                            std::cout << "Member";
                            break;
                        case ExprType::Index:
                            std::cout << "Index";
                            break;
                        case ExprType::Function:
                            std::cout << "Function";
                            break;
                    }
                    std::cout << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cout << "  Exception: " << e.what() << std::endl;
        }
    }
}

void testStatements() {
    std::cout << "\nStatement Parsing Test:" << std::endl;

    std::vector<std::string> statementTests = {
        // Expression statements
        "42",
        "x",
        "print(\"test\")",
        "a + b * c",

        // Local declarations
        "local x",
        "local y = 10",
        "local name = \"Alice\"",
        "local result = 2 + 3 * 4",
        "local flag = true and false",

        // Assignment statements
        "x = 5",
        "table[key] = value",
        "obj.property = \"new value\"",

        // If statements
        "if x > 0 then print(\"positive\") end",
        "if a == b then return true else return false end"
    };

    for (const auto& test : statementTests) {
        std::cout << "\nTesting statement: " << test << std::endl;

        try {
            Parser parser(test);
            auto statements = parser.parse();

            if (parser.hasError()) {
                std::cout << "  Parse Error!" << std::endl;
            } else {
                std::cout << "  Parsed successfully! (" << statements.size() << " statements)" << std::endl;

                for (const auto& stmt : statements) {
                    std::cout << "  Statement type: ";
                    switch (stmt->getType()) {
                        case StmtType::Expression:
                            std::cout << "Expression";
                            break;
                        case StmtType::Local:
                            std::cout << "Local Declaration";
                            break;
                        case StmtType::Assign:
                            std::cout << "Assignment";
                            break;
                        case StmtType::If:
                            std::cout << "If Statement";
                            break;
                        case StmtType::Block:
                            std::cout << "Block";
                            break;
                        default:
                            std::cout << "Unknown";
                            break;
                    }
                    std::cout << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cout << "  Exception: " << e.what() << std::endl;
        }
    }
}

void testWhileLoop() {
    std::cout << "\nWhile Loop Parsing Test:" << std::endl;

    std::vector<std::string> whileTests = {
        // Basic while loop
        "while x > 0 do\n    x = x - 1\nend",
        
        // While loop with complex condition
        "while a < 10 and b > 0 do\n    print(a)\n    a = a + 1\nend",
        
        // Nested while loops
        "while i < 5 do\n    local j = 0\n    while j < 3 do\n        print(i, j)\n        j = j + 1\n    end\n    i = i + 1\nend",
        
        // While loop with function calls
        "while not isEmpty(queue) do\n    local item = pop(queue)\n    process(item)\nend",
        
        // While loop with table access
        "while table[index] ~= nil do\n    print(table[index])\n    index = index + 1\nend",
        
        // Simple infinite loop pattern
        "while true do\n    local input = getInput()\n    if input == \"quit\" then\n        break\n    end\nend"
    };

    for (const auto& test : whileTests) {
        std::cout << "\nTesting while loop: " << test << std::endl;

        try {
            Parser parser(test);
            auto statements = parser.parse();

            if (parser.hasError()) {
                std::cout << "  Parse Error!" << std::endl;
            } else {
                std::cout << "  Parsed successfully! (" << statements.size() << " statements)" << std::endl;

                for (const auto& stmt : statements) {
                    if (stmt->getType() == StmtType::While) {
                        std::cout << "  Found While statement" << std::endl;
                        
                        // Cast to WhileStmt to access condition and body
                        auto whileStmt = static_cast<const WhileStmt*>(stmt.get());
                        
                        std::cout << "  Condition type: ";
                        auto condition = whileStmt->getCondition();
                        switch (condition->getType()) {
                            case ExprType::Binary:
                                std::cout << "Binary expression";
                                break;
                            case ExprType::Variable:
                                std::cout << "Variable";
                                break;
                            case ExprType::Literal:
                                std::cout << "Literal";
                                break;
                            case ExprType::Call:
                                std::cout << "Function call";
                                break;
                            default:
                                std::cout << "Other";
                                break;
                        }
                        std::cout << std::endl;
                        
                        std::cout << "  Body type: ";
                         auto body = whileStmt->getBody();
                         switch (body->getType()) {
                             case StmtType::Block:
                                 std::cout << "Block statement";
                                 // Check statements inside the block
                                 if (auto blockStmt = static_cast<const BlockStmt*>(body)) {
                                     const auto& blockStatements = blockStmt->getStatements();
                                     std::cout << " (" << blockStatements.size() << " statements)";
                                     for (const auto& innerStmt : blockStatements) {
                                         if (innerStmt->getType() == StmtType::Break) {
                                             std::cout << "\n    Found Break statement inside while loop";
                                         } else if (innerStmt->getType() == StmtType::Local) {
                                             std::cout << "\n    Found Local declaration inside while loop";
                                         } else if (innerStmt->getType() == StmtType::If) {
                                             std::cout << "\n    Found If statement inside while loop";
                                             // Check inside the if statement for break
                                             if (auto ifStmt = static_cast<const IfStmt*>(innerStmt.get())) {
                                                 auto thenBranch = ifStmt->getThenBranch();
                                                 if (thenBranch && thenBranch->getType() == StmtType::Block) {
                                                     if (auto thenBlock = static_cast<const BlockStmt*>(thenBranch)) {
                                                         const auto& thenStatements = thenBlock->getStatements();
                                                         for (const auto& thenStmt : thenStatements) {
                                                             if (thenStmt->getType() == StmtType::Break) {
                                                                 std::cout << "\n      Found Break statement inside if statement";
                                                             }
                                                         }
                                                     }
                                                 }
                                             }
                                         } else {
                                             std::cout << "\n    Found " << static_cast<int>(innerStmt->getType()) << " statement inside while loop";
                                         }
                                     }
                                 }
                                 break;
                             case StmtType::Expression:
                                 std::cout << "Expression statement";
                                 break;
                             default:
                                 std::cout << "Other statement";
                                 break;
                         }
                         std::cout << std::endl;
                     } else if (stmt->getType() == StmtType::Break) {
                         std::cout << "  Found Break statement" << std::endl;
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cout << "  Exception: " << e.what() << std::endl;
        }
    }
}

void testASTVisitor() {
    std::cout << "\nAST Visitor Test:" << std::endl;

    std::vector<std::string> visitorTests = {
        // Test complex expressions and statements
        "local x = 10 + 20 * 30",
        "if a > b then\n    print(a)\n    return true\nelse\n    print(b)\n    return false\nend",
        "local function add(a, b)\n    return a + b\nend",
        "local tbl = {x = 1, y = 2, [\"key\"] = \"value\"}"
    };

    for (const auto& test : visitorTests) {
        std::cout << "\nTesting AST for: " << test << std::endl;

        try {
            Parser parser(test);
            auto statements = parser.parse();

            if (!parser.hasError()) {
                // Print AST structure
                std::cout << "AST Structure:" << std::endl;
                std::cout << ASTUtils::printAST(statements) << std::endl;

                // Count nodes
                int nodeCount = ASTUtils::countNodes(statements);
                std::cout << "Total nodes: " << nodeCount << std::endl;

                // Collect variables
                auto variables = ASTUtils::collectVariables(statements);
                std::cout << "Variables used:";
                for (const auto& var : variables) {
                    std::cout << " " << var;
                }
                std::cout << std::endl;

                // Test specific variable presence
                std::cout << "Contains 'x': " << (ASTUtils::hasVariable(statements, "x") ? "yes" : "no") << std::endl;
            } else {
                std::cout << "Parse error!" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "Exception: " << e.what() << std::endl;
        }
    }
}

} // namespace Tests
} // namespace Lua