#include <cassert>
#include <cstring>
#include <cstddef>
#include <stdlib.h>

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
    
    // Allocate Buffer
    base_ptr = malloc(required_size);
    assert(base_ptr != nullptr);
    
    total_size = required_size;
    memset(base_ptr, 0, total_size);

    // Initialize Free List
    head = reinterpret_cast<Region*>(base_ptr);
    head->size = total_size - sizeof(Region);
    head->padding = 0u; // No padding for initial memory region
    head->next = nullptr;

    free_bytes = head->size;
}

MemoryAllocator::~MemoryAllocator() {
    // Immediately Clear all Acquired Heap Memory
    // FreeList Pointers are Invalidated, but will never be accessed after this point.
    free(base_ptr);
}

void *MemoryAllocator::Allocate(size_t bytes, size_t alignment) {

#ifdef LOB_DEBUG
    RunVerificationTests();
#endif //LOB_DEBUG

    // Check Request Validity
    if (alignment == 0u)
        alignment = alignof(std::max_align_t);
    assert(bytes > 0 && (alignment & (alignment - 1)) == 0);

    // Find First Fit
    Region* avail = head;
    Region* prev = nullptr;
    while (avail != nullptr) {
        // Align Address
        uintptr_t avail_addr = reinterpret_cast<uintptr_t>(avail) + sizeof(Region);
        uintptr_t aligned_addr = ComputeAlignedAddress(avail_addr, alignment);
        size_t padding = aligned_addr - avail_addr;

        // Find Available Block
        size_t required_size = bytes + padding;
        size_t avail_size = avail->size;
        if (required_size <= avail_size) {
            // Can this Region be Divided?
            size_t leftover_size = avail_size - required_size;
            if (leftover_size > sizeof(Region)) { // Size must exceed atleast Header
                // Divide Regions
                uintptr_t leftover_region_addr = aligned_addr + bytes;

                // Place New Header at Leftover Region
                Region* leftover = reinterpret_cast<Region*>(leftover_region_addr);
                leftover->size = leftover_size - sizeof(Region);
                leftover->padding = 0u;

                // Remove Available Block from List and Add Leftover Block to List
                leftover->next = avail->next;
                if (prev != nullptr)
                    prev->next = leftover;
                else
                    head = leftover;
            } else {
                // No Division Possible, Use Entire Region
                if (prev != nullptr)
                    prev->next = avail->next;
                else
                    head = avail->next;
            }

            // Move Available Region's Header right before the aligned address
            // This enables Free(ptr) to easily access the Header for any valid ptr 
            uintptr_t moved_header_addr = aligned_addr - sizeof(Region);
            Region* requested = reinterpret_cast<Region*>(moved_header_addr);
            requested->size = bytes;
            requested->padding = padding; // Behind this Header
            requested->next = nullptr; // Region is no longer free
            
            // Consider Leftover Size for Free Bytes tracking as well       
            free_bytes = (leftover_size > sizeof(Region)) ? free_bytes - required_size - sizeof(Region) : free_bytes - avail_size;
            allocated_bytes += bytes;

#ifdef LOB_DEBUG
            RunVerificationTests();
#endif //LOB_DEBUG

            return reinterpret_cast<void*>(aligned_addr);
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
    uintptr_t header_addr = addr - sizeof(Region);
    
    // Extract Header Info
    Region* alloc_region = reinterpret_cast<Region*>(header_addr);
    size_t alloc_size = alloc_region->size;
    size_t alloc_padding = alloc_region->padding;

    // Get Original Placement of Header
    uintptr_t orig_region_addr = header_addr - alloc_padding;

    // Recreate Free Region
    Region* orig_region = reinterpret_cast<Region*>(orig_region_addr);
    orig_region->size = alloc_size + alloc_padding; // Exclude Header Size
    orig_region->padding = 0u;

    free_bytes += alloc_size + alloc_padding;
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
        orig_region->padding = 0u;
        orig_region->next = curr->next;

        free_bytes += sizeof(Region);
    }
    // Try Merging with Previous Region
    if (prev != nullptr && CanCoalesce(prev, orig_region)) {
        prev->size += sizeof(Region) + orig_region->size; // Padding not included since Free Regions should always have 0 Padding
        prev->padding = 0u;
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