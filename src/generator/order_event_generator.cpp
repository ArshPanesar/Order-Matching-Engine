#include "order_event_generator.h"
#include <iostream>

inline void OrderEventGenerator::AddActiveOrder(OrderID id) {
    active_orders_list.push_back(id);
    active_orders_id_table[id] = active_orders_list.size() - 1;
}

inline void OrderEventGenerator::RemoveActiveOrder(OrderID id) {
    auto itr = active_orders_id_table.find(id);
    if (itr == active_orders_id_table.end())
        return;

    size_t old_index = itr->second;
    OrderID swapped_id = active_orders_list.back();

    std::swap(active_orders_list[old_index], active_orders_list.back());
    active_orders_list.pop_back();

    if (old_index < active_orders_list.size())
        active_orders_id_table[swapped_id] = old_index;

    active_orders_id_table.erase(itr);
}

OrderEventGenerator::OrderEventGenerator(MemoryAllocator& mem_allocator, const GeneratorParams& _params, size_t max_live_orders, size_t order_table_size) : 
    params(_params),
    id_gen(0u),
    timestamp_ns(0.0f),
    timestamp_lambda(2.0f),
    initial_new_events_count(10),
    best_price(100u),
    geo_dist_prob(0.5f),
    qty_lognorm_scale(100),
    lot_size(10u),
    rng(_params.seed),
    uniform_dist_01(0.0f, 1.0f),
    exp_dist(timestamp_lambda),
    lognorm_dist(0.0f, 1.0f),
    event_type_dist(),
    geo_dist(geo_dist_prob),
    active_orders_list(),
    active_orders_id_table(),
    order_sink(),
    trade_sink(),
    matching_engine(order_sink, trade_sink, mem_allocator, _params.min_price, _params.max_price, max_live_orders, order_table_size) {

    // Setup Event Type Weights
    event_type_dist = std::discrete_distribution<>(params.event_type_class_prob, params.event_type_class_prob + 3);

    // Compute Best Price as Average of Min/Max
    best_price = (OrderPrice)( ((float)params.min_price + (float)params.max_price) / 2.0f );

    // Reserve Enough Memory
    active_orders_list.reserve(max_live_orders);
    active_orders_id_table.reserve(max_live_orders);
}

OrderEvent OrderEventGenerator::Step() { 
    OrderEvent new_event{};

    // Generate Event Type
    if (initial_new_events_count > 0) {
        // First Few Events should be New Orders
        --initial_new_events_count;
        new_event.event_type = eOrderEventType::NEW;
    } else {
        // Weighted Choice
        int choice = event_type_dist(rng);
        switch (choice) {
            case 1: 
                new_event.event_type = eOrderEventType::CANCEL;
                break;
            case 2: 
                new_event.event_type = eOrderEventType::AMEND;
                break;
            default:
                new_event.event_type = eOrderEventType::NEW;
        }
    }

    // Generate Order ID
    if (new_event.event_type == eOrderEventType::NEW || active_orders_list.size() == 0) {
        new_event.order.id = id_gen++;
        AddActiveOrder(new_event.order.id);
    } else {
        // Cancel/Amend an Older Order
        //
        // Pick a Random Index from currently live Orders
        size_t random_old_ind = (size_t)((float)active_orders_list.size() * uniform_dist_01(rng));
        random_old_ind = std::min(random_old_ind, active_orders_list.size() - 1);

        new_event.order.id = active_orders_list[random_old_ind];

        // Remove from Internal Set if Order is to be Cancelled
        if (new_event.event_type == eOrderEventType::CANCEL)
            RemoveActiveOrder(new_event.order.id);
    }

    // Generate Timestamp
    float dt = exp_dist(rng);
    timestamp_ns += dt;
    new_event.order.timestamp = static_cast<OrderTimestamp>(timestamp_ns);

    // Generate Side
    new_event.side = (uniform_dist_01(rng) < params.bid_side_prob) ? eOrderSide::BID : eOrderSide::ASK;

    // Generate Order Type
    new_event.type = (uniform_dist_01(rng) < params.limit_order_prob) ? eOrderType::LIMIT : eOrderType::MARKET;

    // Generate Price
    // 
    // Cluster near Best Prices
    int price_offset = geo_dist(rng);
    price_offset = (new_event.side == eOrderSide::BID) ? -price_offset : price_offset;
    const OrderNode* best_price_order = (new_event.side == eOrderSide::BID) ? matching_engine.GetOrderBook()->GetBestBid() : matching_engine.GetOrderBook()->GetBestAsk();
    if (best_price_order != nullptr)
        best_price = best_price_order->price;
    new_event.order.price = static_cast<OrderPrice>((int64_t)best_price + price_offset);
    // Don't Let Limit Orders cross the book
    if (new_event.type == eOrderType::LIMIT) {
        if (new_event.side == eOrderSide::ASK)
            new_event.order.price = std::max(new_event.order.price, best_price);
        else
            new_event.order.price = std::min(new_event.order.price, best_price);
    }
    // Clamp Prices
    new_event.order.price = std::clamp(new_event.order.price, params.min_price, params.max_price);

    // Generate Quantity
    float qty = lognorm_dist(rng) * (float)qty_lognorm_scale;
    OrderQuantity qty_format = static_cast<OrderQuantity>(std::max(1.0f, qty));
    // Round to Lot Size
    qty_format = std::max(lot_size, (qty_format / lot_size) * lot_size);
    new_event.order.initial_quantity = qty_format;
    new_event.order.remaining_quantity = qty_format;
    

    // Run Internal Engine
    order_sink.Add(new_event);
    matching_engine.Run();

    // Remove Executed Orders
    for (TradeEvent& trade_event : trade_sink.trade_event_list) {
        RemoveActiveOrder(trade_event.ask_order_id);
        RemoveActiveOrder(trade_event.bid_order_id);
    }
    trade_sink.trade_event_list.clear();

    return new_event;
}
