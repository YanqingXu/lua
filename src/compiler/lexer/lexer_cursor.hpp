#pragma once

/**
 * @file lexer_cursor.hpp
 * @brief Lua 词法分析器使用的输入游标
 */

#include "common/types.hpp"
#include "io/input_stream.hpp"
#include "runtime/lua_allocator.hpp"

namespace Lua {

/**
 * @brief 词法分析器输入使用的缓冲字符游标
 *
 * 负责词法分析层的字符缓冲、前瞻、重放状态，以及按 Lua 换行规则进行的行列计数。
 */
class InputCursor {
public:
    struct State {
        usize cursor;
        i32 line;
        i32 column;
        i32 pendingNewlineChar;
    };

    explicit InputCursor(IO::InputStream& input, LuaAllocator* allocator = nullptr);
    InputCursor(const InputCursor&) = delete;
    InputCursor& operator=(const InputCursor&) = delete;
    InputCursor(InputCursor&&) = delete;
    InputCursor& operator=(InputCursor&&) = delete;

    char advance();
    char peek(usize offset = 0) const noexcept;
    bool isAtEnd() const noexcept;

    State save() const noexcept;
    void restore(const State& state);

    i32 line() const noexcept {
        return line_;
    }
    i32 column() const noexcept {
        return column_;
    }
    usize offset() const noexcept {
        return cursor_;
    }

private:
    void ensureBuffered(usize absoluteIndex);
    void ensureLookahead();
    static bool isNewline(char c) noexcept;

private:
    IO::InputStream* input_;
    LuaAllocator allocator_;
    LuaVector<i32> buffer_;
    usize cursor_;
    bool reachedEof_;
    i32 line_;
    i32 column_;
    i32 pendingNewlineChar_;
};

} // namespace Lua
