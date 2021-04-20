#pragma once

#include "ViewportInterface.h"

class VectorViewport : public ViewportInterface
{
    Q_OBJECT
public:
    VectorViewport(SimulationViewWidget* simulationViewWidget, QObject* parent = nullptr);
    virtual ~VectorViewport() = default;

    QRectF getRect() const override;


private:
    SimulationViewWidget* _simulationViewWidget;
    double _zoomFactor = 0.0;
};