#pragma once
#include <cstdint>
#include <cstddef>

// Core Defs

#define CACHE_LINE_SIZE 64

inline uintptr_t ComputeAlignedAddress(uintptr_t addr, size_t alignment) noexcept {
    return (addr + (alignment - 1)) & ~(alignment - 1);
}

// Main Allocator for the Engine (FreeList Implementation)
// Intended to contain Large Heap Allocations that persist throughout the execution of the program
class MemoryAllocator {
private:
    // Free List
    struct Region {
        size_t size{}; // Total Size Available (Excluding Size of this Header)
        size_t padding{}; // Any Padding applied BEFORE this Header (Used when Freeing Regions)
        Region* next = nullptr; // Next available Free Region
    };
    
    Region* head;

    // Heap Allocation
    void* base_ptr;
    size_t total_size;

    // Stats
    size_t free_bytes; // Excluding Header Size
    size_t allocated_bytes; // Excluding Header Size and Padding

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

    // Total Number of Bytes Allocated (excluding padding/header bytes)
    size_t GetAllocatedBytes() const;
    // Total Number of Free Bytes (excluding header bytes)
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
