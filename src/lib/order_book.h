#pragma once

#include "order_book_core.h"
#include "matching_core.h"

// Declaring MatchingEngine before OrderBook is defined
template<OrderEventSinkConcept, TradeEventSinkConcept>
class MatchingEngine;

class OrderBook {
private:
    // MatchingEngine has authorization to change OrderBooks!
    template<OrderEventSinkConcept, TradeEventSinkConcept>
    friend class MatchingEngine;

    MemoryAllocator& allocator;

    OrderTable order_table;

    // Flat Arrays for Price Levels
    PriceLevel* bids_price_levels;
    PriceLevel* asks_price_levels;
    
    // Bitsets for Bids and Asks
    PriceLevelBitset bids_bitset;
    PriceLevelBitset asks_bitset;

    // Utility
    OrderNodePool order_node_pool;

    // Bounded Price Range
    OrderPrice min_price;
    OrderPrice max_price;
    size_t num_price_levels;

    size_t num_active_orders;
    size_t max_active_orders;
    
    OrderNode* AccessBestBid();
    OrderNode* AccessBestAsk();

public:
    OrderBook(MemoryAllocator& mem_allocator, OrderPrice _min_price, OrderPrice _max_price, size_t _max_active_orders = (1 << 12));
    ~OrderBook();

    void AddOrder(const Order& new_order, const eOrderSide side);
    void RemoveOrder(const OrderID& old_order_id);

    const OrderNode* GetBestBid() const;
    const OrderNode* GetBestAsk() const;

    const OrderNode* GetOrderByID(const OrderID& order_id) const;
};
