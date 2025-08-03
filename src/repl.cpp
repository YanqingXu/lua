#include <iostream>
#include <string>
#include <sstream>
#include <stack>
#include <vector>
#include <cmath>
#include <cctype>
#include <csignal>
#include <cstdlib>

#include "common/types.hpp"
#include "vm/state.hpp"
#include "vm/vm.hpp"
#include "compiler/compiler.hpp"
#include "lib/core/lib_manager.hpp"
#include "parser/parser.hpp"
#include "lexer/lexer.hpp"

namespace Lua {

// Global variables for signal handling
volatile bool g_interrupted = false;

// Signal handler function
void signalHandler(int signal) {
    if (signal == SIGINT) {
        g_interrupted = true;
        std::cout << "\n"; // newline
    }
}

// REPL exit function
Value replExit(State* state, i32 nargs) {
    int exitCode = 0;
    if (nargs > 0) {
        Value arg = state->get(-nargs);
        if (arg.isNumber()) {
            exitCode = static_cast<int>(arg.asNumber());
        }
    }
    std::exit(exitCode);
    return Value(nullptr); // never reached
}

// REPL incomplete statement detector
class IncompleteStatementDetector {
private:
    enum class BlockType {
        Function,    // function ... end
        If,         // if ... end
        While,      // while ... end
        For,        // for ... end
        Repeat,     // repeat ... until
        Do,         // do ... end
        Table,      // { ... }
        Parentheses // ( ... )
    };

    struct BlockInfo {
        BlockType type;
        int line;
        int column;

        BlockInfo(BlockType t, int l, int c) : type(t), line(l), column(c) {}
    };

    std::stack<BlockInfo> blockStack;
    int parenCount = 0;
    int braceCount = 0;
    int bracketCount = 0;
    bool inString = false;
    bool inComment = false;
    bool inLongString = false;
    bool inLongComment = false;
    int longStringLevel = 0;
    int longCommentLevel = 0;

public:
    // Check if code is complete
    bool isComplete(const std::string& code) {
        reset();
        return analyzeCode(code);
    }

    // Get incomplete reason
    std::string getIncompleteReason() const {
        if (!blockStack.empty()) {
            const auto& block = blockStack.top();
            switch (block.type) {
                case BlockType::Function:
                    return "Incomplete function definition, need 'end'";
                case BlockType::If:
                    return "Incomplete if statement, need 'end'";
                case BlockType::While:
                    return "Incomplete while loop, need 'end'";
                case BlockType::For:
                    return "Incomplete for loop, need 'end'";
                case BlockType::Repeat:
                    return "Incomplete repeat loop, need 'until'";
                case BlockType::Do:
                    return "Incomplete do block, need 'end'";
                case BlockType::Table:
                    return "Incomplete table definition, need '}'";
                case BlockType::Parentheses:
                    return "Incomplete parentheses expression, need ')'";
            }
        }

        if (parenCount > 0) return "Unclosed parentheses";
        if (braceCount > 0) return "Unclosed braces";
        if (bracketCount > 0) return "Unclosed brackets";
        if (inString) return "Incomplete string";
        if (inLongString) return "Incomplete long string";
        if (inLongComment) return "Incomplete long comment";

        return "Statement incomplete";
    }

private:
    void reset() {
        while (!blockStack.empty()) blockStack.pop();
        parenCount = braceCount = bracketCount = 0;
        inString = inComment = inLongString = inLongComment = false;
        longStringLevel = longCommentLevel = 0;
    }

    bool analyzeCode(const std::string& code) {
        try {
            Lexer lexer(code);
            Token token;

            while ((token = lexer.nextToken()).type != TokenType::Eof) {
                if (!processToken(token)) {
                    return false;
                }
            }

            // Check if all blocks are closed
            return blockStack.empty() && parenCount == 0 &&
                   braceCount == 0 && bracketCount == 0 &&
                   !inString && !inLongString && !inLongComment;

        } catch (const std::exception& e) {
            // If lexical analysis fails, might be incomplete input
            std::cerr << "Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool processToken(const Token& token) {
        switch (token.type) {
            case TokenType::Function:
                blockStack.push(BlockInfo(BlockType::Function, token.line, token.column));
                break;

            case TokenType::If:
                blockStack.push(BlockInfo(BlockType::If, token.line, token.column));
                break;

            case TokenType::While:
                blockStack.push(BlockInfo(BlockType::While, token.line, token.column));
                break;

            case TokenType::For:
                blockStack.push(BlockInfo(BlockType::For, token.line, token.column));
                break;

            case TokenType::Repeat:
                blockStack.push(BlockInfo(BlockType::Repeat, token.line, token.column));
                break;

            case TokenType::Do:
                blockStack.push(BlockInfo(BlockType::Do, token.line, token.column));
                break;

            case TokenType::End:
                if (blockStack.empty()) {
                    return true; // Extra end, but doesn't affect completeness
                }
                {
                    const auto& block = blockStack.top();
                    if (block.type == BlockType::Repeat) {
                        return false; // repeat needs until not end
                    }
                    blockStack.pop();
                }
                break;

            case TokenType::Until:
                if (blockStack.empty() || blockStack.top().type != BlockType::Repeat) {
                    return true; // Syntax error, but doesn't affect completeness detection
                }
                blockStack.pop();
                break;

            case TokenType::LeftParen:
                parenCount++;
                break;

            case TokenType::RightParen:
                parenCount--;
                if (parenCount < 0) parenCount = 0; // Prevent negative
                break;

            case TokenType::LeftBrace:
                braceCount++;
                blockStack.push(BlockInfo(BlockType::Table, token.line, token.column));
                break;

            case TokenType::RightBrace:
                braceCount--;
                if (braceCount < 0) braceCount = 0;
                if (!blockStack.empty() && blockStack.top().type == BlockType::Table) {
                    blockStack.pop();
                }
                break;

            case TokenType::LeftBracket:
                bracketCount++;
                break;

            case TokenType::RightBracket:
                bracketCount--;
                if (bracketCount < 0) bracketCount = 0;
                break;

            default:
                break;
        }

        return true;
    }
};

} // namespace Lua

// Function declarations
void executeCode(Lua::State& state, const std::string& code);

// Initialize REPL state
void initializeREPL(Lua::State& state) {
    // Set default prompts
    state.setGlobal("_PROMPT", Lua::Value(">"));
    state.setGlobal("_PROMPT2", Lua::Value(">>"));

    // Set version info
    state.setGlobal("_VERSION", Lua::Value("Lua 5.1.5"));

    // Add REPL specific global functions (legacy)
    auto exitFunc = Lua::Function::createNativeLegacy(Lua::replExit);
    state.setGlobal("exit", Lua::Value(exitFunc));

    // Set signal handling
    std::signal(SIGINT, Lua::signalHandler);
}

// This function will be called from main.cpp
void run_repl() {
    Lua::State state;
    Lua::StandardLibrary::initializeAll(&state);
    initializeREPL(state);

    std::cout << "Lua 5.1.5  Copyright (C) 1994-2012 Lua.org, PUC-Rio" << std::endl;

    Lua::IncompleteStatementDetector detector;
    std::string currentInput;
    std::string line;

    while (true) {
        // Check if interrupted
        if (Lua::g_interrupted) {
            Lua::g_interrupted = false;
            currentInput.clear();
            continue;
        }

        // Get prompt
        std::string prompt = currentInput.empty() ? "> " : ">> ";

        // Try to get custom prompt from Lua state
        try {
            if (currentInput.empty()) {
                // Main prompt
                Lua::Value promptVal = state.getGlobal("_PROMPT");
                if (promptVal.isString()) {
                    prompt = promptVal.toString() + " ";
                }
            } else {
                // Continuation prompt
                Lua::Value prompt2Val = state.getGlobal("_PROMPT2");
                if (prompt2Val.isString()) {
                    prompt = prompt2Val.toString() + " ";
                }
            }
        } catch (...) {
            // Ignore errors when getting prompts
        }

        std::cout << prompt;
        if (!std::getline(std::cin, line)) {
            break; // EOF
        }

        // Check exit command
        if (line == "exit" && currentInput.empty()) {
            break;
        }

        // Add to current input
        if (!currentInput.empty()) {
            currentInput += "\n";
        }
        currentInput += line;

        // Check if input is complete
        if (detector.isComplete(currentInput)) {
            // Input complete, execute code
            if (!currentInput.empty()) {
                executeCode(state, currentInput);
            }
            currentInput.clear();
        } else {
            // Input incomplete, check for obvious syntax errors
            try {
                Lua::Parser parser(currentInput);
                parser.parse();

                // If parsing succeeds but detector thinks incomplete, continue reading
                // In this case, detector might be more accurate
            } catch (const std::exception& e) {
                // Parsing failed, might be syntax error or incomplete input
                // If current input is only one line, might be syntax error
                if (currentInput.find('\n') == std::string::npos) {
                    // Single line input parsing failed, might be syntax error
                    // Use the same error reporting system as file execution
                    Lua::Parser parser(currentInput);
                    try {
                        parser.parse();
                    } catch (const std::exception& parseError) {
                        // Parsing threw an exception, but we still want to get formatted errors
                    }

                    if (parser.hasError()) {
                        // Output parsing errors in Lua 5.1 format
                        std::string formattedErrors = parser.getFormattedErrors();
                        if (!formattedErrors.empty()) {
                            std::cerr << formattedErrors << std::endl;
                        }
                    }
                    currentInput.clear();
                } else {
                    // Multi-line input, continue waiting for more input
                }
            }
        }
    }
}

// Format and display Lua value
std::string formatValue(const Lua::Value& value) {
    if (value.isNil()) {
        return "nil";
    } else if (value.isBoolean()) {
        return value.asBoolean() ? "true" : "false";
    } else if (value.isNumber()) {
        double num = value.asNumber();
        // Check if it's an integer
        if (num == std::floor(num) && num >= -2147483648.0 && num <= 2147483647.0) {
            return std::to_string(static_cast<long long>(num));
        } else {
            return std::to_string(num);
        }
    } else if (value.isString()) {
        return "\"" + value.toString() + "\"";
    } else if (value.isTable()) {
        return "table: " + std::to_string(reinterpret_cast<uintptr_t>(value.asTable().get()));
    } else if (value.isFunction()) {
        return "function: " + std::to_string(reinterpret_cast<uintptr_t>(value.asFunction().get()));
    } else {
        return "userdata";
    }
}

// Check if code is pure expression (not statement)
bool isPureExpression(const std::string& code) {
    // Simple heuristic check
    std::string trimmed = code;
    // Remove leading and trailing whitespace
    size_t start = trimmed.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return false;
    trimmed = trimmed.substr(start);

    size_t end = trimmed.find_last_not_of(" \t\n\r");
    if (end != std::string::npos) {
        trimmed = trimmed.substr(0, end + 1);
    }

    // Check if starts with statement keywords
    const std::vector<std::string> stmtKeywords = {
        "local", "function", "if", "while", "for", "repeat", "do", "return", "break"
    };

    for (const auto& keyword : stmtKeywords) {
        if (trimmed.length() >= keyword.length() &&
            trimmed.substr(0, keyword.length()) == keyword &&
            (trimmed.length() == keyword.length() ||
             std::isspace(trimmed[keyword.length()]))) {
            return false;
        }
    }

    // Check if contains assignment operation
    if (trimmed.find('=') != std::string::npos) {
        // Simple check, might need more complex parsing
        size_t eqPos = trimmed.find('=');
        if (eqPos > 0 && eqPos < trimmed.length() - 1) {
            char before = trimmed[eqPos - 1];
            char after = trimmed[eqPos + 1];
            // Exclude ==, <=, >=, ~= comparison operators
            if (before != '=' && before != '<' && before != '>' && before != '~' &&
                after != '=') {
                return false; // Might be assignment statement
            }
        }
    }

    return true;
}



void executeCode(Lua::State& state, const std::string& code) {
    bool isExpression = isPureExpression(code);

    if (isExpression) {
        // Try to execute as expression
        try {
            std::string exprCode = "return " + code;
            Lua::Value result = state.doStringWithResult(exprCode);

            // Display result if it's not nil
            if (!result.isNil()) {
                std::cout << formatValue(result) << std::endl;
            }
            return;
        } catch (const Lua::LuaException& e) {
            std::string errorMsg = e.what();

            // Check if it's any type of parse error that would likely occur again
            // These patterns indicate syntax/lexical errors that won't be fixed by trying as statement
            bool isParseError = (errorMsg.find("Parse error") != std::string::npos ||
                               errorMsg.find("unexpected symbol") != std::string::npos ||
                               errorMsg.find("unfinished string") != std::string::npos ||
                               errorMsg.find("malformed number") != std::string::npos ||
                               errorMsg.find("stdin:") != std::string::npos);

            if (isParseError) {
                return; // Parse error already displayed, don't try again
            }

            // For other errors, we might want to display them
            std::cerr << "lua: " << errorMsg << std::endl;
            return;
        } catch (...) {
            // Failed as expression, continue trying as statement
        }
    }

    // Execute as statement using doString which already handles error formatting
    bool success = state.doString(code);
    // doString already outputs formatted errors, so we don't need additional error handling here
}