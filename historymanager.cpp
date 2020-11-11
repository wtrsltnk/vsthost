#include "historymanager.h"

void HistoryEntry::ApplyState(
    ITracksManager *tracksManager)
{
    tracksManager->SetTracks(this->_tracks);
}

HistoryManager::HistoryManager()
{
    _firstEntryInHistoryTrack._title = "New Song";
    _currentEntryInHistoryTrack = &_firstEntryInHistoryTrack;
}

HistoryManager::~HistoryManager()
{
    Cleanup(_firstEntryInHistoryTrack._nextEntry);

    _currentEntryInHistoryTrack = nullptr;
}

void HistoryManager::SetTracksManager(
    ITracksManager *tracksManager)
{
    _tracks = tracksManager;
}

void HistoryManager::Cleanup(
    HistoryEntry *from)
{
    while (from != nullptr)
    {
        auto tmp = from;
        from = tmp->_nextEntry;

        delete tmp;
    }
}

void HistoryManager::SetInitialState(
    const char *title,
    const std::vector<Track> &tracks)
{
    _firstEntryInHistoryTrack._title = title;
    _firstEntryInHistoryTrack._tracks = tracks;
}

void HistoryManager::AddEntry(
    const char *title)
{
    if (_currentEntryInHistoryTrack != nullptr)
    {
        Cleanup(_currentEntryInHistoryTrack->_nextEntry);
    }

    auto entry = new HistoryEntry();

    entry->_title = title;
    entry->_tracks = _tracks->GetTracks();
    entry->_prevEntry = _currentEntryInHistoryTrack;

    if (_currentEntryInHistoryTrack != nullptr)
    {
        _currentEntryInHistoryTrack->_nextEntry = entry;
    }

    _currentEntryInHistoryTrack = entry;
}

bool HistoryManager::HasUndo()
{
    return _currentEntryInHistoryTrack != &_firstEntryInHistoryTrack;
}

void HistoryManager::Undo()
{
    if (!HasUndo())
    {
        return;
    }

    _currentEntryInHistoryTrack->_tracksUnDone = _tracks->GetTracks();
    _currentEntryInHistoryTrack->ApplyState(_tracks);
    _currentEntryInHistoryTrack = _currentEntryInHistoryTrack->_prevEntry;
}

bool HistoryManager::HasRedo()
{
    return _currentEntryInHistoryTrack != nullptr && _currentEntryInHistoryTrack->_nextEntry != nullptr;
}

void HistoryManager::Redo()
{
    if (!HasRedo())
    {
        return;
    }

    _currentEntryInHistoryTrack = _currentEntryInHistoryTrack->_nextEntry;
    _tracks->SetTracks(_currentEntryInHistoryTrack->_tracksUnDone);
}
