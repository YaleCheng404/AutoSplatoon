#pragma once
#include "inputemulator.h"
#include <QImage>
#include <QVector>

// Adapted from zhougz520/AutoSplatoon: explicit action frames, shared ETA.
struct ActionFrame {
    quint64 action = InputEmulator::NO_INPUT;
    int durationMs = 70;
    int row = 0;
    int column = 0;
};
struct DrawPlan {
    QVector<ActionFrame> frames;
    qint64 durationMs = 0;
    int blackPixels = 0;
    int skippedPixels = 0;
};
DrawPlan makeDrawPlan(const QImage& image, int row, int column,
                      int intervalMs, bool saveWhenFinished);
