#ifndef ATG_ENGINE_SIM_OSCILLOSCOPE_CLUSTER_H
#define ATG_ENGINE_SIM_OSCILLOSCOPE_CLUSTER_H

#include "ui_element.h"

#include "simulator.h"
#include "oscilloscope.h"
#include "history_clock.h"
#include <optional>
#include <functional>
#include <deque>

class OscilloscopeCluster : public UiElement {
    friend class EngineSimApplication; // Application-owned integration validation.
    private:
        static constexpr int MaxLayeredScopes = 5;

    public:
        OscilloscopeCluster();
        virtual ~OscilloscopeCluster();

        virtual void initialize(EngineSimApplication *app);
        virtual void destroy();
        virtual void signal(UiElement *element, Event event);

        virtual void update(float dt);
        virtual void render();

        void sample();
        void setSimulator(Simulator *simulator);
        void registerHistoryChannel(const std::string &id, const std::string &label,
            const std::string &unit, std::function<double()> source, double low, double high);
        void toggleHistoryChannel(const std::string &id);
        bool isHistoryChannelSelected(const std::string &id) const;
        ysVector historyChannelColor(const std::string &id) const;
        virtual void onMouseScroll(int delta);

        Oscilloscope *getTotalExhaustFlowOscilloscope() const { return m_totalExhaustFlowScope; }
        Oscilloscope *getExhaustFlowOscilloscope() const { return m_exhaustFlowScope; }
        Oscilloscope *getIntakeFlowOscilloscope() const { return m_intakeFlowScope; }
        Oscilloscope *getAudioWaveformOscilloscope() const { return m_audioWaveformScope; }
        Oscilloscope *getIntakeValveLiftOscilloscope() const { return m_intakeValveLiftScope; }
        Oscilloscope *getExhaustValveLiftOscilloscope() const { return m_exhaustValveLiftScope; }
        Oscilloscope *getCylinderPressureScope() const { return m_cylinderPressureScope; }
        Oscilloscope *getSparkAdvanceScope() const { return m_sparkAdvanceScope; }
        Oscilloscope *getCylinderMoleculesScope() const { return m_cylinderMoleculesScope; }
        Oscilloscope *getPvScope() const { return m_pvScope; }
        void setDynoMaxRange(double redline) { m_torqueScope->m_xMax = redline + 500; m_powerScope->m_xMax = redline + 500; }

    protected:
        Bounds getFocusBounds() const { return m_bounds.verticalSplit(0.0f, 0.78f); }
        void renderChannelHistory(const Bounds &bounds);
        struct HistoryChannel {
            std::string id, label, unit;
            std::function<double()> source;
            Oscilloscope *scope = nullptr;
            bool selected = false;
            bool *legacySelection = nullptr;
            double latest = 0;
            bool available = false;
            double initialLow = 0, initialHigh = 1;
            bool isSelected() const { return legacySelection ? *legacySelection : selected; }
            void toggle() { if (legacySelection) *legacySelection = !*legacySelection; else selected = !selected; }
        };
        std::deque<HistoryChannel> m_historyChannels;
        int m_scalePage = 0;
        Bounds getHistoryPlotBounds(const Bounds &bounds) const;
        // Only populated by --validate-history-ui; normal UI uses Raw Input.
        std::optional<Point> m_validationMousePosition;
        struct HistoryCursorReading {
            double time = 0;
            Oscilloscope::DataPoint rpm {}, torque {}, power {};
            bool hasRpm = false, hasTorque = false, hasPower = false;
        };
        HistoryCursorReading getHistoryCursor(double time) const;
        bool m_cursorActive = false;
        HistoryCursorReading m_cursorReading;
        Oscilloscope *m_rpmScope = nullptr;
        Oscilloscope *m_torqueHistoryScope = nullptr;
        Oscilloscope *m_powerHistoryScope = nullptr;
        bool m_historyMode = true;
        bool m_showRpm = true;
        bool m_showTorque = true;
        bool m_showPower = true;
        bool m_torqueAvailable = false;
        double m_latestTorque = 0;
        double m_latestPower = 0;
        HistoryClock m_historyClock;
        std::string m_focusTitle;
        void renderScope(
            Oscilloscope *osc,
            const Bounds &bounds,
            const std::string &title,
            bool overlay=false);

        Simulator *m_simulator;
        Oscilloscope
            *m_torqueScope,
            *m_powerScope,
            *m_audioWaveformScope,
            *m_exhaustValveLiftScope,
            *m_intakeValveLiftScope,
            *m_cylinderPressureScope,
            *m_totalExhaustFlowScope,
            *m_exhaustFlowScope,
            *m_intakeFlowScope,
            *m_cylinderMoleculesScope,
            *m_sparkAdvanceScope,
            *m_pvScope,
            *m_currentFocusScopes[MaxLayeredScopes];
        float m_updatePeriod;
        float m_updateTimer;

        double m_torque;
        double m_power;

        std::string m_powerUnits;
        std::string m_torqueUnits;

};

#endif /* ATG_ENGINE_SIM_OSCILLOSCOPE_CLUSTER_H */
