#pragma once
#include <vector>
#include <unordered_set>
#include <random>

#include "matching_engine.h"

class OrderEventGenerator {
private:
    // UID: Simple Increment
    OrderID id_gen;
    float old_id_upper_limit; // IDs are old if their indices in consecutive order are in range [0, old_id_upper_limit]

    // Timestamp: Exponential Differences b/w Timestamps
    float timestamp_ns; // Nanosecond Measure
    float timestamp_lambda;

    // EventType: Class RNG
    std::vector<float> event_type_weights; // Must Sum to 1.0
    OrderID initial_new_events_count; // First Few Events should be New Orders

    // Side RNG
    float bid_side_chance;

    // Price: Distribution around Mid Price
    OrderPrice best_price;
    float geo_dist_prob;
    
    // Quantity: Log-Normal Distribution
    OrderQuantity qty_lognorm_scale;
    OrderQuantity lot_size;

    // Order Type
    float limit_order_chance;

    // RNG
    std::mt19937_64 rng;

    // Distributions
    std::uniform_real_distribution<float> uniform_dist_01;
    std::exponential_distribution<float> exp_dist;
    std::lognormal_distribution<float> lognorm_dist;
    std::discrete_distribution<int> event_type_dist;
    std::geometric_distribution<int> geo_dist;

    // Keeping Track of Currently Active Orders in Book
    std::unordered_set<OrderID> active_orders_set;

    // Internal Reference Matching Engine
    //
    // OrderEvent Sink for Generator's Internal Matching Engine
    class GeneratorOrderSink {
    public:
        // Simple List
        std::queue<OrderEvent> order_event_queue;

    public:
        GeneratorOrderSink() = default;
        ~GeneratorOrderSink() = default;

        void Add(const OrderEvent& order_event) {
            order_event_queue.push(order_event);
        }

        // Conform to Concept
        OrderEvent ExtractNext() {
            OrderEvent event = order_event_queue.front();
            return event;
        }
    };
    // TradeEvent Sink for Generator's Internal Matching Engine
    class GeneratorTradeSink {
    public:
        // Simple List
        std::vector<TradeEvent> trade_event_list;

    public:
        GeneratorTradeSink() = default;
        ~GeneratorTradeSink() = default;

        // Conform to Concept
        void Accept(const TradeEvent& trade_event) {
            trade_event_list.push_back(trade_event);
        }
    };
    GeneratorOrderSink order_sink;
    GeneratorTradeSink trade_sink;
    // Internal Matching Engine
    MatchingEngine<GeneratorOrderSink, GeneratorTradeSink> matching_engine;

public:
    explicit OrderEventGenerator(uint64_t seed = 42);
    ~OrderEventGenerator() = default;

    // Generate Next Event
    OrderEvent Step();
};
