#pragma once
#include <cstdint>
#include <cstddef>

// Core Defs

#define CACHE_LINE_SIZE 64u

inline uintptr_t compute_aligned_address(uintptr_t addr, size_t alignment) noexcept {
    return (addr + (alignment - 1)) & ~(alignment - 1);
}

// Main Allocator for the Engine (Implemented as a Free List)
// Intended to contain Large Heap Allocations that persist throughout the execution of the program
class MemoryAllocator {
private:
    // Free List
    struct Region {
        size_t size{}; // Total Size Available (Excludes this Header)
        size_t padding{}; // Any Padding applied BEFORE this Header (Useful when Freeing Regions)
        Region* next = nullptr;
    };
    
    Region* head;

    // Heap Allocation
    void* base_ptr;
    size_t total_size;

public:
    // Allocate on Construct
    explicit MemoryAllocator(size_t num_bytes);
    // Free on Destruct
    ~MemoryAllocator();

    // Non-Copyable
    MemoryAllocator(const MemoryAllocator&) = delete;
    MemoryAllocator& operator=(const MemoryAllocator&) = delete;
    
    // Allocate from a Free Region
    void* Allocate(size_t bytes, size_t alignment);
    void Free(void* ptr);
};
