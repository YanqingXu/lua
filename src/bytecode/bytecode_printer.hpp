#pragma once

#include <iosfwd>

namespace Lua {

class Proto;

// 打印 Lua C++ 实现的 Proto 字节码，格式尽量对齐 Lua 5.1 的 luac -l 输出。
void printProtoBytecode(const Proto* f, std::ostream& out, bool full);

} // namespace Lua
