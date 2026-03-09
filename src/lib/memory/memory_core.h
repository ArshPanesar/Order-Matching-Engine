#pragma once
#include "util/errors.h"
#include "util/hints.h"
#include <cstdint>
#include <cstddef>
#include <bit>

// Core Defs

#define MAX_ALIGNMENT 64

inline uintptr_t ComputeAlignedAddress(uintptr_t addr, size_t alignment) noexcept {
    return (addr + (alignment - 1)) & ~(alignment - 1);
}

inline size_t NextPowerOf2(size_t n) {
    return std::bit_ceil(n);
}

// Main Allocator for the Engine (FreeList Implementation)
// Intended to contain Large Heap Allocations that persist throughout the execution of the program
// Alignment per Allocation Request must be a Power of 2 and Bounded to MAX_ALIGNMENT 
class MemoryAllocator {
private:
    // Free List Header
    struct alignas(MAX_ALIGNMENT) Region {
        size_t size{}; // Total Size Available (Excluding Size of this Header)
        Region* next = nullptr; // Next available Free Region
    };
    static_assert(sizeof(Region) % MAX_ALIGNMENT == 0);
    
    Region* head;

    // Heap Allocation
    void* base_ptr;
    size_t total_size;

    // Stats
    size_t free_bytes;
    size_t allocated_bytes;

    bool CanCoalesce(Region* prev, Region* next);

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

    // Total Number of Bytes Allocated
    size_t GetAllocatedBytes() const;
    // Total Number of Free Bytes
    size_t GetFreeBytes() const;
    // Total Number of Heap Bytes
    size_t GetTotalBytes() const; 

private:

#ifdef LOB_DEBUG
    // Internal Verification Tests
    //
    // Verify Regions exist in correct Order of Addresses (Should be Strictly Increasing)
    void VerifyRegionsIncreasingOrder();
    // Verify Regions do not overlap
    void VerifyNonOverlappingRegions();
    // Verify No Adjacent Regions exist
    void VerifyCoalescenceImpossible();
    // Verify No Region with Size less than zero exists in the FreeList
    void VerifyNonZeroRegions();
    // Verify Total Number of Free Bytes is always equal to internal tracker `free_bytes`
    void VerifyFreeBytes();

    void RunVerificationTests();
#endif //LOB_DEBUG

};
