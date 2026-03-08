#include "order_book.h"
#include <gtest/gtest.h>


// Helper Functions
OrderTimestamp current_time = 1;

Order CreateOrder(OrderID id, OrderPrice price, OrderQuantity quantity) {
    Order order{};

    order.id = id;
    order.timestamp = current_time;
    ++current_time;

    order.price = price;
    order.initial_quantity = quantity;
    order.remaining_quantity = quantity;
    
    return order;
}

// Adding the First Order
TEST(OrderBookTest, AddingFirstOrder) {

    MemoryAllocator allocator(1 << 20);
    OrderBook book(allocator, 0, 1000, (1 << 8));
    Order first_order = CreateOrder(1, 200, 300);

    book.AddOrder(first_order, eOrderSide::BID);

    const OrderNode* best_bid = book.GetBestBid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->id, 1u);
    EXPECT_EQ(best_bid->price, 200u);
    EXPECT_EQ(best_bid->current_quantity, 300u);
}

// Adding Multiple Orders at the Same Price
TEST(OrderBookTest, AddingMultipleOrdersSamePrice) {
    
    MemoryAllocator allocator(1 << 20);
    OrderBook book(allocator, 0, 1000, (1 << 8));
    Order order_1 = CreateOrder(1, 100, 10);
    Order order_2 = CreateOrder(2, 100, 20);
    Order order_3 = CreateOrder(3, 100, 30);
    
    book.AddOrder(order_1, eOrderSide::BID);
    book.AddOrder(order_2, eOrderSide::BID);
    book.AddOrder(order_3, eOrderSide::BID);
    

    const OrderNode* best_bid = book.GetBestBid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->id, 1u);
    EXPECT_EQ(best_bid->price, 100u);
    EXPECT_EQ(best_bid->current_quantity, 10u);
}

// Adding Multiple Orders at the Different Prices
TEST(OrderBookTest, AddingMultipleOrdersDiffPrice) {
    
    MemoryAllocator allocator(1 << 20);
    OrderBook book(allocator, 0, 1000, (1 << 8));
    Order order_1 = CreateOrder(1, 101, 10);
    Order order_2 = CreateOrder(2, 100, 20);
    Order order_3 = CreateOrder(3, 102, 30);
    Order order_4 = CreateOrder(4, 103, 40);
    
    
    book.AddOrder(order_1, eOrderSide::BID);
    book.AddOrder(order_2, eOrderSide::BID);
    book.AddOrder(order_3, eOrderSide::ASK);
    book.AddOrder(order_4, eOrderSide::ASK);
    
    const OrderNode* best_bid = book.GetBestBid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->id, 1u); // Highest Price in Bids

    const OrderNode* best_ask = book.GetBestAsk();
    ASSERT_NE(best_ask, nullptr);
    EXPECT_EQ(best_ask->id, 3u); // Lowest Price in Asks
}

// Removing an Order
TEST(OrderBookTest, RemovingOrder) {
    
    MemoryAllocator allocator(1 << 20);
    OrderBook book(allocator, 0, 1000, (1 << 8));
    Order order_1 = CreateOrder(1, 100, 10);
    Order order_2 = CreateOrder(2, 100, 20);
    
    
    book.AddOrder(order_1, eOrderSide::BID);
    book.AddOrder(order_2, eOrderSide::BID);
    
    book.RemoveOrder(order_1.id);

    const OrderNode* best_bid = book.GetBestBid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->id, 2u);
}

// Removing an Entire Price Level
TEST(OrderBookTest, RemovingPriceLevel) {
    
    MemoryAllocator allocator(1 << 20);
    OrderBook book(allocator, 0, 1000, (1 << 8));

    Order order_1 = CreateOrder(1, 100, 10);
    
    book.AddOrder(order_1, eOrderSide::BID);
    
    book.RemoveOrder(order_1.id);

    const OrderNode* best_bid = book.GetBestBid();
    EXPECT_EQ(best_bid, nullptr);
}

// Removing a Non-Existant Order
TEST(OrderBookTest, RemovingNonExistantOrder) {
    
    MemoryAllocator allocator(1 << 20);
    OrderBook book(allocator, 0, 1000, (1 << 8));
    Order order_1 = CreateOrder(1, 100, 10);
    Order order_2 = CreateOrder(2, 101, 10);
    
    
    book.AddOrder(order_1, eOrderSide::BID);
    
    book.RemoveOrder(order_2.id);

    const OrderNode* best_bid = book.GetBestBid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->id, 1u);
}

// Empty Book Operations
TEST(OrderBookTest, EmptyBookOperations) {
    
    MemoryAllocator allocator(1 << 20);
    OrderBook book(allocator, 0, 1000, (1 << 8));

    const OrderNode* best_bid = book.GetBestBid();
    EXPECT_EQ(best_bid, nullptr);

    const OrderNode* best_ask = book.GetBestAsk();
    EXPECT_EQ(best_ask, nullptr);
}

// Test for Sorting Correctness at Price Level
TEST(OrderBookTest, PriceLevelSortingCorrectness) {
    
    MemoryAllocator allocator(1 << 20);
    OrderBook book(allocator, 0, 1000, (1 << 8));

    // Bids
    Order order_1 = CreateOrder(1, 100, 1);
    Order order_2 = CreateOrder(2, 101, 20);
    Order order_3 = CreateOrder(3, 102, 300);
    
    // Asks
    Order order_4 = CreateOrder(4, 110, 1);
    Order order_5 = CreateOrder(5, 109, 20);
    Order order_6 = CreateOrder(6, 108, 300);

    book.AddOrder(order_1, eOrderSide::BID);
    book.AddOrder(order_2, eOrderSide::BID);
    book.AddOrder(order_3, eOrderSide::BID);
    
    book.AddOrder(order_4, eOrderSide::ASK);
    book.AddOrder(order_5, eOrderSide::ASK);
    book.AddOrder(order_6, eOrderSide::ASK);
    
    // Check Sorting of Best Bid
    const OrderNode* best_bid = book.GetBestBid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->id, 3u);

    book.RemoveOrder(order_3.id);
    best_bid = book.GetBestBid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->id, 2u);


    book.RemoveOrder(order_2.id);
    best_bid = book.GetBestBid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->id, 1u);

    book.RemoveOrder(order_1.id);
    best_bid = book.GetBestBid();
    EXPECT_EQ(best_bid, nullptr);

    // Check Sorting of Best Ask
    const OrderNode* best_ask = book.GetBestAsk();
    ASSERT_NE(best_ask, nullptr);
    EXPECT_EQ(best_ask->id, 6u);

    book.RemoveOrder(order_6.id);
    best_ask = book.GetBestAsk();
    ASSERT_NE(best_ask, nullptr);
    EXPECT_EQ(best_ask->id, 5u);


    book.RemoveOrder(order_5.id);
    best_ask = book.GetBestAsk();
    ASSERT_NE(best_ask, nullptr);
    EXPECT_EQ(best_ask->id, 4u);

    book.RemoveOrder(order_4.id);
    best_ask = book.GetBestAsk();
    EXPECT_EQ(best_ask, nullptr);
}

// Test for Sorting Correctness at FIFO Level
TEST(OrderBookTest, FIFOSortingCorrectness) {
    
    MemoryAllocator allocator(1 << 20);
    OrderBook book(allocator, 0, 1000, (1 << 8));

    // Bids
    Order order_1 = CreateOrder(1, 100, 1);
    Order order_2 = CreateOrder(2, 100, 20);
    Order order_3 = CreateOrder(3, 100, 300);
    
    // Asks
    Order order_4 = CreateOrder(4, 110, 1);
    Order order_5 = CreateOrder(5, 110, 20);
    Order order_6 = CreateOrder(6, 110, 300);

    book.AddOrder(order_1, eOrderSide::BID);
    book.AddOrder(order_2, eOrderSide::BID);
    book.AddOrder(order_3, eOrderSide::BID);
    
    book.AddOrder(order_4, eOrderSide::ASK);
    book.AddOrder(order_5, eOrderSide::ASK);
    book.AddOrder(order_6, eOrderSide::ASK);
    
    // Check Sorting of Best Bid
    const OrderNode* best_bid = book.GetBestBid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->id, 1u);

    book.RemoveOrder(order_1.id);
    best_bid = book.GetBestBid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->id, 2u);


    book.RemoveOrder(order_2.id);
    best_bid = book.GetBestBid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->id, 3u);

    book.RemoveOrder(order_3.id);
    best_bid = book.GetBestBid();
    EXPECT_EQ(best_bid, nullptr);

    // Check Sorting of Best Ask
    const OrderNode* best_ask = book.GetBestAsk();
    ASSERT_NE(best_ask, nullptr);
    EXPECT_EQ(best_ask->id, 4u);

    book.RemoveOrder(order_4.id);
    best_ask = book.GetBestAsk();
    ASSERT_NE(best_ask, nullptr);
    EXPECT_EQ(best_ask->id, 5u);


    book.RemoveOrder(order_5.id);
    best_ask = book.GetBestAsk();
    ASSERT_NE(best_ask, nullptr);
    EXPECT_EQ(best_ask->id, 6u);

    book.RemoveOrder(order_6.id);
    best_ask = book.GetBestAsk();
    EXPECT_EQ(best_ask, nullptr);
}

// Test for General Sorting Correctness
TEST(OrderBookTest, GeneralSortingCorrectness) {
    
    MemoryAllocator allocator(1 << 20);
    OrderBook book(allocator, 0, 1000, (1 << 8));

    // Bids
    Order order_1 = CreateOrder(1, 100, 1);
    Order order_2 = CreateOrder(2, 100, 20);
    Order order_3 = CreateOrder(3, 99, 300);
    
    // Asks
    Order order_4 = CreateOrder(4, 110, 1);
    Order order_5 = CreateOrder(5, 110, 20);
    Order order_6 = CreateOrder(6, 115, 300);

    book.AddOrder(order_1, eOrderSide::BID);
    book.AddOrder(order_2, eOrderSide::BID);
    book.AddOrder(order_3, eOrderSide::BID);
    
    book.AddOrder(order_4, eOrderSide::ASK);
    book.AddOrder(order_5, eOrderSide::ASK);
    book.AddOrder(order_6, eOrderSide::ASK);
    
    // Check Sorting of Best Bid
    const OrderNode* best_bid = book.GetBestBid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->id, 1u);

    book.RemoveOrder(order_1.id);
    best_bid = book.GetBestBid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->id, 2u);


    book.RemoveOrder(order_2.id);
    best_bid = book.GetBestBid();
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->id, 3u);

    book.RemoveOrder(order_3.id);
    best_bid = book.GetBestBid();
    EXPECT_EQ(best_bid, nullptr);

    // Check Sorting of Best Ask
    const OrderNode* best_ask = book.GetBestAsk();
    ASSERT_NE(best_ask, nullptr);
    EXPECT_EQ(best_ask->id, 4u);

    book.RemoveOrder(order_4.id);
    best_ask = book.GetBestAsk();
    ASSERT_NE(best_ask, nullptr);
    EXPECT_EQ(best_ask->id, 5u);

    book.RemoveOrder(order_5.id);
    best_ask = book.GetBestAsk();
    ASSERT_NE(best_ask, nullptr);
    EXPECT_EQ(best_ask->id, 6u);

    book.RemoveOrder(order_6.id);
    best_ask = book.GetBestAsk();
    EXPECT_EQ(best_ask, nullptr);
}

// Search for Existing Order by ID
TEST(OrderBookTest, SearchExistingOrder) {
    
    MemoryAllocator allocator(1 << 20);
    OrderBook book(allocator, 0, 1000, (1 << 8));

    Order order_1 = CreateOrder(1, 100, 10);
    Order order_2 = CreateOrder(2, 101, 10);
    
    book.AddOrder(order_1, eOrderSide::BID);
    
    book.RemoveOrder(order_2.id);

    const OrderNode* best_bid = book.GetOrderByID(1);
    ASSERT_NE(best_bid, nullptr);
    EXPECT_EQ(best_bid->id, 1u);
}

// Search for Non-Existant Order by ID
TEST(OrderBookTest, SearchNonExistantOrder) {
    
    MemoryAllocator allocator(1 << 20);
    OrderBook book(allocator, 0, 1000, (1 << 8));

    Order order_1 = CreateOrder(1, 100, 10);
    Order order_2 = CreateOrder(2, 101, 10);
    
    book.AddOrder(order_1, eOrderSide::BID);
    
    book.RemoveOrder(order_2.id);

    const OrderNode* best_bid = book.GetOrderByID(2);
    EXPECT_EQ(best_bid, nullptr);
}