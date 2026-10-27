#ifndef ATG_ENGINE_SIM_HISTORY_CLOCK_H
#define ATG_ENGINE_SIM_HISTORY_CLOCK_H

#include <cmath>

// Driven exclusively by completed simulation steps, never by rendering FPS.
class HistoryClock {
public:
    static constexpr double SamplePeriod = 0.02;
    void reset() { m_time = m_sinceSample = 0; }
    double time() const { return m_time; }
    bool advance(double timestep) {
        if (!std::isfinite(timestep) || timestep <= 0) return false;
        m_time += timestep;
        m_sinceSample += timestep;
        if (m_sinceSample + 1e-10 < SamplePeriod) return false;
        m_sinceSample = std::fmod(m_sinceSample + 1e-10, SamplePeriod);
        return true;
    }
private:
    double m_time = 0;
    double m_sinceSample = 0;
};

#endif
