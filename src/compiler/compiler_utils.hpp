#pragma once

#include "../common/types.hpp"
#include "../vm/instruction.hpp"
#include "../vm/value.hpp"

namespace Lua {
    // Local variable information
    struct Local {
        Str name;
        int depth;
        bool isCaptured;
        int slot;    // Register index
        
        Local(const Str& name, int depth, int slot)
            : name(name), depth(depth), isCaptured(false), slot(slot) {}
    };
    
    // Jump patch information
    struct JumpPatch {
        int address;
        int target;
        
        JumpPatch(int addr, int tgt) : address(addr), target(tgt) {}
    };
    
    // Compiler utilities class
    class CompilerUtils {
    public:
        // Register management
        static int allocateRegister(int& nextReg, int maxRegs = 255);
        static void freeRegister(int& nextReg);
        static bool isValidRegister(int reg);
        
        // Constant table management
        static int addConstant(Vec<Value>& constants, const Value& value);
        static int findConstant(const Vec<Value>& constants, const Value& value);
        
        // Local variable management
        static int resolveLocal(const Vec<Local>& locals, const Str& name, int scopeDepth);
        static void addLocal(Vec<Local>& locals, const Str& name, int depth, int slot);
        static void removeLocalsAtDepth(Vec<Local>& locals, int depth);
        
        // Jump management
        static int createJumpPlaceholder(Vec<Instruction>& code);
        static void patchJump(Vec<Instruction>& code, int jumpAddr, int targetAddr);
        static void patchJumpToHere(Vec<Instruction>& code, int jumpAddr, int currentAddr);
        
        // Instruction helpers
        static void emitInstruction(Vec<Instruction>& code, const Instruction& instr);
        static int getCurrentAddress(const Vec<Instruction>& code);
        
        // Scope management helpers
        static void enterScope(int& scopeDepth);
        static void exitScope(int& scopeDepth, Vec<Local>& locals);
        
        // Error handling
        static void reportCompilerError(const Str& message, int line = -1);
        static void reportWarning(const Str& message, int line = -1);
        
        // Optimization helpers
        static bool isConstantExpression(const Value& value);
        static bool canOptimizeInstruction(const Instruction& instr);
        static Instruction optimizeInstruction(const Instruction& instr);
        
        // Debug helpers
        static Str instructionToString(const Instruction& instr);
        static void dumpBytecode(const Vec<Instruction>& code, const Vec<Value>& constants);
        static void dumpLocals(const Vec<Local>& locals);
    };
}