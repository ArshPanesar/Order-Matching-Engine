#pragma once
#include "memory/memory_core.h"
#include "order_core.h"

// Order Information to be stored in FIFO order
struct OrderNode {
    OrderPrice price{};
    OrderQuantity current_quantity{};
    OrderNode* next = nullptr;
    OrderNode* prev = nullptr;
};

// Price Level Information
struct PriceLevel {
    OrderPrice price{};
    OrderNode* head = nullptr;
    OrderNode* tail = nullptr;
};

// Order Table to store ID to OrderNode mappings
// Implemented as a Fixed-Size Hash Table (Uses Robin Hood Hashing)
class OrderTable {
private:
    struct Slot {
        OrderID id{};
        OrderNode* order_node = nullptr;
        
        size_t distance{}; // Distance of this Slot (if not empty) from its Hashed index to its Current index
    };

    Slot* table;

    size_t capacity;
    size_t size;

    MemoryAllocator& allocator;
private:
    inline uint64_t Hash(const OrderID& order_id) const {
        // MurmurHash3 Finalizer (64-bit)
        uint64_t h = order_id;
        h ^= h >> 33;
        h *= 0xff51afd7ed558ccdULL;
        h ^= h >> 33;
        h *= 0xc4ceb9fe1a85ec53ULL;
        h ^= h >> 33;
        return h;
    }

public: 
    OrderTable(MemoryAllocator& mem_allocator, size_t _capacity = (1 << 12));
    ~OrderTable();

    // No Copies allowed
    OrderTable(const OrderTable&) = delete;
    OrderTable& operator=(const OrderTable&) = delete; 
    
    void Insert(const OrderID& order_id, OrderNode* order_node);
    void Remove(const OrderID& order_id);
    
    OrderNode* Find(const OrderID& order_id);

    const size_t GetSize() const { return size; };
    const size_t GetCapacity() const { return capacity; };
    

#ifdef LOB_DEBUG
    double ComputeLoadFactor() const;
    
    // Find the Distance of the Slot storing id from its home index
    size_t FindDistance(const OrderID& order_id) const;
    // Find the Maximum and Average Distances within Clusters
    void ComputeAvgAndMaxDistances(float& avg_distance, size_t& max_distance) const;
    
#endif //LOB_DEBUG
};
