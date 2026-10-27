#include "../include/history_continuity.h"
#include <gtest/gtest.h>

TEST(HistoryContinuity, ContinuousMeasurementsConnect) {
    HistoryContinuity history;
    EXPECT_FALSE(history.beginPoint());
    EXPECT_FALSE(history.beginPoint());
}

TEST(HistoryContinuity, MissingMeasurementsBreakOnlyTheNextSegment) {
    HistoryContinuity history;
    history.beginPoint();
    for (int i = 0; i < 100; ++i) history.markGap();
    EXPECT_TRUE(history.beginPoint());
    EXPECT_FALSE(history.beginPoint());
    history.markGap();
    EXPECT_TRUE(history.beginPoint());
}

TEST(HistoryContinuity, ResetDiscardsPendingGap) {
    HistoryContinuity history;
    history.markGap();
    history.reset();
    EXPECT_FALSE(history.beginPoint());
}
