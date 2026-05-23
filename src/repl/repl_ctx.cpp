#include "repl/repl_ctx.hpp"

#include "repl/repl_sig.hpp"

#include <cstdio>
#include <format>
#include <ostream>
#include <string_view>

namespace Lua::REPL::detail {
namespace {

constexpr std::string_view kErrorColor = "\x1b[31m";
constexpr std::string_view kResetColor = "\x1b[0m";

bool shouldColorizeErrors(ReplContext& context) {
    switch (context.errorColorMode()) {
        case ErrorColorMode::Never:
            return false;
        case ErrorColorMode::Always:
            return true;
        case ErrorColorMode::Auto:
            break;
    }

    return context.isInteractiveErrorContext()
        && isTerminal(stdout)
        && isTerminal(stderr)
        && enableVirtualTerminalFor(stderr);
}

}  // namespace

void ReplContext::setProgramName(const char* name) {
    if (name != nullptr && name[0] != '\0') {
        const char* p = name;
        const char* lastSep = nullptr;
        while (*p) {
            if (*p == '/' || *p == '\\') {
                lastSep = p;
            }
            p++;
        }
        programName_ = lastSep != nullptr ? lastSep + 1 : name;
        return;
    }

    programName_ = DEFAULT_PROGNAME;
}

const char* ReplContext::programName() const {
    return programName_.c_str();
}

void ReplContext::setErrorColorMode(ErrorColorMode mode) {
    errorColorMode_ = mode;
}

ErrorColorMode ReplContext::errorColorMode() const {
    return errorColorMode_;
}

bool ReplContext::isInteractiveErrorContext() const {
    return interactiveErrorContext_;
}

void ReplContext::setInteractiveErrorContext(bool enabled) {
    interactiveErrorContext_ = enabled;
}

ErrorColorContextGuard::ErrorColorContextGuard(ReplContext& context)
    : context_(context), previous_(context.isInteractiveErrorContext()) {
    context_.setInteractiveErrorContext(true);
}

ErrorColorContextGuard::~ErrorColorContextGuard() {
    context_.setInteractiveErrorContext(previous_);
}

ReplContext& globalContext() {
    static ReplContext context;
    return context;
}

void writeErrorLine(ReplContext& context, std::ostream& err, std::string_view message) {
    if (shouldColorizeErrors(context)) {
        err << kErrorColor << message << kResetColor << '\n';
        return;
    }

    err << message << '\n';
}

void reportError(ReplContext& context, std::ostream& err, std::string_view msg,
                 bool showProgName) {
    if (showProgName && context.programName()[0] != '\0') {
        writeErrorLine(context, err, std::format("{}: {}", context.programName(), msg));
        return;
    }

    writeErrorLine(context, err, msg);
}

void reportError(ReplContext& context, std::ostream& err, std::string_view source, int line,
                 std::string_view msg, bool showProgName) {
    const Str message = std::format("{}:{}: {}", source, line, msg);
    if (showProgName && context.programName()[0] != '\0') {
        writeErrorLine(context, err, std::format("{}: {}", context.programName(), message));
        return;
    }

    writeErrorLine(context, err, message);
}

}  // namespace Lua::REPL::detail
