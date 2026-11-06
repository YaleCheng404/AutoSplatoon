#pragma once

#include <QFrame>
#include <QHash>
#include <QPixmap>
#include <functional>

class QLabel;
class QVariantAnimation;

namespace WorkbenchStyle {
void apply(bool dark);
bool highContrast();
QPixmap artwork(const QString& name, QSize size, qreal dpr = 1);
QWidget* dialogHeading(const QString& title, const QString& caption, const QString& art);
}

class ArtPanel : public QFrame {
public:
    enum class Surface { Banner, Gallery };
    explicit ArtPanel(Surface surface, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    Surface surface;
    QPixmap texture;
    QPixmap ink;
};

class UiMotion : public QObject {
public:
    static UiMotion& instance();
    void animate(QWidget* target, qreal from, qreal to, int ms, std::function<void(qreal)> update);
    void reveal(QWidget* target, int ms, int lift = 0, qreal from = 0);
    void cancel(QWidget* target, bool finish = true);
    void finishAll();
    void setReduced(bool reduced);
    void setDrawing(bool drawing);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    explicit UiMotion(QObject* parent);
    bool enabled() const;
    QHash<QObject*, QVariantAnimation*> animations;
    bool reduced = false;
    bool drawing = false;
};
