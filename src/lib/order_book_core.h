#pragma once
#include "memory/memory_core.h"
#include "order_core.h"

// Order Information to be stored in FIFO order
struct OrderNode {
    OrderID id{};
    OrderPrice price{};
    OrderQuantity current_quantity{};
    eOrderSide side{};
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
    
    OrderNode* Find(const OrderID& order_id) const;

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

// Fixed-Size Pool Allocator for OrderNodes
class OrderNodePool {
private:
    struct Block {
        OrderNode order_node{};
        Block* next = nullptr;
    };

    MemoryAllocator& allocator;

    Block* block_pool;
    size_t capacity;

    Block* head;


public:
    OrderNodePool(MemoryAllocator& mem_allocator, const size_t& _capacity = (1 << 12));
    ~OrderNodePool();

    // Get an OrderNode from this Pool
    OrderNode* Acquire();
    // Release OrderNode back to this Pool
    void Release(OrderNode* order_node);
};


// Bitset for finding Best Bid/Ask
class PriceLevelBitset {
private: 
    using Bit64 = uint64_t;

    // 64 Bits per Word
    Bit64* words;
    size_t num_words;
    size_t num_price_levels;
    

    MemoryAllocator& allocator;
public:
    PriceLevelBitset(MemoryAllocator& mem_allocator, const size_t& _num_price_levels);
    ~PriceLevelBitset();

    // Set a Price Level to be Active
    void ActivatePriceLevel(size_t price_level_index);
    // Remove a Price Level
    void DeactivatePriceLevel(size_t price_level_index);

    // Get Best Bid (Returns SIZE_MAX if not found)
    size_t GetBestBidPriceLevel() const;
    // Get Best Ask (Returns SIZE_MAX if not found)
    size_t GetBestAskPriceLevel() const;
};
