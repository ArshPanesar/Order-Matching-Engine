#pragma once
#include <cstdint>
#include <cstddef>

// Core Defs

#define CACHE_LINE_SIZE 64u

// A Source of Memory (Intended to contain Large Heap Allocations that persist throughout the execution of the program)
class MemoryResource {
private:
    void* base_ptr;
    size_t current_offset;
    size_t total_size;

public:
    // Allocate on Construct
    explicit MemoryResource(size_t num_bytes);
    // Free on Destruct
    ~MemoryResource();

    // Non-Copyable
    MemoryResource(const MemoryResource&) = delete;
    MemoryResource& operator=(const MemoryResource&) = delete;

    // Allocate within Resource
    // NOTE: aligment == 0 will Align the requested Buffer by std::max_align_t (Alignment that works for all fundamental types)
    void* Allocate(size_t num_bytes, size_t alignment);

    // Clear Memory Usage (Only moves offset pointer backwards, does not free memory)
    // WARN: Any Addresses allocated within this resource will be invalidated!
    void Reset();

    // Stats
    size_t GetTotalAmt() const noexcept;
    size_t GetAllocatedAmt() const noexcept;
    size_t GetFreeAmt() const noexcept;
};
