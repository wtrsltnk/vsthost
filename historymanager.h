#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include "itracksmanager.h"

class HistoryEntry
{
public:
    const char *_title;
    ITrack *_track;
    ITrack::RegionCollection _regions;
    ITrack::RegionCollection _regionsUnDone;

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
        ITrack *track,
        const ITrack::RegionCollection &regions);

    void AddEntry(
        const char *title,
        ITrack *track,
        const ITrack::RegionCollection &regions);

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
