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

TEST(OrderNodePoolTest, SingleAcquire) {
    
    MemoryAllocator memory_allocator(1 << 12);
    OrderNodePool pool(memory_allocator, 8);

    OrderNode* node = pool.Acquire();

    EXPECT_NE(node, nullptr);
}

TEST(OrderNodePoolTest, PoolExhaustion) {
    
    MemoryAllocator memory_allocator(1 << 12);
    OrderNodePool pool(memory_allocator, 2);

    pool.Acquire();
    pool.Acquire();

    EXPECT_DEATH(pool.Acquire(), ".*");
}

TEST(OrderNodePoolTest, ReleaseReuse) {
    
    MemoryAllocator memory_allocator(1 << 12);
    OrderNodePool pool(memory_allocator, 2);

    OrderNode* n1 = pool.Acquire();
    OrderNode* n2 = pool.Acquire();

    pool.Release(n1);

    OrderNode* n3 = pool.Acquire();

    EXPECT_EQ(n3, n1);
}

TEST(OrderNodePoolTest, FreeListBehavior) {
    
    MemoryAllocator memory_allocator(1 << 12);
    OrderNodePool pool(memory_allocator, 3);

    OrderNode* n1 = pool.Acquire();
    OrderNode* n2 = pool.Acquire();
    OrderNode* n3 = pool.Acquire();

    pool.Release(n1);
    pool.Release(n2);

    OrderNode* n4 = pool.Acquire();
    OrderNode* n5 = pool.Acquire();

    EXPECT_EQ(n4, n2);
    EXPECT_EQ(n5, n1);
}

TEST(OrderNodePoolTest, StressTest) {
    MemoryAllocator memory_allocator(1 << 14);

    const size_t capacity = 256;
    OrderNodePool pool(memory_allocator, capacity);

    for (int cycle = 0; cycle < 100; ++cycle) {
        std::vector<OrderNode*> nodes;

        for (size_t i = 0; i < capacity; ++i)
            nodes.push_back(pool.Acquire());

        for (OrderNode* n : nodes)
            pool.Release(n);
    }
}

// 
// PriceLevelBitset Tests
// 


TEST(PriceLevelBitsetTest, EmptyBitset) {

    MemoryAllocator allocator(1 << 14);
    PriceLevelBitset bitset(allocator, 512);

    EXPECT_EQ(bitset.GetBestBidPriceLevel(), SIZE_MAX);
    EXPECT_EQ(bitset.GetBestAskPriceLevel(), SIZE_MAX);
}

TEST(PriceLevelBitsetTest, SinglePriceLevelActivate) {

    MemoryAllocator allocator(1 << 14);
    PriceLevelBitset bitset(allocator, 512);

    bitset.ActivatePriceLevel(122);

    EXPECT_EQ(bitset.GetBestBidPriceLevel(), 122);
    EXPECT_EQ(bitset.GetBestAskPriceLevel(), 122);
}

TEST(PriceLevelBitsetTest, MultiplePriceLevelActivate) {

    MemoryAllocator allocator(1 << 14);
    PriceLevelBitset bitset(allocator, 512);

    bitset.ActivatePriceLevel(10);
    bitset.ActivatePriceLevel(200);
    bitset.ActivatePriceLevel(511);

    EXPECT_EQ(bitset.GetBestAskPriceLevel(), 10);
    EXPECT_EQ(bitset.GetBestBidPriceLevel(), 511);
}

TEST(PriceLevelBitsetTest, ActivateAndDeactivatePriceLevels) {

    MemoryAllocator allocator(1 << 14);
    PriceLevelBitset bitset(allocator, 512);

    bitset.ActivatePriceLevel(50);
    bitset.ActivatePriceLevel(100);

    bitset.DeactivatePriceLevel(100);

    EXPECT_EQ(bitset.GetBestBidPriceLevel(), 50);
    EXPECT_EQ(bitset.GetBestAskPriceLevel(), 50);

    bitset.DeactivatePriceLevel(50);

    EXPECT_EQ(bitset.GetBestBidPriceLevel(), SIZE_MAX);
    EXPECT_EQ(bitset.GetBestAskPriceLevel(), SIZE_MAX);
}

TEST(PriceLevelBitsetTest, BoundsCheck) {

    MemoryAllocator allocator(1 << 14);
    PriceLevelBitset bitset(allocator, 512);

    bitset.ActivatePriceLevel(0);
    bitset.ActivatePriceLevel(511);

    EXPECT_EQ(bitset.GetBestAskPriceLevel(), 0);
    EXPECT_EQ(bitset.GetBestBidPriceLevel(), 511);
}

TEST(PriceLevelBitsetTest, StressTest) {

    MemoryAllocator allocator(1 << 14);
    PriceLevelBitset bitset(allocator, 512);

    std::set<size_t> price_level_set;

    for (size_t i = 0; i < 100; ++i) {
        size_t level = rand() % 512;
        bitset.ActivatePriceLevel(level);
        price_level_set.insert(level);
    }

    EXPECT_EQ(bitset.GetBestAskPriceLevel(), *price_level_set.begin());
    EXPECT_EQ(bitset.GetBestBidPriceLevel(), *price_level_set.rbegin());
}