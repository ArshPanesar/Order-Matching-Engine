#include "generator.h"
#include <iostream>

OrderEventGenerator::OrderEventGenerator(uint64_t seed) : 
    id_gen(0u),
    old_id_upper_limit(16),
    timestamp_ns(0.0f),
    timestamp_lambda(2.0f),
    event_type_weights(),
    initial_new_events_count(10),
    bid_side_chance(0.5f),
    best_price(100u),
    geo_dist_prob(0.5f),
    qty_lognorm_scale(100),
    lot_size(10u),
    limit_order_chance(0.9f),
    rng(seed),
    uniform_dist_01(0.0f, 1.0f),
    exp_dist(timestamp_lambda),
    lognorm_dist(0.0f, 1.0f),
    event_type_dist(),
    geo_dist(geo_dist_prob),
    active_orders_set(),
    trade_sink(),
    matching_engine(trade_sink) {

    // Setup Event Type Weights
    event_type_weights.push_back(0.7f); // NEW
    event_type_weights.push_back(0.2f); // CANCEL
    event_type_weights.push_back(0.1f); // AMEND
    
    event_type_dist = std::discrete_distribution<>(event_type_weights.begin(), event_type_weights.end());
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
    if (new_event.event_type == eOrderEventType::NEW || active_orders_set.size() == 0) {
        new_event.order.id = id_gen++;
        active_orders_set.insert(new_event.order.id);
    } else {
        // Cancel/Amend an Older Order
        //
        // Pick from Oldest Few Active Orders
        int num_active_orders = active_orders_set.size();
        int random_old_ind = old_id_upper_limit * uniform_dist_01(rng);
        
        // Count upto Index and get the UID
        int count = 0;
        OrderID old_id = 0u;
        for (OrderID uid : active_orders_set) {
            old_id = uid;

            ++count;
            if (count >= random_old_ind)
                break;
        }
        new_event.order.id = old_id;
        active_orders_set.erase(old_id);
    }

    // Generate Timestamp
    float dt = exp_dist(rng);
    timestamp_ns += dt;
    new_event.order.timestamp = static_cast<OrderTimestamp>(timestamp_ns);

    // Generate Side
    new_event.side = (uniform_dist_01(rng) < bid_side_chance) ? eOrderSide::BID : eOrderSide::ASK;

    // Generate Price
    // 
    // Cluster near Best Prices
    int price_offset = geo_dist(rng);
    price_offset = (new_event.side == eOrderSide::BID) ? -price_offset : price_offset;
    const Order* best_price_order = (new_event.side == eOrderSide::BID) ? matching_engine.GetOrderBook()->GetBestBid() : matching_engine.GetOrderBook()->GetBestAsk();
    if (best_price_order != nullptr)
        best_price = best_price_order->price;
    new_event.order.price = static_cast<OrderPrice>(best_price + price_offset);
    // Don't Let Limit Orders cross the book
    if (new_event.type == eOrderType::LIMIT) {
        if (new_event.side == eOrderSide::ASK)
            new_event.order.price = std::max(new_event.order.price, best_price);
        else
            new_event.order.price = std::min(new_event.order.price, best_price);
    }


    // Generate Quantity
    float qty = lognorm_dist(rng) * qty_lognorm_scale;
    OrderQuantity qty_format = static_cast<OrderQuantity>(std::max(1.0f, qty));
    // Round to Lot Size
    qty_format = std::max(lot_size, (qty_format / lot_size) * lot_size);
    new_event.order.initial_quantity = qty_format;
    new_event.order.remaining_quantity = qty_format;
    
    // Generate Order Type
    new_event.type = (uniform_dist_01(rng) < limit_order_chance) ? eOrderType::LIMIT : eOrderType::MARKET;

    // Run Internal Engine
    matching_engine.ProcessEvent(new_event);
    // Remove Executed Orders (Also removes partially filled orders, this is intended)
    // std::cout << trade_sink.trade_event_list.size() << "\n";
    for (TradeEvent& trade_event : trade_sink.trade_event_list) {
        active_orders_set.erase(trade_event.ask_order_id);
        active_orders_set.erase(trade_event.bid_order_id);
    }
    trade_sink.trade_event_list.clear();


    return new_event;
}
