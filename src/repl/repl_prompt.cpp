#include "repl/repl_prompt.hpp"

#include "core/gc_string.hpp"

#include <format>

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
        // Fall back to the default prompt if user-provided globals misbehave.
    }

    return defaultPrompt;
}

bool isDefaultPrompt(const Str& prompt, bool firstLine) {
    return prompt == (firstLine ? DEFAULT_PROMPT1 : DEFAULT_PROMPT2);
}

}  // namespace

Str formatLinePrompt(usize lineNumber, bool firstLine) {
    const usize visibleLine = lineNumber == 0 ? 1 : lineNumber;
    return std::format("lua:{}{} ", visibleLine, firstLine ? ">" : ">>");
}

Str getPrompt(LuaState* L, bool firstLine, usize lineNumber) {
    const Str configuredPrompt = getConfiguredPrompt(L, firstLine);
    if (isDefaultPrompt(configuredPrompt, firstLine)) {
        return formatLinePrompt(lineNumber, firstLine);
    }

    return configuredPrompt;
}

}  // namespace Lua::REPL::detail
