#include "imageprocessor.h"
#include <QPainter>
#include <opencv2/imgproc.hpp>
#include <opencv2/photo.hpp>
#include <stdexcept>

QImage processImage(const QImage& composition, const ProcessOptions& o)
{
    if (composition.isNull()) throw std::invalid_argument("Empty image");
    // Own the RGB storage and honor QImage's padded stride.
    QImage rgb(composition.size(), QImage::Format_RGB888);
    rgb.fill(Qt::white);
    { QPainter painter(&rgb); painter.drawImage(0, 0, composition); }
    cv::Mat src(rgb.height(), rgb.width(), CV_8UC3, rgb.bits(), rgb.bytesPerLine());
    cv::Mat gray;
    if (o.preset == ImagePreset::Pencil) {
        cv::Mat bgr, color;
        cv::cvtColor(src, bgr, cv::COLOR_RGB2BGR);
        cv::pencilSketch(bgr, gray, color, 20.f + o.detail, .07f, .1f);
    } else {
        cv::cvtColor(src, gray, cv::COLOR_RGB2GRAY);
    }
    cv::resize(gray, gray, cv::Size(320, 120), 0, 0, cv::INTER_AREA);
    if (o.denoise > 0 && o.preset != ImagePreset::Dither) {
        cv::Mat smooth;
        cv::bilateralFilter(gray, smooth, 5, 15. * o.denoise, 3.);
        gray = smooth;
    }
    cv::Mat binary;
    if (o.preset == ImagePreset::LineArt) {
        cv::adaptiveThreshold(gray, binary, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                              cv::THRESH_BINARY, 15, 2 + (100 - o.detail) / 10.);
        // Adaptive threshold alone erases uniformly dark regions.
        binary.setTo(0, gray < 32);
    } else if (o.preset == ImagePreset::Dither) {
        QImage input(gray.data, gray.cols, gray.rows, int(gray.step), QImage::Format_Grayscale8);
        QImage mono = input.convertToFormat(QImage::Format_Mono, Qt::DiffuseDither | Qt::PreferDither)
                          .convertToFormat(QImage::Format_Grayscale8);
        if (o.invert) mono.invertPixels();
        return mono;
    } else {
        cv::threshold(gray, binary, o.threshold, 255, cv::THRESH_BINARY);
    }
    if (o.denoise > 0 && (o.preset == ImagePreset::LineArt || o.preset == ImagePreset::Pencil)) {
        cv::Mat foreground, labels, stats, centers;
        cv::bitwise_not(binary, foreground);
        const int count = cv::connectedComponentsWithStats(foreground, labels, stats, centers, 8);
        for (int label = 1; label < count; ++label)
            if (stats.at<int>(label, cv::CC_STAT_AREA) <= o.denoise)
                binary.setTo(255, labels == label);
    }
    if (o.thickness > 1)
        cv::erode(binary, binary, cv::getStructuringElement(cv::MORPH_ELLIPSE,
                  cv::Size(o.thickness, o.thickness)));
    if (o.invert) cv::bitwise_not(binary, binary);
    return QImage(binary.data, 320, 120, int(binary.step), QImage::Format_Grayscale8).copy();
}
