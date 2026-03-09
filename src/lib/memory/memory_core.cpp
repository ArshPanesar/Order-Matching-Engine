#include <cassert>
#include <cstring>
#include <cstddef>
#include <stdlib.h>
#include <new>

#include "memory_core.h"

bool MemoryAllocator::CanCoalesce(Region* prev, Region* next) {
    uintptr_t prev_addr = reinterpret_cast<uintptr_t>(prev);
    uintptr_t next_addr = reinterpret_cast<uintptr_t>(next);
    
    uintptr_t prev_end = prev_addr + sizeof(Region) + prev->size;

    return prev_end == next_addr;
}

MemoryAllocator::MemoryAllocator(size_t num_bytes) : 
    head(nullptr),
    base_ptr(nullptr),
    total_size(num_bytes),
    free_bytes(0u),
    allocated_bytes(0u) {
    // Additional Bytes for Header
    size_t required_size = num_bytes + sizeof(Region);
    required_size = ComputeAlignedAddress(required_size, MAX_ALIGNMENT);

    // Allocate Buffer
    base_ptr = ::operator new(required_size, std::align_val_t(MAX_ALIGNMENT));
    assert(base_ptr != nullptr);
    
    total_size = required_size;
    memset(base_ptr, 0, total_size);

    // Initialize Free List
    head = reinterpret_cast<Region*>(base_ptr);
    head->size = total_size - sizeof(Region);
    head->next = nullptr;

    free_bytes = head->size;
}

MemoryAllocator::~MemoryAllocator() {
    // Immediately Clear all Acquired Heap Memory
    // FreeList Pointers are Invalidated, but will never be accessed after this point.
    ::operator delete(base_ptr, std::align_val_t(MAX_ALIGNMENT));
}

void *MemoryAllocator::Allocate(size_t bytes, size_t alignment) {

#ifdef LOB_DEBUG
    RunVerificationTests();
#endif //LOB_DEBUG

    // Check Request Validity
    if (alignment == 0u)
        alignment = alignof(std::max_align_t);
    assert(bytes > 0 && (alignment & (alignment - 1)) == 0);
    assert(alignment <= MAX_ALIGNMENT);

    // Find First Fit
    Region* avail = head;
    Region* prev = nullptr;
    while (avail != nullptr) {
        // Check if enough size is available
        if (avail->size >= bytes) {
            size_t removed_bytes = 0u;

            // Available Addresses will always aligned for powers of 2 upto MAX_ALIGNMENT
            uintptr_t aligned_avail_addr = reinterpret_cast<uintptr_t>(avail) + sizeof(Region);
            // Ending Address of this Allocation
            uintptr_t alloc_end_addr = aligned_avail_addr + bytes;

            // Possible Address for a New Region (if this Region can be split)
            uintptr_t new_region_aligned_addr = ComputeAlignedAddress(alloc_end_addr, MAX_ALIGNMENT); 

            // Ending Address of this Region
            uintptr_t avail_end_addr = aligned_avail_addr + avail->size;

            // Allocation Request to be fulfilled by Addresses [aligned_avail_addr, aligned_avail_addr + bytes]
            // Check if Extra Space is available (to be reused by the FreeList)
            if (avail_end_addr > (new_region_aligned_addr + sizeof(Region))) {
                // Place the New Region Header
                Region* leftover = reinterpret_cast<Region*>(new_region_aligned_addr);
                leftover->size = avail_end_addr - new_region_aligned_addr - sizeof(Region);
                leftover->next = avail->next;

                // Add to Free List
                if (prev != nullptr)
                    prev->next = leftover;
                else
                    head = leftover;

                // Store New Size of Allocated Region
                avail->size = bytes;
                // Removed Bytes: Caller Allocated, Leftover Header and Padding
                removed_bytes = bytes + sizeof(Region) + (new_region_aligned_addr - alloc_end_addr);
            } else {
                // Not enough leftover space, use entire Region
                // Add to Free List
                if (prev != nullptr)
                    prev->next = avail->next;
                else
                    head = avail->next;

                avail->size = avail_end_addr - aligned_avail_addr;
                removed_bytes = avail->size;
            }

            // Region No Longer Free
            avail->next = nullptr;

            // Track Usage
            free_bytes -= removed_bytes;
            allocated_bytes += avail->size;

#ifdef LOB_DEBUG
            RunVerificationTests();
#endif //LOB_DEBUG

            assert(aligned_avail_addr % alignment == 0);
            assert(aligned_avail_addr % alignof(std::max_align_t) == 0);
            assert(alloc_end_addr > aligned_avail_addr);
            assert(alloc_end_addr - aligned_avail_addr >= bytes);
            

            // Return Aligned Pointer
            return reinterpret_cast<void*>(aligned_avail_addr);
        }
        
        // Move to Next Free Region
        prev = avail;
        avail = avail->next;
    }
    
    assert(false && "MemoryAllocator ran out of memory!");
    return nullptr;
}

void MemoryAllocator::Free(void *ptr) {
#ifdef LOB_DEBUG
    RunVerificationTests();
#endif //LOB_DEBUG

    if (!ptr)
        return;
    
    // Ensure Address Fits within the Heap Allocated Block
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    assert(addr >= ((uintptr_t)base_ptr + sizeof(Region)) && "MemoryAllocator tried to free invalid pointer!");
    assert(addr <= ((uintptr_t)base_ptr + total_size) && "MemoryAllocator tried to free invalid pointer!");
    
    // Move Address Backwards to get the Header
    uintptr_t orig_region_addr = addr - sizeof(Region);
    
    // Extract Header Info
    Region* orig_region = reinterpret_cast<Region*>(orig_region_addr);
    assert(orig_region->next == nullptr);

    size_t alloc_size = orig_region->size;

    free_bytes += alloc_size;
    allocated_bytes -= alloc_size;

    if (head == nullptr) {
        // No Other Free Blocks
        head = orig_region;

#ifdef LOB_DEBUG
    RunVerificationTests();
#endif //LOB_DEBUG
        return;
    }

    // Add Free Region back to the List by Coalescing
    //
    // Find where the Region should exist
    Region* prev = nullptr;
    Region* curr = head;
    while (curr != nullptr) {
        if (curr > orig_region)
            break;
        prev = curr;
        curr = curr->next;
    }
    // Order should be prev -> orig_region -> curr
    orig_region->next = curr;
    if (prev != nullptr)
        prev->next = orig_region;
    else
        head = orig_region;

    // Try Merging with Next Region
    if (curr != nullptr && CanCoalesce(orig_region, curr)) {
        orig_region->size += sizeof(Region) + curr->size; // Padding not included since Free Regions should always have 0 Padding
        orig_region->next = curr->next;

        free_bytes += sizeof(Region);
    }
    // Try Merging with Previous Region
    if (prev != nullptr && CanCoalesce(prev, orig_region)) {
        prev->size += sizeof(Region) + orig_region->size; // Padding not included since Free Regions should always have 0 Padding
        prev->next = orig_region->next;

        free_bytes += sizeof(Region);
    }
    
#ifdef LOB_DEBUG
    RunVerificationTests();
#endif //LOB_DEBUG
}

size_t MemoryAllocator::GetAllocatedBytes() const {
    return allocated_bytes;
}

size_t MemoryAllocator::GetFreeBytes() const {
    return free_bytes;
}

size_t MemoryAllocator::GetTotalBytes() const {
    return total_size;
}


//
// Internal Verification
//


#ifdef LOB_DEBUG

void MemoryAllocator::VerifyRegionsIncreasingOrder() {
    if (head == nullptr || head->next == nullptr)
        return;
    
    Region* prev = head;
    Region* curr = head->next;
    uintptr_t prev_addr = reinterpret_cast<uintptr_t>(prev);
    uintptr_t curr_addr = reinterpret_cast<uintptr_t>(curr);

    while (curr != nullptr) {
        if (curr_addr <= prev_addr)
            assert(false && "INTERNAL TEST FAILED: VerifyRegionsIncreasingOrder()");
        
        prev = curr;
        curr = curr->next;

        prev_addr = reinterpret_cast<uintptr_t>(prev);
        curr_addr = reinterpret_cast<uintptr_t>(curr);
    }
}

void MemoryAllocator::VerifyNonOverlappingRegions() {
    if (head == nullptr || head->next == nullptr)
        return;
    
    Region* prev = head;
    Region* curr = head->next;
    uintptr_t prev_addr = reinterpret_cast<uintptr_t>(prev);
    uintptr_t curr_addr = reinterpret_cast<uintptr_t>(curr);

    while (curr != nullptr) {
        size_t prev_end = prev_addr + prev->size;
        if (curr_addr <= prev_end)
            assert(false && "INTERNAL TEST FAILED: VerifyNonOverlappingRegions()");
        
        prev = curr;
        curr = curr->next;

        prev_addr = reinterpret_cast<uintptr_t>(prev);
        curr_addr = reinterpret_cast<uintptr_t>(curr);
    }
}

void MemoryAllocator::VerifyCoalescenceImpossible() {
    if (head == nullptr)
        return;
    
    Region* prev = head;
    Region* curr = head->next;
    while (curr != nullptr) {
        if (CanCoalesce(prev, curr))
            assert(false && "INTERNAL TEST FAILED: VerifyCoalescenceImpossible()");
        
        prev = curr;
        curr = curr->next;
    }
}

void MemoryAllocator::VerifyNonZeroRegions() {
    Region* curr = head;
    while (curr != nullptr) {
        if (curr->size <= 0)
            assert(false && "INTERNAL TEST FAILED: VerifyNonZeroRegions()");
        
        curr = curr->next;
    }
}

void MemoryAllocator::VerifyFreeBytes() {
    Region* curr = head;
    size_t count_bytes = 0u;
    while (curr != nullptr) {
        count_bytes += curr->size;
        curr = curr->next;
    }

    assert(count_bytes == free_bytes && "INTERNAL TEST FAILED: VerifyFreeBytes()");
}

void MemoryAllocator::RunVerificationTests() {
    VerifyRegionsIncreasingOrder();
    VerifyNonOverlappingRegions();
    VerifyCoalescenceImpossible();
    VerifyNonZeroRegions();
    VerifyFreeBytes();
}

#endif //LOB_DEBUG