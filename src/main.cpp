#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "types.hpp"
#include "vm/state.hpp"
#include "vm/value.hpp"
#include "vm/table.hpp"
#include "vm/function.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "parser/visitor.hpp"
#include "compiler/symbol_table.hpp"
#include "common/defines.hpp"
#include "lib/base_lib.hpp"

using namespace Lua;

// Read file content
std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw LuaException("Could not open file: " + path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Test lexer
void testLexer(const std::string& source) {
    std::cout << "Lexer Test:" << std::endl;
    std::cout << "Source: " << source << std::endl;

    Lexer lexer(source);
    Token token;

    do {
        token = lexer.nextToken();
        std::cout << "Token: " << static_cast<int>(token.type) << " Lexeme: " << token.lexeme 
            << " Line: " << token.line << " Column: " << token.column << std::endl;
    } while (token.type != TokenType::Eof && token.type != TokenType::Error);
}

// Test Lua values
void testValues() {
    std::cout << "\nValue Test:" << std::endl;

    Value nil;
    Value boolean(true);
    Value number(42.5);
    Value string("Hello, Lua!");

    std::cout << "nil: " << nil.toString() << std::endl;
    std::cout << "boolean: " << boolean.toString() << std::endl;
    std::cout << "number: " << number.toString() << std::endl;
    std::cout << "string: " << string.toString() << std::endl;

    // Test table
    auto table = make_ptr<Table>();
    table->set(Value(1), Value("one"));
    table->set(Value(2), Value("two"));
    table->set(Value("name"), Value("lua"));

    Value tableValue(table);
    std::cout << "table: " << tableValue.toString() << std::endl;
    std::cout << "table[1]: " << table->get(Value(1)).toString() << std::endl;
    std::cout << "table[2]: " << table->get(Value(2)).toString() << std::endl;
    std::cout << "table['name']: " << table->get(Value("name")).toString() << std::endl;
}

// Test state
void testState() {
    std::cout << "\nState Test:" << std::endl;

    State state;

    // Register base library
    registerBaseLib(&state);

    // Test global variables
    state.setGlobal("x", Value(10));
    state.setGlobal("y", Value(20));
    state.setGlobal("z", Value("Lua"));

    std::cout << "x: " << state.getGlobal("x").toString() << std::endl;
    std::cout << "y: " << state.getGlobal("y").toString() << std::endl;
    std::cout << "z: " << state.getGlobal("z").toString() << std::endl;

    // Test stack operations
    state.push(Value(1));
    state.push(Value(2));
    state.push(Value(3));

    std::cout << "Stack size: " << state.getTop() << std::endl;
    std::cout << "Stack[1]: " << state.get(1).toString() << std::endl;
    std::cout << "Stack[2]: " << state.get(2).toString() << std::endl;
    std::cout << "Stack[3]: " << state.get(3).toString() << std::endl;

    // Call native function
    Value printFn = state.getGlobal("print");
    if (printFn.isFunction()) {
        Vec<Value> args;
        args.push_back(Value("Hello from native function!"));
        state.call(printFn, args);
    }
}

// Test executing lua code
void testExecute() {
    std::cout << "\nExecute Test:" << std::endl;

    State state;
    registerBaseLib(&state);

    // Execute simple Lua code
    state.doString("print('Hello from Lua!')");
}

// Test parser expressions
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
        "(a + b) * (c - d) / (e + f)"

        // Expression statements (test expressionStatement)
        "x",
        "42",
        "print(\"hello\")",
        "a + b",
        "func(1, 2, 3);",
        "math.max(10, 20);",

        // Local declarations (test localDeclaration)
        "local x",
        "local y = 10",
        "local name = \"John\"",
        "local result = a + b * c",
        "local flag = true",
        "local table = {1, 2, 3}",
        "local func = function() end",
        "local pi = 3.14159;",

        // Mixed statements
        "local x = 5; x = x + 1",
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

// Test AST visitor functionality
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

// Test while loop parsing
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

// Symbol Table Test Function
void testSymbolTable() {
    std::cout << "\nSymbol Table Test:" << std::endl;

    SymbolTable symbolTable;

    // Test global scope
    std::cout << "Testing global scope:" << std::endl;
    symbolTable.define("print", SymbolType::Function);
    symbolTable.define("globalVar", SymbolType::Variable);

    auto printSymbol = symbolTable.resolve("print");
    if (printSymbol) {
        std::cout << "Found 'print' in scope level: " << printSymbol->scopeLevel << std::endl;
    }

    // Test local scope
    std::cout << "\nTesting local scope:" << std::endl;
    symbolTable.enterScope(); // Enter a new scope

    symbolTable.define("localVar", SymbolType::Variable);
    symbolTable.define("x", SymbolType::Parameter);

    // Should find local variable
    auto localVar = symbolTable.resolve("localVar");
    if (localVar) {
        std::cout << "Found 'localVar' in scope level: " << localVar->scopeLevel << std::endl;
    }

    // Should still find global variable
    auto globalVar = symbolTable.resolve("globalVar");
    if (globalVar) {
        std::cout << "Found 'globalVar' in scope level: " << globalVar->scopeLevel << std::endl;
    }

    // Test nested scope
    std::cout << "\nTesting nested scope:" << std::endl;
    symbolTable.enterScope(); // Enter a nested scope

    symbolTable.define("innerVar", SymbolType::Variable);
    // Try to define a variable that shadows an outer scope variable
    symbolTable.define("x", SymbolType::Variable);

    auto innerVar = symbolTable.resolve("innerVar");
    if (innerVar) {
        std::cout << "Found 'innerVar' in scope level: " << innerVar->scopeLevel << std::endl;
    }

    // Should find the inner 'x'
    auto innerX = symbolTable.resolve("x");
    if (innerX) {
        std::cout << "Found 'x' in scope level: " << innerX->scopeLevel << std::endl;
    }

    // Leave the nested scope
    symbolTable.leaveScope();

    // Should now find the outer 'x'
    auto outerX = symbolTable.resolve("x");
    if (outerX) {
        std::cout << "Found 'x' in scope level: " << outerX->scopeLevel << std::endl;
    }

    // Leave the local scope
    symbolTable.leaveScope();

    // Should not find local variables anymore
    auto notFound = symbolTable.resolve("localVar");
    if (!notFound) {
        std::cout << "'localVar' is no longer accessible" << std::endl;
    }
}

int main(int argc, char** argv) {
    try {
        std::cout << LUA_VERSION << " " << LUA_COPYRIGHT << std::endl;

        // Run all tests
        testLexer("local x = 10 + 20");
        testValues();
        testState();
        testParser();
        testStatements();
        testWhileLoop();
        testASTVisitor();
        testSymbolTable();

        return 0;
    } 

    catch (const LuaException& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
    } 

    catch (const std::exception& e) {
        std::cerr << "Standard error: " << e.what() << std::endl;
        return 1;
    }
}
