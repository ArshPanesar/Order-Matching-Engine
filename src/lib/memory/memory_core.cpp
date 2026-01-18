#include <cassert>
#include <cstring>
#include <cstddef>
#include <stdlib.h>

#include "memory_core.h"

MemoryResource::MemoryResource(size_t num_bytes) :
    base_ptr(nullptr),
    current_offset(0u),
    total_size(num_bytes) {
    
    // Ensure Allocation Size is a multiple of Alignment
    size_t padded_num_bytes = (num_bytes + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1);
    total_size = padded_num_bytes;

    base_ptr = aligned_alloc(CACHE_LINE_SIZE, total_size);
    assert(base_ptr != nullptr);
    
    memset(base_ptr, 0, total_size);
}

MemoryResource::~MemoryResource() {
    free(base_ptr);
}

void *MemoryResource::Allocate(size_t num_bytes, size_t alignment) {
    // Allow No Alignment
    if (alignment == 0)
        alignment = alignof(std::max_align_t);
    assert((alignment & (alignment - 1)) == 0); // Alignment must be a power of 2

    std::byte* base_address = static_cast<std::byte*>(base_ptr); // Pointer to Numeric
    uintptr_t address = reinterpret_cast<uintptr_t>(base_address + current_offset); // Reinterpret for Alignment Arithmetic
    uintptr_t aligned_address = (address + (alignment - 1)) & ~(alignment - 1);

    size_t padding = aligned_address - address; 
    if (padding > (total_size - current_offset) || num_bytes > (total_size - current_offset - padding))
        assert(false && "MemoryResource::Allocate() Failed: Requested Size too Large!");

    // Bump Up
    current_offset += num_bytes + padding;

    return reinterpret_cast<void*>(aligned_address);
}

void MemoryResource::Reset() {
    current_offset = 0u;
}

size_t MemoryResource::GetTotalAmt() const noexcept {
    return total_size;
}

size_t MemoryResource::GetAllocatedAmt() const noexcept {
    return current_offset;
}

size_t MemoryResource::GetFreeAmt() const noexcept {
    return (total_size - current_offset);
}