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
    auto bps = 60.0f / _bpm;

    return ((time / 1000.0f) / bps) * 4000;
}

std::chrono::milliseconds::rep State::StepsToMs(
    long time)
{
    float bps = 60.0f / _bpm;

    return ((time / 4000.0f) * bps) * 1000;
}

void State::OpenRegion(
    std::tuple<uint32_t, std::chrono::milliseconds::rep> region)
{
    this->ui._activeCenterScreen = 1;
    this->_cursor = std::get<std::chrono::milliseconds::rep>(region);
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

void State::SetCursorAtStep(
    int step)
{
    _cursor = step * 1000;
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

    _cursor = 0;
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

#ifdef TEST_YOUR_CODE

#include <cassert>
#include <iostream>

void State::Tests()
{
    auto sut = State();
    auto startMs = 12345;
    auto tmp = sut.MsToSteps(startMs);
    auto endSteps = sut.StepsToMs(tmp);

    if (endSteps != startMs)
    {
        std::cout << endSteps << "!=" << startMs << std::endl;
    }
}
#endif
