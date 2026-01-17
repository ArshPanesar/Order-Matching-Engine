#include <iostream>
#include "generator/order_event_generator.h"

std::string FormatOrderEvent(const OrderEvent& event) {
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

std::string FormatTradeEvent(const TradeEvent& event) {
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

// Running Engine on Synthetic Order Data
//
// Input Event Sink
class SyntheticOrderEventSink {
private:
    OrderEventGenerator generator;

public:
    SyntheticOrderEventSink() = default;
    ~SyntheticOrderEventSink() = default;
    
    // Conform to Concept
    OrderEvent ExtractNext() {
        return generator.Step();
    }
};
//
// Output Event Sink
class SyntheticTradeEventSink {
private:
    std::vector<TradeEvent> trade_event_list;

public:
    SyntheticTradeEventSink() = default;
    ~SyntheticTradeEventSink() = default;
    
    // Conform to Concept
    void Accept(const TradeEvent& trade_event) {
        trade_event_list.push_back(trade_event);
    }

    void Print() {
        for (size_t i = 0; i < trade_event_list.size(); ++i) {
            std::cout << FormatTradeEvent(trade_event_list[i]) << "\n";
        }
    }

    void Clear() {
        trade_event_list.clear();
    }
};

int main()
{
    // Synthetic Data Test
    SyntheticOrderEventSink synthetic_order_sink;
    SyntheticTradeEventSink synthetic_trade_sink;
    
    MatchingEngine<SyntheticOrderEventSink, SyntheticTradeEventSink> synthetic_data_engine(synthetic_order_sink, synthetic_trade_sink);

    // Avoiding Infinite Loop
    std::cout << "Starting Order Stream...\n";

    uint64_t max_orders = 200000u;
    while (max_orders > 0) {
        
        synthetic_data_engine.Run();
        synthetic_trade_sink.Clear();
        --max_orders;
    }

    std::cout << "Orders Exhausted\n";

    return 0;
}
