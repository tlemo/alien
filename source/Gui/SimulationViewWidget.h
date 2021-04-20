#pragma once

#include <QWidget>
#include <QVector2D>

#include "EngineInterface/Definitions.h"
#include "Definitions.h"

namespace Ui {
	class SimulationViewWidget;
}

class SimulationViewWidget : public QWidget
{
    Q_OBJECT
public:
    SimulationViewWidget(QWidget *parent = nullptr);
    virtual ~SimulationViewWidget();

    void resize(IntVector2D const& sceneSize);

    IntVector2D getViewSize() const;
    void setScene(QGraphicsScene* scene);

    int getHorizontalScrollPosition() const;
    void setHorizontalScrollPosition(int pos);
    int getVerticalScrollPosition() const;
    void setVerticalScrollPosition(int pos);

private:
    Ui::SimulationViewWidget *ui;
};





