#ifndef LUA_REPL_CONTEXT_HPP
#define LUA_REPL_CONTEXT_HPP

#include "repl.hpp"

#include <iosfwd>
#include <string_view>

namespace Lua::REPL::detail {

class ReplContext {
public:
    void setProgramName(const char* name);
    const char* programName() const;

    void setErrorColorMode(ErrorColorMode mode);
    ErrorColorMode errorColorMode() const;

    bool isInteractiveErrorContext() const;
    void setInteractiveErrorContext(bool enabled);

private:
    Str programName_ = DEFAULT_PROGNAME;
    ErrorColorMode errorColorMode_ = ErrorColorMode::Auto;
    bool interactiveErrorContext_ = false;
};

class ErrorColorContextGuard {
public:
    explicit ErrorColorContextGuard(ReplContext& context);
    ~ErrorColorContextGuard();

    ErrorColorContextGuard(const ErrorColorContextGuard&) = delete;
    ErrorColorContextGuard& operator=(const ErrorColorContextGuard&) = delete;

private:
    ReplContext& context_;
    bool previous_;
};

ReplContext& globalContext();

void writeErrorLine(ReplContext& context, std::ostream& err, std::string_view message);
void reportError(ReplContext& context, std::ostream& err, std::string_view msg,
                 bool showProgName);
void reportError(ReplContext& context, std::ostream& err, std::string_view source, int line,
                 std::string_view msg, bool showProgName);

}  // namespace Lua::REPL::detail

#endif  // LUA_REPL_CONTEXT_HPP
