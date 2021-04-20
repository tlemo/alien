#include "SimulationViewController.h"
#include "SimulationViewWidget.h"
#include "PixelUniverseView.h"
#include "VectorUniverseView.h"
#include "ItemUniverseView.h"

SimulationViewController::SimulationViewController(QWidget* parent)
    : QObject(parent)
{
    _simulationViewWidget = new SimulationViewWidget(parent);
    //    _pixelUniverse = new PixelUniverseView(_simulationViewWidget, this);
    _vectorUniverse = new VectorUniverseView(_simulationViewWidget, this);
//    _itemUniverse = new ItemUniverseView(_simulationViewWidget, this);
}

SimulationViewController::~SimulationViewController() {

}

QWidget* SimulationViewController::getWidget() const
{
    return _simulationViewWidget;
}

void SimulationViewController::init(
    Notifier* notifier,
    SimulationController* controller,
    SimulationAccess* access,
    DataRepository* repository)
{
    auto const InitialZoomFactor = 4.0;

    _controller = controller;

//    _pixelUniverse->init(notifier, controller, access, repository);
    _vectorUniverse->init(notifier, controller, access, repository);
//     _itemUniverse->init(notifier, controller, repository);

    _vectorUniverse->activate(InitialZoomFactor);

    _vectorUniverse->connectView();
    _vectorUniverse->refresh();

    Q_EMIT zoomFactorChanged(InitialZoomFactor);
}

void SimulationViewController::connectView()
{
    _vectorUniverse->connectView();
}

void SimulationViewController::disconnectView()
{
    _vectorUniverse->disconnectView();
}

void SimulationViewController::refresh()
{
    _vectorUniverse->refresh();
}

ActiveView SimulationViewController::getActiveView() const
{
    return ActiveView::VectorScene;
}

void SimulationViewController::setActiveScene(ActiveView activeScene)
{
    _vectorUniverse->activate(getZoomFactor());
}

double SimulationViewController::getZoomFactor()
{
    return _vectorUniverse->getZoomFactor();
}

void SimulationViewController::setZoomFactor(double factor)
{
    auto centerPos = _vectorUniverse->getCenterPositionOfScreen();
    _vectorUniverse->setZoomFactor(factor);
    _vectorUniverse->centerTo(centerPos);

     Q_EMIT zoomFactorChanged(factor);
}

QVector2D SimulationViewController::getViewCenterWithIncrement()
{
    return QVector2D();
}

void SimulationViewController::toggleCenterSelection(bool value) {}
