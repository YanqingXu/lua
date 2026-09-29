/**
 * @file bytecode_printer.cpp
 * @brief Lua 函数原型字节码打印与差异比较的实现
 */

#include "common/types.hpp"
#include "bytecode_printer.hpp"
#include "compiler/opcode.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/value.hpp"

#include <algorithm>
#include <format>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Lua {

namespace {

Str escapeString(CharPtr value) {
    Str out;
    for (CharPtr p = value ? value : ""; *p; ++p) {
        switch (*p) {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out += *p;
            break;
        }
    }
    return out;
}

Str formatValue(const Value& value) {
    if (value.isNil()) {
        return "nil";
    } else if (value.isBoolean()) {
        return std::format("boolean {}", value.asBoolean() ? "true" : "false");
    } else if (value.isNumber()) {
        return std::format("number {}", value.asNumber());
    } else if (value.isString()) {
        return std::format("string \"{}\"", escapeString(value.asString() ? value.asString()->c_str() : ""));
    } else if (value.isLightUserdata()) {
        return "lightuserdata";
    } else if (value.isTable()) {
        return "table";
    } else if (value.isFunction()) {
        return "function";
    } else if (value.isUserdata()) {
        return "userdata";
    } else if (value.isThread()) {
        return "thread";
    } else {
        return "unknown";
    }
}

Str formatConstant(const Proto* proto, i32 index) {
    Str prefix = std::format("K[{}]", index);
    if (!proto || index < 0 || static_cast<usize>(index) >= proto->getConstantCount()) {
        return std::format("{} = <out of range>", prefix);
    }

    return std::format("{} = {}", prefix, formatValue(proto->getConstant(static_cast<usize>(index))));
}

Str formatProtoSummary(const Proto* proto) {
    if (!proto) {
        return "<null>";
    }

    CharPtr source = proto->getSource() ? proto->getSource()->c_str() : "?";
    if (proto->getLineDefined() > 0) {
        return std::format("{}:{}", source, proto->getLineDefined());
    }
    return source;
}

Str formatSubProto(const Proto* proto, i32 index) {
    Str prefix = std::format("proto[{}]", index);
    if (!proto || index < 0 || static_cast<usize>(index) >= proto->getSubProtoCount()) {
        return std::format("{} = <out of range>", prefix);
    }

    return std::format("{} = {}", prefix, formatProtoSummary(proto->getSubProto(static_cast<usize>(index))));
}

Str indentFor(usize depth) {
    return Str(depth * 2, ' ');
}

void addRKComment(Vec<Str>& comments, const Proto* proto, CharPtr name, i32 operand, OpArgMask mode) {
    if (mode != OpArgMask::OpArgK || !ISK(operand)) {
        return;
    }

    comments.push_back(std::format("{}={}", name, formatConstant(proto, INDEXK(operand))));
}

void printComments(std::ostream& out, const Vec<Str>& comments) {
    if (comments.empty()) {
        return;
    }

    out << " ; ";
    for (usize i = 0; i < comments.size(); ++i) {
        if (i != 0) {
            out << "; ";
        }
        out << comments[i];
    }
}

void printProtoHeader(const Proto* f, std::ostream& out, StrView indent) {
    out << indent << "Proto" << '\n';
    out << indent << "  source: " << (f->getSource() ? f->getSource()->c_str() : "?") << '\n';
    out << indent << "  linedefined: " << f->getLineDefined() << '\n';
    out << indent << "  lastlinedefined: " << f->getLastLineDefined() << '\n';
    out << indent << "  numparams: " << static_cast<int>(f->getNumParams()) << '\n';
    out << indent
        << std::format("  is_vararg: {} (flags={})\n", f->isVararg() ? "true" : "false",
                       static_cast<int>(f->getVarargFlags()));
    out << indent << "  maxStackSize: " << static_cast<int>(f->getMaxStackSize()) << '\n';

    const usize upvalueCount = std::max(static_cast<usize>(f->getNumUpvalues()), f->getUpvalueNameCount());
    out << indent << "  upvalues (" << upvalueCount << "): ";
    if (upvalueCount == 0) {
        out << "(none)";
    } else {
        for (usize i = 0; i < upvalueCount; ++i) {
            if (i != 0) {
                out << ", ";
            }
            GCString* name = f->getUpvalueName(i);
            out << (name ? name->c_str() : "?");
        }
    }
    out << '\n';
}

void printInstruction(const Proto* f, usize pc, Instruction inst, std::ostream& out, StrView indent) {
    const OpCode op = GET_OPCODE(inst);
    const OpcodeMetadata& metadata = opcodeMetadata(op);
    Vec<Str> comments;

    out << indent << std::format("{:04} | line {} | {} | ", pc, f->getLine(pc), metadata.name);

    switch (metadata.mode) {
    case OpMode::iABC: {
        const i32 a = GETARG_A(inst);
        const i32 b = GETARG_B(inst);
        const i32 c = GETARG_C(inst);
        out << "A=" << a << " B=" << b << " C=" << c;
        addRKComment(comments, f, "B", b, metadata.bMode);
        addRKComment(comments, f, "C", c, metadata.cMode);
        break;
    }
    case OpMode::iABx: {
        const i32 a = GETARG_A(inst);
        const i32 bx = GETARG_Bx(inst);
        out << "A=" << a << " Bx=" << bx;
        if (metadata.bMode == OpArgMask::OpArgK) {
            comments.push_back(formatConstant(f, bx));
        } else if (metadata.opcode == OpCode::CLOSURE) {
            comments.push_back(formatSubProto(f, bx));
        }
        break;
    }
    case OpMode::iAsBx: {
        const i32 a = GETARG_A(inst);
        const i32 sbx = GETARG_sBx(inst);
        out << "A=" << a << " sBx=" << sbx;
        comments.push_back(std::format("target={}", static_cast<i32>(pc) + 1 + sbx));
        break;
    }
    }

    printComments(out, comments);
    out << '\n';
}

void printInstructions(const Proto* f, std::ostream& out, StrView indent) {
    const auto code = f->getInstructionSpan();
    out << indent << "instructions (" << code.size() << ")" << '\n';
    for (usize pc = 0; pc < code.size(); ++pc) {
        printInstruction(f, pc, code[pc], out, indent);
    }
}

void printConstants(const Proto* f, std::ostream& out, StrView indent) {
    out << indent << "constants (" << f->getConstantCount() << ")" << '\n';
    for (usize i = 0; i < f->getConstantCount(); ++i) {
        out << indent << "  " << formatConstant(f, static_cast<i32>(i)) << '\n';
    }
}

bool containsProto(const Vec<const Proto*>& protos, const Proto* proto) {
    return std::find(protos.begin(), protos.end(), proto) != protos.end();
}

void printProtoBytecodeRecursive(const Proto* f, std::ostream& out, bool full, usize depth,
                                 const Vec<const Proto*>& ancestry);

void printChildProtos(const Proto* f, std::ostream& out, usize depth, const Vec<const Proto*>& ancestry) {
    const Str indent = indentFor(depth);
    out << indent << "child protos (" << f->getSubProtoCount() << ")" << '\n';

    for (usize i = 0; i < f->getSubProtoCount(); ++i) {
        const Proto* child = f->getSubProto(i);
        out << indent << "  proto[" << i << "] " << formatProtoSummary(child) << '\n';

        if (!child) {
            out << indent << "    Proto <null>" << '\n';
            continue;
        }

        if (containsProto(ancestry, child)) {
            out << indent << "    Proto <cycle>" << '\n';
            continue;
        }

        printProtoBytecodeRecursive(child, out, true, depth + 2, ancestry);
    }
}

void printProtoBytecodeRecursive(const Proto* f, std::ostream& out, bool full, usize depth,
                                 const Vec<const Proto*>& ancestry) {
    const Str indent = indentFor(depth);

    if (!f) {
        out << indent << "Proto <null>" << '\n';
        return;
    }

    printProtoHeader(f, out, indent);
    printInstructions(f, out, indent);
    printConstants(f, out, indent);

    if (full) {
        Vec<const Proto*> nextAncestry = ancestry;
        nextAncestry.push_back(f);
        printChildProtos(f, out, depth, nextAncestry);
    }
}

Str renderProtoBytecode(const Proto* proto, bool full) {
    std::ostringstream rendered;
    printProtoBytecodeRecursive(proto, rendered, full, 0, {});
    return rendered.str();
}

Vec<Str> splitLines(const Str& text) {
    Vec<Str> lines;
    usize start = 0;

    while (start < text.size()) {
        usize end = text.find('\n', start);
        if (end == Str::npos) {
            end = text.size();
        }

        Str line = text.substr(start, end - start);
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(std::move(line));

        if (end == text.size()) {
            break;
        }
        start = end + 1;
    }

    return lines;
}

bool isSourceMetadataLine(StrView line) {
    const usize first = line.find_first_not_of(' ');
    if (first == StrView::npos) {
        return false;
    }

    constexpr StrView kSourcePrefix = "source: ";
    return line.substr(first).starts_with(kSourcePrefix);
}

Vec<Str> removeDiffNoise(Vec<Str> lines) {
    std::erase_if(lines, [](const Str& line) { return isSourceMetadataLine(line); });
    return lines;
}

usize countChangedLines(const Vec<Str>& leftLines, const Vec<Str>& rightLines) {
    const usize lineCount = std::max(leftLines.size(), rightLines.size());
    usize changed = 0;

    for (usize i = 0; i < lineCount; ++i) {
        const bool hasLeft = i < leftLines.size();
        const bool hasRight = i < rightLines.size();
        const StrView left = hasLeft ? StrView(leftLines[i]) : StrView();
        const StrView right = hasRight ? StrView(rightLines[i]) : StrView();
        if (!hasLeft || !hasRight || left != right) {
            changed += 1;
        }
    }

    return changed;
}

struct CfgBlock {
    usize id = 0;
    usize startPc = 0;
    usize endPc = 0;
};

struct CfgEdge {
    usize fromBlock = 0;
    i32 targetPc = 0;
    Str label;
};

struct CfgGraph {
    Vec<CfgBlock> blocks;
    Vec<CfgEdge> edges;
    Vec<i32> pcToBlock;
};

struct CfgRenderState {
    usize nextProtoId = 0;
};

bool isCompanionJumpConsumer(OpCode op) {
    return op == OpCode::TEST || op == OpCode::TESTSET || op == OpCode::TFORLOOP;
}

bool isComparisonOpcode(OpCode op) {
    return op == OpCode::EQ || op == OpCode::LT || op == OpCode::LE;
}

i32 nextPc(usize pc) {
    return static_cast<i32>(pc) + 1;
}

i32 jumpTarget(usize pc, Instruction inst) {
    return static_cast<i32>(pc) + 1 + GETARG_sBx(inst);
}

bool hasCompanionJump(const Vec<Instruction>& code, usize pc) {
    return pc + 1 < code.size() && GET_OPCODE(code[pc + 1]) == OpCode::JMP;
}

void addLeader(Vec<bool>& leaders, i32 pc) {
    if (pc >= 0 && static_cast<usize>(pc) < leaders.size()) {
        leaders[static_cast<usize>(pc)] = true;
    }
}

Vec<bool> collectCfgLeaders(const Vec<Instruction>& code) {
    Vec<bool> leaders(code.size(), false);
    if (code.empty()) {
        return leaders;
    }

    leaders[0] = true;

    for (usize pc = 0; pc < code.size(); ++pc) {
        const Instruction inst = code[pc];
        const OpCode op = GET_OPCODE(inst);

        if (isCompanionJumpConsumer(op) && hasCompanionJump(code, pc)) {
            const usize companionPc = pc + 1;
            addLeader(leaders, jumpTarget(companionPc, code[companionPc]));
            addLeader(leaders, static_cast<i32>(pc) + 2);
            continue;
        }

        if (isComparisonOpcode(op)) {
            addLeader(leaders, nextPc(pc));
            addLeader(leaders, static_cast<i32>(pc) + 2);
            continue;
        }

        switch (op) {
        case OpCode::JMP:
            addLeader(leaders, jumpTarget(pc, inst));
            addLeader(leaders, nextPc(pc));
            break;
        case OpCode::LOADBOOL:
            if (GETARG_C(inst) != 0) {
                addLeader(leaders, nextPc(pc));
                addLeader(leaders, static_cast<i32>(pc) + 2);
            }
            break;
        case OpCode::RETURN:
        case OpCode::TAILCALL:
            addLeader(leaders, nextPc(pc));
            break;
        case OpCode::FORPREP:
            addLeader(leaders, jumpTarget(pc, inst));
            addLeader(leaders, nextPc(pc));
            break;
        case OpCode::FORLOOP:
            addLeader(leaders, jumpTarget(pc, inst));
            addLeader(leaders, nextPc(pc));
            break;
        default:
            break;
        }
    }

    return leaders;
}

Vec<CfgBlock> buildCfgBlocks(const Vec<Instruction>& code, const Vec<bool>& leaders) {
    Vec<usize> leaderPcs;
    for (usize pc = 0; pc < leaders.size(); ++pc) {
        if (leaders[pc]) {
            leaderPcs.push_back(pc);
        }
    }

    Vec<CfgBlock> blocks;
    blocks.reserve(leaderPcs.size());
    for (usize i = 0; i < leaderPcs.size(); ++i) {
        const usize start = leaderPcs[i];
        const usize nextLeader = (i + 1 < leaderPcs.size()) ? leaderPcs[i + 1] : code.size();
        if (start >= code.size() || nextLeader == 0 || nextLeader <= start) {
            continue;
        }

        blocks.push_back(CfgBlock{
            blocks.size(),
            start,
            nextLeader - 1,
        });
    }

    return blocks;
}

Vec<i32> buildPcToBlock(const Vec<Instruction>& code, const Vec<CfgBlock>& blocks) {
    Vec<i32> pcToBlock(code.size(), -1);
    for (const CfgBlock& block : blocks) {
        for (usize pc = block.startPc; pc <= block.endPc && pc < pcToBlock.size(); ++pc) {
            pcToBlock[pc] = static_cast<i32>(block.id);
        }
    }
    return pcToBlock;
}

void addCfgEdge(Vec<CfgEdge>& edges, usize fromBlock, i32 targetPc, Str label) {
    edges.push_back(CfgEdge{fromBlock, targetPc, std::move(label)});
}

bool blockEndsWithCompanionJump(const Vec<Instruction>& code, const CfgBlock& block) {
    if (block.endPc == 0 || block.endPc <= block.startPc || block.endPc >= code.size()) {
        return false;
    }

    return isCompanionJumpConsumer(GET_OPCODE(code[block.endPc - 1])) && GET_OPCODE(code[block.endPc]) == OpCode::JMP;
}

Vec<CfgEdge> buildCfgEdges(const Vec<Instruction>& code, const Vec<CfgBlock>& blocks) {
    Vec<CfgEdge> edges;

    for (const CfgBlock& block : blocks) {
        if (block.endPc >= code.size()) {
            continue;
        }

        if (blockEndsWithCompanionJump(code, block)) {
            const OpCode op = GET_OPCODE(code[block.endPc - 1]);
            const i32 jump = jumpTarget(block.endPc, code[block.endPc]);
            const i32 fallthrough = static_cast<i32>(block.endPc) + 1;

            if (op == OpCode::TFORLOOP) {
                addCfgEdge(edges, block.id, jump, "iterator next");
                addCfgEdge(edges, block.id, fallthrough, "iterator done");
            } else {
                addCfgEdge(edges, block.id, jump, "test jump");
                addCfgEdge(edges, block.id, fallthrough, "test fallthrough");
            }
            continue;
        }

        const Instruction inst = code[block.endPc];
        const OpCode op = GET_OPCODE(inst);

        if (isComparisonOpcode(op)) {
            addCfgEdge(edges, block.id, nextPc(block.endPc), "compare next");
            addCfgEdge(edges, block.id, static_cast<i32>(block.endPc) + 2, "compare skip");
            continue;
        }

        switch (op) {
        case OpCode::JMP:
            addCfgEdge(edges, block.id, jumpTarget(block.endPc, inst), "jump");
            break;
        case OpCode::LOADBOOL:
            if (GETARG_C(inst) != 0) {
                addCfgEdge(edges, block.id, static_cast<i32>(block.endPc) + 2, "skip");
            } else {
                addCfgEdge(edges, block.id, nextPc(block.endPc), "fallthrough");
            }
            break;
        case OpCode::RETURN:
        case OpCode::TAILCALL:
            addCfgEdge(edges, block.id, static_cast<i32>(code.size()), "return");
            break;
        case OpCode::FORPREP:
            addCfgEdge(edges, block.id, jumpTarget(block.endPc, inst), "prepare");
            break;
        case OpCode::FORLOOP:
            addCfgEdge(edges, block.id, jumpTarget(block.endPc, inst), "loop");
            addCfgEdge(edges, block.id, nextPc(block.endPc), "exit");
            break;
        default:
            addCfgEdge(edges, block.id, nextPc(block.endPc), "fallthrough");
            break;
        }
    }

    return edges;
}

CfgGraph buildCfgGraph(const Proto* proto) {
    Vec<Instruction> code;
    if (proto) {
        const auto span = proto->getInstructionSpan();
        code.assign(span.begin(), span.end());
    }

    const Vec<bool> leaders = collectCfgLeaders(code);
    Vec<CfgBlock> blocks = buildCfgBlocks(code, leaders);
    Vec<i32> pcToBlock = buildPcToBlock(code, blocks);
    Vec<CfgEdge> edges = buildCfgEdges(code, blocks);
    return CfgGraph{std::move(blocks), std::move(edges), std::move(pcToBlock)};
}

Str escapeMermaidLabel(StrView value) {
    Str escaped;
    escaped.reserve(value.size());

    for (char ch : value) {
        switch (ch) {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            break;
        default:
            escaped += ch;
            break;
        }
    }

    return escaped;
}

Str cfgNodeId(usize protoId, usize blockId) {
    return std::format("P{}_B{}", protoId, blockId);
}

Str cfgExitNodeId(usize protoId) {
    return std::format("P{}_EXIT", protoId);
}

Str cfgBlockPcRange(const CfgBlock& block) {
    if (block.startPc == block.endPc) {
        return std::format("pc {}", block.startPc);
    }

    return std::format("pc {}..{}", block.startPc, block.endPc);
}

Str cfgBlockOpcodeSummary(const Proto* proto, const CfgBlock& block) {
    const auto code = proto->getInstructionSpan();
    StrView first = opcodeMetadata(GET_OPCODE(code[block.startPc])).name;
    StrView last = opcodeMetadata(GET_OPCODE(code[block.endPc])).name;

    if (block.startPc == block.endPc) {
        return Str(first);
    }

    return std::format("{} -> {}", first, last);
}

Str cfgBlockLabel(const Proto* proto, const CfgBlock& block) {
    return std::format("B{}\n{}\n{}", block.id, cfgBlockPcRange(block), cfgBlockOpcodeSummary(proto, block));
}

void printCfgEdge(const CfgGraph& graph, usize protoId, const CfgEdge& edge, std::ostream& out) {
    const Str from = cfgNodeId(protoId, edge.fromBlock);
    Str to = cfgExitNodeId(protoId);

    if (edge.targetPc >= 0 && static_cast<usize>(edge.targetPc) < graph.pcToBlock.size()) {
        const i32 blockId = graph.pcToBlock[static_cast<usize>(edge.targetPc)];
        if (blockId >= 0) {
            to = cfgNodeId(protoId, static_cast<usize>(blockId));
        }
    }

    out << "  " << from << " -->|" << edge.label << "| " << to << '\n';
}

void printSingleProtoCfg(const Proto* proto, std::ostream& out, usize protoId, StrView title) {
    out << std::format("  subgraph P{}[\"{}\"]\n", protoId, escapeMermaidLabel(title));

    if (!proto) {
        out << std::format("    {}((\"exit\"))\n", cfgExitNodeId(protoId));
        out << "  end" << '\n';
        return;
    }

    const CfgGraph graph = buildCfgGraph(proto);
    out << std::format("    {}((\"exit\"))\n", cfgExitNodeId(protoId));
    for (const CfgBlock& block : graph.blocks) {
        out << std::format("    {}[\"{}\"]\n", cfgNodeId(protoId, block.id),
                           escapeMermaidLabel(cfgBlockLabel(proto, block)));
    }
    out << "  end" << '\n';

    for (const CfgEdge& edge : graph.edges) {
        printCfgEdge(graph, protoId, edge, out);
    }
}

void printProtoCfgRecursive(const Proto* proto, std::ostream& out, bool full, CfgRenderState& state, StrView title,
                            const Vec<const Proto*>& ancestry) {
    const usize protoId = state.nextProtoId++;
    printSingleProtoCfg(proto, out, protoId, title);

    if (!full || !proto) {
        return;
    }

    Vec<const Proto*> nextAncestry = ancestry;
    nextAncestry.push_back(proto);

    for (usize i = 0; i < proto->getSubProtoCount(); ++i) {
        const Proto* child = proto->getSubProto(i);
        if (child && containsProto(nextAncestry, child)) {
            out << std::format("  %% proto[{}] skipped: cycle\n", i);
            continue;
        }

        printProtoCfgRecursive(child, out, true, state, std::format("proto[{}] {}", i, formatProtoSummary(child)),
                               nextAncestry);
    }
}

} // namespace

void printProtoBytecode(const Proto* f, std::ostream& out, bool full) {
    printProtoBytecodeRecursive(f, out, full, 0, {});
}

void printProtoBytecodeDiff(const Proto* left, const Proto* right, std::ostream& out, bool full, StrView leftLabel,
                            StrView rightLabel) {
    Vec<Str> leftLines = removeDiffNoise(splitLines(renderProtoBytecode(left, full)));
    Vec<Str> rightLines = removeDiffNoise(splitLines(renderProtoBytecode(right, full)));
    const usize changed = countChangedLines(leftLines, rightLines);

    out << "Bytecode diff" << '\n';
    out << "  left: " << leftLabel << '\n';
    out << "  right: " << rightLabel << '\n';
    out << "  mode: " << (full ? "full" : "compact") << '\n';
    out << "  status: " << (changed == 0 ? "identical" : "different") << '\n';
    out << "  changed lines: " << changed << '\n';

    if (changed == 0) {
        return;
    }

    out << "line | left | right" << '\n';

    const usize lineCount = std::max(leftLines.size(), rightLines.size());
    for (usize i = 0; i < lineCount; ++i) {
        const bool hasLeft = i < leftLines.size();
        const bool hasRight = i < rightLines.size();
        const StrView leftLine = hasLeft ? StrView(leftLines[i]) : StrView("<missing>");
        const StrView rightLine = hasRight ? StrView(rightLines[i]) : StrView("<missing>");

        if (hasLeft && hasRight && leftLine == rightLine) {
            continue;
        }

        out << std::format("{:04} | {} | {}\n", i, leftLine, rightLine);
    }
}

void printProtoBytecodeCfg(const Proto* f, std::ostream& out, bool full) {
    out << "flowchart TD" << '\n';
    out << "  %% lua_bytecode Mermaid CFG" << '\n';

    CfgRenderState state;
    printProtoCfgRecursive(f, out, full, state, std::format("Proto {}", formatProtoSummary(f)), {});
}

} // namespace Lua
