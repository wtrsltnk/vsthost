#include "notepreviewservice.h"

#include "instrument.h"
#include "track.h"
#include <iostream>
#include <spdlog/spdlog.h>

NotePreviewService::NotePreviewService() = default;

void NotePreviewService::SetState(
    State *state)
{
    _state = state;
}

void NotePreviewService::SetTracksManager(
    ITracksManager *tracks)
{
    _tracks = tracks;
}

VstPlugin *NotePreviewService::GetActivePlugin()
{
    auto trackId = _tracks->GetActiveTrackId();
    if (trackId == Track::Null)
    {
        spdlog::debug("GetActiveTrack is null");
        return nullptr;
    }

    auto instrument = _tracks->GetInstrument(trackId);
    if (instrument == nullptr)
    {
        spdlog::debug("GetInstrument is null");
        return nullptr;
    }

    auto plugin = instrument->Plugin();
    if (plugin == nullptr)
    {
        spdlog::debug("Plugin is null");
        return nullptr;
    }

    return plugin;
}

void NotePreviewService::PreviewNote(
    uint32_t note,
    uint32_t velocity,
    uint32_t length)
{
    auto plugin = GetActivePlugin();

    if (plugin == nullptr)
    {
        return;
    }

    if (_activePreviewNote > 0)
    {
        plugin->sendMidiNote(1, _activePreviewNote, false, 0);
    }

    plugin->sendMidiNote(1, note, true, velocity);

    _activePreviewNote = note;
    _activePreviewNoteTimeLeft = _state->StepsToMs(length);
}

void NotePreviewService::HandleMidiEventsInTimeRange(
    std::chrono::milliseconds::rep diff)
{
    auto plugin = GetActivePlugin();

    if (plugin == nullptr)
    {
        return;
    }

    if (_activePreviewNoteTimeLeft < diff)
    {
        plugin->sendMidiNote(1, _activePreviewNote, false, 0);
    }
    else
    {
        _activePreviewNoteTimeLeft -= diff;
    }
}
