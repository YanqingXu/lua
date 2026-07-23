#ifndef LUA_REPL_CTX_HPP
#define LUA_REPL_CTX_HPP

/**
 * @file repl_ctx.hpp
 * @brief REPL 会话上下文及错误输出配置
 */

#include "repl.hpp"

#include <iosfwd>
#include <string_view>

namespace Lua::REPL::detail {

/** @brief 保存 REPL 会话的程序名、错误颜色模式与交互状态。 */
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

/** @brief 在作用域内临时启用交互式错误颜色上下文。 */
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

/** @brief 获取进程级 REPL 上下文。 */
ReplContext& globalContext();

/** @brief 按上下文颜色设置写出一行错误消息。 */
void writeErrorLine(ReplContext& context, std::ostream& err, std::string_view message);
/** @brief 报告不带源码位置的错误。 */
void reportError(ReplContext& context, std::ostream& err, std::string_view msg, bool showProgName);
/** @brief 报告带源码位置的错误。 */
void reportError(ReplContext& context, std::ostream& err, std::string_view source, int line, std::string_view msg,
                 bool showProgName);

} // namespace Lua::REPL::detail

#endif // LUA_REPL_CTX_HPP
