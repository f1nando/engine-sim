#ifndef ATG_ENGINE_SIM_OSCILLOSCOPE_H
#define ATG_ENGINE_SIM_OSCILLOSCOPE_H

#include "ui_element.h"
#include "history_continuity.h"
#include "history_lookup.h"

class Oscilloscope : public UiElement {
    public:
        struct DataPoint {
            double x, y;
            bool breakBefore = false;
        };

    public:
        Oscilloscope();
        virtual ~Oscilloscope();

        virtual void initialize(EngineSimApplication *app);
        virtual void destroy();

        virtual void update(float dt);
        virtual void render();
        void render(const Bounds &bounds);

        Point dataPointToRenderPosition(
            const DataPoint &p,
            const Bounds &bounds) const;

        void addDataPoint(double x, double y);
        void markGap() { m_continuity.markGap(); }

        void setBufferSize(int n);
        void reset();
        void discardBeforeX(double x);
        int getPointCount() const { return m_pointCount; }
        bool findNearestPoint(double time, double tolerance, DataPoint &point) const {
            return findNearestHistoryPoint(m_pointCount,
                [this](int index, DataPoint &p) { return getDataPoint(index, p); },
                time, tolerance, point);
        }
        bool getDataPoint(int offset, DataPoint &point) const {
            if (offset < 0 || offset >= m_pointCount) return false;
            point = m_points[(m_writeIndex - m_pointCount + offset + m_bufferSize) % m_bufferSize];
            return true;
        }

        double m_xMin;
        double m_xMax;

        double m_yMin;
        double m_yMax;

        double m_dynamicallyResizeX;
        double m_dynamicallyResizeY;

        double m_lineWidth;
        bool m_drawReverse;
        bool m_drawZero;
        int m_maxRenderPoints = 0; // 0: unchanged full-detail rendering.

        ysVector i_color;

    protected:
        DataPoint *m_points;
        Point *m_renderBuffer;
        int m_writeIndex;
        int m_bufferSize;
        int m_pointCount;
        HistoryContinuity m_continuity;
};

#endif /* ATG_ENGINE_SIM_OSCILLOSCOPE_H */
