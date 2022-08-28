#ifndef NOTEPREVIEWSERVICE_H
#define NOTEPREVIEWSERVICE_H

#include "itracksmanager.h"
#include "state.h"
#include <chrono>
#include <memory>

class NotePreviewService
{
public:
    NotePreviewService();

    void SetState(
        State *state);

    void SetTracksManager(
        ITracksManager *tracks);

    void PreviewNote(
        uint32_t note,
        uint32_t velocity,
        uint32_t length);

    void HandleMidiEventsInTimeRange(
        std::chrono::milliseconds::rep diff);

private:
    State *_state = nullptr;
    ITracksManager *_tracks = nullptr;
    uint32_t _activePreviewNote;
    std::chrono::milliseconds::rep _activePreviewNoteTimeLeft;

    std::shared_ptr<Instrument> GetActiveInstrument();
};

extern NotePreviewService _notePreviewService;

#endif // NOTEPREVIEWSERVICE_H
