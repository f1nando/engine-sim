#ifndef ATG_ENGINE_SIM_HISTORY_LOOKUP_H
#define ATG_ENGINE_SIM_HISTORY_LOOKUP_H

#include <cmath>

// Read points in chronological order, including wrapped ring buffers.
// Never extrapolate, interpolate across missing intervals, or select a stale
// neighbouring channel value when an exact timestamp was requested.
template <typename Point, typename Reader>
bool findNearestHistoryPoint(int count, Reader read, double time, double tolerance, Point &point) {
    if (count <= 0 || !std::isfinite(time) || !std::isfinite(tolerance) || tolerance < 0) return false;
    Point first, last;
    if (!read(0, first) || !read(count - 1, last)
        || time < first.x || time > last.x) return false;
    int low = 0, high = count;
    while (low < high) {
        const int middle = low + (high - low) / 2;
        Point candidate;
        if (!read(middle, candidate)) return false;
        if (candidate.x < time) low = middle + 1;
        else high = middle;
    }
    Point best;
    if (!read(low < count ? low : count - 1, best)) return false;
    if (low > 0) {
        Point previous;
        if (!read(low - 1, previous)) return false;
        if (std::abs(previous.x - time) <= std::abs(best.x - time)) best = previous;
    }
    if (!std::isfinite(best.x) || !std::isfinite(best.y) || std::abs(best.x - time) > tolerance) return false;
    point = best;
    return true;
}

#endif
