#include "compiler_utils.hpp"
#include "../common/opcodes.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>

namespace Lua {
    int CompilerUtils::allocateRegister(int& nextReg, int maxRegs) {
        if (nextReg >= maxRegs) {
            throw LuaException("Too many registers in use");
        }
        return nextReg++;
    }
    
    void CompilerUtils::freeRegister(int& nextReg) {
        if (nextReg > 0) {
            nextReg--;
        }
    }

    int CompilerUtils::reserveRegisters(int& nextReg, int count, int maxRegs) {
        if (nextReg + count > maxRegs) {
            throw LuaException("Too many registers in use");
        }
        int baseReg = nextReg;
        nextReg += count;
        return baseReg;
    }

    void CompilerUtils::freeRegisters(int& nextReg, int count) {
        if (nextReg >= count) {
            nextReg -= count;
        } else {
            nextReg = 0;
        }
    }
    
    bool CompilerUtils::isValidRegister(int reg) {
        return reg >= 0 && reg < 255;
    }
    
    int CompilerUtils::addConstant(Vec<Value>& constants, const Value& value) {
        // Check if constant already exists
        int existingIdx = findConstant(constants, value);
        if (existingIdx != -1) {
            return existingIdx;
        }
        
        // Add new constant
        if (constants.size() >= 255) {
            throw LuaException("Too many constants");
        }
        
        constants.push_back(value);
        return static_cast<int>(constants.size() - 1);
    }
    
    int CompilerUtils::findConstant(const Vec<Value>& constants, const Value& value) {
        for (size_t i = 0; i < constants.size(); ++i) {
            if (constants[i] == value) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
    
    int CompilerUtils::resolveLocal(const Vec<Local>& locals, const Str& name, int scopeDepth) {
        // Search from most recent to oldest
        for (int i = static_cast<int>(locals.size()) - 1; i >= 0; --i) {
            const Local& local = locals[i];
            if (local.name == name && local.depth <= scopeDepth) {
                return local.slot;
            }
        }
        return -1; // Not found
    }
    
    void CompilerUtils::addLocal(Vec<Local>& locals, const Str& name, int depth, int slot) {
        if (locals.size() >= 255) {
            throw LuaException("Too many local variables");
        }
        
        // Check for duplicate in current scope
        for (const auto& local : locals) {
            if (local.name == name && local.depth == depth) {
                reportWarning("Local variable '" + name + "' shadows existing variable");
                break;
            }
        }
        
        locals.emplace_back(name, depth, slot);
    }
    
    void CompilerUtils::removeLocalsAtDepth(Vec<Local>& locals, int depth) {
        auto it = std::remove_if(locals.begin(), locals.end(),
            [depth](const Local& local) {
                return local.depth >= depth;
            });
        locals.erase(it, locals.end());
    }
    
    int CompilerUtils::createJumpPlaceholder(Vec<Instruction>& code) {
        int jumpAddr = static_cast<int>(code.size());
        // Emit placeholder jump instruction
        code.push_back(Instruction::createJMP(0));
        return jumpAddr;
    }
    
    void CompilerUtils::patchJump(Vec<Instruction>& code, int jumpAddr, int targetAddr) {
        if (jumpAddr < 0 || jumpAddr >= static_cast<int>(code.size())) {
            throw LuaException("Invalid jump address for patching");
        }
        
        int offset = targetAddr - jumpAddr - 1;
        Instruction& jumpInstr = code[jumpAddr];
        
        // Update the jump instruction with the correct offset
        jumpInstr = Instruction::createJMP(offset);
    }
    
    void CompilerUtils::patchJumpToHere(Vec<Instruction>& code, int jumpAddr, int currentAddr) {
        patchJump(code, jumpAddr, currentAddr);
    }
    
    void CompilerUtils::emitInstruction(Vec<Instruction>& code, const Instruction& instr) {
        code.push_back(instr);
    }
    
    int CompilerUtils::getCurrentAddress(const Vec<Instruction>& code) {
        return static_cast<int>(code.size());
    }
    
    void CompilerUtils::enterScope(int& scopeDepth) {
        scopeDepth++;
    }
    
    void CompilerUtils::exitScope(int& scopeDepth, Vec<Local>& locals) {
        if (scopeDepth > 0) {
            removeLocalsAtDepth(locals, scopeDepth);
            scopeDepth--;
        }
    }
    
    void CompilerUtils::reportCompilerError(const Str& message, int line) {
        std::cerr << "Compiler Error";
        if (line >= 0) {
            std::cerr << " at line " << line;
        }
        std::cerr << ": " << message << std::endl;
    }
    
    void CompilerUtils::reportWarning(const Str& message, int line) {
        std::cerr << "Compiler Warning";
        if (line >= 0) {
            std::cerr << " at line " << line;
        }
        std::cerr << ": " << message << std::endl;
    }
    
    bool CompilerUtils::isConstantExpression(const Value& value) {
        ValueType type = value.type();
        return type == ValueType::Nil || 
               type == ValueType::Boolean || 
               type == ValueType::Number || 
               type == ValueType::String;
    }
    
    bool CompilerUtils::canOptimizeInstruction(const Instruction& instr) {
        OpCode op = instr.getOpCode();
        
        // Simple optimizations for constant operations
        switch (op) {
            case OpCode::ADD:
            case OpCode::SUB:
            case OpCode::MUL:
            case OpCode::DIV:
                return true;
            default:
                return false;
        }
    }
    
    Instruction CompilerUtils::optimizeInstruction(const Instruction& instr) {
        // For now, return the instruction as-is
        // TODO: Implement constant folding and other optimizations
        return instr;
    }
    
    Str CompilerUtils::instructionToString(const Instruction& instr) {
        OpCode op = instr.getOpCode();
        u8 a = instr.getA();
        u8 b = instr.getB();
        u8 c = instr.getC();
        
        Str opName;
        switch (op) {
            case OpCode::MOVE: opName = "MOVE"; break;
            case OpCode::LOADK: opName = "LOADK"; break;
            case OpCode::LOADBOOL: opName = "LOADBOOL"; break;
            case OpCode::LOADNIL: opName = "LOADNIL"; break;
            case OpCode::GETGLOBAL: opName = "GETGLOBAL"; break;
            case OpCode::SETGLOBAL: opName = "SETGLOBAL"; break;
            case OpCode::GETTABLE: opName = "GETTABLE"; break;
            case OpCode::SETTABLE: opName = "SETTABLE"; break;
            case OpCode::NEWTABLE: opName = "NEWTABLE"; break;
            case OpCode::ADD: opName = "ADD"; break;
            case OpCode::SUB: opName = "SUB"; break;
            case OpCode::MUL: opName = "MUL"; break;
            case OpCode::DIV: opName = "DIV"; break;
            case OpCode::MOD: opName = "MOD"; break;
            case OpCode::POW: opName = "POW"; break;
            case OpCode::UNM: opName = "UNM"; break;
            case OpCode::NOT: opName = "NOT"; break;
            case OpCode::LEN: opName = "LEN"; break;
            case OpCode::EQ: opName = "EQ"; break;
            case OpCode::LT: opName = "LT"; break;
            case OpCode::LE: opName = "LE"; break;
            case OpCode::JMP: opName = "JMP"; break;
            case OpCode::CALL: opName = "CALL"; break;
            case OpCode::RETURN: opName = "RETURN"; break;
            case OpCode::CLOSURE: opName = "CLOSURE"; break;
        case OpCode::GETUPVAL: opName = "GETUPVAL"; break;
        case OpCode::SETUPVAL: opName = "SETUPVAL"; break;
            default: opName = "UNKNOWN"; break;
        }
        
        return opName + " " + std::to_string(a) + " " + 
               std::to_string(b) + " " + std::to_string(c);
    }
    
    void CompilerUtils::dumpBytecode(const Vec<Instruction>& code, const Vec<Value>& constants) {
        std::cout << "=== Bytecode Dump ===" << std::endl;
        std::cout << "Constants (" << constants.size() << "):" << std::endl;
        
        for (size_t i = 0; i < constants.size(); ++i) {
            std::cout << "  [" << i << "] ";
            const Value& val = constants[i];
            switch (val.type()) {
                case ValueType::Nil:
                    std::cout << "nil";
                    break;
                case ValueType::Boolean:
                    std::cout << (val.asBoolean() ? "true" : "false");
                    break;
                case ValueType::Number:
                    std::cout << val.asNumber();
                    break;
                case ValueType::String:
                    std::cout << "\"" << val.asString() << "\"";
                    break;
                default:
                    std::cout << "<unknown>";
                    break;
            }
            std::cout << std::endl;
        }
        
        std::cout << "\nInstructions (" << code.size() << "):" << std::endl;
        for (size_t i = 0; i < code.size(); ++i) {
            std::cout << std::setw(4) << i << ": " 
                      << instructionToString(code[i]) << std::endl;
        }
        std::cout << "=== End Dump ===" << std::endl;
    }
    
    void CompilerUtils::dumpLocals(const Vec<Local>& locals) {
        std::cout << "=== Local Variables ===" << std::endl;
        for (size_t i = 0; i < locals.size(); ++i) {
            const Local& local = locals[i];
            std::cout << "  [" << i << "] " << local.name 
                      << " (depth: " << local.depth 
                      << ", slot: " << local.slot
                      << ", captured: " << (local.isCaptured ? "yes" : "no")
                      << ")" << std::endl;
        }
        std::cout << "=== End Locals ===" << std::endl;
    }
}