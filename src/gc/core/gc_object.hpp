#pragma once

#include "../../common/types.hpp"
#include "../utils/gc_types.hpp"

namespace Lua {
    // Forward declarations
    class GarbageCollector;
    class State;

    /**
     * @brief Base class for all garbage-collected objects in the Lua interpreter
     * 
     * This class provides the foundation for Lua's tri-color mark-and-sweep
     * garbage collection algorithm. All objects that need to be managed by
     * the garbage collector must inherit from this class.
     * 
     * The GC uses a tri-color marking algorithm:
     * - White: Objects that may be garbage (not yet visited)
     * - Gray: Objects that are reachable but whose children haven't been scanned
     * - Black: Objects that are reachable and whose children have been scanned
     */
    class GCObject {
    private:
        // GC marking information packed into a single byte
        // Bits 0-1: Color (White0, White1, Gray, Black)
        // Bit 2: Fixed object flag (never collected)
        // Bit 3: Finalized flag
        // Bit 4: Weak reference flag
        // Bit 5: Separated flag (for weak tables)
        // Bits 6-7: Reserved for future use
        mutable Atom<u8> gcMark{static_cast<u8>(GCColor::White0)};
        
        // Object type for efficient type checking during GC
        GCObjectType objectType;
        
        // Size of this object in bytes (for memory accounting)
        usize objectSize;
        
        // Next object in the allocation chain (intrusive linked list)
        GCObject* nextObject = nullptr;
        
        // Previous object in the allocation chain
        GCObject* prevObject = nullptr;
        
        // Finalizer state for objects that need cleanup
        FinalizerState finalizerState = FinalizerState::None;
        
        // Generation for generational GC (0 = young, 1+ = old)
        u8 generation = 0;
        
        // Reference count for debugging and optimization
        #ifdef DEBUG_GC
        mutable Atom<u32> debugRefCount{0};
        #endif
        
        friend class GarbageCollector;
        
    public:
        /**
         * @brief Construct a new GCObject
         * @param type The type of this object
         * @param size The size of this object in bytes
         */
        explicit GCObject(GCObjectType type, usize size = sizeof(GCObject))
            : objectType(type), objectSize(size) {}
        
        /**
         * @brief Virtual destructor for proper cleanup
         */
        virtual ~GCObject() = default;
        
        // Disable copy construction and assignment
        GCObject(const GCObject&) = delete;
        GCObject& operator=(const GCObject&) = delete;
        
        // Allow move construction and assignment
        GCObject(GCObject&& other) noexcept
            : gcMark(other.gcMark.load())
            , objectType(other.objectType)
            , objectSize(other.objectSize)
            , nextObject(other.nextObject)
            , prevObject(other.prevObject)
            , finalizerState(other.finalizerState)
            , generation(other.generation) {
            other.nextObject = nullptr;
            other.prevObject = nullptr;
        }
        
        GCObject& operator=(GCObject&& other) noexcept {
            if (this != &other) {
                gcMark.store(other.gcMark.load());
                objectType = other.objectType;
                objectSize = other.objectSize;
                nextObject = other.nextObject;
                prevObject = other.prevObject;
                finalizerState = other.finalizerState;
                generation = other.generation;
                other.nextObject = nullptr;
                other.prevObject = nullptr;
            }
            return *this;
        }
        
        /**
         * @brief Mark all objects referenced by this object
         * 
         * This method must be implemented by all derived classes to traverse
         * and mark all objects that this object references. This is crucial
         * for the mark phase of garbage collection.
         * 
         * @param gc Pointer to the garbage collector
         */
        virtual void markReferences(GarbageCollector* gc) = 0;
        
        /**
         * @brief Optional finalizer called before object destruction
         * 
         * Override this method if the object needs special cleanup before
         * being destroyed by the garbage collector. The finalizer is called
         * during the finalization phase of GC.
         * 
         * @param state Pointer to the Lua state
         */
        virtual void finalize(State* state) {}
        
        /**
         * @brief Optional finalizer called before object destruction (no state)
         * 
         * This overload is called when no Lua state is available during finalization.
         */
        virtual void finalize() {}
        
        /**
         * @brief Check if this object has a finalizer
         * @return true if the object needs finalization
         */
        virtual bool hasFinalizer() const { return false; }
        
        /**
         * @brief Check if this object needs finalization
         * @return true if the object has a finalizer and hasn't been finalized yet
         */
        bool needsFinalization() const {
            return hasFinalizer() && !isFinalized();
        }
        
        /**
         * @brief Get the size of this object for memory accounting
         * 
         * Override this method if the object's size changes dynamically
         * or includes additional allocated memory not reflected in objectSize.
         * 
         * @return Size in bytes
         */
        virtual usize getSize() const { return objectSize; }
        
        /**
         * @brief Get additional memory used by this object
         * 
         * For objects that allocate additional memory (like strings or tables),
         * this method should return the size of that additional memory.
         * 
         * @return Additional memory size in bytes
         */
        virtual usize getAdditionalSize() const { return 0; }
        
        // === GC Color Management ===
        
        /**
         * @brief Get the current GC color of this object
         * @return Current GC color
         */
        GCColor getColor() const {
            return GCUtils::getColor(gcMark.load(std::memory_order_acquire));
        }
        
        /**
         * @brief Set the GC color of this object
         * @param color New GC color
         */
        void setColor(GCColor color) {
            u8 currentMark = gcMark.load(std::memory_order_acquire);
            u8 newMark = GCUtils::setColor(currentMark, color);
            gcMark.store(newMark, std::memory_order_release);
        }
        
        /**
         * @brief Check if this object is white (potentially garbage)
         * @return true if the object is white
         */
        bool isWhite() const {
            return GCUtils::isWhite(gcMark.load(std::memory_order_acquire));
        }
        
        /**
         * @brief Check if this object is gray (marked but not traced)
         * @return true if the object is gray
         */
        bool isGray() const {
            return GCUtils::isGray(gcMark.load(std::memory_order_acquire));
        }
        
        /**
         * @brief Check if this object is black (marked and traced)
         * @return true if the object is black
         */
        bool isBlack() const {
            return GCUtils::isBlack(gcMark.load(std::memory_order_acquire));
        }
        
        // === Object Properties ===
        
        /**
         * @brief Get the type of this object
         * @return Object type
         */
        GCObjectType getType() const { return objectType; }
        
        /**
         * @brief Check if this object is fixed (never collected)
         * @return true if the object is fixed
         */
        bool isFixed() const {
            return GCUtils::isFixed(gcMark.load(std::memory_order_acquire));
        }
        
        /**
         * @brief Set the fixed flag for this object
         * @param fixed Whether the object should be fixed
         */
        void setFixed(bool fixed) {
            u8 currentMark = gcMark.load(std::memory_order_acquire);
            u8 newMark = GCUtils::setFixed(currentMark, fixed);
            gcMark.store(newMark, std::memory_order_release);
        }
        
        /**
         * @brief Check if this object has been finalized
         * @return true if the object has been finalized
         */
        bool isFinalized() const {
            return GCUtils::isFinalized(gcMark.load(std::memory_order_acquire));
        }
        
        /**
         * @brief Set the finalized flag for this object
         * @param finalized Whether the object has been finalized
         */
        void setFinalized(bool finalized) {
            u8 currentMark = gcMark.load(std::memory_order_acquire);
            u8 newMark = GCUtils::setFinalized(currentMark, finalized);
            gcMark.store(newMark, std::memory_order_release);
        }
        
        /**
         * @brief Get the generation of this object (for generational GC)
         * @return Generation number (0 = young, 1+ = old)
         */
        u8 getGeneration() const { return generation; }
        
        /**
         * @brief Set the generation of this object
         * @param gen New generation number
         */
        void setGeneration(u8 gen) { generation = gen; }
        
        /**
         * @brief Promote this object to the next generation
         */
        void promoteGeneration() {
            if (generation < 255) {
                generation++;
            }
        }
        
        /**
         * @brief Get the finalizer state
         * @return Current finalizer state
         */
        FinalizerState getFinalizerState() const { return finalizerState; }
        
        /**
         * @brief Set the finalizer state
         * @param state New finalizer state
         */
        void setFinalizerState(FinalizerState state) { finalizerState = state; }
        
        // === Linked List Management (for GC) ===
        
        /**
         * @brief Get the next object in the allocation chain
         * @return Pointer to next object or nullptr
         */
        GCObject* getNext() const { return nextObject; }
        
        /**
         * @brief Get the previous object in the allocation chain
         * @return Pointer to previous object or nullptr
         */
        GCObject* getPrev() const { return prevObject; }
        
        /**
         * @brief Set the next object in the allocation chain
         * @param next Pointer to next object
         */
        void setNext(GCObject* next) { nextObject = next; }
        
        /**
         * @brief Set the previous object in the allocation chain
         * @param prev Pointer to previous object
         */
        void setPrev(GCObject* prev) { prevObject = prev; }
        
        // === Atomic Operations for Thread Safety ===
        
        /**
         * @brief Atomically compare and swap the GC mark
         * @param expected Expected current value
         * @param desired New value to set
         * @return true if the swap was successful
         */
        bool compareAndSwapMark(u8 expected, u8 desired) {
            return gcMark.compare_exchange_weak(expected, desired, 
                std::memory_order_acq_rel, std::memory_order_acquire);
        }
        
        /**
         * @brief Atomically get the current GC mark
         * @return Current GC mark value
         */
        u8 getMark() const {
            return gcMark.load(std::memory_order_acquire);
        }
        
        /**
         * @brief Atomically set the GC mark
         * @param mark New mark value
         */
        void setMark(u8 mark) {
            gcMark.store(mark, std::memory_order_release);
        }
        
        #ifdef DEBUG_GC
        /**
         * @brief Increment debug reference count
         */
        void addRef() const {
            debugRefCount.fetch_add(1, std::memory_order_relaxed);
        }
        
        /**
         * @brief Decrement debug reference count
         */
        void removeRef() const {
            debugRefCount.fetch_sub(1, std::memory_order_relaxed);
        }
        
        /**
         * @brief Get debug reference count
         * @return Current reference count
         */
        u32 getRefCount() const {
            return debugRefCount.load(std::memory_order_relaxed);
        }
        #endif
        
    protected:
        /**
         * @brief Update the object size (for derived classes)
         * @param newSize New size in bytes
         */
        void updateSize(usize newSize) {
            objectSize = newSize;
        }
    };
    
    // === Utility Functions ===
    
    /**
     * @brief Cast a GCObject to a specific type with type checking
     * @tparam T Target type (must inherit from GCObject)
     * @param obj Object to cast
     * @param expectedType Expected GC object type
     * @return Pointer to T or nullptr if cast fails
     */
    template<typename T>
    T* gc_cast(GCObject* obj, GCObjectType expectedType) {
        if (obj && obj->getType() == expectedType) {
            return static_cast<T*>(obj);
        }
        return nullptr;
    }
    
    /**
     * @brief Safe cast with runtime type checking
     * @tparam T Target type
     * @param obj Object to cast
     * @return Pointer to T or nullptr if cast fails
     */
    template<typename T>
    T* gc_dynamic_cast(GCObject* obj) {
        return dynamic_cast<T*>(obj);
    }
    
    /**
     * @brief Check if an object is of a specific type
     * @param obj Object to check
     * @param type Expected type
     * @return true if the object is of the specified type
     */
    inline bool isObjectType(const GCObject* obj, GCObjectType type) {
        return obj && obj->getType() == type;
    }
    
    /**
     * @brief Get the total size of an object including additional memory
     * @param obj Object to measure
     * @return Total size in bytes
     */
    inline usize getTotalObjectSize(const GCObject* obj) {
        return obj ? (obj->getSize() + obj->getAdditionalSize()) : 0;
    }
}