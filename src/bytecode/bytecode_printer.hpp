#pragma once

#include <iosfwd>
#include <string_view>

namespace Lua {

class Proto;

// 打印 Lua C++ 实现的 Proto 字节码，格式尽量对齐 Lua 5.1 的 luac -l 输出。
void printProtoBytecode(const Proto* f, std::ostream& out, bool full);

void printProtoBytecodeDiff(const Proto* left,
                            const Proto* right,
                            std::ostream& out,
                            bool full = false,
                            std::string_view leftLabel = "left",
                            std::string_view rightLabel = "right");

} // namespace Lua
