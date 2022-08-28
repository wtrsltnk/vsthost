#ifndef ARPEGGIOPREVIEWSERVICE_H
#define ARPEGGIOPREVIEWSERVICE_H

#include "arpeggiator.h"
#include "itracksmanager.h"
#include "state.h"

#include <chrono>

class ArpeggiatorPreviewService
{
public:
    ArpeggiatorPreviewService();

    void SetState(
        State *state);

    void SetTracksManager(
        ITracksManager *tracks);

    void TriggerNote(
        uint32_t note,
        uint32_t velocity);

    void SendMidiNotesInTimeRange(
        std::chrono::milliseconds::rep diff);

    void SetEnabled(
        bool enabled);

    bool Enabled = false;

    Arpeggiator CurrentArpeggiator;


    std::chrono::milliseconds::rep _timeSinceLastNote = 0;
    std::chrono::milliseconds::rep _localCursor = 0;
    size_t _cursorInNotes = 0;
private:
    State *_state = nullptr;
    ITracksManager *_tracks = nullptr;
};

extern ArpeggiatorPreviewService _arpeggiatorPreviewService;

#endif // ARPEGGIOPREVIEWSERVICE_H
