#include "../include/history_lookup.h"
#include <gtest/gtest.h>
#include <limits>
#include <vector>

namespace {
struct Point { double x, y; };
bool lookup(const std::vector<Point> &points, double time, double tolerance, Point &point) {
    return findNearestHistoryPoint(static_cast<int>(points.size()),
        [&](int i, Point &p) { p = points[i]; return true; }, time, tolerance, point);
}
}

TEST(HistoryLookup, EmptyAndInvalidRequestsAreUnavailable) {
    Point point;
    EXPECT_FALSE(lookup({}, 1, 0.02, point));
    EXPECT_FALSE(lookup({{1, 10}}, std::numeric_limits<double>::quiet_NaN(), 0.02, point));
    EXPECT_FALSE(lookup({{1, 10}}, 1, -1, point));
}
TEST(HistoryLookup, FindsNearestRecordedSampleWithoutInterpolation) {
    Point point;
    ASSERT_TRUE(lookup({{1, 10}, {1.02, 20}, {1.04, 30}}, 1.015, 0.02, point));
    EXPECT_DOUBLE_EQ(point.x, 1.02);
    EXPECT_DOUBLE_EQ(point.y, 20);
}
TEST(HistoryLookup, NeverExtrapolatesIntoFutureOrExpiredHistory) {
    Point point;
    EXPECT_FALSE(lookup({{1, 10}, {1.02, 20}}, 0.999, 0.02, point));
    EXPECT_FALSE(lookup({{1, 10}, {1.02, 20}}, 1.021, 0.02, point));
    EXPECT_TRUE(lookup({{1, 10}}, 1, 0, point));
}
TEST(HistoryLookup, ExactChannelLookupRejectsValuesAcrossMissingIntervals) {
    Point point;
    const std::vector<Point> points = {{1, 10}, {1.02, 20}, {2, 40}, {2.02, 50}};
    EXPECT_FALSE(lookup(points, 1.5, 1e-9, point));
    EXPECT_FALSE(lookup(points, 1.04, 1e-9, point));
    EXPECT_TRUE(lookup(points, 2, 1e-9, point));
    EXPECT_DOUBLE_EQ(point.y, 40);
}
TEST(HistoryLookup, RespectsChronologicalReaderForWrappedStorage) {
    const Point ring[] = {{3, 30}, {4, 40}, {1, 10}, {2, 20}};
    Point point;
    ASSERT_TRUE(findNearestHistoryPoint(4, [&](int i, Point &p) {
        p = ring[(i + 2) % 4]; return true;
    }, 2.2, 0.3, point));
    EXPECT_DOUBLE_EQ(point.x, 2);
}
