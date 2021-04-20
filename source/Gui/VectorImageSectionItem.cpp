#include <QPainter>

#include "VectorImageSectionItem.h"
#include "ViewportInterface.h"

VectorImageSectionItem::VectorImageSectionItem(IntVector2D imageSize, std::mutex& mutex)
    : QGraphicsItem(), _mutex(mutex)
{
    _image = boost::make_shared<QImage>(imageSize.x, imageSize.y, QImage::Format_ARGB32);
}

VectorImageSectionItem::~VectorImageSectionItem()
{
}

QImagePtr VectorImageSectionItem::getImageOfVisibleRect()
{
    return _image;
}

QRectF VectorImageSectionItem::boundingRect() const
{
    return QRectF(0, 0, _image->width() - 1, _image->height() - 1);
}

void VectorImageSectionItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget /*= Q_NULLPTR*/)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_image) {
        painter->drawImage(0, 0, *_image);
    }
}

