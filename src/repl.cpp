/**
 * @file repl.cpp
 * @brief Public REPL facade and session loop.
 */

#include "repl.hpp"

#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/string_pool.hpp"
#include "repl/completion.hpp"
#include "repl/context.hpp"
#include "repl/executor.hpp"
#include "repl/history.hpp"
#include "repl/meta_commands.hpp"
#include "repl/signals.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/state/global_state.hpp"

#include <cstdlib>
#include <iostream>
#include <utility>

namespace Lua::REPL {
namespace {

Str getPrompt(LuaState* L, bool firstLine) {
    static Str cachedPrompt1;
    static Str cachedPrompt2;

    const char* varName = firstLine ? "_PROMPT" : "_PROMPT2";
    const char* defaultPrompt = firstLine ? DEFAULT_PROMPT1 : DEFAULT_PROMPT2;
    Str& cachedPrompt = firstLine ? cachedPrompt1 : cachedPrompt2;

    try {
        Value val = L->getGlobal(varName);
        if (val.isString()) {
            cachedPrompt = val.asString()->c_str();
            return cachedPrompt;
        }
    } catch (...) {
        // Fall back to the default prompt if user-provided globals misbehave.
    }

    return defaultPrompt;
}

int luaB_exit(LuaState* L) {
    int exitCode = 0;

    if (L->getTop() > 0) {
        Value arg = L->at(-1);
        if (arg.isNumber()) {
            exitCode = static_cast<int>(arg.asNumber());
        } else if (arg.isBoolean()) {
            exitCode = arg.asBoolean() ? 0 : 1;
        }
    }

    std::exit(exitCode);
    return 0;
}

enum class SessionStep {
    Continue,
    Exit,
};

class ReplSession {
public:
    ReplSession(detail::ReplContext& context, LuaState* L, detail::SignalController& signals,
                std::istream& input, std::ostream& output, std::ostream& error)
        : context_(context), L_(L), signals_(signals), input_(input), output_(output),
          error_(error) {}

    int run() {
        printBanner();

        Vec<Str> history;
        loadHistory(DEFAULT_HISTORY_FILE, history);

        while (true) {
            if (consumeLoopInterrupt()) {
                continue;
            }

            const Str prompt = getPrompt(L_, isFirstLine_);
            Str line;
            if (!readLine(prompt, line)) {
                output_ << std::endl;
                break;
            }

            if (consumeReadInterrupt()) {
                continue;
            }

            recordHistory(history, line);
            if (processLine(line) == SessionStep::Exit) {
                break;
            }
        }

        saveHistory(DEFAULT_HISTORY_FILE, history);
        output_ << "Goodbye!" << std::endl;
        return 0;
    }

private:
    void printBanner() {
        output_ << VERSION << "  " << COPYRIGHT << std::endl;
        output_ << "Type '.help' for commands, 'exit' or press Ctrl+D to quit." << std::endl;
    }

    bool consumeLoopInterrupt() {
        if (!signals_.wasInterrupted()) {
            return false;
        }

        signals_.clearInterrupt();
        output_ << std::endl;
        resetInput();
        return true;
    }

    bool consumeReadInterrupt() {
        if (!signals_.wasInterrupted()) {
            return false;
        }

        signals_.clearInterrupt();
        resetInput();
        return true;
    }

    bool readLine(const Str& prompt, Str& line) {
        output_ << prompt << std::flush;

        if (signals_.wasInterrupted()) {
            signals_.clearInterrupt();
            output_ << std::endl;
            line.clear();
            return true;
        }

        const detail::ConsoleReadStatus consoleStatus = detail::readInteractiveConsoleLine(
            L_, prompt, line, output_, detail::applyInteractiveCompletion);
        if (consoleStatus == detail::ConsoleReadStatus::LineRead) {
            return true;
        }
        if (consoleStatus == detail::ConsoleReadStatus::Eof) {
            return false;
        }

        if (!std::getline(input_, line)) {
            if (signals_.wasInterrupted()) {
                signals_.clearInterrupt();
                input_.clear();
                output_ << std::endl;
                line.clear();
                return true;
            }
            return false;
        }

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        detail::applySubmittedTabCompletion(L_, line, output_);
        return true;
    }

    SessionStep processLine(const Str& line) {
        if (isFirstLine_ && (line == "exit" || line == "quit")) {
            return SessionStep::Exit;
        }

        if (isFirstLine_ && line.empty()) {
            return SessionStep::Continue;
        }

        if (isFirstLine_ && handleMetaCommand(line)) {
            return SessionStep::Continue;
        }

        appendInputLine(line);
        prepareAndExecuteBufferedInput();
        return SessionStep::Continue;
    }

    bool handleMetaCommand(const Str& line) {
        const MetaCommand command = parseMetaCommand(line);
        if (command.kind == MetaCommandKind::None) {
            return false;
        }

        detail::runMetaCommand(context_, L_, command, output_, error_);
        resetInput();
        return true;
    }

    void appendInputLine(const Str& line) {
        if (isFirstLine_) {
            bool wasExplicitReturn = false;
            inputBuffer_ = detail::tryAsExpression(line, wasExplicitReturn);
            bufferIsExpression_ = wasExplicitReturn;
            return;
        }

        inputBuffer_ += "\n" + line;
    }

    void prepareAndExecuteBufferedInput() {
        auto prepared =
            detail::prepareInputForExecution(L_, inputBuffer_, bufferIsExpression_);
        if (!prepared) {
            handleParseFailure(prepared.error());
            return;
        }

        detail::executePreparedInput(context_, L_, std::move(*prepared), output_, error_);
        resetInput();
    }

    void handleParseFailure(const ParseError& error) {
        if (detail::isIncompleteInput(error.what())) {
            isFirstLine_ = false;
            return;
        }

        detail::reportError(context_, error_, "stdin", error.getLine(), error.what(), false);
        resetInput();
    }

    void resetInput() {
        inputBuffer_.clear();
        bufferIsExpression_ = false;
        isFirstLine_ = true;
    }

    detail::ReplContext& context_;
    LuaState* L_;
    detail::SignalController& signals_;
    std::istream& input_;
    std::ostream& output_;
    std::ostream& error_;
    Str inputBuffer_;
    bool bufferIsExpression_ = false;
    bool isFirstLine_ = true;
};

}  // namespace

void setProgName(const char* name) {
    detail::globalContext().setProgramName(name);
}

const char* getProgName() {
    return detail::globalContext().programName();
}

void setErrorColorMode(ErrorColorMode mode) {
    detail::globalContext().setErrorColorMode(mode);
}

ErrorColorMode getErrorColorMode() {
    return detail::globalContext().errorColorMode();
}

void reportError(const char* msg, bool showProgName) {
    detail::reportError(detail::globalContext(), std::cerr, msg, showProgName);
}

void reportError(const char* source, int line, const char* msg, bool showProgName) {
    detail::reportError(detail::globalContext(), std::cerr, source, line, msg, showProgName);
}

void initialize(LuaState* L) {
    RuntimeServices services(L->getGlobalState());
    StringPool& pool = services.strings;

    GCString* versionVal = pool.intern(LUA_VERSION);
    L->setGlobal("_VERSION", Value(versionVal));

    GCString* prompt1Val = pool.intern(DEFAULT_PROMPT1);
    L->setGlobal("_PROMPT", Value(prompt1Val));

    GCString* prompt2Val = pool.intern(DEFAULT_PROMPT2);
    L->setGlobal("_PROMPT2", Value(prompt2Val));

    Function* exitFunc = new Function(luaB_exit);
    L->getGlobalState().getGC().registerObject(exitFunc);
    L->setGlobal("exit", Value(exitFunc));
}

int run(LuaState* L) {
    detail::SignalController signals;
    detail::ErrorColorContextGuard colorContext(detail::globalContext());

    ReplSession session(detail::globalContext(), L, signals, std::cin, std::cout, std::cerr);
    return session.run();
}

}  // namespace Lua::REPL
