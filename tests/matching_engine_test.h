#pragma once
#include <vector>

#include "matching_engine.h"

class TestTradeEventSink {
public:
    // Simple List
    std::vector<TradeEvent> trade_event_list;

public:
    TestTradeEventSink() = default;
    ~TestTradeEventSink() = default;

    // Conform to Concept
    void Accept(const TradeEvent& trade_event) {
        trade_event_list.push_back(trade_event);
    }

    void Clear() {
        trade_event_list.clear();
    }
};



// Helper Functions
inline OrderEvent MakeOrderEvent(Order id, eOrderEventType event_type, eOrderSide side, eOrderType type) {
    return OrderEvent{id, event_type, side, type};
}

