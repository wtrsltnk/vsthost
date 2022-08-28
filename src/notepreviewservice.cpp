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

std::shared_ptr<Instrument> NotePreviewService::GetActiveInstrument()
{
    auto trackId = _tracks->GetActiveTrackId();
    if (trackId == Track::Null)
    {
        return nullptr;
    }

    auto instrument = _tracks->GetInstrument(trackId);
    if (instrument == nullptr)
    {
        return nullptr;
    }

    return instrument;
}

void NotePreviewService::PreviewNote(
    uint32_t note,
    uint32_t velocity,
    uint32_t length)
{
    auto instument = GetActiveInstrument();

    if (instument == nullptr)
    {
        return;
    }

    instument->Lock();
    if (instument->Plugin() == nullptr)
    {
        instument->Unlock();

        return;
    }

    if (_activePreviewNote > 0)
    {
        instument->Plugin()->sendMidiNote(1, _activePreviewNote, false, 0);
    }

    instument->Plugin()->sendMidiNote(1, note, true, velocity);

    _activePreviewNote = note;
    _activePreviewNoteTimeLeft = _state->StepsToMs(length);

    instument->Unlock();
}

void NotePreviewService::HandleMidiEventsInTimeRange(
    std::chrono::milliseconds::rep diff)
{
    auto instument = GetActiveInstrument();

    if (instument == nullptr)
    {
        return;
    }

    instument->Lock();

    if (instument->Plugin() == nullptr)
    {
        instument->Unlock();
        return;
    }

    if (_activePreviewNoteTimeLeft < diff)
    {
        instument->Plugin()->sendMidiNote(1, _activePreviewNote, false, 0);
    }
    else
    {
        _activePreviewNoteTimeLeft -= diff;
    }

    instument->Unlock();
}
