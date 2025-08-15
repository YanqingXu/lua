#include "write_barrier.hpp"
#include "../../vm/lua_state.hpp"
#include "../../vm/global_state.hpp"
#include "../../vm/value.hpp"
#include "../core/garbage_collector.hpp"
#include "../utils/gc_types.hpp"
#include <cassert>

namespace Lua {
    namespace WriteBarrier {

        void barrierForward(LuaState* L, GCObject* parent, GCObject* child) {
            if (!L || !parent || !child) {
                return;
            }

            // 获取全局状态和GC
            GlobalState* G = L->getGlobalState();
            if (!G) return;

            GarbageCollector* gc = G->getGC();
            if (!gc) return;

            // Lua 5.1兼容的断言检查
            assert(GCUtils::isblack(parent) && GCUtils::iswhite(child));
            assert(!GCUtils::isdead(gc, child) && !GCUtils::isdead(gc, parent));

            // 检查GC状态 - 在finalize和pause阶段不执行屏障
            GCState gcState = gc->getState();
            assert(gcState != GCState::Finalize && gcState != GCState::Pause);

            // 检查父对象类型 - 表对象不应该使用前向屏障
            // assert(parent->getObjectType() != GCObjectType::Table);

            // 如果GC正在传播阶段，将子对象标记为灰色
            if (gcState == GCState::Propagate) {
                GCUtils::white2gray(child);
            } else {
                // 否则将父对象标记为白色以避免其他屏障
                GCUtils::makewhite(gc, parent);
            }
        }

        void barrierBackward(LuaState* L, GCObject* obj) {
            if (!L || !obj) {
                return;
            }

            // 获取全局状态和GC
            GlobalState* G = L->getGlobalState();
            if (!G) return;

            GarbageCollector* gc = G->getGC();
            if (!gc) return;

            // Lua 5.1兼容的断言检查
            assert(GCUtils::isblack(obj) && !GCUtils::isdead(gc, obj));

            // 检查GC状态
            GCState gcState = gc->getState();
            assert(gcState != GCState::Finalize && gcState != GCState::Pause);

            // 将黑色对象重新标记为灰色（后向屏障的核心操作）
            GCUtils::black2gray(obj);

            // 如果GC正在传播阶段，需要将对象重新加入灰色列表
            if (gcState == GCState::Propagate) {
                gc->addToGrayList(obj);
            }
        }

        bool needsBarrier(GCObject* parent, GCObject* child) {
            if (!parent || !child) {
                return false;
            }

            // 需要屏障的条件：父对象是黑色，子对象是白色
            return GCUtils::isblack(parent) && GCUtils::iswhite(child);
        }

    } // namespace WriteBarrier
    
    // 全局写屏障函数实现 - 对应官方API

    /**
     * @brief 对应官方luaC_barrierf函数
     * 前向写屏障：当黑色对象引用白色对象时调用
     */
    void luaC_barrierf(LuaState* L, GCObject* o, GCObject* v) {
        WriteBarrier::barrierForward(L, o, v);
    }

    /**
     * @brief 对应官方luaC_barrierback函数
     * 后向写屏障：将黑色表对象重新标记为灰色
     */
    void luaC_barrierback(LuaState* L, void* t) {
        if (t) {
            WriteBarrier::barrierBackward(L, static_cast<GCObject*>(t));
        }
    }

    /**
     * @brief 检查并执行写屏障 - 通用接口
     */
    void luaC_checkBarrier(LuaState* L, GCObject* parent, GCObject* child) {
        if (WriteBarrier::needsBarrier(parent, child)) {
            WriteBarrier::barrierForward(L, parent, child);
        }
    }

    /**
     * @brief 表专用写屏障检查
     */
    void luaC_checkTableBarrier(LuaState* L, GCObject* table, GCObject* value) {
        if (WriteBarrier::needsBarrier(table, value)) {
            WriteBarrier::barrierBackward(L, table);
        }
    }

    // 辅助函数实现

    /**
     * @brief 检查值是否为可回收类型
     */
    bool isCollectable(const Value* v) {
        if (!v) return false;

        // 检查Value类型是否为可回收的GC对象类型
        return v->isString() || v->isTable() || v->isFunction() ||
               v->isUserdata() || v->isThread();
    }

    /**
     * @brief 从值中获取GCObject指针
     */
    GCObject* gcvalue(const Value* v) {
        if (!v || !isCollectable(v)) {
            return nullptr;
        }

        // 根据Value类型提取GCObject指针
        // 注意：这是一个简化实现，实际需要根据具体的Value内部结构来实现
        if (v->isTable()) {
            auto table = v->asTable();
            return reinterpret_cast<GCObject*>(table.get());
        } else if (v->isFunction()) {
            auto func = v->asFunction();
            return reinterpret_cast<GCObject*>(func.get());
        } else if (v->isUserdata()) {
            auto userdata = v->asUserdata();
            return reinterpret_cast<GCObject*>(userdata.get());
        } else if (v->isThread()) {
            auto thread = v->asThread();
            return reinterpret_cast<GCObject*>(thread.get());
        }

        // 对于字符串，需要特殊处理，因为asString()返回的是引用
        // 这里暂时返回nullptr，实际实现需要从字符串引用中获取GCObject
        return nullptr;
    }
    
} // namespace Lua
