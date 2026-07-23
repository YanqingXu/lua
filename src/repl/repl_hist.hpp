#ifndef LUA_REPL_HIST_HPP
#define LUA_REPL_HIST_HPP

/**
 * @file repl_hist.hpp
 * @brief REPL 历史记录的读写接口
 */

#include "repl.hpp"

namespace Lua::REPL {

/** @brief 将非空输入行记录到历史列表。 */
void recordHistory(Vec<Str>& history, const Str& line);
/** @brief 从文件加载 REPL 历史记录。 */
bool loadHistory(const Str& path, Vec<Str>& history);
/** @brief 将 REPL 历史记录保存到文件。 */
bool saveHistory(const Str& path, const Vec<Str>& history);

} // namespace Lua::REPL

#endif // LUA_REPL_HIST_HPP
