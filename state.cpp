#include "state.h"

std::chrono::milliseconds::rep State::CurrentTime()
{
    auto now = std::chrono::system_clock::now().time_since_epoch();

    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

void State::Update()
{
    auto currentTime = State::CurrentTime();

    auto diff = currentTime - _lastTime;

    UpdateByDiff(diff);
}

long State::MsToSteps(
    std::chrono::milliseconds::rep time)
{
    // 1 beat = 1000 in timesteps
    return ((time / 1000.0f) / (60.0f / _bpm)) * 4000;
}

void State::UpdateByDiff(
    std::chrono::milliseconds::rep diff)
{
    if (IsPlaying())
    {
        _lastTime = _cursor;
        _cursor += MsToSteps(diff);
    }
}

void State::StartPlaying()
{
    _playing = true;
    _lastTime = CurrentTime();
}

void State::Pause()
{
    _playing = false;
}

void State::TogglePlaying()
{
    if (IsPlaying())
    {
        Pause();
    }
    else
    {
        StartPlaying();
    }
}

void State::StopPlaying()
{
    if (_playing)
    {
        _playing = false;

        KillAllNotes();
    }
    else
    {
        _cursor = 0;
    }
}

bool State::IsPlaying() const
{
    return _playing;
}

void State::StartRecording()
{
    _recording = true;
    StartPlaying();
}

void State::ToggleRecording()
{
    if (IsRecording())
    {
        StopRecording();
    }
    else
    {
        StartRecording();
    }
}

void State::StopRecording()
{
    _recording = false;
}

bool State::IsRecording() const
{
    return _recording;
}
