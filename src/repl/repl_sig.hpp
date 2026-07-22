#ifndef LUA_REPL_SIG_HPP
#define LUA_REPL_SIG_HPP

/**
 * @file repl_sig.hpp
 * @brief REPL 控制台信号与中断读取接口
 */

#include "common/types.hpp"

#include <cstdio>
#include <iosfwd>

namespace Lua {

class LuaState;

namespace REPL::detail {

/** @brief 交互式控制台读取结果。 */
enum class ConsoleReadStatus {
    NotHandled,
    LineRead,
    Eof,
};

/** @brief REPL 输入补全回调签名。 */
using CompletionHandler = void (*)(LuaState* L, const Str& prompt, Str& line, std::ostream& out);

/** @brief 在对象生命周期内安装并管理 REPL 中断信号处理。 */
class SignalController {
public:
    SignalController();
    ~SignalController();

    SignalController(const SignalController&) = delete;
    SignalController& operator=(const SignalController&) = delete;

    /** @brief 检查是否收到中断信号。 */
    bool wasInterrupted() const;
    /** @brief 清除已记录的中断状态。 */
    void clearInterrupt();
};

/** @brief 判断指定文件流是否连接到终端。 */
bool isTerminal(FILE* stream);
/** @brief 为指定终端流启用虚拟终端处理。 */
bool enableVirtualTerminalFor(FILE* stream);
/** @brief 判断标准输入是否连接到终端。 */
bool isInputTerminal();

/**
 * @brief 从交互式控制台读取一行，并按需执行补全
 * @param L Lua 状态指针
 * @param prompt 要显示的提示符
 * @param line 接收输入的字符串
 * @param out 输出流
 * @param completionHandler 补全回调
 * @return 控制台读取状态
 */
ConsoleReadStatus readInteractiveConsoleLine(LuaState* L, const Str& prompt, Str& line,
                                             std::ostream& out,
                                             CompletionHandler completionHandler);

}  // namespace REPL::detail
}  // namespace Lua

#endif  // LUA_REPL_SIG_HPP
