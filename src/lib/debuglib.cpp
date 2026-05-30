/**
 * @file debuglib.cpp
 * @brief Lua debug library implementation
 *
 * Uses the modern C++ fluent registration API.
 *
 * @author Lua C++ Project
 * @date 2026-04-10
 */

#include "lib/debuglib.hpp"

#include "compiler/codegen/codegen.hpp"
#include "compiler/opcode.hpp"
#include "compiler/parser/parser.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/thread.hpp"
#include "core/upvalue.hpp"
#include "core/userdata.hpp"
#include "lib/baselib.hpp"
#include "lib/lib_manager.hpp"
#include "lib/lib_registry.hpp"
#include "vm/state/call_info.hpp"
#include "vm/vm.hpp"

#include <algorithm>
#include <cstring>
#include <format>
#include <iostream>

namespace Lua {

namespace {

// =====================================================================
// Internal Helpers
// =====================================================================

constexpr usize SHORT_SRC_LIMIT = 60;

GCString* internString(LuaState* L, StrView value) {
    return L->getGlobalState().getStringPool().intern(value);
}

GCString* internString(LuaState* L, const char* value) {
    return internString(L, value ? StrView(value) : StrView(""));
}

GCString* internEmptyString(LuaState* L) {
    return internString(L, "");
}

void setField(Table* table, LuaState* L, const char* key, const Value& value) {
    table->set(Value(internString(L, key)), value);
}

void setStringField(Table* table, LuaState* L, const char* key, StrView value) {
    setField(table, L, key, Value(internString(L, value)));
}

void setNumberField(Table* table, LuaState* L, const char* key, i32 value) {
    setField(table, L, key, Value(static_cast<LuaNumber>(value)));
}

Str makeShortSource(StrView source) {
    if (source.empty()) {
        return "[string \"\"]";
    }

    if (source.front() == '=') {
        StrView literal = source.substr(1);
        if (literal.size() <= SHORT_SRC_LIMIT) {
            return Str(literal);
        }
        return Str(literal.substr(0, SHORT_SRC_LIMIT));
    }

    if (source.front() == '@') {
        StrView filename = source.substr(1);
        if (filename.size() <= SHORT_SRC_LIMIT) {
            return Str(filename);
        }
        constexpr usize prefix = 3;
        return "..." + Str(filename.substr(filename.size() - (SHORT_SRC_LIMIT - prefix)));
    }

    constexpr StrView open = "[string \"";
    constexpr StrView close = "\"]";
    constexpr StrView ellipsis = "...";
    const usize newline = source.find_first_of("\r\n");
    const bool hasNewline = newline != StrView::npos;
    StrView body = source.substr(0, hasNewline ? newline : source.size());

    const usize overhead = open.size() + close.size();
    const usize maxBody = SHORT_SRC_LIMIT > overhead ? SHORT_SRC_LIMIT - overhead : 0;
    bool truncated = hasNewline || body.size() > maxBody;
    if (truncated && maxBody > ellipsis.size()) {
        body = body.substr(0, maxBody - ellipsis.size());
    } else if (body.size() > maxBody) {
        body = body.substr(0, maxBody);
    }

    Str result(open);
    result.append(body);
    if (truncated) {
        result.append(ellipsis);
    }
    result.append(close);
    return result;
}

LuaState* getThreadArgument(LuaState* L, i32& argBase) {
    argBase = 1;
    if (L->getTop() >= 1 && L->at(1).isThread()) {
        argBase = 2;
        return L->at(1).asThread()->getLuaState();
    }
    return L;
}

Function* functionFromCallInfo(LuaState* ownerL, const CallInfo& ci) {
    Stack& stack = ownerL->getStack();
    if (ci.func >= stack.size()) {
        return nullptr;
    }

    Value& funcValue = stack[ci.func];
    if (!funcValue.isFunction()) {
        return nullptr;
    }

    return funcValue.asFunction();
}

struct DebugFrameRef {
    Function* func = nullptr;
    const CallInfo* ci = nullptr;
    usize stackIndex = 0;
};

enum class StackLevelKind {
    Invalid,
    Physical,
    Tail,
};

StackLevelKind resolveStackLevelKind(LuaState* ownerL, i32 level, DebugFrameRef& outFrame) {
    if (level < 0) {
        return StackLevelKind::Invalid;
    }

    usize currentIndex = ownerL->getCurrentCI();
    Vec<CallInfo>& frames = ownerL->getCallStack();
    if (currentIndex >= frames.size()) {
        return StackLevelKind::Invalid;
    }

    usize targetIndex = currentIndex;
    while (level > 0 && targetIndex > 0) {
        const CallInfo& ci = frames[targetIndex];
        --level;
        level -= ci.tailcalls;
        --targetIndex;
    }

    if (level < 0) {
        return StackLevelKind::Tail;
    }
    if (level > 0) {
        return StackLevelKind::Invalid;
    }

    const CallInfo& ci = frames[targetIndex];
    Function* func = functionFromCallInfo(ownerL, ci);
    if (func == nullptr) {
        return StackLevelKind::Invalid;
    }

    outFrame.func = func;
    outFrame.ci = &ci;
    outFrame.stackIndex = targetIndex;
    return StackLevelKind::Physical;
}

bool resolveStackLevel(LuaState* ownerL, i32 level, DebugFrameRef& outFrame) {
    return resolveStackLevelKind(ownerL, level, outFrame) == StackLevelKind::Physical;
}

i32 currentPcForFrame(Function* func, const CallInfo* ci) {
    if (func == nullptr || ci == nullptr || func->isCFunction()) {
        return 0;
    }

    Proto* proto = func->getProto();
    if (proto == nullptr) {
        return 0;
    }

    const Vec<Instruction>& code = proto->getCode();
    if (code.empty() || ci->savedpc == nullptr) {
        return 0;
    }

    i32 pc = static_cast<i32>(ci->savedpc - code.data()) - 1;
    if (pc < 0) {
        pc = 0;
    }
    if (pc >= static_cast<i32>(code.size())) {
        pc = static_cast<i32>(code.size()) - 1;
    }

    return pc;
}

i32 currentLineForFrame(Function* func, const CallInfo* ci) {
    if (func == nullptr || ci == nullptr || func->isCFunction()) {
        return -1;
    }

    Proto* proto = func->getProto();
    if (proto == nullptr) {
        return -1;
    }

    return proto->getLine(static_cast<usize>(currentPcForFrame(func, ci)));
}

const LocVar* resolveLocalInfo(Function* func, const CallInfo* ci, i32 localNumber, bool activeFrame) {
    if (func == nullptr || func->isCFunction() || localNumber <= 0) {
        return nullptr;
    }

    Proto* proto = func->getProto();
    if (proto == nullptr) {
        return nullptr;
    }

    i32 pc = activeFrame ? currentPcForFrame(func, ci) : 0;
    return proto->getLocalVarInfo(localNumber, pc);
}

bool getTemporaryLocalSlot(LuaState* ownerL, const DebugFrameRef& frame, i32 localNumber, usize& outSlot) {
    if (ownerL == nullptr || frame.ci == nullptr || localNumber <= 0) {
        return false;
    }

    usize absSlot = frame.ci->base + static_cast<usize>(localNumber - 1);
    usize limit = ownerL->getStack().size();
    if (frame.stackIndex == ownerL->getCurrentCI()) {
        limit = std::min(limit, ownerL->getAbsoluteTop());
    } else {
        limit = std::min(limit, frame.ci->top);
    }

    if (frame.func != nullptr && frame.func->isLuaFunction()) {
        Proto* proto = frame.func->getProto();
        if (proto != nullptr && frame.ci->savedpc != nullptr) {
            i32 pc = currentPcForFrame(frame.func, frame.ci);
            if (pc >= 0 && pc < static_cast<i32>(proto->getInstructionCount())) {
                Instruction inst = proto->getInstruction(static_cast<usize>(pc));
                OpCode op = GET_OPCODE(inst);
                if (op == OpCode::CALL || op == OpCode::TAILCALL) {
                    limit = std::min(limit, frame.ci->base + static_cast<usize>(GETARG_A(inst)));
                }
            }
        }
    }

    if (absSlot >= limit) {
        return false;
    }

    const Value& value = ownerL->getStack().at(absSlot);
    if (value.isNil()) {
        return false;
    }

    outSlot = absSlot;
    return true;
}

GCString* upvalueNameOrEmpty(LuaState* L, Function* func, usize index) {
    if (func == nullptr) {
        return nullptr;
    }

    if (func->isLuaFunction()) {
        Proto* proto = func->getProto();
        if (proto != nullptr) {
            if (GCString* name = proto->getUpvalueName(index)) {
                return name;
            }
        }
    }

    return internEmptyString(L);
}

Table* createGCManagedTable(LuaState* L) {
    Table* table = new Table();
    L->getGlobalState().getGC().registerObject(table);
    return table;
}

void populateInfoS(Table* info, LuaState* L, Function* func, const CallInfo* ci) {
    if (func == nullptr) {
        return;
    }

    if (func->isCFunction()) {
        setStringField(info, L, "source", "=[C]");
        setStringField(info, L, "short_src", "[C]");
        setStringField(info, L, "what", "C");
        setNumberField(info, L, "linedefined", -1);
        setNumberField(info, L, "lastlinedefined", -1);
        setNumberField(info, L, "currentline", -1);
        return;
    }

    Proto* proto = func->getProto();
    StrView source = proto && proto->getSource() ? proto->getSource()->view() : StrView("=?");

    setStringField(info, L, "source", source);
    setStringField(info, L, "short_src", makeShortSource(source));
    setStringField(
        info,
        L,
        "what",
        (proto && proto->getLineDefined() == 0) ? StrView("main") : StrView("Lua")
    );
    setNumberField(info, L, "linedefined", proto ? proto->getLineDefined() : -1);
    setNumberField(info, L, "lastlinedefined", proto ? proto->getLastLineDefined() : -1);
    setNumberField(info, L, "currentline", currentLineForFrame(func, ci));
}

void populateTailInfoS(Table* info, LuaState* L) {
    setStringField(info, L, "source", "=(tail call)");
    setStringField(info, L, "short_src", "(tail call)");
    setStringField(info, L, "what", "tail");
    setNumberField(info, L, "linedefined", -1);
    setNumberField(info, L, "lastlinedefined", -1);
    setNumberField(info, L, "currentline", -1);
}

void populateInfoU(Table* info, LuaState* L, Function* func) {
    if (func == nullptr) {
        return;
    }

    setNumberField(info, L, "nups", static_cast<i32>(func->getNumUpvalues()));
}

bool isValidGetInfoOption(char option) {
    switch (option) {
        case 'S':
        case 'l':
        case 'u':
        case 'n':
        case 'L':
        case 'f':
            return true;
        default:
            return false;
    }
}

GCString* stringConstantOrQuestion(LuaState* L, Proto* proto, i32 index) {
    if (proto != nullptr && index >= 0 && static_cast<usize>(index) < proto->getConstantCount()) {
        Value constant = proto->getConstant(static_cast<usize>(index));
        if (constant.isString()) {
            return constant.asString();
        }
    }

    return internString(L, "?");
}

GCString* rkNameOrQuestion(LuaState* L, Proto* proto, i32 operand) {
    if (ISK(operand)) {
        return stringConstantOrQuestion(L, proto, INDEXK(operand));
    }

    return internString(L, "?");
}

GCString* activeLocalNameForRegister(Proto* proto, i32 reg, i32 pc) {
    if (proto == nullptr || reg < 0) {
        return nullptr;
    }

    GCString* bestMatch = nullptr;
    i32 bestStartPc = -1;
    for (usize i = 0; i < proto->getLocVarCount(); i++) {
        const LocVar& local = proto->getLocVar(i);
        if (local.varname == nullptr || local.reg != reg) {
            continue;
        }
        if (local.startpc <= pc && pc < local.endpc && local.startpc >= bestStartPc) {
            bestMatch = local.varname;
            bestStartPc = local.startpc;
        }
    }

    return bestMatch;
}

bool instructionWritesRegister(Instruction instruction, i32 reg) {
    OpCode op = GET_OPCODE(instruction);
    i32 a = GETARG_A(instruction);

    switch (op) {
        case OpCode::MOVE:
        case OpCode::LOADK:
        case OpCode::LOADBOOL:
        case OpCode::GETUPVAL:
        case OpCode::GETGLOBAL:
        case OpCode::GETTABLE:
        case OpCode::NEWTABLE:
        case OpCode::ADD:
        case OpCode::SUB:
        case OpCode::MUL:
        case OpCode::DIV:
        case OpCode::MOD:
        case OpCode::POW:
        case OpCode::UNM:
        case OpCode::NOT:
        case OpCode::LEN:
        case OpCode::CONCAT:
        case OpCode::TESTSET:
        case OpCode::CLOSURE:
            return reg == a;

        case OpCode::LOADNIL:
            return a <= reg && reg <= GETARG_B(instruction);

        case OpCode::SELF:
            return reg == a || reg == (a + 1);

        case OpCode::CALL:
        case OpCode::TAILCALL: {
            i32 results = GETARG_C(instruction) - 1;
            if (results == MULTRET) {
                return reg >= a;
            }
            return results > 0 && a <= reg && reg < (a + results);
        }

        case OpCode::VARARG: {
            i32 results = GETARG_B(instruction) - 1;
            if (results == MULTRET) {
                return reg >= a;
            }
            return results >= 0 && a <= reg && reg < (a + results);
        }

        case OpCode::TFORLOOP:
            return reg >= (a + 3) && reg <= (a + 2 + GETARG_C(instruction));

        case OpCode::FORLOOP:
            return reg == a || reg == (a + 3);

        default:
            return false;
    }
}

bool findRegisterSetter(Proto* proto, i32 pc, i32 reg, i32& setterPc, Instruction& setter) {
    if (proto == nullptr || reg < 0) {
        return false;
    }

    const Vec<Instruction>& code = proto->getCode();
    i32 upperBound = std::min(pc - 1, static_cast<i32>(code.size()) - 1);
    for (i32 i = upperBound; i >= 0; i--) {
        Instruction instruction = code[static_cast<usize>(i)];
        if (instructionWritesRegister(instruction, reg)) {
            setterPc = i;
            setter = instruction;
            return true;
        }
    }

    return false;
}

const char* inferObjectName(
    LuaState* L,
    Proto* proto,
    i32 pc,
    i32 reg,
    GCString*& outName,
    i32 depth = 0
) {
    if (proto == nullptr || reg < 0 || pc < 0 || depth > 16) {
        return nullptr;
    }

    if (GCString* localName = activeLocalNameForRegister(proto, reg, pc)) {
        outName = localName;
        return "local";
    }

    Instruction setter = 0;
    i32 setterPc = -1;
    if (!findRegisterSetter(proto, pc, reg, setterPc, setter)) {
        return nullptr;
    }

    switch (GET_OPCODE(setter)) {
        case OpCode::GETGLOBAL:
            outName = stringConstantOrQuestion(L, proto, GETARG_Bx(setter));
            return "global";

        case OpCode::MOVE: {
            i32 a = GETARG_A(setter);
            i32 b = GETARG_B(setter);
            if (b < a) {
                return inferObjectName(L, proto, setterPc, b, outName, depth + 1);
            }
            break;
        }

        case OpCode::GETTABLE:
            outName = rkNameOrQuestion(L, proto, GETARG_C(setter));
            return "field";

        case OpCode::GETUPVAL: {
            GCString* upvalueName = proto->getUpvalueName(static_cast<usize>(GETARG_B(setter)));
            outName = upvalueName ? upvalueName : internString(L, "?");
            return "upvalue";
        }

        case OpCode::SELF:
            outName = rkNameOrQuestion(L, proto, GETARG_C(setter));
            return "method";

        default:
            break;
    }

    return nullptr;
}

void populateInfoN(Table* info, LuaState* L, LuaState* ownerL, const DebugFrameRef& frame) {
    setStringField(info, L, "namewhat", "");

    if (frame.ci == nullptr || frame.stackIndex == 0) {
        return;
    }

    if (frame.func != nullptr && frame.func->isLuaFunction() && frame.ci->tailcalls > 0) {
        return;
    }

    const CallInfo& callerCi = ownerL->getCallStack()[frame.stackIndex - 1];
    Function* callerFunc = functionFromCallInfo(ownerL, callerCi);
    if (callerFunc == nullptr || callerFunc->isCFunction()) {
        return;
    }

    Proto* callerProto = callerFunc->getProto();
    i32 callerPc = currentPcForFrame(callerFunc, &callerCi);
    if (callerProto == nullptr || callerPc < 0 ||
        static_cast<usize>(callerPc) >= callerProto->getInstructionCount()) {
        return;
    }

    Instruction instruction = callerProto->getInstruction(static_cast<usize>(callerPc));
    OpCode op = GET_OPCODE(instruction);

    i32 targetReg = -1;
    if (op == OpCode::CALL || op == OpCode::TAILCALL || op == OpCode::TFORLOOP) {
        targetReg = GETARG_A(instruction);
    }
    if (targetReg < 0) {
        return;
    }

    GCString* name = nullptr;
    const char* nameWhat = inferObjectName(L, callerProto, callerPc, targetReg, name);
    if (nameWhat == nullptr) {
        return;
    }

    setStringField(info, L, "namewhat", nameWhat);
    if (name != nullptr) {
        setField(info, L, "name", Value(name));
    }
}

void populateInfoL(Table* info, LuaState* L, Function* func) {
    if (func == nullptr || func->isCFunction()) {
        return;
    }

    Proto* proto = func->getProto();
    if (proto == nullptr) {
        return;
    }

    Table* lines = createGCManagedTable(L);
    for (i32 line : proto->getLineInfo()) {
        if (line > 0 && line != proto->getLineDefined()) {
            lines->set(Value(static_cast<LuaNumber>(line)), Value(true));
        }
    }
    if (proto->getLastLineDefined() > 0) {
        lines->set(Value(static_cast<LuaNumber>(proto->getLastLineDefined())), Value(true));
    }

    setField(info, L, "activelines", Value(lines));
}

Str describeFunction(Function* func) {
    if (func == nullptr) {
        return "?";
    }

    if (func->isCFunction()) {
        return "C function";
    }

    Proto* proto = func->getProto();
    if (proto == nullptr) {
        return "Lua function";
    }

    Str source = proto->getSource() ? proto->getSource()->c_str() : "?";
    if (proto->getLineDefined() <= 0) {
        return "main chunk";
    }

    return std::format("function <{}:{}>", source, proto->getLineDefined());
}

bool inferFrameCallName(
    LuaState* ownerL,
    usize stackIndex,
    const char*& outNameWhat,
    GCString*& outName
) {
    Vec<CallInfo>& frames = ownerL->getCallStack();
    if (stackIndex == 0 || stackIndex >= frames.size()) {
        return false;
    }

    const CallInfo& callerCi = frames[stackIndex - 1];
    Function* callerFunc = functionFromCallInfo(ownerL, callerCi);
    if (callerFunc == nullptr || callerFunc->isCFunction()) {
        return false;
    }

    Proto* callerProto = callerFunc->getProto();
    i32 callerPc = currentPcForFrame(callerFunc, &callerCi);
    if (callerProto == nullptr || callerPc < 0 ||
        static_cast<usize>(callerPc) >= callerProto->getInstructionCount()) {
        return false;
    }

    Instruction instruction = callerProto->getInstruction(static_cast<usize>(callerPc));
    OpCode op = GET_OPCODE(instruction);

    i32 targetReg = -1;
    if (op == OpCode::CALL || op == OpCode::TAILCALL || op == OpCode::TFORLOOP) {
        targetReg = GETARG_A(instruction);
    }
    if (targetReg < 0) {
        return false;
    }

    outNameWhat = inferObjectName(ownerL, callerProto, callerPc, targetReg, outName);
    return outNameWhat != nullptr && outName != nullptr;
}

Str describeNamedCall(const char* nameWhat, GCString* name) {
    if (nameWhat == nullptr || name == nullptr) {
        return "";
    }

    if (std::strcmp(nameWhat, "method") == 0) {
        return std::format("method '{}'", name->c_str());
    }
    return std::format("function '{}'", name->c_str());
}

Str describeCFunctionAtFrame(LuaState* ownerL, usize stackIndex) {
    const char* nameWhat = nullptr;
    GCString* name = nullptr;
    if (!inferFrameCallName(ownerL, stackIndex, nameWhat, name)) {
        return "C function";
    }

    Str namedCall = describeNamedCall(nameWhat, name);
    return namedCall.empty() ? "C function" : namedCall;
}

Str formatFrameLine(LuaState* ownerL, const CallInfo& ci, usize stackIndex) {
    Function* func = functionFromCallInfo(ownerL, ci);
    if (func == nullptr) {
        return "\t?";
    }

    if (func->isCFunction()) {
        return "\t[C]: in " + describeCFunctionAtFrame(ownerL, stackIndex);
    }

    Proto* proto = func->getProto();
    Str source = proto && proto->getSource() ? proto->getSource()->c_str() : "?";
    i32 line = currentLineForFrame(func, &ci);
    if (line < 0 && proto != nullptr) {
        line = proto->getLineDefined();
    }

    Str description = describeFunction(func);
    const char* nameWhat = nullptr;
    GCString* name = nullptr;
    if (inferFrameCallName(ownerL, stackIndex, nameWhat, name)) {
        Str namedCall = describeNamedCall(nameWhat, name);
        if (!namedCall.empty()) {
            description = namedCall;
        }
    }

    if (line > 0) {
        return std::format("\t{}:{}: in {}", source, line, description);
    }
    return std::format("\t{}: in {}", source, description);
}

Str formatTailCallLine() {
    return "\t(tail call): ?";
}

Function* checkFunctionArg(LuaState* L, i32 idx, const char* message) {
    if (!L->isFunction(idx)) {
        L->error(message);
    }
    return L->at(idx).asFunction();
}

i32 checkPositiveIndex(LuaState* L, i32 idx, const char* message) {
    if (!L->isNumber(idx)) {
        L->error(message);
    }

    i32 value = static_cast<i32>(L->toNumber(idx));
    if (value <= 0) {
        L->error(message);
    }
    return value;
}

i32 checkNonNegativeLevel(LuaState* L, i32 idx, const char* message) {
    if (!L->isNumber(idx)) {
        L->error(message);
    }

    i32 value = static_cast<i32>(L->toNumber(idx));
    if (value < 0) {
        L->error(message);
    }
    return value;
}

bool parseHookMask(StrView mask, u8& outMask) {
    outMask = 0;

    for (char ch : mask) {
        switch (ch) {
            case 'c':
                outMask |= HookMaskCall;
                break;
            case 'r':
                outMask |= HookMaskReturn;
                break;
            case 'l':
                outMask |= HookMaskLine;
                break;
            default:
                return false;
        }
    }

    return true;
}

Str hookMaskToString(u8 mask) {
    Str result;
    if ((mask & HookMaskCall) != 0) {
        result.push_back('c');
    }
    if ((mask & HookMaskReturn) != 0) {
        result.push_back('r');
    }
    if ((mask & HookMaskLine) != 0) {
        result.push_back('l');
    }
    return result;
}

void runDebugCommand(LuaState* L, const Str& source) {
    auto& pool = L->getGlobalState().getStringPool();

    Parser parser(source);
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk, "=(debug command)");
    if (proto == nullptr) {
        throw std::runtime_error("debug.debug: compilation failed");
    }

    Function* func = new Function(proto);
    L->getGlobalState().getGC().registerObject(func);
    func->setEnv(L->getGlobalTable());

    usize savedTop = L->getAbsoluteTop();
    try {
        L->pushFunction(func);
        VM::call(L, 0, 0);
        L->getStack().setTop(savedTop);
        L->setAbsoluteTop(savedTop);
    } catch (...) {
        L->getStack().setTop(savedTop);
        L->setAbsoluteTop(savedTop);
        throw;
    }
}

} // namespace

// =====================================================================
// debug.getregistry() - Get the registry table
// =====================================================================

i32 luaDebug_getregistry(LuaState* L) {
    L->pushTable(L->getGlobalState().getRegistry());
    return 1;
}

// =====================================================================
// debug.getupvalue(func, up) - Get a function upvalue
// =====================================================================

i32 luaDebug_getupvalue(LuaState* L) {
    Function* func = checkFunctionArg(
        L,
        1,
        "bad argument #1 to 'getupvalue' (function expected)"
    );
    if (!L->isNumber(2)) {
        L->error("bad argument #2 to 'getupvalue' (number expected)");
    }
    i32 upIndex = static_cast<i32>(L->toNumber(2));
    if (upIndex <= 0) {
        L->pushNil();
        return 1;
    }

    Upvalue* upvalue = func->getUpvalue(static_cast<usize>(upIndex - 1));
    if (upvalue == nullptr) {
        L->pushNil();
        return 1;
    }

    GCString* name = upvalueNameOrEmpty(L, func, static_cast<usize>(upIndex - 1));
    L->pushString(name);
    L->pushValue(upvalue->getValue(L->getStack()));
    return 2;
}

// =====================================================================
// debug.setupvalue(func, up, value) - Set a function upvalue
// =====================================================================

i32 luaDebug_setupvalue(LuaState* L) {
    Function* func = checkFunctionArg(
        L,
        1,
        "bad argument #1 to 'setupvalue' (function expected)"
    );
    if (!L->isNumber(2)) {
        L->error("bad argument #2 to 'setupvalue' (number expected)");
    }
    i32 upIndex = static_cast<i32>(L->toNumber(2));
    if (upIndex <= 0) {
        L->pushNil();
        return 1;
    }

    Upvalue* upvalue = func->getUpvalue(static_cast<usize>(upIndex - 1));
    if (upvalue == nullptr) {
        L->pushNil();
        return 1;
    }

    upvalue->setValue(L->getStack(), L->at(3));
    L->pushString(upvalueNameOrEmpty(L, func, static_cast<usize>(upIndex - 1)));
    return 1;
}

// =====================================================================
// debug.getinfo(thread|func|level [, what]) - Get debug information
// =====================================================================

i32 luaDebug_getinfo(LuaState* L) {
    i32 argBase = 1;
    LuaState* ownerL = getThreadArgument(L, argBase);

    if (L->getTop() < argBase) {
        L->error("bad argument to 'getinfo' (function or level expected)");
    }

    StrView options = "flnSu";
    if (L->getTop() >= argBase + 1) {
        if (!L->isString(argBase + 1)) {
            L->error("bad argument to 'getinfo' (string expected)");
        }
        options = L->toString(argBase + 1);
    }

    for (char option : options) {
        if (!isValidGetInfoOption(option)) {
            L->error("bad argument to 'getinfo' (invalid option)");
        }
    }

    DebugFrameRef frame;
    Function* func = nullptr;
    bool isTailFrame = false;

    if (L->isFunction(argBase)) {
        func = L->at(argBase).asFunction();
    } else if (L->isNumber(argBase)) {
        i32 level = checkNonNegativeLevel(
            L,
            argBase,
            "bad argument to 'getinfo' (stack level expected)"
        );
        StackLevelKind kind = resolveStackLevelKind(ownerL, level, frame);
        if (kind == StackLevelKind::Invalid) {
            L->pushNil();
            return 1;
        }
        if (kind == StackLevelKind::Tail) {
            isTailFrame = true;
        } else {
            func = frame.func;
        }
    } else {
        L->error("bad argument to 'getinfo' (function or level expected)");
    }

    Table* info = createGCManagedTable(L);
    for (char option : options) {
        switch (option) {
            case 'S':
                if (isTailFrame) {
                    populateTailInfoS(info, L);
                } else {
                    populateInfoS(info, L, func, frame.ci);
                }
                break;
            case 'u':
                if (isTailFrame) {
                    setNumberField(info, L, "nups", 0);
                } else {
                    populateInfoU(info, L, func);
                }
                break;
            case 'f':
                setField(info, L, "func", isTailFrame ? Value() : Value(func));
                break;
            case 'l':
                setNumberField(info, L, "currentline", isTailFrame ? -1 : currentLineForFrame(func, frame.ci));
                break;
            case 'L':
                if (!isTailFrame) {
                    populateInfoL(info, L, func);
                }
                break;
            case 'n':
                if (!isTailFrame) {
                    populateInfoN(info, L, ownerL, frame);
                } else {
                    setStringField(info, L, "namewhat", "");
                }
                break;
            default:
                break;
        }
    }

    L->pushTable(info);
    return 1;
}

// =====================================================================
// debug.getlocal(thread|func|level, local) - Get a local variable
// =====================================================================

i32 luaDebug_getlocal(LuaState* L) {
    i32 argBase = 1;
    LuaState* ownerL = getThreadArgument(L, argBase);

    if (L->getTop() < argBase + 1) {
        L->error("bad argument to 'getlocal' (value and local index expected)");
    }

    if (!L->isNumber(argBase + 1)) {
        L->error("bad argument to 'getlocal' (local index expected)");
    }
    i32 localIndex = static_cast<i32>(L->toNumber(argBase + 1));
    if (localIndex <= 0) {
        L->pushNil();
        return 1;
    }

    if (L->isFunction(argBase)) {
        Function* func = L->at(argBase).asFunction();
        const LocVar* localInfo = resolveLocalInfo(func, nullptr, localIndex, false);
        if (localInfo == nullptr || localInfo->varname == nullptr) {
            L->pushNil();
            return 1;
        }

        L->pushString(localInfo->varname);
        return 1;
    }

    i32 level = checkNonNegativeLevel(
        L,
        argBase,
        "bad argument to 'getlocal' (stack level expected)"
    );

    DebugFrameRef frame;
    if (!resolveStackLevel(ownerL, level, frame)) {
        L->pushNil();
        return 1;
    }

    const LocVar* localInfo = resolveLocalInfo(frame.func, frame.ci, localIndex, true);
    if (localInfo == nullptr || localInfo->varname == nullptr) {
        usize tempSlot = 0;
        if (getTemporaryLocalSlot(ownerL, frame, localIndex, tempSlot)) {
            L->pushString(internString(L, "(*temporary)"));
            L->pushValue(ownerL->getStack().at(tempSlot));
            return 2;
        }

        L->pushNil();
        return 1;
    }

    usize absSlot = frame.ci->base + static_cast<usize>(localInfo->reg);
    if (absSlot >= ownerL->getStack().size()) {
        L->pushNil();
        return 1;
    }

    L->pushString(localInfo->varname);
    L->pushValue(ownerL->getStack().at(absSlot));
    return 2;
}

// =====================================================================
// debug.setlocal([thread,] level, local, value) - Set an active local variable
// =====================================================================

i32 luaDebug_setlocal(LuaState* L) {
    i32 argBase = 1;
    LuaState* ownerL = getThreadArgument(L, argBase);

    if (L->getTop() < argBase + 2) {
        L->error("bad argument to 'setlocal' (stack level, local index and value expected)");
    }

    i32 level = checkNonNegativeLevel(
        L,
        argBase,
        "bad argument to 'setlocal' (stack level expected)"
    );
    i32 localIndex = checkPositiveIndex(
        L,
        argBase + 1,
        "bad argument to 'setlocal' (positive local index expected)"
    );

    DebugFrameRef frame;
    if (!resolveStackLevel(ownerL, level, frame)) {
        L->pushNil();
        return 1;
    }

    const LocVar* localInfo = resolveLocalInfo(frame.func, frame.ci, localIndex, true);
    if (localInfo == nullptr || localInfo->varname == nullptr) {
        usize tempSlot = 0;
        if (getTemporaryLocalSlot(ownerL, frame, localIndex, tempSlot)) {
            ownerL->getStack().at(tempSlot) = L->at(argBase + 2);
            L->pushString(internString(L, "(*temporary)"));
            return 1;
        }

        L->pushNil();
        return 1;
    }

    usize absSlot = frame.ci->base + static_cast<usize>(localInfo->reg);
    if (absSlot >= ownerL->getStack().size()) {
        L->pushNil();
        return 1;
    }

    ownerL->getStack().at(absSlot) = L->at(argBase + 2);
    L->pushString(localInfo->varname);
    return 1;
}

// =====================================================================
// debug.getmetatable(object) / debug.setmetatable(object, table|nil)
// =====================================================================

i32 luaDebug_getmetatable(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("debug.getmetatable: missing argument");
    }

    const Value& value = L->at(1);
    Table* metatable = nullptr;
    if (value.isTable()) {
        metatable = value.asTable()->getMetatable();
    } else if (value.isUserdata()) {
        metatable = value.asUserdata()->getMetatable();
    }

    if (metatable != nullptr) {
        L->pushTable(metatable);
    } else {
        L->pushNil();
    }
    return 1;
}

i32 luaDebug_setmetatable(LuaState* L) {
    if (L->getTop() < 2) {
        L->error("debug.setmetatable: expected 2 arguments");
    }

    const Value& value = L->at(1);
    if (!value.isTable() && !value.isUserdata()) {
        L->error("debug.setmetatable: table or userdata expected");
    }

    if (!L->at(2).isNil() && !L->at(2).isTable()) {
        L->error("debug.setmetatable: metatable must be nil or table");
    }

    L->pushValue(2);
    if (!L->setMetatable(1)) {
        L->error("debug.setmetatable: cannot set metatable");
    }

    L->setTop(1);
    return 1;
}

// =====================================================================
// debug.getfenv(f) / debug.setfenv(f, table)
// =====================================================================

i32 luaDebug_getfenv(LuaState* L) {
    if (L->getTop() >= 1 && L->at(1).isThread()) {
        LuaState* threadL = L->at(1).asThread()->getLuaState();
        L->pushTable(threadL->getGlobalTable());
        return 1;
    }

    return luaB_getfenv(L);
}

i32 luaDebug_setfenv(LuaState* L) {
    if (L->getTop() >= 1 && L->at(1).isThread()) {
        if (L->getTop() < 2 || !L->at(2).isTable()) {
            L->error("debug.setfenv: 'table' expected");
        }

        LuaState* threadL = L->at(1).asThread()->getLuaState();
        threadL->setGlobalTable(L->at(2).asTable());
        L->pushValue(1);
        return 1;
    }

    return luaB_setfenv(L);
}

// =====================================================================
// debug.traceback([thread,] [message [, level]]) - Build a traceback string
// =====================================================================

i32 luaDebug_traceback(LuaState* L) {
    i32 argBase = 1;
    LuaState* ownerL = getThreadArgument(L, argBase);

    const char* message = nullptr;
    if (L->getTop() >= argBase && !L->isNil(argBase)) {
        if (!L->isString(argBase)) {
            L->pushValue(L->at(argBase));
            return 1;
        }
        message = L->toString(argBase);
    }

    i32 level = (ownerL == L) ? 1 : 0;
    if (L->getTop() >= argBase + 1) {
        if (!L->isNumber(argBase + 1)) {
            L->error("bad argument to 'traceback' (number expected)");
        }
        level = std::max(0, static_cast<i32>(L->toNumber(argBase + 1)));
    }

    Str traceback;
    if (message != nullptr && *message != '\0') {
        traceback = std::format("{}\nstack traceback:", message);
    } else {
        traceback = "stack traceback:";
    }

    i32 startIndex = static_cast<i32>(ownerL->getCurrentCI()) - level;
    for (i32 index = startIndex; index >= 0; --index) {
        const CallInfo& ci = ownerL->getCallStack()[static_cast<usize>(index)];
        Function* func = functionFromCallInfo(ownerL, ci);
        if (func == nullptr) {
            continue;
        }

        traceback += std::format("\n{}", formatFrameLine(ownerL, ci, static_cast<usize>(index)));
        for (i32 tail = 0; tail < ci.tailcalls; ++tail) {
            traceback += std::format("\n{}", formatTailCallLine());
        }
    }

    L->pushString(internString(L, traceback));
    return 1;
}

// =====================================================================
// debug.sethook([thread,] hook, mask [, count]) - Install a debug hook
// =====================================================================

i32 luaDebug_sethook(LuaState* L) {
    i32 argBase = 1;
    LuaState* ownerL = getThreadArgument(L, argBase);

    if (L->getTop() < argBase) {
        ownerL->setDebugHook(nullptr, 0, 0);
        return 0;
    }

    Function* hook = nullptr;
    if (!L->isNil(argBase)) {
        hook = checkFunctionArg(
            L,
            argBase,
            "bad argument to 'sethook' (function or nil expected)"
        );
    }

    u8 mask = 0;
    i32 count = 0;
    if (hook != nullptr) {
        if (L->getTop() < argBase + 1 || !L->isString(argBase + 1)) {
            L->error("bad argument to 'sethook' (mask string expected)");
        }

        if (!parseHookMask(L->toString(argBase + 1), mask)) {
            L->error("bad argument to 'sethook' (invalid hook mask)");
        }

        if (L->getTop() >= argBase + 2) {
            if (!L->isNumber(argBase + 2)) {
                L->error("bad argument to 'sethook' (count expected)");
            }
            count = std::max(0, static_cast<i32>(L->toNumber(argBase + 2)));
        }
    }

    ownerL->setDebugHook(hook, mask, count);
    if (hook != nullptr && (mask & HookMaskLine) != 0) {
        Vec<CallInfo>& frames = ownerL->getCallStack();
        for (usize i = 0; i <= ownerL->getCurrentCI() && i < frames.size(); ++i) {
            CallInfo& ci = frames[i];
            Function* currentFunc = functionFromCallInfo(ownerL, ci);
            if (currentFunc != nullptr && currentFunc->isLuaFunction()) {
                ci.hookLine = currentLineForFrame(currentFunc, &ci);
                ci.hookPc = currentPcForFrame(currentFunc, &ci);
            }
        }
    }
    return 0;
}

// =====================================================================
// debug.gethook([thread]) - Query the current debug hook
// =====================================================================

i32 luaDebug_gethook(LuaState* L) {
    i32 argBase = 1;
    LuaState* ownerL = getThreadArgument(L, argBase);
    (void)argBase;

    Function* hook = ownerL->getDebugHook();
    if (hook != nullptr) {
        L->pushFunction(hook);
    } else {
        L->pushNil();
    }

    L->pushString(internString(L, hookMaskToString(ownerL->getDebugHookMask())));
    L->pushNumber(static_cast<LuaNumber>(ownerL->getDebugHookCount()));
    return 3;
}

// =====================================================================
// debug.debug() - Enter the interactive debug console
// =====================================================================

i32 luaDebug_debug(LuaState* L) {
    Str line;

    for (;;) {
        std::cout << "lua_debug> " << std::flush;
        if (!std::getline(std::cin, line)) {
            break;
        }

        if (line == "cont") {
            break;
        }

        if (line.empty()) {
            continue;
        }

        try {
            runDebugCommand(L, line);
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }

    return 0;
}

// =====================================================================
// Debug library registration entry
// =====================================================================

void DebugLibModule::registerFunctions(LuaState* L) {
    if (!L) {
        return;
    }

    Table* debugTable = FunctionRegistrar::createLibTable(L, "debug");
    if (!debugTable) {
        L->error("Failed to create debug library table");
        return;
    }

    FunctionRegistrar(L)
        .addGlobal("getregistry", luaDebug_getregistry)
        .addGlobal("getupvalue", luaDebug_getupvalue)
        .addGlobal("setupvalue", luaDebug_setupvalue)
        .addGlobal("getinfo", luaDebug_getinfo)
        .addGlobal("getlocal", luaDebug_getlocal)
        .addGlobal("setlocal", luaDebug_setlocal)
        .addGlobal("getmetatable", luaDebug_getmetatable)
        .addGlobal("setmetatable", luaDebug_setmetatable)
        .addGlobal("getfenv", luaDebug_getfenv)
        .addGlobal("setfenv", luaDebug_setfenv)
        .addGlobal("traceback", luaDebug_traceback)
        .addGlobal("sethook", luaDebug_sethook)
        .addGlobal("gethook", luaDebug_gethook)
        .addGlobal("debug", luaDebug_debug)
        .commitToTable(debugTable);
}

void openDebugLib(LuaState* L) {
    if (!L) {
        return;
    }

    DebugLibModule module;
    StandardLibrary::openModule(L, module);
}

} // namespace Lua
