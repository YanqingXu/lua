/**
 * @file repl_exe.cpp
 * @brief REPL 源码编译、执行和错误转换的实现
 */

#include "repl/repl_exe.hpp"

#include "compiler/codegen/codegen.hpp"
#include "compiler/parser/parser.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/value.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/vm.hpp"
#include "vm/state/global_state.hpp"

#include <exception>
#include <ostream>
#include <stdexcept>

namespace Lua::REPL::detail {
namespace {

void printValue(std::ostream& out, const Value& value) {
    if (value.isNil()) {
        out << "nil";
    } else if (value.isBoolean()) {
        out << (value.asBoolean() ? "true" : "false");
    } else if (value.isNumber()) {
        out << value.asNumber();
    } else if (value.isString()) {
        out << value.asString()->c_str();
    } else if (value.isTable()) {
        out << "table: " << value.asTable();
    } else if (value.isFunction()) {
        out << "function: " << value.asFunction();
    } else {
        out << value.toString();
    }
}

void printExpressionResults(LuaState* L, usize stackSizeBefore, std::ostream& out) {
    const usize stackSizeAfter = L->getStack().size();
    if (stackSizeAfter <= stackSizeBefore) {
        return;
    }

    const usize nresults = stackSizeAfter - stackSizeBefore;
    for (usize i = 0; i < nresults; ++i) {
        const usize idx = stackSizeBefore + i;
        if (idx < L->getStack().size()) {
            printValue(out, L->getStack()[idx]);
            if (i < nresults - 1) {
                out << '\t';
            }
        }
    }
    out << '\n';
}

Function* createFunction(LuaState* L, Proto* proto) {
    Function* func = L->getGlobalState().getGC().create<Function>(proto);
    func->setEnv(L->getGlobalTable());
    return func;
}

} // namespace

Str tryAsExpression(const Str& source, bool& wasExplicitReturn) {
    if (!source.empty() && source[0] == '=') {
        wasExplicitReturn = true;
        return "return " + source.substr(1);
    }
    wasExplicitReturn = false;
    return source;
}

bool isIncompleteInput(const Str& errorMessage) {
    return errorMessage.find("<eof>") != Str::npos || errorMessage.find("unexpected end of input") != Str::npos ||
           errorMessage.find("Unterminated string") != Str::npos ||
           errorMessage.find("Unterminated long string") != Str::npos ||
           errorMessage.find("Unterminated long comment") != Str::npos;
}

std::expected<PreparedInput, ParseError> prepareInputForExecution(LuaState* L, const Str& source, bool isExpression) {
    RuntimeServices services(L->getGlobalState());
    Parser parser(source, services);
    auto parsed = parser.parse();
    if (!parsed) {
        return std::unexpected(parsed.error());
    }

    PreparedInput input;
    input.chunk = std::move(*parsed);
    input.source = source;
    input.isExpression = isExpression;
    return input;
}

int executePreparedInput(ReplContext& context, LuaState* L, PreparedInput&& input, std::ostream& out,
                         std::ostream& err) {
    try {
        RuntimeServices services(L->getGlobalState());
        CodeGenerator codegen(services);
        Proto* proto = codegen.generate(input.chunk, "=(repl)");

        if (proto == nullptr) {
            reportError(context, err, "code generation failed", false);
            return 1;
        }

        Function* func = createFunction(L, proto);
        const usize stackSizeBefore = L->getStack().size();

        VM::execute(services, L, func);

        if (input.isExpression) {
            printExpressionResults(L, stackSizeBefore, out);
        }

        return 0;
    } catch (const ParseError& e) {
        reportError(context, err, "stdin", e.getLine(), e.what(), false);
        return 1;
    } catch (const LuaError& e) {
        reportError(context, err, e.what(), false);
        return 1;
    } catch (const std::runtime_error& e) {
        reportError(context, err, e.what(), false);
        return 1;
    } catch (const std::exception& e) {
        reportError(context, err, e.what(), false);
        return 1;
    }
}

} // namespace Lua::REPL::detail
