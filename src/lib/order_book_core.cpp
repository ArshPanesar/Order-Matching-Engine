#include "order_book_core.h"
#include <assert.h>
#include <utility>

OrderTable::OrderTable(MemoryAllocator& mem_allocator, size_t _capacity) : 
    table(nullptr),
    capacity(_capacity),
    size(0u),
    allocator(mem_allocator) {
    
    // Ensure Capacity is a Power of 2
    assert((capacity & (capacity - 1)) == 0);

    // Allocate Memory for the Table
    table = (Slot*)allocator.Allocate(capacity * sizeof(Slot), alignof(Slot));
    
    // Initialize Slots
    for (size_t i = 0; i < capacity; ++i) {
        table[i].id = 0;
        table[i].order_node = nullptr; // Invalid OrderNode denotes an Empty Cell
        table[i].distance = 0;
    }
}

OrderTable::~OrderTable() {
    // Free Table Memory
    allocator.Free((void*)table);

    table = nullptr;
    capacity = 0u;
    size = 0u;
}

void OrderTable::Insert(const OrderID& order_id, OrderNode* order_node) {
    size_t index = Hash(order_id) & (capacity - 1);
    
    // Find an Empty Slot
    Slot slot;
    slot.id = order_id;
    slot.order_node = order_node;
    slot.distance = 0u;
    
    size_t current_index = index;
    while (table[current_index].order_node != nullptr) {
        // Swap if Distance of Current Element is Larger
        if (slot.distance > table[current_index].distance) {
            std::swap(slot, table[current_index]);
        }

        ++slot.distance;
        assert(slot.distance < capacity);
        
        current_index = (current_index + 1) & (capacity - 1); // Wrap around
    }

    // Store Current Item in the Empty Slot
    table[current_index] = slot;
    
    ++size;
}

void OrderTable::Remove(const OrderID& order_id) {
    size_t index = Hash(order_id) & (capacity - 1);
    
    // Find the ID
    size_t current_index = index;
    size_t distance = 0u;
    while (table[current_index].order_node != nullptr) {
        if (distance > table[current_index].distance)
            return;
        
        if (table[current_index].id == order_id)
            break;

        ++distance;
        current_index = (current_index + 1) & (capacity - 1); // Wrap around
    }

    // Return if Empty
    if (table[current_index].order_node == nullptr)
        return;

    // Backwards Shift Deletion
    size_t prev_index = current_index;
    current_index = (current_index + 1) & (capacity - 1);
    while (table[current_index].order_node != nullptr && table[current_index].distance > 0) {
        table[prev_index] = table[current_index];
        table[prev_index].distance -= 1;
        
        prev_index = current_index;
        current_index = (current_index + 1) & (capacity - 1);
    }

    // Empty Final Slot
    table[prev_index].id = 0u;
    table[prev_index].order_node = nullptr;
    table[prev_index].distance = 0u;

    --size;
}

OrderNode* OrderTable::Find(const OrderID& order_id) {
    size_t index = Hash(order_id) & (capacity - 1);

    size_t current_index = index;
    size_t distance = 0u;
    while (table[current_index].order_node != nullptr) {
        // Early Out if Lookup Distance Exceeds Distance of Current Element
        if (distance > table[current_index].distance)
            return nullptr;
        
        if (table[current_index].id == order_id)
            return table[current_index].order_node;

        ++distance;
        current_index = (current_index + 1) & (capacity - 1); // Wrap around
    }

    return nullptr;
}

#ifdef LOB_DEBUG

double OrderTable::ComputeLoadFactor() const {
    return (double)size / (double)capacity;
}

size_t OrderTable::FindDistance(const OrderID &order_id) const
{
    size_t index = Hash(order_id) & (capacity - 1);

    size_t current_index = index;
    size_t distance = 0u;
    while (table[current_index].order_node != nullptr) {
        // Early Out if Lookup Distance Exceeds Distance of Current Element
        if (distance > table[current_index].distance)
            return 0;
        
        if (table[current_index].id == order_id)
            return distance;

        ++distance;
        current_index = (current_index + 1) & (capacity - 1); // Wrap around
    }

    return 0;
}

void OrderTable::ComputeAvgAndMaxDistances(float& avg_distance, size_t& max_distance) const {
    
    size_t largest_distance = 0u;
    size_t sum_distance = 0u;

    for (size_t i = 0; i < capacity; ++i) {
        if (table[i].order_node != nullptr) {
            sum_distance += table[i].distance;
            largest_distance = (largest_distance < table[i].distance) ? table[i].distance : largest_distance;
        }
    }

    avg_distance = (float)sum_distance / (float)size;
    max_distance = largest_distance;
}

#endif //LOB_DEBUG



// 
// OrderNodePool Impl
// 

OrderNodePool::OrderNodePool(MemoryAllocator& mem_allocator, const size_t& _capacity) : 
    allocator(mem_allocator),
    block_pool(nullptr),
    capacity(_capacity),
    head(nullptr) {

    // Allocate Pool
    block_pool = (Block*)allocator.Allocate(sizeof(Block) * capacity, alignof(Block));

    // Initialize Pool
    // Last Block of the Array
    block_pool[capacity - 1].next = nullptr;
    // Link Up the rest of the Blocks
    for (int64_t i = capacity - 2; i >= 0; --i) {
        block_pool[i].next = &block_pool[i + 1];
    }

    // Initialize Free List
    head = &block_pool[0];
}

OrderNodePool::~OrderNodePool() {
    // Free Pool and Stack
    allocator.Free(block_pool);
}

OrderNode* OrderNodePool::Acquire() {
    assert(head != nullptr && "OrderNodePool ran out of Memory!");
    
    // Release Head Block and Move Forward
    Block* block = head;
    head = head->next;

    return &(block->order_node);
}

void OrderNodePool::Release(OrderNode* order_node) {
    // Get Pointer to this OrderNode's Block
    uintptr_t node_addr = reinterpret_cast<uintptr_t>(order_node);
    uintptr_t base_addr = reinterpret_cast<uintptr_t>(block_pool);
    assert(node_addr >= base_addr && node_addr <= (base_addr + sizeof(Block) * capacity) && "OrderNodePool: Tried to release an invalid OrderNode");

    // Prepend Free Block
    Block* released_block = reinterpret_cast<Block*>(reinterpret_cast<char*>(node_addr) - offsetof(Block, order_node));
    
    Block* next = head;
    head = released_block;
    head->next = next;
}
