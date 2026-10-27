#include "../include/engine_sim_application.h"
#include "../include/oscilloscope_cluster.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

void EngineSimApplication::bindHistory(UiElement *element, const std::string &id,
    const std::string &label, const std::string &unit, std::function<double()> source, double low, double high)
{
    m_oscCluster->registerHistoryChannel(id, label, unit, source, low, high);
    element->bindHistory(id);
}

void EngineSimApplication::toggleHistory(const std::string &id) { m_oscCluster->toggleHistoryChannel(id); }
bool EngineSimApplication::isHistorySelected(const std::string &id) const {
    return m_oscCluster != nullptr && m_oscCluster->isHistoryChannelSelected(id);
}
ysVector EngineSimApplication::getHistoryColor(const std::string &id) const {
    return m_oscCluster->historyChannelColor(id);
}

void OscilloscopeCluster::registerHistoryChannel(const std::string &id, const std::string &label,
    const std::string &unit, std::function<double()> source, double low, double high)
{
    for (const auto &channel : m_historyChannels) if (channel.id == id) return;
    HistoryChannel entry;
    entry.id = id; entry.label = label; entry.unit = unit; entry.source = source;
    entry.initialLow = low; entry.initialHigh = high > low ? high : low + 1;
    entry.scope = addElement<Oscilloscope>();
    entry.scope->setVisible(false); // Only the explicitly rendered lower history, not an extra thumbnail.
    entry.scope->setBufferSize(1501);
    entry.scope->m_xMin = 0; entry.scope->m_xMax = 30;
    entry.scope->m_yMin = entry.initialLow; entry.scope->m_yMax = entry.initialHigh;
    entry.scope->m_dynamicallyResizeX = entry.scope->m_dynamicallyResizeY = false;
    entry.scope->m_drawReverse = entry.scope->m_drawZero = false;
    entry.scope->m_maxRenderPoints = 600;
    entry.scope->m_lineWidth = 2;
    // Deterministic golden-angle palette; no per-frame or per-engine random colours.
    const double h = std::fmod((m_historyChannels.size() - 3) * 0.61803398875 + 0.52, 1.0) * 6;
    const float x = static_cast<float>(0.85 * (1 - std::abs(std::fmod(h, 2.0) - 1)));
    const int sector = static_cast<int>(h);
    const float rgb[6][3] = {{0.85f,x,0},{x,0.85f,0},{0,0.85f,x},{0,x,0.85f},{x,0,0.85f},{0.85f,0,x}};
    entry.scope->i_color = ysMath::LoadVector(rgb[sector][0] + 0.15f, rgb[sector][1] + 0.15f,
        rgb[sector][2] + 0.15f, 1.0f);
    m_historyChannels.push_back(entry);
}

bool OscilloscopeCluster::isHistoryChannelSelected(const std::string &id) const {
    for (const auto &channel : m_historyChannels) if (channel.id == id) return channel.isSelected();
    return false;
}
ysVector OscilloscopeCluster::historyChannelColor(const std::string &id) const {
    for (const auto &channel : m_historyChannels) if (channel.id == id) return channel.scope->i_color;
    return m_app->getForegroundColor();
}
void OscilloscopeCluster::toggleHistoryChannel(const std::string &id) {
    int selected = 0;
    for (auto &channel : m_historyChannels) {
        if (channel.id == id) {
            channel.toggle();
            if (channel.isSelected()) m_scalePage = selected / 3;
        }
        if (channel.isSelected()) ++selected;
    }
    m_historyMode = true;
}
void OscilloscopeCluster::onMouseScroll(int delta) {
    if (m_historyMode && delta != 0) m_scalePage += delta > 0 ? -1 : 1;
}

void OscilloscopeCluster::renderChannelHistory(const Bounds &bounds) {
    std::vector<HistoryChannel *> selected;
    for (auto &channel : m_historyChannels) if (channel.isSelected()) selected.push_back(&channel);
    const int count = static_cast<int>(selected.size());
    m_scalePage = (std::max)(0, (std::min)(m_scalePage, ((std::max)(1, count) - 1) / 3));
    const Bounds plot = getHistoryPlotBounds(bounds);
    int mouseX = 0, mouseY = 0;
    m_app->getEngine()->GetOsMousePos(&mouseX, &mouseY);
    const Point mouse = m_validationMousePosition.value_or(worldToLocal({(float)mouseX, (float)mouseY}));
    m_cursorActive = count > 0 && plot.width() > 0 && plot.height() > 0 && plot.overlaps(mouse)
        && (m_validationMousePosition.has_value() || m_app->getEngine()->GetGameWindow()->IsActive());
    if (m_cursorActive) {
        m_cursorReading = getHistoryCursor(m_rpmScope->m_xMin
            + (mouse.x - plot.m0.x) / plot.width() * 30.0);
    }
    auto textColor = [&](const ysVector &color) {
        m_app->getTextRenderer()->SetColor(ysColor::linearToSrgb(color));
    };
    for (int i = 0; i < count; ++i) {
        auto &channel = *selected[i];
        auto *scope = channel.scope;
        scope->render(plot);
        Oscilloscope::DataPoint point {};
        bool available = channel.available;
        double value = channel.latest;
        if (m_cursorActive) {
            available = m_cursorReading.hasRpm && scope->findNearestPoint(m_cursorReading.time, 1e-9, point);
            if (available) value = point.y;
        }
        if (i / 3 == m_scalePage) {
            const int slot = i % 3;
            const float labelX = slot == 0 ? bounds.m0.x + 8 : plot.m1.x + 12 + (slot - 1) * 150;
            const float axisX = slot == 0 ? plot.m0.x : plot.m1.x + (slot - 1) * 150;
            textColor(scope->i_color);
            Bounds label = bounds.inset(8);
            label.m0.x = labelX;
            const std::string name = channel.label.substr(0, 19);
            drawText(name, label, 12, Bounds::tl);
            label.m1.y -= 17;
            std::ostringstream reading;
            if (available) reading << std::fixed << std::setprecision(1) << value;
            else reading << "N/A";
            reading << " " << channel.unit;
            drawText(reading.str(), label, 12, Bounds::tl);
            for (int tick = 0; tick <= 4; ++tick) {
                const double y = scope->m_yMin + (scope->m_yMax - scope->m_yMin) * tick / 4;
                label.m1.y = plot.m0.y + plot.height() * tick / 4 + 7;
                std::ostringstream number;
                number << std::setprecision(4) << y;
                drawText(number.str(), label, 11, Bounds::tl);
            }
            drawBox(Bounds(axisX, axisX + 1, plot.m0.y, plot.m1.y), scope->i_color);
        }
        if (m_cursorActive && available) {
            const float x = plot.m0.x + static_cast<float>((m_cursorReading.time - scope->m_xMin) / 30) * plot.width();
            const float y = plot.m0.y + static_cast<float>((value - scope->m_yMin) / (scope->m_yMax - scope->m_yMin)) * plot.height();
            drawBox(Bounds(std::fmax(plot.m0.x,x-3), std::fmin(plot.m1.x,x+3),
                std::fmax(plot.m0.y,y-3), std::fmin(plot.m1.y,y+3)), scope->i_color);
        }
    }
    textColor(m_app->getForegroundColor());
    if (m_cursorActive) {
        const float x = plot.m0.x + static_cast<float>((m_cursorReading.time - m_rpmScope->m_xMin) / 30) * plot.width();
        drawBox(Bounds(x, x+1, plot.m0.y, plot.m1.y), m_app->getForegroundColor());
    }
    for (int tick = 0; tick <= 3; ++tick) {
        Bounds label = bounds;
        label.m0.x = plot.m0.x + plot.width() * tick / 3 - (tick == 3 ? 45 : 0);
        label.m1.y = bounds.m0.y + 17;
        std::ostringstream text;
        text << std::fixed << std::setprecision(1) << m_rpmScope->m_xMin + tick * 10 << "s";
        drawText(text.str(), label, 12, Bounds::tl);
    }
    if (count == 0) drawText("Click an instrument to add its history", plot.inset(8), 14, Bounds::tl);
    else if (m_cursorActive && !m_cursorReading.hasRpm)
        drawText("No recorded sample at this time", plot.inset(8), 14, Bounds::tl);
}
