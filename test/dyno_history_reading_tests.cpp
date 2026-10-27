#include "../include/dyno_history_reading.h"
#include <gtest/gtest.h>
#include <limits>

TEST(DynoHistoryReading, ConvertsTorqueAndPowerToExplicitSiUnits) {
    const auto reading = DynoHistoryReading::measure(true, 200 * units::Nm, units::rpm(3000));
    EXPECT_TRUE(reading.available);
    EXPECT_DOUBLE_EQ(reading.torqueNm, 200);
    // Upstream constants::pi is rounded to eight decimal places.
    EXPECT_NEAR(reading.powerKw, 62.8318530718, 1e-6);
}

TEST(DynoHistoryReading, DisabledDynoHasNoMeasurementEvenWithStaleTorque) {
    EXPECT_FALSE(DynoHistoryReading::measure(false, 500 * units::Nm, units::rpm(6000)).available);
}

TEST(DynoHistoryReading, ZeroAndNegativeMeasurementsAreValid) {
    const auto stopped = DynoHistoryReading::measure(true, 0, 0);
    EXPECT_TRUE(stopped.available);
    EXPECT_DOUBLE_EQ(stopped.powerKw, 0);
    const auto braking = DynoHistoryReading::measure(true, -100 * units::Nm, units::rpm(3000));
    EXPECT_TRUE(braking.available);
    EXPECT_LT(braking.torqueNm, 0);
    EXPECT_NEAR(braking.powerKw, -31.4159265359, 1e-6);
}

TEST(DynoHistoryReading, RejectsNonfiniteMeasurementsAndOverflow) {
    EXPECT_FALSE(DynoHistoryReading::measure(true,
        std::numeric_limits<double>::quiet_NaN(), 100).available);
    EXPECT_FALSE(DynoHistoryReading::measure(true, 100,
        std::numeric_limits<double>::infinity()).available);
    EXPECT_FALSE(DynoHistoryReading::measure(true,
        std::numeric_limits<double>::max(), 1000).available);
}
