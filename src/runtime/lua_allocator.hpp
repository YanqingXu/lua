#pragma once

/**
 * @file lua_allocator.hpp
 * @brief Mutable Lua 5.1 allocator callback and user-data ownership.
 */

#include <cstddef>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <type_traits>
#include <vector>

namespace Lua {

namespace AllocatorDetail {

inline std::mutex& implementationAllocationMutex() {
    static std::mutex mutex;
    return mutex;
}

inline std::vector<void*>& implementationAllocations() {
    static std::vector<void*> allocations;
    return allocations;
}

inline void trackImplementationAllocation(void* pointer) {
    std::scoped_lock lock(implementationAllocationMutex());
    implementationAllocations().push_back(pointer);
}

inline bool releaseImplementationAllocation(void* pointer) noexcept {
    std::scoped_lock lock(implementationAllocationMutex());
    auto& allocations = implementationAllocations();
    for (auto it = allocations.begin(); it != allocations.end(); ++it) {
        if (*it == pointer) {
            allocations.erase(it);
            return true;
        }
    }
    return false;
}

} // namespace AllocatorDetail

using LuaAllocatorFunction = void* (*)(void* userData, void* pointer, std::size_t oldSize, std::size_t newSize);

class LuaAllocator {
public:
    LuaAllocator() = default;

    LuaAllocator(LuaAllocatorFunction function, void* userData) noexcept : function_(function), userData_(userData) {}

    [[nodiscard]] bool isConfigured() const noexcept {
        return function_ != nullptr;
    }

    [[nodiscard]] void* allocate(std::size_t size) const noexcept {
        return function_ != nullptr ? function_(userData_, nullptr, 0, size) : nullptr;
    }

    [[nodiscard]] void* reallocate(void* pointer, std::size_t oldSize, std::size_t newSize) const noexcept {
        return function_ != nullptr ? function_(userData_, pointer, oldSize, newSize) : nullptr;
    }

    void deallocate(void* pointer, std::size_t oldSize) const noexcept {
        if (AllocatorDetail::releaseImplementationAllocation(pointer)) {
            ::operator delete(pointer);
            return;
        }
        if (function_ != nullptr && pointer != nullptr) {
            (void)function_(userData_, pointer, oldSize, 0);
        }
    }

    void set(LuaAllocatorFunction function, void* userData) noexcept {
        function_ = function;
        userData_ = userData;
    }

    [[nodiscard]] LuaAllocatorFunction getFunction() const noexcept {
        return function_;
    }

    [[nodiscard]] void* getUserData() const noexcept {
        return userData_;
    }

private:
    LuaAllocatorFunction function_ = nullptr;
    void* userData_ = nullptr;
};

template <typename T> class LuaStdAllocator {
public:
    using value_type = T;
    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::false_type;

    LuaStdAllocator() noexcept = default;
    explicit LuaStdAllocator(LuaAllocator* allocator) noexcept : allocator_(allocator) {}

    template <typename U>
    LuaStdAllocator(const LuaStdAllocator<U>& other) noexcept : allocator_(other.getLuaAllocator()) {}

    [[nodiscard]] T* allocate(std::size_t count) {
        if (count > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
            throw std::bad_array_new_length();
        }
        if (count == 0) {
            // lua_Alloc reserves nsize == 0 for deallocation, while standard
            // containers are permitted to request a zero-sized allocation.
            return std::allocator<T>{}.allocate(count);
        }
        if (isImplementationMetadata()) {
            // MSVC Debug rebinds user allocators for _Container_proxy. Keep
            // that implementation-only block outside Lua's failure contract,
            // while tracking it because destruction may arrive through a
            // differently rebound allocator specialization.
            T* memory = static_cast<T*>(::operator new(count * sizeof(T)));
            if (allocator_ != nullptr && allocator_->isConfigured()) {
                try {
                    AllocatorDetail::trackImplementationAllocation(memory);
                } catch (...) {
                    ::operator delete(memory);
                    throw;
                }
            }
            return memory;
        }

        if (allocator_ == nullptr || !allocator_->isConfigured()) {
            return std::allocator<T>{}.allocate(count);
        }

        void* memory = allocator_->allocate(count * sizeof(T));
        if (memory == nullptr) {
            throw std::bad_alloc();
        }
        return static_cast<T*>(memory);
    }

    void deallocate(T* pointer, std::size_t count) noexcept {
        if (pointer == nullptr) {
            return;
        }
        const bool implementationAllocation = AllocatorDetail::releaseImplementationAllocation(pointer);
        if (implementationAllocation) {
            ::operator delete(pointer);
            return;
        }
        if (count == 0 || isImplementationMetadata()) {
            std::allocator<T>{}.deallocate(pointer, count);
            return;
        }
        if (allocator_ == nullptr || !allocator_->isConfigured()) {
            std::allocator<T>{}.deallocate(pointer, count);
            return;
        }
        allocator_->deallocate(pointer, count * sizeof(T));
    }

    [[nodiscard]] LuaAllocator* getLuaAllocator() const noexcept {
        return allocator_;
    }

    template <typename U> bool operator==(const LuaStdAllocator<U>& other) const noexcept {
        return allocator_ == other.getLuaAllocator();
    }

    template <typename U> bool operator!=(const LuaStdAllocator<U>& other) const noexcept {
        return !(*this == other);
    }

private:
    static constexpr bool isImplementationMetadata() noexcept {
#ifdef _MSC_VER
        return std::is_same_v<std::remove_cv_t<T>, std::_Container_proxy>;
#else
        return false;
#endif
    }

    LuaAllocator* allocator_ = nullptr;
};

template <typename T> using LuaVector = std::vector<T, LuaStdAllocator<T>>;

} // namespace Lua
