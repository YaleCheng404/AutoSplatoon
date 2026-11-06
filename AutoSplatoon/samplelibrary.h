#pragma once

#include <QImage>
#include <QString>
#include <array>

struct SampleArt {
    QString id;
    QString title;
    QString description;
    QString credit;
    QImage image;
};

const std::array<SampleArt, 3>& sampleLibrary();
