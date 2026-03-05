#include <iostream>
#include <fstream>
#include "util/spsc_queue.h"
#include "generator/order_event_generator.h"
#include <chrono>

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

void GenerateEventsToFile(MemoryAllocator& mem_allocator, size_t max_live_orders, size_t max_order_events) {
    // Create the Generator
    OrderEventGenerator generator(mem_allocator, 42, max_live_orders);

    // Open binary output file
    std::ofstream output_file_stream("generated_events.bin", std::ios::binary);
    if (!output_file_stream) {
        std::cout << "Failed to Write File!\n";
    }

    // Large Buffer for Fast Writing
    constexpr size_t BUFFER_SIZE = 1 << 20; // 1MB
    static char buffer[BUFFER_SIZE];
    output_file_stream.rdbuf()->pubsetbuf(buffer, BUFFER_SIZE);

    for (size_t i = 0; i < max_order_events; ++i) {
        OrderEvent event = generator.Step();
        output_file_stream.write(reinterpret_cast<const char*>(&event), sizeof(OrderEvent));
    }

    output_file_stream.close();
}

// Running Engine on Synthetic Order Data

// Input Event Sink
class SyntheticOrderEventSink {
private:
    OrderEventGenerator generator;
    SPSCQueue<OrderEvent> queue;

    std::jthread generator_thread;
public:
    SyntheticOrderEventSink(MemoryAllocator& mem_allocator, size_t max_live_orders) : 
        generator(mem_allocator, 42, max_live_orders),
        queue(max_live_orders) {};
    ~SyntheticOrderEventSink() = default;
    
    void Run(const uint64_t& max_orders) {

        generator_thread = std::jthread([&]() {
            
            // Linux Only: Pinning this Thread to a separate core
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(1, &cpuset);
            pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

            auto start = std::chrono::high_resolution_clock::now();
            
            uint64_t order_count = 0;
            while (order_count < max_orders) {
                OrderEvent next_event = generator.Step();
                while (!queue.Push(next_event));
                ++order_count;
            }

            auto end = std::chrono::high_resolution_clock::now();

            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            // std::cout << "Generator Thread: " << duration.count() << " ms" << std::endl;
        });
    }

    // Conform to Concept
    OrderEvent ExtractNext() {
        OrderEvent new_event;
        while(!queue.Pop(new_event));
        return new_event;
    }
};


// File Reader Order Event Sink
class FileOrderEventSink {
private:
    SPSCQueue<OrderEvent> queue;

    std::jthread reader_thread;
public:
    FileOrderEventSink(size_t max_live_orders) : 
        queue(max_live_orders) {};
    ~FileOrderEventSink() = default;
    
    void Run(const uint64_t& max_orders) {

        reader_thread = std::jthread([&]() {
            
            // Linux Only: Pinning this Thread to a separate core
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(1, &cpuset);
            pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

            auto start = std::chrono::high_resolution_clock::now();

            std::ifstream input_file_stream("../generated_events.bin", std::ios::binary);
            if (!input_file_stream.is_open()) {
                std::cout << "FileOrderEventSink::Run(): File could not be opened\n";
                return;
            }

            OrderEvent event;

            uint64_t order_count = 0;
            while (order_count < max_orders) {
                input_file_stream.read(reinterpret_cast<char*>(&event), sizeof(OrderEvent));
                while (!queue.Push(event));
                ++order_count;
            }

            auto end = std::chrono::high_resolution_clock::now();

            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            // std::cout << "Generator Thread: " << duration.count() << " ms" << std::endl;
        });
    }

    // Conform to Concept
    OrderEvent ExtractNext() {
        OrderEvent new_event;
        while(!queue.Pop(new_event));
        return new_event;
    }
};

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

int main() {

    MemoryAllocator mem_allocator((1u << 30u));
    

    uint64_t max_orders = 7000000u;
    size_t max_live_orders = (1 << 22);


    // Synthetic Data Test
    FileOrderEventSink synthetic_order_sink(max_live_orders);
    SyntheticTradeEventSink synthetic_trade_sink;
    
    MatchingEngine<FileOrderEventSink, SyntheticTradeEventSink> synthetic_data_engine(synthetic_order_sink, synthetic_trade_sink, mem_allocator, 10, 200, max_live_orders);

    // Avoiding Infinite Loop
    std::cout << "Starting Order Stream...\n";

    // Start the Generator
    synthetic_order_sink.Run(max_orders);
    
    // Run Matching Engine in a Separate Thread
    std::jthread engine_thread([&]() {
        // Linux Only: Pinning this Thread to a separate core
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(4, &cpuset);
        pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);


        auto start = std::chrono::high_resolution_clock::now();

        uint64_t order_count = 0;        
        while (order_count < max_orders) {
            
            synthetic_data_engine.Run();

            // synthetic_trade_sink.Print();
            synthetic_trade_sink.Clear();
            ++order_count;
        }
        
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Engine Thread: " << duration.count() << " ms" << std::endl;
        // std::cout << "Orders Exhausted\n";
    });
    
    engine_thread.join();


    return 0;
}
