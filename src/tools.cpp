#include "util/utility.h"
#include "generator/order_event_generator.h"
#include <getopt.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cmath>

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
bool PerformTask_GenerateEvents(const std::string& output_file_path, const std::string& config_file_path, OrderEventGenerator::GeneratorParams& gen_params) {
    // Try creating the Output File
    std::ofstream output_file_stream(output_file_path, std::ios::binary);
    if (!output_file_stream) {
        std::cout << "ERROR: Output File could not be written to. Given Path: " << output_file_path << "\n";
        return false;
    }

    // Load the Config File
    MatchingEngineConfig config = LoadConfigFromFile(config_file_path);

    // Create the Generator
    size_t max_order_events = config.max_order_events;

    MemoryParams mem_params;
    if (!ComputeMemoryParams(max_order_events, config.max_live_orders, config.min_price, config.max_price, mem_params)) {
        std::cout << "ERROR: Memory Requirements could not be fulfilled.\n";
        return false;
    }
    PrintMemoryParams(mem_params);

    MemoryAllocator mem_allocator(mem_params.mem_allocator_bytes);
    size_t max_live_orders = config.max_live_orders;

    gen_params.min_price = config.min_price;
    gen_params.max_price = config.max_price;
    
    OrderEventGenerator generator(mem_allocator, gen_params, max_live_orders, mem_params.order_table_size);    

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
        {"config", required_argument, NULL, 'c'}, // Engine Config File Path
        {"gen-seed", required_argument, NULL, 's'}, // Generator: RNG Seed
        {"gen-limit-prob", required_argument, NULL, 'l'}, // Generator: Probability of an OrderEvent bieng a LIMIT Order
        {"gen-bid-prob", required_argument, NULL, 'b'}, // Generator: Probability of an OrderEvent bieng a BID side Order
        {"gen-event-prob-array", required_argument, NULL, 'e'}, // Generator: 3 Probabilities (Must Sum to 1) of an OrderEvent bieng of NEW, CANCEL, or AMEND Type
        {0, 0, 0, 0}
    };

    // Options Storage
    std::string input_file_path;
    bool input_file_provided = false;
    
    std::string output_file_path;
    bool output_file_provided = false;
    
    std::string config_file_path;
    bool config_file_provided = false;
    
    // Optional Parameters for Order Generator
    OrderEventGenerator::GeneratorParams gen_params{};

    // Parse Options
    int o;
    optind = 2; // Skip Program Name and Task
    while ((o = getopt_long(argc, argv, "i:o:c:s:l:b:e:", config_options, NULL)) != -1) {
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
            
            case 's':
                {
                    std::string seed_str = optarg;
                    uint64_t rng_seed = gen_params.seed;
                    try {
                        rng_seed = std::stoull(seed_str);
                    } catch (const std::out_of_range& oor) {
                        std::cout << "Provided Seed is Out of Range for uint64_t type. Given Argument: " << seed_str << "\n";
                        return EXIT_FAILURE;
                    } catch (const std::invalid_argument& e) {
                        std::cout << "Not a Valid Number. Given Argument: " << seed_str << "\n";
                        return EXIT_FAILURE;
                    }

                    gen_params.seed = rng_seed;
                }
                break;
            
            case 'l':
                {
                    std::string limit_prob_str = optarg;
                    auto limit_prob = gen_params.limit_order_prob;
                    try {
                        limit_prob = std::stof(limit_prob_str);
                    } catch (const std::invalid_argument& e) {
                        std::cout << "Not a Valid Number. Given Argument: " << limit_prob_str << "\n";
                        return EXIT_FAILURE;
                    }
                    if (limit_prob < 0.0f || limit_prob > 1.0f) {
                        std::cout << "Limit Order Probability cannot be greater than 1 or Less than 0. Given: " << limit_prob << "\n";
                        return EXIT_FAILURE;
                    }

                    gen_params.limit_order_prob = limit_prob;
                }
                break;
            
            case 'b':
                {
                    std::string bid_prob_str = optarg;
                    auto bid_prob = gen_params.bid_side_prob;
                    try {
                        bid_prob = std::stof(bid_prob_str);
                    } catch (const std::invalid_argument& e) {
                        std::cout << "Not a Valid Number. Given Argument: " << bid_prob_str << "\n";
                        return EXIT_FAILURE;
                    }
                    if (bid_prob < 0.0f || bid_prob > 1.0f) {
                        std::cout << "Bid Order Probability cannot be greater than 1 or Less than 0. Given: " << bid_prob << "\n";
                        return EXIT_FAILURE;
                    }

                    gen_params.bid_side_prob = bid_prob;
                }
                break;
            
            case 'e':
                {
                    std::string event_type_prob_str = optarg;
                    float event_type_prob[3] = {0.0f, 0.0f, 0.0f}; // Exactly 3 Elements
                    
                    // Extract Comma Separated List
                    std::vector<std::string> prob_str_list(3);
                    std::stringstream str_stream(event_type_prob_str);
                    std::string word;
                    size_t count = 0;
                    while (std::getline(str_stream, word, ',') && count < 3) {
                        prob_str_list[count] = word;
                        ++count;
                    }
                    if (count != 3) {
                        std::cout << "Event Probability Array must have exactly 3 floats that sum to 1. Given Count: " << count << "\n";
                        return EXIT_FAILURE;
                    }

                    // Convert to Floating Point
                    count = 0;
                    float sum = 0.0f;
                    for (auto& prob_str : prob_str_list) {
                        try {
                            event_type_prob[count] = std::stof(prob_str);
                            if (event_type_prob[count] < 0.0f) {
                                std::cout << "Event Probability Array must not have negative floats. Given Argument: " << prob_str << "\n";
                                return EXIT_FAILURE;    
                            }
                            sum += event_type_prob[count++];
                        } catch (const std::invalid_argument& e) {
                            std::cout << "Not a Valid Number. Given Argument: " << prob_str << "\n";
                            return EXIT_FAILURE;
                        }
                    }

                    // Verify Sum to 1 (with some tolerance)
                    if (std::abs(sum - 1.0f) > 1e-5f) {
                        std::cout << "Event Probability Array must have exactly 3 floats that sum to 1. Given: " << event_type_prob_str << "\n";
                        return EXIT_FAILURE;
                    }
                    
                    for (count = 0; count < 3; ++count)
                        gen_params.event_type_class_prob[count] = event_type_prob[count];
                }
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


    if (CheckStringEquals(given_task, task_options[TASK_GENERATE_EVENTS])) {
        if (!ValidateBinaryOutputFile())
            return EXIT_FAILURE;
        if (!ValidateConfigFile())
            return EXIT_FAILURE;

        if (PerformTask_GenerateEvents(output_file_path, config_file_path, gen_params)) {
            std::cout << "SUCCESS: Generated Events are now stored in the given Binary File: " << output_file_path << "\n";
        } else
            return EXIT_FAILURE;
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
