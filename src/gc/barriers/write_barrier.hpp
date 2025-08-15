#pragma once

#include "../core/gc_object.hpp"
#include "../utils/gc_types.hpp"
#include "../../common/types.hpp"

namespace Lua {
    // Forward declarations
    class LuaState;
    class GarbageCollector;
    class Value;

    // Lua 5.1兼容的全局写屏障函数声明
    void luaC_barrierf(LuaState* L, GCObject* o, GCObject* v);
    void luaC_barrierback(LuaState* L, void* t);  // 使用void*以支持不同类型的表
    
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
         * @brief 检查对象是否为白色 - Lua 5.1兼容版本
         * @param obj GC对象
         * @param gc 垃圾收集器实例（用于获取当前白色）
         * @return true 如果对象是白色
         */
        inline bool isWhite(GCObject* obj, const GarbageCollector* gc) {
            return GCUtils::iswhite(obj);
        }

        /**
         * @brief 检查对象是否为黑色 - Lua 5.1兼容版本
         * @param obj GC对象
         * @return true 如果对象是黑色
         */
        inline bool isBlack(GCObject* obj) {
            return GCUtils::isblack(obj);
        }

        /**
         * @brief 检查对象是否为灰色 - Lua 5.1兼容版本
         * @param obj GC对象
         * @return true 如果对象是灰色
         */
        inline bool isGray(GCObject* obj) {
            return GCUtils::isgray(obj);
        }

        /**
         * @brief 检查对象是否已死亡 - Lua 5.1兼容版本
         * @param obj GC对象
         * @param gc 垃圾收集器实例
         * @return true 如果对象已死亡
         */
        inline bool isDead(GCObject* obj, const GarbageCollector* gc) {
            return GCUtils::isdead(gc, obj);
        }
    }
    
    // Lua 5.1兼容的写屏障宏定义
    
    /**
     * @brief 通用写屏障宏 - 对应官方luaC_barrier
     * 当父对象(黑色)引用子对象(白色)时触发前向写屏障
     */
    #define luaC_barrier(L, p, v) \
        do { \
            if ((v) && WriteBarrier::isWhite(static_cast<GCObject*>(v), nullptr) && \
                WriteBarrier::isBlack(static_cast<GCObject*>(p))) { \
                luaC_barrierf((L), static_cast<GCObject*>(p), static_cast<GCObject*>(v)); \
            } \
        } while(0)

    /**
     * @brief 表写屏障宏 - 对应官方luaC_barriert
     * 专门用于表对象的写屏障，使用后向屏障策略
     */
    #define luaC_barriert(L, t, v) \
        do { \
            if ((v) && WriteBarrier::isWhite(static_cast<GCObject*>(v), nullptr) && \
                WriteBarrier::isBlack(static_cast<GCObject*>(t))) { \
                luaC_barrierback((L), static_cast<void*>(t)); \
            } \
        } while(0)

    /**
     * @brief 对象写屏障宏 - 对应官方luaC_objbarrier
     * 用于对象间的引用关系
     */
    #define luaC_objbarrier(L, p, o) \
        do { \
            if ((o) && WriteBarrier::isWhite(static_cast<GCObject*>(o), nullptr) && \
                WriteBarrier::isBlack(static_cast<GCObject*>(p))) { \
                luaC_barrierf((L), static_cast<GCObject*>(p), static_cast<GCObject*>(o)); \
            } \
        } while(0)

    /**
     * @brief 表对象写屏障宏 - 对应官方luaC_objbarriert
     * 专门用于表对象引用其他对象
     */
    #define luaC_objbarriert(L, t, o) \
        do { \
            if ((o) && WriteBarrier::isWhite(static_cast<GCObject*>(o), nullptr) && \
                WriteBarrier::isBlack(static_cast<GCObject*>(t))) { \
                luaC_barrierback((L), static_cast<void*>(t)); \
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
        (isCollectable(v) && GCUtils::iswhite(gcvalue(v)))

    /**
     * @brief 检查值是否为可回收类型
     * 对应官方iscollectable宏
     */
    bool isCollectable(const Value* v);

    /**
     * @brief 从值中获取GCObject指针
     * 对应官方gcvalue宏
     */
    GCObject* gcvalue(const Value* v);
}
