#include <iostream>
#include <fstream>
#include "util/utility.h"
#include "generator/order_event_generator.h"
#include <chrono>

// File Reader Order Event Sink
class FileOrderEventSink {
private:
    SPSCQueue<OrderEvent> queue;

    std::jthread reader_thread;
public:
    FileOrderEventSink(size_t max_live_orders) : 
        queue(max_live_orders) {};
    ~FileOrderEventSink() = default;
    
    void Run() {

        reader_thread = std::jthread([&]() {
            
            // Linux Only: Pinning this Thread to a separate core
            // cpu_set_t cpuset;
            // CPU_ZERO(&cpuset);
            // CPU_SET(1, &cpuset);
            // pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

            auto start = std::chrono::high_resolution_clock::now();

            std::ifstream input_file_stream("../events.bin", std::ios::binary);
            if (!input_file_stream.is_open()) {
                std::cout << "FileOrderEventSink::Run(): File could not be opened\n";
                return;
            }

            OrderEvent event;
            while (input_file_stream.read(reinterpret_cast<char*>(&event), sizeof(OrderEvent))) {
                while (!queue.Push(event));
            }

            // Signal MatchingEngine to Stop
            OrderEvent stop_event{};
            stop_event.event_type = eOrderEventType::STOP_EXECUTION;
            while (!queue.Push(stop_event));

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
class FileTradeEventSink {
private:
    SPSCQueue<TradeEvent> queue;

    std::jthread writer_thread;

public:
    FileTradeEventSink(size_t max_live_orders) : 
        queue(max_live_orders) {}
    ~FileTradeEventSink() = default;
    
    // Conform to Concept
    void Accept(const TradeEvent& trade_event) {
        while (!queue.Push(trade_event));
    }

    void Run() {
        writer_thread = std::jthread([&]() {
            
            // Linux Only: Pinning this Thread to a separate core
            // cpu_set_t cpuset;
            // CPU_ZERO(&cpuset);
            // CPU_SET(1, &cpuset);
            // pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

            auto start = std::chrono::high_resolution_clock::now();

            std::ofstream output_file_stream("../trade_events.bin", std::ios::binary);
            if (!output_file_stream.is_open()) {
                std::cout << "FileTradeEventSink::Run(): File could not be opened\n";
                return;
            }

            // Large Buffer for Fast Writing
            constexpr size_t BUFFER_SIZE = 1 << 20; // 1 MB
            char buffer[BUFFER_SIZE];
            output_file_stream.rdbuf()->pubsetbuf(buffer, BUFFER_SIZE);

            TradeEvent event;
            while (true) {
                while (!queue.Pop(event)) {  }
                output_file_stream.write(reinterpret_cast<const char*>(&event), sizeof(TradeEvent));
                
                // Stop on Signal
                if (event.type == eTradeEventType::STOP_EXECUTION)
                    break;
            }


            output_file_stream.close();
            

            auto end = std::chrono::high_resolution_clock::now();

            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            // std::cout << "Generator Thread: " << duration.count() << " ms" << std::endl;
        });
    }
};

int main() {

    MemoryAllocator mem_allocator((1u << 30u));
    

    uint64_t max_orders = 7000000u;
    size_t max_live_orders = (1 << 22);


    // Synthetic Data Test
    FileOrderEventSink file_order_sink(max_live_orders);
    FileTradeEventSink file_trade_sink(max_live_orders);
    
    MatchingEngine<FileOrderEventSink, FileTradeEventSink> matching_engine(file_order_sink, file_trade_sink, mem_allocator, 10, 200, max_live_orders);

    // Avoiding Infinite Loop
    std::cout << "Starting Order Stream...\n";

    // Start the Generator
    file_order_sink.Run();
    file_trade_sink.Run();

    // Run Matching Engine in a Separate Thread
    std::jthread engine_thread([&]() {
        // Linux Only: Pinning this Thread to a separate core
        // cpu_set_t cpuset;
        // CPU_ZERO(&cpuset);
        // CPU_SET(4, &cpuset);
        // pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);


        auto start = std::chrono::high_resolution_clock::now();

        while (matching_engine.Run());
        
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Engine Thread: " << duration.count() << " ms" << std::endl;
        // std::cout << "Orders Exhausted\n";
    });
    
    engine_thread.join();


    return 0;
}
