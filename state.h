#ifndef STATE_H
#define STATE_H

#include <chrono>

extern void KillAllNotes();

class State
{
    bool _playing = false;

public:
    static std::chrono::milliseconds::rep CurrentTime();
    void Update();
    void UpdateByDiff(std::chrono::milliseconds::rep diff);
    void StartPlaying();
    void Pause();
    void TogglePlaying();
    void StopPlaying();
    bool IsPlaying() const;
    void StartRecording();
    void ToggleRecording();
    void StopRecording();
    bool IsRecording() const;

    long MsToSteps(std::chrono::milliseconds::rep diff);

    int _width;
    int _height;
    std::chrono::milliseconds::rep _lastTime;
    bool _recording = false;
    bool _loop;
    unsigned int _bpm = 48;
    std::chrono::milliseconds::rep _cursor = 0;
};

#endif // STATE_H
