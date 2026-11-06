#include "samplelibrary.h"

#include <QCoreApplication>
#include <QPainter>
#include <QSvgRenderer>

namespace {
bool place(QPainter& painter, const QString& name, const QRectF& bounds)
{
    QSvgRenderer svg(QStringLiteral(":/art/") + name + QStringLiteral(".svg"));
    if (!svg.isValid()) return false;
    QSizeF size = svg.defaultSize();
    size.scale(bounds.size(), Qt::KeepAspectRatio);
    svg.render(&painter, QRectF(bounds.center() - QPointF(size.width(), size.height()) / 2, size));
    return true;
}

QImage compose(std::initializer_list<std::pair<QString, QRectF>> pieces)
{
    QImage image(1280, 480, QImage::Format_RGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    bool valid = true;
    for (const auto& [name, bounds] : pieces)
        valid = place(painter, name, bounds) && valid;
    painter.end();
    return valid ? image : QImage{};
}
}

const std::array<SampleArt, 3>& sampleLibrary()
{
    Q_INIT_RESOURCE(resources);
    static const std::array<SampleArt, 3> samples{{
        {"squid-party", QCoreApplication::translate("Samples", "鱿鱼集合"),
         QCoreApplication::translate("Samples", "轮廓与大色块"), "Mzyy94 / Slidoon",
         compose({{"squid-yellow", {52, 34, 378, 412}}, {"squid-purple", {451, 34, 378, 412}},
                  {"squid-blue", {850, 34, 378, 412}}})},
        {"ink-party", QCoreApplication::translate("Samples", "墨迹派对"),
         QCoreApplication::translate("Samples", "飞溅墨点与重叠边缘"), "Mzyy94 / Slidoon, 3070260 / oksmith",
         compose({{"splatter-purple", {75, 15, 460, 450}}, {"ink-splash", {742, 21, 460, 438}},
                  {"squid-yellow", {482, 51, 316, 375}}, {"squid-blue", {904, 175, 191, 222}}})},
        {"controller-lines", QCoreApplication::translate("Samples", "手柄线稿"),
         QCoreApplication::translate("Samples", "规则线条与按键符号"), "Zacksly",
         compose({{"pad-outline", {250, -104, 680, 680}}, {"pad-dpad", {56, 126, 194, 194}},
                  {"pad-x", {991, 62, 108, 108}}, {"pad-y", {894, 170, 108, 108}},
                  {"pad-a", {1088, 170, 108, 108}}, {"pad-b", {991, 278, 108, 108}}})}
    }};
    return samples;
}
