#ifndef ATG_ENGINE_SIM_HISTORY_CONTINUITY_H
#define ATG_ENGINE_SIM_HISTORY_CONTINUITY_H

// A missing measurement breaks the next segment, rather than drawing across it.
class HistoryContinuity {
public:
    void markGap() { m_gap = true; }
    bool beginPoint() {
        const bool gap = m_gap;
        m_gap = false;
        return gap;
    }
    void reset() { m_gap = false; }
private:
    bool m_gap = false;
};

#endif
