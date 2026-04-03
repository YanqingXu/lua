/**
 * @file json_trace_sink.cpp
 * @brief JSONL Trace Sink 实现
 */

#include "debug/json_trace_sink.hpp"
#include "debug/value_serializer.hpp"
#include "compiler/opcode.hpp"
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

    file_ << "{\"seq\":" << evt.seq
          << ",\"kind\":\"instruction\""
          << ",\"pc\":" << evt.pc
          << ",\"op\":\"" << getOpName(evt.op) << "\""
          << ",\"a\":" << evt.a
          << ",\"b\":" << evt.b
          << ",\"c\":" << evt.c
          << ",\"bx\":" << evt.bx
          << ",\"sbx\":" << evt.sbx
          << ",\"line\":" << evt.line
          << ",\"source\":\"" << Trace::jsonEscape(evt.source ? evt.source : "?") << "\""
          << ",\"callDepth\":" << evt.callDepth;

    // 寄存器快照
    if (evt.base && evt.maxStack > 0) {
        file_ << ",\"registers\":"
              << Trace::serializeRegisters(evt.base, evt.maxStack, evt.proto, evt.pc);
    }

    file_ << "}\n";
    ++eventCount_;
}

void JsonTraceSink::onCall(const TraceEvent& evt) {
    if (!file_.is_open() || reachedLimit()) return;

    file_ << "{\"seq\":" << evt.seq
          << ",\"kind\":\"call\""
          << ",\"funcName\":\"" << Trace::jsonEscape(evt.funcName ? evt.funcName : "?") << "\""
          << ",\"source\":\"" << Trace::jsonEscape(evt.source ? evt.source : "?") << "\""
          << ",\"line\":" << evt.line
          << ",\"callDepth\":" << evt.callDepth
          << "}\n";
    ++eventCount_;
}

void JsonTraceSink::onReturn(const TraceEvent& evt) {
    if (!file_.is_open() || reachedLimit()) return;

    file_ << "{\"seq\":" << evt.seq
          << ",\"kind\":\"return\""
          << ",\"callDepth\":" << evt.callDepth
          << "}\n";
    ++eventCount_;
}

void JsonTraceSink::onError(const TraceEvent& evt) {
    if (!file_.is_open() || reachedLimit()) return;

    file_ << "{\"seq\":" << evt.seq
          << ",\"kind\":\"error\""
          << ",\"message\":\"" << Trace::jsonEscape(evt.errorMsg ? evt.errorMsg : "") << "\""
          << ",\"source\":\"" << Trace::jsonEscape(evt.source ? evt.source : "?") << "\""
          << ",\"line\":" << evt.line
          << ",\"callDepth\":" << evt.callDepth
          << "}\n";
    ++eventCount_;
}

void JsonTraceSink::flush() {
    if (file_.is_open()) {
        file_.flush();
    }
}

} // namespace Lua
