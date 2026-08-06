#pragma once

/**
 * @file runtime_output.hpp
 * @brief Optional host-owned output channel for embedded/runtime frontends.
 */

#include "common/types.hpp"

namespace Lua {

class RuntimeOutputSink {
public:
    virtual ~RuntimeOutputSink() = default;
    virtual void writeRuntimeOutput(StrView text, bool standardError) = 0;
};

} // namespace Lua
