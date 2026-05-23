#include "repl/repl_sig.hpp"

#include <cctype>
#include <csignal>
#include <iostream>

#ifdef _WIN32
#include <conio.h>
#include <io.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#else
#include <unistd.h>
#endif

namespace Lua::REPL::detail {
namespace {

volatile sig_atomic_t interrupted = 0;

void signalHandler([[maybe_unused]] int signal) {
    interrupted = 1;
#ifdef _WIN32
    std::signal(SIGINT, signalHandler);
#endif
}

}  // namespace

SignalController::SignalController() {
    std::signal(SIGINT, signalHandler);
}

SignalController::~SignalController() {
    std::signal(SIGINT, SIG_DFL);
}

bool SignalController::wasInterrupted() const {
    return interrupted != 0;
}

void SignalController::clearInterrupt() {
    interrupted = 0;
}

bool isTerminal(FILE* stream) {
#ifdef _WIN32
    return stream != nullptr && _isatty(_fileno(stream)) != 0;
#else
    return stream != nullptr && isatty(fileno(stream)) != 0;
#endif
}

bool enableVirtualTerminalFor(FILE* stream) {
#ifdef _WIN32
    if (stream == nullptr) {
        return false;
    }

    const intptr_t osHandle = _get_osfhandle(_fileno(stream));
    if (osHandle == -1) {
        return false;
    }

    HANDLE handle = reinterpret_cast<HANDLE>(osHandle);
    DWORD mode = 0;
    if (GetConsoleMode(handle, &mode) == 0) {
        return false;
    }

    if ((mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0) {
        return true;
    }

    return SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
#else
    (void)stream;
    return true;
#endif
}

bool isInputTerminal() {
    return isTerminal(stdin);
}

ConsoleReadStatus readInteractiveConsoleLine(LuaState* L, const Str& prompt, Str& line,
                                             std::ostream& out,
                                             CompletionHandler completionHandler) {
#ifdef _WIN32
    if (!isInputTerminal()) {
        return ConsoleReadStatus::NotHandled;
    }

    line.clear();
    while (true) {
        const int ch = _getch();
        if (ch == 0 || ch == 224) {
            (void)_getch();
            continue;
        }
        if (ch == '\r' || ch == '\n') {
            out << std::endl;
            return ConsoleReadStatus::LineRead;
        }
        if (ch == '\t') {
            if (completionHandler != nullptr) {
                completionHandler(L, prompt, line, out);
            }
            continue;
        }
        if (ch == '\b' || ch == 127) {
            if (!line.empty()) {
                line.pop_back();
                out << "\b \b" << std::flush;
            }
            continue;
        }
        if (ch == 4 || ch == 26) {
            return ConsoleReadStatus::Eof;
        }
        if (ch == 3) {
            interrupted = 0;
            out << std::endl;
            line.clear();
            return ConsoleReadStatus::LineRead;
        }
        if (std::isprint(static_cast<unsigned char>(ch)) != 0) {
            line.push_back(static_cast<char>(ch));
            out << static_cast<char>(ch) << std::flush;
        }
    }
#else
    (void)L;
    (void)prompt;
    (void)line;
    (void)out;
    (void)completionHandler;
    return ConsoleReadStatus::NotHandled;
#endif
}

}  // namespace Lua::REPL::detail
