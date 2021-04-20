#pragma once

#include <QGraphicsScene>
#include <QVector2D>
#include <QTimer>

#include "EngineInterface/Definitions.h"
#include "EngineInterface/Descriptions.h"
#include "UniverseView.h"
#include "Definitions.h"

class VectorUniverseView : public UniverseView
{
    Q_OBJECT
public:
    VectorUniverseView(SimulationViewWidget* simulationViewWidget, QObject* parent = nullptr);
    virtual ~VectorUniverseView() = default;

    virtual void init(
        Notifier* notifier,
        SimulationController* controller,
        SimulationAccess* access,
        DataRepository* repository);

    void connectView() override;
    void disconnectView() override;
    void refresh() override;

    bool isActivated() const override;
    void activate(double zoomFactor) override;

    double getZoomFactor() const override;
    void setZoomFactor(double zoomFactor) override;

    std::pair<double, double> getCenterPositionOfScreen() const override;

    void centerTo(std::pair<double, double> const& position) override;

protected:
    bool eventFilter(QObject* object, QEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent* e);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
    Q_SLOT void receivedNotifications(set<Receiver> const& targets);
    Q_SLOT void requestImage();
    Q_SLOT void imageReady();
    Q_SLOT void scrolled();

    list<QMetaObject::Connection> _connections;

    SimulationViewWidget* _simulationViewWidget = nullptr;
    QGraphicsScene* _scene = nullptr;

    SimulationAccess* _access = nullptr;
    DataRepository* _repository = nullptr;
    SimulationController* _controller = nullptr;

    VectorImageSectionItem* _item = nullptr;

    Notifier* _notifier = nullptr;
    double _zoomFactor = 0.0;
};

