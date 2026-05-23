#ifndef LUA_REPL_SIGNALS_HPP
#define LUA_REPL_SIGNALS_HPP

#include "common/types.hpp"

#include <cstdio>
#include <iosfwd>

namespace Lua {

class LuaState;

namespace REPL::detail {

enum class ConsoleReadStatus {
    NotHandled,
    LineRead,
    Eof,
};

using CompletionHandler = void (*)(LuaState* L, const Str& prompt, Str& line, std::ostream& out);

class SignalController {
public:
    SignalController();
    ~SignalController();

    SignalController(const SignalController&) = delete;
    SignalController& operator=(const SignalController&) = delete;

    bool wasInterrupted() const;
    void clearInterrupt();
};

bool isTerminal(FILE* stream);
bool enableVirtualTerminalFor(FILE* stream);
bool isInputTerminal();

ConsoleReadStatus readInteractiveConsoleLine(LuaState* L, const Str& prompt, Str& line,
                                             std::ostream& out,
                                             CompletionHandler completionHandler);

}  // namespace REPL::detail
}  // namespace Lua

#endif  // LUA_REPL_SIGNALS_HPP
