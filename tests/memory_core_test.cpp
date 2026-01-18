#include <gtest/gtest.h>
#include "memory/memory_core.h"

TEST(MemoryTest, InitialAllocCacheAligned) {
    // Already Cache-Aligned Case
    MemoryResource mem_resource_1(CACHE_LINE_SIZE);

    EXPECT_EQ(mem_resource_1.GetTotalAmt(), CACHE_LINE_SIZE);
    EXPECT_EQ(mem_resource_1.GetFreeAmt(), CACHE_LINE_SIZE);
    EXPECT_EQ(mem_resource_1.GetAllocatedAmt(), 0u);
    
    // Non-Aligned Case
    MemoryResource mem_resource_2(CACHE_LINE_SIZE + 15u);

    EXPECT_EQ(mem_resource_2.GetTotalAmt() % CACHE_LINE_SIZE, 0u);
    EXPECT_EQ(mem_resource_2.GetFreeAmt() % CACHE_LINE_SIZE, 0u);
    EXPECT_EQ(mem_resource_2.GetAllocatedAmt(), 0u);
}

TEST(MemoryTest, InitialAllocStatsCorrect) {
    MemoryResource mem_resource(CACHE_LINE_SIZE * 4);

    EXPECT_EQ(mem_resource.GetTotalAmt(), CACHE_LINE_SIZE * 4);
    EXPECT_EQ(mem_resource.GetFreeAmt(), CACHE_LINE_SIZE * 4);
    EXPECT_EQ(mem_resource.GetAllocatedAmt(), 0u);
    
    // Allocate some bytes
    mem_resource.Allocate(16u, 0u);

    EXPECT_EQ(mem_resource.GetTotalAmt(), CACHE_LINE_SIZE * 4);
    EXPECT_EQ(mem_resource.GetFreeAmt(), mem_resource.GetTotalAmt() - 16);
    EXPECT_EQ(mem_resource.GetAllocatedAmt(), 16);
}

TEST(MemoryTest, DiffSizesAndAlignments) {
    size_t total_size = CACHE_LINE_SIZE * 1024;
    MemoryResource mem_resource(total_size);

    EXPECT_EQ(mem_resource.GetTotalAmt(), total_size);
    EXPECT_EQ(mem_resource.GetFreeAmt(), total_size);
    EXPECT_EQ(mem_resource.GetAllocatedAmt(), 0u);
    
    // Allocate some bytes
    unsigned char* c = (unsigned char*)mem_resource.Allocate(1u, 0u);
    short* s = (short*)mem_resource.Allocate(2u, 1u);
    int* i = (int*)mem_resource.Allocate(4u, 2u);
    double* d = (double*)mem_resource.Allocate(8u, 4u);
    
    *c = 15;
    *s = 255;
    *i = 1234;
    *d = 3.141567;

    EXPECT_EQ(mem_resource.GetTotalAmt(), total_size);
    EXPECT_EQ(*c, 15);
    EXPECT_EQ(*s, 255);
    EXPECT_EQ(*i, 1234);
    EXPECT_EQ(*d, 3.141567);
}

TEST(MemoryTest, StatsOnReset) {
    size_t total_size = CACHE_LINE_SIZE * 1024;
    MemoryResource mem_resource(total_size);

    EXPECT_EQ(mem_resource.GetTotalAmt(), total_size);
    EXPECT_EQ(mem_resource.GetFreeAmt(), total_size);
    EXPECT_EQ(mem_resource.GetAllocatedAmt(), 0u);
    
    // Allocate some bytes
    mem_resource.Allocate(1u, 0u);
    mem_resource.Allocate(2u, 1u);
    mem_resource.Allocate(4u, 2u);
    mem_resource.Allocate(8u, 4u);
    mem_resource.Allocate(16u, 8u);
    mem_resource.Allocate(32u, 16u);
    mem_resource.Allocate(64u, 32u);
    mem_resource.Allocate(128u, 64u);
    mem_resource.Allocate(256u, 128u);
    mem_resource.Allocate(512u, 256u);
    
    mem_resource.Reset();

    EXPECT_EQ(mem_resource.GetTotalAmt(), total_size);
    EXPECT_EQ(mem_resource.GetFreeAmt(), total_size);
    EXPECT_EQ(mem_resource.GetAllocatedAmt(), 0u);
}