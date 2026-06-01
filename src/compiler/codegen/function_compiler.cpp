/**
 * @file function_compiler.cpp
 * @brief Function prototype compilation boundary implementation.
 */

#include "compiler/codegen/function_compiler.hpp"
#include "compiler/codegen/codegen.hpp"
#include "compiler/codegen/codegen_types.hpp"
#include "compiler/codegen/gc_allocation_guard.hpp"

namespace Lua {

namespace {

bool exprUsesDirectVararg(const Expr& expr);
bool stmtUsesDirectVararg(const Stmt& stmt);

bool exprListUsesDirectVararg(const Vec<ExprPtr>& exprs) {
    for (const auto& expr : exprs) {
        if (expr && exprUsesDirectVararg(*expr)) {
            return true;
        }
    }
    return false;
}

bool stmtListUsesDirectVararg(const Vec<StmtPtr>& stmts) {
    for (const auto& stmt : stmts) {
        if (stmt && stmtUsesDirectVararg(*stmt)) {
            return true;
        }
    }
    return false;
}

bool tableFieldsUseDirectVararg(const Vec<TableField>& fields) {
    for (const TableField& field : fields) {
        if ((field.key && exprUsesDirectVararg(*field.key)) ||
            (field.value && exprUsesDirectVararg(*field.value))) {
            return true;
        }
    }
    return false;
}

bool exprUsesDirectVararg(const Expr& expr) {
    return std::visit(ValueResultVisitor{
        [](const VarargExpr&) { return true; },
        [](const NilExpr&) { return false; },
        [](const BoolExpr&) { return false; },
        [](const NumberExpr&) { return false; },
        [](const StringExpr&) { return false; },
        [](const NameExpr&) { return false; },
        [](const FunctionExpr&) { return false; },
        [](const BinaryExpr& e) {
            return (e.left && exprUsesDirectVararg(*e.left)) ||
                   (e.right && exprUsesDirectVararg(*e.right));
        },
        [](const UnaryExpr& e) {
            return e.operand && exprUsesDirectVararg(*e.operand);
        },
        [](const TableExpr& e) {
            return tableFieldsUseDirectVararg(e.fields);
        },
        [](const CallExpr& e) {
            return (e.func && exprUsesDirectVararg(*e.func)) ||
                   exprListUsesDirectVararg(e.args);
        },
        [](const IndexExpr& e) {
            return (e.table && exprUsesDirectVararg(*e.table)) ||
                   (e.index && exprUsesDirectVararg(*e.index));
        },
        [](const MemberExpr& e) {
            return e.table && exprUsesDirectVararg(*e.table);
        },
        [](const ParenExpr& e) {
            return e.expression && exprUsesDirectVararg(*e.expression);
        },
    }, expr.variant);
}

bool stmtUsesDirectVararg(const Stmt& stmt) {
    return std::visit(ValueResultVisitor{
        [](const EmptyStmt&) { return false; },
        [](const BreakStmt&) { return false; },
        [](const FunctionStmt&) { return false; },
        [](const AssignStmt& s) {
            return exprListUsesDirectVararg(s.targets) || exprListUsesDirectVararg(s.values);
        },
        [](const LocalStmt& s) {
            return exprListUsesDirectVararg(s.values);
        },
        [](const CallStmt& s) {
            return s.call && exprUsesDirectVararg(*s.call);
        },
        [](const IfStmt& s) {
            for (const IfStmt::Branch& branch : s.branches) {
                if ((branch.condition && exprUsesDirectVararg(*branch.condition)) ||
                    stmtListUsesDirectVararg(branch.body)) {
                    return true;
                }
            }
            return stmtListUsesDirectVararg(s.elseBranch);
        },
        [](const WhileStmt& s) {
            return (s.condition && exprUsesDirectVararg(*s.condition)) ||
                   stmtListUsesDirectVararg(s.body);
        },
        [](const RepeatStmt& s) {
            return stmtListUsesDirectVararg(s.body) ||
                   (s.condition && exprUsesDirectVararg(*s.condition));
        },
        [](const ForNumStmt& s) {
            return (s.init && exprUsesDirectVararg(*s.init)) ||
                   (s.limit && exprUsesDirectVararg(*s.limit)) ||
                   (s.step && exprUsesDirectVararg(*s.step)) ||
                   stmtListUsesDirectVararg(s.body);
        },
        [](const ForInStmt& s) {
            return exprListUsesDirectVararg(s.iterators) ||
                   stmtListUsesDirectVararg(s.body);
        },
        [](const ReturnStmt& s) {
            return exprListUsesDirectVararg(s.values);
        },
        [](const DoStmt& s) {
            return stmtListUsesDirectVararg(s.body);
        },
    }, stmt.variant);
}

}  // namespace

CompiledFunction FunctionCompiler::compile(const Vec<Str>& params, bool isVararg, const Vec<StmtPtr>& body,
                                           i32 linedefined, i32 lastlinedefined) {
    CodeGenerator child(owner_.state_.services);
    child.state_.parent = &owner_;

    const bool needsCompatArg = isVararg && !stmtListUsesDirectVararg(body);
    u8 varargFlags = 0;
    if (isVararg) {
        varargFlags = VARARG_ISVARARG;
        if (needsCompatArg) {
            varargFlags |= VARARG_HASARG | VARARG_NEEDSARG;
        }
    }

    GCAllocationGuard<Proto> protoGuard(owner_.state_.services.gc);
    Proto* newProto = protoGuard.get();
    newProto->setNumParams(static_cast<u8>(params.size()));
    newProto->setLineDefined(linedefined);
    newProto->setLastLineDefined(lastlinedefined);

    if (owner_.state_.proto != nullptr) {
        newProto->setSource(owner_.state_.proto->getSource());
    }

    child.state_.resetForProto(*newProto, isVararg);
    newProto->setVarargFlags(varargFlags);
    child.state_.currentLine = linedefined;

    for (const Str& param : params) {
        child.scopes_.addLocalVar(param);
    }
    child.scopes_.adjustLocalVars(static_cast<i32>(params.size()));

    if (needsCompatArg) {
        child.scopes_.addLocalVar("arg");
        child.scopes_.adjustLocalVars(1);
    }

    child.statements_.block(body);

    {
        i32 savedLine = child.state_.currentLine;
        child.state_.currentLine = lastlinedefined > 0 ? lastlinedefined : linedefined;
        child.codeABC(OpCode::RETURN, 0, 1, 0);
        child.state_.currentLine = savedLine;
    }

    newProto->setNumUpvalues(static_cast<u8>(child.scopes_.upvalues().size()));
    for (const UpvalueCapture& uv : child.scopes_.upvalues()) {
        newProto->addUpvalueName(owner_.state_.pool->intern(uv.name));
    }

    child.functions_.attachDebugMetadata();

    if (static_cast<i32>(newProto->getMaxStackSize()) < child.state_.registers.current()) {
        newProto->setMaxStackSize(static_cast<u8>(child.state_.registers.current()));
    }

    CompiledFunction compiled;
    compiled.upvalues = child.scopes_.upvalues();
    compiled.protoIndex = owner_.state_.bytecode.addSubProto(newProto);
    compiled.proto = protoGuard.commit();
    return compiled;
}

void FunctionCompiler::emitClosureUpvalues(const Vec<UpvalueCapture>& upvalues) {
    for (const UpvalueCapture& uv : upvalues) {
        if (uv.inStack) {
            owner_.codeABC(OpCode::MOVE, 0, uv.index, 0);
        } else {
            owner_.codeABC(OpCode::GETUPVAL, 0, uv.index, 0);
        }
    }
}

void FunctionCompiler::attachDebugMetadata() {
    if (owner_.state_.proto == nullptr) {
        return;
    }

    for (const LocalVar& local : owner_.scopes_.localVars()) {
        i32 endpc = local.endpc >= 0
            ? local.endpc
            : owner_.state_.bytecode.instructionCount();
        owner_.state_.bytecode.addLocalDebug(local.name, local.startpc, endpc, local.reg);
    }
}

}  // namespace Lua
