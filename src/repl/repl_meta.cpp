/**
 * @file repl_meta.cpp
 * @brief REPL 字节码、AST 与垃圾回收元命令的实现
 */

#include "repl/repl_meta.hpp"

#include "bytecode/bytecode_printer.hpp"
#include "common/lua_error.hpp"
#include "compiler/ast_visitor.hpp"
#include "compiler/codegen/codegen.hpp"
#include "compiler/parser/parser.hpp"
#include "core/function.hpp"
#include "gc/gc_strategy.hpp"
#include "repl/repl_exe.hpp"
#include "repl/repl_txt.hpp"
#include "runtime/runtime_services.hpp"

#include <expected>
#include <exception>
#include <format>
#include <ostream>
#include <string_view>

namespace Lua::REPL {
namespace detail {

std::expected<Proto*, ParseError> compileForBytecode(LuaState* L, const Str& source) {
    RuntimeServices services(L->getGlobalState());

    Parser parser(source, services);
    auto parsed = parser.parse();
    if (!parsed) {
        return std::unexpected(parsed.error());
    }

    CodeGenerator codegen(services);
    Proto* proto = codegen.generate(*parsed, "=(repl bytecode)");
    if (proto == nullptr) {
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

void printParseError(ReplContext& context, std::ostream& err, const ParseError& error) {
    reportError(context, err, "stdin", error.getLine(), error.what(), false);
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
    out << "usage: .gc [stats|collect|strategy|help]" << '\n';
}

void printGcStrategy(RuntimeServices& services, std::ostream& out) {
    out << "GC" << '\n';
    out << "  command: strategy" << '\n';
    out << "  active: " << services.gc.getStrategyName() << '\n';
    out << "  active summary: " << services.gc.getStrategy().summary() << '\n';
    out << "  available: mark-sweep, incremental" << '\n';
    out << "  planned: incremental write barriers and scheduling" << '\n';
    out << "  boundary: RuntimeServices.gc owns the active collector" << '\n';
    out << "  switch: collectgarbage(\"strategy\", \"mark-sweep\"|\"incremental\")" << '\n';
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

class AstPrinter : public AstVisitor<AstPrinter> {
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
        lineWithLocation(std::format("CallExpr method={}", node.isMethodCall ? "true" : "false"),
                         node);
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
        visit(expr);
    }

    void printStmt(const Stmt& stmt) {
        visit(stmt);
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
                           fullName, node.isLocal ? "true" : "false",
                           node.isMethod ? "true" : "false", joinNames(node.params),
                           node.isVararg ? "true" : "false");
    }

    std::ostream& out_;
    usize indent_ = 0;
};

int printBytecode(ReplContext& context, LuaState* L, const Str& source, std::ostream& out,
                  std::ostream& err) {
    if (L == nullptr) {
        reportError(context, err, ".bytecode: LuaState is null", false);
        return 1;
    }

    const Str input = trimCopy(source);
    if (input.empty()) {
        reportError(context, err, "usage: .bytecode <expr|chunk>", false);
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
            printParseError(context, err, expression.error());
            return 1;
        }

        printParseError(context, err, primary.error());
        return 1;
    } catch (const LuaError& e) {
        reportError(context, err, e.what(), false);
        return 1;
    } catch (const std::exception& e) {
        reportError(context, err, e.what(), false);
        return 1;
    }
}

int printAst(ReplContext& context, LuaState* L, const Str& source, std::ostream& out,
             std::ostream& err) {
    if (L == nullptr) {
        reportError(context, err, ".ast: LuaState is null", false);
        return 1;
    }

    const Str input = trimCopy(source);
    if (input.empty()) {
        reportError(context, err, "usage: .ast <expr|chunk>", false);
        return 1;
    }

    try {
        bool wasExplicitReturn = false;
        const Str primarySource = tryAsExpression(input, wasExplicitReturn);
        auto primary = parseForAst(L, primarySource);
        if (primary) {
            out << "AST" << '\n';
            out << "  mode: " << (wasExplicitReturn ? "expression" : "chunk") << '\n';
            AstPrinter printer(out);
            printer.print(*primary);
            return 0;
        }

        if (!wasExplicitReturn) {
            auto expression = parseForAst(L, "return " + input);
            if (expression) {
                out << "AST" << '\n';
                out << "  mode: expression" << '\n';
                AstPrinter printer(out);
                printer.print(*expression);
                return 0;
            }
            printParseError(context, err, expression.error());
            return 1;
        }

        printParseError(context, err, primary.error());
        return 1;
    } catch (const LuaError& e) {
        reportError(context, err, e.what(), false);
        return 1;
    } catch (const std::exception& e) {
        reportError(context, err, e.what(), false);
        return 1;
    }
}

int printGc(ReplContext& context, LuaState* L, const Str& argument, std::ostream& out,
            std::ostream& err) {
    if (L == nullptr) {
        reportError(context, err, ".gc: LuaState is null", false);
        return 1;
    }

    const Str option = trimCopy(argument);
    if (option.empty() || option == "stats" || option == "status") {
        RuntimeServices services(L->getGlobalState());
        out << "GC" << '\n';
        out << "  command: stats" << '\n';
        out << "  strategy: " << services.gc.getStrategyName() << '\n';
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
        out << "  strategy: " << services.gc.getStrategyName() << '\n';
        out << "  collected objects: " << collected << '\n';
        printGcSnapshot(out, "  before:", before);
        printGcSnapshot(out, "  after:", after);
        return 0;
    }

    if (option == "strategy") {
        RuntimeServices services(L->getGlobalState());
        printGcStrategy(services, out);
        return 0;
    }

    if (option == "help") {
        printGcUsage(out);
        return 0;
    }

    reportError(context, err, std::format(".gc: unknown option '{}'", option), false);
    printGcUsage(err);
    return 1;
}

int runMetaCommand(ReplContext& context, LuaState* L, const MetaCommand& command,
                   std::ostream& out, std::ostream& err) {
    switch (command.kind) {
        case MetaCommandKind::None:
            return 0;
        case MetaCommandKind::Help:
            printHelp(out);
            return 0;
        case MetaCommandKind::Bytecode:
            return printBytecode(context, L, command.argument, out, err);
        case MetaCommandKind::Ast:
            return printAst(context, L, command.argument, out, err);
        case MetaCommandKind::Gc:
            return printGc(context, L, command.argument, out, err);
        case MetaCommandKind::Unknown:
            reportError(context, err, std::format("unknown REPL command: .{}", command.argument),
                        false);
            return 1;
    }

    return 1;
}

}  // namespace detail

MetaCommand parseMetaCommand(const Str& line) {
    const Str trimmed = detail::trimCopy(line);
    if (trimmed.empty() || trimmed[0] != '.') {
        return {};
    }

    usize commandEnd = 1;
    while (commandEnd < trimmed.size() && !detail::isSpace(trimmed[commandEnd])) {
        commandEnd++;
    }

    const Str command = trimmed.substr(1, commandEnd - 1);
    const Str argument = detail::trimCopy(trimmed.substr(commandEnd));

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
    out << "REPL commands:" << '\n';
    out << "  .help                  show this help" << '\n';
    out << "  .bytecode <expr|chunk> compile input and print bytecode" << '\n';
    out << "  .ast <expr|chunk>      parse input and print AST" << '\n';
    out << "  .gc [stats|collect|strategy|help] inspect or run the active GC" << '\n';
    out << "  =expr                  evaluate expression and print results" << '\n';
    out << "  exit, quit             leave the REPL" << '\n';
}

int printBytecode(LuaState* L, const Str& source, std::ostream& out, std::ostream& err) {
    return detail::printBytecode(detail::globalContext(), L, source, out, err);
}

int printAst(LuaState* L, const Str& source, std::ostream& out, std::ostream& err) {
    return detail::printAst(detail::globalContext(), L, source, out, err);
}

int printGc(LuaState* L, const Str& argument, std::ostream& out, std::ostream& err) {
    return detail::printGc(detail::globalContext(), L, argument, out, err);
}

int runMetaCommand(LuaState* L, const MetaCommand& command, std::ostream& out,
                   std::ostream& err) {
    return detail::runMetaCommand(detail::globalContext(), L, command, out, err);
}

}  // namespace Lua::REPL
