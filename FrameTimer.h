// Copyright 2000-2021 Mark H. P. Lord

#ifndef PRIME_FRAMETIMER_H
#define PRIME_FRAMETIMER_H

#include "Config.h"
#include <math.h>

namespace Prime {

/// Computes the number of frames an animation should run, trying to keep it in sync with vsync if locked, or
/// close to it if not, running extra frames as necessary and "borrowing" future frames to ensure at least one
/// update happens per rendered frame. Also supports variable frame rates, so you can use this class's pause and
/// lag logic.
class FrameTimer {
public:
    FrameTimer() { construct(); }

    /// Use framesPerSecond zero for a variable frame rate.
    explicit FrameTimer(int framesPerSecond) PRIME_NOEXCEPT
    {
        construct();
        setFramesPerSecond(framesPerSecond);
    }

    /// Use framesPerSecond zero for a variable frame rate.
    void setFramesPerSecond(int framesPerSecond) PRIME_NOEXCEPT
    {
        _framesPerSecond = framesPerSecond;
        _frameInterval = framesPerSecond ? 1.0 / framesPerSecond : 0;
    }

    int getFramesPerSecond() const PRIME_NOEXCEPT { return _framesPerSecond; }

    bool isVariable() const PRIME_NOEXCEPT { return _framesPerSecond == 0; }

    /// 1.0 / getFramesPerSecond(), or zero if using a variable frame rate.
    double getFrameInterval() const PRIME_NOEXCEPT { return _frameInterval; }

    /// If the game experiences a significant lag, resulting in > timeBeforePause seconds of time elapsing
    /// without a frame, then retroactively pause the game for that time.
    void setMaxTimeBeforePause(double timeBeforePause) PRIME_NOEXCEPT { _timeBeforePause = timeBeforePause; }

    /// This method can only be called if we have a fixed frame rate (i.e., isVariable() returns false).
    int updateTimeAndGetFramesToRun(double time) PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(!isVariable());

        if (_firstFrame) {
            _firstFrame = false;
            _firstFrameTime = time;

            return 0;
        }

        int64_t totalFramesShouldBe = (int64_t)floor((time - _firstFrameTime) / _frameInterval);

        int64_t frameDifference = totalFramesShouldBe - _totalFrames;
        if (frameDifference < 0) {
            frameDifference = 0;
        }

        if (_timeBeforePause > 0 && (double)frameDifference / _framesPerSecond > _timeBeforePause) {
            resync(time);
            return updateTimeAndGetFramesToRun(time);
        }

        int frames = (int)frameDifference;

        if (!frames) {
            if (_borrowed < _framesPerSecond / 2) {
                ++_borrowed;
                ++frames;
            }
        } else if (frames > 1) {
            if (_borrowed > -1) {
                --_borrowed;
                --frames;
            }
        }

        // if (frames != 1) {
        //  Log::getGlobal()->note("%d frames", frames);
        // }

        _totalFrames += frames;
        _gameTime = _firstFrameTime + _totalFrames * _framesPerSecond;
        _deltaTime = frames * _frameInterval;

        return frames;
    }

    /// This method can only be called if we have a variable frame rate (i.e., isVariable() returns true).
    void updateTime(double time) PRIME_NOEXCEPT
    {
        PRIME_DEBUG_ASSERT(isVariable());

        if (_firstFrame) {
            _firstFrame = false;
            _firstFrameTime = time;
        }

        double newGameTime = time - _firstFrameTime;

        double newDeltaTime = newGameTime - _gameTime;

        if (newDeltaTime > _timeBeforePause) {
            _firstFrameTime = time - _gameTime;

            newGameTime = time - _firstFrameTime;
            newDeltaTime = newGameTime - _gameTime;

            PRIME_ASSERT(fabs(newDeltaTime) < 0.001);
        }

        _gameTime = newGameTime;
        _deltaTime = newDeltaTime;
    }

    double getGameTime() const PRIME_NOEXCEPT { return _gameTime; }

    double getFirstFrameTime() const PRIME_NOEXCEPT { return _firstFrame; }

    int64_t getTotalFrames() const PRIME_NOEXCEPT { return _totalFrames; }

    double getDeltaTime() const PRIME_NOEXCEPT { return _deltaTime; }

    void reset() PRIME_NOEXCEPT { construct(); }

private:
    void construct() PRIME_NOEXCEPT
    {
        _framesPerSecond = 0;
        _frameInterval = 0;
        _timeBeforePause = -1;
        _gameTime = 0;
        _firstFrame = true;
        _totalFrames = 0;
        _borrowed = 0;
    }

    void resync(double time) PRIME_NOEXCEPT
    {
        _firstFrameTime = time - ((double)(_totalFrames + 1) / _framesPerSecond);
    }

    int _framesPerSecond;
    double _frameInterval;

    double _timeBeforePause;

    double _gameTime;
    bool _firstFrame;
    double _firstFrameTime;

    int64_t _totalFrames;

    double _deltaTime;

    int _borrowed;
};
}

#endif
