#include "generator.h"
#include <iostream>


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

int main()
{
    OrderEventGenerator generator{};

    for (int i = 0; i < 100; ++i) {
        OrderEvent event = generator.Step();
        std::string output = FormatOrderEvent(event);

        std::cout << output << "\n";
    }

    return 0;
}
