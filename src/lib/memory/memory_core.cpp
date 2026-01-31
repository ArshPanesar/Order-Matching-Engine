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
    available_bytes(0u),
    used_bytes(0u) {
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

    available_bytes = head->size;
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
        uintptr_t aligned_addr = ComputeAlignedAddress(avail_addr, alignment);
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
            // This enables Free(ptr) to easily access the Header for any valid ptr 
            uintptr_t moved_header_addr = aligned_addr - sizeof(Region);
            Region* requested = reinterpret_cast<Region*>(moved_header_addr);
            requested->size = bytes;
            requested->padding = padding; // Behind this Header
            requested->next = nullptr; // Region is no longer free
            
            available_bytes -= required_size;
            used_bytes += required_size;

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
    
    // Ensure Address Fits within the Heap Allocated Block
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    assert(addr >= (uintptr_t)base_ptr);
    assert(addr <= (uintptr_t)base_ptr + total_size);
    
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

    available_bytes += orig_region->size;
    used_bytes -= orig_region->size;

    if (head == nullptr) {
        // No Other Free Blocks
        head = orig_region;
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
    }
    // Try Merging with Previous Region
    if (prev != nullptr && CanCoalesce(prev, orig_region)) {
        prev->size += sizeof(Region) + orig_region->size; // Padding not included since Free Regions should always have 0 Padding
        prev->padding = 0u;
        prev->next = orig_region->next; 
    }
}

const size_t MemoryAllocator::GetUsedBytes() const {
    return used_bytes;
}

const size_t MemoryAllocator::GetFreeBytes() const {
    return available_bytes;
}
