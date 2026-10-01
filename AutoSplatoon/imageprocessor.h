#pragma once
#include <QImage>

enum class ImagePreset { LineArt, Pencil, Threshold, Dither };
struct ProcessOptions {
    ImagePreset preset = ImagePreset::LineArt;
    int detail = 50;
    int denoise = 1;
    int thickness = 1;
    int threshold = 160;
    bool invert = false;
};
QImage processImage(const QImage& composition, const ProcessOptions& options);
