/**
 * @file debug_cli.cpp
 * @brief Protocol-independent internal debugger command driver.
 */

#include "debugger/debug_cli.hpp"

#include <charconv>
#include <sstream>

namespace Lua::Debugger {

namespace {

DebugResult<i32> parseLine(StrView text) {
    i32 line = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), line);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() || line <= 0) {
        return std::unexpected(DebugError{DebugErrorCode::InvalidReference, "break expects path:positive-line"});
    }
    return line;
}

} // namespace

DebugResult<Str> DebugCli::execute(StrView command) {
    const usize separator = command.find(' ');
    const StrView verb = command.substr(0, separator);
    const StrView argument = separator == StrView::npos ? StrView{} : command.substr(separator + 1);

    if (verb == "run") {
        DebugResult<void> result = runtime_.configurationDone();
        return result ? DebugResult<Str>{"running"} : DebugResult<Str>{std::unexpected(result.error())};
    }
    if (verb == "pause") {
        DebugResult<void> result = runtime_.pause(ThreadId{1});
        return result ? DebugResult<Str>{"pause pending"} : DebugResult<Str>{std::unexpected(result.error())};
    }
    if (verb == "continue") {
        DebugResult<void> result = runtime_.continueExecution(ThreadId{1});
        return result ? DebugResult<Str>{"continued"} : DebugResult<Str>{std::unexpected(result.error())};
    }
    if (verb == "backtrace") {
        return backtrace();
    }
    if (verb == "locals") {
        return locals();
    }
    if (verb == "break") {
        const usize lineSeparator = argument.rfind(':');
        if (lineSeparator == StrView::npos || lineSeparator == 0) {
            return std::unexpected(DebugError{DebugErrorCode::InvalidReference, "break expects path:positive-line"});
        }
        const DebugResult<i32> line = parseLine(argument.substr(lineSeparator + 1));
        if (!line) {
            return std::unexpected(line.error());
        }
        const SourceId source = runtime_.registerFilePath(argument.substr(0, lineSeparator));
        const SourceBreakpoint requested{*line};
        const DebugResult<Vec<BreakpointBinding>> result = runtime_.setBreakpoints(source, {&requested, 1});
        if (!result || result->empty()) {
            return result ? DebugResult<Str>{std::unexpected(
                                DebugError{DebugErrorCode::RuntimeFailure, "breakpoint response is empty"})}
                          : DebugResult<Str>{std::unexpected(result.error())};
        }
        const BreakpointBinding& binding = result->front();
        return Str("breakpoint ") + (binding.verified ? "verified" : "pending") + " line " +
               std::to_string(binding.line);
    }

    return std::unexpected(DebugError{DebugErrorCode::Unsupported, "unknown debugger command"});
}

DebugResult<Str> DebugCli::backtrace() {
    const DebugResult<Vec<DebugStackFrame>> frames = runtime_.stackTrace(ThreadId{1}, 0, 100);
    if (!frames) {
        return std::unexpected(frames.error());
    }

    std::ostringstream output;
    for (usize index = 0; index < frames->size(); ++index) {
        const DebugStackFrame& frame = frames->at(index);
        output << '#' << index << ' ' << frame.name << " source=" << frame.location.sourceId.value()
               << " line=" << frame.location.line << (frame.native ? " [native]" : "") << '\n';
    }
    return output.str();
}

DebugResult<Str> DebugCli::locals() {
    const DebugResult<Vec<DebugStackFrame>> frames = runtime_.stackTrace(ThreadId{1}, 0, 1);
    if (!frames || frames->empty()) {
        return frames ? DebugResult<Str>{std::unexpected(DebugError{DebugErrorCode::InvalidReference, "no frame"})}
                      : DebugResult<Str>{std::unexpected(frames.error())};
    }
    const DebugResult<Vec<DebugScope>> frameScopes = runtime_.scopes(frames->front().id);
    if (!frameScopes) {
        return std::unexpected(frameScopes.error());
    }
    for (const DebugScope& scope : *frameScopes) {
        if (scope.kind != DebugScopeKind::Locals) {
            continue;
        }
        const DebugResult<Vec<DebugVariable>> frameLocals = runtime_.variables(scope.variablesReference, 0, 100);
        if (!frameLocals) {
            return std::unexpected(frameLocals.error());
        }
        std::ostringstream output;
        for (const DebugVariable& variable : *frameLocals) {
            output << variable.name << " = " << variable.value << " : " << variable.type << '\n';
        }
        return output.str();
    }
    return Str{};
}

} // namespace Lua::Debugger
