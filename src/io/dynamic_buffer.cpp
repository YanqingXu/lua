/**
 * @file dynamic_buffer.cpp
 * @brief 动态缓冲区实现
 */

#include "common/types.hpp"
#include "dynamic_buffer.hpp"
#include <algorithm>
#include <cstring>

namespace Lua {
namespace IO {

void DynamicBuffer::append(char c) {
    buffer_.push_back(c);
}

void DynamicBuffer::append(StrView str) {
    buffer_.insert(buffer_.end(), str.begin(), str.end());
}

StrView DynamicBuffer::view() const noexcept {
    return StrView(buffer_.data(), buffer_.size());
}

Str DynamicBuffer::toString() && {
    return Str(buffer_.begin(), buffer_.end());
}

void DynamicBuffer::clear() noexcept {
    buffer_.clear();
}

void DynamicBuffer::reset() noexcept {
    buffer_.clear();
    buffer_.shrink_to_fit();
}

void DynamicBuffer::reserve(usize capacity) {
    buffer_.reserve(capacity);
}

usize DynamicBuffer::size() const noexcept {
    return buffer_.size();
}

usize DynamicBuffer::capacity() const noexcept {
    return buffer_.capacity();
}

bool DynamicBuffer::empty() const noexcept {
    return buffer_.empty();
}

CharPtr DynamicBuffer::data() const noexcept {
    return buffer_.data();
}

} // namespace IO
} // namespace Lua
