#include <gtest/gtest.h>

#include "matching_engine_test.h"

//
// CANCEL EVENTS
//

TEST(OrderEventsTest, CancelRestingOrder) {
    TestTradeEventSink sink;
    MatchingEngine<TestTradeEventSink> engine(sink);

    Order o1{1, 1, 100, 10, 10};
    
    OrderEvent e1 = MakeOrderEvent(o1, eOrderEventType::NEW, eOrderSide::BID, eOrderType::LIMIT);
    engine.ProcessEvent(e1);
    
    OrderEvent e2 = MakeOrderEvent(o1, eOrderEventType::CANCEL, eOrderSide::BID, eOrderType::LIMIT);
    engine.ProcessEvent(e2);

    // Incoming sell should NOT match
    Order o2{2, 2, 100, 5, 5};
    OrderEvent e3 = MakeOrderEvent(o2, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::LIMIT);
    engine.ProcessEvent(e3);

    EXPECT_TRUE(sink.trade_event_list.empty());
}

 
// 
// AMEND EVENTS
// 

TEST(OrderEventsTest, AmendUnfilledOrderAllowed) {
    TestTradeEventSink sink;
    MatchingEngine<TestTradeEventSink> engine(sink);

    Order b1{1, 1, 100, 10, 10};
    OrderEvent e1 = MakeOrderEvent(b1, eOrderEventType::NEW, eOrderSide::BID, eOrderType::LIMIT);
    engine.ProcessEvent(e1);

    Order b1_amended = b1;
    b1_amended.price = 101;

    OrderEvent e2 = MakeOrderEvent(b1_amended, eOrderEventType::AMEND, eOrderSide::BID, eOrderType::LIMIT);
    engine.ProcessEvent(e2);

    Order s1{2, 2, 101, 5, 5};
    OrderEvent e3 = MakeOrderEvent(s1, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::LIMIT);
    engine.ProcessEvent(e3);

    ASSERT_EQ(sink.trade_event_list.size(), 1);
    EXPECT_EQ(sink.trade_event_list[0].executed_price, 101);
}


TEST(OrderEventsTest, AmendPartiallyFilledOrderRejected) {
    TestTradeEventSink sink;
    MatchingEngine<TestTradeEventSink> engine(sink);

    // Resting Bid
    Order b1{1, 1, 100, 10, 10};
    OrderEvent e1 = MakeOrderEvent(b1, eOrderEventType::NEW, eOrderSide::BID, eOrderType::LIMIT);
    engine.ProcessEvent(e1);

    // Partial Fill
    Order s1{2, 2, 100, 4, 4};
    OrderEvent e2 = MakeOrderEvent(s1, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::LIMIT);
    engine.ProcessEvent(e2);

    ASSERT_EQ(sink.trade_event_list.size(), 1);

    // Attempt amend
    Order b1_amended = b1;
    b1_amended.price = 101;
    b1_amended.remaining_quantity = 6;


    OrderEvent e3 = MakeOrderEvent(b1_amended, eOrderEventType::AMEND, eOrderSide::BID, eOrderType::LIMIT);
    engine.ProcessEvent(e3);

    // Should NOT match at 101
    Order s2{3, 3, 101, 5, 5};

    OrderEvent e4 = MakeOrderEvent(s2, eOrderEventType::NEW, eOrderSide::ASK, eOrderType::LIMIT);
    engine.ProcessEvent(e4);

    // No new trades
    EXPECT_EQ(sink.trade_event_list.size(), 1);
}

TEST(OrderEventsTest, AmendNonExistentOrderIgnored) {
    TestTradeEventSink sink;
    MatchingEngine<TestTradeEventSink> engine(sink);

    Order fake{99, 1, 100, 10, 10};
    OrderEvent e1 = MakeOrderEvent(fake, eOrderEventType::AMEND, eOrderSide::BID, eOrderType::LIMIT);
    engine.ProcessEvent(e1);

    EXPECT_TRUE(sink.trade_event_list.empty());
}
