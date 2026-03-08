#pragma once
#include <iostream>
#include <iomanip>
#include "spsc_queue.h"
#include "../lib/matching_engine.h"
#include <unordered_map>
#include <fstream>

inline std::string FormatOrderEvent(const OrderEvent& event) {
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

inline std::string FormatTradeEvent(const TradeEvent& event) {
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

// 
// Memory
// 

struct MemoryParams {
    size_t max_order_events{};
    size_t mem_allocator_bytes{}; // Total Number of bytes needed for the Memory Allocator

    //
    // Memory Breakdown
    //

    size_t order_table_size{};
    size_t order_table_bytes{};

    size_t order_pool_size{};
    size_t order_pool_bytes{};

    size_t price_level_arr_size{};
    size_t price_level_arr_bytes{};

    size_t price_level_bitset_size{};
    size_t price_level_bitset_bytes{};
};

inline bool ComputeMemoryParams(size_t max_order_events, size_t min_price, size_t max_price, MemoryParams& params) {
    
    if (max_order_events < 1) {
        std::cerr << "Failed to compute memory requirements: Max Order Events should be greater than 0." << "\n";
        return false;
    }
    if (max_price < min_price) {
        std::cerr << "Failed to compute memory requirements: Invalid max and min prices provided. Given Min and Max: " << min_price << ", " << max_price << "\n";
        return false;
    }

    size_t num_price_levels = max_price - min_price + 1;
    params.max_order_events = max_order_events;

    try {
        OrderTable::ComputeMemoryParams(max_order_events, params.order_table_size, params.order_table_bytes);
        OrderNodePool::ComputeMemoryParams(max_order_events, params.order_pool_size, params.order_pool_bytes);
        
        PriceLevelBitset::ComputeMemoryParams(min_price, max_price, params.price_level_bitset_bytes);
        params.price_level_bitset_size = num_price_levels;

        params.price_level_arr_size = num_price_levels;
        if (__builtin_mul_overflow(sizeof(PriceLevel), num_price_levels, &params.price_level_arr_bytes))
            throw std::runtime_error("PriceLevelArrays: Overflow occurred when computing required bytes");

        // Double the Size and Bytes for Bid and Ask Sides
        size_t total_price_level_arr_bytes{};
        size_t total_price_level_bitset_bytes{};
        
        if (__builtin_mul_overflow(params.price_level_arr_bytes, 2, &total_price_level_arr_bytes))
            throw std::runtime_error("MemoryAllocator Bytes: Overflow occurred when doubling size of PriceLevel Arrays");
        if (__builtin_mul_overflow(params.price_level_bitset_bytes, 2, &total_price_level_bitset_bytes))
            throw std::runtime_error("MemoryAllocator Bytes: Overflow occurred when doubling size of PriceLevel Bitsets");

        // Sum up required bytes
        if (__builtin_add_overflow(params.order_table_bytes, params.order_pool_bytes, &params.mem_allocator_bytes))
            throw std::runtime_error("MemoryAllocator Bytes: Overflow occurred when summing OrderTable and OrderNodePool required bytes");
        
        if (__builtin_add_overflow(params.mem_allocator_bytes, total_price_level_arr_bytes, &params.mem_allocator_bytes))
            throw std::runtime_error("MemoryAllocator Bytes: Overflow occurred when adding PriceLevel Array bytes");
        
        if (__builtin_add_overflow(params.mem_allocator_bytes, total_price_level_bitset_bytes, &params.mem_allocator_bytes))
            throw std::runtime_error("MemoryAllocator Bytes: Overflow occurred when adding PriceLevel Bitset bytes");

        // Extra Bytes for Headers or Metadata
        size_t extra_bytes = (1 << 14);
        if (__builtin_add_overflow(params.mem_allocator_bytes, extra_bytes, &params.mem_allocator_bytes))
            throw std::runtime_error("MemoryAllocator Bytes: Overflow occurred when adding Extra Bytes for Metadata");


    } catch (const std::exception& e) {
        std::cerr << "Failed to compute memory requirements: " << e.what() << "\n";
        return false;
    }

    return true;
}

inline void PrintMemoryParams(const MemoryParams& params) {
    constexpr double BYTES_TO_MB = 1.0 / (1024.0 * 1024.0);

    auto ConvertBytesToMegaBytes = [&](size_t bytes) {
        return ((double)bytes * BYTES_TO_MB);
    };

    std::cout << "\nMemory Requirements:\n";

    std::cout << "Max Order Events: " << params.max_order_events << "\n\n";

    std::cout << std::fixed << std::setprecision(2);

    std::cout << "OrderTable:\n";
    std::cout << "  Capacity : " << params.order_table_size << "\n";
    std::cout << "  Memory   : " << params.order_table_bytes << " bytes (" << ConvertBytesToMegaBytes(params.order_table_bytes) << " MB)\n\n";

    std::cout << "OrderNodePool:\n";
    std::cout << "  Capacity : " << params.order_pool_size << "\n";
    std::cout << "  Memory   : " << params.order_pool_bytes << " bytes (" << ConvertBytesToMegaBytes(params.order_pool_bytes) << " MB)\n\n";

    std::cout << "PriceLevel Array (per side):\n";
    std::cout << "  Size     : " << params.price_level_arr_size << "\n";
    std::cout << "  Memory   : " << params.price_level_arr_bytes << " bytes (" << ConvertBytesToMegaBytes(params.price_level_arr_bytes) << " MB)\n\n";

    std::cout << "PriceLevel Bitset (per side):\n";
    std::cout << "  Size     : " << params.price_level_bitset_size << "\n";
    std::cout << "  Memory   : " << params.price_level_bitset_bytes << " bytes (" << ConvertBytesToMegaBytes(params.price_level_bitset_bytes) << " MB)\n\n";

    std::cout << "Total Memory Required (Allocator):\n";
    std::cout << "  " << params.mem_allocator_bytes << " bytes (" << ConvertBytesToMegaBytes(params.mem_allocator_bytes) << " MB)\n";
}

// 
// Matching Engine
// 

struct MatchingEngineConfig {
    size_t max_order_events{};
    size_t min_price{};
    size_t max_price{};
};

// Config File should be in a simple Key=Value Format
inline MatchingEngineConfig LoadConfigFromFile(const std::string& config_file_path) {
    std::ifstream config_input_stream(config_file_path);
    if (!config_input_stream)
        throw std::runtime_error("ERROR: Cannot Open Configuration File!");

    // Container for Key=Value pairs
    std::unordered_map<std::string, std::string> table;
    
    // Parse the Config File
    std::string line;
    while (std::getline(config_input_stream, line)) {
        if (line.empty())
            continue;

        auto pos = line.find('=');
        if (pos == std::string::npos)
            continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        table[key] = value;
    }

    MatchingEngineConfig conf{};

    conf.max_order_events = std::stoull(table["max_order_events"]);
    conf.min_price = std::stoul(table["min_price"]);
    conf.max_price = std::stoul(table["max_price"]);

    return conf;
}
