#include <cassert>
#include <cstring>
#include <cstddef>
#include <stdlib.h>

#include "memory_core.h"

MemoryAllocator::MemoryAllocator(size_t num_bytes) :
    head(nullptr),
    base_ptr(nullptr),
    total_size(num_bytes) {
    
    // Additional Bytes for Header
    size_t total_bytes = num_bytes + sizeof(Region);
    
    // Allocated Size must be a multiple of the Alignment
    size_t alignment = alignof(Region); // Align to Free List Header
    total_size = (total_bytes + (alignment - 1)) & ~(alignment - 1);
    base_ptr = aligned_alloc(alignment, total_size);
    assert(base_ptr != nullptr);
    
    memset(base_ptr, 0, total_size);

    // Initialize Free List
    head = reinterpret_cast<Region*>(base_ptr);
    head->size = total_size - sizeof(Region);
    head->padding = 0u; // We don't care about any initial padding since we only need the requested total_size
    head->next = nullptr;
}

MemoryAllocator::~MemoryAllocator() {
    // Immediately Clear all Acquired Heap Memory
    // FreeList Pointers are Invalidated, but will never be accessed after this point.
    free(base_ptr);
}

void *MemoryAllocator::Allocate(size_t bytes, size_t alignment) {
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
        uintptr_t aligned_addr = compute_aligned_address(avail_addr, alignment);
        size_t padding = aligned_addr - avail_addr;

        // Find Available Block
        size_t required_size = bytes + padding;
        if (required_size <= avail->size) {
            // Can this Region be Divided?
            size_t leftover_size = avail->size - required_size;
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
            // Gap left after move is exactly equal to required Padding! 
            uintptr_t moved_header_addr = aligned_addr - sizeof(Region);
            Region* requested = reinterpret_cast<Region*>(moved_header_addr);
            requested->size = bytes;
            requested->padding = padding; // Behind this Header
            requested->next = nullptr; // Out of List so we don't care
            
            return reinterpret_cast<void*>(aligned_addr);
        }
        
        // Move to Next Free Region
        prev = avail;
        avail = avail->next;
    }

    assert(false && "RAN OUT OF MEMORY!");
    return nullptr;
}

void MemoryAllocator::Free(void *ptr) {
    if (!ptr)
        return;
    
    // Move Address Backwards to get the Header
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
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

    if (head == nullptr) {
        // No Other Free Blocks
        head = orig_region;
        return;
    }

    // Add Free Region back to List by Coalescing
    // Find where Region should exist
    Region* prev = nullptr;
    Region* curr = head;
    while (curr != nullptr) {
        if (curr > orig_region)
            break;
        prev = curr;
        curr = curr->next;
    }
    // If Merge Not Possible, then the order will be prev -> orig_region -> curr
    //
    // Try Merging with Next Region
    if (curr != nullptr) {
        uintptr_t curr_addr = reinterpret_cast<uintptr_t>(curr);
        uintptr_t orig_region_end = orig_region_addr + sizeof(Region) + orig_region->size;
        if (curr_addr == orig_region_end) {
            // Merge
            orig_region->size += sizeof(Region) + curr->size; // Padding not included since Free Regions should always have 0 Padding 
            orig_region->padding = 0u;
            orig_region->next = curr->next; // Newly Freed Region consumes curr
        } else {
            // Can't Merge
            orig_region->next = curr;
        }
    } else {
        orig_region->next = nullptr;
    }
    // Try Merging with Previous Region
    if (prev != nullptr) {
        uintptr_t prev_addr = reinterpret_cast<uintptr_t>(prev);
        uintptr_t prev_end = prev_addr + prev->size + sizeof(Region);
        if (orig_region_addr == prev_end) {
            // Merge
            prev->size += sizeof(Region) + orig_region->size; // Padding not included since Free Regions should always have 0 Padding 
            prev->padding = 0u;
            prev->next = orig_region->next; // prev consumes Newly Freed Region
        } else {
            // Can't Merge
            prev->next = orig_region;
        }
    } else {
        head = orig_region;
    }
}
