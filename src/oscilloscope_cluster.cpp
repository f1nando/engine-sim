#include "../include/oscilloscope_cluster.h"

#include "../include/engine_sim_application.h"
#include "../include/dyno_history_reading.h"

#include <sstream>
#include <iomanip>
#include <algorithm>

OscilloscopeCluster::OscilloscopeCluster() {
    m_simulator = nullptr;
    m_torqueScope = nullptr;
    m_powerScope = nullptr;
    m_totalExhaustFlowScope = nullptr;
    m_intakeFlowScope = nullptr;
    m_exhaustFlowScope = nullptr;
    m_exhaustValveLiftScope = nullptr;
    m_intakeValveLiftScope = nullptr;
    m_audioWaveformScope = nullptr;
    m_cylinderPressureScope = nullptr;
    m_sparkAdvanceScope = nullptr;
    m_cylinderMoleculesScope = nullptr;
    m_pvScope = nullptr;

    for (int i = 0; i < MaxLayeredScopes; ++i) {
        m_currentFocusScopes[i] = nullptr;
    }

    m_torque = 0;
    m_power = 0;

    m_updatePeriod = 0.25f;
    m_updateTimer = 0.0f;
}

OscilloscopeCluster::~OscilloscopeCluster() {
    /* void */
}

void OscilloscopeCluster::initialize(EngineSimApplication *app) {
    UiElement::initialize(app);

    m_torqueScope = addElement<Oscilloscope>(this);
    m_powerScope = addElement<Oscilloscope>(this);
    m_exhaustFlowScope = addElement<Oscilloscope>(this);
    m_totalExhaustFlowScope = addElement<Oscilloscope>(this);
    m_intakeFlowScope = addElement<Oscilloscope>(this);
    m_audioWaveformScope = addElement<Oscilloscope>(this);
    m_intakeValveLiftScope = addElement<Oscilloscope>(this);
    m_exhaustValveLiftScope = addElement<Oscilloscope>(this);
    m_cylinderPressureScope = addElement<Oscilloscope>(this);
    m_sparkAdvanceScope = addElement<Oscilloscope>(this);
    m_cylinderMoleculesScope = addElement<Oscilloscope>(this);
    m_pvScope = addElement<Oscilloscope>(this);
    m_rpmScope = addElement<Oscilloscope>(this);
    m_rpmScope->setBufferSize(1501);
    m_rpmScope->m_xMin = 0;
    m_rpmScope->m_xMax = 30;
    m_rpmScope->m_yMin = 0;
    m_rpmScope->m_yMax = 8000;
    m_rpmScope->m_dynamicallyResizeX = false;
    m_rpmScope->m_dynamicallyResizeY = false;
    m_rpmScope->m_drawReverse = false;
    m_rpmScope->m_lineWidth = 2;
    m_rpmScope->i_color = m_app->getGreen();
    m_torqueHistoryScope = addElement<Oscilloscope>(this);
    m_torqueHistoryScope->setBufferSize(1501);
    m_torqueHistoryScope->m_xMin = 0;
    m_torqueHistoryScope->m_xMax = 30;
    m_torqueHistoryScope->m_yMin = -100;
    m_torqueHistoryScope->m_yMax = 500;
    m_torqueHistoryScope->m_dynamicallyResizeX = false;
    m_torqueHistoryScope->m_dynamicallyResizeY = false;
    m_torqueHistoryScope->m_drawReverse = false;
    m_torqueHistoryScope->m_drawZero = false;
    m_torqueHistoryScope->m_lineWidth = 2;
    m_torqueHistoryScope->i_color = m_app->getOrange();
    m_powerHistoryScope = addElement<Oscilloscope>(this);
    m_powerHistoryScope->setBufferSize(1501);
    m_powerHistoryScope->m_xMin = 0;
    m_powerHistoryScope->m_xMax = 30;
    m_powerHistoryScope->m_yMin = -50;
    m_powerHistoryScope->m_yMax = 250;
    m_powerHistoryScope->m_dynamicallyResizeX = false;
    m_powerHistoryScope->m_dynamicallyResizeY = false;
    m_powerHistoryScope->m_drawReverse = false;
    m_powerHistoryScope->m_drawZero = false;
    m_powerHistoryScope->m_lineWidth = 2;
    m_powerHistoryScope->i_color = m_app->getPink();

    // Torque
    m_torqueScope->setBufferSize(100);
    m_torqueScope->m_xMin = 0.0f;
    m_torqueScope->m_yMin = 0.0f;
    m_torqueScope->m_yMax = 0.0f;
    m_torqueScope->m_lineWidth = 2.0f;
    m_torqueScope->m_drawReverse = false;
    m_torqueScope->m_dynamicallyResizeX = true;
    m_torqueScope->i_color = m_app->getOrange();

    // Power
    m_powerScope->setBufferSize(100);
    m_powerScope->m_xMin = 0.0f;
    m_powerScope->m_yMin = 0.0f;
    m_powerScope->m_yMax = 0.0f;
    m_powerScope->m_lineWidth = 2.0f;
    m_powerScope->m_drawReverse = false;
    m_powerScope->m_dynamicallyResizeX = true;
    m_powerScope->i_color = m_app->getPink();

    // Total exhaust flow
    m_totalExhaustFlowScope->setBufferSize(1024);
    m_totalExhaustFlowScope->m_xMin = 0.0f;
    m_totalExhaustFlowScope->m_xMax = constants::pi * 4;
    m_totalExhaustFlowScope->m_yMin = -units::flow(10, units::scfm);
    m_totalExhaustFlowScope->m_yMax = units::flow(10, units::scfm);
    m_totalExhaustFlowScope->m_lineWidth = 2.0f;
    m_totalExhaustFlowScope->m_drawReverse = false;
    m_totalExhaustFlowScope->i_color = m_app->getOrange();

    // Exhaust flow
    m_exhaustFlowScope->setBufferSize(1024);
    m_exhaustFlowScope->m_xMin = 0.0f;
    m_exhaustFlowScope->m_xMax = constants::pi * 4;
    m_exhaustFlowScope->m_yMin = -units::flow(10.0, units::scfm);
    m_exhaustFlowScope->m_yMax = units::flow(10.0, units::scfm);
    m_exhaustFlowScope->m_lineWidth = 2.0f;
    m_exhaustFlowScope->m_drawReverse = false;
    m_exhaustFlowScope->i_color = m_app->getOrange();

    // Intake flow
    m_intakeFlowScope->setBufferSize(1024);
    m_intakeFlowScope->m_xMin = 0.0f;
    m_intakeFlowScope->m_xMax = constants::pi * 4;
    m_intakeFlowScope->m_yMin = -units::flow(10.0, units::scfm);
    m_intakeFlowScope->m_yMax = units::flow(10.0, units::scfm);
    m_intakeFlowScope->m_lineWidth = 2.0f;
    m_intakeFlowScope->m_drawReverse = false;
    m_intakeFlowScope->i_color = m_app->getBlue();

    // Cylinder molcules
    m_cylinderMoleculesScope->setBufferSize(1024);
    m_cylinderMoleculesScope->m_xMin = 0.0f;
    m_cylinderMoleculesScope->m_xMax = constants::pi * 4;
    m_cylinderMoleculesScope->m_yMin = -0.05;
    m_cylinderMoleculesScope->m_yMax = 0.2;
    m_cylinderMoleculesScope->m_lineWidth = 4.0f;
    m_cylinderMoleculesScope->m_drawReverse = false;
    m_cylinderMoleculesScope->i_color = m_app->getForegroundColor();

    // Audio waveform scope
    m_audioWaveformScope->setBufferSize(44100 / 50);
    m_audioWaveformScope->m_xMin = 0.0f;
    m_audioWaveformScope->m_xMax = 44100 / 10;
    m_audioWaveformScope->m_yMin = -1.5f;
    m_audioWaveformScope->m_yMax = 1.5f;
    m_audioWaveformScope->m_lineWidth = 2.0f;
    m_audioWaveformScope->m_drawReverse = false;
    m_audioWaveformScope->i_color = m_app->getBlue();

    // Valve lift scopes
    m_exhaustValveLiftScope->setBufferSize(1024);
    m_exhaustValveLiftScope->m_xMin = 0.0f;
    m_exhaustValveLiftScope->m_xMax = constants::pi * 4;
    m_exhaustValveLiftScope->m_yMin = (float)units::distance(-10, units::thou);
    m_exhaustValveLiftScope->m_yMax = (float)units::distance(10, units::thou);
    m_exhaustValveLiftScope->m_lineWidth = 2.0f;
    m_exhaustValveLiftScope->m_drawReverse = false;
    m_exhaustValveLiftScope->i_color = m_app->getOrange();

    m_intakeValveLiftScope->setBufferSize(1024);
    m_intakeValveLiftScope->m_xMin = 0.0f;
    m_intakeValveLiftScope->m_xMax = constants::pi * 4;
    m_intakeValveLiftScope->m_yMin = (float)units::distance(-10, units::thou);
    m_intakeValveLiftScope->m_yMax = (float)units::distance(10, units::thou);
    m_intakeValveLiftScope->m_lineWidth = 2.0f;
    m_intakeValveLiftScope->m_drawReverse = false;
    m_intakeValveLiftScope->i_color = m_app->getBlue();

    // Cylinder pressure scope
    m_cylinderPressureScope->setBufferSize(1024);
    m_cylinderPressureScope->m_xMin = 0.0f;
    m_cylinderPressureScope->m_xMax = constants::pi * 4;
    m_cylinderPressureScope->m_yMin = -(float)std::sqrt(units::pressure(1, units::psi));
    m_cylinderPressureScope->m_yMax = (float)std::sqrt(units::pressure(1, units::psi));
    m_cylinderPressureScope->m_lineWidth = 2.0f;
    m_cylinderPressureScope->m_drawReverse = false;
    m_cylinderPressureScope->i_color = m_app->getOrange();

    // Pressure volume scope
    m_pvScope->setBufferSize(1024);
    m_pvScope->m_xMin = 0.0f;
    m_pvScope->m_xMax = units::volume(0.1, units::L);
    m_pvScope->m_yMin = -(float)std::sqrt(units::pressure(1, units::psi));
    m_pvScope->m_yMax = (float)std::sqrt(units::pressure(1, units::psi));
    m_pvScope->m_lineWidth = 2.0f;
    m_pvScope->m_drawReverse = true;
    m_pvScope->i_color = m_app->getOrange();
    m_pvScope->m_dynamicallyResizeX = true;

    // Spark advance scope
    m_sparkAdvanceScope->setBufferSize(1024);
    m_sparkAdvanceScope->m_xMin = 0.0f;
    m_sparkAdvanceScope->m_xMax = units::rpm(10000);
    m_sparkAdvanceScope->m_yMin = -units::angle(30, units::deg);
    m_sparkAdvanceScope->m_yMax = units::angle(60, units::deg);
    m_sparkAdvanceScope->m_lineWidth = 2.0f;
    m_sparkAdvanceScope->m_drawReverse = true;
    m_sparkAdvanceScope->i_color = m_app->getOrange();

    m_currentFocusScopes[0] = m_rpmScope;
    m_currentFocusScopes[1] = nullptr;

    m_torqueUnits = app->getAppSettings()->torqueUnits;
    m_powerUnits = app->getAppSettings()->powerUnits;
    auto channel = [&](const char *id, const char *label, const char *unit,
        Oscilloscope *scope, bool *selected) {
        HistoryChannel entry;
        entry.id = id; entry.label = label; entry.unit = unit;
        entry.scope = scope; entry.legacySelection = selected;
        scope->m_maxRenderPoints = 600;
        m_historyChannels.push_back(entry);
    };
    channel("rpm", "Engine speed", "RPM", m_rpmScope, &m_showRpm);
    channel("dyno_torque", "Dyno torque", "Nm", m_torqueHistoryScope, &m_showTorque);
    channel("dyno_power", "Dyno power", "kW", m_powerHistoryScope, &m_showPower);
    m_checkMouse = true;
}

void OscilloscopeCluster::destroy() {
    UiElement::destroy();
}

void OscilloscopeCluster::signal(UiElement *element, Event event) {
    if (event == Event::Clicked) {
        if (element == m_rpmScope || element == m_torqueHistoryScope || element == m_powerHistoryScope) {
            if (m_historyMode) {
                if (element == m_rpmScope) m_showRpm = !m_showRpm;
                else if (element == m_torqueHistoryScope) m_showTorque = !m_showTorque;
                else m_showPower = !m_showPower;
            }
            // Returning from an original graph restores the chosen history lines.
            m_historyMode = true;
            return;
        }
        m_historyMode = false;
        if (element == m_audioWaveformScope) {
            m_currentFocusScopes[0] = m_audioWaveformScope;
            m_currentFocusScopes[1] = nullptr;
        }
        else if (element == m_powerScope || element == m_torqueScope) {
            m_currentFocusScopes[0] = m_torqueScope;
            m_currentFocusScopes[1] = m_powerScope;
            m_currentFocusScopes[2] = nullptr;
        }
        else if (element == m_totalExhaustFlowScope) {
            m_currentFocusScopes[0] = m_totalExhaustFlowScope;
            m_currentFocusScopes[1] = nullptr;
        }
        else if (
            element == m_intakeValveLiftScope
            || element == m_exhaustValveLiftScope)
        {
            m_currentFocusScopes[0] = m_intakeValveLiftScope;
            m_currentFocusScopes[1] = m_exhaustValveLiftScope;
            m_currentFocusScopes[2] = nullptr;
        }
        else if (element == m_pvScope) {
            m_currentFocusScopes[0] = m_pvScope;
            m_currentFocusScopes[1] = nullptr;
        }
        else if (
            element == m_intakeFlowScope
            || element == m_exhaustFlowScope
            || element == m_cylinderMoleculesScope)
        {
            m_currentFocusScopes[0] = m_intakeFlowScope;
            m_currentFocusScopes[1] = m_exhaustFlowScope;
            m_currentFocusScopes[2] = m_cylinderMoleculesScope;
            m_currentFocusScopes[3] = nullptr;
        }
        else if (element == m_cylinderPressureScope) {
            m_currentFocusScopes[0] = m_cylinderPressureScope;
            m_currentFocusScopes[1] = nullptr;
        }
        else if (element == m_sparkAdvanceScope) {
            m_currentFocusScopes[0] = m_sparkAdvanceScope;
            m_currentFocusScopes[1] = nullptr;
        }
    }
}

void OscilloscopeCluster::update(float dt) {
    m_mouseBounds = getFocusBounds();
    const double torque = (m_torqueUnits == "Nm")
        ? (units::convert(m_simulator->getFilteredDynoTorque(), units::Nm))
        : (units::convert(m_simulator->getFilteredDynoTorque(), units::ft_lb));

    const double power = (m_powerUnits == "kW")
        ? (units::convert(m_simulator->getDynoPower(), units::kW))
        : (units::convert(m_simulator->getDynoPower(), units::hp));

    m_torque = m_torque * 0.95 + 0.05 * torque;
    m_power = m_power * 0.95 + 0.05 * power;

    Engine *engine = m_simulator->getEngine();
    if (engine != nullptr) {
        if (m_updateTimer <= 0 && m_simulator->m_dyno.m_enabled) {
            m_updateTimer = m_updatePeriod;

            m_torqueScope->addDataPoint(engine->getRpm(), m_torque);
            m_powerScope->addDataPoint(engine->getRpm(), m_power);
        }

        m_sparkAdvanceScope->addDataPoint(
            -engine->getCrankshaft(0)->m_body.v_theta,
            engine->getIgnitionModule()->getTimingAdvance());
    }

    m_updateTimer -= dt;

    UiElement::update(dt);
}

void OscilloscopeCluster::render() {
    Grid grid;
    grid.h_cells = 9;
    grid.v_cells = 1;
    const Bounds cards = m_bounds.verticalSplit(0.78f, 1.0f);

    const Bounds &hpTorqueBounds = grid.get(cards, 3, 0);
    renderScope(m_torqueScope, hpTorqueBounds, "Dyno / RPM");
    renderScope(m_powerScope, hpTorqueBounds, "", true);

    const Bounds &valveLiftBounds = grid.get(cards, 7, 0);
    renderScope(m_intakeValveLiftScope, valveLiftBounds, "Valve Lift");
    renderScope(m_exhaustValveLiftScope, valveLiftBounds, "", true);

    const Bounds &flowBounds = grid.get(cards, 8, 0);
    renderScope(m_intakeFlowScope, flowBounds, "Flow");
    renderScope(m_exhaustFlowScope, flowBounds, "", true);
    renderScope(m_cylinderMoleculesScope, flowBounds, "", true);

    const Bounds &audioWaveformBounds = grid.get(cards, 4, 0);
    renderScope(m_audioWaveformScope, audioWaveformBounds, "Waveform");

    const Bounds &cylinderPressureBounds = grid.get(cards, 5, 0);
    renderScope(m_pvScope, cylinderPressureBounds, "pressure-volume");

    const Bounds &totalExhaustPressureBounds = grid.get(cards, 6, 0);
    renderScope(m_totalExhaustFlowScope, totalExhaustPressureBounds, "Total Exhaust Flow");
    renderScope(m_rpmScope, grid.get(cards, 0, 0), "RPM / time");
    renderScope(m_torqueHistoryScope, grid.get(cards, 1, 0), "Dyno Nm / time");
    renderScope(m_powerHistoryScope, grid.get(cards, 2, 0), "Power kW / time");

    const Bounds focusBounds = getFocusBounds();
    Bounds focusTitle = focusBounds;
    focusTitle.m0.y = focusTitle.m1.y - (24.0f + 15.0f);
    Bounds focusBody = focusBounds;
    focusBody.m1 = focusBody.m1 - Point(0.0f, 24.0f + 15.0f);

    drawFrame(focusTitle, 1.0, m_app->getForegroundColor(), m_app->getBackgroundColor());
    drawFrame(focusBody, 1.0, m_app->getForegroundColor(), m_app->getBackgroundColor());
    if (m_historyMode) {
        renderChannelHistory(focusBody);
        std::ostringstream title;
        if (m_cursorActive) {
            title << "Cursor: " << std::fixed << std::setprecision(3) << m_cursorReading.time
                << " s (sim) | " << (m_cursorReading.hasRpm
                    ? "recorded values at one sample" : "no recorded sample");
        }
        else title << "Live values | 30s (sim) | click instruments to add/remove | hover: cursor";
        int selected = 0;
        for (const auto &channel : m_historyChannels) if (channel.isSelected()) ++selected;
        title << " | scales " << (selected ? m_scalePage * 3 + 1 : 0) << "-"
            << (std::min)(selected, m_scalePage * 3 + 3) << "/" << selected;
        if (selected > 3) title << " (wheel)";
        drawText(title.str(), focusTitle.inset(8.0f), 14.0f, Bounds::tl);
    }
    else {
        drawText(m_focusTitle + " | original axes (not time history)",
            focusTitle.inset(8.0f), 18.0f, Bounds::tl);
    }

    for (int i = 0; !m_historyMode && i < MaxLayeredScopes; ++i) {
        if (m_currentFocusScopes[i] != nullptr) {
            m_currentFocusScopes[i]->render(focusBody);
        }
        else break;
    }

    UiElement::render();
}

void OscilloscopeCluster::sample() {
    Engine *engine = m_simulator->getEngine();
    if (engine == nullptr) return;
    if (m_historyClock.advance(m_simulator->getTimestep())) {
        const double rpm = engine->getRpm();
        m_rpmScope->m_xMin = std::fmax(0.0, m_historyClock.time() - 30.0);
        m_rpmScope->m_xMax = m_rpmScope->m_xMin + 30.0;
        m_rpmScope->discardBeforeX(m_rpmScope->m_xMin);
        m_torqueHistoryScope->m_xMin = m_rpmScope->m_xMin;
        m_torqueHistoryScope->m_xMax = m_rpmScope->m_xMax;
        m_torqueHistoryScope->discardBeforeX(m_rpmScope->m_xMin);
        m_powerHistoryScope->m_xMin = m_rpmScope->m_xMin;
        m_powerHistoryScope->m_xMax = m_rpmScope->m_xMax;
        m_powerHistoryScope->discardBeforeX(m_rpmScope->m_xMin);
        const auto reading = DynoHistoryReading::measure(m_simulator->m_dyno.m_enabled,
            m_simulator->getFilteredDynoTorque(), engine->getSpeed());
        const double torque = reading.torqueNm;
        m_torqueAvailable = reading.available;
        if (m_torqueAvailable) {
            m_latestTorque = torque;
            m_torqueHistoryScope->m_yMin = std::fmin(m_torqueHistoryScope->m_yMin,
                std::floor(torque / 100.0) * 100.0);
            m_torqueHistoryScope->m_yMax = std::fmax(m_torqueHistoryScope->m_yMax,
                std::ceil(torque / 100.0 + 1.0) * 100.0);
            m_torqueHistoryScope->addDataPoint(m_historyClock.time(), torque);
            m_latestPower = reading.powerKw;
            m_powerHistoryScope->m_yMin = std::fmin(m_powerHistoryScope->m_yMin,
                std::floor(reading.powerKw / 50.0) * 50.0);
            m_powerHistoryScope->m_yMax = std::fmax(m_powerHistoryScope->m_yMax,
                std::ceil(reading.powerKw / 50.0 + 1.0) * 50.0);
            m_powerHistoryScope->addDataPoint(m_historyClock.time(), reading.powerKw);
        }
        else {
            m_torqueHistoryScope->markGap();
            m_powerHistoryScope->markGap();
        }
        if (std::isfinite(rpm)) {
            m_rpmScope->m_yMax = std::fmax(m_rpmScope->m_yMax,
                std::ceil(rpm / 1000.0 + 1.0) * 1000.0);
            m_rpmScope->addDataPoint(m_historyClock.time(), rpm);
        }
        for (auto &channel : m_historyChannels) {
            if (!channel.source) {
                channel.available = channel.scope == m_rpmScope ? std::isfinite(rpm) : m_torqueAvailable;
                channel.latest = channel.scope == m_rpmScope ? rpm
                    : channel.scope == m_torqueHistoryScope ? m_latestTorque : m_latestPower;
                continue;
            }
            channel.scope->m_xMin = m_rpmScope->m_xMin;
            channel.scope->m_xMax = m_rpmScope->m_xMax;
            channel.scope->discardBeforeX(m_rpmScope->m_xMin);
            const double value = channel.source();
            channel.available = std::isfinite(value);
            if (channel.available) {
                channel.latest = value;
                const double margin = std::fmax(0.01, std::abs(value) * 0.1);
                channel.scope->m_yMin = std::fmin(channel.scope->m_yMin, value - margin);
                channel.scope->m_yMax = std::fmax(channel.scope->m_yMax, value + margin);
                channel.scope->addDataPoint(m_historyClock.time(), value);
            }
            else channel.scope->markGap();
        }
    }

    const double cylinderPressure = engine->getChamber(0)->m_system.pressure()
        + engine->getChamber(0)->m_system.dynamicPressure(-1.0, 0.0);

    if (m_simulator->getCurrentIteration() % 2 == 0) {
        double cycleAngle = engine->getCrankshaft(0)->getCycleAngle();
        if (!engine->isSpinningCw()) {
            cycleAngle = 4 * constants::pi - cycleAngle;
        }

        getTotalExhaustFlowOscilloscope()->addDataPoint(
            cycleAngle,
            m_simulator->getTotalExhaustFlow() / m_simulator->getTimestep());
        getCylinderPressureScope()->addDataPoint(
            engine->getCrankshaft(0)->getCycleAngle(constants::pi),
            std::sqrt(cylinderPressure));
        getExhaustFlowOscilloscope()->addDataPoint(
            cycleAngle,
            engine->getChamber(0)->getLastTimestepExhaustFlow() / m_simulator->getTimestep());
        getIntakeFlowOscilloscope()->addDataPoint(
            cycleAngle,
            engine->getChamber(0)->getLastTimestepIntakeFlow() / m_simulator->getTimestep());
        getCylinderMoleculesScope()->addDataPoint(
            cycleAngle,
            engine->getChamber(0)->m_system.n());
        getExhaustValveLiftOscilloscope()->addDataPoint(
            cycleAngle,
            engine->getChamber(0)->getCylinderHead()->exhaustValveLift(
                engine->getChamber(0)->getPiston()->getCylinderIndex()));
        getIntakeValveLiftOscilloscope()->addDataPoint(
            cycleAngle,
            engine->getChamber(0)->getCylinderHead()->intakeValveLift(
                engine->getChamber(0)->getPiston()->getCylinderIndex()));
        getPvScope()->addDataPoint(
            engine->getChamber(0)->getVolume(),
            std::sqrt(engine->getChamber(0)->m_system.pressure()));
    }

    m_exhaustFlowScope->m_yMin = m_intakeFlowScope->m_yMin =
        std::fmin(m_intakeFlowScope->m_yMin, m_exhaustFlowScope->m_yMin);
    m_exhaustFlowScope->m_yMax = m_intakeFlowScope->m_yMax =
        std::fmax(m_intakeFlowScope->m_yMax, m_exhaustFlowScope->m_yMax);

    m_torqueScope->m_yMin = m_powerScope->m_yMin =
        std::fmin(m_torqueScope->m_yMin, m_powerScope->m_yMin);
    m_torqueScope->m_yMax = m_powerScope->m_yMax =
        std::fmax(m_torqueScope->m_yMax, m_powerScope->m_yMax);

    m_powerScope->m_xMax = m_torqueScope->m_xMax =
        std::fmax(m_powerScope->m_xMax, units::toRpm(engine->getSpeed()));
}

void OscilloscopeCluster::setSimulator(Simulator *simulator) {
    m_simulator = simulator;
    m_cursorActive = false;
    m_cursorReading = HistoryCursorReading {};
    for (auto &channel : m_historyChannels) {
        channel.available = false;
        channel.latest = 0;
        if (channel.source) {
            channel.scope->reset();
            channel.scope->m_xMin = 0;
            channel.scope->m_xMax = 30;
            channel.scope->m_yMin = channel.initialLow;
            channel.scope->m_yMax = channel.initialHigh;
        }
    }
    m_historyClock.reset();
    m_rpmScope->reset();
    m_torqueHistoryScope->reset();
    m_powerHistoryScope->reset();
    m_powerHistoryScope->m_xMin = 0;
    m_powerHistoryScope->m_xMax = 30;
    m_powerHistoryScope->m_yMin = -50;
    m_powerHistoryScope->m_yMax = 250;
    m_latestPower = 0;
    m_torqueHistoryScope->m_xMin = 0;
    m_torqueHistoryScope->m_xMax = 30;
    m_torqueHistoryScope->m_yMin = -100;
    m_torqueHistoryScope->m_yMax = 500;
    m_torqueAvailable = false;
    m_latestTorque = 0;
    m_rpmScope->m_xMin = 0;
    m_rpmScope->m_xMax = 30;
    Engine *engine = simulator != nullptr ? simulator->getEngine() : nullptr;
    m_rpmScope->m_yMax = engine != nullptr
        ? std::fmax(1000.0, std::ceil(units::toRpm(engine->getRedline()) / 1000.0) * 1000.0)
        : 8000.0;
}

void OscilloscopeCluster::renderScope(
    Oscilloscope *osc,
    const Bounds &bounds,
    const std::string &title,
    bool overlay)
{
    if (!overlay) {
        drawFrame(bounds, 1.0, m_app->getForegroundColor(), m_app->getBackgroundColor());
        if (osc == m_rpmScope || osc == m_torqueHistoryScope || osc == m_powerHistoryScope) {
            const bool selected = osc == m_rpmScope ? m_showRpm
                : osc == m_torqueHistoryScope ? m_showTorque : m_showPower;
            drawFrame(bounds.inset(2.0f), selected ? 2.0f : 1.0f,
                selected ? osc->i_color : m_app->getForegroundColor(),
                m_app->getBackgroundColor(), false);
        }
    }

    if (!overlay) {
        drawText(title, bounds.inset(5.0f), 12.0f, Bounds::tl);
    }
    if (osc == m_currentFocusScopes[0] && osc != m_rpmScope) {
        m_focusTitle = title;
    }
    osc->m_bounds = bounds.inset(5.0f);
    osc->m_bounds.m1.y -= 18.0f;
}

OscilloscopeCluster::HistoryCursorReading OscilloscopeCluster::getHistoryCursor(double time) const {
    HistoryCursorReading reading;
    reading.time = time;
    // RPM is the reference clock even when its line is hidden. Match the other
    // channels to that exact stored timestamp, never independently to the mouse.
    reading.hasRpm = m_rpmScope->findNearestPoint(time, HistoryClock::SamplePeriod, reading.rpm);
    if (reading.hasRpm) {
        reading.time = reading.rpm.x;
        reading.hasTorque = m_torqueHistoryScope->findNearestPoint(reading.time, 1e-9, reading.torque);
        reading.hasPower = m_powerHistoryScope->findNearestPoint(reading.time, 1e-9, reading.power);
    }
    return reading;
}

Bounds OscilloscopeCluster::getHistoryPlotBounds(const Bounds &bounds) const {
    Bounds plot = bounds.inset(12.0f);
    plot.m0.x += 130.0f;
    plot.m1.x -= 310.0f;
    plot.m0.y += 22.0f;
    plot.m1.y -= 45.0f;
    return plot;
}
