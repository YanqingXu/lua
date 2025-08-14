#pragma once

#include "../core/gc_object.hpp"
#include "../utils/gc_types.hpp"
#include "../../common/types.hpp"

namespace Lua {
    // Forward declarations
    class LuaState;
    class GarbageCollector;
    
    /**
     * @brief 写屏障系统 - Lua 5.1兼容实现
     * 
     * 写屏障确保在增量GC过程中，当黑色对象引用白色对象时，
     * 能够正确维护三色不变性，防止白色对象被错误回收。
     */
    namespace WriteBarrier {
        
        /**
         * @brief 前向写屏障 - 当黑色对象引用白色对象时调用
         * @param L Lua状态
         * @param parent 父对象(黑色)
         * @param child 子对象(白色)
         * 
         * 对应官方luaC_barrierf函数
         */
        void barrierForward(LuaState* L, GCObject* parent, GCObject* child);
        
        /**
         * @brief 后向写屏障 - 将黑色对象重新标记为灰色
         * @param L Lua状态
         * @param obj 需要重新标记的对象
         * 
         * 对应官方luaC_barrierback函数，主要用于表对象
         */
        void barrierBackward(LuaState* L, GCObject* obj);
        
        /**
         * @brief 检查是否需要写屏障
         * @param parent 父对象
         * @param child 子对象
         * @return true 如果需要写屏障
         */
        bool needsBarrier(GCObject* parent, GCObject* child);
        
        /**
         * @brief 获取对象的GC颜色
         * @param obj GC对象
         * @return GCColor 对象颜色
         */
        inline GCColor getObjectColor(GCObject* obj) {
            return obj ? obj->getColor() : GCColor::White0;
        }
        
        /**
         * @brief 检查对象是否为白色
         * @param obj GC对象
         * @param currentWhite 当前白色标记
         * @return true 如果对象是白色
         */
        inline bool isWhite(GCObject* obj, GCColor currentWhite) {
            if (!obj) return false;
            GCColor color = obj->getColor();
            return color == GCColor::White0 || color == GCColor::White1;
        }
        
        /**
         * @brief 检查对象是否为黑色
         * @param obj GC对象
         * @return true 如果对象是黑色
         */
        inline bool isBlack(GCObject* obj) {
            return obj && obj->getColor() == GCColor::Black;
        }
        
        /**
         * @brief 检查对象是否为灰色
         * @param obj GC对象
         * @return true 如果对象是灰色
         */
        inline bool isGray(GCObject* obj) {
            return obj && obj->getColor() == GCColor::Gray;
        }
    }
    
    // Lua 5.1兼容的写屏障宏定义
    
    /**
     * @brief 通用写屏障宏 - 对应官方luaC_barrier
     * 当父对象(黑色)引用子对象(白色)时触发前向写屏障
     */
    #define luaC_barrier(L, p, v) \
        do { \
            if ((v) && WriteBarrier::isWhite((v), GCColor::White0) && \
                WriteBarrier::isBlack(obj2gco(p))) { \
                WriteBarrier::barrierForward((L), obj2gco(p), (v)); \
            } \
        } while(0)
    
    /**
     * @brief 表写屏障宏 - 对应官方luaC_barriert
     * 专门用于表对象的写屏障，使用后向屏障策略
     */
    #define luaC_barriert(L, t, v) \
        do { \
            if ((v) && WriteBarrier::isWhite((v), GCColor::White0) && \
                WriteBarrier::isBlack(obj2gco(t))) { \
                WriteBarrier::barrierBackward((L), obj2gco(t)); \
            } \
        } while(0)
    
    /**
     * @brief 对象写屏障宏 - 对应官方luaC_objbarrier
     * 用于对象间的引用关系
     */
    #define luaC_objbarrier(L, p, o) \
        do { \
            if ((o) && WriteBarrier::isWhite(obj2gco(o), GCColor::White0) && \
                WriteBarrier::isBlack(obj2gco(p))) { \
                WriteBarrier::barrierForward((L), obj2gco(p), obj2gco(o)); \
            } \
        } while(0)
    
    /**
     * @brief 表对象写屏障宏 - 对应官方luaC_objbarriert
     * 专门用于表对象引用其他对象
     */
    #define luaC_objbarriert(L, t, o) \
        do { \
            if ((o) && WriteBarrier::isWhite(obj2gco(o), GCColor::White0) && \
                WriteBarrier::isBlack(obj2gco(t))) { \
                WriteBarrier::barrierBackward((L), obj2gco(t)); \
            } \
        } while(0)
    
    // 辅助宏定义
    
    /**
     * @brief 将对象指针转换为GCObject指针
     * 对应官方obj2gco宏
     */
    #define obj2gco(o) (static_cast<GCObject*>(o))
    
    /**
     * @brief 检查值是否为白色可回收对象
     * 对应官方valiswhite宏
     */
    #define valiswhite(v) \
        (isCollectable(v) && WriteBarrier::isWhite(gcvalue(v), GCColor::White0))
    
    /**
     * @brief 检查值是否为可回收类型
     * 对应官方iscollectable宏
     */
    inline bool isCollectable(const void* v) {
        // 简化实现，实际需要根据Value类型判断
        return v != nullptr;
    }
    
    /**
     * @brief 从值中获取GCObject指针
     * 对应官方gcvalue宏
     */
    inline GCObject* gcvalue(const void* v) {
        // 简化实现，实际需要从Value中提取GCObject
        return static_cast<GCObject*>(const_cast<void*>(v));
    }
}
