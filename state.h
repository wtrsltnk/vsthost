#ifndef STATE_H
#define STATE_H

#include "historymanager.h"
#include <chrono>

extern void KillAllNotes();

class State
{
public:
    static std::chrono::milliseconds::rep CurrentTime();

    void Update();

    void UpdateByDiff(
        std::chrono::milliseconds::rep diff);

    void SetCursorAtStep(
        int step);

    void StartPlaying();

    void Pause();

    void TogglePlaying();

    void StopPlaying();

    bool IsPlaying() const;

    void StartRecording();

    void ToggleRecording();

    void StopRecording();

    bool IsRecording() const;

    long MsToSteps(
        std::chrono::milliseconds::rep diff);

    std::chrono::milliseconds::rep StepsToMs(
        long diff);

    struct
    {
        int _width;
        int _height;
        int _activeCenterScreen = 0;
    } ui;

    std::chrono::milliseconds::rep _lastTime;
    bool _recording = false;
    bool _loop;
    uint32_t _bpm = 48;
    std::chrono::milliseconds::rep _cursor = 0;
    HistoryManager _historyManager;

private:
    bool _playing = false;

public:
#ifdef TEST_YOUR_CODE
    static void Tests();
#endif
};

#endif // STATE_H
