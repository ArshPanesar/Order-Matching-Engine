#include "../src/generator/order_event_generator.h"
#include <iostream>
#include <chrono>
#include <locale>

class BenchmarkTimer {
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;

    double* duration_ns_ptr;

public:
    BenchmarkTimer(double* duration_ns) : 
        start_time(std::chrono::high_resolution_clock::now()),
        duration_ns_ptr(duration_ns)
    {}
    
    ~BenchmarkTimer() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);

        *duration_ns_ptr = duration_ns.count();
    }

    // Avoiding accidental copies
    BenchmarkTimer(const BenchmarkTimer&) = delete;
    BenchmarkTimer& operator=(const BenchmarkTimer&) = delete;
};


void BenchmarkOrderBook(size_t allocation_bytes, OrderPrice min_price, OrderPrice max_price, size_t num_order_events, 
                            size_t table_size, const std::vector<OrderEvent>& generated_order_events) {
    // OrderBook Operations Benchmarks
    MemoryAllocator mem_allocator(allocation_bytes);
    OrderBook order_book(mem_allocator, min_price, max_price, num_order_events, table_size);
    
    double insert_duration_ns{};
    double remove_duration_ns{};
    double find_duration_ns{};
    
    // Insertion
    {
        BenchmarkTimer timer(&insert_duration_ns);

        for (size_t i = 0; i < generated_order_events.size(); ++i)
            order_book.AddOrder(generated_order_events[i].order, generated_order_events[i].side);
    }
    // Search
    {
        BenchmarkTimer timer(&find_duration_ns);

        for (size_t i = 0; i < generated_order_events.size(); ++i)
            order_book.GetOrderByID(generated_order_events[i].order.id);
    }
    // Removal
    {
        BenchmarkTimer timer(&remove_duration_ns);

        for (size_t i = 0; i < generated_order_events.size(); ++i)
            order_book.RemoveOrder(generated_order_events[i].order.id);
    }

    // Compute Operations per Second
    auto ConvertNanoToSeconds = [](const double& nano_seconds) {
        return (nano_seconds / 1e9);
    };
    double insert_ops_per_sec = num_order_events / ConvertNanoToSeconds(insert_duration_ns);
    double remove_ops_per_sec = num_order_events / ConvertNanoToSeconds(remove_duration_ns);
    
    double find_ns_per_op = find_duration_ns / num_order_events;

    std::cout << "OrderBook Stats for " << num_order_events << " Orders [Only Storage, No Matching Logic]\n";

    std::cout << "Insertion: " << std::fixed << std::setprecision(3) << insert_duration_ns / 1e6 << " ms\n";
    std::cout << "Insertion (ops/s): " << std::fixed << std::setprecision(0) << insert_ops_per_sec << "\n";

    std::cout << "Removal: " << std::fixed << std::setprecision(3) << remove_duration_ns / 1e6 << " ms\n";
    std::cout << "Removal (ops/s): " << std::fixed << std::setprecision(0) << remove_ops_per_sec << "\n";

    std::cout << "Search [Table Lookup]: " << std::fixed << std::setprecision(3) << find_duration_ns << " ns\n";
    std::cout << "Search [Table Lookup]: " << std::fixed << std::setprecision(5) << find_ns_per_op << " ns/op\n";
}

void BenchmarkMatchingEngine(size_t allocation_bytes, OrderPrice min_price, OrderPrice max_price, size_t num_order_events, 
                            size_t table_size, const std::vector<OrderEvent>& generated_order_events) {
    // MatchingEngine Operations Benchmarks
    class BenchmarkOrderEventSink {
        const std::vector<OrderEvent>& event_queue;
        size_t curr_index;
    public:
        BenchmarkOrderEventSink(const std::vector<OrderEvent>& _event_queue) : curr_index(0u), event_queue(_event_queue) {}
        ~BenchmarkOrderEventSink() = default;

        // Conform to Concept
        OrderEvent ExtractNext() {
            if (curr_index == event_queue.size()) {
                OrderEvent stop_event{};
                stop_event.event_type = eOrderEventType::STOP_EXECUTION;
                return stop_event;
            }
            OrderEvent event = event_queue[curr_index++];
            return event;
        }
    };
    class BenchmarkTradeEventSink {
    public:
        std::vector<TradeEvent> trades_list;
    public:
        BenchmarkTradeEventSink(size_t num_order_events) : trades_list() { trades_list.reserve(num_order_events); }
        ~BenchmarkTradeEventSink() = default;

        // Conform to Concept
        void Accept(const TradeEvent& trade_event) {
            trades_list.push_back(trade_event);
        }
    };
    
    // Prepare Matching Engine
    MemoryAllocator mem_allocator(allocation_bytes);
    BenchmarkOrderEventSink orders_sink(generated_order_events);
    BenchmarkTradeEventSink trades_sink(num_order_events);
    MatchingEngine<BenchmarkOrderEventSink, BenchmarkTradeEventSink> matching_engine(orders_sink, trades_sink, mem_allocator, 
                                                                                        min_price, max_price, num_order_events, table_size);
    
    double duration_ns{};
    {
        BenchmarkTimer timer(&duration_ns);
        while (matching_engine.Run());
    }

    size_t num_trades = trades_sink.trades_list.size();

    double duration_sec = duration_ns / 1e9;

    // Compute Stats
    double orders_per_sec = num_order_events / duration_sec;
    double trades_per_sec = num_trades / duration_sec;
    double ns_per_order = duration_ns / num_order_events;

    // Print Stats
    std::cout << "\n\nMatchingEngine Stats for " << num_order_events << " Order Events [Storage and Matching Logic]\n";
    std::cout << "Engine Duration: " << std::fixed << std::setprecision(3) << duration_ns / 1e6 << " ms\n";
    std::cout << "Orders/sec: " << std::fixed << std::setprecision(0) << orders_per_sec << "\n";
    std::cout << "Trades/sec: " << std::fixed << std::setprecision(0) << trades_per_sec << "\n";
    std::cout << "ns per Order Event: " << std::fixed << std::setprecision(2) << ns_per_order << " ns/op\n";
}

int main() {
    // Nice Formatting
    std::cout.imbue(std::locale(""));

    size_t num_order_events = 1000000u;
    size_t bench_min_price = 50u;
    size_t bench_max_price = 200u;
    size_t order_table_size = std::bit_ceil(num_order_events);
    size_t mem_allocator_bytes = (1 << 30);

    // Prepare Generated Events
    std::vector<OrderEvent> generated_order_events;
    generated_order_events.reserve(num_order_events);
    {
        // Prepare Generator
        std::cout << "Generating Random Events...\n";
        MemoryAllocator gen_mem_allocator(mem_allocator_bytes);
        OrderEventGenerator::GeneratorParams gen_params{};
        gen_params.min_price = bench_min_price;
        gen_params.max_price = bench_max_price;
        OrderEventGenerator generator(gen_mem_allocator, gen_params, num_order_events, order_table_size);


        for (size_t i = 0; i < num_order_events; ++i) {
            OrderEvent new_event = generator.Step();
            generated_order_events.push_back(std::move(new_event));
        }
    }

    std::cout << '\n' << num_order_events << " Order Events Generated. Running Benchmarks...\n\n";

    BenchmarkOrderBook(mem_allocator_bytes, bench_min_price, bench_max_price, num_order_events, order_table_size, generated_order_events);
    BenchmarkMatchingEngine(mem_allocator_bytes, bench_min_price, bench_max_price, num_order_events, order_table_size, generated_order_events);

    return EXIT_SUCCESS;
}