#include <gtest/gtest.h>

#include "matching_engine_test.h"

//
// CANCEL EVENTS
//

TEST(OrderEventsTest, CancelRestingOrder) {
    TestOrderEventSink order_sink;
    TestTradeEventSink trade_sink;
    MatchingEngine<TestOrderEventSink, TestTradeEventSink> engine(order_sink, trade_sink);

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

TEST(OrderEventsTest, AmendUnfilledOrderAllowed) {
    TestOrderEventSink order_sink;
    TestTradeEventSink trade_sink;
    MatchingEngine<TestOrderEventSink, TestTradeEventSink> engine(order_sink, trade_sink);

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


TEST(OrderEventsTest, AmendPartiallyFilledOrderRejected) {
    TestOrderEventSink order_sink;
    TestTradeEventSink trade_sink;
    MatchingEngine<TestOrderEventSink, TestTradeEventSink> engine(order_sink, trade_sink);

    // Resting Bid
    Order b1{1, 1, 100, 10, 10};
    OrderEvent e1 = MakeOrderEvent(b1, eOrderEventType::NEW, eOrderSide::BID, eOrderType::LIMIT);
    ProcessSingleEvent(e1, order_sink, engine);

    // Partial Fill
    Order s1{2, 2, 100, 4, 4};
    OrderEvent e2 = MakeOrderEvent(s1, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::LIMIT);
    ProcessSingleEvent(e2, order_sink, engine);

    ASSERT_EQ(trade_sink.trade_event_list.size(), 1);

    // Attempt amend
    Order b1_amended = b1;
    b1_amended.price = 101;
    b1_amended.remaining_quantity = 6;


    OrderEvent e3 = MakeOrderEvent(b1_amended, eOrderEventType::AMEND, eOrderSide::BID, eOrderType::LIMIT);
    ProcessSingleEvent(e3, order_sink, engine);

    // Should NOT match at 101
    Order s2{3, 3, 101, 5, 5};

    OrderEvent e4 = MakeOrderEvent(s2, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::LIMIT);
    ProcessSingleEvent(e4, order_sink, engine);

    // No new trades
    EXPECT_EQ(trade_sink.trade_event_list.size(), 1);
}

TEST(OrderEventsTest, AmendNonExistentOrderIgnored) {
    TestOrderEventSink order_sink;
    TestTradeEventSink trade_sink;
    MatchingEngine<TestOrderEventSink, TestTradeEventSink> engine(order_sink, trade_sink);

    Order fake{99, 1, 100, 10, 10};
    OrderEvent e1 = MakeOrderEvent(fake, eOrderEventType::AMEND, eOrderSide::BID, eOrderType::LIMIT);
    ProcessSingleEvent(e1, order_sink, engine);

    EXPECT_TRUE(trade_sink.trade_event_list.empty());
}
