#include "../include/history_render_indices.h"
#include <gtest/gtest.h>

TEST(HistoryRenderIndices, RetainsEndpointsOrderAndBothExtremesWithinBudget) {
    const auto indices = historyRenderIndices(1501, 600, [](int i) {
        return i == 501 ? 10000 : i == 502 ? -10000 : 0;
    });
    ASSERT_LE(indices.size(), 600u);
    EXPECT_EQ(indices.front(), 0);
    EXPECT_EQ(indices.back(), 1500);
    bool high = false, low = false;
    for (size_t i = 0; i < indices.size(); ++i) {
        high = high || indices[i] == 501;
        low = low || indices[i] == 502;
        if (i > 0) EXPECT_LT(indices[i - 1], indices[i]);
    }
    EXPECT_TRUE(high);
    EXPECT_TRUE(low);
}
TEST(HistoryRenderIndices, LeavesOriginalScopesUnchangedWithoutLimit) {
    const auto indices = historyRenderIndices(100, 0, [](int i) { return i; });
    EXPECT_EQ(indices.size(), 100u);
    EXPECT_EQ(indices.front(), 0);
    EXPECT_EQ(indices.back(), 99);
}
