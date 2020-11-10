#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include "itracksmanager.h"
#include "track.h"

class HistoryEntry
{
public:
    const char *_title;
    Track *_track;
    Track::RegionCollection _regions;
    Track::RegionCollection _regionsUnDone;

    HistoryEntry *_prevEntry = nullptr;
    HistoryEntry *_nextEntry = nullptr;

    void ApplyState();
};

class HistoryManager
{
public:
    HistoryManager();

    ~HistoryManager();

    void Cleanup(
        HistoryEntry *from);

    void SetTracksManager(
        ITracksManager *tracks);

    void SetInitialState(
        const char *title,
        Track *track,
        const Track::RegionCollection &regions);

    void AddEntry(
        const char *title,
        Track *track,
        const Track::RegionCollection &regions);

    bool HasUndo();

    void Undo();

    bool HasRedo();

    void Redo();

    const HistoryEntry *FirstEntryInHistoryTrack() const { return &_firstEntryInHistoryTrack; }

    const HistoryEntry *CurrentEntryInHistoryTrack() const { return _currentEntryInHistoryTrack; }

private:
    ITracksManager *_tracks = nullptr;

    HistoryEntry _firstEntryInHistoryTrack;
    HistoryEntry *_currentEntryInHistoryTrack = nullptr;
};

#endif // HISTORYMANAGER_H
