#pragma once

#include <QGraphicsItem>

#include "Definitions.h"

class VectorImageSectionItem
    : public QGraphicsItem
{
public:
    VectorImageSectionItem(IntVector2D imageSize, std::mutex& mutex);
    ~VectorImageSectionItem();

    QImagePtr getImageOfVisibleRect();
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;

private:
    QImagePtr _image = nullptr;
    std::mutex& _mutex;
};
