#pragma once

/**
 * @file lexer_cursor.hpp
 * @brief Input cursor used by the Lua lexer
 */

#include "common/types.hpp"
#include "io/input_stream.hpp"

namespace Lua {

/**
 * @brief Buffered character cursor for lexer input
 *
 * Owns lexer-level character buffering, lookahead, replay state, and line/column
 * accounting under Lua newline rules.
 */
class InputCursor {
public:
    struct State {
        usize cursor;
        i32 line;
        i32 column;
        i32 pendingNewlineChar;
    };

    explicit InputCursor(IO::InputStream& input);

    char advance();
    char peek(usize offset = 0) const noexcept;
    bool isAtEnd() const noexcept;

    State save() const noexcept;
    void restore(const State& state);

    i32 line() const noexcept { return line_; }
    i32 column() const noexcept { return column_; }
    usize offset() const noexcept { return cursor_; }

private:
    void ensureBuffered(usize absoluteIndex);
    void ensureLookahead();
    static bool isNewline(char c) noexcept;

private:
    IO::InputStream* input_;
    Vec<i32> buffer_;
    usize cursor_;
    bool reachedEof_;
    i32 line_;
    i32 column_;
    i32 pendingNewlineChar_;
};

} // namespace Lua
