#pragma once

#include <QWidget>

#include <QVector2D>

#include "Definitions.h"
#include "EngineInterface/Definitions.h"

class SimulationViewController: public QObject
{
    Q_OBJECT
public:
    SimulationViewController(QWidget* parent = nullptr);
    virtual ~SimulationViewController();

    QWidget* getWidget() const;

    void
    init(Notifier* notifier, SimulationController* controller, SimulationAccess* access, DataRepository* repository);

    void connectView();
    void disconnectView();
    void refresh();

    ActiveView getActiveView() const;
    void setActiveScene(ActiveView activeScene);

    double getZoomFactor();
    void setZoomFactor(double factor);

    QVector2D getViewCenterWithIncrement();

    void toggleCenterSelection(bool value);

    Q_SIGNAL void zoomFactorChanged(double factor);

private:
    SimulationController* _controller = nullptr;

    SimulationViewWidget* _simulationViewWidget = nullptr;

    PixelUniverseView* _pixelUniverse = nullptr;
    VectorUniverseView* _vectorUniverse = nullptr;
    ItemUniverseView* _itemUniverse = nullptr;

    qreal _posIncrement = 0.0;
};
