#ifndef ATG_ENGINE_SIM_HISTORY_RENDER_INDICES_H
#define ATG_ENGINE_SIM_HISTORY_RENDER_INDICES_H

#include <vector>

// Bound GPU geometry while retaining each time bucket's extrema. Full-resolution
// samples remain in the ring buffer for cursor/measurement lookup.
template <typename Reader>
std::vector<int> historyRenderIndices(int count, int limit, Reader value) {
    std::vector<int> indices;
    if (limit < 4 || count <= limit) {
        for (int i = 0; i < count; ++i) indices.push_back(i);
        return indices;
    }
    indices.reserve(limit);
    indices.push_back(0);
    const int buckets = (limit - 2) / 2;
    for (int b = 0; b < buckets; ++b) {
        const int begin = 1 + b * (count - 2) / buckets;
        const int end = 1 + (b + 1) * (count - 2) / buckets;
        int lo = begin, hi = begin;
        for (int i = begin + 1; i < end; ++i) {
            if (value(i) < value(lo)) lo = i;
            if (value(i) > value(hi)) hi = i;
        }
        if (lo < hi) { indices.push_back(lo); indices.push_back(hi); }
        else { indices.push_back(hi); if (lo != hi) indices.push_back(lo); }
    }
    indices.push_back(count - 1);
    return indices;
}

#endif
