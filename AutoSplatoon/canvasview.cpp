#include "canvasview.h"
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <QMouseEvent>
#include <algorithm>

CanvasView::CanvasView(bool edit, QWidget* parent) : QGraphicsView(parent), editable(edit)
{
    setScene(&scene);
    scene.setSceneRect(0, 0, 320, 120);
    scene.setBackgroundBrush(Qt::white);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAlignment(Qt::AlignCenter);
    setMinimumSize(240, 64);
    setRenderHint(QPainter::SmoothPixmapTransform, editable);
}
void CanvasView::drawBackground(QPainter* painter, const QRectF& rect)
{
    painter->fillRect(rect, palette().brush(QPalette::Mid));
    painter->fillRect(scene.sceneRect(), Qt::white);
}
void CanvasView::drawForeground(QPainter* painter, const QRectF& rect)
{
    // Keep dragged images outside the drawing area hidden; this is a preview overlay only.
    QPainterPath outside, canvas;
    outside.addRect(rect);
    canvas.addRect(scene.sceneRect());
    painter->fillPath(outside.subtracted(canvas), palette().brush(QPalette::Mid));
    QPen border(QColor("#6c83b5"), 2);
    border.setCosmetic(true);
    painter->setPen(border);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(scene.sceneRect());
}
void CanvasView::setImage(const QImage& image)
{
    original = image;
    scene.clear();
    item = scene.addPixmap(QPixmap::fromImage(image));
    item->setFlag(QGraphicsItem::ItemIsMovable, editable);
    resetComposition();
}
void CanvasView::resetComposition()
{
    if (!item || original.isNull()) return;
    double scale = std::min(320. / original.width(), 120. / original.height());
    item->setScale(scale);
    item->setPos((320 - original.width() * scale) / 2, (120 - original.height() * scale) / 2);
    fitInView(scene.sceneRect(), Qt::KeepAspectRatio);
    if (editable) emit compositionChanged();
}
QImage CanvasView::composition() const
{
    QImage canvas(1280, 480, QImage::Format_RGB32);
    canvas.fill(Qt::white);
    if (item && !original.isNull()) {
        QPainter painter(&canvas);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.scale(4, 4);
        painter.setTransform(item->sceneTransform(), true);
        painter.drawImage(QPointF(0, 0), original);
    }
    return canvas;
}
void CanvasView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    fitInView(scene.sceneRect(), Qt::KeepAspectRatio);
}
void CanvasView::wheelEvent(QWheelEvent* event)
{
    if (!editable || !item) { event->ignore(); return; }
    const QPointF before = item->mapFromScene(mapToScene(event->position().toPoint()));
    const double scale = std::clamp(item->scale() * (event->angleDelta().y() > 0 ? 1.1 : 1 / 1.1),
                                   .001, 100.);
    item->setScale(scale);
    const QPointF after = item->mapToScene(before);
    item->setPos(item->pos() + mapToScene(event->position().toPoint()) - after);
    emit compositionChanged();
    event->accept();
}
void CanvasView::mouseReleaseEvent(QMouseEvent* event)
{
    QGraphicsView::mouseReleaseEvent(event);
    if (editable && item) emit compositionChanged();
}
