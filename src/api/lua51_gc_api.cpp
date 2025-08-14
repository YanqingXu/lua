#include "lua51_gc_api.hpp"
#include "../vm/global_state.hpp"
#include <cassert>

namespace Lua {
    
    // === 核心GC函数实现 ===
    
    void luaC_step(LuaState* L) {
        if (!L) return;
        
        GarbageCollector* gc = L->getGlobalState()->getGC();
        if (gc) {
            gc->step(L);
        }
    }
    
    void luaC_fullgc(LuaState* L) {
        if (!L) return;
        
        GarbageCollector* gc = L->getGlobalState()->getGC();
        if (gc) {
            gc->fullGC(L);
        }
    }
    
    void luaC_freeall(LuaState* L) {
        if (!L) return;
        
        // 设置特殊的白色标记以回收所有对象
        GarbageCollector* gc = L->getGlobalState()->getGC();
        if (gc) {
            // 这需要在GarbageCollector中实现freeAll方法
            // 暂时使用fullGC作为替代
            gc->fullGC(L);
        }
    }
    
    void luaC_link(LuaState* L, GCObject* o, u8 tt) {
        if (!L || !o) return;
        
        GarbageCollector* gc = L->getGlobalState()->getGC();
        if (gc) {
            // 设置对象类型
            o->setType(static_cast<GCObjectType>(tt));
            
            // 设置为当前白色
            o->setColor(static_cast<GCColor>(luaC_white(L)));
            
            // 注册到GC
            gc->registerObject(o);
        }
    }
    
    void luaC_linkupval(LuaState* L, GCObject* uv) {
        if (!L || !uv) return;
        
        // Upvalue的特殊链接逻辑
        luaC_link(L, uv, static_cast<u8>(GCObjectType::Upvalue));
        
        // 如果upvalue是灰色的，需要特殊处理
        GarbageCollector* gc = L->getGlobalState()->getGC();
        if (gc && uv->getColor() == GCColor::Gray) {
            if (gc->getState() == GCState::Propagate) {
                // 在传播阶段，将灰色upvalue标记为黑色并应用屏障
                uv->setColor(GCColor::Black);
                // 这里需要应用写屏障，但需要upvalue的值
            } else {
                // 在扫描阶段，将其标记为白色
                uv->setColor(static_cast<GCColor>(luaC_white(L)));
            }
        }
    }
    
    usize luaC_separateudata(LuaState* L, int all) {
        if (!L) return 0;
        
        // 这个函数需要遍历所有userdata并分离需要终结的对象
        // 暂时返回0，完整实现需要userdata系统支持
        (void)all; // 抑制未使用参数警告
        return 0;
    }
    
    void luaC_callGCTM(LuaState* L) {
        if (!L) return;
        
        // 调用所有待处理的GC标记方法
        // 这需要终结器队列的支持
        // 暂时为空实现
    }
    
    // === 内存管理辅助函数实现 ===
    
    u8 luaC_white(LuaState* L) {
        if (!L) return 0;
        
        GarbageCollector* gc = L->getGlobalState()->getGC();
        if (gc) {
            // 返回当前白色标记的位表示
            GCColor currentWhite = gc->getCurrentWhite();
            return static_cast<u8>(currentWhite) & WHITEBITS;
        }
        return 0;
    }
    
    bool luaC_isdead(LuaState* L, GCObject* v) {
        if (!L || !v) return true;
        
        u8 objectMark = v->getGCMark();
        u8 otherWhite = luaC_white(L) ^ WHITEBITS;
        
        return (objectMark & otherWhite & WHITEBITS) != 0;
    }
    
    void luaC_makewhite(LuaState* L, GCObject* x) {
        if (!L || !x) return;
        
        u8 currentWhite = luaC_white(L);
        u8 mark = x->getGCMark();
        
        // 清除颜色位并设置为当前白色
        mark = (mark & ~WHITEBITS) | currentWhite;
        x->setGCMark(mark);
    }
    
    void luaC_changewhite(GCObject* x) {
        if (!x) return;
        
        u8 mark = x->getGCMark();
        mark ^= WHITEBITS; // 翻转白色位
        x->setGCMark(mark);
    }
    
    void luaC_gray2black(GCObject* x) {
        if (!x) return;
        
        x->setColor(GCColor::Black);
    }
    
    void luaC_black2gray(GCObject* x) {
        if (!x) return;
        
        x->setColor(GCColor::Gray);
    }
    
    void luaC_white2gray(GCObject* x) {
        if (!x) return;
        
        u8 mark = x->getGCMark();
        reset2bits(mark, WHITE0BIT, WHITE1BIT);
        x->setGCMark(mark);
    }
    
    // === GC参数配置实现 ===
    
    void luaC_setgcpause(LuaState* L, int pause) {
        if (!L) return;
        
        GarbageCollector* gc = L->getGlobalState()->getGC();
        if (gc) {
            GCConfig config = gc->getConfig();
            config.gcpause = pause;
            gc->setConfig(config);
        }
    }
    
    void luaC_setgcstepmul(LuaState* L, int stepmul) {
        if (!L) return;
        
        GarbageCollector* gc = L->getGlobalState()->getGC();
        if (gc) {
            GCConfig config = gc->getConfig();
            config.gcstepmul = stepmul;
            gc->setConfig(config);
        }
    }
    
    int luaC_getgcpause(LuaState* L) {
        if (!L) return 200; // 默认值
        
        GarbageCollector* gc = L->getGlobalState()->getGC();
        if (gc) {
            return gc->getConfig().gcpause;
        }
        return 200;
    }
    
    int luaC_getgcstepmul(LuaState* L) {
        if (!L) return 200; // 默认值
        
        GarbageCollector* gc = L->getGlobalState()->getGC();
        if (gc) {
            return gc->getConfig().gcstepmul;
        }
        return 200;
    }
    
    // === 内存统计实现 ===
    
    usize luaC_gettotalbytes(LuaState* L) {
        if (!L) return 0;
        
        return L->getGlobalState()->getTotalBytes();
    }
    
    usize luaC_getthreshold(LuaState* L) {
        if (!L) return 0;
        
        return L->getGlobalState()->getGCThreshold();
    }
    
    void luaC_setthreshold(LuaState* L, usize threshold) {
        if (!L) return;
        
        L->getGlobalState()->setGCThreshold(threshold);
    }
    
    usize luaC_getestimate(LuaState* L) {
        if (!L) return 0;
        
        GarbageCollector* gc = L->getGlobalState()->getGC();
        if (gc) {
            return gc->getStats().currentUsage;
        }
        return 0;
    }
    
    usize luaC_getgcdept(LuaState* L) {
        if (!L) return 0;
        
        // 这需要在GarbageCollector中暴露gcdept字段
        // 暂时返回0
        return 0;
    }
    
} // namespace Lua
