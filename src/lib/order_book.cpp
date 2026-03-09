#include "order_book.h"
#include <cassert>
#include <iostream>

OrderNode* OrderBook::AccessBestBid() {
    return const_cast<OrderNode*>(GetBestBid());
}

OrderNode* OrderBook::AccessBestAsk() {
    return const_cast<OrderNode*>(GetBestAsk());
}

OrderBook::OrderBook(MemoryAllocator& mem_allocator, OrderPrice _min_price, OrderPrice _max_price, size_t _max_active_orders, size_t table_size) :
    allocator(mem_allocator),
    order_table(mem_allocator, table_size),
    bids_price_levels(nullptr),
    asks_price_levels(nullptr),
    bids_bitset(mem_allocator, _max_price - _min_price + 1),
    asks_bitset(mem_allocator, _max_price - _min_price + 1),
    order_node_pool(mem_allocator, _max_active_orders),
    min_price(_min_price),
    max_price(_max_price),
    num_price_levels(0u),
    num_active_orders(0u),
    max_active_orders(_max_active_orders) {
    
    // Allocate Arrays for Bids and Asks and Initialize them
    num_price_levels = max_price - min_price + 1;
    
    bids_price_levels = (PriceLevel*)allocator.Allocate(sizeof(PriceLevel) *  num_price_levels, alignof(PriceLevel));
    for (size_t i = 0; i < num_price_levels; ++i) {
        bids_price_levels[i].price = min_price + i;
        bids_price_levels[i].head = nullptr;
        bids_price_levels[i].tail = nullptr;
    }

    asks_price_levels = (PriceLevel*)allocator.Allocate(sizeof(PriceLevel) *  num_price_levels, alignof(PriceLevel));
    for (size_t i = 0; i < num_price_levels; ++i) {
        asks_price_levels[i].price = min_price + i;
        asks_price_levels[i].head = nullptr;
        asks_price_levels[i].tail = nullptr;
    }
}

OrderBook::~OrderBook() {
    // Free Price Level Arrays
    allocator.Free(asks_price_levels);
    allocator.Free(bids_price_levels);
}

void OrderBook::AddOrder(const Order &new_order, const eOrderSide side) {
    if (UNLIKELY_BRANCH(new_order.price < min_price || new_order.price > max_price))
        TERMINATE_ON_ERROR("OrderBook received an Order with Price = ", new_order.price, "; Which is Out of Min/Max Price Range of [", min_price, ", ", max_price, "].");

    // Determine Side
    auto* side_price_levels = (side == eOrderSide::BID) ? bids_price_levels : asks_price_levels;
    auto& side_bitset = (side == eOrderSide::BID) ? bids_bitset : asks_bitset;

    // Create an Order Node 
    OrderNode* new_order_node = order_node_pool.Acquire();
    new_order_node->id = new_order.id;
    new_order_node->price = new_order.price;
    new_order_node->current_quantity = new_order.remaining_quantity;
    new_order_node->side = side;
    new_order_node->next = nullptr;
    new_order_node->prev = nullptr;
    
    // Add Order Node to Table
    order_table.Insert(new_order.id, new_order_node);

    // Find Price Level Index for this Order
    size_t price_level_index = new_order.price - min_price;

    // Place Order Node in Linked List
    auto& price_level = side_price_levels[price_level_index];
    
    if (price_level.head == nullptr) {
        // Construct the Price Level
        price_level.head = new_order_node;
        price_level.tail = new_order_node;

        side_bitset.ActivatePriceLevel(price_level_index);
    } else {
        // Place Order Node in already active Price Level
        // Append as Tail
        OrderNode* prev = price_level.tail;
        price_level.tail = new_order_node;

        price_level.tail->prev = prev;
        prev->next = new_order_node;
    }

    ++num_active_orders;
}

void OrderBook::RemoveOrder(const OrderID& old_order_id) {
    
    // Access the Order Node
    OrderNode* old_order_node = order_table.Find(old_order_id);
    if (!old_order_node)
        return;

    // Determine Side
    auto side = old_order_node->side;
    auto* side_price_levels = (side == eOrderSide::BID) ? bids_price_levels : asks_price_levels;
    auto& side_bitset = (side == eOrderSide::BID) ? bids_bitset : asks_bitset;

    // Remove Order Node from Table
    order_table.Remove(old_order_id);

    // Find Price Level Index for this Order
    size_t price_level_index = old_order_node->price - min_price;
    auto& price_level = side_price_levels[price_level_index];
    
    // Remove Order Node from Price Level
    //
    // Check if Single Order in this Price Level
    if (price_level.head == old_order_node && price_level.tail == old_order_node) {
        price_level.head = nullptr;
        price_level.tail = nullptr;
      
        side_bitset.DeactivatePriceLevel(price_level_index);
    } else if (price_level.head == old_order_node) {
        price_level.head = old_order_node->next;
        if (price_level.head)
            price_level.head->prev = nullptr; // New Head
    } else if (price_level.tail == old_order_node) {
        price_level.tail = old_order_node->prev;
        if (price_level.tail)
            price_level.tail->next = nullptr; // New Tail
    } else {
        // Node is somewhere in the middle of the list
        OrderNode* prev_node = old_order_node->prev;
        OrderNode* next_node = old_order_node->next;

        prev_node->next = next_node;
        next_node->prev = prev_node;
    }

    // Reset Pointers for Safety
    old_order_node->next = nullptr;
    old_order_node->prev = nullptr;

    // Release Order Node back to the Pool
    order_node_pool.Release(old_order_node);

    --num_active_orders;
}

const OrderNode* OrderBook::GetBestBid() const {
    // Best Bid will be at the Highest Price Level
    size_t price_level_index = bids_bitset.GetBestBidPriceLevel();
    if (price_level_index != SIZE_MAX)
        return bids_price_levels[price_level_index].head;
    
    return nullptr;
}

const OrderNode* OrderBook::GetBestAsk() const {
    // Best Ask will be at the Lowest Price Level
    size_t price_level_index = asks_bitset.GetBestAskPriceLevel();
    if (price_level_index != SIZE_MAX)
        return asks_price_levels[price_level_index].head;
    
    return nullptr;
}

const OrderNode* OrderBook::GetOrderByID(const OrderID& order_id) const {
    return order_table.Find(order_id);
}
