#pragma once
#include <vector>
#include <queue>
#include <unordered_map>
#include <random>

#include "matching_engine.h"

class OrderEventGenerator {
public:
    struct GeneratorParams {
        uint64_t seed = 42;
        OrderPrice min_price = 50;
        OrderPrice max_price = 200;
        
        // Probabilities of Event Type
        // Index 0 -> NEW
        // Index 1 -> CANCEL
        // Index 2 -> AMEND
        // Must sum to 1.0
        float event_type_class_prob[3] = { 0.7f, 0.2f, 0.1f };

        // Probability of Side bieng BID
        float bid_side_prob = 0.5f;

        // Probability of a LIMIT Order
        float limit_order_prob = 0.9f;
    };

private:
    // Generator Parameters
    GeneratorParams params;
    
    // UID: Simple Increment
    OrderID id_gen;

    // Timestamp: Exponential Differences b/w Timestamps
    double timestamp_ns; // Nanosecond Measure
    float timestamp_lambda;

    // EventType: Class RNG
    OrderID initial_new_events_count; // First Few Events should be New Orders

    // Price: Distribution around Mid Price
    OrderPrice best_price;
    float geo_dist_prob;
    
    // Quantity: Log-Normal Distribution
    OrderQuantity qty_lognorm_scale;
    OrderQuantity lot_size;

    // RNG
    std::mt19937_64 rng;

    // Distributions
    std::uniform_real_distribution<float> uniform_dist_01;
    std::exponential_distribution<float> exp_dist;
    std::lognormal_distribution<float> lognorm_dist;
    std::discrete_distribution<int> event_type_dist;
    std::geometric_distribution<int> geo_dist;

    // Keeping Track of Currently Active Orders in Book
    std::vector<OrderID> active_orders_list;
    std::unordered_map<OrderID, size_t> active_orders_id_table; // Stores ID to Index Mappings

    inline void AddActiveOrder(OrderID id);
    inline void RemoveActiveOrder(OrderID id);

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
            order_event_queue.pop();
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

    explicit OrderEventGenerator(MemoryAllocator& mem_allocator, const GeneratorParams& _params, size_t max_live_orders = (1 << 16), 
                size_t order_table_size = (1 << 17));
    ~OrderEventGenerator() = default;



    // Generate Next Event
    OrderEvent Step();
};
