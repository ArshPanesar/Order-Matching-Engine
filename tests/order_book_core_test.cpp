#define LOB_DEBUG

#include "order_book_core.h"
#include <gtest/gtest.h>

//
// OrderTable Tests
//
void PrintOrderTableMetrics(OrderTable& table) {
    std::cout << "OrderTable Metrics:\n";
    std::cout << "Items: " << table.GetSize() << "\n";
    std::cout << "Load Factor: " << table.ComputeLoadFactor() << "\n";
    
    size_t max_dist;
    float avg_dist;
    
    table.ComputeAvgAndMaxDistances(avg_dist, max_dist);
    
    std::cout << "Cluster Maximum Distance: " << max_dist << "\n";
    std::cout << "Cluster Average Distance: " << avg_dist << "\n";
    std::cout << std::endl;
}

TEST(OrderTableTest, InsertAndFindSingle) {
    MemoryAllocator memory_allocator(1 << 17);
    OrderTable table(memory_allocator);

    OrderNode node{42, 42, nullptr, nullptr};
    table.Insert(1, &node);

    auto* found = table.Find(1);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->price, 42);
}

TEST(OrderTableTest, FindNonExistantID) {
    MemoryAllocator memory_allocator(1 << 17);
    OrderTable table(memory_allocator);

    OrderNode node{42, 42, nullptr, nullptr};
    table.Insert(10, &node);
    table.Insert(15, &node);
    table.Insert(20, &node);
    table.Insert(25, &node);

    EXPECT_EQ(table.Find(9), nullptr);
    EXPECT_EQ(table.Find(11), nullptr);
    EXPECT_EQ(table.Find(14), nullptr);
    EXPECT_EQ(table.Find(19), nullptr);
    EXPECT_EQ(table.Find(21), nullptr);
    EXPECT_EQ(table.Find(24), nullptr);
    EXPECT_EQ(table.Find(26), nullptr);

    EXPECT_EQ(table.Find(0), nullptr);
    EXPECT_EQ(table.Find(99), nullptr);
}

TEST(OrderTableTest, MultipleInserts) {
    MemoryAllocator memory_allocator(1 << 17);
    OrderTable table(memory_allocator);

    OrderNode node_1{10, 10, nullptr, nullptr};
    OrderNode node_2{20, 20, nullptr, nullptr};
    OrderNode node_3{30, 30, nullptr, nullptr};
    
    table.Insert(1, &node_1);
    table.Insert(2, &node_2);
    table.Insert(3, &node_3);

    EXPECT_EQ(table.Find(1)->price, 10);
    EXPECT_EQ(table.Find(2)->price, 20);
    EXPECT_EQ(table.Find(3)->price, 30);
}

TEST(OrderTableTest, HandlesCollisions) {
    MemoryAllocator memory_allocator(1 << 17);
    OrderTable table(memory_allocator, 4); // Smaller Size to allow Collisions

    OrderNode node_1{10, 10, nullptr, nullptr};
    OrderNode node_2{20, 20, nullptr, nullptr};
    OrderNode node_3{30, 30, nullptr, nullptr};
    
    table.Insert(1, &node_1);
    table.Insert(2, &node_2);
    table.Insert(3, &node_3);

    EXPECT_EQ(table.Find(1)->price, 10);
    EXPECT_EQ(table.Find(2)->price, 20);
    EXPECT_EQ(table.Find(3)->price, 30);
}

TEST(OrderTableTest, RemoveExistingElement) {
    MemoryAllocator memory_allocator(1 << 17);
    OrderTable table(memory_allocator, 4); // Smaller Size to allow Collisions

    OrderNode node_1{10, 10, nullptr, nullptr};
    OrderNode node_2{20, 20, nullptr, nullptr};
    OrderNode node_3{30, 30, nullptr, nullptr};
    
    table.Insert(1, &node_1);
    table.Insert(2, &node_2);
    table.Insert(3, &node_3);

    table.Remove(1);

    EXPECT_EQ(table.Find(1), nullptr);
    EXPECT_EQ(table.Find(2)->price, 20);
    EXPECT_EQ(table.Find(3)->price, 30);
}

TEST(OrderTableTest, RemoveMultipleElements) {
    MemoryAllocator memory_allocator(1 << 17);
    OrderTable table(memory_allocator, 4); // Smaller Size to allow Collisions

    OrderNode node_1{10, 10, nullptr, nullptr};
    OrderNode node_2{20, 20, nullptr, nullptr};
    OrderNode node_3{30, 30, nullptr, nullptr};
    
    table.Insert(1, &node_1);
    table.Insert(2, &node_2);
    table.Insert(3, &node_3);

    table.Remove(1);
    table.Remove(2);
    table.Remove(3);

    EXPECT_EQ(table.Find(1), nullptr);
    EXPECT_EQ(table.Find(2), nullptr);
    EXPECT_EQ(table.Find(3), nullptr);
}

TEST(OrderTableTest, LoadFactorCorrect) {
    MemoryAllocator memory_allocator(1 << 17);
    OrderTable table(memory_allocator, 8);

    OrderNode node_1{10, 10, nullptr, nullptr};
    OrderNode node_2{20, 20, nullptr, nullptr};
    OrderNode node_3{30, 30, nullptr, nullptr};
    OrderNode node_4{30, 30, nullptr, nullptr};

    table.Insert(1, &node_1);
    table.Insert(2, &node_2);

    EXPECT_DOUBLE_EQ(table.ComputeLoadFactor(), 0.25);
    
    table.Insert(3, &node_3);
    table.Insert(4, &node_4);

    EXPECT_DOUBLE_EQ(table.ComputeLoadFactor(), 0.5);
}

TEST(OrderTableTest, StressTest) {
    
    MemoryAllocator memory_allocator(1 << 17);
    OrderTable table(memory_allocator, 4096);

    size_t N = 2048u;

    std::vector<OrderNode> nodes(N);

    // Insert all Elements
    for (size_t i = 0; i < N; ++i) {
        nodes[i].price = i;
        table.Insert(i, &nodes[i]);
    }

    EXPECT_DOUBLE_EQ(table.ComputeLoadFactor(), 0.5);
    PrintOrderTableMetrics(table);

    // Verify that all Elements were Inserted Correctly
    for (size_t i = 0; i < N; ++i) {
        ASSERT_NE(table.Find(i), nullptr);
    }

    // Remove Half of the Items (Even Indexed)
    for (size_t i = 0; i < N; i += 2) {
        table.Remove(i);
    }

    EXPECT_DOUBLE_EQ(table.ComputeLoadFactor(), 0.25);

    // Verify Odd Elements still Exist
    for (size_t i = 0; i < N; ++i) {
        if (i % 2 == 0)
            EXPECT_EQ(table.Find(i), nullptr);
        else
            EXPECT_NE(table.Find(i), nullptr);
    }
}