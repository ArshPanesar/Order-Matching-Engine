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


void OrderBook::AddOrder(const Order &new_order, const eOrderSide side) {
    // Determine Side Table
    auto& price_levels_table = (side == eOrderSide::BID) ? bids_table : asks_table;

    // Add new order to back of queue
    // This also creates a new Price Level if it didn't exist before
    price_levels_table[new_order.price].push_back(new_order);
}

void OrderBook::RemoveOrder(const OrderID& old_order_id, const eOrderSide side) {
    // Determine Side Table
    auto& price_levels_table = (side == eOrderSide::BID) ? bids_table : asks_table;

    // Search through the Entire Table (TODO: Replace this by a Faster Lookup)
    for (auto& itr : price_levels_table) {
        // Search through FIFO Queue
        auto& fifo_queue = itr.second;
        for (auto q_itr = fifo_queue.begin(); q_itr != fifo_queue.end(); ++q_itr) {
            if (q_itr->id == old_order_id) {
                fifo_queue.erase(q_itr);
                break;
            }
        }

        // Remove Price Level if Queue is Empty
        if (fifo_queue.empty()) {
            price_levels_table.erase(itr.first); // Order Price
            break;
        }
    }
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

const Order* OrderBook::GetOrderByID(const OrderID& order_id) const {
    // Search on Both Sides
    eOrderSide side = eOrderSide::BID;
    for (int s = 0; s < 2; ++s) {

        auto& price_levels_table = (side == eOrderSide::BID) ? bids_table : asks_table;

        // Search through the Entire Table (TODO: Replace this by a Faster Lookup)
        for (auto& itr : price_levels_table) {
            // Search through FIFO Queue
            auto& fifo_queue = itr.second;
            for (size_t i = 0; i < fifo_queue.size(); ++i) {
                if (fifo_queue[i].id == order_id)
                    return &fifo_queue[i];
            }
        }

        side = eOrderSide::ASK;
    }

    return nullptr;
}
