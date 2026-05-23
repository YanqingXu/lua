/**
 * @file repl.cpp
 * @brief REPL（交互式解释器）模块实现
 *
 * 详细说明：
 * 本文件实现了 Lua 的交互式 REPL（Read-Eval-Print Loop）功能，
 * 参考官方 Lua 5.1.5 的 lua.c 中的 dotty()、loadline()、pushline() 等函数。
 *
 * 改进功能（参考 lua_with_cpp/src/repl.cpp）：
 * - 信号处理：支持 Ctrl+C 中断
 * - 可配置提示符：从 _PROMPT/_PROMPT2 全局变量读取
 * - exit() 全局函数：支持退出码
 * - _VERSION 全局变量：自动设置
 *
 * @author Lua C++ Project
 * @date 2025-12-04
 */

#include "repl.hpp"
#include "bytecode/bytecode_printer.hpp"
#include "vm/vm.hpp"
#include "vm/state/global_state.hpp"
#include "compiler/parser/parser.hpp"
#include "compiler/codegen/codegen.hpp"
#include "compiler/ast_visitor.hpp"
#include "core/string_pool.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "gc/garbage_collector.hpp"
#include "runtime/runtime_services.hpp"
#include "common/lua_error.hpp"

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <string>
#include <string_view>
#include <csignal>
#include <cctype>
#include <cstdlib>
#include <expected>
#include <format>
#include <fstream>
#include <ostream>

#ifdef _WIN32
#include <conio.h>
#include <io.h>
#else
#include <unistd.h>
#endif

namespace Lua {
namespace REPL {

// ============================================================================
// 全局状态（用于信号处理和错误报告）
// ============================================================================

/// 全局 LuaState 指针，用于信号处理器访问
static LuaState* g_currentState = nullptr;

/// 中断标志，由 SIGINT 信号处理器设置
static volatile sig_atomic_t g_interrupted = 0;

/// 程序名（用于错误消息前缀），参考官方 Lua 的 progname
static const char* g_progname = DEFAULT_PROGNAME;

// ============================================================================
// 错误报告函数（参考 lua_c_analysis/src/lua.c 的 l_message 和 report）
// ============================================================================

void setProgName(const char* name) {
    if (name != nullptr && name[0] != '\0') {
        // 提取基本文件名（去除路径）
        const char* p = name;
        const char* lastSep = nullptr;
        while (*p) {
            if (*p == '/' || *p == '\\') {
                lastSep = p;
            }
            p++;
        }
        g_progname = lastSep ? lastSep + 1 : name;
    } else {
        g_progname = DEFAULT_PROGNAME;
    }
}

const char* getProgName() {
    return g_progname;
}

void reportError(const char* msg, bool showProgName) {
    // 参考官方 Lua 的 l_message() 函数
    if (showProgName && g_progname) {
        std::cerr << std::format("{}: {}", g_progname, msg) << std::endl;
    } else {
        std::cerr << msg << std::endl;
    }
}

void reportError(const char* source, int line, const char* msg, bool showProgName) {
    // 格式（脚本模式）：progname: source:line: message
    // 格式（REPL 模式）：source:line: message
    const Str message = std::format("{}:{}: {}", source, line, msg);
    if (showProgName && g_progname) {
        std::cerr << std::format("{}: {}", g_progname, message) << std::endl;
    } else {
        std::cerr << message << std::endl;
    }
}

// ============================================================================
// 内部辅助函数（匿名命名空间）
// ============================================================================

namespace {

/**
 * @brief SIGINT (Ctrl+C) 信号处理器
 *
 * 当用户按下 Ctrl+C 时，设置中断标志而不是终止程序。
 * 这允许 REPL 优雅地处理中断，取消当前输入并继续运行。
 *
 * @param signal 信号编号
 */
void signalHandler([[maybe_unused]] int signal) {
    g_interrupted = 1;
    // 在 Windows 上，需要重新注册信号处理器
#ifdef _WIN32
    std::signal(SIGINT, signalHandler);
#endif
}

/**
 * @brief 安装信号处理器
 */
void installSignalHandler() {
    std::signal(SIGINT, signalHandler);
}

/**
 * @brief 恢复默认信号处理
 */
void restoreSignalHandler() {
    std::signal(SIGINT, SIG_DFL);
}

/**
 * @brief 检查是否被中断
 * @return true 如果收到 Ctrl+C 信号
 */
bool wasInterrupted() {
    return g_interrupted != 0;
}

/**
 * @brief 清除中断标志
 */
void clearInterruptFlag() {
    g_interrupted = 0;
}

bool isSpace(char ch) {
    return std::isspace(static_cast<unsigned char>(ch)) != 0;
}

Str trimCopy(const Str& text) {
    usize first = 0;
    while (first < text.size() && isSpace(text[first])) {
        first++;
    }

    usize last = text.size();
    while (last > first && isSpace(text[last - 1])) {
        last--;
    }

    return text.substr(first, last - first);
}

bool startsWith(std::string_view text, std::string_view prefix) {
    return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

bool isIdentifierChar(char ch) {
    return std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '_';
}

bool isCompletionTokenChar(char ch) {
    return isIdentifierChar(ch) || ch == '.';
}

void sortUnique(Vec<Str>& values) {
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
}

Str commonPrefix(const Vec<Str>& values) {
    if (values.empty()) {
        return "";
    }

    Str prefix = values.front();
    for (usize i = 1; i < values.size(); ++i) {
        const Str& value = values[i];
        usize len = 0;
        while (len < prefix.size() && len < value.size() && prefix[len] == value[len]) {
            len++;
        }
        prefix.resize(len);
        if (prefix.empty()) {
            break;
        }
    }
    return prefix;
}

CompletionResult buildCompletionResult(const Str& line, usize tokenStart, const Str& token,
                                       Vec<Str> candidates) {
    sortUnique(candidates);

    CompletionResult result;
    result.completedLine = line;
    result.candidates = std::move(candidates);

    if (result.candidates.empty()) {
        return result;
    }

    const Str replacement = result.candidates.size() == 1
                                ? result.candidates.front()
                                : commonPrefix(result.candidates);
    if (replacement.size() > token.size()) {
        result.completedLine = line.substr(0, tokenStart) + replacement;
    }

    return result;
}

void collectStringKeys(Table* table, std::string_view prefix, std::string_view candidatePrefix,
                       Vec<Str>& candidates) {
    if (table == nullptr) {
        return;
    }

    Value key;
    Value nextKey;
    Value nextValue;
    while (table->next(key, nextKey, nextValue)) {
        if (nextKey.isString()) {
            const Str name = nextKey.asString()->c_str();
            if (startsWith(name, prefix)) {
                candidates.push_back(Str(candidatePrefix) + name);
            }
        }
        key = nextKey;
    }
}

Vec<Str> splitDottedPath(std::string_view path) {
    Vec<Str> parts;
    usize start = 0;
    while (start <= path.size()) {
        const usize dot = path.find('.', start);
        const usize end = dot == std::string_view::npos ? path.size() : dot;
        if (end == start) {
            return {};
        }
        parts.emplace_back(path.substr(start, end - start));
        if (dot == std::string_view::npos) {
            break;
        }
        start = dot + 1;
    }
    return parts;
}

Table* resolveTablePath(LuaState* L, std::string_view path) {
    if (L == nullptr || path.empty()) {
        return nullptr;
    }

    Vec<Str> parts = splitDottedPath(path);
    if (parts.empty()) {
        return nullptr;
    }

    StringPool& pool = L->getGlobalState().getStringPool();
    Value current = L->getGlobal(parts.front());
    for (usize i = 1; i < parts.size(); ++i) {
        if (!current.isTable()) {
            return nullptr;
        }
        GCString* key = pool.intern(parts[i]);
        current = current.asTable()->get(Value(key));
    }

    return current.isTable() ? current.asTable() : nullptr;
}

usize findCompletionTokenStart(const Str& line) {
    usize start = line.size();
    while (start > 0 && isCompletionTokenChar(line[start - 1])) {
        start--;
    }
    return start;
}

Vec<Str> completeMetaCommandToken(std::string_view token) {
    static constexpr std::string_view kCommands[] = {
        ".ast",
        ".bytecode",
        ".gc",
        ".help",
    };

    Vec<Str> candidates;
    for (std::string_view command : kCommands) {
        if (startsWith(command, token)) {
            candidates.emplace_back(command);
        }
    }
    return candidates;
}

Vec<Str> completeGcOption(std::string_view token) {
    static constexpr std::string_view kOptions[] = {
        "collect",
        "help",
        "stats",
        "status",
        "strategy",
    };

    Vec<Str> candidates;
    for (std::string_view option : kOptions) {
        if (startsWith(option, token)) {
            candidates.emplace_back(option);
        }
    }
    return candidates;
}

CompletionResult completeMetaInput(const Str& line, usize commandStart) {
    usize commandEnd = commandStart;
    while (commandEnd < line.size() && !isSpace(line[commandEnd])) {
        commandEnd++;
    }

    if (commandEnd == line.size()) {
        const Str token = line.substr(commandStart);
        return buildCompletionResult(line, commandStart, token, completeMetaCommandToken(token));
    }

    const Str command = line.substr(commandStart, commandEnd - commandStart);
    if (command != ".gc") {
        return {line, {}};
    }

    usize optionStart = line.size();
    while (optionStart > commandEnd && !isSpace(line[optionStart - 1])) {
        optionStart--;
    }

    const Str token = line.substr(optionStart);
    return buildCompletionResult(line, optionStart, token, completeGcOption(token));
}

CompletionResult completeLuaInput(LuaState* L, const Str& line) {
    if (L == nullptr) {
        return {line, {}};
    }

    const usize tokenStart = findCompletionTokenStart(line);
    const Str token = line.substr(tokenStart);
    if (token.empty()) {
        return {line, {}};
    }

    Vec<Str> candidates;
    const usize dot = token.rfind('.');
    if (dot != Str::npos) {
        const Str tablePath = token.substr(0, dot);
        const Str fieldPrefix = token.substr(dot + 1);
        Table* table = resolveTablePath(L, tablePath);
        collectStringKeys(table, fieldPrefix, tablePath + ".", candidates);
        return buildCompletionResult(line, tokenStart, token, std::move(candidates));
    }

    collectStringKeys(L->getGlobalTable(), token, "", candidates);
    return buildCompletionResult(line, tokenStart, token, std::move(candidates));
}

bool isInputTerminal() {
#ifdef _WIN32
    return _isatty(_fileno(stdin)) != 0;
#else
    return isatty(fileno(stdin)) != 0;
#endif
}

void printCompletionCandidates(const Vec<Str>& candidates) {
    if (candidates.empty()) {
        return;
    }

    std::cout << '\n';
    for (usize i = 0; i < candidates.size(); ++i) {
        if (i != 0) {
            std::cout << "  ";
        }
        std::cout << candidates[i];
    }
    std::cout << '\n';
}

void redrawInputLine(const Str& prompt, const Str& line) {
    std::cout << prompt << line << std::flush;
}

void applyInteractiveCompletion(LuaState* L, const Str& prompt, Str& line) {
    const CompletionResult completion = completeInput(L, line);
    if (completion.candidates.empty()) {
        std::cout << '\a' << std::flush;
        return;
    }

    const Str oldLine = line;
    line = completion.completedLine;

    if (completion.candidates.size() > 1) {
        printCompletionCandidates(completion.candidates);
        redrawInputLine(prompt, line);
        return;
    }

    if (startsWith(line, oldLine)) {
        std::cout << line.substr(oldLine.size()) << std::flush;
        return;
    }

    std::cout << '\n';
    redrawInputLine(prompt, line);
}

void applySubmittedTabCompletion(LuaState* L, Str& line) {
    usize tab = line.find('\t');
    while (tab != Str::npos) {
        const Str beforeTab = line.substr(0, tab);
        const Str afterTab = line.substr(tab + 1);
        const CompletionResult completion = completeInput(L, beforeTab);
        line = completion.completedLine + afterTab;
        if (completion.candidates.size() > 1) {
            printCompletionCandidates(completion.candidates);
        }
        tab = line.find('\t');
    }
}

/**
 * @brief 检测输入是否因为不完整而导致解析失败
 *
 * 参考 lua_c_analysis/src/lua.c 的 incomplete() 函数：
 * 如果错误消息以 "<eof>" 结尾，说明输入不完整（需要更多输入）
 *
 * @param errorMessage 解析错误消息
 * @return true 如果输入不完整，需要更多输入
 */
bool isIncompleteInput(const Str& errorMessage) {
    // 官方 Lua 的不完整输入错误以 "'<eof>'" 或 "<eof>" 结尾
    const char* eofPatterns[] = {
        "<eof>",
        "'end' expected",
        "Expected 'end'",
        "'until' expected",
        "Expected 'until'",
        "unexpected end of input",
        "Unexpected token in expression",
        "to close function",
        "to close 'if'",
        "to close 'while'",
        "to close 'for'",
        "to close 'do'",
    };

    for (const char* pattern : eofPatterns) {
        if (errorMessage.find(pattern) != Str::npos) {
            return true;
        }
    }

    return false;
}

/**
 * @brief 获取提示符（支持用户自定义）
 *
 * 从 Lua 全局变量 _PROMPT 或 _PROMPT2 读取提示符。
 * 如果全局变量未设置或不是字符串，返回默认值。
 *
 * 这允许用户通过设置 _PROMPT = ">>> " 来自定义提示符。
 *
 * @param L Lua 状态机
 * @param firstLine 是否是第一行（true 返回主提示符，false 返回续行提示符）
 * @return 提示符字符串
 */
Str getPrompt(LuaState* L, bool firstLine) {
    // 缓存提示符字符串，避免每次都创建
    static Str cachedPrompt1;
    static Str cachedPrompt2;

    const char* varName = firstLine ? "_PROMPT" : "_PROMPT2";
    const char* defaultPrompt = firstLine ? DEFAULT_PROMPT1 : DEFAULT_PROMPT2;
    Str& cachedPrompt = firstLine ? cachedPrompt1 : cachedPrompt2;

    try {
        // getGlobal 使用 const Str& 参数
        Value val = L->getGlobal(varName);

        if (val.isString()) {
            cachedPrompt = val.asString()->c_str();
            return cachedPrompt;
        }
    } catch (...) {
        // 忽略错误，使用默认提示符
    }

    return defaultPrompt;
}

/**
 * @brief 读取一行用户输入
 *
 * @param prompt 显示的提示符
 * @param line [out] 读取的行
 * @return true 如果成功读取，false 如果 EOF 或被中断
 */
bool readLine(LuaState* L, const Str& prompt, Str& line) {
    std::cout << prompt << std::flush;

    // 检查是否被中断
    if (wasInterrupted()) {
        clearInterruptFlag();
        std::cout << std::endl;
        line.clear();
        return true;  // 返回 true 但 line 为空，让主循环继续
    }

    if (isInputTerminal()) {
#ifdef _WIN32
        line.clear();
        while (true) {
            const int ch = _getch();
            if (ch == 0 || ch == 224) {
                (void)_getch();
                continue;
            }
            if (ch == '\r' || ch == '\n') {
                std::cout << std::endl;
                return true;
            }
            if (ch == '\t') {
                applyInteractiveCompletion(L, prompt, line);
                continue;
            }
            if (ch == '\b' || ch == 127) {
                if (!line.empty()) {
                    line.pop_back();
                    std::cout << "\b \b" << std::flush;
                }
                continue;
            }
            if (ch == 4 || ch == 26) {
                return false;
            }
            if (ch == 3) {
                clearInterruptFlag();
                std::cout << std::endl;
                line.clear();
                return true;
            }
            if (std::isprint(static_cast<unsigned char>(ch)) != 0) {
                line.push_back(static_cast<char>(ch));
                std::cout << static_cast<char>(ch) << std::flush;
            }
        }
#endif
    }

    if (!std::getline(std::cin, line)) {
        // 检查是否是因为中断导致的读取失败
        if (wasInterrupted()) {
            clearInterruptFlag();
            std::cin.clear();  // 清除 EOF 状态
            std::cout << std::endl;
            line.clear();
            return true;
        }
        return false;  // EOF (Ctrl+D on Unix, Ctrl+Z on Windows)
    }

    // 处理 Windows 的 \r\n 换行符
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }

    applySubmittedTabCompletion(L, line);

    return true;
}

/**
 * @brief 检查并处理 "=" 前缀的快速表达式求值
 *
 * 官方 Lua 的行为：
 * - 如果输入以 "=" 开头，将其视为 "return <表达式>"
 * - 这允许用户快速求值表达式并打印结果
 *
 * @param source 原始输入
 * @param wasExplicitReturn [out] 是否使用了 "=" 前缀
 * @return 转换后的源码（如果需要），否则返回原始源码
 */
Str tryAsExpression(const Str& source, bool& wasExplicitReturn) {
    if (!source.empty() && source[0] == '=') {
        wasExplicitReturn = true;
        return "return " + source.substr(1);
    }
    wasExplicitReturn = false;
    return source;
}

std::expected<Proto*, ParseError> compileForBytecode(LuaState* L, const Str& source) {
    RuntimeServices services(L->getGlobalState());

    Parser parser(source, services);
    auto parsed = parser.parse();
    if (!parsed) {
        return std::unexpected(parsed.error());
    }

    CodeGenerator codegen(services);
    Proto* proto = codegen.generate(*parsed, "=(repl bytecode)");
    if (!proto) {
        throw RuntimeError("code generation failed");
    }

    return proto;
}

std::expected<Chunk, ParseError> parseForAst(LuaState* L, const Str& source) {
    RuntimeServices services(L->getGlobalState());

    Parser parser(source, services);
    auto parsed = parser.parse();
    if (!parsed) {
        return std::unexpected(parsed.error());
    }

    return std::move(*parsed);
}

void printParseError(std::ostream& err, const ParseError& error) {
    err << std::format("stdin:{}: {}", error.getLine(), error.what()) << std::endl;
}

struct GcSnapshot {
    usize objects = 0;
    usize roots = 0;
    usize memoryBytes = 0;
};

GcSnapshot captureGcSnapshot(RuntimeServices& services) {
    GcSnapshot snapshot;
    services.gc.getStatistics(snapshot.objects, snapshot.roots, snapshot.memoryBytes);
    return snapshot;
}

double memoryKilobytes(usize bytes) {
    return static_cast<double>(bytes) / 1024.0;
}

void printGcSnapshot(std::ostream& out, std::string_view label, const GcSnapshot& snapshot) {
    out << label << '\n';
    out << "    objects: " << snapshot.objects << '\n';
    out << "    roots: " << snapshot.roots << '\n';
    out << "    memory bytes: " << snapshot.memoryBytes << '\n';
    out << std::format("    memory KB: {:.3f}\n", memoryKilobytes(snapshot.memoryBytes));
}

void printGcUsage(std::ostream& out) {
    out << "usage: .gc [stats|collect|strategy|help]" << std::endl;
}

void printGcStrategy(std::ostream& out) {
    out << "GC" << '\n';
    out << "  command: strategy" << '\n';
    out << "  active: mark-sweep" << '\n';
    out << "  available: mark-sweep" << '\n';
    out << "  planned: incremental" << '\n';
    out << "  boundary: RuntimeServices.gc owns the active collector" << '\n';
}

const char* binaryOpName(BinaryExpr::Op op) {
    switch (op) {
        case BinaryExpr::Op::Add:
            return "Add";
        case BinaryExpr::Op::Sub:
            return "Sub";
        case BinaryExpr::Op::Mul:
            return "Mul";
        case BinaryExpr::Op::Div:
            return "Div";
        case BinaryExpr::Op::Mod:
            return "Mod";
        case BinaryExpr::Op::Pow:
            return "Pow";
        case BinaryExpr::Op::Eq:
            return "Eq";
        case BinaryExpr::Op::Ne:
            return "Ne";
        case BinaryExpr::Op::Lt:
            return "Lt";
        case BinaryExpr::Op::Le:
            return "Le";
        case BinaryExpr::Op::Gt:
            return "Gt";
        case BinaryExpr::Op::Ge:
            return "Ge";
        case BinaryExpr::Op::And:
            return "And";
        case BinaryExpr::Op::Or:
            return "Or";
        case BinaryExpr::Op::Concat:
            return "Concat";
    }

    return "Unknown";
}

const char* unaryOpName(UnaryExpr::Op op) {
    switch (op) {
        case UnaryExpr::Op::Not:
            return "Not";
        case UnaryExpr::Op::Neg:
            return "Neg";
        case UnaryExpr::Op::Len:
            return "Len";
    }

    return "Unknown";
}

Str escapeAstString(const Str& value) {
    Str escaped;
    for (char ch : value) {
        switch (ch) {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped += ch;
                break;
        }
    }
    return escaped;
}

class AstPrinter : public ExprVisitor<AstPrinter>, public StmtVisitor<AstPrinter> {
public:
    explicit AstPrinter(std::ostream& out) : out_(out) {}

    void print(const Chunk& chunk) {
        line("Chunk");
        IndentGuard indent(*this);
        line(std::format("statements ({})", chunk.statements.size()));
        printStmtList(chunk.statements);
    }

    void visitNode(const NilExpr& node) {
        lineWithLocation("NilExpr", node);
    }

    void visitNode(const BoolExpr& node) {
        lineWithLocation(std::format("BoolExpr value={}", node.value ? "true" : "false"), node);
    }

    void visitNode(const NumberExpr& node) {
        lineWithLocation(std::format("NumberExpr value={}", node.value), node);
    }

    void visitNode(const StringExpr& node) {
        lineWithLocation(std::format("StringExpr value=\"{}\"", escapeAstString(node.value)), node);
    }

    void visitNode(const VarargExpr& node) {
        lineWithLocation("VarargExpr", node);
    }

    void visitNode(const NameExpr& node) {
        lineWithLocation(std::format("NameExpr name={}", node.name), node);
    }

    void visitNode(const BinaryExpr& node) {
        lineWithLocation(std::format("BinaryExpr op={}", binaryOpName(node.op)), node);
        IndentGuard indent(*this);
        printExprField("left", node.left.get());
        printExprField("right", node.right.get());
    }

    void visitNode(const UnaryExpr& node) {
        lineWithLocation(std::format("UnaryExpr op={}", unaryOpName(node.op)), node);
        IndentGuard indent(*this);
        printExprField("operand", node.operand.get());
    }

    void visitNode(const TableExpr& node) {
        lineWithLocation(std::format("TableExpr fields={}", node.fields.size()), node);
        IndentGuard indent(*this);
        for (usize i = 0; i < node.fields.size(); ++i) {
            line(std::format("field[{}]", i));
            IndentGuard fieldIndent(*this);
            printExprField("key", node.fields[i].key.get());
            printExprField("value", node.fields[i].value.get());
        }
    }

    void visitNode(const CallExpr& node) {
        lineWithLocation(std::format("CallExpr method={}", node.isMethodCall ? "true" : "false"), node);
        IndentGuard indent(*this);
        printExprField("func", node.func.get());
        line(std::format("args ({})", node.args.size()));
        printExprList(node.args);
    }

    void visitNode(const IndexExpr& node) {
        lineWithLocation("IndexExpr", node);
        IndentGuard indent(*this);
        printExprField("table", node.table.get());
        printExprField("index", node.index.get());
    }

    void visitNode(const MemberExpr& node) {
        lineWithLocation(std::format("MemberExpr member={}", node.member), node);
        IndentGuard indent(*this);
        printExprField("table", node.table.get());
    }

    void visitNode(const FunctionExpr& node) {
        lineWithLocation(functionLabel("FunctionExpr", node.params, node.isVararg), node);
        IndentGuard indent(*this);
        line(std::format("body ({})", node.body.size()));
        printStmtList(node.body);
    }

    void visitNode(const ParenExpr& node) {
        lineWithLocation("ParenExpr", node);
        IndentGuard indent(*this);
        printExprField("expression", node.expression.get());
    }

    void visitNode(const EmptyStmt& node) {
        lineWithLocation("EmptyStmt", node);
    }

    void visitNode(const AssignStmt& node) {
        lineWithLocation("AssignStmt", node);
        IndentGuard indent(*this);
        line(std::format("targets ({})", node.targets.size()));
        printExprList(node.targets);
        line(std::format("values ({})", node.values.size()));
        printExprList(node.values);
    }

    void visitNode(const LocalStmt& node) {
        lineWithLocation(std::format("LocalStmt names=[{}]", joinNames(node.names)), node);
        IndentGuard indent(*this);
        line(std::format("values ({})", node.values.size()));
        printExprList(node.values);
    }

    void visitNode(const CallStmt& node) {
        lineWithLocation("CallStmt", node);
        IndentGuard indent(*this);
        printExprField("call", node.call.get());
    }

    void visitNode(const IfStmt& node) {
        lineWithLocation(std::format("IfStmt branches={}", node.branches.size()), node);
        IndentGuard indent(*this);
        for (usize i = 0; i < node.branches.size(); ++i) {
            line(std::format("branch[{}]", i));
            IndentGuard branchIndent(*this);
            printExprField("condition", node.branches[i].condition.get());
            line(std::format("body ({})", node.branches[i].body.size()));
            printStmtList(node.branches[i].body);
        }
        if (!node.elseBranch.empty()) {
            line(std::format("else ({})", node.elseBranch.size()));
            printStmtList(node.elseBranch);
        }
    }

    void visitNode(const WhileStmt& node) {
        lineWithLocation("WhileStmt", node);
        IndentGuard indent(*this);
        printExprField("condition", node.condition.get());
        line(std::format("body ({})", node.body.size()));
        printStmtList(node.body);
    }

    void visitNode(const RepeatStmt& node) {
        lineWithLocation("RepeatStmt", node);
        IndentGuard indent(*this);
        line(std::format("body ({})", node.body.size()));
        printStmtList(node.body);
        printExprField("condition", node.condition.get());
    }

    void visitNode(const ForNumStmt& node) {
        lineWithLocation(std::format("ForNumStmt var={}", node.var), node);
        IndentGuard indent(*this);
        printExprField("init", node.init.get());
        printExprField("limit", node.limit.get());
        printExprField("step", node.step.get());
        line(std::format("body ({})", node.body.size()));
        printStmtList(node.body);
    }

    void visitNode(const ForInStmt& node) {
        lineWithLocation(std::format("ForInStmt vars=[{}]", joinNames(node.vars)), node);
        IndentGuard indent(*this);
        line(std::format("iterators ({})", node.iterators.size()));
        printExprList(node.iterators);
        line(std::format("body ({})", node.body.size()));
        printStmtList(node.body);
    }

    void visitNode(const FunctionStmt& node) {
        lineWithLocation(functionStmtLabel(node), node);
        IndentGuard indent(*this);
        line(std::format("body ({})", node.body.size()));
        printStmtList(node.body);
    }

    void visitNode(const ReturnStmt& node) {
        lineWithLocation("ReturnStmt", node);
        IndentGuard indent(*this);
        line(std::format("values ({})", node.values.size()));
        printExprList(node.values);
    }

    void visitNode(const BreakStmt& node) {
        lineWithLocation("BreakStmt", node);
    }

    void visitNode(const DoStmt& node) {
        lineWithLocation("DoStmt", node);
        IndentGuard indent(*this);
        line(std::format("body ({})", node.body.size()));
        printStmtList(node.body);
    }

private:
    class IndentGuard {
    public:
        explicit IndentGuard(AstPrinter& printer) : printer_(printer) {
            printer_.indent_ += 1;
        }

        ~IndentGuard() {
            printer_.indent_ -= 1;
        }

    private:
        AstPrinter& printer_;
    };

    template <typename Node>
    void lineWithLocation(std::string_view label, const Node& node) {
        line(std::format("{} @ {}:{}", label, node.line, node.column));
    }

    void line(std::string_view text) {
        out_ << Str(indent_ * 2, ' ') << text << '\n';
    }

    void printExpr(const Expr& expr) {
        ExprVisitor<AstPrinter>::visit(expr);
    }

    void printStmt(const Stmt& stmt) {
        StmtVisitor<AstPrinter>::visit(stmt);
    }

    void printExprField(std::string_view label, const Expr* expr) {
        line(std::format("{}:", label));
        IndentGuard indent(*this);
        if (expr == nullptr) {
            line("<none>");
            return;
        }
        printExpr(*expr);
    }

    void printExprList(const Vec<ExprPtr>& expressions) {
        IndentGuard indent(*this);
        for (usize i = 0; i < expressions.size(); ++i) {
            line(std::format("[{}]", i));
            IndentGuard itemIndent(*this);
            printExpr(*expressions[i]);
        }
    }

    void printStmtList(const Vec<StmtPtr>& statements) {
        IndentGuard indent(*this);
        for (usize i = 0; i < statements.size(); ++i) {
            line(std::format("[{}]", i));
            IndentGuard itemIndent(*this);
            printStmt(*statements[i]);
        }
    }

    Str joinNames(const Vec<Str>& names) {
        Str text;
        for (usize i = 0; i < names.size(); ++i) {
            if (i != 0) {
                text += ", ";
            }
            text += names[i];
        }
        return text;
    }

    Str functionLabel(std::string_view nodeName, const Vec<Str>& params, bool isVararg) {
        return std::format("{} params=[{}] vararg={}", nodeName, joinNames(params),
                           isVararg ? "true" : "false");
    }

    Str functionStmtLabel(const FunctionStmt& node) {
        Str fullName = node.name;
        for (const Str& part : node.tablePath) {
            fullName += ".";
            fullName += part;
        }

        return std::format("FunctionStmt name={} local={} method={} params=[{}] vararg={}",
                           fullName,
                           node.isLocal ? "true" : "false",
                           node.isMethod ? "true" : "false",
                           joinNames(node.params),
                           node.isVararg ? "true" : "false");
    }

    std::ostream& out_;
    usize indent_ = 0;
};

/**
 * @brief 执行 REPL 输入并打印结果
 *
 * @param L Lua 状态
 * @param source 源代码
 * @param isExpression 是否是表达式（需要打印结果）
 * @return 执行状态码（0=成功）
 */
int executeREPLInput(LuaState* L, const Str& source, bool isExpression) {
    try {
        // 解析源码
        RuntimeServices services(L->getGlobalState());

        Parser parser(source, services);
        auto parsed = parser.parse();
        if (!parsed) {
            throw parsed.error();
        }
        Chunk chunk = std::move(*parsed);

        // 生成字节码
        CodeGenerator codegen(services);
        Proto* proto = codegen.generate(chunk, "=(repl)");

        if (!proto) {
            std::cerr << "code generation failed" << std::endl;
            return 1;
        }

        // 创建函数对象并注册到 GC
        Function* func = new Function(proto);
        L->getGlobalState().getGC().registerObject(func);

        // 设置函数环境为全局表
        func->setEnv(L->getGlobalTable());

        // 记录执行前的栈大小
        usize stackSizeBefore = L->getStack().size();

        // 执行字节码
        VM::execute(services, L, func);

        // 如果是表达式，打印返回值
        if (isExpression) {
            usize stackSizeAfter = L->getStack().size();

            // 计算新增的返回值数量
            if (stackSizeAfter > stackSizeBefore) {
                usize nresults = stackSizeAfter - stackSizeBefore;

                // 打印所有返回值
                for (usize i = 0; i < nresults; ++i) {
                    usize idx = stackSizeBefore + i;
                    if (idx < L->getStack().size()) {
                        const Value& v = L->getStack()[idx];
                        // 转换为字符串并打印
                        if (v.isNil()) {
                            std::cout << "nil";
                        } else if (v.isBoolean()) {
                            std::cout << (v.asBoolean() ? "true" : "false");
                        } else if (v.isNumber()) {
                            std::cout << v.asNumber();
                        } else if (v.isString()) {
                            std::cout << v.asString()->c_str();
                        } else if (v.isTable()) {
                            std::cout << "table: " << v.asTable();
                        } else if (v.isFunction()) {
                            std::cout << "function: " << v.asFunction();
                        } else {
                            std::cout << v.toString();
                        }
                        if (i < nresults - 1) {
                            std::cout << "\t";  // 多值用 tab 分隔
                        }
                    }
                }
                std::cout << std::endl;
            }
        }

        // Proto由GC管理，并通过Function的标记路径保持可达。

        return 0;

    } catch (const ParseError& e) {
        // REPL 模式：不显示程序名前缀
        // 格式：stdin:line: message
        reportError("stdin", e.getLine(), e.what(), false);
        return 1;

    } catch (const LuaError& e) {
        // REPL 模式：不显示程序名前缀
        reportError(e.what(), false);
        return 1;

    } catch (const std::runtime_error& e) {
        // REPL 模式：不显示程序名前缀
        reportError(e.what(), false);
        return 1;

    } catch (const std::exception& e) {
        // REPL 模式：不显示程序名前缀
        reportError(e.what(), false);
        return 1;
    }
}

/**
 * @brief exit() 函数的 C 函数实现
 *
 * 支持可选的退出码参数：
 * - exit() - 退出码为 0
 * - exit(n) - 退出码为 n
 *
 * @param L Lua 状态机
 * @return 不返回（调用 std::exit）
 */
int luaB_exit(LuaState* L) {
    int exitCode = 0;

    // 检查是否有参数
    if (L->getTop() > 0) {
        Value arg = L->at(-1);
        if (arg.isNumber()) {
            exitCode = static_cast<int>(arg.asNumber());
        } else if (arg.isBoolean()) {
            exitCode = arg.asBoolean() ? 0 : 1;
        }
    }

    std::exit(exitCode);
    return 0;  // 永远不会到达
}

}  // anonymous namespace

// ============================================================================
// REPL 公共接口实现
// ============================================================================

MetaCommand parseMetaCommand(const Str& line) {
    const Str trimmed = trimCopy(line);
    if (trimmed.empty() || trimmed[0] != '.') {
        return {};
    }

    usize commandEnd = 1;
    while (commandEnd < trimmed.size() && !isSpace(trimmed[commandEnd])) {
        commandEnd++;
    }

    const Str command = trimmed.substr(1, commandEnd - 1);
    const Str argument = trimCopy(trimmed.substr(commandEnd));

    if (command == "help") {
        return {MetaCommandKind::Help, ""};
    }
    if (command == "bytecode") {
        return {MetaCommandKind::Bytecode, argument};
    }
    if (command == "ast") {
        return {MetaCommandKind::Ast, argument};
    }
    if (command == "gc") {
        return {MetaCommandKind::Gc, argument};
    }

    return {MetaCommandKind::Unknown, command};
}

void printHelp(std::ostream& out) {
    out << "REPL commands:" << std::endl;
    out << "  .help                  show this help" << std::endl;
    out << "  .bytecode <expr|chunk> compile input and print bytecode" << std::endl;
    out << "  .ast <expr|chunk>      parse input and print AST" << std::endl;
    out << "  .gc [stats|collect|strategy|help] inspect or run the active GC" << std::endl;
    out << "  =expr                  evaluate expression and print results" << std::endl;
    out << "  exit, quit             leave the REPL" << std::endl;
}

void recordHistory(Vec<Str>& history, const Str& line) {
    if (!line.empty()) {
        history.push_back(line);
    }
}

bool loadHistory(const Str& path, Vec<Str>& history) {
    history.clear();

    std::ifstream input(path);
    if (!input) {
        return false;
    }

    Str line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        recordHistory(history, line);
    }

    return input.eof() || input.good();
}

bool saveHistory(const Str& path, const Vec<Str>& history) {
    std::ofstream output(path, std::ios::trunc);
    if (!output) {
        return false;
    }

    for (const Str& line : history) {
        output << line << '\n';
    }

    return output.good();
}

CompletionResult completeInput(LuaState* L, const Str& line) {
    usize first = 0;
    while (first < line.size() && isSpace(line[first])) {
        first++;
    }

    if (first < line.size() && line[first] == '.') {
        return completeMetaInput(line, first);
    }

    return completeLuaInput(L, line);
}

int printBytecode(LuaState* L, const Str& source, std::ostream& out, std::ostream& err) {
    if (L == nullptr) {
        err << ".bytecode: LuaState is null" << std::endl;
        return 1;
    }

    const Str input = trimCopy(source);
    if (input.empty()) {
        err << "usage: .bytecode <expr|chunk>" << std::endl;
        return 1;
    }

    try {
        bool wasExplicitReturn = false;
        const Str primarySource = tryAsExpression(input, wasExplicitReturn);
        auto primary = compileForBytecode(L, primarySource);
        if (primary) {
            printProtoBytecode(*primary, out, false);
            return 0;
        }

        if (!wasExplicitReturn) {
            auto expression = compileForBytecode(L, "return " + input);
            if (expression) {
                printProtoBytecode(*expression, out, false);
                return 0;
            }
            printParseError(err, expression.error());
            return 1;
        }

        printParseError(err, primary.error());
        return 1;
    } catch (const LuaError& e) {
        err << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        err << e.what() << std::endl;
        return 1;
    }
}

int printAst(LuaState* L, const Str& source, std::ostream& out, std::ostream& err) {
    if (L == nullptr) {
        err << ".ast: LuaState is null" << std::endl;
        return 1;
    }

    const Str input = trimCopy(source);
    if (input.empty()) {
        err << "usage: .ast <expr|chunk>" << std::endl;
        return 1;
    }

    try {
        bool wasExplicitReturn = false;
        const Str primarySource = tryAsExpression(input, wasExplicitReturn);
        auto primary = parseForAst(L, primarySource);
        if (primary) {
            out << "AST" << std::endl;
            out << "  mode: " << (wasExplicitReturn ? "expression" : "chunk") << std::endl;
            AstPrinter printer(out);
            printer.print(*primary);
            return 0;
        }

        if (!wasExplicitReturn) {
            auto expression = parseForAst(L, "return " + input);
            if (expression) {
                out << "AST" << std::endl;
                out << "  mode: expression" << std::endl;
                AstPrinter printer(out);
                printer.print(*expression);
                return 0;
            }
            printParseError(err, expression.error());
            return 1;
        }

        printParseError(err, primary.error());
        return 1;
    } catch (const LuaError& e) {
        err << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        err << e.what() << std::endl;
        return 1;
    }
}

int printGc(LuaState* L, const Str& argument, std::ostream& out, std::ostream& err) {
    if (L == nullptr) {
        err << ".gc: LuaState is null" << std::endl;
        return 1;
    }

    const Str option = trimCopy(argument);
    if (option.empty() || option == "stats" || option == "status") {
        RuntimeServices services(L->getGlobalState());
        out << "GC" << '\n';
        out << "  command: stats" << '\n';
        out << "  strategy: mark-sweep" << '\n';
        out << "  boundary: RuntimeServices.gc" << '\n';
        printGcSnapshot(out, "  current:", captureGcSnapshot(services));
        return 0;
    }

    if (option == "collect") {
        RuntimeServices services(L->getGlobalState());
        const GcSnapshot before = captureGcSnapshot(services);
        const usize collected = services.gc.collect(L);
        const GcSnapshot after = captureGcSnapshot(services);

        out << "GC" << '\n';
        out << "  command: collect" << '\n';
        out << "  strategy: mark-sweep" << '\n';
        out << "  collected objects: " << collected << '\n';
        printGcSnapshot(out, "  before:", before);
        printGcSnapshot(out, "  after:", after);
        return 0;
    }

    if (option == "strategy") {
        printGcStrategy(out);
        return 0;
    }

    if (option == "help") {
        printGcUsage(out);
        return 0;
    }

    err << std::format(".gc: unknown option '{}'", option) << std::endl;
    printGcUsage(err);
    return 1;
}

int runMetaCommand(LuaState* L, const MetaCommand& command, std::ostream& out, std::ostream& err) {
    switch (command.kind) {
        case MetaCommandKind::None:
            return 0;
        case MetaCommandKind::Help:
            printHelp(out);
            return 0;
        case MetaCommandKind::Bytecode:
            return printBytecode(L, command.argument, out, err);
        case MetaCommandKind::Ast:
            return printAst(L, command.argument, out, err);
        case MetaCommandKind::Gc:
            return printGc(L, command.argument, out, err);
        case MetaCommandKind::Unknown:
            err << std::format("unknown REPL command: .{}", command.argument) << std::endl;
            return 1;
    }

    return 1;
}

void initialize(LuaState* L) {
    RuntimeServices services(L->getGlobalState());
    StringPool& pool = services.strings;

    // 设置 _VERSION 全局变量
    GCString* versionVal = pool.intern(LUA_VERSION);
    L->setGlobal("_VERSION", Value(versionVal));

    // 设置默认提示符（可被用户修改）
    GCString* prompt1Val = pool.intern(DEFAULT_PROMPT1);
    L->setGlobal("_PROMPT", Value(prompt1Val));

    GCString* prompt2Val = pool.intern(DEFAULT_PROMPT2);
    L->setGlobal("_PROMPT2", Value(prompt2Val));

    // 注册 exit() 函数
    Function* exitFunc = new Function(luaB_exit);
    L->getGlobalState().getGC().registerObject(exitFunc);
    L->setGlobal("exit", Value(exitFunc));
}

int run(LuaState* L) {
    // 安装信号处理器
    installSignalHandler();
    g_currentState = L;

    // 显示欢迎信息
    std::cout << VERSION << "  " << COPYRIGHT << std::endl;
    std::cout << "Type '.help' for commands, 'exit' or press Ctrl+D to quit." << std::endl;

    Str inputBuffer;  // 累积的输入
    Vec<Str> history;
    bool isFirstLine = true;

    loadHistory(DEFAULT_HISTORY_FILE, history);

    while (true) {
        // 检查中断标志
        if (wasInterrupted()) {
            clearInterruptFlag();
            std::cout << std::endl;
            inputBuffer.clear();
            isFirstLine = true;
            continue;
        }

        // 获取提示符（支持用户自定义）
        Str prompt = getPrompt(L, isFirstLine);

        // 读取一行输入
        Str line;
        if (!readLine(L, prompt, line)) {
            // EOF，退出 REPL
            std::cout << std::endl;
            break;
        }

        // 检查中断（读取过程中可能被中断）
        if (wasInterrupted()) {
            clearInterruptFlag();
            inputBuffer.clear();
            isFirstLine = true;
            continue;
        }

        recordHistory(history, line);

        // 检查退出命令（仅在首行时检查）
        if (isFirstLine && (line == "exit" || line == "quit")) {
            break;
        }

        // 跳过空行（仅在首行时）
        if (isFirstLine && line.empty()) {
            continue;
        }

        if (isFirstLine) {
            const MetaCommand command = parseMetaCommand(line);
            if (command.kind != MetaCommandKind::None) {
                runMetaCommand(L, command, std::cout, std::cerr);
                inputBuffer.clear();
                isFirstLine = true;
                continue;
            }
        }

        // 累积输入
        bool wasExplicitReturn = false;
        if (isFirstLine) {
            inputBuffer = tryAsExpression(line, wasExplicitReturn);
        } else {
            inputBuffer += "\n" + line;
        }

        // 尝试解析输入
        bool parseSuccess = false;
        bool isExpression = wasExplicitReturn;
        Str sourceToExecute;

        try {
            // 官方 Lua 5.1.5 行为：
            // - 只有 "=expr" 语法才会打印表达式结果
            // - 普通输入直接作为语句处理，不自动包装为表达式
            RuntimeServices services(L->getGlobalState());
            Parser parser(inputBuffer, services);
            auto parsed = parser.parse();
            if (!parsed) {
                throw parsed.error();
            }
            sourceToExecute = inputBuffer;
            isExpression = wasExplicitReturn;  // 只有使用了 "=" 前缀才打印结果
            parseSuccess = true;
        } catch (const ParseError& e) {
            // 检查是否是不完整输入
            if (isIncompleteInput(e.what())) {
                // 需要更多输入，继续读取
                isFirstLine = false;
                continue;
            }

            // 真正的语法错误 - REPL 模式不显示程序名前缀
            // 格式：stdin:line: message
            reportError("stdin", e.getLine(), e.what(), false);
            inputBuffer.clear();
            isFirstLine = true;
            continue;
        }

        // 执行代码
        if (parseSuccess) {
            executeREPLInput(L, sourceToExecute, isExpression);
        }

        // 重置状态，准备下一个输入
        inputBuffer.clear();
        isFirstLine = true;
    }

    // 恢复默认信号处理
    restoreSignalHandler();
    g_currentState = nullptr;

    saveHistory(DEFAULT_HISTORY_FILE, history);

    std::cout << "Goodbye!" << std::endl;
    return 0;
}

}  // namespace REPL
}  // namespace Lua

