#include <gtest/gtest.h>

#include "matching_engine_test.h"


// 
// LIMIT ORDERS
// 

// Adding a Single Resting Order to an Empty Book
TEST(MatchingEngineLimitOrdersTest, RestingOrderOnly) {
    TestTradeEventSink sink;
    MatchingEngine<TestTradeEventSink> engine(sink);

    Order o1{1, 100, 100, 10, 10};
    OrderEvent event_1 = MakeOrderEvent(o1, eOrderEventType::NEW, eOrderSide::BID, eOrderType::LIMIT);
    engine.ProcessEvent(event_1);

    ASSERT_EQ(sink.trade_event_list.size(), 0); // No trade
}

// Single Trade
TEST(MatchingEngineLimitOrdersTest, SingleCrossingTrade) {
    TestTradeEventSink sink;
    MatchingEngine<TestTradeEventSink> engine(sink);

    // Resting sell order
    Order sell{1, 100, 105, 5, 5};
    OrderEvent event_1 = MakeOrderEvent(sell, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::LIMIT);
    engine.ProcessEvent(event_1);

    // Aggressive buy
    Order buy{2, 101, 110, 5, 5};
    OrderEvent event_2 = MakeOrderEvent(buy, eOrderEventType::NEW, eOrderSide::BID, eOrderType::LIMIT);
    engine.ProcessEvent(event_2);

    ASSERT_EQ(sink.trade_event_list.size(), 1);
    EXPECT_EQ(sink.trade_event_list[0].bid_order_id, 2);
    EXPECT_EQ(sink.trade_event_list[0].ask_order_id, 1);
    EXPECT_EQ(sink.trade_event_list[0].executed_price, 105);
    EXPECT_EQ(sink.trade_event_list[0].filled_quantity, 5);
}

// Trade involving a Partial Fill
TEST(MatchingEngineLimitOrdersTest, PartialFillRestingOrder) {
    TestTradeEventSink sink;
    MatchingEngine<TestTradeEventSink> engine(sink);

    Order sell{1, 100, 105, 10, 10};
    OrderEvent event_1 = MakeOrderEvent(sell, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::LIMIT);
    engine.ProcessEvent(event_1);

    Order buy{2, 101, 110, 5, 5};
    OrderEvent event_2 = MakeOrderEvent(buy, eOrderEventType::NEW, eOrderSide::BID, eOrderType::LIMIT);
    engine.ProcessEvent(event_2);

    ASSERT_EQ(sink.trade_event_list.size(), 1);
    EXPECT_EQ(sink.trade_event_list[0].filled_quantity, 5);

    // Resting sell should now have 5 remaining
    const Order* remaining_sell = engine.GetOrderBook()->GetBestAsk();
    ASSERT_NE(remaining_sell, nullptr);
    EXPECT_EQ(remaining_sell->remaining_quantity, 5);
}

// Testing Multiple Orders Filled: FIFO Level
TEST(MatchingEngineLimitOrdersTest, MultipleFillsFIFO) {
    TestTradeEventSink sink;
    MatchingEngine<TestTradeEventSink> engine(sink);

    // Two resting sell orders at same price
    Order s1{1, 100, 105, 5, 5};
    Order s2{2, 101, 105, 5, 5};
    
    OrderEvent event_1 = MakeOrderEvent(s1, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::LIMIT);
    OrderEvent event_2 = MakeOrderEvent(s2, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::LIMIT);
    
    engine.ProcessEvent(event_1);
    engine.ProcessEvent(event_2);

    // Aggressive buy order quantity = 8
    Order buy{3, 102, 110, 8, 8};
    OrderEvent event_3 = MakeOrderEvent(buy, eOrderEventType::NEW, eOrderSide::BID, eOrderType::LIMIT);
    engine.ProcessEvent(event_3);

    ASSERT_EQ(sink.trade_event_list.size(), 2);
    EXPECT_EQ(sink.trade_event_list[0].filled_quantity, 5); // First resting order fully filled
    EXPECT_EQ(sink.trade_event_list[1].filled_quantity, 3); // Second partially filled
}

// Testing Multiple Orders Filled: Price Levels
TEST(MatchingEngineLimitOrdersTest, MultiplePriceLevels) {
    TestTradeEventSink sink;
    MatchingEngine<TestTradeEventSink> engine(sink);

    Order s1{1, 100, 105, 5, 5};
    Order s2{2, 101, 106, 5, 5};

    OrderEvent event_1 = MakeOrderEvent(s1, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::LIMIT);
    OrderEvent event_2 = MakeOrderEvent(s2, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::LIMIT);
    
    engine.ProcessEvent(event_1);
    engine.ProcessEvent(event_2);

    // Aggressive buy order quantity = 7
    Order buy{3, 102, 110, 7, 7};
    OrderEvent event_3 = MakeOrderEvent(buy, eOrderEventType::NEW, eOrderSide::BID, eOrderType::LIMIT);
    engine.ProcessEvent(event_3);

    ASSERT_EQ(sink.trade_event_list.size(), 2);
    EXPECT_EQ(sink.trade_event_list[0].executed_price, 105);
    EXPECT_EQ(sink.trade_event_list[0].filled_quantity, 5);
    EXPECT_EQ(sink.trade_event_list[1].executed_price, 106);
    EXPECT_EQ(sink.trade_event_list[1].filled_quantity, 2);
}


// Long Running Test Helper Function
OrderEvent MakeLimitOrder(OrderID id, eOrderSide side, OrderPrice price, OrderQuantity qty, OrderTimestamp timestamp) {
    Order order;
    order.id = id;
    order.price = price;
    order.initial_quantity = qty;
    order.remaining_quantity = qty;
    order.timestamp = timestamp;

    OrderEvent evt;
    evt.event_type = eOrderEventType::NEW;
    evt.side = side;
    evt.type = eOrderType::LIMIT;
    evt.order = order;

    return evt;
}


// Long Running Test
TEST(MatchingEngineLimitOrdersTest, LongRunningTest) {
    TestTradeEventSink sink;
    MatchingEngine<TestTradeEventSink> engine(sink);

    OrderTimestamp current_timestamp = 1;

    // Build Book
    OrderEvent event_1 = MakeLimitOrder(1, eOrderSide::BID, 100, 10, current_timestamp++);
    OrderEvent event_2 = MakeLimitOrder(2, eOrderSide::BID, 101, 5, current_timestamp++);
    OrderEvent event_3 = MakeLimitOrder(3, eOrderSide::ASK, 105, 7, current_timestamp++);
    OrderEvent event_4 = MakeLimitOrder(4, eOrderSide::ASK, 104, 8, current_timestamp++);

    engine.ProcessEvent(event_1);
    engine.ProcessEvent(event_2);
    engine.ProcessEvent(event_3);
    engine.ProcessEvent(event_4);

    EXPECT_TRUE(sink.trade_event_list.empty());

    // First Crossing
    OrderEvent event_5 = MakeLimitOrder(5, eOrderSide::BID, 104, 6, current_timestamp++);
    engine.ProcessEvent(event_5);

    ASSERT_EQ(sink.trade_event_list.size(), 1);
    EXPECT_EQ(sink.trade_event_list[0].bid_order_id, 5);
    EXPECT_EQ(sink.trade_event_list[0].ask_order_id, 4);
    EXPECT_EQ(sink.trade_event_list[0].executed_price, 104);
    EXPECT_EQ(sink.trade_event_list[0].filled_quantity, 6);


    OrderEvent event_6 = MakeLimitOrder(6, eOrderSide::BID, 104, 5, current_timestamp++);
    engine.ProcessEvent(event_6);

    ASSERT_EQ(sink.trade_event_list.size(), 2);
    EXPECT_EQ(sink.trade_event_list[1].bid_order_id, 6);
    EXPECT_EQ(sink.trade_event_list[1].ask_order_id, 4);
    EXPECT_EQ(sink.trade_event_list[1].executed_price, 104);
    EXPECT_EQ(sink.trade_event_list[1].filled_quantity, 2);

    // Large Quantity Sold
    OrderEvent event_7 = MakeLimitOrder(7, eOrderSide::ASK, 100, 12, current_timestamp++);
    engine.ProcessEvent(event_7);

    ASSERT_EQ(sink.trade_event_list.size(), 5);

    // Trade 2
    EXPECT_EQ(sink.trade_event_list[2].bid_order_id, 6);
    EXPECT_EQ(sink.trade_event_list[2].ask_order_id, 7);
    EXPECT_EQ(sink.trade_event_list[2].executed_price, 104);
    EXPECT_EQ(sink.trade_event_list[2].filled_quantity, 3);

    // Trade 3
    EXPECT_EQ(sink.trade_event_list[3].bid_order_id, 2);
    EXPECT_EQ(sink.trade_event_list[3].ask_order_id, 7);
    EXPECT_EQ(sink.trade_event_list[3].executed_price, 101);
    EXPECT_EQ(sink.trade_event_list[3].filled_quantity, 5);

    // Trade 4
    EXPECT_EQ(sink.trade_event_list[4].bid_order_id, 1);
    EXPECT_EQ(sink.trade_event_list[4].ask_order_id, 7);
    EXPECT_EQ(sink.trade_event_list[4].executed_price, 100);
    EXPECT_EQ(sink.trade_event_list[4].filled_quantity, 4);

    // Partial Resting Ask
    OrderEvent event_8 = MakeLimitOrder(8, eOrderSide::ASK, 100, 10, current_timestamp++);
    engine.ProcessEvent(event_8);

    ASSERT_EQ(sink.trade_event_list.size(), 6);
    EXPECT_EQ(sink.trade_event_list[5].bid_order_id, 1);
    EXPECT_EQ(sink.trade_event_list[5].ask_order_id, 8);
    EXPECT_EQ(sink.trade_event_list[5].executed_price, 100);
    EXPECT_EQ(sink.trade_event_list[5].filled_quantity, 6);

    // Late Buys
    OrderEvent event_9 = MakeLimitOrder(9, eOrderSide::BID, 99, 5, current_timestamp++);
    engine.ProcessEvent(event_9);
    EXPECT_EQ(sink.trade_event_list.size(), 6);  // no trade

    OrderEvent event_10 = MakeLimitOrder(10, eOrderSide::BID, 100, 3, current_timestamp++);
    engine.ProcessEvent(event_10);

    ASSERT_EQ(sink.trade_event_list.size(), 7);
    EXPECT_EQ(sink.trade_event_list[6].bid_order_id, 10);
    EXPECT_EQ(sink.trade_event_list[6].ask_order_id, 8);
    EXPECT_EQ(sink.trade_event_list[6].executed_price, 100);
    EXPECT_EQ(sink.trade_event_list[6].filled_quantity, 3);

    OrderEvent event_11 = MakeLimitOrder(11, eOrderSide::BID, 100, 2, current_timestamp++);
    engine.ProcessEvent(event_11);

    ASSERT_EQ(sink.trade_event_list.size(), 8);
    EXPECT_EQ(sink.trade_event_list[7].bid_order_id, 11);
    EXPECT_EQ(sink.trade_event_list[7].ask_order_id, 8);
    EXPECT_EQ(sink.trade_event_list[7].executed_price, 100);
    EXPECT_EQ(sink.trade_event_list[7].filled_quantity, 1);
}


// 
// MARKET ORDERS
// 


// Market Order on Empty Book
TEST(MatchingEngineMarketOrdersTest, MarketOrderOnEmptyBook) {
    TestTradeEventSink sink;
    MatchingEngine<TestTradeEventSink> engine(sink);

    Order s1{1, 1, 0, 5, 5};
    OrderEvent event_1 = MakeOrderEvent(s1, eOrderEventType::NEW, eOrderSide::BID, eOrderType::MARKET);
    
    engine.ProcessEvent(event_1);

    // No Trade should occur, order is rejected
    EXPECT_EQ(sink.trade_event_list.size(), 0);
    EXPECT_EQ(engine.GetOrderBook()->GetBestAsk(), nullptr); // Book Empty
    EXPECT_EQ(engine.GetOrderBook()->GetBestBid(), nullptr); // Book Empty
}

// Market Order Fully Filled by a Single Ask
TEST(MatchingEngineMarketOrdersTest, MarketOrderFullyFilled) {
    TestTradeEventSink sink;
    MatchingEngine<TestTradeEventSink> engine(sink);

    // Initial Book with a Single Ask
    Order s1{1, 1, 100, 50, 50};
    OrderEvent event_1 = MakeOrderEvent(s1, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::LIMIT);
    
    engine.ProcessEvent(event_1);

    // Market Order
    Order s2{2, 2, 0, 50, 50};
    OrderEvent event_2 = MakeOrderEvent(s2, eOrderEventType::NEW, eOrderSide::BID, eOrderType::MARKET);
    
    engine.ProcessEvent(event_2);

    ASSERT_EQ(sink.trade_event_list.size(), 1); // Exactly One Trade
    EXPECT_EQ(engine.GetOrderBook()->GetBestAsk(), nullptr); // Book Emptied
    EXPECT_EQ(engine.GetOrderBook()->GetBestBid(), nullptr); // Book Emptied

    EXPECT_EQ(sink.trade_event_list[0].executed_price, 100);
    EXPECT_EQ(sink.trade_event_list[0].filled_quantity, 50);
}

// Market Order Partially Filled by a Single Ask
TEST(MatchingEngineMarketOrdersTest, MarketOrderPartiallyFilled) {
    TestTradeEventSink sink;
    MatchingEngine<TestTradeEventSink> engine(sink);

    // Initial Book with a Single Ask
    Order s1{1, 1, 100, 100, 100};
    OrderEvent event_1 = MakeOrderEvent(s1, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::LIMIT);
    
    engine.ProcessEvent(event_1);

    // Market Order
    Order s2{2, 2, 0, 40, 40};
    OrderEvent event_2 = MakeOrderEvent(s2, eOrderEventType::NEW, eOrderSide::BID, eOrderType::MARKET);
    
    engine.ProcessEvent(event_2);

    ASSERT_EQ(sink.trade_event_list.size(), 1); // Exactly One Trade

    EXPECT_EQ(sink.trade_event_list[0].executed_price, 100);
    EXPECT_EQ(sink.trade_event_list[0].filled_quantity, 40);

    EXPECT_EQ(engine.GetOrderBook()->GetBestAsk()->price, 100);
    EXPECT_EQ(engine.GetOrderBook()->GetBestAsk()->remaining_quantity, 60);
}

// Market Order at Multiple Price Levels
TEST(MatchingEngineMarketOrdersTest, MarketOrderMultiplePriceLevels) {
    TestTradeEventSink sink;
    MatchingEngine<TestTradeEventSink> engine(sink);

    // Initial Book with a Single Ask
    Order s1{1, 1, 100, 50, 50};
    Order s2{2, 2, 101, 60, 60};
    Order s3{3, 3, 102, 40, 40};
    
    OrderEvent event_1 = MakeOrderEvent(s1, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::LIMIT);
    OrderEvent event_2 = MakeOrderEvent(s2, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::LIMIT);
    OrderEvent event_3 = MakeOrderEvent(s3, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::LIMIT);
    
    engine.ProcessEvent(event_1);
    engine.ProcessEvent(event_2);
    engine.ProcessEvent(event_3);

    // Market Order
    Order m{4, 4, 0, 120, 120};
    OrderEvent event_4 = MakeOrderEvent(m, eOrderEventType::NEW, eOrderSide::BID, eOrderType::MARKET);
    
    engine.ProcessEvent(event_4);

    ASSERT_EQ(sink.trade_event_list.size(), 3);

    EXPECT_EQ(sink.trade_event_list[0].executed_price, 100);
    EXPECT_EQ(sink.trade_event_list[0].filled_quantity, 50);

    EXPECT_EQ(sink.trade_event_list[1].executed_price, 101);
    EXPECT_EQ(sink.trade_event_list[1].filled_quantity, 60);
    
    EXPECT_EQ(sink.trade_event_list[2].executed_price, 102);
    EXPECT_EQ(sink.trade_event_list[2].filled_quantity, 10);
    
    EXPECT_EQ(engine.GetOrderBook()->GetBestAsk()->price, 102);
    EXPECT_EQ(engine.GetOrderBook()->GetBestAsk()->remaining_quantity, 30);
}

// Market Order exhausts Liquidity
TEST(MatchingEngineMarketOrdersTest, MarketOrderExhaustsLiquidity) {
    TestTradeEventSink sink;
    MatchingEngine<TestTradeEventSink> engine(sink);

    // Initial Book with a Single Ask
    Order s1{1, 1, 100, 50, 50};
    Order s2{2, 2, 101, 50, 50};
    
    OrderEvent event_1 = MakeOrderEvent(s1, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::LIMIT);
    OrderEvent event_2 = MakeOrderEvent(s2, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::LIMIT);
    
    engine.ProcessEvent(event_1);
    engine.ProcessEvent(event_2);

    // Market Order
    Order m{3, 3, 0, 200, 200};
    OrderEvent event_3 = MakeOrderEvent(m, eOrderEventType::NEW, eOrderSide::BID, eOrderType::MARKET);
    
    engine.ProcessEvent(event_3);

    ASSERT_EQ(sink.trade_event_list.size(), 2);

    EXPECT_EQ(sink.trade_event_list[0].executed_price, 100);
    EXPECT_EQ(sink.trade_event_list[0].filled_quantity, 50);

    EXPECT_EQ(sink.trade_event_list[1].executed_price, 101);
    EXPECT_EQ(sink.trade_event_list[1].filled_quantity, 50);
    
    EXPECT_EQ(engine.GetOrderBook()->GetBestAsk(), nullptr);
}

// Market Sell Order
TEST(MatchingEngineMarketOrdersTest, MarketOrderSell) {
    TestTradeEventSink sink;
    MatchingEngine<TestTradeEventSink> engine(sink);

    // Initial Book with a Single Ask
    Order s1{1, 1, 99, 40, 40};
    Order s2{2, 2, 98, 60, 60};
    
    OrderEvent event_1 = MakeOrderEvent(s1, eOrderEventType::NEW, eOrderSide::BID, eOrderType::LIMIT);
    OrderEvent event_2 = MakeOrderEvent(s2, eOrderEventType::NEW, eOrderSide::BID, eOrderType::LIMIT);
    
    engine.ProcessEvent(event_1);
    engine.ProcessEvent(event_2);

    // Market Order
    Order m{3, 3, 0, 70, 70};
    OrderEvent event_3 = MakeOrderEvent(m, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::MARKET);
    
    engine.ProcessEvent(event_3);

    ASSERT_EQ(sink.trade_event_list.size(), 2);

    EXPECT_EQ(sink.trade_event_list[0].executed_price, 99);
    EXPECT_EQ(sink.trade_event_list[0].filled_quantity, 40);

    EXPECT_EQ(sink.trade_event_list[1].executed_price, 98);
    EXPECT_EQ(sink.trade_event_list[1].filled_quantity, 30);
    
    EXPECT_EQ(engine.GetOrderBook()->GetBestBid()->price, 98);
    EXPECT_EQ(engine.GetOrderBook()->GetBestBid()->remaining_quantity, 30);
}

// Market Order must Respect FIFO
TEST(MatchingEngineMarketOrdersTest, MarketOrderFIFO) {
    TestTradeEventSink sink;
    MatchingEngine<TestTradeEventSink> engine(sink);

    // Initial Book with a Single Ask
    Order s1{1, 1, 100, 30, 30};
    Order s2{2, 2, 100, 20, 20};
    
    OrderEvent event_1 = MakeOrderEvent(s1, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::LIMIT);
    OrderEvent event_2 = MakeOrderEvent(s2, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::LIMIT);
    
    engine.ProcessEvent(event_1);
    engine.ProcessEvent(event_2);

    // Market Order
    Order m{3, 3, 0, 40, 40};
    OrderEvent event_3 = MakeOrderEvent(m, eOrderEventType::NEW, eOrderSide::BID, eOrderType::MARKET);
    
    engine.ProcessEvent(event_3);

    ASSERT_EQ(sink.trade_event_list.size(), 2);

    EXPECT_EQ(sink.trade_event_list[0].executed_price, 100);
    EXPECT_EQ(sink.trade_event_list[0].filled_quantity, 30);

    EXPECT_EQ(sink.trade_event_list[1].executed_price, 100);
    EXPECT_EQ(sink.trade_event_list[1].filled_quantity, 10);
    
    EXPECT_EQ(engine.GetOrderBook()->GetBestAsk()->price, 100);
    EXPECT_EQ(engine.GetOrderBook()->GetBestAsk()->remaining_quantity, 10);
}

// Market Order exactly filled by Multiple Levels
TEST(MatchingEngineMarketOrdersTest, MarketOrderExactFill) {
    TestTradeEventSink sink;
    MatchingEngine<TestTradeEventSink> engine(sink);

    // Initial Book with a Single Ask
    Order s1{1, 1, 100, 50, 50};
    Order s2{2, 2, 101, 50, 50};
    
    OrderEvent event_1 = MakeOrderEvent(s1, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::LIMIT);
    OrderEvent event_2 = MakeOrderEvent(s2, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::LIMIT);
    
    engine.ProcessEvent(event_1);
    engine.ProcessEvent(event_2);

    // Market Order
    Order m{3, 3, 0, 100, 100};
    OrderEvent event_3 = MakeOrderEvent(m, eOrderEventType::NEW, eOrderSide::BID, eOrderType::MARKET);
    
    engine.ProcessEvent(event_3);

    ASSERT_EQ(sink.trade_event_list.size(), 2);

    EXPECT_EQ(sink.trade_event_list[0].executed_price, 100);
    EXPECT_EQ(sink.trade_event_list[0].filled_quantity, 50);

    EXPECT_EQ(sink.trade_event_list[1].executed_price, 101);
    EXPECT_EQ(sink.trade_event_list[1].filled_quantity, 50);
    
    EXPECT_EQ(engine.GetOrderBook()->GetBestAsk(), nullptr);
}

// Market Order does not touch same side
TEST(MatchingEngineMarketOrdersTest, MarketOrderIgnoreSameSide) {
    TestTradeEventSink sink;
    MatchingEngine<TestTradeEventSink> engine(sink);

    // Initial Book with a Single Ask
    Order s1{1, 1, 99, 50, 50};
    Order s2{2, 2, 98, 50, 50};
    
    OrderEvent event_1 = MakeOrderEvent(s1, eOrderEventType::NEW, eOrderSide::BID, eOrderType::LIMIT);
    OrderEvent event_2 = MakeOrderEvent(s2, eOrderEventType::NEW, eOrderSide::BID, eOrderType::LIMIT);
    
    engine.ProcessEvent(event_1);
    engine.ProcessEvent(event_2);

    // Market Order
    Order m{3, 3, 0, 100, 100};
    OrderEvent event_3 = MakeOrderEvent(m, eOrderEventType::NEW, eOrderSide::BID, eOrderType::MARKET);
    
    engine.ProcessEvent(event_3);

    ASSERT_EQ(sink.trade_event_list.size(), 0);
    
    EXPECT_EQ(engine.GetOrderBook()->GetBestBid()->price, 99);
}

// Multiple Market Orders
TEST(MatchingEngineMarketOrdersTest, MultipleMarketOrders) {
    TestTradeEventSink sink;
    MatchingEngine<TestTradeEventSink> engine(sink);

    Order s1{1, 1, 100, 100, 100};
    
    OrderEvent event_1 = MakeOrderEvent(s1, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::LIMIT);
    
    engine.ProcessEvent(event_1);
    
    // Market Order
    Order m1{2, 2, 0, 30, 30};
    Order m2{3, 3, 0, 50, 50};
    
    OrderEvent event_2 = MakeOrderEvent(m1, eOrderEventType::NEW, eOrderSide::BID, eOrderType::MARKET);
    OrderEvent event_3 = MakeOrderEvent(m2, eOrderEventType::NEW, eOrderSide::BID, eOrderType::MARKET);
    
    engine.ProcessEvent(event_2);
    engine.ProcessEvent(event_3);

    ASSERT_EQ(sink.trade_event_list.size(), 2);

    
    EXPECT_EQ(sink.trade_event_list[0].executed_price, 100);
    EXPECT_EQ(sink.trade_event_list[0].filled_quantity, 30);

    EXPECT_EQ(sink.trade_event_list[1].executed_price, 100);
    EXPECT_EQ(sink.trade_event_list[1].filled_quantity, 50);
    
    EXPECT_EQ(engine.GetOrderBook()->GetBestAsk()->price, 100);
    EXPECT_EQ(engine.GetOrderBook()->GetBestAsk()->remaining_quantity, 20);
}

// Infinite Loop Test (Stuck Pointer)
TEST(MatchingEngineMarketOrdersTest, NoInfiniteLoop) {
    TestTradeEventSink sink;
    MatchingEngine<TestTradeEventSink> engine(sink);

    Order s1{1, 1, 100, 10, 10};
    
    OrderEvent event_1 = MakeOrderEvent(s1, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::LIMIT);
    
    engine.ProcessEvent(event_1);
    
    // Market Order
    Order m1{2, 2, 0, 10, 10};
    
    OrderEvent event_2 = MakeOrderEvent(m1, eOrderEventType::NEW, eOrderSide::BID, eOrderType::MARKET);

    engine.ProcessEvent(event_2);

    ASSERT_EQ(sink.trade_event_list.size(), 1);

    EXPECT_EQ(sink.trade_event_list[0].executed_price, 100);
    EXPECT_EQ(sink.trade_event_list[0].filled_quantity, 10);

    EXPECT_EQ(engine.GetOrderBook()->GetBestAsk(), nullptr);
    EXPECT_EQ(engine.GetOrderBook()->GetBestBid(), nullptr);
}