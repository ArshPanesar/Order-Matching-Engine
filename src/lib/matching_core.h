#pragma once
#include <concepts>

#include "order_core.h"

// Order Events for Matching Engine Input
enum class eOrderEventType {
    NEW,
    CANCEL,
    AMEND,
    STOP_EXECUTION // Used to signal completion/halting of incoming OrderEvents
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

enum class eTradeEventType {
    NEW,
    STOP_EXECUTION // Used to signal completion/halting of incoming TradeEvents
};

struct TradeEvent {
    TradeID trade_id{};
    TradeTimestamp timestamp{};

    OrderID bid_order_id{};
    OrderID ask_order_id{};

    OrderPrice executed_price{};
    OrderQuantity filled_quantity{};

    eTradeEventType type{};
};

//
// For Flexibility of Matching Engine I/O, a "Sink" concept will be used to allow various implementations of Input or Output Containers of Events
// Sinks are defined as Template Concepts below.
//

// Matching Engine will accept incoming OrderEvents through sinks derived from the concept below
template<typename SinkType>
concept OrderEventSinkConcept = requires(SinkType& sink) {
    // Must have "ExtractNext" method to push out incoming OrderEvents (in sequence of arrival)
    { sink.ExtractNext() } -> std::same_as<OrderEvent>;
};

// Matching Engine will send executed Trades through TradeEvents to sinks derived from the concept below
template<typename SinkType>
concept TradeEventSinkConcept = requires(SinkType& sink, const TradeEvent& trade_event) {
    // Must have "Accept" method to accept any outgoing TradeEvents
    { sink.Accept(trade_event) } -> std::same_as<void>;
};


