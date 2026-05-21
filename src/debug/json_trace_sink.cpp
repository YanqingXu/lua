/**
 * @file json_trace_sink.cpp
 * @brief JSONL Trace Sink 实现
 */

#include "debug/json_trace_sink.hpp"
#include "debug/value_serializer.hpp"
#include "compiler/opcode.hpp"
#include <format>
#include <iostream>

namespace Lua {

// =========================================================================
// 构造 / 析构
// =========================================================================

JsonTraceSink::JsonTraceSink(const Str& filePath, u64 maxEvents)
    : maxEvents_(maxEvents)
{
    file_.open(filePath, std::ios::out | std::ios::trunc);
    if (!file_.is_open()) {
        std::cerr << "[TRACE] Warning: cannot open trace file: " << filePath << std::endl;
    }
}

JsonTraceSink::~JsonTraceSink() {
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
}

// =========================================================================
// 事件处理
// =========================================================================

void JsonTraceSink::onInstruction(const TraceEvent& evt) {
    if (!file_.is_open() || reachedLimit()) return;

    Str registers;
    if (evt.base && evt.maxStack > 0) {
        registers = std::format(
            ",\"registers\":{}",
            Trace::serializeRegisters(evt.base, evt.maxStack, evt.proto, evt.pc)
        );
    }

    file_ << std::format(
        "{{\"seq\":{},\"kind\":\"instruction\",\"pc\":{},\"op\":\"{}\","
        "\"a\":{},\"b\":{},\"c\":{},\"bx\":{},\"sbx\":{},\"line\":{},"
        "\"source\":\"{}\",\"callDepth\":{}{}}}\n",
        evt.seq,
        evt.pc,
        getOpName(evt.op),
        evt.a,
        evt.b,
        evt.c,
        evt.bx,
        evt.sbx,
        evt.line,
        Trace::jsonEscape(evt.source ? evt.source : "?"),
        evt.callDepth,
        registers
    );
    ++eventCount_;
}

void JsonTraceSink::onCall(const TraceEvent& evt) {
    if (!file_.is_open() || reachedLimit()) return;

    file_ << std::format(
        "{{\"seq\":{},\"kind\":\"call\",\"funcName\":\"{}\","
        "\"source\":\"{}\",\"line\":{},\"callDepth\":{}}}\n",
        evt.seq,
        Trace::jsonEscape(evt.funcName ? evt.funcName : "?"),
        Trace::jsonEscape(evt.source ? evt.source : "?"),
        evt.line,
        evt.callDepth
    );
    ++eventCount_;
}

void JsonTraceSink::onReturn(const TraceEvent& evt) {
    if (!file_.is_open() || reachedLimit()) return;

    file_ << std::format(
        "{{\"seq\":{},\"kind\":\"return\",\"callDepth\":{}}}\n",
        evt.seq,
        evt.callDepth
    );
    ++eventCount_;
}

void JsonTraceSink::onError(const TraceEvent& evt) {
    if (!file_.is_open() || reachedLimit()) return;

    file_ << std::format(
        "{{\"seq\":{},\"kind\":\"error\",\"message\":\"{}\","
        "\"source\":\"{}\",\"line\":{},\"callDepth\":{}}}\n",
        evt.seq,
        Trace::jsonEscape(evt.errorMsg ? evt.errorMsg : ""),
        Trace::jsonEscape(evt.source ? evt.source : "?"),
        evt.line,
        evt.callDepth
    );
    ++eventCount_;
}

void JsonTraceSink::flush() {
    if (file_.is_open()) {
        file_.flush();
    }
}

} // namespace Lua
