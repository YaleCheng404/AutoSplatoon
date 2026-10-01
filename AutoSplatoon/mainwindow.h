#pragma once
#include "actionqueueplayer.h"
#include "canvasview.h"
#include "imageprocessor.h"
#include "serialconnection.h"
#include <QMainWindow>
#include <QFutureWatcher>
#include <QElapsedTimer>
class QComboBox;
class QSpinBox;
class QCheckBox;
class QPushButton;
class QLabel;
class QProgressBar;
class QScrollArea;

struct ProcessResult { QImage image; QString error; };
class AutoSplatoon : public QMainWindow {
    Q_OBJECT
public:
    explicit AutoSplatoon(QWidget* parent = nullptr);
    ~AutoSplatoon() override;
    void loadImage(const QImage& image);
    QImage outputImage() const { return output; }
private:
    void refreshPorts();
    void importImage();
    void requestProcessing();
    void runProcessing();
    void updatePlan();
    void updateButtons();
    void applyTheme();
    void showGuide();
    void manualControl();
    QComboBox *ports, *preset, *theme;
    QSpinBox *detail, *denoise, *thickness, *threshold, *interval, *startRow, *startColumn;
    QCheckBox *inverse, *saveWhenFinished;
    QPushButton *connectButton, *flashButton, *startButton, *pauseButton, *stopButton, *manualButton, *exportButton;
    QLabel *statistics, *state, *elapsed;
    QProgressBar* progress;
    QScrollArea* parameters;
    CanvasView *compositionView, *resultView;
    SerialController serial;
    ActionQueuePlayer player;
    QTimer debounce, elapsedTimer;
    QElapsedTimer taskClock;
    QFutureWatcher<ProcessResult> worker;
    QImage output;
    DrawPlan plan;
    quint64 generation = 0, workingGeneration = 0;
    bool hasImage = false, processing = false;
};
