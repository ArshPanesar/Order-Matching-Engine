#include "order_core.h"
#include <gtest/gtest.h>

/*
 *  Testing for Default Initialization of Order object
 */
TEST(OrderCoreTest, OrderDefaultInitialization) {
    // Value-Initialized Order
    Order new_order{};

    // Check Default Values (should all be unsigned integers, therefore unsigned int literals are used to check for equivalency)
    EXPECT_EQ(new_order.id, 0u);
    EXPECT_EQ(new_order.timestamp, 0u);
    
    EXPECT_EQ(new_order.price, 0u);
    EXPECT_EQ(new_order.initial_quantity, 0u);
    EXPECT_EQ(new_order.remaining_quantity, 0u);
}

/*
 *  Check if Explicit Values are retained in the Order object
 */
TEST(OrderCoreTest, OrderValuesRetained) {
    // Value-Initialized Order
    Order new_order{};

    // Set Explicit Values
    new_order.id = 12345u;
    new_order.timestamp = 9999u;

    new_order.price = 500u;
    new_order.initial_quantity = 10000u;
    new_order.remaining_quantity = 10000u;

    // Check Explicit Values
    EXPECT_EQ(new_order.id, 12345u);
    EXPECT_EQ(new_order.timestamp, 9999u);
    
    EXPECT_EQ(new_order.price, 500u);
    EXPECT_EQ(new_order.initial_quantity, 10000u);
    EXPECT_EQ(new_order.remaining_quantity, 10000u);
}