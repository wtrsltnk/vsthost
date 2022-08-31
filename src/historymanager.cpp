#include "historymanager.h"

void HistoryEntry::Undo(
    ITracksManager *tracksManager)
{
    tracksManager->SetTracks(this->_tracks);

    for (auto &track : tracksManager->GetTracks())
    {
        track.UploadInstrumentSettings();
    }
}

void HistoryEntry::Redo(
    ITracksManager *tracksManager)
{
    tracksManager->SetTracks(this->_tracksUnDone);

    for (auto &track : tracksManager->GetTracks())
    {
        track.UploadInstrumentSettings();
    }
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

bool HistoryManager::HasUndo(
    std::string &titleOfUndoAction)
{
    titleOfUndoAction = _currentEntryInHistoryTrack->_title;

    return _currentEntryInHistoryTrack != &_firstEntryInHistoryTrack;
}

void HistoryManager::Undo()
{
    std::string title;

    if (!HasUndo(title))
    {
        return;
    }

    _currentEntryInHistoryTrack->_tracksUnDone = _tracks->GetTracks();
    _currentEntryInHistoryTrack->Undo(_tracks);
    _currentEntryInHistoryTrack = _currentEntryInHistoryTrack->_prevEntry;
}

bool HistoryManager::HasRedo(
    std::string &titleOfRedoAction)
{
    if (_currentEntryInHistoryTrack->_nextEntry != nullptr)
    {
        titleOfRedoAction = _currentEntryInHistoryTrack->_nextEntry->_title;
    }
    else
    {
        titleOfRedoAction = "";
    }

    return _currentEntryInHistoryTrack != nullptr && _currentEntryInHistoryTrack->_nextEntry != nullptr;
}

void HistoryManager::Redo()
{
    std::string title;

    if (!HasRedo(title))
    {
        return;
    }

    _currentEntryInHistoryTrack = _currentEntryInHistoryTrack->_nextEntry;
    _currentEntryInHistoryTrack->Redo(_tracks);
}
