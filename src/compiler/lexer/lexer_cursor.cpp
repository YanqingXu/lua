/**
 * @file lexer_cursor.cpp
 * @brief Lexer input cursor implementation
 */

#include "lexer_cursor.hpp"

namespace Lua {

// =====================================================================
// InputCursor
// =====================================================================

InputCursor::InputCursor(IO::InputStream& input, LuaAllocator* allocator)
    : input_(&input), allocator_(allocator != nullptr ? *allocator : LuaAllocator{}),
      buffer_(LuaStdAllocator<i32>(&allocator_)), cursor_(0), reachedEof_(false), line_(1), column_(1),
      pendingNewlineChar_(-1) {
    ensureLookahead();
}

char InputCursor::advance() {
    ensureBuffered(cursor_);
    i32 ch = buffer_[cursor_];
    if (ch == -1)
        return '\0';

    cursor_++;

    // Lua 5.1 将 CRLF/LFCR 视为一个换行。
    if (isNewline(static_cast<char>(ch))) {
        if (pendingNewlineChar_ != -1 && ch != pendingNewlineChar_) {
            pendingNewlineChar_ = -1;
        } else {
            line_++;
            column_ = 1;
            pendingNewlineChar_ = ch;
        }
    } else {
        column_++;
        pendingNewlineChar_ = -1;
    }

    ensureLookahead();

    return static_cast<char>(ch);
}

char InputCursor::peek(usize offset) const noexcept {
    usize index = cursor_ + offset;
    if (index >= buffer_.size() || buffer_[index] == -1) {
        return '\0';
    }
    return static_cast<char>(buffer_[index]);
}

bool InputCursor::isAtEnd() const noexcept {
    return cursor_ < buffer_.size() && buffer_[cursor_] == -1;
}

InputCursor::State InputCursor::save() const noexcept {
    return State{cursor_, line_, column_, pendingNewlineChar_};
}

void InputCursor::restore(const State& state) {
    cursor_ = state.cursor;
    line_ = state.line;
    column_ = state.column;
    pendingNewlineChar_ = state.pendingNewlineChar;
    ensureLookahead();
}

void InputCursor::ensureBuffered(usize absoluteIndex) {
    while (buffer_.size() <= absoluteIndex && !reachedEof_) {
        i32 ch = input_->getChar();
        buffer_.push_back(ch);
        if (ch == -1) {
            reachedEof_ = true;
        }
    }
}

void InputCursor::ensureLookahead() {
    ensureBuffered(cursor_ + 1);
}

bool InputCursor::isNewline(char c) noexcept {
    return c == '\n' || c == '\r';
}

} // namespace Lua
