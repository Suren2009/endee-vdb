#include <gtest/gtest.h>

#include "hnsw/hnswalg.h"

TEST(VectorCacheSizingTest, SmallIndexDoesNotForceMinimumCacheBits) {
    const size_t bits = hnswlib::VectorCache::calculateCacheBits(100'000, 50);

    EXPECT_EQ(bits, 16);
    EXPECT_EQ(size_t{1} << bits, 65'536);
}

TEST(VectorCacheSizingTest, MinimumCacheBitsApplyOnlyWhenIndexCanUseThem) {
    const size_t min_slots = size_t{1} << settings::VECTOR_CACHE_MIN_BITS;

    const size_t bits = hnswlib::VectorCache::calculateCacheBits(min_slots, 1);

    EXPECT_EQ(bits, settings::VECTOR_CACHE_MIN_BITS);
    EXPECT_EQ(size_t{1} << bits, min_slots);
}

TEST(VectorCacheSizingTest, ZeroCachePercentageDisablesCache) {
    EXPECT_EQ(hnswlib::VectorCache::calculateCacheBits(100'000, 0), 0);
}
