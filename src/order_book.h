#pragma once
#include <map>
#include <queue>

#include "matching_core.h"

using FIFOContainer = std::deque<Order>;
using PriceLevelContainer = std::map<OrderPrice, FIFOContainer>;

template<TradeEventSinkConcept>
class MatchingEngine;

class OrderBook {
private:
    // MatchingEngine has authorization to change OrderBooks!
    template<TradeEventSinkConcept>
    friend class MatchingEngine;

    PriceLevelContainer bids_table;
    PriceLevelContainer asks_table;


    Order* AccessBestBid();
    Order* AccessBestAsk();

public:
    OrderBook() = default;
    ~OrderBook() = default;

    void AddOrder(const Order& new_order, const eOrderSide side);
    void RemoveOrder(const Order& old_order, const eOrderSide side);

    const Order* GetBestBid() const;
    const Order* GetBestAsk() const;
};
