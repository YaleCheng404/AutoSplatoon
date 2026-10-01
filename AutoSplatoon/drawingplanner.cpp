#include "drawingplanner.h"
#include <stdexcept>

DrawPlan makeDrawPlan(const QImage& image, int row, int column,
                     int intervalMs, bool saveWhenFinished)
{
    if (image.isNull() || row < 0 || row >= image.height() || column < 0 ||
        column >= image.width() || intervalMs < 1)
        throw std::invalid_argument("Invalid drawing image, cursor or timing");
    DrawPlan plan;
    auto tap = [&](quint64 action, int y, int x) {
        plan.frames.append({action, intervalMs, y, x});
        plan.frames.append({InputEmulator::NO_INPUT, intervalMs, y, x});
        plan.durationMs += 2 * intervalMs;
    };
    for (int y = 0; y < image.height(); ++y)
        for (int x = 0; x < image.width(); ++x)
            if (qGray(image.pixel(x, y)) < 128) {
                ++plan.blackPixels;
                if (y < row || (y == row && ((row % 2 == 0 && x < column) ||
                                             (row % 2 != 0 && x > column))))
                    ++plan.skippedPixels;
            }
    // Full-width serpentine scanning avoids the old ambiguous zero-distance
    // sentinel and next-row bounds bugs. Cursor starts at the user's position.
    if (plan.blackPixels > plan.skippedPixels) {
        for (int y = row; y < image.height(); ++y) {
            const int step = y % 2 == 0 ? 1 : -1;
            const int edge = step == 1 ? image.width() - 1 : 0;
            for (;;) {
                if (qGray(image.pixel(column, y)) < 128)
                    tap(InputEmulator::BTN_A, y, column);
                if (column == edge) break;
                tap(step == 1 ? InputEmulator::DPAD_R : InputEmulator::DPAD_L, y, column);
                column += step;
            }
            if (y + 1 < image.height()) tap(InputEmulator::DPAD_D, y, column);
        }
    }
    if (saveWhenFinished) tap(InputEmulator::BTN_MINUS,
        plan.blackPixels > plan.skippedPixels ? image.height() - 1 : row, column);
    return plan;
}
