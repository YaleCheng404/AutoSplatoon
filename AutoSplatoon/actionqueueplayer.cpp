#include "actionqueueplayer.h"

#include <QtGlobal>

ActionQueuePlayer::ActionQueuePlayer(QObject* parent)
    : QObject(parent)
{
    _timer.setSingleShot(true);
    _timer.setTimerType(Qt::PreciseTimer);
    connect(&_timer, &QTimer::timeout, this, &ActionQueuePlayer::playNextFrame);
}

bool ActionQueuePlayer::isRunning() const
{
    return _running;
}

bool ActionQueuePlayer::isPaused() const
{
    return _paused;
}

int ActionQueuePlayer::currentIndex() const
{
    return _currentIndex;
}

void ActionQueuePlayer::start(const DrawPlan& plan)
{
    _timer.stop();
    _generation++;
    resetState();
    _plan = plan;
    _running = true;
    _elapsedTimer.restart();
    playNextFrame();
}

void ActionQueuePlayer::pause()
{
    if (!_running || _paused) {
        return;
    }

    if (_frameActive) {
        _pauseRequested = true;
        return;
    }

    _paused = true;
    _skipNextTimingCheck = true;
    emit sendAction(InputEmulator::NO_INPUT);
}

void ActionQueuePlayer::resume()
{
    if (!_running || !_paused) {
        return;
    }

    _paused = false;
    _pauseRequested = false;
    _skipNextTimingCheck = true;
    _expectedNextTimeoutMs = 0;
    _elapsedTimer.restart();
    playNextFrame();
}

void ActionQueuePlayer::stop()
{
    const bool wasActive = _running || _paused || _timer.isActive();
    _generation++;
    _timer.stop();
    resetState();
    emit sendAction(InputEmulator::NO_INPUT);
    if (wasActive) {
        emit stopped();
    }
}

void ActionQueuePlayer::playNextFrame()
{
    if (!_running || _paused) {
        return;
    }

    _frameActive = false;

    if (_currentIndex > 0) {
        if (_skipNextTimingCheck) {
            _skipNextTimingCheck = false;
        } else {
            const int actualElapsed = static_cast<int>(_elapsedTimer.elapsed());
            const int lateBy = actualElapsed - _expectedNextTimeoutMs;
            if (lateBy > _overrunThresholdMs) {
                _paused = true;
                _skipNextTimingCheck = true;
                emit sendAction(InputEmulator::NO_INPUT);
                emit timingOverrun(_currentIndex, lateBy);
                return;
            }
            _expectedNextTimeoutMs = actualElapsed;
        }
    }

    if (_pauseRequested) {
        _pauseRequested = false;
        _paused = true;
        _skipNextTimingCheck = true;
        emit sendAction(InputEmulator::NO_INPUT);
        return;
    }

    if (_currentIndex >= _plan.frames.size()) {
        _running = false;
        emit sendAction(InputEmulator::NO_INPUT);
        emit finished();
        return;
    }

    const ActionFrame frame = _plan.frames.at(_currentIndex);
    const int generation = _generation;
    _frameActive = true;
    emit sendAction(frame.action);
    if (generation != _generation || !_running) {
        return;
    }
    emit progressChanged(_currentIndex + 1,
                         _plan.frames.size(),
                         frame.row,
                         frame.column);
    if (generation != _generation || !_running) {
        return;
    }
    _currentIndex++;
    _expectedNextTimeoutMs += qMax(0, frame.durationMs);
    _timer.start(qMax(0, frame.durationMs));
}

void ActionQueuePlayer::resetState()
{
    _plan = DrawPlan();
    _currentIndex = 0;
    _expectedNextTimeoutMs = 0;
    _running = false;
    _paused = false;
    _pauseRequested = false;
    _frameActive = false;
    _skipNextTimingCheck = false;
}
