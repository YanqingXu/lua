#include "repl/completion.hpp"

#include "core/gc_string.hpp"
#include "core/string_pool.hpp"
#include "core/table.hpp"
#include "repl/text.hpp"
#include "vm/state/global_state.hpp"

#include <algorithm>
#include <cctype>
#include <ostream>
#include <string_view>

namespace Lua::REPL {
namespace detail {

bool isIdentifierChar(char ch) {
    return std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '_';
}

bool isCompletionTokenChar(char ch) {
    return isIdentifierChar(ch) || ch == '.';
}

void sortUnique(Vec<Str>& values) {
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
}

Str commonPrefix(const Vec<Str>& values) {
    if (values.empty()) {
        return "";
    }

    Str prefix = values.front();
    for (usize i = 1; i < values.size(); ++i) {
        const Str& value = values[i];
        usize len = 0;
        while (len < prefix.size() && len < value.size() && prefix[len] == value[len]) {
            len++;
        }
        prefix.resize(len);
        if (prefix.empty()) {
            break;
        }
    }
    return prefix;
}

CompletionResult buildCompletionResult(const Str& line, usize tokenStart, const Str& token,
                                       Vec<Str> candidates) {
    sortUnique(candidates);

    CompletionResult result;
    result.completedLine = line;
    result.candidates = std::move(candidates);

    if (result.candidates.empty()) {
        return result;
    }

    const Str replacement = result.candidates.size() == 1
                                ? result.candidates.front()
                                : commonPrefix(result.candidates);
    if (replacement.size() > token.size()) {
        result.completedLine = line.substr(0, tokenStart) + replacement;
    }

    return result;
}

void collectStringKeys(Table* table, std::string_view prefix, std::string_view candidatePrefix,
                       Vec<Str>& candidates) {
    if (table == nullptr) {
        return;
    }

    Value key;
    Value nextKey;
    Value nextValue;
    while (table->next(key, nextKey, nextValue)) {
        if (nextKey.isString()) {
            const Str name = nextKey.asString()->c_str();
            if (startsWith(name, prefix)) {
                candidates.push_back(Str(candidatePrefix) + name);
            }
        }
        key = nextKey;
    }
}

Vec<Str> splitDottedPath(std::string_view path) {
    Vec<Str> parts;
    usize start = 0;
    while (start <= path.size()) {
        const usize dot = path.find('.', start);
        const usize end = dot == std::string_view::npos ? path.size() : dot;
        if (end == start) {
            return {};
        }
        parts.emplace_back(path.substr(start, end - start));
        if (dot == std::string_view::npos) {
            break;
        }
        start = dot + 1;
    }
    return parts;
}

Table* resolveTablePath(LuaState* L, std::string_view path) {
    if (L == nullptr || path.empty()) {
        return nullptr;
    }

    Vec<Str> parts = splitDottedPath(path);
    if (parts.empty()) {
        return nullptr;
    }

    StringPool& pool = L->getGlobalState().getStringPool();
    Value current = L->getGlobal(parts.front());
    for (usize i = 1; i < parts.size(); ++i) {
        if (!current.isTable()) {
            return nullptr;
        }
        GCString* key = pool.intern(parts[i]);
        current = current.asTable()->get(Value(key));
    }

    return current.isTable() ? current.asTable() : nullptr;
}

usize findCompletionTokenStart(const Str& line) {
    usize start = line.size();
    while (start > 0 && isCompletionTokenChar(line[start - 1])) {
        start--;
    }
    return start;
}

Vec<Str> completeMetaCommandToken(std::string_view token) {
    static constexpr std::string_view kCommands[] = {
        ".ast",
        ".bytecode",
        ".gc",
        ".help",
    };

    Vec<Str> candidates;
    for (std::string_view command : kCommands) {
        if (startsWith(command, token)) {
            candidates.emplace_back(command);
        }
    }
    return candidates;
}

Vec<Str> completeGcOption(std::string_view token) {
    static constexpr std::string_view kOptions[] = {
        "collect",
        "help",
        "stats",
        "status",
        "strategy",
    };

    Vec<Str> candidates;
    for (std::string_view option : kOptions) {
        if (startsWith(option, token)) {
            candidates.emplace_back(option);
        }
    }
    return candidates;
}

CompletionResult completeMetaInput(const Str& line, usize commandStart) {
    usize commandEnd = commandStart;
    while (commandEnd < line.size() && !isSpace(line[commandEnd])) {
        commandEnd++;
    }

    if (commandEnd == line.size()) {
        const Str token = line.substr(commandStart);
        return buildCompletionResult(line, commandStart, token, completeMetaCommandToken(token));
    }

    const Str command = line.substr(commandStart, commandEnd - commandStart);
    if (command != ".gc") {
        return {line, {}};
    }

    usize optionStart = line.size();
    while (optionStart > commandEnd && !isSpace(line[optionStart - 1])) {
        optionStart--;
    }

    const Str token = line.substr(optionStart);
    return buildCompletionResult(line, optionStart, token, completeGcOption(token));
}

CompletionResult completeLuaInput(LuaState* L, const Str& line) {
    if (L == nullptr) {
        return {line, {}};
    }

    const usize tokenStart = findCompletionTokenStart(line);
    const Str token = line.substr(tokenStart);
    if (token.empty()) {
        return {line, {}};
    }

    Vec<Str> candidates;
    const usize dot = token.rfind('.');
    if (dot != Str::npos) {
        const Str tablePath = token.substr(0, dot);
        const Str fieldPrefix = token.substr(dot + 1);
        Table* table = resolveTablePath(L, tablePath);
        collectStringKeys(table, fieldPrefix, tablePath + ".", candidates);
        return buildCompletionResult(line, tokenStart, token, std::move(candidates));
    }

    collectStringKeys(L->getGlobalTable(), token, "", candidates);
    return buildCompletionResult(line, tokenStart, token, std::move(candidates));
}

void printCompletionCandidates(const Vec<Str>& candidates, std::ostream& out) {
    if (candidates.empty()) {
        return;
    }

    out << '\n';
    for (usize i = 0; i < candidates.size(); ++i) {
        if (i != 0) {
            out << "  ";
        }
        out << candidates[i];
    }
    out << '\n';
}

void redrawInputLine(const Str& prompt, const Str& line, std::ostream& out) {
    out << prompt << line << std::flush;
}

void applyInteractiveCompletion(LuaState* L, const Str& prompt, Str& line, std::ostream& out) {
    const CompletionResult completion = completeInput(L, line);
    if (completion.candidates.empty()) {
        out << '\a' << std::flush;
        return;
    }

    const Str oldLine = line;
    line = completion.completedLine;

    if (completion.candidates.size() > 1) {
        printCompletionCandidates(completion.candidates, out);
        redrawInputLine(prompt, line, out);
        return;
    }

    if (startsWith(line, oldLine)) {
        out << line.substr(oldLine.size()) << std::flush;
        return;
    }

    out << '\n';
    redrawInputLine(prompt, line, out);
}

void applySubmittedTabCompletion(LuaState* L, Str& line, std::ostream& out) {
    usize tab = line.find('\t');
    while (tab != Str::npos) {
        const Str beforeTab = line.substr(0, tab);
        const Str afterTab = line.substr(tab + 1);
        const CompletionResult completion = completeInput(L, beforeTab);
        line = completion.completedLine + afterTab;
        if (completion.candidates.size() > 1) {
            printCompletionCandidates(completion.candidates, out);
        }
        tab = line.find('\t');
    }
}

}  // namespace detail

CompletionResult completeInput(LuaState* L, const Str& line) {
    usize first = 0;
    while (first < line.size() && detail::isSpace(line[first])) {
        first++;
    }

    if (first < line.size() && line[first] == '.') {
        return detail::completeMetaInput(line, first);
    }

    return detail::completeLuaInput(L, line);
}

}  // namespace Lua::REPL
