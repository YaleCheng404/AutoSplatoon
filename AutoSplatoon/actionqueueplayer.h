#ifndef ACTIONQUEUEPLAYER_H
#define ACTIONQUEUEPLAYER_H

#include "drawingplanner.h"

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

class ActionQueuePlayer : public QObject {
    Q_OBJECT

public:
    explicit ActionQueuePlayer(QObject* parent = nullptr);

    bool isRunning() const;
    bool isPaused() const;
    int currentIndex() const;

public slots:
    void start(const DrawPlan& plan);
    void pause();
    void resume();
    void stop();

signals:
    void sendAction(quint64 action);
    void progressChanged(int currentFrame, int totalFrames, int row, int column);
    void timingOverrun(int currentFrame, int lateByMs);
    void finished();
    void stopped();

private slots:
    void playNextFrame();

private:
    void resetState();

    DrawPlan _plan;
    QTimer _timer;
    QElapsedTimer _elapsedTimer;
    int _currentIndex = 0;
    int _expectedNextTimeoutMs = 0;
    int _overrunThresholdMs = 25;
    int _generation = 0;
    bool _running = false;
    bool _paused = false;
    bool _pauseRequested = false;
    bool _frameActive = false;
    bool _skipNextTimingCheck = false;
};

#endif // ACTIONQUEUEPLAYER_H
