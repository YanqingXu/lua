#pragma once

#include "../common/types.hpp"
#include "../gc/core/gc_object.hpp"
#include "../gc/core/gc_ref.hpp"
#include <memory>
#include <cstring>

namespace Lua {
    // Forward declarations
    class Table;
    class GarbageCollector;
    template<typename T> class GCRef;
    
    /**
     * @brief Userdata type enumeration
     * 
     * Lua 5.1 supports two types of userdata:
     * - Light userdata: Simple pointer wrapper, not managed by GC
     * - Full userdata: GC-managed memory block with optional metatable
     */
    enum class UserdataType : u8 {
        Light,      // Light userdata - external pointer, not GC managed
        Full        // Full userdata - GC managed memory block with metatable
    };
    
    /**
     * @brief Base class for all userdata objects in Lua
     * 
     * This class implements both light and full userdata according to Lua 5.1
     * specification. Light userdata stores external pointers while full userdata
     * manages its own memory block and supports metatables.
     * 
     * Memory layout for full userdata:
     * [Userdata header][User data block]
     * 
     * The user data block is aligned to 8-byte boundary for optimal performance.
     */
    class Userdata : public GCObject {
    private:
        UserdataType type_;              // Type of userdata (Light or Full)
        usize size_;                     // Size of user data in bytes
        void* data_;                     // Pointer to user data
        GCRef<Table> metatable_;         // Metatable (only for Full userdata)
        
        // For full userdata, data follows immediately after the object
        // For light userdata, data_ points to external memory
        
    public:
        /**
         * @brief Create light userdata from external pointer
         * 
         * Light userdata is not managed by the garbage collector and simply
         * wraps an external pointer. The caller is responsible for the lifetime
         * of the pointed-to memory.
         * 
         * @param ptr External pointer to wrap
         * @return GCRef to the created light userdata
         */
        static GCRef<Userdata> createLight(void* ptr);
        
        /**
         * @brief Create full userdata with specified size
         * 
         * Full userdata allocates a memory block of the specified size and
         * is managed by the garbage collector. The memory is zero-initialized.
         * 
         * @param size Size of the user data block in bytes
         * @return GCRef to the created full userdata
         */
        static GCRef<Userdata> createFull(usize size);
        
        /**
         * @brief Get the type of this userdata
         * @return Userdata type (Light or Full)
         */
        UserdataType getType() const noexcept { return type_; }
        
        /**
         * @brief Get pointer to the user data
         * @return Pointer to user data block
         */
        void* getData() const noexcept { return data_; }
        
        /**
         * @brief Get size of the user data in bytes
         * @return Size in bytes
         */
        usize getUserDataSize() const noexcept { return size_; }
        
        /**
         * @brief Get metatable for this userdata (Full userdata only)
         * @return GCRef to metatable or null for light userdata
         */
        GCRef<Table> getMetatable() const;
        
        /**
         * @brief Set metatable for this userdata (Full userdata only)
         * @param mt Metatable to set
         * @throws std::runtime_error if called on light userdata
         */
        void setMetatable(GCRef<Table> mt);

        /**
         * @brief Set metatable with write barrier support
         * @param mt Metatable to set
         * @param L Lua state for write barrier
         */
        void setMetatableWithBarrier(GCRef<Table> mt, LuaState* L);
        
        /**
         * @brief Check if this userdata has a metatable
         * @return true if metatable is set
         */
        bool hasMetatable() const;
        
        /**
         * @brief Get typed pointer to user data
         * @tparam T Type to cast to
         * @return Typed pointer or nullptr if size mismatch
         */
        template<typename T>
        T* getTypedData() const {
            if (sizeof(T) > size_) {
                return nullptr;
            }
            return static_cast<T*>(data_);
        }
        
        /**
         * @brief Set user data from typed object (Full userdata only)
         * @tparam T Type of object to copy
         * @param obj Object to copy into userdata
         * @return true if successful, false if size mismatch or light userdata
         */
        template<typename T>
        bool setTypedData(const T& obj) {
            if (type_ == UserdataType::Light || sizeof(T) > size_) {
                return false;
            }
            std::memcpy(data_, &obj, sizeof(T));
            return true;
        }
        
        // === GCObject Interface ===
        
        /**
         * @brief Mark all objects referenced by this userdata
         * @param gc Garbage collector instance
         */
        void markReferences(GarbageCollector* gc) override;
        
        /**
         * @brief Get the total size of this object including user data
         * @return Total size in bytes
         */
        usize getSize() const override;
        
        /**
         * @brief Get additional size beyond the base object
         * @return Additional size in bytes
         */
        usize getAdditionalSize() const override;
        
        /**
         * @brief Finalize this userdata object
         * 
         * Called by the garbage collector before the object is destroyed.
         * Can be overridden by derived classes for custom cleanup.
         */
        virtual void finalize();
        
    protected:
        /**
         * @brief Protected constructor for light userdata
         * @param ptr External pointer
         */
        explicit Userdata(void* ptr);
        
        /**
         * @brief Protected constructor for full userdata
         * @param size Size of user data block
         */
        explicit Userdata(usize size);
        
        /**
         * @brief Virtual destructor
         */
        virtual ~Userdata();
        
    private:
        // Disable copy and move operations
        Userdata(const Userdata&) = delete;
        Userdata(Userdata&&) = delete;
        Userdata& operator=(const Userdata&) = delete;
        Userdata& operator=(Userdata&&) = delete;
        
        /**
         * @brief Initialize full userdata memory layout
         */
        void initializeFullUserdata();
        
        friend class GarbageCollector;
    };
    
    // === Utility Functions ===
    
    /**
     * @brief Create light userdata from typed pointer
     * @tparam T Type of object
     * @param ptr Pointer to object
     * @return GCRef to light userdata
     */
    template<typename T>
    GCRef<Userdata> makeLightUserdata(T* ptr) {
        return Userdata::createLight(static_cast<void*>(ptr));
    }
    
    /**
     * @brief Create full userdata for typed object
     * @tparam T Type of object
     * @param obj Object to store in userdata
     * @return GCRef to full userdata containing the object
     */
    template<typename T>
    GCRef<Userdata> makeFullUserdata(const T& obj) {
        auto ud = Userdata::createFull(sizeof(T));
        ud->setTypedData(obj);
        return ud;
    }
    
    /**
     * @brief Check if a userdata contains a specific type
     * @tparam T Type to check for
     * @param ud Userdata to check
     * @return true if userdata can hold type T
     */
    template<typename T>
    bool isUserdataType(const GCRef<Userdata>& ud) {
        return ud && ud->getUserDataSize() >= sizeof(T);
    }
}
