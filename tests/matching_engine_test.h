#pragma once
#include <vector>
#include <queue>

#include "matching_engine.h"

class TestOrderEventSink {
public:
    // Simple Queue
    std::queue<OrderEvent> order_event_queue;

public:
    TestOrderEventSink() = default;
    ~TestOrderEventSink() = default;

    void Add(const OrderEvent& order_event) {
        order_event_queue.push(order_event);
    }

    // Conform to Concept
    OrderEvent ExtractNext() {
        OrderEvent event = order_event_queue.front();
        order_event_queue.pop();
        return event;
    }
};

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

inline void ProcessSingleEvent(const OrderEvent& event, TestOrderEventSink& order_sink, MatchingEngine<TestOrderEventSink, TestTradeEventSink>& engine) {
    order_sink.Add(event);
    engine.Run();
}