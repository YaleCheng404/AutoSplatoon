#pragma once
#include <QGraphicsView>
#include <QGraphicsPixmapItem>

class CanvasView : public QGraphicsView {
    Q_OBJECT
public:
    explicit CanvasView(bool editable, QWidget* parent = nullptr);
    void setImage(const QImage& image);
    void resetComposition();
    QImage composition() const;
signals:
    void compositionChanged();
protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void drawForeground(QPainter* painter, const QRectF& rect) override;
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
private:
    bool editable;
    QGraphicsScene scene;
    QGraphicsPixmapItem* item = nullptr;
    QImage original;
};
