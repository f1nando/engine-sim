#include "../include/frequency_calibration.h"
#include <gtest/gtest.h>
#include <limits>

static void measure(FrequencyCalibration &c, double cost) {
    for (int i = 0; i < 250; ++i) c.sample(0.02, cost * 200, 200);
}

TEST(FrequencyCalibration, FastCpuKeepsConservativeLimit) {
    FrequencyCalibration c;
    c.begin(20000);
    measure(c, 0.00001);
    EXPECT_TRUE(c.ready());
    EXPECT_EQ(c.recommendation(), 10000);
}
TEST(FrequencyCalibration, SlowCpuLowersFrequency) {
    FrequencyCalibration c;
    c.begin(20000);
    measure(c, 0.00008);
    EXPECT_EQ(c.recommendation(), 5000);
    EXPECT_FALSE(c.hasHeadroom(c.recommendation()));
}
TEST(FrequencyCalibration, ReservesBudgetForHighRpm) {
    FrequencyCalibration c;
    c.begin(20000);
    // Away from a floating-point rounding boundary: ~7692 Hz budget -> 7500.
    measure(c, 0.000039);
    EXPECT_EQ(c.recommendation(), 7500);
    EXPECT_TRUE(c.hasHeadroom(c.recommendation()));
    EXPECT_FALSE(c.hasHeadroom(10000));
}
TEST(FrequencyCalibration, RoundingBoundaryStaysConservative) {
    FrequencyCalibration c;
    c.begin(20000);
    measure(c, 0.00004);
    // Dividing measured durations may yield just below the mathematical 7500.
    // Rounding down to 7000 is valid: never raise the budget to satisfy the test.
    EXPECT_GE(c.recommendation(), 7000);
    EXPECT_LE(c.recommendation(), 7500);
    EXPECT_EQ(c.recommendation() % 500, 0);
    EXPECT_TRUE(c.hasHeadroom(c.recommendation()));
}
TEST(FrequencyCalibration, KeepsLowModelDefault) {
    FrequencyCalibration c;
    c.begin(4000);
    measure(c, 0.0002);
    EXPECT_EQ(c.recommendation(), 4000);
}
TEST(FrequencyCalibration, WarmupAndInvalidSamplesDoNotFinish) {
    FrequencyCalibration c;
    c.begin(10000);
    for (int i = 0; i < 100; ++i) c.sample(0.02, std::numeric_limits<double>::quiet_NaN(), 200);
    EXPECT_FALSE(c.ready());
    c.cancel();
    measure(c, 0.00001);
    EXPECT_FALSE(c.ready());
}
TEST(FrequencyCalibration, ReloadDiscardsOldMeasurements) {
    FrequencyCalibration c;
    c.begin(10000);
    measure(c, 0.0001);
    c.begin(8000);
    EXPECT_FALSE(c.ready());
    EXPECT_EQ(c.recommendation(), 8000);
    measure(c, 0.00001);
    EXPECT_EQ(c.recommendation(), 8000);
}
