#pragma once
#include <cstdint>

// Aliases for Order Info data types
using OrderID = uint64_t;
using OrderTimestamp = uint64_t;
using OrderPrice = uint64_t;
using OrderQuantity = uint64_t;

struct Order
{
    OrderID id{}; // Unique Identifier
    OrderTimestamp timestamp{}; // Timestamp of Order Arrival/Creation
    
    OrderPrice price{};
    OrderQuantity initial_quantity{};
    OrderQuantity remaining_quantity{};

    // Operator Overloads
    //
    // Equivalency Check: Only by ID
    bool operator==(const Order& other) const;
};

enum class eOrderSide : uint8_t {
    BID,
    ASK
};

enum class eOrderType : uint8_t {
    LIMIT,
    MARKET
};
