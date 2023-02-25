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
        spdlog::error("missing instrument");
        return;
    }

    if (length < 100)
    {
         // make sure the length is atleast 100, otherwise it might not produce any preview note
        length = 100;
    }

    instument->Lock();
    if (instument->Plugin() == nullptr)
    {
        spdlog::error("missing plugin");
        instument->Unlock();

        return;
    }

    if (_activePreviewNote > 0)
    {
        spdlog::info("note still active: {}", _activePreviewNote);
        instument->Plugin()->sendMidiNote(1, _activePreviewNote, false, 0);
    }

    spdlog::info("sending note {}", note);
    instument->Plugin()->sendMidiNote(1, note, true, velocity);

    _activePreviewNote = note;
    _activePreviewNoteTimeLeft = _state->StepsToMs(length);

    instument->Unlock();
}

void NotePreviewService::HandleMidiEventsInTimeRange(
    std::chrono::milliseconds::rep diff)
{
    if (_activePreviewNote <= 0)
    {
        return;
    }

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

    if (_activePreviewNoteTimeLeft < diff && _activePreviewNote > 0)
    {
        spdlog::info("de-activating note: {}", _activePreviewNote);
        instument->Plugin()->sendMidiNote(1, _activePreviewNote, false, 0);
        _activePreviewNote = 0;
    }
    else
    {
        _activePreviewNoteTimeLeft -= diff;
    }

    instument->Unlock();
}
