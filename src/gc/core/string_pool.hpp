#pragma once

#include "../../common/types.hpp"
#include "gc_string.hpp"
#include "../utils/gc_types.hpp"  // 为luaS_fix宏添加GCUtils支持
#include <mutex>
#include <iostream>
#include <cstring>

namespace Lua {
    // Forward declarations
    class GarbageCollector;
    
    /**
     * @brief String pool for string interning
     * 
     * This class implements string interning to reduce memory usage by ensuring
     * that identical strings share the same memory location. It maintains a pool
     * of all created GCString objects and returns existing instances when possible.
     * 
     * The string pool is integrated with the garbage collector to ensure proper
     * memory management of interned strings.
     */
    class StringPool {
    private:
        // Hash set to store unique GCString objects
        HashSet<GCString*, GCStringHash, GCStringEqual> pool;
        
        // Mutex for thread safety
        mutable std::mutex poolMutex;
        


        // Private constructor for singleton pattern
        StringPool() {
        }
        
    public:
        // Disable copy and move operations
        StringPool(const StringPool&) = delete;
        StringPool& operator=(const StringPool&) = delete;
        StringPool(StringPool&&) = delete;
        StringPool& operator=(StringPool&&) = delete;
        
        /**
         * @brief Get the singleton instance of the string pool
         * @return Reference to the string pool instance
         */
        static StringPool& getInstance();
        
        /**
         * @brief Intern a string (create or return existing)
         * @param str The string content
         * @return Pointer to the interned GCString
         */
        GCString* intern(const Str& str);
        
        /**
         * @brief Intern a C-style string (create or return existing)
         * @param cstr The C-style string
         * @return Pointer to the interned GCString
         */
        GCString* intern(const char* cstr);
        
        /**
         * @brief Intern a string with move semantics (create or return existing)
         * @param str The string content to move
         * @return Pointer to the interned GCString
         */
        GCString* intern(Str&& str);
        
        /**
         * @brief Remove a string from the pool
         * 
         * This method is called by the garbage collector when a string
         * is being collected to remove it from the pool.
         * 
         * @param gcString The string to remove
         */
        void remove(GCString* gcString);
        
        /**
         * @brief Mark all strings in the pool as reachable
         * 
         * This method is called during the mark phase of garbage collection
         * to mark all strings in the pool as reachable from roots.
         * 
         * @param gc Pointer to the garbage collector
         */
        void markAll(GarbageCollector* gc);
        
        /**
         * @brief Get the number of strings in the pool
         * @return Number of interned strings
         */
        usize size() const;
        
        /**
         * @brief Check if the pool is empty
         * @return true if the pool is empty
         */
        bool empty() const;
        
        /**
         * @brief Clear all strings from the pool
         * 
         * This method should only be called during shutdown or
         * when all strings are known to be unreachable.
         */
        void clear();
        
        /**
         * @brief Get memory usage statistics
         * @return Total memory used by the string pool in bytes
         */
        usize getMemoryUsage() const;
        
        /**
         * @brief Get all strings in the pool for GC marking
         * @return Vector containing all GCString pointers in the pool
         */
        std::vector<GCString*> getAllStrings() const;

    };

    // Lua 5.1兼容的字符串函数 - 渐进式添加

    /**
     * @brief 创建新字符串 - 对应官方luaS_newlstr
     * @param L Lua状态（暂时可以为nullptr）
     * @param str 字符串内容
     * @param len 字符串长度
     * @return 新创建的字符串对象
     */
    GCString* luaS_newlstr(void* L, const char* str, size_t len);

    /**
     * @brief Lua 5.1兼容的字符串哈希算法
     * @param str 字符串内容
     * @param len 字符串长度
     * @return 哈希值
     */
    u32 luaS_hash(const char* str, size_t len);

    /**
     * @brief 创建新字符串 - 对应官方luaS_new
     * @param L Lua状态（暂时可以为nullptr）
     * @param s C风格字符串
     * @return 新创建的字符串对象
     */
    #define luaS_new(L, s) (luaS_newlstr(L, s, strlen(s)))

    /**
     * @brief 创建字面量字符串 - 对应官方luaS_newliteral
     * @param L Lua状态
     * @param s 字符串字面量
     * @return 新创建的字符串对象
     *
     * 这个宏在编译时计算字符串长度，避免运行时strlen调用
     */
    #define luaS_newliteral(L, s) (luaS_newlstr(L, "" s, (sizeof(s)/sizeof(char))-1))

    /**
     * @brief 固定字符串 - 对应官方luaS_fix
     * @param s 要固定的字符串
     *
     * 将字符串标记为固定，防止被垃圾收集器回收。
     * 通常用于重要的系统字符串，如元方法名称等。
     */
    #define luaS_fix(s) GCUtils::setfixed(static_cast<GCObject*>(s))

    /**
     * @brief 调整字符串表大小 - 对应官方luaS_resize
     * @param L Lua状态（暂时可以为nullptr）
     * @param newsize 新的字符串表大小
     *
     * 动态调整字符串表大小以优化哈希性能。
     * 当字符串数量增长时，增大表大小可以减少哈希冲突。
     */
    void luaS_resize(void* L, int newsize);

}