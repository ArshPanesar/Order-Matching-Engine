#include "order_book.h"

Order *OrderBook::AccessBestBid() {
    // Best Bid will be at the Highest Price Level
    if (!bids_table.empty()) {
        // Last Key of Table and First Order in FIFO Queue (Time Priority) is the Best Bid
        auto& fifo_queue = bids_table.rbegin()->second;
        return &fifo_queue.front();
    }

    return nullptr;
}

Order *OrderBook::AccessBestAsk() {
    // Best Ask will be at the Lowest Price Level
    if (!asks_table.empty()) {
        // First Key of Table and First Order in FIFO Queue (Time Priority) is the Best Ask
        auto& fifo_queue = asks_table.begin()->second;
        return &fifo_queue.front();
    }

    return nullptr;
}


void OrderBook::AddOrder(const Order &new_order, const eOrderSide side)
{

    // Determine Side Table
    auto& price_levels_table = (side == eOrderSide::BID) ? bids_table : asks_table;

    // Add new order to back of queue
    // This also creates a new Price Level if it didn't exist before
    price_levels_table[new_order.price].push_back(new_order);
}

void OrderBook::RemoveOrder(const Order &old_order, const eOrderSide side) {
    
    // Determine Side Table
    auto& price_levels_table = (side == eOrderSide::BID) ? bids_table : asks_table;

    // Early out if Price Level doesn't exist
    auto price_level_itr = price_levels_table.find(old_order.price);
    if (price_level_itr == price_levels_table.end()) 
        return;

    // Remove by ID from FIFO Queue
    auto& fifo_queue = price_level_itr->second;
    
    // Iterate over Queue and Remove Order if it Exists
    for (auto itr = fifo_queue.begin(); itr != fifo_queue.end(); ++itr) {
        if (itr->id == old_order.id) {
            fifo_queue.erase(itr);
            break;
        }
    }

    // Remove Price Level if FIFO Queue is Empty!
    if (fifo_queue.empty())
        price_levels_table.erase(old_order.price);
}

const Order* OrderBook::GetBestBid() const {
    // Best Bid will be at the Highest Price Level
    if (!bids_table.empty()) {
        // Last Key of Table and First Order in FIFO Queue (Time Priority) is the Best Bid
        auto& fifo_queue = bids_table.rbegin()->second;
        return &fifo_queue.front();
    }

    return nullptr;
}

const Order* OrderBook::GetBestAsk() const {
    // Best Ask will be at the Lowest Price Level
    if (!asks_table.empty()) {
        // First Key of Table and First Order in FIFO Queue (Time Priority) is the Best Ask
        auto& fifo_queue = asks_table.begin()->second;
        return &fifo_queue.front();
    }

    return nullptr;
}
