#include "matching_core.h"
#include <gtest/gtest.h>

/*
 *  Testing for Default Initialization of OrderEvent object
 */
TEST(MatchingCoreTest, OrderEventDefaultInitialization) {
    // Value-Initialized
    OrderEvent new_order_event{};

    // Ensure Default Values
    EXPECT_EQ(new_order_event.order, Order{});
    EXPECT_EQ(new_order_event.event_type, eOrderEventType::NEW);
    EXPECT_EQ(new_order_event.side, eOrderSide::BID);
    EXPECT_EQ(new_order_event.type, eOrderType::LIMIT);
}

/*
 *  Check if Explicit Values are retained in the OrderEvent object
 */
TEST(MatchingCoreTest, OrderEventValuesRetained) {
    OrderEvent new_order_event{};

    new_order_event.event_type = eOrderEventType::AMEND;
    new_order_event.side = eOrderSide::BID;
    new_order_event.type = eOrderType::LIMIT;

    // Ensure Default Values
    EXPECT_EQ(new_order_event.order, Order{});
    EXPECT_EQ(new_order_event.event_type, eOrderEventType::AMEND);
    EXPECT_EQ(new_order_event.side, eOrderSide::BID);
    EXPECT_EQ(new_order_event.type, eOrderType::LIMIT);
}

/*
 *  Testing for Default Initialization of TradeEvent object
 */
TEST(MatchingCoreTest, TradeEventDefaultInitialization) {
    // Value-Initialized
    TradeEvent new_trade_event{};

    // Ensure Default Values
    EXPECT_EQ(new_trade_event.trade_id, 0u);
    EXPECT_EQ(new_trade_event.timestamp, 0u);
    EXPECT_EQ(new_trade_event.bid_order_id, 0u);
    EXPECT_EQ(new_trade_event.ask_order_id, 0u);
    EXPECT_EQ(new_trade_event.executed_price, 0u);
    EXPECT_EQ(new_trade_event.filled_quantity, 0u);
}

/*
 *  Check if Explicit Values are retained in the TradeEvent object
 */
TEST(MatchingCoreTest, TradeEventValuesRetained) {
    // Value-Initialized
    TradeEvent new_trade_event{};

    new_trade_event.trade_id = 54321u;
    new_trade_event.timestamp = 9999u;
    new_trade_event.bid_order_id = 10001u;
    new_trade_event.ask_order_id = 20001u;
    new_trade_event.executed_price = 2000u;
    new_trade_event.filled_quantity = 8000u;
    
    // Ensure Default Values
    EXPECT_EQ(new_trade_event.trade_id, 54321u);
    EXPECT_EQ(new_trade_event.timestamp, 9999u);
    EXPECT_EQ(new_trade_event.bid_order_id, 10001u);
    EXPECT_EQ(new_trade_event.ask_order_id, 20001u);
    EXPECT_EQ(new_trade_event.executed_price, 2000u);
    EXPECT_EQ(new_trade_event.filled_quantity, 8000u);
}