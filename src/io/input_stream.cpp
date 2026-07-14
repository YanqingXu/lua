/**
 * @file input_stream.cpp
 * @brief 输入流抽象实现
 */

#include "input_stream.hpp"
#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace Lua {
namespace IO {

// =====================================================================
// 构造函数
// =====================================================================

InputStream::InputStream(StrView source)
    : stream_(nullptr), stringView_(source), bufferPos_(0), bufferSize_(0), eof_(source.empty()), position_(0),
      useStringView_(true), sourceName_("string") {}

InputStream::InputStream(std::istream& stream, usize bufferSize)
    : stream_(&stream), buffer_(bufferSize), bufferPos_(0), bufferSize_(0), eof_(false), position_(0),
      useStringView_(false), sourceName_("stream") {
    fillBuffer(); // 预填充缓冲区
    // 如果流是空的（构造时就没有数据），设置 EOF
    if (bufferSize_ == 0) {
        eof_ = true;
    }
}

InputStream::InputStream(FromFile /* tag */, const Str& filePath, usize bufferSize)
    : stream_(nullptr), ownedFileStream_(std::ifstream(filePath, std::ios::binary)), buffer_(bufferSize), bufferPos_(0),
      bufferSize_(0), eof_(false), position_(0), useStringView_(false), sourceName_(filePath) {
    // 检查文件是否成功打开
    if (!ownedFileStream_->is_open()) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }

    // 设置 stream_ 指向拥有的文件流
    stream_ = &(*ownedFileStream_);

    // 预填充缓冲区
    fillBuffer();

    // 如果文件是空的，设置 EOF
    if (bufferSize_ == 0) {
        eof_ = true;
    }
}

// =====================================================================
// 字符读取
// =====================================================================

i32 InputStream::getChar() {
    if (useStringView_) {
        // 字符串视图模式（零拷贝）
        if (position_ >= stringView_.size()) {
            eof_ = true;
            return -1;
        }
        i32 ch = static_cast<u8>(stringView_[position_++]);
        // 检查是否读取完所有数据
        if (position_ >= stringView_.size()) {
            eof_ = true;
        }
        return ch;
    }

    // 流模式
    if (bufferPos_ >= bufferSize_) {
        fillBuffer();
        // 如果 fillBuffer() 后仍然没有数据，说明到达 EOF
        if (bufferSize_ == 0) {
            eof_ = true;
            return -1;
        }
    }

    position_++;
    return static_cast<u8>(buffer_[bufferPos_++]);
}

i32 InputStream::peekChar() {
    if (useStringView_) {
        // 字符串视图模式
        if (position_ >= stringView_.size()) {
            eof_ = true;
            return -1;
        }
        return static_cast<u8>(stringView_[position_]);
    }

    // 流模式
    if (bufferPos_ >= bufferSize_) {
        fillBuffer();
        // 如果 fillBuffer() 后仍然没有数据，说明到达 EOF
        if (bufferSize_ == 0) {
            eof_ = true;
            return -1;
        }
    }

    return static_cast<u8>(buffer_[bufferPos_]);
}

// =====================================================================
// 批量读取
// =====================================================================

usize InputStream::read(void* buffer, usize size) {
    if (useStringView_) {
        // 字符串视图模式
        usize available = stringView_.size() - position_;
        usize toRead = std::min(size, available);
        std::memcpy(buffer, stringView_.data() + position_, toRead);
        position_ += toRead;
        if (position_ >= stringView_.size()) {
            eof_ = true;
        }
        return toRead;
    }

    // 流模式的批量读取
    usize totalRead = 0;
    char* dest = static_cast<char*>(buffer);

    while (totalRead < size) {
        if (bufferPos_ >= bufferSize_) {
            fillBuffer();
            // 如果 fillBuffer() 后没有数据，说明流已经结束
            if (bufferSize_ == 0) {
                // 流已经结束，设置 EOF 标志
                eof_ = true;
                break;
            }
        }

        usize available = bufferSize_ - bufferPos_;
        usize toRead = std::min(size - totalRead, available);

        std::memcpy(dest + totalRead, buffer_.data() + bufferPos_, toRead);
        bufferPos_ += toRead;
        totalRead += toRead;
        position_ += toRead;
    }

    return totalRead;
}

// =====================================================================
// 状态查询
// =====================================================================

bool InputStream::isEof() const noexcept {
    return eof_;
}

usize InputStream::getPosition() const noexcept {
    return position_;
}

const Str& InputStream::getSourceName() const noexcept {
    return sourceName_;
}

void InputStream::setSourceName(const Str& name) {
    sourceName_ = name;
}

// =====================================================================
// 内部辅助函数
// =====================================================================

void InputStream::fillBuffer() {
    if (stream_ == nullptr) {
        bufferSize_ = 0;
        return;
    }

    if (buffer_.size() > static_cast<usize>(std::numeric_limits<std::streamsize>::max())) {
        throw std::length_error("Input stream buffer is too large");
    }

    const std::streamsize readSize = static_cast<std::streamsize>(buffer_.size());
    stream_->read(buffer_.data(), readSize);
    bufferSize_ = static_cast<usize>(stream_->gcount());
    bufferPos_ = 0;

    // 检查流错误（不包括 EOF）
    if (stream_->bad()) {
        throw std::runtime_error("I/O error while reading from stream: " + sourceName_);
    }

    // 注意：不在这里设置 eof_，而是在调用者中根据 bufferSize_ 来判断
    // 这样可以让调用者决定何时设置 EOF 标志
}

} // namespace IO
} // namespace Lua
