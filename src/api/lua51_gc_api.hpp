#pragma once

#include "../vm/lua_state.hpp"
#include "../gc/core/garbage_collector.hpp"
#include "../gc/barriers/write_barrier.hpp"
#include "../common/types.hpp"

namespace Lua {
    
    /**
     * @brief Lua 5.1兼容的垃圾回收API
     * 
     * 这个文件提供与官方Lua 5.1完全兼容的GC API接口，
     * 包括所有标准的GC函数和宏定义。
     */
    
    // === 核心GC函数 - 对应官方lgc.h中的函数 ===
    
    /**
     * @brief 执行一步增量垃圾回收 - 对应官方luaC_step
     * @param L Lua状态
     */
    void luaC_step(LuaState* L);
    
    /**
     * @brief 执行完整垃圾回收 - 对应官方luaC_fullgc
     * @param L Lua状态
     */
    void luaC_fullgc(LuaState* L);
    
    /**
     * @brief 释放所有对象 - 对应官方luaC_freeall
     * @param L Lua状态
     */
    void luaC_freeall(LuaState* L);
    
    /**
     * @brief 链接新对象到GC - 对应官方luaC_link
     * @param L Lua状态
     * @param o GC对象
     * @param tt 对象类型
     */
    void luaC_link(LuaState* L, GCObject* o, u8 tt);
    
    /**
     * @brief 链接upvalue到GC - 对应官方luaC_linkupval
     * @param L Lua状态
     * @param uv Upvalue对象
     */
    void luaC_linkupval(LuaState* L, GCObject* uv);
    
    /**
     * @brief 分离需要终结的userdata - 对应官方luaC_separateudata
     * @param L Lua状态
     * @param all 是否处理所有userdata
     * @return 分离的内存大小
     */
    usize luaC_separateudata(LuaState* L, int all);
    
    /**
     * @brief 调用所有GC标记方法 - 对应官方luaC_callGCTM
     * @param L Lua状态
     */
    void luaC_callGCTM(LuaState* L);
    
    // === 写屏障函数 - 已在write_barrier.hpp中定义 ===
    // luaC_barrierf, luaC_barrierback 等函数
    
    // === GC检查宏 - 对应官方luaC_checkGC ===
    
    /**
     * @brief GC检查宏 - 对应官方luaC_checkGC
     * 在内存分配后检查是否需要触发GC
     */
    #define luaC_checkGC(L) \
        do { \
            if ((L) && (L)->getGlobalState()->shouldCollectGarbage()) { \
                luaC_step(L); \
            } \
        } while(0)
    
    // === 内存管理辅助函数 ===
    
    /**
     * @brief 获取当前白色标记 - 对应官方luaC_white
     * @param L Lua状态
     * @return 当前白色标记
     */
    u8 luaC_white(LuaState* L);
    
    /**
     * @brief 检查对象是否已死 - 对应官方isdead
     * @param L Lua状态
     * @param v 对象
     * @return true 如果对象已死
     */
    bool luaC_isdead(LuaState* L, GCObject* v);
    
    /**
     * @brief 将对象标记为白色 - 对应官方makewhite
     * @param L Lua状态
     * @param x 对象
     */
    void luaC_makewhite(LuaState* L, GCObject* x);
    
    /**
     * @brief 改变对象白色标记 - 对应官方changewhite
     * @param x 对象
     */
    void luaC_changewhite(GCObject* x);
    
    /**
     * @brief 将灰色对象标记为黑色 - 对应官方gray2black
     * @param x 对象
     */
    void luaC_gray2black(GCObject* x);
    
    /**
     * @brief 将黑色对象标记为灰色 - 对应官方black2gray
     * @param x 对象
     */
    void luaC_black2gray(GCObject* x);
    
    /**
     * @brief 将白色对象标记为灰色 - 对应官方white2gray
     * @param x 对象
     */
    void luaC_white2gray(GCObject* x);
    
    // === GC参数配置 ===
    
    /**
     * @brief 设置GC暂停参数 - 对应官方gcpause
     * @param L Lua状态
     * @param pause 暂停参数(百分比)
     */
    void luaC_setgcpause(LuaState* L, int pause);
    
    /**
     * @brief 设置GC步长倍数 - 对应官方gcstepmul
     * @param L Lua状态
     * @param stepmul 步长倍数
     */
    void luaC_setgcstepmul(LuaState* L, int stepmul);
    
    /**
     * @brief 获取GC暂停参数
     * @param L Lua状态
     * @return 暂停参数
     */
    int luaC_getgcpause(LuaState* L);
    
    /**
     * @brief 获取GC步长倍数
     * @param L Lua状态
     * @return 步长倍数
     */
    int luaC_getgcstepmul(LuaState* L);
    
    // === 内存统计 ===
    
    /**
     * @brief 获取总分配字节数 - 对应官方totalbytes
     * @param L Lua状态
     * @return 总分配字节数
     */
    usize luaC_gettotalbytes(LuaState* L);
    
    /**
     * @brief 获取GC阈值 - 对应官方GCthreshold
     * @param L Lua状态
     * @return GC阈值
     */
    usize luaC_getthreshold(LuaState* L);
    
    /**
     * @brief 设置GC阈值
     * @param L Lua状态
     * @param threshold 新阈值
     */
    void luaC_setthreshold(LuaState* L, usize threshold);
    
    /**
     * @brief 获取内存使用估计 - 对应官方estimate
     * @param L Lua状态
     * @return 内存使用估计
     */
    usize luaC_getestimate(LuaState* L);
    
    /**
     * @brief 获取GC债务 - 对应官方gcdept
     * @param L Lua状态
     * @return GC债务
     */
    usize luaC_getgcdept(LuaState* L);
    
    // === 兼容性宏定义 ===
    
    // 位操作宏 - 对应官方lgc.h中的宏
    #define resetbits(x,m)      ((x) &= cast(u8, ~(m)))
    #define setbits(x,m)        ((x) |= (m))
    #define testbits(x,m)       ((x) & (m))
    #define bitmask(b)          (1<<(b))
    #define bit2mask(b1,b2)     (bitmask(b1) | bitmask(b2))
    #define l_setbit(x,b)       setbits(x, bitmask(b))
    #define resetbit(x,b)       resetbits(x, bitmask(b))
    #define testbit(x,b)        testbits(x, bitmask(b))
    #define set2bits(x,b1,b2)   setbits(x, (bit2mask(b1, b2)))
    #define reset2bits(x,b1,b2) resetbits(x, (bit2mask(b1, b2)))
    #define test2bits(x,b1,b2)  testbits(x, (bit2mask(b1, b2)))
    
    // 颜色位定义 - 对应官方lgc.h
    #define WHITE0BIT   0
    #define WHITE1BIT   1
    #define BLACKBIT    2
    #define FINALIZEDBIT 3
    #define KEYWEAKBIT  3
    #define VALUEWEAKBIT 4
    #define FIXEDBIT    5
    #define SFIXEDBIT   6
    #define WHITEBITS   bit2mask(WHITE0BIT, WHITE1BIT)
    
    // 颜色检查宏 - 对应官方lgc.h
    #define iswhite(x)      test2bits((x)->getGCMark(), WHITE0BIT, WHITE1BIT)
    #define isblack(x)      testbit((x)->getGCMark(), BLACKBIT)
    #define isgray(x)       (!isblack(x) && !iswhite(x))
    
    // 其他白色相关宏
    #define otherwhite(g)   ((g)->getCurrentWhite() ^ WHITEBITS)
    #define isdead(g,v)     ((v)->getGCMark() & otherwhite(g) & WHITEBITS)
    
    // 类型转换宏
    #define cast(t, exp)    ((t)(exp))
    
} // namespace Lua
