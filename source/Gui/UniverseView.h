#pragma once

#include <QObject>

class UniverseView : public QObject
{
    Q_OBJECT
public:
    UniverseView(QObject* parent = nullptr);
    virtual ~UniverseView() = default;

    virtual void connectView() = 0;
    virtual void disconnectView() = 0;
    virtual void refresh() = 0;

    virtual bool isActivated() const = 0;
    virtual void activate(double zoomFactor) = 0;

    virtual double getZoomFactor() const = 0;
    virtual void setZoomFactor(double zoomFactor) = 0;

    virtual std::pair<double, double> getCenterPositionOfScreen() const = 0;
    virtual void centerTo(std::pair<double, double> const& position) = 0;
};