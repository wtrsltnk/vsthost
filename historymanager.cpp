#include "historymanager.h"

void HistoryEntry::ApplyState()
{
    this->_track->SetRegions(this->_regions);
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
    Track *track,
    const Track::RegionCollection &regions)
{
    _firstEntryInHistoryTrack._title = title;
    _firstEntryInHistoryTrack._track = track;
    _firstEntryInHistoryTrack._regions = regions;
}

void HistoryManager::AddEntry(
    const char *title,
    Track *track,
    const Track::RegionCollection &regions)
{
    if (_currentEntryInHistoryTrack != nullptr)
    {
        Cleanup(_currentEntryInHistoryTrack->_nextEntry);
    }

    auto entry = new HistoryEntry();

    entry->_title = title;
    entry->_track = track;
    entry->_regions = regions;
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

    _currentEntryInHistoryTrack->_regionsUnDone = _currentEntryInHistoryTrack->_track->Regions();
    _currentEntryInHistoryTrack->ApplyState();
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
    _currentEntryInHistoryTrack->_track->SetRegions(_currentEntryInHistoryTrack->_regionsUnDone);
}
