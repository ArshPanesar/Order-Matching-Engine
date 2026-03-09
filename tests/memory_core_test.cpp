#include <gtest/gtest.h>
#include "memory/memory_core.h"

// Alignment Computation Test
TEST(MemoryCoreTests, AlignmentCorrectness) {

    EXPECT_EQ(ComputeAlignedAddress(64, 4), 64);
    EXPECT_EQ(ComputeAlignedAddress(64, 8), 64);
    EXPECT_EQ(ComputeAlignedAddress(64, 16), 64);
    EXPECT_EQ(ComputeAlignedAddress(64, 32), 64);

    EXPECT_EQ(ComputeAlignedAddress(64, 128), 128);
    EXPECT_EQ(ComputeAlignedAddress(64, 256), 256);

    EXPECT_EQ(ComputeAlignedAddress(0, 1), 0);
    EXPECT_EQ(ComputeAlignedAddress(0, 4), 0);
    EXPECT_EQ(ComputeAlignedAddress(0, 8), 0);

    EXPECT_EQ(ComputeAlignedAddress(3, 8), 8);
    EXPECT_EQ(ComputeAlignedAddress(5, 16), 16);
    EXPECT_EQ(ComputeAlignedAddress(7, 32), 32);
    EXPECT_EQ(ComputeAlignedAddress(356, 8), 360);
    EXPECT_EQ(ComputeAlignedAddress(5213, 16), 5216);
    EXPECT_EQ(ComputeAlignedAddress(7754, 32), 7776);
}

TEST(MemoryCoreTests, SingleAllocation) {
    MemoryAllocator alloc(1024);

    void* p = alloc.Allocate(64, 8);

    ASSERT_NE(p, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(p) % 8, 0);
}

TEST(MemoryCoreTests, MultipleAllocations) {
    MemoryAllocator alloc(1024);

    void* p1 = alloc.Allocate(64, 8);
    void* p2 = alloc.Allocate(64, 8);
    void* p3 = alloc.Allocate(64, 8);

    ASSERT_NE(p1, p2);
    ASSERT_NE(p2, p3);
    ASSERT_NE(p1, p3);
}

TEST(MemoryCoreTests, MixedAlignments) {
    MemoryAllocator alloc(2048);

    void* p1 = alloc.Allocate(1, 1);
    void* p2 = alloc.Allocate(1, 2);
    void* p3 = alloc.Allocate(1, 4);
    void* p4 = alloc.Allocate(1, 8);
    void* p5 = alloc.Allocate(1, 64);

    EXPECT_EQ(reinterpret_cast<uintptr_t>(p1) % 1, 0);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(p2) % 2, 0);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(p3) % 4, 0);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(p4) % 8, 0);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(p5) % 64, 0);
}

TEST(MemoryCoreTests, FreeAndReuseSameSize) {
    MemoryAllocator alloc(1024);

    void* p1 = alloc.Allocate(128, 8);
    alloc.Free(p1);
    void* p2 = alloc.Allocate(128, 8);

    EXPECT_EQ(p1, p2);
}

TEST(MemoryCoreTests, FreeInReverseOrder) {
    MemoryAllocator alloc(1024);

    void* p1 = alloc.Allocate(64, 8);
    void* p2 = alloc.Allocate(64, 8);
    void* p3 = alloc.Allocate(64, 8);

    alloc.Free(p3);
    alloc.Free(p2);
    alloc.Free(p1);
}

TEST(MemoryCoreTests, FragmentationReuseMiddle) {
    MemoryAllocator alloc(1024);

    void* p1 = alloc.Allocate(128, 8);
    void* p2 = alloc.Allocate(128, 8);
    void* p3 = alloc.Allocate(128, 8);

    alloc.Free(p2);

    void* p4 = alloc.Allocate(64, 8);
    EXPECT_EQ(p4, p2);
}

TEST(MemoryCoreTests, FragmentationBlocksLargeAlloc) {
    MemoryAllocator alloc(512);

    void* a = alloc.Allocate(128, 8);
    void* b = alloc.Allocate(128, 8);
    void* c = alloc.Allocate(128, 8);

    alloc.Free(a);
    alloc.Free(c);

    ASSERT_DEATH(
        alloc.Allocate(256, 8),
        "MemoryAllocator ran out of memory!"
    );
}

TEST(MemoryCoreTests, FreeInvalidPointerDeath) {
    MemoryAllocator alloc(1024);
    int x;

    EXPECT_DEATH(
        alloc.Free(&x),
        "MemoryAllocator tried to free invalid pointer!"
    );
}

TEST(MemoryCoreTests, StatsCorrectness) {
    MemoryAllocator alloc(1024);

    void* p1 = alloc.Allocate(100, 8);
    void* p2 = alloc.Allocate(200, 16);

    EXPECT_EQ(alloc.GetAllocatedBytes(), 300);

    alloc.Free(p1);
    EXPECT_EQ(alloc.GetAllocatedBytes(), 200);

    alloc.Free(p2);
    EXPECT_EQ(alloc.GetAllocatedBytes(), 0);
}

TEST(MemoryCoreTests, StressTest) {
    MemoryAllocator alloc(64 * 2048);
    std::vector<void*> ptrs;

    for (int i = 0; i < 1000; ++i) {
        if (!ptrs.empty() && rand() % 2) {
            // Pick a Random Index
            int idx = rand() % ptrs.size();
            // Free the Pointer and remove from List
            alloc.Free(ptrs[idx]);
            ptrs.erase(ptrs.begin() + idx);
        } else {
            size_t size = (rand() % 128) + 1; // Random Size in Range [1, 128]
            size_t align = 1 << (rand() % 6); // Random Power of 2
            ptrs.push_back(alloc.Allocate(size, align));
        }
    }
}
