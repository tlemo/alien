#include <QScrollBar>
#include <QTimer>
#include <QGraphicsItem>
#include <QGraphicsBlurEffect>
#include <QFile>
#include <QTextStream>

#include "Gui/Settings.h"
#include "EngineInterface/SimulationAccess.h"
#include "EngineInterface/SimulationController.h"
#include "EngineInterface/SimulationContext.h"
#include "EngineInterface/SpaceProperties.h"

#include "PixelUniverseView.h"
#include "VectorUniverseView.h"
#include "ItemUniverseView.h"
#include "QApplicationHelper.h"
#include "StartupController.h"

#include "SimulationViewWidget.h"
#include "ui_SimulationViewWidget.h"


SimulationViewWidget::SimulationViewWidget(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::SimulationViewWidget)
{
    ui->setupUi(this);

    ui->simulationView->horizontalScrollBar()->setStyleSheet(Const::ScrollbarStyleSheet);
    ui->simulationView->verticalScrollBar()->setStyleSheet(Const::ScrollbarStyleSheet);

    auto startupScene = new QGraphicsScene(this);
    startupScene->setBackgroundBrush(QBrush(Const::UniverseColor));
    ui->simulationView->setScene(startupScene);
    ui->simulationView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->simulationView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->simulationView->setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
}

SimulationViewWidget::~SimulationViewWidget()
{
    delete ui;
}

void SimulationViewWidget::resize(IntVector2D const& sceneSize)
{
    auto viewSize = getViewSize();
    ui->horizontalScrollBar->setRange(0, sceneSize.x - viewSize.x);
    ui->horizontalScrollBar->setPageStep(viewSize.x);
    ui->verticalScrollBar->setRange(0, sceneSize.y - viewSize.y);
    ui->verticalScrollBar->setPageStep(viewSize.y);
}

IntVector2D SimulationViewWidget::getViewSize() const
{
    return {ui->simulationView->width(), ui->simulationView->height()};
}

void SimulationViewWidget::setScene(QGraphicsScene* scene)
{
    ui->simulationView->setScene(scene);
    ui->simulationView->resetTransform();
}

int SimulationViewWidget::getHorizontalScrollPosition() const
{
    return ui->horizontalScrollBar->sliderPosition();
}

void SimulationViewWidget::setHorizontalScrollPosition(int pos)
{
    ui->horizontalScrollBar->setSliderPosition(pos);
}

int SimulationViewWidget::getVerticalScrollPosition() const
{
    return ui->verticalScrollBar->sliderPosition();
}

void SimulationViewWidget::setVerticalScrollPosition(int pos)
{
    ui->verticalScrollBar->setSliderPosition(pos);
}
