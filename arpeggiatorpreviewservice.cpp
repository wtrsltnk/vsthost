#include "arpeggiatorpreviewservice.h"

#include "instrument.h"
#include "track.h"
#include "vstplugin.h"
#include <iostream>

ArpeggiatorPreviewService::ArpeggiatorPreviewService() = default;

void ArpeggiatorPreviewService::SetState(
    State *state)
{
    _state = state;
}

void ArpeggiatorPreviewService::SetTracksManager(
    ITracksManager *tracks)
{
    _tracks = tracks;
}

void ArpeggiatorPreviewService::TriggerNote(
    uint32_t note,
    uint32_t velocity)
{
    CurrentArpeggiator.Notes.push_back(ArpeggiatorNote({velocity, note}));
}

void ArpeggiatorPreviewService::SetEnabled(
    bool enabled)
{
    Enabled = enabled;
    auto step = _state->MsToSteps(1000.0 / (int)CurrentArpeggiator.Rate);

    _localCursor = 0;
    _timeSinceLastNote = -step;
    _cursorInNotes = -1;

    if (!Enabled)
    {
        KillAllNotes();
    }
}

void ArpeggiatorPreviewService::SendMidiNotesInTimeRange(
    std::chrono::milliseconds::rep diff)
{
    if (!Enabled)
    {
        return;
    }

    if (CurrentArpeggiator.Notes.empty())
    {
        std::cout << "Notes is empty\n";
        return;
    }

    if (_state == nullptr)
    {
        std::cout << "_state is null\n";
        return;
    }

    if (_tracks == nullptr)
    {
        std::cout << "_tracks is null\n";
        return;
    }

    auto track = _tracks->GetActiveTrackId();
    if (track == Track::Null)
    {
        std::cout << "GetActiveTrack is null\n";
        return;
    }

    auto instrument = _tracks->GetInstrument(track);
    if (instrument == nullptr)
    {
        std::cout << "GetInstrument is null\n";
        return;
    }

    auto plugin = instrument->Plugin();
    if (plugin == nullptr)
    {
        std::cout << "Plugin is null\n";
        return;
    }

    auto step = _state->MsToSteps(1000.0 / (int)CurrentArpeggiator.Rate);

    auto note = CurrentArpeggiator.Notes[_cursorInNotes % CurrentArpeggiator.Notes.size()];

    if (_timeSinceLastNote + step < _localCursor)
    {
        plugin->sendMidiNote(1, note.Note, false, 0);

        // lets do step
        _cursorInNotes = (_cursorInNotes + 1) % CurrentArpeggiator.Notes.size();

        note = CurrentArpeggiator.Notes[_cursorInNotes];

        plugin->sendMidiNote(1, note.Note, true, note.Velocity);

        _timeSinceLastNote += step;
    }

    // At the end we progress the local cursor
    _localCursor += diff;
}
