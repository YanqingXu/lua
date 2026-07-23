/**
 * @file json_trace_sink.cpp
 * @brief JSONL 追踪输出端实现
 */

#include "debug/json_trace_sink.hpp"
#include "debug/value_serializer.hpp"
#include "compiler/opcode.hpp"
#include <format>
#include <iostream>

namespace Lua {

namespace {

Str serializeChangedRegisters(const Vec<TraceRegisterChange>& changes) {
    Str out = "[";

    for (usize i = 0; i < changes.size(); ++i) {
        if (i > 0) {
            out += ",";
        }

        const TraceRegisterChange& change = changes[i];
        out += "{\"slot\":";
        out += std::to_string(change.slot);
        out += ",\"name\":";
        if (change.hasName) {
            out += "\"" + Trace::jsonEscape(change.name) + "\"";
        } else {
            out += "null";
        }
        out += ",\"old\":";
        out += change.oldValue.empty() ? "null" : change.oldValue;
        out += ",\"new\":";
        out += change.newValue.empty() ? "null" : change.newValue;
        out += ",\"oldType\":\"";
        out += Trace::jsonEscape(change.oldType ? change.oldType : "unknown");
        out += "\",\"newType\":\"";
        out += Trace::jsonEscape(change.newType ? change.newType : "unknown");
        out += "\"}";
    }

    out += "]";
    return out;
}

} // namespace

// =========================================================================
// 构造 / 析构
// =========================================================================

JsonTraceSink::JsonTraceSink(const Str& filePath, u64 maxEvents) : maxEvents_(maxEvents) {
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
    if (!file_.is_open() || reachedLimit())
        return;

    Str registers;
    if (evt.base && evt.maxStack > 0) {
        registers =
            std::format(",\"registers\":{}", Trace::serializeRegisters(evt.base, evt.maxStack, evt.proto, evt.pc));
    }

    Str changedRegisters;
    if (evt.includeChangedRegisters) {
        changedRegisters = std::format(",\"changedRegisters\":{}", serializeChangedRegisters(evt.changedRegisters));
    }

    file_ << std::format("{{\"seq\":{},\"kind\":\"instruction\",\"funcName\":\"{}\",\"pc\":{},\"op\":\"{}\","
                         "\"a\":{},\"b\":{},\"c\":{},\"bx\":{},\"sbx\":{},\"line\":{},"
                         "\"source\":\"{}\",\"callDepth\":{}{}{}}}\n",
                         evt.seq, Trace::jsonEscape(evt.funcName), evt.pc, getOpName(evt.op), evt.a, evt.b, evt.c,
                         evt.bx, evt.sbx, evt.line, Trace::jsonEscape(evt.source ? evt.source : "?"), evt.callDepth,
                         registers, changedRegisters);
    ++eventCount_;
}

void JsonTraceSink::onCall(const TraceEvent& evt) {
    if (!file_.is_open() || reachedLimit())
        return;

    file_ << std::format("{{\"seq\":{},\"kind\":\"call\",\"funcName\":\"{}\","
                         "\"source\":\"{}\",\"line\":{},\"callDepth\":{}}}\n",
                         evt.seq, Trace::jsonEscape(evt.funcName), Trace::jsonEscape(evt.source ? evt.source : "?"),
                         evt.line, evt.callDepth);
    ++eventCount_;
}

void JsonTraceSink::onReturn(const TraceEvent& evt) {
    if (!file_.is_open() || reachedLimit())
        return;

    file_ << std::format("{{\"seq\":{},\"kind\":\"return\",\"funcName\":\"{}\","
                         "\"source\":\"{}\",\"line\":{},\"callDepth\":{}}}\n",
                         evt.seq, Trace::jsonEscape(evt.funcName), Trace::jsonEscape(evt.source ? evt.source : "?"),
                         evt.line, evt.callDepth);
    ++eventCount_;
}

void JsonTraceSink::onError(const TraceEvent& evt) {
    if (!file_.is_open() || reachedLimit())
        return;

    file_ << std::format("{{\"seq\":{},\"kind\":\"error\",\"funcName\":\"{}\",\"message\":\"{}\","
                         "\"source\":\"{}\",\"line\":{},\"callDepth\":{}}}\n",
                         evt.seq, Trace::jsonEscape(evt.funcName), Trace::jsonEscape(evt.errorMsg ? evt.errorMsg : ""),
                         Trace::jsonEscape(evt.source ? evt.source : "?"), evt.line, evt.callDepth);
    ++eventCount_;
}

void JsonTraceSink::flush() {
    if (file_.is_open()) {
        file_.flush();
    }
}

} // namespace Lua
