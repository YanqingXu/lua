#include "function.hpp"
#include "value.hpp"
#include "../gc/core/garbage_collector.hpp"
#include "../gc/core/gc_ref.hpp"
#include "../gc/memory/allocator.hpp"
#include "../gc/barriers/write_barrier.hpp"  // 为写屏障支持
#include "../api/lua51_gc_api.hpp"
#include "lua_state.hpp"  // 为LuaState类型

namespace Lua {
    Function::Function(Type type) : GCObject(GCObjectType::Function, sizeof(Function)), type(type) {
        if (type == Type::Lua) {
            lua.code = nullptr;
            lua.nparams = 0;
            lua.nlocals = 0;
            lua.nupvalues = 0;
        } else {
            native.fn = nullptr;
        }
    }
    
    GCRef<Function> Function::createLua(
        Ptr<Vec<Instruction>> code, 
        const Vec<Value>& constants,
        const Vec<GCRef<Function>>& prototypes,
        u8 nparams,
        u8 nlocals,
        u8 nupvalues,
        bool isVariadic
    ) {
        // Create a new Function object using GC allocator
        extern GCAllocator* g_gcAllocator;
        GCRef<Function> func;
        if (g_gcAllocator) {
            Function* obj = g_gcAllocator->allocateObject<Function>(GCObjectType::Function, Type::Lua);
            func = GCRef<Function>(obj);
        } else {
            // Fallback to direct allocation
            Function* obj = new Function(Type::Lua);
            func = GCRef<Function>(obj);
        }
        
        // Set code
        func->lua.code = code;
        
        // Handle constants carefully, avoid using direct assignment
        // Reserve space first
        func->lua.constants.reserve(constants.size());
        
        // Add elements one by one instead of bulk assignment
        for (const auto& value : constants) {
            func->lua.constants.push_back(value);
        }
        
        // Handle prototypes
        func->lua.prototypes.reserve(prototypes.size());
        for (const auto& prototype : prototypes) {
            func->lua.prototypes.push_back(prototype);
        }
        
        // Through regular vector operations
        func->lua.nparams = nparams;
        func->lua.nlocals = nlocals;
        func->lua.nupvalues = nupvalues;
        func->lua.isVariadic = isVariadic;
        
        // Initialize upvalues array
        func->lua.upvalues.resize(nupvalues);
        
        return func;
    }

    GCRef<Function> Function::createLuaWithBarrier(
        LuaState* L,
        Ptr<Vec<Instruction>> code,
        const Vec<Value>& constants,
        const Vec<GCRef<Function>>& prototypes,
        u8 nparams,
        u8 nlocals,
        u8 nupvalues,
        bool isVariadic
    ) {
        // 创建函数对象
        GCRef<Function> func = createLua(code, constants, prototypes, nparams, nlocals, nupvalues, isVariadic);

        // 应用写屏障 - 函数引用常量中的GC对象
        if (L) {
            for (const auto& constant : constants) {
                if (constant.isGCObject()) {
                    GCObject* constObj = constant.asGCObject();
                    if (constObj) {
                        luaC_objbarrier(L, func.get(), constObj);
                    }
                }
            }

            // 应用写屏障 - 函数引用原型
            for (const auto& prototype : prototypes) {
                if (prototype) {
                    luaC_objbarrier(L, func.get(), prototype.get());
                }
            }
        }

        return func;
    }

    GCRef<Function> Function::createNative(NativeFn fn) {
        // Create a new Function object using GC allocator
        extern GCAllocator* g_gcAllocator;
        GCRef<Function> func;
        if (g_gcAllocator) {
            Function* obj = g_gcAllocator->allocateObject<Function>(GCObjectType::Function, Type::Native);
            func = GCRef<Function>(obj);
        } else {
            // Fallback to direct allocation
            Function* obj = new Function(Type::Native);
            func = GCRef<Function>(obj);
        }
        func->native.fn = fn;
        func->native.isLegacy = false;
        return func;
    }

    GCRef<Function> Function::createNativeWithBarrier(LuaState* L, NativeFn fn) {
        // 创建native函数对象
        GCRef<Function> func = createNative(fn);

        // Native函数通常不需要额外的写屏障，因为它们不引用其他GC对象
        // 但为了一致性，我们保留这个接口
        (void)L;  // 避免未使用参数警告

        return func;
    }

    GCRef<Function> Function::createNativeLegacy(NativeFnLegacy fn) {
        // Create a new Function object using GC allocator
        extern GCAllocator* g_gcAllocator;
        GCRef<Function> func;
        if (g_gcAllocator) {
            Function* obj = g_gcAllocator->allocateObject<Function>(GCObjectType::Function, Type::Native);
            func = GCRef<Function>(obj);
        } else {
            // Fallback to direct allocation
            Function* obj = new Function(Type::Native);
            func = GCRef<Function>(obj);
        }
        func->native.fnLegacy = fn;
        func->native.isLegacy = true;
        return func;
    }
    
    const Vec<Instruction>& Function::getCode() const {
        static const Vec<Instruction> empty;
        return type == Type::Lua && lua.code ? *lua.code : empty;
    }
    
    const Vec<Value>& Function::getConstants() const {
        static const Vec<Value> empty;
        return type == Type::Lua ? lua.constants : empty;
    }
    
    const Vec<GCRef<Function>>& Function::getPrototypes() const {
        static const Vec<GCRef<Function>> empty;
        return type == Type::Lua ? lua.prototypes : empty;
    }
    
    usize Function::getConstantCount() const {
        return type == Type::Lua ? lua.constants.size() : 0;
    }
    
    NativeFn Function::getNative() const {
        return (type == Type::Native && !native.isLegacy) ? native.fn : nullptr;
    }

    NativeFnLegacy Function::getNativeLegacy() const {
        return (type == Type::Native && native.isLegacy) ? native.fnLegacy : nullptr;
    }

    bool Function::isNativeLegacy() const {
        return type == Type::Native && native.isLegacy;
    }
    
    // GCObject virtual function implementations
    void Function::markReferences(GarbageCollector* gc) {
        // Lua 5.1兼容的函数GC标记实现
        // 参考官方lgc.c中的traverseclosure和traverseproto函数
        // 使用类型化的GC标记策略

        markReferencesTyped(gc);
    }
    
    usize Function::getSize() const {
        return sizeof(Function);
    }
    
    usize Function::getAdditionalSize() const {
        usize additionalSize = 0;
        
        if (type == Type::Lua) {
            // Calculate size of constants vector
            additionalSize += lua.constants.capacity() * sizeof(Value);
            
            // Calculate size of prototypes vector
            additionalSize += lua.prototypes.capacity() * sizeof(GCRef<Function>);
            
            // Calculate size of upvalues vector
            additionalSize += lua.upvalues.capacity() * sizeof(Value*);
            
            // Calculate size of code vector if it exists
            if (lua.code) {
                additionalSize += lua.code->capacity() * sizeof(Instruction);
            }
        }
        
        return additionalSize;
    }
    
    GCRef<Upvalue> Function::getUpvalue(usize index) const {
        if (type != Type::Lua || index >= lua.upvalues.size()) {
            return GCRef<Upvalue>();
        }
        return lua.upvalues[index];
    }
    
    void Function::setUpvalue(usize index, GCRef<Upvalue> upvalue) {
         if (type == Type::Lua && index < lua.upvalues.size()) {
             lua.upvalues[index] = upvalue;
         }
     }

     void Function::setUpvalueWithBarrier(usize index, GCRef<Upvalue> upvalue, LuaState* L) {
         // 应用写屏障 - 函数引用新的upvalue
         if (L && upvalue && type == Type::Lua && index < lua.upvalues.size()) {
             luaC_objbarrier(L, this, upvalue.get());
         }

         // 设置upvalue
         setUpvalue(index, upvalue);
     }

     void Function::setConstantWithBarrier(usize index, const Value& value, LuaState* L) {
         if (type != Type::Lua || index >= lua.constants.size()) {
             return;
         }

         // 应用写屏障 - 函数引用新的常量对象
         if (L && value.isGCObject()) {
             GCObject* valueObj = value.asGCObject();
             if (valueObj) {
                 luaC_objbarrier(L, this, valueObj);
             }
         }

         // 设置常量
         lua.constants[index] = value;
     }

     void Function::addPrototypeWithBarrier(GCRef<Function> prototype, LuaState* L) {
         if (type != Type::Lua) {
             return;
         }

         // 应用写屏障 - 函数引用新的原型
         if (L && prototype) {
             luaC_objbarrier(L, this, prototype.get());
         }

         // 添加原型
         lua.prototypes.push_back(prototype);
     }

     // === 原型共享机制优化实现 ===

     void Function::setParentPrototype(Function* parent) {
         if (type == Type::Lua) {
             lua.prototype = parent;
         }
     }

     Function* Function::getParentPrototype() const {
         return (type == Type::Lua) ? lua.prototype : nullptr;
     }

     bool Function::isSharedPrototype() const {
         if (type != Type::Lua) {
             return false;
         }

         // 检查是否有多个函数引用这个原型
         // 简化实现：如果有父原型或子原型，则认为是共享的
         return lua.prototype != nullptr || !lua.prototypes.empty();
     }

     usize Function::getPrototypeChainDepth() const {
         if (type != Type::Lua) {
             return 0;
         }

         usize depth = 0;
         Function* current = lua.prototype;
         while (current && depth < 100) {  // 防止无限循环
             depth++;
             current = current->getParentPrototype();
         }

         return depth;
     }

     // === C函数与Lua函数差异化GC处理实现 ===

     bool Function::requiresFullGCProcessing() const {
         return type == Type::Lua;
     }

     Function::GCProcessingType Function::getGCProcessingType() const {
         switch (type) {
             case Type::Lua:
                 return GCProcessingType::Full;
             case Type::Native:
                 return GCProcessingType::Lightweight;
             default:
                 return GCProcessingType::None;
         }
     }

     void Function::markReferencesTyped(GarbageCollector* gc) {
         // 根据函数类型执行不同的GC标记策略
         switch (getGCProcessingType()) {
             case GCProcessingType::Full:
                 markLuaFunctionReferences(gc);
                 break;
             case GCProcessingType::Lightweight:
                 markNativeFunctionReferences(gc);
                 break;
             case GCProcessingType::None:
             default:
                 // 无需处理
                 break;
         }
     }

     void Function::markNativeFunctionReferences(GarbageCollector* gc) {
         // C函数的轻量级GC处理
         // 通常C函数不引用其他GC对象，但为了完整性保留这个方法

         // 在未来的实现中，这里可能需要标记：
         // - C函数的环境表（如果有）
         // - C函数的upvalue（如果支持）
         // - 其他C函数特定的GC对象

         (void)gc;  // 避免未使用参数警告
     }

     void Function::markLuaFunctionReferences(GarbageCollector* gc) {
         // Lua函数的完整GC处理
         // 这是原来markReferences方法中Lua函数部分的逻辑

         // 1. 标记常量表中的所有GC对象
         for (const auto& constant : lua.constants) {
             if (constant.isGCObject()) {
                 GCObject* constObj = constant.asGCObject();
                 if (constObj) {
                     gc->markObject(constObj);
                 }
             }
         }

         // 2. 标记所有嵌套函数原型（子原型）
         for (const auto& prototype : lua.prototypes) {
             if (prototype) {
                 gc->markObject(prototype.get());
             }
         }

         // 3. 标记所有upvalue引用
         for (const auto& upvalue : lua.upvalues) {
             if (upvalue) {
                 gc->markObject(upvalue.get());
             }
         }

         // 4. 标记父函数原型（如果存在）
         if (lua.prototype != nullptr) {
             gc->markObject(lua.prototype);
         }
     }
     
     const Value& Function::getConstant(usize index) const {
         static const Value empty;
         if (type != Type::Lua || index >= lua.constants.size()) {
             return empty;
         }
         return lua.constants[index];
     }
     
     void Function::setPrototype(Function* proto) {
         if (type == Type::Lua) {
             lua.prototype = proto;
         }
     }
     
     void Function::closeUpvalues() {
        if (type == Type::Lua) {
            // Close all upvalues by calling their close method
            // This properly handles the upvalue closing protocol
            for (auto& upvalue : lua.upvalues) {
                if (upvalue) {
                    upvalue->close();
                }
            }
        }
    }
    
    usize Function::estimateMemoryUsage() const {
        if (type != Type::Lua) {
            return sizeof(Function);
        }
        
        usize totalSize = sizeof(Function);
        
        // Add code size
        if (lua.code) {
            totalSize += lua.code->size() * sizeof(Instruction);
        }
        
        // Add constants size (rough estimate)
        totalSize += lua.constants.size() * sizeof(Value);
        
        // Add upvalues size
        totalSize += lua.upvalues.size() * sizeof(GCRef<Upvalue>);
        
        // Add prototypes size (recursive)
        for (const auto& proto : lua.prototypes) {
            if (proto) {
                totalSize += proto->estimateMemoryUsage();
            }
        }
        
        return totalSize;
    }
    
    bool Function::validateUpvalueCount() const {
        return getUpvalueCount() <= MAX_UPVALUES_PER_CLOSURE;
    }
    
    bool Function::validateNestingDepth(u8 currentDepth) const {
        return currentDepth <= MAX_FUNCTION_NESTING_DEPTH;
    }
    
    bool Function::isValidUpvalueIndex(usize index) const {
        if (type != Type::Lua) {
            return false;
        }
        return index < lua.nupvalues;
    }
}
