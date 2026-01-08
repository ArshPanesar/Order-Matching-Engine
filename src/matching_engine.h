#pragma once

#include "matching_core.h"
#include "order_book.h"

// Matching Engine maintains an Order Book, processes incoming Order Events and executes Trades per Instrument
// All executed Trades are output as TradeEvents into the injected TradeEventSink
template<TradeEventSinkConcept TradeEventSink>
class MatchingEngine {
private:
    OrderBook order_book;

    // Trade Event ID Generator
    TradeID avail_trade_id = 0;

    // Reference to Sink
    TradeEventSink& trade_event_sink;

private:
    TradeID GenerateTradeID() {
        return avail_trade_id++;
    }

    TradeEvent GenerateTradeEvent(const Order* aggressive_order, const Order* passive_order, const eOrderSide aggressive_side, const OrderPrice& executed_price, const OrderQuantity& trade_quantity) {
        TradeEvent new_trade_event;
        new_trade_event.trade_id = GenerateTradeID();

        new_trade_event.timestamp = aggressive_order->timestamp; // TODO: Should use actual time as timestamp
        
        if (aggressive_side == eOrderSide::BID) {
            new_trade_event.bid_order_id = aggressive_order->id;
            new_trade_event.ask_order_id = passive_order->id;
        } else {
            new_trade_event.bid_order_id = passive_order->id;
            new_trade_event.ask_order_id = aggressive_order->id;
        }

        new_trade_event.executed_price = executed_price;
        new_trade_event.filled_quantity = trade_quantity;

        return new_trade_event;
    }

    void OnNewOrderEvent(OrderEvent& order_event) {
        // Process a New Order
        Order& new_order = order_event.order;
        eOrderSide order_side = order_event.side;
        eOrderType order_type = order_event.type;

        // Lambdas as easy utility
        auto GetOppositeSideBestOrder = [&]() -> Order* {
            return (order_side == eOrderSide::BID) ? order_book.AccessBestAsk() : order_book.AccessBestBid();
        };
        auto GetOppositeSide = [&]() -> eOrderSide {
            return (order_side == eOrderSide::BID) ? eOrderSide::ASK : eOrderSide::BID;
        };
        auto IsBookCrossed = [&](const Order* best_opp_side_order) -> bool {
            // Order crosses the Book if:
            // 1. Its a Bid and its Price is Higher than Best Ask
            // OR
            // 2. Its an Ask and its Price is Lower than Best Bid
            return (order_side == eOrderSide::BID) ? (new_order.price >= best_opp_side_order->price) : (new_order.price <= best_opp_side_order->price);
        };

        if (order_type == eOrderType::LIMIT) {
            // New Limit Order
            //
            // Check if Order crosses the Book
            Order* current_best_opp = GetOppositeSideBestOrder();
            if (current_best_opp == nullptr) {
                // Book Empty on Opposite Side, New Order must rest!
                order_book.AddOrder(new_order, order_side);
                return;
            }

            if (IsBookCrossed(current_best_opp)) {
                // Crossed the Book
                while (current_best_opp != nullptr && IsBookCrossed(current_best_opp) && new_order.remaining_quantity > 0) {
                    // Execute Trade
                    OrderQuantity trade_quantity = std::min(new_order.remaining_quantity, current_best_opp->remaining_quantity);
                    new_order.remaining_quantity -= trade_quantity;
                    current_best_opp->remaining_quantity -= trade_quantity;
                    
                    // Generate Trade Event
                    TradeEvent new_trade_event = GenerateTradeEvent(&new_order, current_best_opp, order_side, current_best_opp->price, trade_quantity);
                    // Immediately Push to Sink
                    trade_event_sink.Accept(new_trade_event);

                    if (current_best_opp->remaining_quantity == 0) 
                        order_book.RemoveOrder(*current_best_opp, GetOppositeSide());  // Resting Order Fully Filled

                    // New Order fully filled
                    if (new_order.remaining_quantity == 0)
                        break;
                                        
                    // Move to next
                    current_best_opp = GetOppositeSideBestOrder();
                }
                
                // Add New Order to the Book if it still has Non-Zero Quantity
                if (new_order.remaining_quantity > 0) {
                    current_best_opp = GetOppositeSideBestOrder();
                    if (current_best_opp == nullptr || !IsBookCrossed(current_best_opp))
                        order_book.AddOrder(new_order, order_side);
                }

            } else {
                // Does not Cross the Book, Must Rest
                order_book.AddOrder(new_order, order_side);
            }
        } else if (order_type == eOrderType::MARKET) {
            // Check Opposite Side
            Order* current_best_opp = GetOppositeSideBestOrder();
            if (current_best_opp == nullptr) {
                // Market Order Rejected
                return;
            }

            // Duplicated Limit Order Matching Loop (slightly changed for Market Order) for Clarity
            while (current_best_opp != nullptr && new_order.remaining_quantity > 0) {
                // Execute Trade
                OrderQuantity trade_quantity = std::min(new_order.remaining_quantity, current_best_opp->remaining_quantity);
                new_order.remaining_quantity -= trade_quantity;
                current_best_opp->remaining_quantity -= trade_quantity;
                
                // Generate Trade Event
                TradeEvent new_trade_event = GenerateTradeEvent(&new_order, current_best_opp, order_side, current_best_opp->price, trade_quantity);
                // Immediately Push to Sink
                trade_event_sink.Accept(new_trade_event);

                if (current_best_opp->remaining_quantity == 0) 
                    order_book.RemoveOrder(*current_best_opp, GetOppositeSide());  // Resting Order Fully Filled

                // New Order fully filled
                if (new_order.remaining_quantity == 0)
                    break;
                                    
                // Move to next
                current_best_opp = GetOppositeSideBestOrder();
            }
        }
    }

public:
    explicit MatchingEngine(TradeEventSink& sink) : 
        trade_event_sink(sink) 
        {};
    
    ~MatchingEngine() = default;

    void ProcessEvent(OrderEvent& order_event) {
        switch(order_event.event_type) {
            case eOrderEventType::NEW:
                OnNewOrderEvent(order_event);
                break;
            
            case eOrderEventType::CANCEL:
                // TODO
                break;
            
            case eOrderEventType::AMEND:
                // TODO
                break;
        }
    }

    // Getters
    const OrderBook* GetOrderBook() const { return &order_book; }
};
