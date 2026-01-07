#pragma once
#include <map>
#include <queue>

#include "order_core.h"

using FIFOContainer = std::deque<Order>;
using PriceLevelContainer = std::map<OrderPrice, FIFOContainer>;

class OrderBook {
private:
    PriceLevelContainer bids_table;
    PriceLevelContainer asks_table;

public:
    OrderBook() = default;
    ~OrderBook() = default;

    void AddOrder(const Order& new_order, const eOrderSide side);
    void RemoveOrder(const Order& old_order, const eOrderSide side);

    const Order* GetBestBid() const;
    const Order* GetBestAsk() const;
};
