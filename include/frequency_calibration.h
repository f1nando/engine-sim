#ifndef ATG_ENGINE_SIM_FREQUENCY_CALIBRATION_H
#define ATG_ENGINE_SIM_FREQUENCY_CALIBRATION_H

#include <algorithm>
#include <cmath>
#include <vector>

// Startup estimate only: a stopped engine cannot predict every high-RPM workload.
class FrequencyCalibration {
public:
    void begin(int modelFrequency) {
        m_limit = modelFrequency < 10000 ? modelFrequency : 10000;
        m_elapsed = 0;
        m_costs.clear();
        m_active = m_limit > 0;
    }
    void cancel() { m_active = false; }
    bool active() const { return m_active; }
    void sample(double frameSeconds, double computeSeconds, int steps) {
        if (!m_active || !std::isfinite(frameSeconds) || frameSeconds <= 0) return;
        // Ignore long UI stalls rather than treating them as measured CPU work.
        m_elapsed += frameSeconds < 0.25 ? frameSeconds : 0.25;
        if (m_elapsed <= 1.0 || steps <= 0 || !std::isfinite(computeSeconds)
            || computeSeconds <= 0 || m_costs.size() >= 512) return;
        m_costs.push_back(computeSeconds / steps);
    }
    bool ready() const { return m_active && m_elapsed >= 4.0 && m_costs.size() >= 30; }
    int recommendation() const {
        if (!ready()) return m_limit;
        auto costs = m_costs;
        std::sort(costs.begin(), costs.end());
        // Reserve 70% of the measured time budget for high-RPM load, UI/audio.
        // This is an estimate, not a benchmark of the engine at its redline.
        const double budgetFrequency = 0.3 / costs[(costs.size() - 1) * 9 / 10];
        const int floor = m_limit < 5000 ? m_limit : 5000;
        if (budgetFrequency >= m_limit) return m_limit;
        if (budgetFrequency <= floor) return floor;
        return static_cast<int>(budgetFrequency / 500) * 500;
    }
    bool hasHeadroom(int frequency) const {
        if (!ready()) return false;
        auto costs = m_costs;
        std::sort(costs.begin(), costs.end());
        return costs[(costs.size() - 1) * 9 / 10] * frequency <= 0.3;
    }
private:
    int m_limit = 10000;
    double m_elapsed = 0;
    bool m_active = false;
    std::vector<double> m_costs;
};

#endif
