#include "../include/history_clock.h"
#include <gtest/gtest.h>
#include <limits>

TEST(HistoryClock, SamplesAtFiftyHz) {
    HistoryClock clock;
    int samples = 0;
    for (int i = 0; i < 10000; ++i) samples += clock.advance(0.0001) ? 1 : 0;
    EXPECT_EQ(samples, 50);
    EXPECT_NEAR(clock.time(), 1.0, 1e-9);
}
TEST(HistoryClock, FrequencyChangePreservesTimeAndSampling) {
    HistoryClock clock;
    int samples = 0;
    for (int i = 0; i < 10000; ++i) samples += clock.advance(0.0001) ? 1 : 0;
    for (int i = 0; i < 5000; ++i) samples += clock.advance(0.0002) ? 1 : 0;
    EXPECT_EQ(samples, 100);
    EXPECT_NEAR(clock.time(), 2.0, 1e-9);
    clock.reset();
    EXPECT_EQ(clock.time(), 0);
    EXPECT_FALSE(clock.advance(0.001));
}
TEST(HistoryClock, InvalidStepsDoNotAdvance) {
    HistoryClock clock;
    EXPECT_FALSE(clock.advance(0));
    EXPECT_FALSE(clock.advance(-1));
    EXPECT_FALSE(clock.advance(std::numeric_limits<double>::quiet_NaN()));
    EXPECT_EQ(clock.time(), 0);
}
