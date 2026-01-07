#pragma once
#include <concepts>

#include "order_core.h"

// Order Events for Matching Engine Input
enum class eOrderEventType {
    NEW,
    CANCEL,
    AMEND
};

struct OrderEvent {
    Order order{};
    
    eOrderEventType event_type{};
    eOrderSide side{};
    eOrderType type{};
};

// Trade Events for Matching Engine Output
// Defining Types for Trade Events
// Same as Order Field Types, but Defining for Clarity!
using TradeID = uint64_t;
using TradeTimestamp = uint64_t;

struct TradeEvent {
    TradeID trade_id{};
    TradeTimestamp timestamp{};

    OrderID bid_order_id{};
    OrderID ask_order_id{};

    OrderPrice executed_price{};
    OrderQuantity filled_quantity{};
};


// For Flexibility of Matching Engine Output, a "Sink" concept will be used to allow various
// implementations of containers/processors of Trade Events. The Matching Engine will send executed Trades through TradeEvents
// to these sinks
template<typename SinkType>
concept TradeEventSinkConcept = requires(SinkType sink, const TradeEvent& trade_event) {
    // Must have "Accept" method to accept any incoming TradeEvents
    { sink.Accept(trade_event) } -> std::same_as<void>;
};


