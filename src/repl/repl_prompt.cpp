/**
 * @file repl_prompt.cpp
 * @brief REPL 可配置提示符的生成实现
 */

#include "repl/repl_prompt.hpp"

#include "core/gc_string.hpp"

namespace Lua::REPL::detail {
namespace {

Str getConfiguredPrompt(LuaState* L, bool firstLine) {
    static Str cachedPrompt1;
    static Str cachedPrompt2;

    const char* varName = firstLine ? "_PROMPT" : "_PROMPT2";
    const char* defaultPrompt = firstLine ? DEFAULT_PROMPT1 : DEFAULT_PROMPT2;
    Str& cachedPrompt = firstLine ? cachedPrompt1 : cachedPrompt2;

    try {
        if (L != nullptr) {
            Value val = L->getGlobal(varName);
            if (val.isString()) {
                cachedPrompt = val.asString()->c_str();
                return cachedPrompt;
            }
        }
    } catch (...) {
        /** @brief 用户提供的全局变量行为异常时回退到默认提示符。 */
    }

    return defaultPrompt;
}

}  // namespace

Str getPrompt(LuaState* L, bool firstLine, usize lineNumber) {
    (void)lineNumber;
    return getConfiguredPrompt(L, firstLine);
}

}  // namespace Lua::REPL::detail
