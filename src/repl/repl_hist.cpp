/**
 * @file repl_hist.cpp
 * @brief REPL 历史记录的维护与持久化实现
 */

#include "repl/repl_hist.hpp"

#include <fstream>

namespace Lua::REPL {

void recordHistory(Vec<Str>& history, const Str& line) {
    if (!line.empty()) {
        history.push_back(line);
    }
}

bool loadHistory(const Str& path, Vec<Str>& history) {
    history.clear();

    std::ifstream input(path);
    if (!input) {
        return false;
    }

    Str line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        recordHistory(history, line);
    }

    return input.eof() || input.good();
}

bool saveHistory(const Str& path, const Vec<Str>& history) {
    std::ofstream output(path, std::ios::trunc);
    if (!output) {
        return false;
    }

    for (const Str& line : history) {
        output << line << '\n';
    }

    return output.good();
}

}  // namespace Lua::REPL
