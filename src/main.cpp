#include <iostream>
#include <fstream>
#include <getopt.h>
#include "util/utility.h"
#include "generator/order_event_generator.h"
#include <chrono>
#include <filesystem>

const std::string BINARY_FILE_EXT = ".bin";

// File Reader Order Event Sink
class FileOrderEventSink {
private:
    SPSCQueue<OrderEvent> queue;

    std::jthread reader_thread;
    std::string file_path;
public:
    FileOrderEventSink(size_t max_live_orders, const std::string& _file_path) : 
        queue(max_live_orders), file_path(_file_path) {};
    ~FileOrderEventSink() = default;
    
    void Run() {
        reader_thread = std::jthread([&]() {
            std::ifstream input_file_stream(file_path, std::ios::binary);
            if (!input_file_stream.is_open()) {
                std::cout << "FileOrderEventSink::Run(): File could not be opened\n";
                return;
            }
            
            // Large Buffer for Fast Reading
            constexpr size_t BUFFER_SIZE = 1 << 20; // 1 MB
            char buffer[BUFFER_SIZE];
            input_file_stream.rdbuf()->pubsetbuf(buffer, BUFFER_SIZE);

            OrderEvent event;
            while (input_file_stream.read(reinterpret_cast<char*>(&event), sizeof(OrderEvent))) {
                while (!queue.Push(event));
            }

            // Signal MatchingEngine to Stop
            OrderEvent stop_event{};
            stop_event.event_type = eOrderEventType::STOP_EXECUTION;
            while (!queue.Push(stop_event));
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
    std::string file_path;
    
public:
    FileTradeEventSink(size_t max_live_orders, const std::string& _file_path) : 
        queue(max_live_orders), file_path(_file_path) {}
    ~FileTradeEventSink() = default;
    
    // Conform to Concept
    void Accept(const TradeEvent& trade_event) {
        while (!queue.Push(trade_event));
    }

    void Run() {
        writer_thread = std::jthread([&]() {
            std::ofstream output_file_stream(file_path, std::ios::binary | std::ios::out);
            if (!output_file_stream.is_open()) {
                std::cout << "FileTradeEventSink::Run(): File could not be opened\n";
                return;
            }

            // Large Buffer for Fast Writing
            constexpr size_t BUFFER_SIZE = 1 << 20; // 1 MB
            char buffer[BUFFER_SIZE];
            output_file_stream.rdbuf()->pubsetbuf(buffer, BUFFER_SIZE);

            // Batching
            constexpr size_t BATCH_SIZE = 1 << 14;
            std::vector<TradeEvent> event_batch(BATCH_SIZE);
            size_t batch_count = 0;

            TradeEvent event;
            while (true) {
                while (!queue.Pop(event));

                event_batch[batch_count++] = event;
                if (batch_count == BATCH_SIZE) {
                    output_file_stream.write(reinterpret_cast<const char*>(event_batch.data()), sizeof(TradeEvent) * batch_count);
                    batch_count = 0;
                }

                // Stop on Signal
                if (event.type == eTradeEventType::STOP_EXECUTION)
                    break;
            }
            if (batch_count > 0)
                output_file_stream.write(reinterpret_cast<const char*>(event_batch.data()), sizeof(TradeEvent) * batch_count);

            output_file_stream.close();
        });
    }
};

int main(int argc, char** argv) {

    // Process Options
    struct option config_options[] = {
        {"input", required_argument, NULL, 'i'}, // Input File Path
        {"output", required_argument, NULL, 'o'}, // Output File Path
        {"config", required_argument, NULL, 'c'}, // Engine Config File Path
        {0, 0, 0, 0}
    };

    // Options Storage
    std::string input_file_path;
    bool input_file_provided = false;
    
    std::string output_file_path;
    bool output_file_provided = false;
    
    std::string config_file_path;
    bool config_file_provided = false;
    
    // Parse Options
    int o;
    optind = 1; // Skip Program Name
    while ((o = getopt_long(argc, argv, "i:o:c:", config_options, NULL)) != -1) {
        switch (o) {
            case 'i':
                input_file_path = optarg;
                input_file_provided = true;
                break;

            case 'o':
                output_file_path = optarg;
                output_file_provided = true;
                break;
            
            case 'c':
                config_file_path = optarg;
                config_file_provided = true;
                break;
            
            default:
                break;
        }
    }

    // Helper Funcs
    auto CheckFileExists = [](const std::string& file_path) {
        return std::filesystem::exists(file_path);
    };
    auto IsBinary = [](const std::string& file_path) {
        std::filesystem::path path(file_path);
        if (path.has_extension())
            return (path.extension().string().compare(BINARY_FILE_EXT) == 0);
        return false;
    };
    auto ValidateBinaryInputFile = [&]() {
        if (input_file_provided) {
            if (!CheckFileExists(input_file_path)) {
                std::cout << "Input File is does not exist. Given Path: " << input_file_path << "\n";
                return false;
            }
            if (!IsBinary(input_file_path)) {
                std::cout << "Input File is not Binary. Ensure .bin extension at the end of the file name. Given Path: " << input_file_path << "\n";
                return false;
            }
        } else {
            std::cout << "Input File not provided. Usage: --input <file_path>\n";
            return false;
        }

        return true;
    };
    auto ValidateBinaryOutputFile = [&]() {
        if (output_file_provided) {
            if (!IsBinary(output_file_path)) {
                std::cout << "Output File is not Binary. Ensure .bin extension at the end of the file name. Given Path: " << output_file_path << "\n";
                return false;
            }
        } else {
            std::cout << "Output File not provided. Use: --output <file_path>\n";
            return false;
        }
        return true;
    };
    auto ValidateConfigFile = [&]() {
        if (config_file_provided) {
            if (!CheckFileExists(config_file_path)) {
                std::cout << "Config File does not exist. Given Path: " << config_file_path << "\n";
                return false;
            }
        } else {
            std::cout << "Config File not provided. Use: --config <file_path>\n";
            return false;
        }
        return true;
    };

    if (!ValidateBinaryInputFile() || !ValidateBinaryOutputFile() || !ValidateConfigFile()) {
        return EXIT_FAILURE;
    }

    MatchingEngineConfig config = LoadConfigFromFile(config_file_path);

    size_t max_order_events = config.max_order_events;
    size_t max_live_orders = config.max_live_orders;
    
    MemoryParams mem_params;
    if (!ComputeMemoryParams(max_order_events, max_live_orders, config.min_price, config.max_price, mem_params)) {
        return EXIT_FAILURE;
    }
    PrintMemoryParams(mem_params);

    MemoryAllocator mem_allocator(mem_params.mem_allocator_bytes);    

    // File Sinks
    FileOrderEventSink file_order_sink(NextPowerOf2(max_order_events), input_file_path);
    FileTradeEventSink file_trade_sink(NextPowerOf2(max_order_events), output_file_path);
    
    MatchingEngine<FileOrderEventSink, FileTradeEventSink> matching_engine(file_order_sink, file_trade_sink, mem_allocator, 
        config.min_price, config.max_price, max_live_orders, mem_params.order_table_size);


    // Start the I/O Streams
    file_order_sink.Run();
    file_trade_sink.Run();

    // Run Matching Engine
    auto start = std::chrono::high_resolution_clock::now();

    while (matching_engine.Run());

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Engine Duration: " << duration.count() << " ms" << std::endl;
    
    return 0;
}
