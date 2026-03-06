#pragma once
#include "spsc_queue.h"
#include "../lib/matching_core.h"

inline std::string FormatOrderEvent(const OrderEvent& event) {
    static std::string event_type_str_table[] = { "NEW", "CANCEL", "AMEND" };
    static std::string order_type_str_table[] = { "LIMIT", "MARKET" };
    static std::string order_side_str_table[] = { "BID", "ASK" };
    
    std::string msg = "";
    
    msg += event_type_str_table[static_cast<int>(event.event_type)] + "\t: [";
    msg += "uid=" + std::to_string(event.order.id) + ", ";
    msg += "ts=" + std::to_string(event.order.timestamp) + ", ";
    msg += "type=" + order_type_str_table[static_cast<int>(event.type)] + ", ";
    msg += "side=" + order_side_str_table[static_cast<int>(event.side)] + ", ";
    msg += "price=" + std::to_string(event.order.price) + ", ";
    msg += "qty=" + std::to_string(event.order.initial_quantity) + "]";

    return msg;
}

inline std::string FormatTradeEvent(const TradeEvent& event) {
    std::string msg = "";
    
    msg += "TRADE: [";
    msg += "uid=" + std::to_string(event.trade_id) + ", ";
    msg += "ts=" + std::to_string(event.timestamp) + ", ";
    msg += "bid_id=" + std::to_string(event.bid_order_id) + ", ";
    msg += "ask_id=" + std::to_string(event.ask_order_id) + ", ";
    msg += "exec_price=" + std::to_string(event.executed_price) + ", ";
    msg += "fill_qty=" + std::to_string(event.filled_quantity) + "]";

    return msg;
}