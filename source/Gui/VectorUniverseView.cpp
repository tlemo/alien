#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>
#include <QScrollBar>
#include <QtCore/qmath.h>
#include <QMatrix4x4>

#include "Base/ServiceLocator.h"
#include "EngineInterface/PhysicalActions.h"
#include "EngineInterface/EngineInterfaceBuilderFacade.h"
#include "EngineInterface/SimulationAccess.h"
#include "EngineInterface/SimulationController.h"
#include "EngineInterface/SimulationContext.h"
#include "EngineInterface/SpaceProperties.h"
#include "Gui/ViewportInterface.h"
#include "Gui/Settings.h"
#include "Gui/Notifier.h"

#include "CoordinateSystem.h"
#include "DataRepository.h"
#include "VectorImageSectionItem.h"
#include "VectorUniverseView.h"
#include "VectorViewport.h"
#include "SimulationViewWidget.h"

VectorUniverseView::VectorUniverseView(SimulationViewWidget* simulationViewWidget, QObject* parent)
    : UniverseView(parent)
    , _simulationViewWidget(simulationViewWidget)
{
    _scene = new QGraphicsScene(parent);
    _scene->setBackgroundBrush(QBrush(Const::BackgroundColor));
    _scene->update();
    _scene->installEventFilter(this);
}

void VectorUniverseView::init(
    Notifier* notifier,
    SimulationController* controller,
    SimulationAccess* access,
    DataRepository* repository)
{
    disconnectView();
    _controller = controller;
    _repository = repository;
    _notifier = notifier;

    SET_CHILD(_access, access);
}

void VectorUniverseView::connectView()
{
    disconnectView();
    _connections.push_back(connect(_controller, &SimulationController::nextFrameCalculated, this, &VectorUniverseView::requestImage));
    _connections.push_back(connect(_notifier, &Notifier::notifyDataRepositoryChanged, this, &VectorUniverseView::receivedNotifications));
    _connections.push_back(connect(_repository, &DataRepository::imageReady, this, &VectorUniverseView::imageReady, Qt::QueuedConnection));
/*
    _connections.push_back(connect(_graphicsView->horizontalScrollBar(), &QScrollBar::valueChanged, this, &VectorUniverseView::scrolled));
    _connections.push_back(connect(_graphicsView->verticalScrollBar(), &QScrollBar::valueChanged, this, &VectorUniverseView::scrolled));
*/
}

void VectorUniverseView::disconnectView()
{
    for (auto const& connection : _connections) {
        disconnect(connection);
    }
    _connections.clear();
}

void VectorUniverseView::refresh()
{
    requestImage();
}

bool VectorUniverseView::isActivated() const
{
    return true/*_graphicsView->scene() == _scene*/;
}

void VectorUniverseView::activate(double zoomFactor)
{
    delete _item;

    auto worldSize = _controller->getContext()->getSpaceProperties()->getSize();
   _simulationViewWidget->resize({static_cast<int>(worldSize.x * zoomFactor), static_cast<int>(worldSize.y * zoomFactor)});

    auto viewSize = _simulationViewWidget->getViewSize();
/*
   IntVector2D minSize{
        std::min(viewSize.x, static_cast<int>(worldSize.x * zoomFactor)), 
        std::min(viewSize.y, static_cast<int>(worldSize.y * zoomFactor))};
*/

    _item = new VectorImageSectionItem(viewSize, _repository->getImageMutex());
    _scene->addItem(_item);

    _simulationViewWidget->setScene(_scene);
    setZoomFactor(zoomFactor);
}

double VectorUniverseView::getZoomFactor() const
{
    return _zoomFactor;
}

void VectorUniverseView::setZoomFactor(double zoomFactor)
{
    _zoomFactor = zoomFactor;
    auto worldSize = _controller->getContext()->getSpaceProperties()->getSize();
    _simulationViewWidget->resize(
        {static_cast<int>(worldSize.x * zoomFactor), static_cast<int>(worldSize.y * zoomFactor)});
    
//    auto const size = _controller->getContext()->getSpaceProperties()->getSize();
//    _scene->setSceneRect(0, 0, size.x * zoomFactor, size.y * zoomFactor);
//    _viewport->setZoomFactor(zoomFactor);
}

std::pair<double, double> VectorUniverseView::getCenterPositionOfScreen() const
{
    auto viewSize = _simulationViewWidget->getViewSize();

    auto x = (static_cast<double>(_simulationViewWidget->getHorizontalScrollPosition()) + static_cast<double>(viewSize.x) / 2.0) / _zoomFactor;
    auto y = (static_cast<double>(_simulationViewWidget->getVerticalScrollPosition()) + static_cast<double>(viewSize.y) / 2.0) / _zoomFactor;
    return std::make_pair(x, y);
    /*
    auto const width = static_cast<double>(_graphicsView->width());
    auto const height = static_cast<double>(_graphicsView->height());
    auto const result = _graphicsView->mapToScene(width / 2.0, height / 2.0);
    return{ static_cast<float>(result.x() / _zoomFactor), static_cast<float>(result.y() / _zoomFactor)};
*/
}

void VectorUniverseView::centerTo(std::pair<double, double> const& position)
{
    auto viewSize = _simulationViewWidget->getViewSize();

    _simulationViewWidget->setHorizontalScrollPosition(static_cast<int>(position.first * _zoomFactor - static_cast<double>(viewSize.x) / 2.0));
    _simulationViewWidget->setVerticalScrollPosition(static_cast<int>(position.second * _zoomFactor - static_cast<double>(viewSize.y) / 2.0));
    /*
    _centerPosition = position;
    _graphicsView->centerOn(position.x() * _zoomFactor, position.y() * _zoomFactor);
*/
}

bool VectorUniverseView::eventFilter(QObject * object, QEvent * event)
{
    if (object != _scene) {
        return false;
    }

    if (event->type() == QEvent::GraphicsSceneMousePress) {
        mousePressEvent(static_cast<QGraphicsSceneMouseEvent*>(event));
    }

    if (event->type() == QEvent::GraphicsSceneMouseMove) {
        mouseMoveEvent(static_cast<QGraphicsSceneMouseEvent*>(event));
    }

    if (event->type() == QEvent::GraphicsSceneMouseRelease) {
        mouseReleaseEvent(static_cast<QGraphicsSceneMouseEvent*>(event));
    }

    return false;
}

void VectorUniverseView::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    if (!_controller->getRun()) {
        QVector2D pos(event->scenePos().x() / _zoomFactor, event->scenePos().y() / _zoomFactor);
        _access->selectEntities(pos);
        requestImage();
    }
}

void VectorUniverseView::mouseMoveEvent(QGraphicsSceneMouseEvent * e)
{
    auto const pos = QVector2D(e->scenePos().x() / _zoomFactor, e->scenePos().y() / _zoomFactor);
    auto const lastPos = QVector2D(e->lastScenePos().x() / _zoomFactor, e->lastScenePos().y() / _zoomFactor);

    if (_controller->getRun()) {
        if (e->buttons() == Qt::MouseButton::LeftButton) {
            auto const force = (pos - lastPos) / 10;
            _access->applyAction(boost::make_shared<_ApplyForceAction>(lastPos, pos, force));
        }
        if (e->buttons() == Qt::MouseButton::RightButton) {
            auto const force = (pos - lastPos) / 10;
            _access->applyAction(boost::make_shared<_ApplyRotationAction>(lastPos, pos, force));
        }
    }
    else {
        if (e->buttons() == Qt::MouseButton::LeftButton) {
            auto const displacement = pos - lastPos;
            _access->applyAction(boost::make_shared<_MoveSelectionAction>(displacement));
            requestImage();
        }
    }
}

void VectorUniverseView::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    if (!_controller->getRun()) {
        _access->deselectAll();
        requestImage();
    }
}

void VectorUniverseView::receivedNotifications(set<Receiver> const & targets)
{
    if (targets.find(Receiver::VisualEditor) == targets.end()) {
        return;
    }

    requestImage();
}

void VectorUniverseView::requestImage()
{
    if (!_connections.empty()) {
        auto viewSize = _simulationViewWidget->getViewSize();
        auto left = static_cast<float>(_simulationViewWidget->getHorizontalScrollPosition() / _zoomFactor);
        auto top = static_cast<float>(_simulationViewWidget->getVerticalScrollPosition() / _zoomFactor);
        _repository->requireVectorImageFromSimulation(
            RealRect{
                {left, top},
                {left + static_cast<float>(viewSize.x / _zoomFactor),
                 top + static_cast<float>(viewSize.y / _zoomFactor)}},
            getZoomFactor(),
            _item->getImageOfVisibleRect());
    }
}

void VectorUniverseView::imageReady()
{
    _scene->update();
}

void VectorUniverseView::scrolled()
{
    requestImage();
}


