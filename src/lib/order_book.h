#pragma once
#include <map>
#include <queue>

#include "matching_core.h"

// Declaring MatchingEngine before OrderBook is defined
template<OrderEventSinkConcept, TradeEventSinkConcept>
class MatchingEngine;


using FIFOContainer = std::deque<Order>;
using PriceLevelContainer = std::map<OrderPrice, FIFOContainer>;


class OrderBook {
private:
    // MatchingEngine has authorization to change OrderBooks!
    template<OrderEventSinkConcept, TradeEventSinkConcept>
    friend class MatchingEngine;

    PriceLevelContainer bids_table;
    PriceLevelContainer asks_table;


    Order* AccessBestBid();
    Order* AccessBestAsk();

public:
    OrderBook() = default;
    ~OrderBook() = default;

    void AddOrder(const Order& new_order, const eOrderSide side);
    void RemoveOrder(const OrderID& old_order_id, const eOrderSide side);

    const Order* GetBestBid() const;
    const Order* GetBestAsk() const;

    const Order* GetOrderByID(const OrderID& order_id) const;
};
