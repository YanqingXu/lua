#include "timeout_memory_detector.hpp"

namespace Lua {
    // thread_local静态成员变量定义
    thread_local i32 RecursionDetector::recursionDepth_ = 0;
}