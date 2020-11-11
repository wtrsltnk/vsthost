#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include "itracksmanager.h"
#include "track.h"

class HistoryEntry
{
public:
    const char *_title;
    std::vector<Track> _tracks;
    std::vector<Track> _tracksUnDone;

    HistoryEntry *_prevEntry = nullptr;
    HistoryEntry *_nextEntry = nullptr;

    void ApplyState(
        ITracksManager *tracksManager);
};

class HistoryManager
{
public:
    HistoryManager();

    ~HistoryManager();

    void Cleanup(
        HistoryEntry *from);

    void SetTracksManager(
        ITracksManager *tracksManager);

    void SetInitialState(
        const char *title,
        const std::vector<Track> &tracks);

    void AddEntry(
        const char *title);

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
