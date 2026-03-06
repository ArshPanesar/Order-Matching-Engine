#include "util/utility.h"
#include "generator/order_event_generator.h"
#include <getopt.h>
#include <iostream>
#include <filesystem>
#include <fstream>

// Task Options
std::string task_options[] = {
    "generate_events",
    "format_events",
    "format_trades"
};
enum {
    TASK_GENERATE_EVENTS = 0,
    TASK_FORMAT_EVENTS,
    TASK_FORMAT_TRADES
};

const std::string BINARY_FILE_EXT = ".bin";

void PrintTaskHelp() {
    std::cout << "One of the following tasks must be provided:\n";
    std::cout << task_options[TASK_GENERATE_EVENTS] << " - Generate Order Events and Store them to an Output Binary File.\n";
    std::cout << task_options[TASK_FORMAT_EVENTS] << " - Format Order Events in Binary Format to Text.\n";
    std::cout << task_options[TASK_FORMAT_TRADES] << " - Format Trade Events in Binary Format to Text.\n";
}

// Generating Events Task
bool PerformTask_GenerateEvents(const std::string& output_file_path) {
    // Try creating the Output File
    std::ofstream output_file_stream(output_file_path, std::ios::binary);
    if (!output_file_stream) {
        std::cout << "ERROR: Output File could not be written to. Given Path: " << output_file_path << "\n";
        return false;
    }

    // Create the Generator
    MemoryAllocator mem_allocator((1 << 30));
    size_t max_live_orders = (1 << 22);
    size_t max_order_events = 1000000;
    OrderEventGenerator generator(mem_allocator, 42, max_live_orders);    

    // Large Buffer for Fast Writing
    constexpr size_t BUFFER_SIZE = 1 << 20; // 1 MB
    char buffer[BUFFER_SIZE];
    output_file_stream.rdbuf()->pubsetbuf(buffer, BUFFER_SIZE);

    for (size_t i = 0; i < max_order_events; ++i) {
        OrderEvent event = generator.Step();
        output_file_stream.write(reinterpret_cast<const char*>(&event), sizeof(OrderEvent));
    }

    output_file_stream.close();

    return true;
}

bool ConvertBinaryOrderEventsToText(const std::string& input_file_path, const std::string& output_file_path) {
    // Try creating the Output File
    std::ofstream output_file_stream(output_file_path);
    if (!output_file_stream) {
        std::cout << "ERROR: Output File could not be written to. Given Path: " << output_file_path << "\n";
        return false;
    }

    // Try reading the Input File
    std::ifstream input_file_stream(input_file_path, std::ios::binary);
    if (!input_file_stream.is_open()) {
        std::cout << "ERROR: File could not be read. Given Path: " << input_file_path << "\n";
        return false;
    }

    OrderEvent event;
    while (input_file_stream.read(reinterpret_cast<char*>(&event), sizeof(OrderEvent))) {
        output_file_stream << FormatOrderEvent(event) << '\n';
    }

    input_file_stream.close();
    output_file_stream.close();

    return true;
}

bool ConvertBinaryTradeEventsToText(const std::string& input_file_path, const std::string& output_file_path) {
    // Try creating the Output File
    std::ofstream output_file_stream(output_file_path);
    if (!output_file_stream) {
        std::cout << "ERROR: Output File could not be written to. Given Path: " << output_file_path << "\n";
        return false;
    }

    // Try reading the Input File
    std::ifstream input_file_stream(input_file_path, std::ios::binary);
    if (!input_file_stream.is_open()) {
        std::cout << "ERROR: File could not be read. Given Path: " << input_file_path << "\n";
        return false;
    }

    TradeEvent event;
    while (input_file_stream.read(reinterpret_cast<char*>(&event), sizeof(TradeEvent))) {
        output_file_stream << FormatTradeEvent(event) << '\n';
    }

    input_file_stream.close();
    output_file_stream.close();

    return true;
}

int main(int argc, char** argv) {
    
    // Ensure Task is Provided
    if (argc < 2) {
        PrintTaskHelp();
        return EXIT_FAILURE;
    }
    
    std::string given_task = argv[1];

    // Process Options
    struct option config_options[] = {
        {"input", required_argument, NULL, 'i'}, // Input File Path
        {"output", required_argument, NULL, 'o'}, // Output File Path
        {0, 0, 0, 0}
    };

    // Options Storage
    std::string input_file_path;
    bool input_file_provided = false;
    
    std::string output_file_path;
    bool output_file_provided = false;
    
    // Parse Options
    int o;
    optind = 2; // Skip Program Name and Task
    while ((o = getopt_long(argc, argv, "i:o:", config_options, NULL)) != -1) {
        switch (o) {
            case 'i':
                input_file_path = optarg;
                input_file_provided = true;
                break;

            case 'o':
                output_file_path = optarg;
                output_file_provided = true;
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
    auto CheckStringEquals = [](const std::string& s1, const std::string& s2) {
        return (s1.compare(s2) == 0);
    };
    auto ValidateBinaryInputFile = [&]() {
        if (input_file_provided) {
            if (!IsBinary(input_file_path)) {
                std::cout << "Input File is not Binary. Ensure .bin extension at the end of the file name. Given Path: " << input_file_path << "\n";
                return false;
            }
        } else {
            std::cout << "Input File not provided. Usage: --input <file_path>\n";
            return false;
        }

        return CheckFileExists(input_file_path);
    };
    auto ValidateBinaryOutputFile = [&]() {
        if (output_file_provided) {
            if (!IsBinary(output_file_path)) {
                std::cout << "Output File is not Binary. Ensure .bin extension at the end of the file name. Given Path: " << output_file_path << "\n";
                return false;
            }
        } else {
            std::cout << "Output File not provided. Usage: --output <file_path>\n";
            return false;
        }
        return true;
    };

    if (CheckStringEquals(given_task, task_options[TASK_GENERATE_EVENTS])) {
        if (!ValidateBinaryOutputFile())
            return EXIT_FAILURE;

        if (PerformTask_GenerateEvents(output_file_path)) {
            std::cout << "SUCCESS: Generated Events are now stored in the given Binary File: " << output_file_path << "\n";
        } else {
            std::cout << "ERROR: Output File could not be written to. Given Path: " << output_file_path << "\n";
            return EXIT_FAILURE;
        }
    } else if (CheckStringEquals(given_task, task_options[TASK_FORMAT_EVENTS])) {
        if (ValidateBinaryInputFile()) {
            if (ConvertBinaryOrderEventsToText(input_file_path, output_file_path)) {
                std::cout << "SUCCESS: Order Events written to given text file: " << output_file_path << "\n";
            } else
                return EXIT_FAILURE;
        } else
            return EXIT_FAILURE;
    } else if (CheckStringEquals(given_task, task_options[TASK_FORMAT_TRADES])) {
        if (ValidateBinaryInputFile()) {
            if (ConvertBinaryTradeEventsToText(input_file_path, output_file_path)) {
                std::cout << "SUCCESS: Trade Events written to given text file: " << output_file_path << "\n";
            } else
                return EXIT_FAILURE;
        } else
            return EXIT_FAILURE;
    } else {
        std::cout << "Unknown Task Provided: " << given_task << "\n";
        
        PrintTaskHelp();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
