#include "../include/engine_sim_application.h"
#include "../include/oscilloscope_cluster.h"
#include "../include/engine_view.h"
#include "../include/right_gauge_cluster.h"

#include <chrono>
#include <cmath>
#include <fstream>

// Native integration check: real physics, the actual scope buffers/controller
// and renderer, without emulating hardware Raw Input or requiring foreground.
// Does not certify physical mouse hit-testing, image appearance, or audio.
bool EngineSimApplication::validateHistoryInterface() {
    struct StopAudioThread {
        Simulator **simulator;
        ~StopAudioThread() { if (*simulator != nullptr) (*simulator)->endAudioRenderingThread(); }
    } stopAudioThread { &m_simulator };
    std::ofstream log("history-ui-validation.log", std::ios::trunc);
    if (!log || m_iceEngine == nullptr) return false;
    log << "ENGINE: " << m_iceEngine->getName() << " | physics Hz="
        << m_simulator->getSimulationFrequency() << '\n';
    bool passed = true;
    auto check = [&](bool condition, const char *description) {
        log << (condition ? "PASS: " : "FAIL: ") << description << '\n';
        log.flush();
        passed = passed && condition;
    };
    m_audioSource->SetVolume(0.0f);
    m_frequencyCalibration.cancel();
    m_paused = false;
    const auto started = std::chrono::steady_clock::now();
    auto advanceTo = [&](double time) {
        while (m_oscCluster->m_historyClock.time() < time) {
            m_engine.StartFrame();
            if (!m_engine.IsOpen() || std::chrono::steady_clock::now() - started > std::chrono::seconds(100)) {
                log << "FAIL: window closed or validation timed out\n";
                return false;
            }
            m_gameWindowHeight = m_engine.GetGameWindow()->GetGameHeight();
            m_screenHeight = m_engine.GetGameWindow()->GetScreenHeight();
            m_screenWidth = m_engine.GetGameWindow()->GetScreenWidth();
            process(m_engine.GetFrameLength());
            m_uiManager.update(m_engine.GetFrameLength());
            renderScene();
            m_engine.EndFrame();
        }
        return true;
    };
    auto empty = [&]() {
        return m_oscCluster->m_rpmScope->getPointCount() == 0
            && m_oscCluster->m_torqueHistoryScope->getPointCount() == 0
            && m_oscCluster->m_powerHistoryScope->getPointCount() == 0
            && m_oscCluster->m_historyClock.time() == 0;
    };
    check(empty(), "all three histories start empty");
    check(!m_oscCluster->getHistoryCursor(0).hasRpm, "cursor on empty history has no fabricated values");
    m_iceEngine->getIgnitionModule()->m_enabled = true;
    m_iceEngine->setSpeedControl(0.2);
    m_simulator->m_starterMotor.m_enabled = true;
    m_simulator->m_dyno.m_enabled = false;
    if (!advanceTo(2.0)) return false;
    check(m_oscCluster->m_rpmScope->getPointCount() > 0, "real simulation produces RPM history");
    check(m_iceEngine->getRpm() > 300, "starter/ignition spin the real engine above 300 RPM");
    check(m_oscCluster->m_torqueHistoryScope->getPointCount() == 0
        && m_oscCluster->m_powerHistoryScope->getPointCount() == 0,
        "disabled dyno produces no false zero Nm/kW samples");
    check(m_oscCluster->m_historyChannels.size() == 33, "registry contains 33 unique instrument channels");
    const double frequencyBeforeClicks = m_simulator->getSimulationFrequency();
    const double throttleBeforeClicks = m_iceEngine->getThrottlePlateAngle();
    const bool ignitionBeforeClicks = m_iceEngine->getIgnitionModule()->m_enabled;
    int bindings = 0;
    bool hitTests = true, toggles = true;
    std::function<void(UiElement *)> testBindings = [&](UiElement *element) {
        const std::string id = element->getHistoryChannel();
        if (!id.empty()) {
            ++bindings;
            const Point point = element->localToWorld(element->m_bounds.getPosition(Bounds::center));
            UiElement *hit = m_uiManager.getRoot()->mouseOver(point);
            if (hit == nullptr || hit->getHistoryChannel() != id) {
                hitTests = false;
                log << "HIT MISS: " << id << '\n';
            }
            const bool selected = isHistorySelected(id);
            element->onMouseClick(element->m_bounds.getPosition(Bounds::center));
            toggles = toggles && isHistorySelected(id) != selected;
            element->onMouseClick(element->m_bounds.getPosition(Bounds::center));
            toggles = toggles && isHistorySelected(id) == selected;
        }
        for (size_t i = 0; i < element->getChildCount(); ++i) testBindings(element->getChild(i));
    };
    testBindings(m_uiManager.getRoot());
    log << "INSTRUMENT BINDINGS: " << bindings << '\n';
    check(bindings == 34 && hitTests, "all 34 instrument regions hit-test to their history binding");
    check(toggles, "instrument clicks toggle only their shared channel and toggle back without duplicates");
    check(m_simulator->getSimulationFrequency() == frequencyBeforeClicks
        && m_iceEngine->getThrottlePlateAngle() == throttleBeforeClicks
        && m_iceEngine->getIgnitionModule()->m_enabled == ignitionBeforeClicks,
        "history selection does not change physics frequency, throttle or ignition controls");

    m_simulator->m_starterMotor.m_enabled = false;
    m_simulator->m_dyno.m_hold = true;
    m_simulator->m_dyno.m_rotationSpeed = units::rpm(1500);
    m_simulator->m_dyno.m_enabled = true;
    if (!advanceTo(4.0)) return false;
    Oscilloscope::DataPoint rpm, torque, power;
    auto latest = [](Oscilloscope *scope, Oscilloscope::DataPoint &point) {
        return scope->getDataPoint(scope->getPointCount() - 1, point);
    };
    const bool measured = latest(m_oscCluster->m_rpmScope, rpm)
        && latest(m_oscCluster->m_torqueHistoryScope, torque)
        && latest(m_oscCluster->m_powerHistoryScope, power);
    check(measured, "enabled dyno records all three channels");
    if (measured) {
        check(rpm.x == torque.x && torque.x == power.x, "RPM/Nm/kW timestamps match exactly");
        check(std::isfinite(power.y) && std::abs(power.y - torque.y * units::rpm(rpm.y) / units::kW) < 1e-6,
            "kW matches measured Nm times angular speed");
        log << "MEASURED: t=" << rpm.x << " RPM=" << rpm.y << " Nm=" << torque.y << " kW=" << power.y << '\n';
        const auto cursor = m_oscCluster->getHistoryCursor(rpm.x - 0.003);
        check(cursor.hasRpm && cursor.hasTorque && cursor.hasPower
            && cursor.time == cursor.rpm.x && cursor.time == cursor.torque.x
            && cursor.time == cursor.power.x && cursor.rpm.y == rpm.y
            && cursor.torque.y == torque.y && cursor.power.y == power.y,
            "cursor snaps all three values to one real recorded sample without interpolation");
    }
    check(!m_oscCluster->getHistoryCursor(25.0).hasRpm,
        "cursor in the unrecorded future does not extrapolate");
    const int torqueBeforeGap = m_oscCluster->m_torqueHistoryScope->getPointCount();
    const int powerBeforeGap = m_oscCluster->m_powerHistoryScope->getPointCount();
    m_simulator->m_dyno.m_enabled = false;
    if (!advanceTo(5.0)) return false;
    check(!m_oscCluster->m_torqueAvailable
        && m_oscCluster->m_torqueHistoryScope->getPointCount() == torqueBeforeGap
        && m_oscCluster->m_powerHistoryScope->getPointCount() == powerBeforeGap,
        "dyno OFF marks readings unavailable and adds no samples");
    const auto gapCursor = m_oscCluster->getHistoryCursor(4.5);
    check(gapCursor.hasRpm && !gapCursor.hasTorque && !gapCursor.hasPower,
        "cursor in a dyno gap shows RPM but no stale Nm/kW values");
    const auto pastCursor = m_oscCluster->getHistoryCursor(3.0);
    check(pastCursor.hasRpm && pastCursor.hasTorque && pastCursor.hasPower,
        "cursor can inspect historical dyno readings while dyno is currently OFF");
    m_simulator->m_dyno.m_enabled = true;
    if (!advanceTo(6.0)) return false;
    Oscilloscope::DataPoint resumedTorque, resumedPower;
    check(m_oscCluster->m_torqueHistoryScope->getDataPoint(torqueBeforeGap, resumedTorque)
        && m_oscCluster->m_powerHistoryScope->getDataPoint(powerBeforeGap, resumedPower)
        && resumedTorque.breakBefore && resumedPower.breakBefore
        && resumedTorque.x == resumedPower.x, "both dyno traces break across the missing interval");

    auto click = [&](Oscilloscope *scope) { m_oscCluster->signal(scope, UiElement::Event::Clicked); };
    const int recordedPower = m_oscCluster->m_powerHistoryScope->getPointCount();
    click(m_oscCluster->m_powerHistoryScope);
    check(!m_oscCluster->m_showPower && m_oscCluster->m_showRpm && m_oscCluster->m_showTorque,
        "power card hides only the power line");
    if (!advanceTo(6.2)) return false;
    check(m_oscCluster->m_powerHistoryScope->getPointCount() > recordedPower,
        "hidden power channel continues recording");
    const auto hiddenCursor = m_oscCluster->getHistoryCursor(6.1);
    check(hiddenCursor.hasRpm && hiddenCursor.hasPower && hiddenCursor.time == hiddenCursor.power.x,
        "cursor lookup remains synchronized while a line is hidden");
    click(m_oscCluster->m_powerHistoryScope);
    check(m_oscCluster->m_showPower, "power card restores power");
    click(m_oscCluster->m_rpmScope);
    click(m_oscCluster->m_torqueHistoryScope);
    click(m_oscCluster->m_powerHistoryScope);
    check(!m_oscCluster->m_showRpm && !m_oscCluster->m_showTorque && !m_oscCluster->m_showPower,
        "all three channels can be independently hidden");
    if (!advanceTo(6.4)) return false; // Exercise empty selection rendering.
    click(m_oscCluster->m_rpmScope);
    click(m_oscCluster->m_torqueHistoryScope);
    click(m_oscCluster->m_powerHistoryScope);
    click(m_oscCluster->getAudioWaveformOscilloscope());
    check(!m_oscCluster->m_historyMode, "original waveform keeps its own axes");
    if (!advanceTo(6.6)) return false;
    click(m_oscCluster->m_rpmScope);
    check(m_oscCluster->m_historyMode && m_oscCluster->m_showRpm
        && m_oscCluster->m_showTorque && m_oscCluster->m_showPower,
        "returning from original graph preserves the selected history lines");

    Bounds cursorBody = m_oscCluster->getFocusBounds();
    cursorBody.m1.y -= 39.0f;
    const Bounds cursorPlot = m_oscCluster->getHistoryPlotBounds(cursorBody);
    m_oscCluster->m_validationMousePosition = Point(
        cursorPlot.m0.x + cursorPlot.width() * 3.0f / 30.0f, cursorPlot.center_v());
    if (!advanceTo(6.8)) return false;
    check(m_oscCluster->m_cursorActive && m_oscCluster->m_cursorReading.hasRpm
        && m_oscCluster->m_cursorReading.hasTorque && m_oscCluster->m_cursorReading.hasPower
        && std::abs(m_oscCluster->m_cursorReading.time - 3.0) < HistoryClock::SamplePeriod,
        "rendered hover cursor maps plot coordinates to a common historical sample and draws all markers");
    m_oscCluster->m_validationMousePosition = Point(cursorPlot.m0.x - 10, cursorPlot.center_v());
    if (!advanceTo(6.9)) return false;
    check(!m_oscCluster->m_cursorActive, "leaving plot clears hover mode and restores live legends");
    m_oscCluster->m_validationMousePosition.reset();

    const int frequency = static_cast<int>(m_simulator->getSimulationFrequency());
    m_simulator->setSimulationFrequency(7500);
    if (!advanceTo(7.0)) return false;
    latest(m_oscCluster->m_rpmScope, rpm);
    latest(m_oscCluster->m_torqueHistoryScope, torque);
    latest(m_oscCluster->m_powerHistoryScope, power);
    check(rpm.x == torque.x && torque.x == power.x && rpm.x >= 7.0 - HistoryClock::SamplePeriod,
        "changing physics frequency preserves the shared simulation timeline");
    m_simulator->setSimulationFrequency(frequency);
    auto *window = m_engine.GetGameWindow();
    const int width = window->GetWidth();
    const int height = window->GetHeight();
    window->SetWindowSize(1400, 900);
    if (!advanceTo(7.2)) return false;
    check(m_oscCluster->m_bounds.width() == window->GetGameWidth()
        && m_engineView->m_bounds.m0.y >= m_oscCluster->m_bounds.m1.y
        && m_rightGaugeCluster->m_bounds.m0.y >= m_oscCluster->m_bounds.m1.y
        && m_oscCluster->getFocusBounds().width() > 460,
        "1400x900 renders full-width history without overlap of top-level panels");
    window->SetWindowSize(width, height);

    for (auto &channel : m_oscCluster->m_historyChannels) {
        if (!channel.isSelected()) m_oscCluster->toggleHistoryChannel(channel.id);
    }
    if (!advanceTo(7.4)) return false;
    int allSelected = 0;
    for (const auto &channel : m_oscCluster->m_historyChannels) if (channel.isSelected()) ++allSelected;
    check(allSelected == 33, "all 33 channels can be selected together without a three-line limit");
    m_oscCluster->m_scalePage = 0;
    m_oscCluster->onMouseScroll(-1);
    if (!advanceTo(7.6)) return false;
    check(m_oscCluster->m_scalePage == 1 && allSelected == 33,
        "scroll changes the visible scale page without hiding selected lines");

    if (!advanceTo(34.0)) return false;
    bool bounded = true;
    for (auto *scope : { m_oscCluster->m_rpmScope, m_oscCluster->m_torqueHistoryScope, m_oscCluster->m_powerHistoryScope }) {
        Oscilloscope::DataPoint first, last;
        bounded = bounded && scope->getPointCount() > 1400 && scope->getPointCount() <= 1501
            && scope->getDataPoint(0, first) && latest(scope, last)
            && first.x >= scope->m_xMin && last.x <= scope->m_xMax
            && std::abs(scope->m_xMin - m_oscCluster->m_rpmScope->m_xMin) < 1e-9;
    }
    check(bounded, "after 34 simulated seconds all buffers retain only the shared 30s window, <=1501 points");
    const auto wrappedCursor = m_oscCluster->getHistoryCursor(33.0);
    check(wrappedCursor.hasRpm && wrappedCursor.hasTorque && wrappedCursor.hasPower
        && wrappedCursor.time == wrappedCursor.torque.x && wrappedCursor.time == wrappedCursor.power.x,
        "cursor finds synchronized samples after ring buffers wrap");
    check(!m_oscCluster->getHistoryCursor(2.0).hasRpm,
        "cursor does not recover expired samples outside the retained window");
    bool channelHistory = true;
    int availableChannels = 0;
    for (const auto &channel : m_oscCluster->m_historyChannels) {
        if (channel.available) {
            ++availableChannels;
            Oscilloscope::DataPoint point;
            channelHistory = channelHistory && channel.scope->findNearestPoint(wrappedCursor.time, 1e-9, point)
                && std::isfinite(point.y) && channel.scope->getPointCount() <= 1501;
        }
    }
    log << "AVAILABLE CHANNELS: " << availableChannels << " / 33\n";
    check(channelHistory && availableChannels >= 30,
        "all currently available instrument channels have bounded synchronized cursor samples");
    m_audioSource->SetMode(ysAudioSource::Mode::Stop);
    loadScript(); // Same loading path as Enter, not just a mocked buffer reset.
    check(empty(), "real script reload clears all histories and their clock");
    bool allEmpty = true;
    for (const auto &channel : m_oscCluster->m_historyChannels) allEmpty = allEmpty && channel.scope->getPointCount() == 0;
    check(allEmpty && m_oscCluster->m_historyChannels.size() == 33,
        "reload recreates 33 channels without stale buffers or duplicate registrations");
    check(!m_oscCluster->m_torqueAvailable, "reload clears dyno measurement availability");
    check(!m_oscCluster->m_cursorActive && !m_oscCluster->getHistoryCursor(33.0).hasRpm,
        "reload clears cursor and prevents values from the previous run");
    if (!advanceTo(0.1)) return false;
    log << (passed ? "RESULT: PASS\n" : "RESULT: FAIL\n");
    return passed;
}
