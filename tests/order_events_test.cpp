#include <gtest/gtest.h>

#include "matching_engine_test.h"

//
// CANCEL EVENTS
//

TEST(OrderEventsTest, CancelRestingOrder) {
    TestOrderEventSink order_sink;
    TestTradeEventSink trade_sink;
    MemoryAllocator allocator(1 << 17);
    MatchingEngine<TestOrderEventSink, TestTradeEventSink> engine(order_sink, trade_sink, allocator, 0, 500, (1 << 8));

    Order o1{1, 1, 100, 10, 10};
    
    OrderEvent e1 = MakeOrderEvent(o1, eOrderEventType::NEW, eOrderSide::BID, eOrderType::LIMIT);
    ProcessSingleEvent(e1, order_sink, engine);
    
    OrderEvent e2 = MakeOrderEvent(o1, eOrderEventType::CANCEL, eOrderSide::BID, eOrderType::LIMIT);
    ProcessSingleEvent(e2, order_sink, engine);

    // Incoming sell should NOT match
    Order o2{2, 2, 100, 5, 5};
    OrderEvent e3 = MakeOrderEvent(o2, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::LIMIT);
    ProcessSingleEvent(e3, order_sink, engine);

    EXPECT_TRUE(trade_sink.trade_event_list.empty());
}

 
// 
// AMEND EVENTS
// 

TEST(OrderEventsTest, AmendOrder) {
    TestOrderEventSink order_sink;
    TestTradeEventSink trade_sink;
    MemoryAllocator allocator(1 << 17);
    MatchingEngine<TestOrderEventSink, TestTradeEventSink> engine(order_sink, trade_sink, allocator, 0, 500, (1 << 8));

    Order b1{1, 1, 100, 10, 10};
    OrderEvent e1 = MakeOrderEvent(b1, eOrderEventType::NEW, eOrderSide::BID, eOrderType::LIMIT);
    ProcessSingleEvent(e1, order_sink, engine);

    Order b1_amended = b1;
    b1_amended.price = 101;

    OrderEvent e2 = MakeOrderEvent(b1_amended, eOrderEventType::AMEND, eOrderSide::BID, eOrderType::LIMIT);
    ProcessSingleEvent(e2, order_sink, engine);

    Order s1{2, 2, 101, 5, 5};
    OrderEvent e3 = MakeOrderEvent(s1, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::LIMIT);
    ProcessSingleEvent(e3, order_sink, engine);

    ASSERT_EQ(trade_sink.trade_event_list.size(), 1);
    EXPECT_EQ(trade_sink.trade_event_list[0].executed_price, 101);
}

TEST(OrderEventsTest, AmendNonExistentOrderIgnored) {
    TestOrderEventSink order_sink;
    TestTradeEventSink trade_sink;
    MemoryAllocator allocator(1 << 17);
    MatchingEngine<TestOrderEventSink, TestTradeEventSink> engine(order_sink, trade_sink, allocator, 0, 500, (1 << 8));

    Order fake{99, 1, 100, 10, 10};
    OrderEvent e1 = MakeOrderEvent(fake, eOrderEventType::AMEND, eOrderSide::BID, eOrderType::LIMIT);
    ProcessSingleEvent(e1, order_sink, engine);

    EXPECT_TRUE(trade_sink.trade_event_list.empty());
}
