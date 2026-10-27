#ifndef ATG_ENGINE_SIM_DYNO_HISTORY_READING_H
#define ATG_ENGINE_SIM_DYNO_HISTORY_READING_H

#include "units.h"
#include <cmath>

// Same filtered torque and T * angular speed formula as Simulator::getDynoPower.
// Evaluate both channels from a single simulation-step snapshot.
struct DynoHistoryReading {
    double torqueNm = 0;
    double powerKw = 0;
    bool available = false;

    static DynoHistoryReading measure(bool enabled, double torque, double angularSpeed) {
        DynoHistoryReading reading;
        if (!enabled || !std::isfinite(torque) || !std::isfinite(angularSpeed)) return reading;
        reading.torqueNm = units::convert(torque, units::Nm);
        reading.powerKw = units::convert(torque * angularSpeed, units::kW);
        reading.available = std::isfinite(reading.torqueNm) && std::isfinite(reading.powerKw);
        return reading;
    }
};

#endif
