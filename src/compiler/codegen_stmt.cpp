/**
 * @file codegen_stmt.cpp
 * @brief CodeGenerator statement lowering, function compilation, and block management.
 */

#include "compiler/codegen.hpp"

#include <stdexcept>

namespace Lua {

// 辅助函数：获取语句块的最后一行号
static i32 getLastLineOfBlock(const Vec<StmtPtr>& body) {
    if (body.empty()) {
        return 0;
    }
    return body.back()->getLine();
}

// =====================================================================
// 语句代码生成（简化版）
// =====================================================================

void CodeGenerator::statement(const Stmt& s) {
    i32 previousLine = currentLine_;
    i32 stmtLine = s.getLine();
    if (stmtLine > 0) {
        currentLine_ = stmtLine;
    }
    std::visit([this](auto&& arg) {
        emitStmt(arg);
    }, s.variant);
    currentLine_ = previousLine;
}

void CodeGenerator::emitStmt(const EmptyStmt&) {
    // 空语句，不生成代码
}

void CodeGenerator::emitStmt(const AssignStmt& s) {
    i32 nvars = static_cast<i32>(s.targets.size());
    i32 nexps = static_cast<i32>(s.values.size());

    // 先处理除最后一个之外的右值表达式（每个表达式固定对应一个左值）
    for (i32 i = 0; i < nexps - 1 && i < nvars; i++) {
        ValueResult val = emitValue(*s.values[i]);
        val = forceSingleValue(val);
        LValueRef target = emitLValue(*s.targets[i]);
        emitStore(target, val);
    }

    // 处理最后一个右值表达式（可能是多返回值表达式）
    if (nexps > 0 && nexps <= nvars) {
        i32 targetIndex = nexps - 1;
        const Expr& lastExpr = *s.values[targetIndex];
        i32 wanted = nvars - targetIndex;

        if (auto* callExpr = std::get_if<CallExpr>(&lastExpr.variant)) {
            CallResultInfo callResult = emitCallExpr(*callExpr);
            setWantedResults(callResult, wanted);
            i32 valueBase = callResult.baseReg;

            for (i32 j = 0; j < wanted; j++) {
                LValueRef target = emitLValue(*s.targets[targetIndex + j]);
                ValueResult tmp;
                tmp.kind = ValueResult::Kind::Register;
                tmp.reg = valueBase + j;
                emitStore(target, tmp);
            }
            return;
        }
        else if (std::holds_alternative<VarargExpr>(lastExpr.variant)) {
            CallResultInfo callResult = emitVarargExpr();
            Instruction inst = proto_->getInstruction(callResult.instructionPc);
            SETARG_B(inst, wanted + 1);
            proto_->setInstruction(callResult.instructionPc, inst);
            i32 valueBase = GETARG_A(inst);

            for (i32 j = 0; j < wanted; j++) {
                LValueRef target = emitLValue(*s.targets[targetIndex + j]);
                ValueResult tmp;
                tmp.kind = ValueResult::Kind::Register;
                tmp.reg = valueBase + j;
                emitStore(target, tmp);
            }
            return;
        }
        else {
            ValueResult val = emitValue(lastExpr);
            val = forceSingleValue(val);
            LValueRef target = emitLValue(*s.targets[targetIndex]);
            emitStore(target, val);
        }
    }

    // 如果变量多于值，剩余变量赋值为 nil
    for (i32 i = nexps; i < nvars; i++) {
        LValueRef target = emitLValue(*s.targets[i]);
        ValueResult nilVal;
        nilVal.kind = ValueResult::Kind::Immediate;
        nilVal.immediate = ValueResult::ImmediateKind::Nil;
        emitStore(target, nilVal);
    }
}

void CodeGenerator::emitStmt(const LocalStmt& s) {
    i32 nvars = static_cast<i32>(s.names.size());
    i32 nexps = static_cast<i32>(s.values.size());

    // ⭐ 关键修复：保存 locals_.nactvar_ 的初始值（第一个变量的寄存器索引）
    i32 base = locals_.nactvar_;

    //std::fprintf(stderr, "DEBUG LocalStmt: nvars=%d, nexps=%d, base=%d, freereg=%d\n",
    //             nvars, nexps, base, regs_.current());

    // 保存当前寄存器状态，表达式求值将从 base 开始分配寄存器
    i32 savedFreereg = regs_.current();

    // 将临时分配指针对齐到第一个局部变量槽位。
    regs_.setFreeReg(base);

    // 为每个变量分配寄存器。
    for (i32 i = 0; i < nvars; i++) {
        addLocalVar(s.names[i]);
    }

    //std::fprintf(stderr, "DEBUG LocalStmt: after addLocalVar, freereg=%d\n", regs_.current());

    // 重置寄存器基址为 base，确保后续分配从 base 开始
    regs_.setFreeReg(base);

    //std::fprintf(stderr, "DEBUG LocalStmt: reset freereg to %d\n", regs_.current());

    // 生成初始化代码
    bool allVarsInitialized = false;  // 标记是否所有变量都已初始化
    if (nexps > 0) {
        // 处理前 nexps-1 个表达式（每个表达式对应一个变量）
        for (i32 i = 0; i < nexps - 1 && i < nvars; i++) {
            ValueResult val = emitValue(*s.values[i]);
            val = forceSingleValue(val);
            materializeValue(val, base + i);
        }

        // 处理最后一个表达式（可能是多返回值表达式）
        if (nexps <= nvars) {
            const Expr& lastExpr = *s.values[nexps - 1];
            i32 wanted = nvars - (nexps - 1);
            i32 targetReg = base + (nexps - 1);

            // PR-5: 从 AST 直接检测 Call/Vararg，使用原生 CallResultInfo 通道
            if (auto* callExpr = std::get_if<CallExpr>(&lastExpr.variant)) {
                CallResultInfo callResult = emitCallExpr(*callExpr);
                setWantedResults(callResult, wanted);

                i32 callBase = callResult.baseReg;
                if (targetReg != callBase) {
                    // 将返回值从 callBase... 搬到 targetReg...
                    // 注意重叠区：target 在右侧时需逆序拷贝，避免覆盖源值。
                    if (targetReg > callBase) {
                        for (i32 j = wanted - 1; j >= 0; --j) {
                            codeABC(OpCode::MOVE, targetReg + j, callBase + j, 0);
                        }
                    } else {
                        for (i32 j = 0; j < wanted; ++j) {
                            codeABC(OpCode::MOVE, targetReg + j, callBase + j, 0);
                        }
                    }
                }
                allVarsInitialized = true;
            }
            else if (std::holds_alternative<VarargExpr>(lastExpr.variant)) {
                CallResultInfo callResult = emitVarargExpr();
                Instruction inst = proto_->getInstruction(callResult.instructionPc);
                SETARG_A(inst, targetReg);
                SETARG_B(inst, wanted + 1);
                proto_->setInstruction(callResult.instructionPc, inst);
                allVarsInitialized = true;
            }
            else {
                // 普通表达式
                ValueResult val = emitValue(lastExpr);
                val = forceSingleValue(val);
                materializeValue(val, base + (nexps - 1));
            }
        }
    }

    // 未初始化的变量设为nil
    // ⭐ 关键修复：如果最后一个表达式是多返回值表达式（Vararg/Call），
    // 它已经初始化了所有剩余变量，不需要再生成 LOADNIL
    if (nexps < nvars && !allVarsInitialized) {
        codeABC(OpCode::LOADNIL, base + nexps, base + nvars - 1, 0);  // ⭐ 使用 base 而不是 locals_.nactvar_
    }

    // 恢复寄存器状态
    regs_.restore(savedFreereg);

    adjustLocalVars(nvars);
}

void CodeGenerator::emitStmt(const ReturnStmt& s) {
    // 返回语句 — PR-5: 支持 return f() / return ... 的开放 multret 传播
    i32 nret = static_cast<i32>(s.values.size());
    if (nret == 0) {
        codeABC(OpCode::RETURN, 0, 1, 0);
    } else {
        i32 base = locals_.nactvar_;
        i32 savedFreereg = regs_.current();
        regs_.setFreeReg(base);
        checkStack(nret);

        // 处理前 nret-1 个值（每个固定为单值）
        for (i32 i = 0; i < nret - 1; i++) {
            ValueResult val = emitValue(*s.values[i]);
            val = forceSingleValue(val);
            materializeValue(val, base + i);
        }

        // 确保下一个空闲寄存器指向最后一个值应落的位置。
        regs_.setFreeReg(base + (nret - 1));

        // 处理最后一个值 — 可能是 Call/Vararg 开放多返回
        const Expr& lastExpr = *s.values[nret - 1];
        if (auto* callExpr = std::get_if<CallExpr>(&lastExpr.variant)) {
            // return ..., f() — 保持 multret 传播
            CallResultInfo info = emitCallExpr(*callExpr, base + (nret - 1));
            setOpenMultiRet(info);
            // emitCallExpr 保证 callBase >= base + nret - 1
            // 如果 callBase 恰好等于 base + (nret - 1)，完美对齐
            if (info.baseReg == base + (nret - 1)) {
                if (nret == 1) {
                    Instruction inst = proto_->getInstruction(info.instructionPc);
                    inst = CREATE_ABC(OpCode::TAILCALL, GETARG_A(inst), GETARG_B(inst), 0);
                    proto_->setInstruction(info.instructionPc, inst);
                }
                codeABC(OpCode::RETURN, base, 0, 0);  // B=0 → 从 base 到栈顶
            } else {
                // callBase 在更高位置（嵌套调用保护发生了搬移），
                // 无法 MOVE multret，退化为单值
                Instruction inst = proto_->getInstruction(info.instructionPc);
                SETARG_C(inst, 2);  // 恢复为单值 C=2
                proto_->setInstruction(info.instructionPc, inst);
                codeABC(OpCode::MOVE, base + (nret - 1), info.baseReg, 0);
                codeABC(OpCode::RETURN, base, nret + 1, 0);
            }
            regs_.restore(savedFreereg);
            return;
        }
        else if (std::holds_alternative<VarargExpr>(lastExpr.variant)) {
            // return ..., ... — 保持 multret 传播
            CallResultInfo info = emitVarargExpr();
            Instruction inst = proto_->getInstruction(info.instructionPc);
            SETARG_A(inst, base + (nret - 1));
            SETARG_B(inst, 0);  // B=0 → 复制全部 vararg
            proto_->setInstruction(info.instructionPc, inst);
            codeABC(OpCode::RETURN, base, 0, 0);  // B=0 → 返回到栈顶
            regs_.restore(savedFreereg);
            return;
        }
        else {
            // 普通最后一个值
            ValueResult val = emitValue(lastExpr);
            val = forceSingleValue(val);
            materializeValue(val, base + (nret - 1));
        }

        codeABC(OpCode::RETURN, base, nret + 1, 0);
        regs_.restore(savedFreereg);
    }
}

void CodeGenerator::emitStmt(const IfStmt& s) {
    if (s.branches.empty()) {
        return;
    }

    PatchList escapelist;
    PatchList flist;

    // first if branch
    {
        const auto& branch = s.branches[0];
        CondResult cond = emitCondResult(*branch.condition);
        flist = cond.falseList;
        block(branch.body);
    }

    // elseif branches
    for (size_t i = 1; i < s.branches.size(); i++) {
        escapelist.append(jump());
        patchtohere(flist);

        const auto& branch = s.branches[i];
        CondResult cond = emitCondResult(*branch.condition);
        flist = cond.falseList;
        block(branch.body);
    }

    // else block
    if (!s.elseBranch.empty()) {
        escapelist.append(jump());
        patchtohere(flist);
        block(s.elseBranch);
    } else {
        escapelist.append(flist);
    }

    patchtohere(escapelist);
}

void CodeGenerator::emitStmt(const WhileStmt& s) {
    i32 whileinit = getLabel();

    // 生成条件表达式，返回假值跳转列表
    CondResult cond = emitCondResult(*s.condition);

    // 进入可break的代码块
    enterBlock(true);

    block(s.body);

    // 生成回跳到循环开始
    patchList(jump(), whileinit);

    // 离开代码块，修补所有break跳转
    leaveBlock();

    // 修补条件为假时的跳转到循环出口
    patchtohere(cond.falseList);
}

void CodeGenerator::emitStmt(const DoStmt& s) {
    // do块
    block(s.body);
}

void CodeGenerator::emitStmt(const CallStmt& s) {
    // PR-5: 直接使用 emitCallExpr，设置 C=1 丢弃所有返回值
    const Expr& callExpr = *s.call;
    if (auto* ce = std::get_if<CallExpr>(&callExpr.variant)) {
        CallResultInfo info = emitCallExpr(*ce);
        setWantedResults(info, 0);  // C=1 → 0 个返回值
        freeReg(info.baseReg);
    } else {
        // 回退：非 CallExpr（理论上 parser 不会生成此情况）
        emitValue(callExpr);
    }

    // 语句级函数调用不会跨语句保留临时寄存器。
    regs_.resetToLocals(locals_.nactvar_);
}

void CodeGenerator::emitStmt(const BreakStmt&) {
    // 查找最近的可 break 代码块
    BlockInfo* bl = blocks_.currentBlock_;
    while (bl && !bl->isbreakable) {
        bl = bl->previous;
    }

    // 如果没有找到可break的代码块，报错
    if (!bl) {
        throw std::runtime_error("no loop to break");
    }

    closeScopeUpvalues(bl->nactvar);

    // 生成跳转指令并添加到break列表
    concatJumpList(bl->breaklist, jump());
}

void CodeGenerator::emitStmt(const RepeatStmt& s) {
    // repeat body until condition
    //
    // 关键语义：body 中声明的局部变量在 until 条件中仍然可见，
    // 因此不能使用 block()（它会在结束时移除局部变量）。
    // Lua 5.1 使用两层 block（loop + scope）处理此问题；
    // 当前实现用单层 loop block + 手动延迟 removeLocalVars 替代。
    // 后续补全 CLOSE 指令支持时再引入 scope block 处理 upvalue 关闭。

    i32 repeat_init = getLabel();

    // 进入可 break 的循环块
    enterBlock(true);  // isbreakable = true

    // 记录循环体前的局部变量数量
    i32 body_nactvar = locals_.nactvar_;

    // 生成循环体（不使用 block()，避免提前移除局部变量）
    for (const auto& stmt : s.body) {
        statement(*stmt);
    }

    // 生成条件表达式（此时 body 局部变量仍然可见）
    CondResult cond = emitCondResult(*s.condition);

    // 条件求值完毕，移除 body 中声明的局部变量
    removeLocalVars(body_nactvar);

    // 条件为假 → 跳回循环开始
    patchList(cond.falseList, repeat_init);

    // 离开循环块，修补所有 break 跳转到当前位置
    leaveBlock();
}
// =====================================================================
// 函数定义和调用
// =====================================================================
Proto* CodeGenerator::compileFunction(const Vec<Str>& params, bool isVararg, const Vec<StmtPtr>& body,
                                     i32 linedefined, i32 lastlinedefined,
                                     Vec<UpvalueCapture>* outUpvalues) {
    // 使用独立子生成器编译子函数，保留父函数上下文以解析upvalue
    CodeGenerator child(services_);
    child.parent_ = this;

    Proto* newProto = new Proto();
    services_.gc.registerObject(newProto);
    newProto->setNumParams(static_cast<u8>(params.size()));
    newProto->setVararg(isVararg);
    newProto->setLineDefined(linedefined);
    newProto->setLastLineDefined(lastlinedefined);

    // 继承父Proto的源文件名
    if (proto_ != nullptr) {
        newProto->setSource(proto_->getSource());
    }

    child.proto_ = newProto;
    child.regs_.bind(newProto);
    child.regs_.setFreeReg(0);
    child.locals_.nactvar_ = 0;
    child.locals_.localVars_.clear();
    child.upvalueCtx_.upvalues_.clear();
    child.pc_ = 0;
    child.blocks_.jpc_ = NO_JUMP;
    child.blocks_.currentBlock_ = nullptr;
    child.currentLine_ = linedefined;

    // 添加参数作为局部变量
    for (const Str& param : params) {
        child.addLocalVar(param);
    }
    child.adjustLocalVars(static_cast<i32>(params.size()));

    // 编译函数体
    child.block(body);

    // 添加隐式return（如果函数体没有显式return）
    if (newProto->getInstructionCount() == 0 ||
        GET_OPCODE(newProto->getInstruction(newProto->getInstructionCount() - 1)) != OpCode::RETURN) {
        child.codeABC(OpCode::RETURN, 0, 1, 0);
    }

    // 写入upvalue元信息（数量 + 名称）
    newProto->setNumUpvalues(static_cast<u8>(child.upvalueCtx_.upvalues_.size()));
    for (const UpvalueCapture& uv : child.upvalueCtx_.upvalues_) {
        newProto->addUpvalueName(pool_->intern(uv.name));
    }

    child.attachDebugMetadata();

    // 设置最大栈大小（只增不减）
    if (static_cast<i32>(newProto->getMaxStackSize()) < child.regs_.current()) {
        newProto->setMaxStackSize(static_cast<u8>(child.regs_.current()));
    }

    if (outUpvalues != nullptr) {
        *outUpvalues = child.upvalueCtx_.upvalues_;
    }

    return newProto;
}

void CodeGenerator::emitClosureUpvalues(const Vec<UpvalueCapture>& upvalues) {
    // Lua 5.1约定：CLOSURE后紧跟nups条伪指令（MOVE或GETUPVAL）
    for (const UpvalueCapture& uv : upvalues) {
        if (uv.inStack) {
            codeABC(OpCode::MOVE, 0, uv.index, 0);
        } else {
            codeABC(OpCode::GETUPVAL, 0, uv.index, 0);
        }
    }
}


void CodeGenerator::emitStmt(const FunctionStmt& s) {
    // 计算函数定义的行号范围
    i32 linedefined = s.line;
    i32 lastlinedefined = getLastLineOfBlock(s.body);
    if (lastlinedefined < linedefined) {
        lastlinedefined = linedefined;  // 空函数体的情况
    }

    // 编译函数体
    Vec<UpvalueCapture> childUpvalues;
    Proto* funcProto = compileFunction(s.params, s.isVararg, s.body, linedefined, lastlinedefined, &childUpvalues);

    // 将Proto添加到当前Proto的子函数列表
    i32 protoIdx = static_cast<i32>(proto_->addProto(funcProto));

    if (s.isLocal) {
        // 局部函数：local function name() end
        // 先添加局部变量，并使用该变量的真实寄存器承载闭包。
        i32 reg = addLocalVar(s.name);

        // 生成CLOSURE指令
        codeABx(OpCode::CLOSURE, reg, protoIdx);
        emitClosureUpvalues(childUpvalues);

        // 激活局部变量
        adjustLocalVars(1);
    } else {
        // 全局/表成员函数：function name() end / function t.a.b:name() end
        i32 savedFreereg = regs_.current();

        if (s.tablePath.empty()) {
            // 生成CLOSURE指令到临时寄存器
            i32 reg = allocReg();
            codeABx(OpCode::CLOSURE, reg, protoIdx);
            emitClosureUpvalues(childUpvalues);

            // 简单全局函数：_G[name] = closure
            i32 k = stringConstant(s.name);
            codeABx(OpCode::SETGLOBAL, reg, k);
        } else {
            // 表成员函数：tablePath.name = closure
            // 例如：
            // - function t.foo() end      -> t["foo"] = closure
            // - function t.a.b:c() end    -> t["a"]["b"]["c"] = closure（参数已含self）
            auto loadNameToReg = [this](const Str& name) -> i32 {
                SymbolRef sym = resolve(name);
                if (sym.kind == SymbolRef::Kind::Local) {
                    return sym.index;
                }
                i32 reg = allocReg();
                ValueResult val = symbolToValue(sym);
                materializeValue(val, reg);
                return reg;
            };

            // 先取到最外层表（tablePath[0]）
            i32 tableReg = loadNameToReg(s.tablePath[0]);

            // 逐层读取中间字段，定位到最终赋值目标表
            for (usize i = 1; i < s.tablePath.size(); i++) {
                i32 nextReg = allocReg();
                i32 k = stringConstant(s.tablePath[i]);
                codeABC(OpCode::GETTABLE, nextReg, tableReg, RKASK(k));
                tableReg = nextReg;
            }

            // 生成CLOSURE（必须先于upvalue伪指令）
            i32 reg = allocReg();
            codeABx(OpCode::CLOSURE, reg, protoIdx);
            emitClosureUpvalues(childUpvalues);

            // 设置最终字段：tableReg[s.name] = closure
            i32 rkKey = RKASK(stringConstant(s.name));
            codeABC(OpCode::SETTABLE, tableReg, rkKey, reg);
        }

        // 释放本语句使用的临时寄存器（包含closure和路径求值临时寄存器）
        regs_.restore(savedFreereg);
        checkStack(0);
    }
}

void CodeGenerator::emitStmt(const ForNumStmt& s) {
    // 数值for循环的字节码模式：
    // R(base) = init
    // R(base+1) = limit
    // R(base+2) = step
    // FORPREP base sBx    ; R(base) -= step, pc += sBx
    // <loop body>
    // FORLOOP base sBx    ; R(base) += step, if R(base) <= limit then pc += sBx

    i32 base = regs_.current();  // 循环变量的基址

    // 计算init, limit, step并存储到R(base), R(base+1), R(base+2)
    ValueResult initVal = emitValue(*s.init);
    initVal = forceSingleValue(initVal);
    valueToNextReg(initVal);  // R(base)

    ValueResult limitVal = emitValue(*s.limit);
    limitVal = forceSingleValue(limitVal);
    valueToNextReg(limitVal);  // R(base+1)

    if (s.step) {
        ValueResult stepVal = emitValue(*s.step);
        stepVal = forceSingleValue(stepVal);
        valueToNextReg(stepVal);  // R(base+2)
    } else {
        // 默认步长为1
        i32 stepReg = allocReg();
        codeABx(OpCode::LOADK, stepReg, numberConstant(1.0));
    }

    // 进入可break的代码块（在添加循环变量之前）
    enterBlock(true);  // isbreakable = true

    // 注册 3 个内部控制变量和可见循环变量为局部变量（与 Lua 5.1 C 一致）。
    // 这确保 locals_.nactvar_ 包含它们，防止后续语句把临时寄存器指针
    // 重置到控制寄存器区域。init/limit/step 已经落在 R(base)..R(base+2)，
    // 这里先回退分配指针，让 addLocalVar 映射到正确槽位。
    regs_.setFreeReg(base);
    addLocalVar("(for index)");   // R(base)
    addLocalVar("(for limit)");   // R(base+1)
    addLocalVar("(for step)");    // R(base+2)
    addLocalVar(s.var);           // R(base+3) — 可见循环变量
    adjustLocalVars(4);

    // 确保临时寄存器从所有保留寄存器之后开始。
    regs_.setFreeReg(base + 4);
    checkStack(0);

    // 生成FORPREP指令（跳转到FORLOOP）
    i32 prep = codeAsBx(OpCode::FORPREP, base, 0);  // sBx稍后回填

    // 生成循环体
    i32 bodyStart = getLabel();
    block(s.body);

    // 生成FORLOOP指令（跳转回循环开始）
    i32 loop = codeAsBx(OpCode::FORLOOP, base, bodyStart - getLabel() - 1);

    // 回填FORPREP的跳转目标（跳到FORLOOP）
    fixjump(prep, loop);

    // 离开代码块，修补所有break跳转，并移除循环变量
    leaveBlock();

    // 释放寄存器
    freeRegs(4);  // init, limit, step, var
}

void CodeGenerator::emitStmt(const ForInStmt& s) {
    // 泛型for循环的字节码模式：
    // R(base) = iterator_func
    // R(base+1) = state
    // R(base+2) = control_var
    // JMP -> TFORLOOP
    // loop:
    // R(base+3), ..., R(base+3+nvars-1) = loop variables
    // <loop body>
    // TFORLOOP base nvars
    // JMP -> loop

    i32 base = regs_.current();  // 迭代器变量的基址
    i32 nvars = static_cast<i32>(s.vars.size());  // 循环变量数量

    // 计算迭代器表达式（应该返回3个值：func, state, var）
    // 例如：for k, v in pairs(t) do ... end
    // pairs(t) 返回 (next, t, nil)

    if (s.iterators.size() != 1) {
        throw std::runtime_error("CodeGenerator: for-in loop requires exactly 1 iterator expression");
    }

    // 计算迭代器表达式。当前实现要求唯一的迭代表达式直接提供
    // generator/state/control 三元组，因此这里显式消费 Call/Vararg 多返回值通道。
    const Expr& iteratorExpr = *s.iterators[0];
    if (auto* callExpr = std::get_if<CallExpr>(&iteratorExpr.variant)) {
        CallResultInfo info = emitCallExpr(*callExpr, base);
        setWantedResults(info, 3);
        regs_.setFreeReg(base + 3);
        checkStack(0);
    } else if (std::holds_alternative<VarargExpr>(iteratorExpr.variant)) {
        CallResultInfo info = emitVarargExpr();
        Instruction inst = proto_->getInstruction(info.instructionPc);
        SETARG_A(inst, base);
        SETARG_B(inst, 4);  // B=4 -> 3 个结果
        proto_->setInstruction(info.instructionPc, inst);
        regs_.setFreeReg(base + 3);
        checkStack(0);
    } else {
        throw std::runtime_error("CodeGenerator: for-in loop iterator must be a function call or vararg");
    }

    // 进入可break的代码块（在添加循环变量之前）
    enterBlock(true);  // isbreakable = true

    // 注册 3 个内部控制变量为局部变量（与 Lua 5.1 C 一致）。
    // 类似数值 for，先回退分配指针以映射到 R(base)..R(base+2)。
    regs_.setFreeReg(base);
    addLocalVar("(for generator)");  // R(base)
    addLocalVar("(for state)");      // R(base+1)
    addLocalVar("(for control)");    // R(base+2)

    // 添加循环变量作为局部变量（R(base+3), R(base+4), ...）
    for (const Str& var : s.vars) {
        addLocalVar(var);
    }
    adjustLocalVars(3 + nvars);

    // 泛型 for 需要保留：
    // - R(base)   = iterator function
    // - R(base+1) = state
    // - R(base+2) = control variable
    // - R(base+3)... = visible loop variables
    // 循环体临时寄存器从保留区之后开始分配。
    regs_.setFreeReg(base + 3 + nvars);
    checkStack(0);

    // 跳转到TFORLOOP（跳过循环体）
    i32 jmpToTfor = jump();  // 跳到 TFORLOOP，稍后回填

    // 循环体开始
    i32 loopStart = getLabel();
    block(s.body);

    // 回填JMP到TFORLOOP的跳转目标
    patchtohere(jmpToTfor);

    // 生成TFORLOOP指令
    codeABC(OpCode::TFORLOOP, base, 0, nvars);

    // 生成JMP回循环开始
    codeAsBx(OpCode::JMP, 0, loopStart - getLabel() - 1);

    // 离开代码块，修补所有break跳转，并移除循环变量
    leaveBlock();

    // 释放寄存器
    freeRegs(3 + nvars);  // func, state, var, loop_vars
}

void CodeGenerator::block(const Vec<StmtPtr>& stmts) {
    i32 oldnactvar = locals_.nactvar_;

    for (const auto& stmt : stmts) {
        statement(*stmt);
    }

    removeLocalVars(oldnactvar);
}

void CodeGenerator::attachDebugMetadata() {
    if (proto_ == nullptr) {
        return;
    }

    for (const LocalVar& local : locals_.localVars_) {
        i32 endpc = local.endpc >= 0
            ? local.endpc
            : static_cast<i32>(proto_->getInstructionCount());
        proto_->addLocVar(pool_->intern(local.name), local.startpc, endpc, local.reg);
    }
}
// =====================================================================
// 代码块管理
// =====================================================================

void CodeGenerator::enterBlock(bool isbreakable) {
    blocks_.enterBlock(isbreakable, locals_.nactvar_);
}

void CodeGenerator::closeScopeUpvalues(i32 level) {
    if (locals_.nactvar_ <= level) {
        return;
    }

    if (proto_->getInstructionCount() > 0) {
        Instruction last = proto_->getInstruction(proto_->getInstructionCount() - 1);
        if (GET_OPCODE(last) == OpCode::RETURN) {
            return;
        }
    }

    codeABC(OpCode::CLOSE, level, 0, 0);
}

void CodeGenerator::leaveBlock() {
    if (blocks_.currentBlock_ == nullptr) {
        throw std::runtime_error("No block to leave");
    }

    BlockInfo* bl = blocks_.currentBlock_;
    blocks_.currentBlock_ = bl->previous;

    removeLocalVars(bl->nactvar);
    patchtohere(bl->breaklist);

    delete bl;
}

}  // namespace Lua